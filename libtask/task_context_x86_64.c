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

#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "task_context.h"

int task_context_init(task_context_t *ctx, routine_t entry, void *arg)
{
  ctx->stackbase = malloc(8192000);
  ctx->stacksize = 8192000;

  ctx->context = (void**)((char*)ctx->stackbase + ctx->stacksize) - 1;
  //16 byte stack alignment
  ctx->context = (void*)((uintptr_t)ctx->context & ~0x0f);
  /*
    context must be decremented an even number of times to maintain 16 byte
    alignement. this decrement is a dummy/filler decrement
   */
  --ctx->context;
  * --ctx->context = arg;
  *--ctx->context = NULL; /* dummy return address */
  *--ctx->context = (void*)entry;
  *--ctx->context = 0;// rbp
  *--ctx->context = 0;// rbx
  *--ctx->context = 0;// r12
  *--ctx->context = 0;// r13
  *--ctx->context = 0;// r14
  *--ctx->context = 0;// r15

  assert(((uintptr_t)ctx->context & 0x0f) == 0); // verify 16 byte alignment
  return 0;
}

int task_context_save(task_context_t* ctx)
{
  memset(ctx, 0, sizeof(*ctx));
  return 0;
}

void task_context_swap(task_context_t* fromctx, task_context_t* toctx)
{
  void*** const from_sp = &fromctx->context;
  void** const to_sp = toctx->context;

  /*
    Here the machine context is saved on the stack.

    A fiber's stack looks like this while it's not running (stop grows down here to match memory):
        param (new contexts only)
        return address (dummy, new contexts only)
        rip (after resuming)
        rbp
        rbx
        r12
        r13
        r14
        r15

    For the current fiber:
        1. calculate the rip for the resume (leaq 0f(%%rip), %%rax -> load address of the label '0' into rax)
        2. save new rip (rax), rbp, rbx, r12, r13, r14, r15
        3. the current fiber's stack pointer is saved into the 'from' context
    For the fiber to be resumed:
        1. rip is fetched off the stack early (so it's available when it's time to jump)
        2. the stack pointer is retrieved from the 'to' context. the stack is now
            switched.
        3. restore r15, r14, r13, r12, rbx, rbp for the 'to' fiber
        4. load 'param' into rdi (needed for new contexts). note that rdi is the first parameter to a function
        5. correct the stack pointer (ie. counter the 'push rax', without overwriting rip)
        6. jump to the new program counter

    NOTES: this switch implementation works seemlessly with new contexts and
            existing contexts

    Credits:
        1. a good chunk of the context switch logic/algorithm was taken from Boost Coroutine, written as
            a Google Summer of Code project, Copyright 2006 Giovanni P. Deretta
            (http://www.crystalclearsoftware.com/soc/coroutine/). This work is subject
            to the Boost Software Licence Version 1.0 as copied below.
        2. the boost coroutine switch does not write rip as far as I can tell - I added
            the leaq 0f(%%rip), %%rax / push rax logic (Brian Watling)
        3. the boost coroutine switch does not save r12 through r15, and needlessly saves
            rax and rdx - i corrected this (Brian Watling)


    Licensing info for x86_64 context switch:

    Boost Software License - Version 1.0 - August 17th, 2003

    Permission is hereby granted, free of charge, to any person or organization
    obtaining a copy of the software and accompanying documentation covered by
    this license (the "Software") to use, reproduce, display, distribute,
    execute, and transmit the Software, and to prepare derivative works of the
    Software, and to permit third-parties to whom the Software is furnished to
    do so, all subject to the following:

    The copyright notices in the Software and this entire statement, including
    the above license grant, this restriction and the following disclaimer,
    must be included in all copies of the Software, in whole or in part, and
    all derivative works of the Software, unless such copies or derivative
    works are solely in the form of machine-executable object code generated by
    a source language processor.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
    SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
    FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
  */

  __builtin_prefetch ((void**)to_sp, 1, 3);
  __builtin_prefetch ((void**)to_sp, 0, 3);
  __builtin_prefetch ((void**)to_sp+64/4, 1, 3);
  __builtin_prefetch ((void**)to_sp+64/4, 0, 3);
  __builtin_prefetch ((void**)to_sp+32/4, 1, 3);
  __builtin_prefetch ((void**)to_sp+32/4, 0, 3);
  __builtin_prefetch ((void**)to_sp-32/4, 1, 3);
  __builtin_prefetch ((void**)to_sp-32/4, 0, 3);
  __builtin_prefetch ((void**)to_sp-64/4, 1, 3);
  __builtin_prefetch ((void**)to_sp-64/4, 0, 3);

  __asm__ volatile
  (
    "leaq 0f(%%rip), %%rax\n\t"
    "movq  48(%[to]), %%rcx\n\t" // quickly grab the new rip for 'to'
    "pushq %%rax\n\t" // save a new rip for after the fiber is resumed
    "pushq %%rbp\n\t"
    "pushq %%rbx\n\t"
    "pushq %%r12\n\t"
    "pushq %%r13\n\t"
    "pushq %%r14\n\t"
    "pushq %%r15\n\t"
    "movq  %%rsp, (%[from])\n\t"
    "movq  %[to], %%rsp\n\t"
    "popq  %%r15\n\t"
    "popq  %%r14\n\t"
    "popq  %%r13\n\t"
    "popq  %%r12\n\t"
    "popq  %%rbx\n\t"
    "popq  %%rbp\n\t"
    "movq  64(%[to]), %%rdi\n\t" // Future Optimization: no need to load this if the fiber has already been started
    "add   $8, %%rsp\n\t"
    "jmp   *%%rcx\n\t" // Future Optimization: if rcx is label '0', no need for the jmp (ie. if the fiber is alread started)
    "0:\n\t"
    :
    : [from] "D" (from_sp),
      [to]   "S" (to_sp)
    : "cc",
      "memory"
  );
}
