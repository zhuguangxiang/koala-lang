/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "modobj.h"
#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Koala does NOT support catch exception.
 * If an exception occurred, it must be fixed.
 */

typedef struct _TraceBack {
    struct _TraceBack *back;
    char *file;
    int lineno;
} TraceBack;

typedef struct _Exception {
    OBJECT_HEAD
    char *msg;
    TraceBack *back;
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

Object *kl_new_exc(char *msg)
{
    Exception *exc = mm_alloc_obj(exc);
    INIT_OBJECT_HEAD(exc, &exc_type, 0);
    exc->msg = strdup(msg);
    exc->back = NULL;
    return (Object *)exc;
}

void _raise_exc_fmt(KoalaState *ks, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char msg[256];
    int len = vsnprintf(msg, 255, fmt, args);
    va_end(args);
    msg[len] = '\0';
    ks->exc = kl_new_exc(msg);
}

void _raise_exc_str(KoalaState *ks, char *str) { ks->exc = kl_new_exc(str); }

void _print_exc(KoalaState *ks)
{
    Object *obj = ks->exc;
    if (!obj) return;
    Exception *exc = (Exception *)obj;
    if (isatty(1)) {
        printf("\x1b[31mError:\x1b[0m %s\n", exc->msg);
    } else {
        printf("Error: %s\n", exc->msg);
    }
}

void kl_trace_here(CallFrame *cf)
{
    TraceBack *tb = mm_alloc_obj_fast(tb);
    tb->back = NULL;
    tb->file = cf->code->cs.filename;
    // TODO:
    tb->lineno = 0;

    Exception *exc = (Exception *)cf->ks->exc;
    tb->back = exc->back;
    exc->back = tb;
}

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

void kl_trace_back(KoalaState *ks)
{
    // Object *obj = ks->exc;
    // if (!obj) return;
    // Exception *exc = (Exception *)obj;
    // TraceBack *tb = exc->back;
    // while (tb) {
    //     printf("  File \"%s\", line %d\n", tb->file, tb->lineno);
    //     tb = tb->back;
    // }

    printf("\nTraceback (most recent call last):\n");
    CallFrame *cf = ks->cf;
    ModuleObject *mo = (ModuleObject *)cf->module;
    while (cf) {
        CodeObject *co = cf->code;
        char *func_name = co ? co->cs.name : "<unknown>";
        LineInfo *line = find_lineinfo(&mo->lineinfos, cf->pc);
        if (line && line->filename) {
            // Print full source location when LineInfo is available
            printf("  File \"%s\", line %d, in %s\n", line->filename, line->lineno, func_name);
            print_source_line(line->filename, line->lineno);
        } else {
            // Fallback using decimal PC directly mapping to --dump=code output
            printf("  [pc %04u] in %s\n", cf->pc, func_name);
        }
        cf = cf->back;
    }

    fflush(stdout);
}

#ifdef __cplusplus
}
#endif
