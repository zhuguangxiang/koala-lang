
#include "task.h"
#include "task_scheduler.h"

void *task_go_func(void *para)
{
  task_t *tsk = (task_t *)para;
  tsk->entry(tsk->para);
  tsk->state = TASK_STATE_DONE;
  task_yield();
  assert(0);
  return NULL;
}

task_t *task_create(task_func entry, void *para)
{
  task_t *tsk = calloc(1, sizeof(*tsk));
  if (!tsk) {
    errno = ENOMEM;
    return NULL;
  }

  init_list_head(&tsk->node);
  tsk->state = TASK_STATE_READY;
  tsk->id = 10;
  tsk->entry = entry;
  tsk->para = para;
  task_context_init(&tsk->context, task_go_func, tsk);
  task_scheduler_t *scheduler = current_scheduler();
  list_add_tail(&tsk->node, &scheduler->list);
  return tsk;
}

task_t *task_create_idle(void)
{
  task_t *tsk = calloc(1, sizeof(*tsk));
  if (!tsk) {
    errno = ENOMEM;
    return NULL;
  }

  init_list_head(&tsk->node);
  tsk->state = TASK_STATE_RUNNING;
  tsk->id = 1;
  task_context_save(&tsk->context);
  return tsk;
}

void task_yield(void)
{
  scheduler_do_yield(current_scheduler());
}

task_t *current_task(void)
{
  task_scheduler_t *scheduler = current_scheduler();
  return scheduler->current;
}
