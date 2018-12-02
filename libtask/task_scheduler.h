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

#ifndef _KOALA_TASK_SCHEDULER_H_
#define _KOALA_TASK_SCHEDULER_H_

#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * scheduler
 * task_scheduler_locked.c(locked)
 * task_scheduler_wsd.c(work stealing deque)
 */

/* initial task scheduler */
int init_scheduler(int num_threads);
/* get scheduler by processor */
scheduler_t *get_scheduler(int proc);
/* schedule the task */
int schedule(scheduler_t *sched, task_t *task);
/* get a task to be run from the scheduler */
task_t *sched_get_task(scheduler_t *sched);
/* scheduler load balance */
void sched_load_balance(scheduler_t *sched);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_TASK_SCHEDULER_H_ */
