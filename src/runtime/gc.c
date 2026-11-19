/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "gc.h"
#include "excobj.h"
#include "list.h"
#include "log.h"
#include "kosched.h"
#include "shadowstack.h"
#include "wsdeque.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Single-threaded, stop-the-world, tri-color mark & sweep.
 *
 * Every block is laid out as `GCHeader + payload`; the payload pointer is
 * returned to the caller and `(GCHeader *)ptr - 1` gets the header back.
 */

typedef enum _GCState {
    GC_DONE,       /* idle, mutator may allocate */
    GC_MARK_ROOTS, /* push roots into the gray deque */
    GC_MARK,       /* drain the gray deque */
    GC_SWEEP,      /* free white blocks, reset black to white */
} GCState;

typedef enum _GCColor {
    GC_COLOR_WHITE = 1,
    GC_COLOR_GRAY,
    GC_COLOR_BLACK,
} GCColor;

/* keeps payload 16-byte aligned */
typedef struct _GCHeader {
    List link;
    uint32_t size;
    uint8_t color;
    uint8_t kind;
    /* survived collections, perm blocks are 255 */
    uint8_t age;
} __attribute__((aligned(16))) GCHeader;

_Static_assert(sizeof(GCHeader) % 16 == 0, "GCHeader must be 16-byte aligned");

#define GC_HEADER(ptr) ((GCHeader *)(ptr) - 1)

static GCState gc_cur_state = GC_DONE;

/* blocks with age below this are marked and swept */
static uint8_t gc_age_threshold = 255;

/* generation: perm blocks are never swept; root blocks are perm blocks traced by _mark as roots */
static List gc_normal_list = LIST_INIT(gc_normal_list);
static List gc_perm_list = LIST_INIT(gc_perm_list);
static List gc_root_list = LIST_INIT(gc_root_list);

/* bytes held by gc blocks, headers included */
static size_t gc_used_size;

/* max of gc_used_size, set by KOALA_GC_MAX, e.g. 512M, 2G */
#define GC_DEFAULT_MAX_SIZE ((size_t)1 << 30)
static size_t gc_max_size = GC_DEFAULT_MAX_SIZE;

/* gray objects waiting to be scanned */
static WSDeque gc_gray_dq;

/* blocks on each list */
static size_t gc_nr_normal;
static size_t gc_nr_perm;

/* allocations minus deallocations since the last collection */
static int64_t gc_count;

/* accumulated and last collection */
static GCStats gc_stats;

/* observers, e.g. the gc module's debug output */
static GCHooks gc_hooks;

static const char *gc_state_names[] = {
    "DONE",
    "MARK_ROOTS",
    "MARK",
    "SWEEP",
};

static void gc_switch(GCState state)
{
    log_debug("gc: %s -> %s", gc_state_names[gc_cur_state], gc_state_names[state]);
    gc_cur_state = state;
}

/*---------------------------------------------------------------------------+
 |   Allocation                                                              |
 +---------------------------------------------------------------------------*/

static void *gc_alloc_block(int size, uint8_t kind, uint8_t age, List *list)
{
    ASSERT(gc_cur_state == GC_DONE);
    size_t total = sizeof(GCHeader) + size;
    if (gc_used_size + total > gc_max_size) {
        panic("gc: out of memory, used %zu + %zu bytes exceeds max %zu bytes", gc_used_size, total,
              gc_max_size);
    }
    GCHeader *hdr = calloc(1, total);
    if (!hdr) {
        log_fatal("gc: calloc %zu bytes failed", total);
        abort();
    }
    gc_used_size += total;
    gc_count++;
    if (list != &gc_normal_list) {
        gc_nr_perm++;
    } else {
        gc_nr_normal++;
    }
    hdr->size = size;
    hdr->color = GC_COLOR_WHITE;
    hdr->kind = kind;
    hdr->age = age;
    list_push_back(list, &hdr->link);
    log_trace("gc: alloc %p size %d kind %d age %d", hdr + 1, size, kind, age);
    return hdr + 1;
}

