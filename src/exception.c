/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "exception.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeObject exc_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "Exception",
};

static Object *_new_exc(const char *msg)
{
    Exception *exc = gc_alloc_obj(exc);
    INIT_OBJECT_HEAD(exc, &exc_type);
    exc->msg = strdup(msg);
    exc->back = NULL;
    return (Object *)exc;
}

void _raise_exc_fmt(KoalaState *ks, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char msg[256];
    int len = vsnprintf(msg, 255, fmt, args);
    va_end(args);
    msg[len] = '\0';
    ks->exc = _new_exc(msg);
}

void _raise_exc_str(KoalaState *ks, const char *str) { ks->exc = _new_exc(str); }

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
