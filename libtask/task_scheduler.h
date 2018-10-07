
#ifndef _KOALA_TASK_SCHEDULER_H_
#define _KOALA_TASK_SCHEDULER_H_

#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * scheduler context
 * task_scheduler_locked.c
 * task_scheduler_wsd.c(work stealing deque)
 */

/*
 * scheduler context API, internal used
 */
int scheduler_init(int num_threads);
scheduler_context_t *scheduler_get(int index);
int scheduler_schedule(scheduler_context_t *sched, task_t *task);
task_t *scheduler_next(scheduler_context_t *sched);
void scheduler_load_balance(scheduler_context_t *sched);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_TASK_SCHEDULER_H_ */