void *gc_alloc(int size) { return gc_alloc_block(size, GC_KIND_OBJECT, 0, &gc_normal_list); }

/*
perm blocks are never marked, scanned or freed; a root one is traced by its _mark each collection
 */
void *gc_alloc_perm(int size, int root)
{
    return gc_alloc_block(size, GC_KIND_OBJECT, 255, root ? &gc_root_list : &gc_perm_list);
}

TValue *gc_alloc_values(int n)
{
    return gc_alloc_block(n * sizeof(TValue), GC_KIND_VALUES, 0, &gc_normal_list);
}

void *gc_alloc_bytes(int size) { return gc_alloc_block(size, GC_KIND_BYTES, 0, &gc_normal_list); }

void *gc_alloc_perm_bytes(int size)
{
    return gc_alloc_block(size, GC_KIND_BYTES, 255, &gc_perm_list);
}

TValue *gc_alloc_perm_values(int n)
{
    return gc_alloc_block(n * sizeof(TValue), GC_KIND_VALUES, 255, &gc_perm_list);
}

static void gc_release_block(GCHeader *hdr)
{
    list_remove(&hdr->link);
    gc_used_size -= sizeof(GCHeader) + hdr->size;
    gc_count--;
    if (hdr->age == 255) {
        gc_nr_perm--;
    } else {
        gc_nr_normal--;
    }
    free(hdr);
}

static void gc_free_block(GCHeader *hdr)
{
    log_trace("gc: free %p size %u kind %d age %d", hdr + 1, hdr->size, hdr->kind, hdr->age);
    if (hdr->kind == GC_KIND_OBJECT) {
        Object *obj = (Object *)(hdr + 1);
        TypeObject *tp = OB_TYPE(obj);
        if (tp && tp->_free) tp->_free(obj);
    }
    gc_release_block(hdr);
}

void gc_free(void *ptr)
{
    if (!ptr) return;
    GCHeader *hdr = GC_HEADER(ptr);
    ASSERT(gc_cur_state == GC_DONE);
    ASSERT(hdr->age != 255);
    log_trace("gc: free (manual) %p size %u kind %d", ptr, hdr->size, hdr->kind);
    gc_release_block(hdr);
}

/*---------------------------------------------------------------------------+
 |   Mark                                                                    |
 +---------------------------------------------------------------------------*/

void gc_mark(void *ptr)
{
    if (!ptr) return;

    GCHeader *hdr = GC_HEADER(ptr);

    if (hdr->age >= gc_age_threshold) return;
    if (hdr->color != GC_COLOR_WHITE) return;

    if (hdr->kind == GC_KIND_BYTES) {
        /* no references, nothing to scan */
        hdr->color = GC_COLOR_BLACK;
        return;
    }

    ASSERT(hdr->kind == GC_KIND_OBJECT || hdr->kind == GC_KIND_VALUES);
    log_trace("gc: gray %p", ptr);
    hdr->color = GC_COLOR_GRAY;
    wsdq_push(&gc_gray_dq, ptr);
}

void gc_mark_value(TValue *val)
{
    Object *obj = NULL;

    if (is_ref(val)) {
        obj = val->obj;
    } else if (is_intf(val)) {
        IntfTable *itab = (IntfTable *)val->itab;
        if (tp_is_ref_type(itab->tp)) obj = val->obj;
    }

    /* type objects are meta info: static or perm, never collected */
    if (!obj || OB_TYPE(obj) == &type_type) return;

    gc_mark(obj);
}

/* native temporary roots of one ks (shadowstack.h) */
static void gc_mark_shadow(ShadowStack *ss)
{
    for (; ss; ss = ss->back) {
        for (uint32_t i = 0; i < ss->count; i++) {
            Object *obj = *(Object **)ss->slots[i];
            if (obj) gc_mark(obj);
        }
    }
}

