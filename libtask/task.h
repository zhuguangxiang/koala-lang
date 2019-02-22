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

#ifndef _KOALA_TASK_H_
#define _KOALA_TASK_H_

#include <stdint.h>
#include "task_context.h"

#ifdef __cplusplus
extern "C" {
#endif

/* task state */
typedef enum {
  TASK_STATE_RUNNING = 1,
  TASK_STATE_READY   = 2,
  TASK_STATE_WAITING = 3,
  TASK_STATE_DONE    = 4,
} task_state_t;

/* joinable state */
typedef enum {
  TASK_JOINABLE        = 0,
  TASK_WAIT_FOR_JOINER = 1,
  TASK_WAIT_TO_JOIN    = 2,
  TASK_DETACHED        = 3,
} join_state_t;

/* task */
typedef struct task {
  task_context_t context;
  task_entry_t entry;
  void *arg;
  task_state_t volatile state;
  uint64_t id;
  void * volatile result;
  void * volatile sched_info;
  join_state_t volatile join_state;
  struct task * volatile join_task;
  void *object;
} task_t;

/* task attribute */
typedef struct task_attr {
  int stacksize;
} task_attr_t;

/* task scheduler */
typedef void *scheduler_t;

/* task processor per thread */
typedef struct task_processor {
  task_t *idle;
  task_t *volatile current;
  scheduler_t *scheduler;
  int id;
} task_processor_t;

/* initial processors, call it firstly */
int task_init_procs(int num_threads);
/* finalize processors */
void task_fini_procs(void);
/* create a task and run it */
task_t *task_create(task_attr_t *attr, task_entry_t entry, void *arg);
/* free task memory */
void task_free(task_t *task);
/* yield cpu and call scheduler */
int task_yield(void);
/* waiting for other task finished */
int task_join(task_t *task, void **result);
/* set task in detached mode */
int task_detach(task_t *task);
/* get current task */
task_t *current_task(void);
/* get current processor in which current task is running */
task_processor_t *current_processor(void);
/* set task private object */
#define task_set_private(obj) \
do { \
  current_task()->object = obj; \
} while (0)

/* get task private object */
#define task_get_private() \
({ \
  current_task()->object; \
})

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_TASK_H_ */
