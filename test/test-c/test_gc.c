/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <signal.h>
#include <sys/wait.h>
#include "gc.h"
#include "testhelp.h"

/* Node: an object with two references, freed nodes are counted */
typedef struct _Node {
    OBJECT_HEAD
    Object *left;
    Object *right;
} Node;

static int nr_freed;

static void node_mark(Object *self)
{
    Node *n = (Node *)self;
    gc_mark(n->left);
    gc_mark(n->right);
}

static void node_free(Object *self)
{
    (void)self;
    nr_freed++;
}

static TypeObject node_type = {
    .name = "Node",
    ._free = node_free,
    ._mark = node_mark,
};

/* Holder: an object referring to a values or bytes block */
typedef struct _Holder {
    OBJECT_HEAD
    void *block;
} Holder;

static void holder_mark(Object *self) { gc_mark(((Holder *)self)->block); }

static TypeObject holder_type = {
    .name = "Holder",
    ._mark = holder_mark,
};

#define NR_SLOTS 8

static TValue stack[NR_SLOTS];
static KoalaState ks = {
    .stack_base = stack,
    .stack_top = stack + NR_SLOTS,
};

static void clear_roots(void) { memset(stack, 0, sizeof(stack)); }

static void collect(void)
{
    nr_freed = 0;
    gc_collect(&ks);
}

static Node *new_node(void)
{
    Node *n = gc_alloc(sizeof(Node));
    INIT_OBJECT_HEAD(n, &node_type, 0);
    return n;
}

static Node *new_perm_node(void)
{
    Node *n = gc_alloc_perm(sizeof(Node));
    INIT_OBJECT_HEAD(n, &node_type, 0);
    return n;
}

static Holder *new_holder(void *block)
{
    Holder *h = gc_alloc(sizeof(Holder));
    INIT_OBJECT_HEAD(h, &holder_type, 0);
    h->block = block;
    return h;
}

static void test_object(void)
{
    clear_roots();
    Node *a = new_node();
    Node *b = new_node();
    Node *c = new_node();
    a->left = (Object *)b;
    new_node(); /* garbage */
    stack[0] = obj_value(a);
    stack[1] = obj_value(c);

    collect();
    test_cond("object: unreachable object is freed", nr_freed == 1);

    stack[1] = (TValue){ 0 };
    collect();
    test_cond("object: dropped root is freed", nr_freed == 1);

    collect();
    test_cond("object: reachable through _mark survives", nr_freed == 0);

    clear_roots();
    collect();
    test_cond("object: whole graph freed after roots cleared", nr_freed == 2);
}

static void test_cycle(void)
{
    clear_roots();
    Node *a = new_node();
    Node *b = new_node();
    a->left = (Object *)b;
    b->left = (Object *)a;
    a->right = (Object *)a;

    stack[0] = obj_value(a);
    collect();
    test_cond("cycle: rooted cycle survives", nr_freed == 0);

    clear_roots();
    collect();
    test_cond("cycle: unrooted cycle is freed", nr_freed == 2);
}

static void test_values(void)
{
    clear_roots();
    TValue *vals = gc_alloc_values(4);
    vals[0] = obj_value(new_node());
    vals[2] = obj_value(new_node());
    /* vals[1], vals[3] stay zeroed: not references */
    Holder *h = new_holder(vals);
    stack[0] = obj_value(h);

    collect();
    test_cond("values: referenced objects survive", nr_freed == 0);

    vals[2] = (TValue){ 0 };
    collect();
    test_cond("values: overwritten slot releases object", nr_freed == 1);

    size_t used = gc_used();
    clear_roots();
    collect();
    test_cond("values: block and its objects are freed", nr_freed == 1 && gc_used() < used);
}

static void test_bytes(void)
{
    clear_roots();
    char *bytes = gc_alloc_bytes(1000);
    memset(bytes, 0xff, 1000); /* garbage must not be taken as references */
    Holder *h = new_holder(bytes);
    stack[0] = obj_value(h);

    size_t used = gc_used();
    collect();
    test_cond("bytes: rooted block survives", gc_used() == used);

    clear_roots();
    collect();
    test_cond("bytes: unrooted block is freed", gc_used() <= used - 1000);
}

static void test_perm(void)
{
    clear_roots();
    new_perm_node();
    collect();
    test_cond("perm: unrooted perm object is never freed", nr_freed == 0);
}

static void test_age(void)
{
    clear_roots();
    Node *a = new_node();
    stack[0] = obj_value(a);
    /* age saturates below the threshold, the block stays collectable */
    for (int i = 0; i < 300; i++) gc_collect(&ks);

    clear_roots();
    collect();
    test_cond("age: old object is still freed", nr_freed == 1);
}

static int nr_hook_free, nr_hook_collect;
static size_t hook_last_freed;

static void hook_free(void *ptr, int kind, size_t size)
{
    (void)ptr;
    (void)size;
    if (kind == GC_KIND_OBJECT) nr_hook_free++;
}

static void hook_collect(const GCStats *stats)
{
    nr_hook_collect++;
    hook_last_freed = stats->last.freed;
}

static void test_hooks(void)
{
    clear_roots();
    GCHooks hooks = { .on_free = hook_free, .on_collect = hook_collect };
    gc_set_hooks(&hooks);
    new_node();
    new_node();
    size_t freed = gc_collect(&ks);
    test_cond("hooks: on_free for each freed block", nr_hook_free == 2);
    test_cond("hooks: on_collect once with last stats", nr_hook_collect == 1 && hook_last_freed == 2);
    test_cond("hooks: collect returns freed blocks", freed == 2);

    gc_set_hooks(NULL);
    new_node();
    gc_collect(&ks);
    test_cond("hooks: removed hooks are not called", nr_hook_free == 2 && nr_hook_collect == 1);
}

/* runs gc with KOALA_GC_MAX=max in a child, allocating size bytes */
static int run_with_max(const char *max, int size)
{
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0) {
        freopen("/dev/null", "w", stdout);
        setenv("KOALA_GC_MAX", max, 1);
        gc_init();
        gc_alloc_bytes(size);
        exit(0);
    }
    int status;
    waitpid(pid, &status, 0);
    return status;
}

static int exited_ok(int status) { return WIFEXITED(status) && WEXITSTATUS(status) == 0; }

static int aborted(int status) { return WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT; }

static void test_max(void)
{
    test_cond("max: alloc under limit", exited_ok(run_with_max("4K", 1024)));
    test_cond("max: alloc over limit panics", aborted(run_with_max("4K", 8192)));
    test_cond("max: suffix is case-insensitive", exited_ok(run_with_max("1m", 8192)));
    test_cond("max: plain bytes", aborted(run_with_max("4096", 8192)));
    test_cond("max: invalid value panics", aborted(run_with_max("12X", 1)));
    test_cond("max: zero panics", aborted(run_with_max("0", 1)));
}

static void test_fini(void)
{
    clear_roots();
    new_node();
    new_perm_node();
    nr_freed = 0;
    /* perm nodes of earlier tests are freed too */
    gc_fini();
    test_cond("fini: frees normal and perm blocks", nr_freed == 3);
    test_cond("fini: no memory left", gc_used() == 0);
}

int main(void)
{
    gc_init();
    test_object();
    test_cycle();
    test_values();
    test_bytes();
    test_perm();
    test_age();
    test_hooks();
    test_max();
    test_fini();
    test_report();
    return 0;
}
