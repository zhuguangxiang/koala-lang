/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "modobj.h"
#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _TraceBack {
    char *filename;
    char *funcname;
    int lineno;
    uint32_t pc;
} TraceBack;

typedef struct _Exception {
    OBJECT_HEAD
    // exception message
    char *msg;
    // failed code object
    CodeObject *co;
    // failed inlined function, NULL if not inlined
    char *inlined_func;
    // tracebacks
    Vector tracebacks;
} Exception;

static TValue _exc_str(TValue *self, TValue *args, int nargs)
{
    // Exception *exc = (Exception *)self;
    // return kl_str_new(exc->msg);
    Object *s = kl_new_str("Exception()");
    return obj_value(s);
}

static MethodDef exc_methods[] = {
    { "__str__", _exc_str },
    { NULL, NULL },
};

TypeObject exc_type = {
    ._type = &type_type,
    .name = "Exception",
    .flags = TP_FLAGS_CLASS,
    .methdefs = exc_methods,
};

static Object *_new_exc(CodeObject *co, char *msg)
{
    Exception *exc = mm_alloc_obj(exc);
    INIT_OBJECT_HEAD(exc, &exc_type, 0);
    exc->msg = strdup(msg);
    exc->co = co;
    vector_init(&exc->tracebacks, sizeof(TraceBack));
    return (Object *)exc;
}

void kl_free_exc(Object *ob)
{
    if (!ob) return;
    Exception *exc = (Exception *)ob;
    vector_fini(&exc->tracebacks);
    free(exc->msg);
    mm_free(exc);
}

void _raise_exc_fmt(KoalaState *ks, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char msg[256];
    int len = vsnprintf(msg, 255, fmt, args);
    va_end(args);
    msg[len] = '\0';
    ks->exc = _new_exc(ks->cf->code, msg);
}

void _raise_exc_str(KoalaState *ks, char *str) { ks->exc = _new_exc(ks->cf->code, str); }

char *kl_exc_get_msg(Object *exc)
{
    if (!exc) return NULL;
    Exception *e = (Exception *)exc;
    return e->msg;
}

/**
 * Binary search for LineInfo matching the given PC in lineinfos vector.
 * Finds the LineInfo entry with the largest pc that satisfies info->pc <= pc.
 */
static LineInfo *find_lineinfo(Vector *lineinfos, uint32_t pc)
{
    if (vector_empty(lineinfos)) return NULL;

    LineInfo *items = VECTOR_ITEMS(lineinfos, LineInfo);
    int low = 0;
    int high = vector_size(lineinfos) - 1;
    LineInfo *result = NULL;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (items[mid].pc <= pc) {
            // Record candidate and search right for closer match
            result = &items[mid];
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    return result;
}

/* the inlined site of pc, 0 if not inlined */
static uint32_t find_inline_site(Vector *inline_pcs, uint32_t pc)
{
    InlinePc *items = VECTOR_ITEMS(inline_pcs, InlinePc);
    int low = 0;
    int high = vector_size(inline_pcs) - 1;
    uint32_t site = 0;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (items[mid].pc <= pc) {
            site = items[mid].site;
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    return site;
}

/**
 * Reads and prints a trimmed line of code from a source file by filename and line number.
 */
static void print_source_line(const char *filename, int target_line)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) return; // Silently skip if source file does not exist

    char line_buf[512];
    int current_line = 1;

    while (fgets(line_buf, sizeof(line_buf), fp)) {
        if (current_line == target_line) {
            // Trim leading whitespace
            char *start = line_buf;
            while (*start && isspace((unsigned char)*start)) {
                start++;
            }

            // Trim trailing whitespace
            char *end = start + strlen(start) - 1;
            while (end >= start && isspace((unsigned char)*end)) {
                *end = '\0';
                end--;
            }

            // Print the trimmed source line with a fixed 4-space indentation
            if (*start) {
                printf("    %s\n", start);
            }
            break;
        }
        current_line++;
    }

    fclose(fp);
}

static int same_str(const char *a, const char *b)
{
    return a == b || (a && b && !strcmp(a, b));
}

