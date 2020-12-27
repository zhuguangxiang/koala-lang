/*===-- task_context.c - Koala Coroutine Context ------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This file implements koala coroutine context used by ucontext.             *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#define _XOPEN_SOURCE
#include "task_context.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

#ifdef __cplusplus
extern "C" {
#endif

int task_context_init(
    task_context_t *ctx, int stacksize, task_entry_t entry, void *para)
{
    ucontext_t *uctx = malloc(sizeof(ucontext_t));
    if (!uctx) {
        errno = ENOMEM;
        return -1;
    }

    ctx->context = uctx;
    ctx->stackbase = malloc(stacksize);
    if (!ctx->stackbase) {
        free(uctx);
        errno = ENOMEM;
        return -1;
    }
    ctx->stacksize = stacksize;

    getcontext(uctx);
    uctx->uc_link = NULL;
    uctx->uc_stack.ss_sp = ctx->stackbase;
    uctx->uc_stack.ss_size = ctx->stacksize;
    uctx->uc_stack.ss_flags = 0;
    makecontext(uctx, (void (*)())entry, 1, para);
    return 0;
}

int task_context_save(task_context_t *ctx)
{
    memset(ctx, 0, sizeof(task_context_t));
    ctx->context = malloc(sizeof(ucontext_t));
    if (!ctx->context) {
        errno = ENOMEM;
        return -1;
    }
    getcontext(ctx->context);
    return 0;
}

void task_context_detroy(task_context_t *ctx)
{
    free(ctx->stackbase);
    free(ctx->context);
}

void task_context_switch(task_context_t *from, task_context_t *to)
{
    swapcontext(from->context, to->context);
}

#ifdef __cplusplus
}
#endif