/* Roots of one coroutine. A parked ks has a valid stack_top: switches happen
 * only inside native calls, which are safepoints (plan §8.1, §8.2). */
static void gc_mark_ks(KoalaState *ks)
{
    CallFrame *cf = ks->cf;
    while (cf) {
        gc_mark(cf->code);
        gc_mark(cf->module);
        cf = cf->back;
    }

    TValue *end = ks->stack_top;
    if (ks->cf && ks->cf->code) end += ks->cf->code->cs.max_call_args;
    for (TValue *v = ks->stack_base; v < end; v++) {
        gc_mark_value(v);
    }

    /* owning task keeps a running detached task alive */
    gc_mark(ks->task);
    /* entry function and args until the coroutine finishes */
    gc_mark(ks->co_fn);
    for (int i = 0; i < ks->co_nargs; i++) gc_mark_value(&ks->co_args[i]);
    /* Exception is mm-allocated: mark the references it holds, not itself */
    kl_exc_mark(ks->exc);
    /* a result handed to a parked coroutine lives only in its WaitRecord */
    if (ks->wait) gc_mark_value(&ks->wait->value);
    gc_mark_shadow(ks->shadow);
}

static void gc_mark_roots(void)
{
    GCHeader *hdr;
    list_foreach(hdr, link, &gc_root_list) {
        Object *obj = (Object *)(hdr + 1);
        TypeObject *tp = OB_TYPE(obj);
        if (tp->_mark) tp->_mark(obj);
    }

    Scheduler *s = __ts->sched;
    gc_mark(s->main->builtin);
    gc_mark(s->main->sys);

    /* every live coroutine's VM stack; dead ks waiting for region recycling
     * are off this list, so their stale stacks are never scanned */
    KoalaState *ks;
    list_foreach(ks, all_link, &s->all) gc_mark_ks(ks);

    /* deadline timers may hold the only reference to an object, e.g. a
     * Group with a timeout whose parent dropped it */
    for (int i = 0; i < s->ntimers; i++) gc_mark(s->timers[i]->root);

    log_debug("gc: mark roots, %ld gray", (long)wsdq_size(&gc_gray_dq));
}

static void gc_scan(void)
{
    void *ptr;
    while ((ptr = wsdq_pop(&gc_gray_dq))) {
        GCHeader *hdr = GC_HEADER(ptr);
        hdr->color = GC_COLOR_BLACK;
        if (hdr->kind == GC_KIND_OBJECT) {
            TypeObject *tp = OB_TYPE((Object *)ptr);
            log_trace("gc: scan %p type %s", ptr, tp->name);
            if (tp->_mark) tp->_mark(ptr);
        } else {
            ASSERT(hdr->kind == GC_KIND_VALUES);
            int n = hdr->size / sizeof(TValue);
            log_trace("gc: scan %p values %d", ptr, n);
            TValue *vals = ptr;
            for (int i = 0; i < n; i++) gc_mark_value(&vals[i]);
        }
        gc_stats.last.scanned++;
    }
    log_debug("gc: mark, %zu scanned", gc_stats.last.scanned);
}

/*---------------------------------------------------------------------------+
 |   Sweep                                                                   |
 +---------------------------------------------------------------------------*/

static void gc_sweep(void)
{
    GCCycleStats *last = &gc_stats.last;
    GCHeader *hdr, *nxt;
    list_foreach_safe(hdr, nxt, link, &gc_normal_list) {
        /* old blocks are not marked, keep them */
        if (hdr->age >= gc_age_threshold) {
            last->old++;
            continue;
        }

        size_t total = sizeof(GCHeader) + hdr->size;
        if (hdr->color == GC_COLOR_WHITE) {
            if (gc_hooks.on_free) gc_hooks.on_free(hdr + 1, hdr->kind, hdr->size);
            last->freed++;
            last->freed_size += total;
            gc_free_block(hdr);
        } else {
            ASSERT(hdr->color == GC_COLOR_BLACK);
            hdr->color = GC_COLOR_WHITE;
            if (hdr->age < 254) hdr->age++;
            last->alive++;
            last->alive_size += total;
        }
    }
    log_debug("gc: sweep, freed %zu (%zu bytes), alive %zu (%zu bytes), old %zu", last->freed,
              last->freed_size, last->alive, last->alive_size, last->old);
}