static int same_traceback(TraceBack *a, TraceBack *b)
{
    return a->lineno == b->lineno && a->pc == b->pc && same_str(a->funcname, b->funcname) &&
           same_str(a->filename, b->filename);
}

/* identical consecutive entries (deep recursion) print at most this many times */
#define TB_REPEAT_MAX 3

static void print_repeated(int n)
{
    if (n > 0) printf("  [Previous line repeated %d more time%s]\n", n, n > 1 ? "s" : "");
}

static void print_tracebacks(Exception *exc)
{
    printf("\nTraceback (most recent call last):\n");

    TraceBack *tb;
    TraceBack *last = NULL;
    int count = 0;
    vector_foreach_ptr(tb, &exc->tracebacks) {
        if (last && same_traceback(last, tb)) {
            if (++count > TB_REPEAT_MAX) continue;
        } else {
            if (last) print_repeated(count - TB_REPEAT_MAX);
            last = tb;
            count = 1;
        }
        if (!tb->funcname) {
            // a frame reused by tail calls to other functions (as Lua does)
            printf("  (... %d tail call%s ...)\n", tb->lineno, tb->lineno > 1 ? "s" : "");
        } else if (tb->filename) {
            printf("  File \"%s\", line %d, in %s\n", tb->filename, tb->lineno, tb->funcname);
            print_source_line(tb->filename, tb->lineno);
        } else {
            printf("  [pc %04u] in %s\n", tb->pc, tb->funcname);
        }
    }
    print_repeated(count - TB_REPEAT_MAX);

    fflush(stdout);
}

void print_exc_and_free(Object *obj)
{
    if (!obj) return;

    Exception *exc = (Exception *)obj;

    CodeObject *code = exc->co;
    printf("\nFailure in func '%s'\n", exc->inlined_func ? exc->inlined_func : code->cs.name);

    print_tracebacks(exc);

    if (isatty(1)) {
        printf("\n\x1b[31mError:\x1b[0m %s\n", exc->msg);
    } else {
        printf("\nError: %s\n", exc->msg);
    }

    fflush(stdout);

    kl_free_exc(obj);
}

static void push_traceback(Exception *exc, char *filename, char *funcname, int lineno, uint32_t pc)
{
    TraceBack tb;
    tb.funcname = funcname;
    if (filename) {
        tb.filename = filename;
        tb.lineno = lineno;
        tb.pc = 0;
    } else {
        // fallback using decimal PC directly mapping to --dump=code output
        tb.filename = NULL;
        tb.lineno = 0;
        tb.pc = pc;
    }
    vector_push_back(&exc->tracebacks, &tb);
}

void trace_here(CallFrame *cf)
{
    KoalaState *ks = cf->ks;
    ASSERT(ks->exc);
    Exception *exc = (Exception *)ks->exc;

    ModuleObject *mo = (ModuleObject *)cf->module;
    CodeObject *co = cf->code;

    char *func_name = co ? co->cs.name : "<unknown>";
    LineInfo *line = find_lineinfo(&mo->lineinfos, cf->pc);
    char *filename = line ? line->filename : NULL;
    int lineno = line ? line->lineno : 0;

    // an inlined frame for each inlined site, from the innermost
    uint32_t index = find_inline_site(&mo->inline_pcs, cf->pc);
    if (index && vector_empty(&exc->tracebacks) && co == exc->co) {
        exc->inlined_func = VECTOR_ITEMS(&mo->inline_sites, InlineSite)[index - 1].funcname;
    }
    while (index) {
        InlineSite *site = VECTOR_ITEMS(&mo->inline_sites, InlineSite) + index - 1;
        push_traceback(exc, filename, site->funcname, lineno, cf->pc);
        filename = site->filename;
        lineno = site->lineno;
        index = site->parent;
    }

    push_traceback(exc, filename, func_name, lineno, cf->pc);

    if (cf->tailcalls > 0) {
        TraceBack tb = { NULL, NULL, cf->tailcalls, 0 };
        vector_push_back(&exc->tracebacks, &tb);
    }
}

#ifdef __cplusplus
}
#endif
