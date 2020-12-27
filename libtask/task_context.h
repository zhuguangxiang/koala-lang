/*===-- task_context.h - Koala Coroutine Context ------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This header declares koala coroutine context.                              *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

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