/*---------------------------------------------------------------------------+
 |   Collect                                                                 |
 +---------------------------------------------------------------------------*/

static double gc_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
}

size_t gc_collect(void)
{
    ASSERT(gc_cur_state == GC_DONE);
    memset(&gc_stats.last, 0, sizeof(gc_stats.last));
    double start = gc_now_ms();
    gc_switch(GC_MARK_ROOTS);

    while (gc_cur_state != GC_DONE) {
        switch (gc_cur_state) {
            case GC_MARK_ROOTS:
                gc_mark_roots();
                gc_switch(GC_MARK);
                break;
            case GC_MARK:
                gc_scan();
                gc_switch(GC_SWEEP);
                break;
            case GC_SWEEP:
                gc_sweep();
                gc_switch(GC_DONE);
                break;
            default:
                UNREACHABLE();
        }
    }

    gc_count = 0;
    gc_stats.last.elapsed_ms = gc_now_ms() - start;
    gc_stats.collections++;
    gc_stats.collected += gc_stats.last.freed;
    gc_stats.collected_size += gc_stats.last.freed_size;

    if (gc_hooks.on_collect) gc_hooks.on_collect(&gc_stats);
    return gc_stats.last.freed;
}

int64_t gc_get_count(void) { return gc_count; }

size_t gc_normal_count(void) { return gc_nr_normal; }

size_t gc_perm_count(void) { return gc_nr_perm; }

void gc_get_stats(GCStats *stats) { *stats = gc_stats; }

void gc_set_hooks(const GCHooks *hooks) { gc_hooks = hooks ? *hooks : (GCHooks){ 0 }; }

size_t gc_used(void) { return gc_used_size; }

size_t gc_max(void) { return gc_max_size; }

/* bytes, or with K/M/G suffix */
static size_t gc_parse_size(const char *s)
{
    char *end;
    errno = 0;
    unsigned long long n = strtoull(s, &end, 10);
    if (errno || end == s) panic("gc: invalid KOALA_GC_MAX '%s'", s);

    int shift = 0;
    switch (toupper((unsigned char)*end)) {
        case 'K':
            shift = 10;
            end++;
            break;
        case 'M':
            shift = 20;
            end++;
            break;
        case 'G':
            shift = 30;
            end++;
            break;
        default:
            break;
    }
    if (*end || n == 0 || n > (SIZE_MAX >> shift)) panic("gc: invalid KOALA_GC_MAX '%s'", s);
    return (size_t)n << shift;
}

void gc_init(void)
{
    const char *max = getenv("KOALA_GC_MAX");
    gc_max_size = max ? gc_parse_size(max) : GC_DEFAULT_MAX_SIZE;
    log_debug("gc: max size %zu bytes", gc_max_size);

    gc_nr_normal = 0;
    gc_nr_perm = 0;
    gc_count = 0;
    memset(&gc_stats, 0, sizeof(gc_stats));

    init_list(&gc_normal_list);
    init_list(&gc_perm_list);
    init_list(&gc_root_list);
    wsdq_init(&gc_gray_dq, 1024);
    gc_switch(GC_DONE);
}

static void gc_free_list(List *list)
{
    GCHeader *hdr, *nxt;
    list_foreach_safe(hdr, nxt, link, list) {
        gc_free_block(hdr);
    }
}

void gc_fini(void)
{
    ASSERT(gc_cur_state == GC_DONE);
    gc_free_list(&gc_normal_list);
    gc_free_list(&gc_perm_list);
    gc_free_list(&gc_root_list);
    wsdq_fini(&gc_gray_dq);
}

#ifdef __cplusplus
}
#endif
