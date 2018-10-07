
#ifndef _KOALA_TASK_H_
#define _KOALA_TASK_H_

#include <stdint.h>
#include "task_context.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * task state
 */
typedef enum {
  TASK_STATE_RUNNING = 1,
  TASK_STATE_READY   = 2,
  TASK_STATE_WAITING = 3,
  TASK_STATE_DONE    = 4,
} task_state_t;

/*
 * joinable state
 */
typedef enum {
  TASK_JOINABLE        = 0,
  TASK_WAIT_FOR_JOINER = 1,
  TASK_WAIT_TO_JOIN    = 2,
  TASK_DETACHED        = 3,
} join_state_t;

/*
 * task
 */
typedef struct task {
  task_context_t context;
  routine_t routine;
  void *arg;
  task_state_t volatile state;
  uint64_t id;
  void * volatile result;
  void * volatile sched_info;
  join_state_t volatile join_state;
  struct task * volatile join_task;
} task_t;

/*
 * task attribute
 */
typedef struct task_attr {
  int stacksize;
} task_attr_t;

typedef void *scheduler_context_t;

/*
 * task scheduler per thread
 */
typedef struct task_scheduler {
  task_t *idle;
  task_t *volatile current;
  task_t *gc;
  scheduler_context_t *sched_ctx;
  int id;
} task_scheduler_t;

/*
 * libtask API
 */
int task_scheduler_init(int num_threads);
task_t *task_create(task_attr_t *attr, routine_t routine, void *arg);
int task_yield(void);
int task_join(task_t *task, void **result);
int task_detach(task_t *task);
task_t *task_self(void);
task_scheduler_t *current_scheduler(void);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_TASK_H_ */
