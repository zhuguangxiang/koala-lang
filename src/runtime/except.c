/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "excobj.h"
#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

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
