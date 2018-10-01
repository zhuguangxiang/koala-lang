
#ifndef _KOALA_TASK_SCHED_H_
#define _KOALA_TASK_SCHED_H_

#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_SCHEDULER_NR 2

#define SCHED_STATE_NONE    0
#define SCHED_STATE_STARTED 1
#define SCHED_STATE_ERROR   2

typedef struct task_scheduler {
  char *id;
  task_t *current;
  task_t *idle;
  struct list_head list;
} task_scheduler_t;

int task_scheduler_init(void);
task_scheduler_t *current_scheduler(void);
void scheduler_do_yield(task_scheduler_t *scheduler);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_TASK_SCHED_H_ */
