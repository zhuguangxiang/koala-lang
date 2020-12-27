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

#ifndef _KOALA_TASK_CONTEXT_H_
#define _KOALA_TASK_CONTEXT_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void *(*task_entry_t)(void *);

typedef struct task_context {
    void *stackbase;
    int stacksize;
    void *context;
} task_context_t;

int task_context_init(
    task_context_t *ctx, int stacksize, task_entry_t entry, void *para);
int task_context_save(task_context_t *ctx);
void task_context_detroy(task_context_t *ctx);
void task_context_switch(task_context_t *from, task_context_t *to);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TASK_CONTEXT_H_ */
