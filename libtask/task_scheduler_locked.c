
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "task_scheduler.h"
#include "locked_linked_deque.h"

typedef struct task_scheduler_lldq {
  lldq_deque_t deque;
  int id;
  uint64_t steal_count;
} task_scheduler_lldq_t;

static int scheduler_num_scheds;
static task_scheduler_lldq_t *task_schedulers;

int scheduler_init(int num_threads)
{
  scheduler_num_scheds = num_threads;
  task_schedulers = calloc(num_threads, sizeof(task_scheduler_lldq_t));
  assert(task_schedulers);
  for (int i = 0; i < num_threads; i++) {
    task_schedulers[i].id = i;
    task_schedulers[i].steal_count = 0;
    lldq_init(&task_schedulers[i].deque);
  }
  return 0;
}

scheduler_context_t *scheduler_get(int index)
{
  assert(index >= 0 && index < scheduler_num_scheds);
  return (scheduler_context_t *)&task_schedulers[index];
}

int scheduler_schedule(scheduler_context_t *sched, task_t *task)
{
  task_scheduler_lldq_t *sched_lldq = (task_scheduler_lldq_t *)sched;
  lldq_node_t *node = task->sched_info;
  if (!node) {
    node = calloc(1, sizeof(lldq_node_t));
    if (!node) {
      errno = ENOMEM;
      return -1;
    }
    task->sched_info = node;
    node->data = task;
  }
  lldq_push_tail(&sched_lldq->deque, node);
  return 0;
}

task_t *scheduler_next(scheduler_context_t *sched)
{
  task_scheduler_lldq_t *sched_lldq = (task_scheduler_lldq_t *)sched;
  lldq_node_t *node = lldq_pop_head(&sched_lldq->deque);
  return node ? node->data : NULL;
}

void scheduler_load_balance(scheduler_context_t *sched)
{
  task_scheduler_lldq_t *sched_lldq = (task_scheduler_lldq_t *)sched;
  int i = sched_lldq->id;
  int end = i + scheduler_num_scheds - 1;
  int mod = scheduler_num_scheds;
  int index;
  lldq_deque_t *other_deque;
  lldq_node_t *node;
  int count = sched_lldq->deque.count;
  int num_steal;
  while (i++ < end) {
    index = i % mod;
    other_deque = &task_schedulers[index].deque;
    assert(other_deque != &sched_lldq->deque);
    num_steal = (other_deque->count - count);
    //printf("balance:[%d] = %d -> [%d] %d\n", index, other_deque->count, sched_lldq->id, count);
    while (num_steal-- > 0) {
      node = lldq_pop_head(other_deque);
      if (node) {
        task_t *task = node->data;
        assert(task);
        printf("task:%lu is stolen by sched-%d\n", task->id, current_scheduler()->id);
        lldq_push_tail(&sched_lldq->deque, node);
        ++sched_lldq->steal_count;
      }
    }
  }
}
