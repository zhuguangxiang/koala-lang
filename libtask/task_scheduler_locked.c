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

static int num_scheds;
static task_scheduler_lldq_t *task_schedulers;

int init_scheduler(int num_threads)
{
  num_scheds = num_threads;
  task_schedulers = calloc(num_threads, sizeof(task_scheduler_lldq_t));
  assert(task_schedulers);
  for (int i = 0; i < num_threads; i++) {
    task_schedulers[i].id = i;
    task_schedulers[i].steal_count = 0;
    lldq_init(&task_schedulers[i].deque);
  }
  return 0;
}

scheduler_t *get_scheduler(int proc)
{
  assert(proc >= 0 && proc < num_scheds);
  return (scheduler_t *)&task_schedulers[proc];
}

int schedule(scheduler_t *sched, task_t *task)
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

task_t *sched_get_task(scheduler_t *sched)
{
  task_scheduler_lldq_t *sched_lldq = (task_scheduler_lldq_t *)sched;
  lldq_node_t *node = lldq_pop_head(&sched_lldq->deque);
  return node ? node->data : NULL;
}

void sched_load_balance(scheduler_t *sched)
{
  task_scheduler_lldq_t *sched_lldq = (task_scheduler_lldq_t *)sched;
  int i = sched_lldq->id;
  int end = i + num_scheds - 1;
  int mod = num_scheds;
  int index;
  lldq_deque_t *other_deque;
  lldq_node_t *node;
  int count = sched_lldq->deque.count;
  int num_steal;
  while (i++ < end) {
    index = i % mod;
    other_deque = &task_schedulers[index].deque;
    assert(other_deque != &sched_lldq->deque);
    num_steal = (other_deque->count - count) / 2;
    while (num_steal-- > 0) {
      node = lldq_pop_head(other_deque);
      if (node) {
        task_t *task = node->data;
        assert(task);
        printf("task:%lu is stolen by proc-%d\n",
                task->id, current_processor()->id);
        lldq_push_tail(&sched_lldq->deque, node);
        ++sched_lldq->steal_count;
      }
    }
  }
}
