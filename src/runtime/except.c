/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

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

static TValue kl_exc_to_str(TValue *self, TValue *args, int nargs)
{
    // Exception *exc = (Exception *)self;
    // return kl_str_new(exc->msg);
    Object *s = kl_new_str("Exception()");
    return obj_value(s);
}

static MethodDef exc_methods[] = {
    { "__str__", kl_exc_to_str },
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

#ifdef __cplusplus
}
#endif
