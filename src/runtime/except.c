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

void exc_free(Object *obj)
{
    if (!obj) return;
    Exception *exc = (Exception *)obj;
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

void kl_panic(char *msg)
{
    fprintf(stderr, "Panic: %s\n", msg);
    exit(1);
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

static void print_tracebacks(Exception *exc)
{
    printf("\nTraceback (most recent call last):\n");

    TraceBack *tb;
    vector_foreach_ptr(tb, &exc->tracebacks) {
        if (tb->filename) {
            printf("  File \"%s\", line %d, in %s\n", tb->filename, tb->lineno, tb->funcname);
            print_source_line(tb->filename, tb->lineno);
        } else {
            printf("  [pc %04u] in %s\n", tb->pc, tb->funcname);
        }
    }

    fflush(stdout);
}

void print_exc(Object *obj)
{
    if (!obj) return;

    Exception *exc = (Exception *)obj;

    CodeObject *code = exc->co;
    printf("\nFailure in func '%s'\n", code->cs.name);

    print_tracebacks(exc);

    if (isatty(1)) {
        printf("\n\x1b[31mError:\x1b[0m %s\n", exc->msg);
    } else {
        printf("Error: %s\n", exc->msg);
    }

    fflush(stdout);
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

    TraceBack tb;

    if (line && line->filename) {
        // // Print full source location when LineInfo is available
        // printf("  File \"%s\", line %d, in %s\n", line->filename, line->lineno, func_name);
        // print_source_line(line->filename, line->lineno);
        tb.filename = line->filename;
        tb.funcname = func_name;
        tb.lineno = line->lineno;
        tb.pc = 0;
    } else {
        // // Fallback using decimal PC directly mapping to --dump=code output
        // printf("  [pc %04u] in %s\n", cf->pc, func_name);
        tb.filename = NULL;
        tb.funcname = func_name;
        tb.lineno = 0;
        tb.pc = cf->pc;
    }

    vector_push_back(&exc->tracebacks, &tb);
}

#ifdef __cplusplus
}
#endif
