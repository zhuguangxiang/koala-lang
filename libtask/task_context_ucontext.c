
#define _XOPEN_SOURCE
#include <ucontext.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "task_context.h"

int task_context_init(task_context_t *ctx, int stacksize,
                      routine_t entry, void *para)
{
  ucontext_t *uctx = malloc(sizeof(ucontext_t));
  if (!uctx) {
    errno = ENOMEM;
    return -1;
  }

  ctx->context = (void **)uctx;
  ctx->stackbase = malloc(stacksize);
  if (!ctx->stackbase) {
    free(uctx);
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
  if(!ctx->context) {
    errno = ENOMEM;
    return -1;
  }
  getcontext((ucontext_t*)ctx->context);
  return 0;
}

void task_context_detroy(task_context_t *ctx);

void task_context_swap(task_context_t *from, task_context_t *to)
{
  swapcontext((ucontext_t *)from->context, (ucontext_t *)to->context);
}
