/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#define _XOPEN_SOURCE
#include "task_context.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

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
