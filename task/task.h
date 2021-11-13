/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_TASK_H_
#define _KOALA_TASK_H_

#define _XOPEN_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <ucontext.h>

#ifdef __cplusplus
extern "C" {
#endif

/* task entry */
typedef void *(*task_entry_t)(void *);

/* opaque task */
typedef struct task task_t;

/* initialize task's procs */
void init_procs(int proc);

/* finalize task's procs */
void fini_procs(void);

/* current proc id */
int current_pid(void);

/* current task id */
uint64_t current_tid(void);

/* create a task with tls and argument */
task_t *task_create(task_entry_t entry, void *arg, void *tls);

/* set task's local storage */
void task_set_tls(void *tls);

/* get task's local storage */
void *task_tls(void);

/* yield cpu */
void task_yield(void);

/* resume a task from suspend state */
void task_resume(task_t *tsk);

/*
 * task sleep a while (ms)
 * < 0: suspend current task
 * = 0: just yield cpu
 * > 0: sleep milisecond
 */
void task_sleep(int timeout);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TASK_H_ */
