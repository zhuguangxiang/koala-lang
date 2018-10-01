
#include "common.h"
#include "task.h"

#if defined(__i386__)
#include "task_context_i386.c"
#elif defined(__x86_64__) && defined(SWITCHING_ASM64)
#include "task_context_x86_64.c"
#else
#define _XOPEN_SOURCE
#include <ucontext.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int task_context_init(task_context_t *ctx, task_func entry, void *para)
{
  ucontext_t *uctx = malloc(sizeof(ucontext_t));
  if (!uctx) {
    errno = ENOMEM;
    return -1;
  }

  ctx->ctx_stack_ptr = uctx;
  ctx->ctx_stack = malloc(DEFAULT_STACK_SIZE);
  if (!ctx->ctx_stack) {
    free(uctx);
    return -1;
  }
  ctx->ctx_stacksize = DEFAULT_STACK_SIZE;

  getcontext(uctx);
  uctx->uc_link = NULL;
  uctx->uc_stack.ss_sp = ctx->ctx_stack;
  uctx->uc_stack.ss_size = ctx->ctx_stacksize;
  uctx->uc_stack.ss_flags = 0;
  makecontext(uctx, (void (*)())entry, 1, para);
  return 0;
}

int task_context_save(task_context_t *ctx)
{
  memset(ctx, 0, sizeof(task_context_t));
  ctx->ctx_stack_ptr = malloc(sizeof(ucontext_t));
  if(!ctx->ctx_stack_ptr) {
    errno = ENOMEM;
    return -1;
  }
  getcontext((ucontext_t*)ctx->ctx_stack_ptr);
  return 0;
}

void task_context_detroy(task_context_t *ctx);
void task_context_swap(task_context_t *from, task_context_t *to)
{
  swapcontext((ucontext_t *)from->ctx_stack_ptr,
              (ucontext_t *)to->ctx_stack_ptr);
}

#endif
