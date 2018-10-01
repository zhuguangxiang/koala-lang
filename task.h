
#ifndef _KOALA_TASK_H_
#define _KOALA_TASK_H_

#include "common.h"
#include "list.h"
#include "task_context.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  TASK_STATE_RUNNING = 1,
  TASK_STATE_READY   = 2,
  TASK_STATE_WAITING = 3,
  TASK_STATE_DONE    = 4,
} task_state_t;

typedef struct task {
  task_state_t state;
  task_func entry;
  void *para;
  uint64 id;
  task_context_t context;
  struct list_head node;
} task_t;

task_t *task_create(task_func entry, void *para);
task_t *task_create_idle(void);
void task_yield(void);
task_t *current_task(void);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_TASK_H_ */
