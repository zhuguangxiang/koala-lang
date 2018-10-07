
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "task_scheduler.h"
#include "task_atomic.h"

/*
 * global variables
 */
#define SCHED_STATE_NONE    0
#define SCHED_STATE_STARTED 1
#define SCHED_STATE_ERROR   2
static int sched_state = SCHED_STATE_NONE;

static int num_pthreads;
static task_scheduler_t *schedulers;
static pthread_t *pthreads;
static int sched_shutdown = 0;
static pthread_key_t local_pthread_key;
static uint64_t task_idgen = 1;

/*
 * get current scheduler(current pthread)
 */
task_scheduler_t *current_scheduler(void)
{
  return pthread_getspecific(local_pthread_key);
}

/*
 * task routine entry function
 */
static void *task_go_routine(void *arg)
{
  task_t *task = arg;
  void *result = task->routine(task->arg);

  // do something after task is finished.
  task->result = result;
  if (task->join_state != TASK_DETACHED) {
    int old_state = atomic_set((int *)&task->join_state, TASK_WAIT_FOR_JOINER);
    if (old_state == TASK_JOINABLE) {
      // this state asserts no task joins us
      assert(!task->join_task);
    } else if (old_state == TASK_WAIT_TO_JOIN) {
      //a joining task is waiting for us to finish.
      task_t *join_task = task->join_task;
      assert(join_task);
      assert(join_task->state == TASK_STATE_WAITING);
      printf("task-%lu is joined by task-%lu, wakeup it.\n", task->id, join_task->id);
      task->join_task = NULL;
      join_task->state = TASK_STATE_READY;
      join_task->result = result;
      scheduler_schedule(current_scheduler()->sched_ctx, join_task);
    } else {
      //it's TASK_WAIT_FOR_JOINER - that's an error!!
      assert(0);
    }
  }
  task->state = TASK_STATE_DONE;
  task_yield();
  //!!never go here!!
  assert(0);
}

/*
 * switch to new task
 */
static void task_switch_to(task_scheduler_t *sched, task_t *to)
{
  task_t *from = sched->current;
  if (from->state == TASK_STATE_RUNNING) {
    from->state = TASK_STATE_READY;
    if (from != sched->idle)
      scheduler_schedule(sched->sched_ctx, from);
  } else if (from->state == TASK_STATE_DONE) {
    printf("task-%lu is done\n", from->id);
  } else if (from->state == TASK_STATE_WAITING) {
    printf("task-%lu is waiting\n", from->id);
  } else {
    assert(0);
  }

  assert(to->state == TASK_STATE_READY);
  to->state = TASK_STATE_RUNNING;
  sched->current = to;
  task_context_swap(&from->context, &to->context);
}

/*
 * pthread routine per scheduler
 * get next task from current scheduler and run it
 */
static void *pthread_routine(void *arg)
{
  pthread_setspecific(local_pthread_key, arg);
  task_scheduler_t *sched = arg;
  task_t *task;
  while (!sched_shutdown) {
    scheduler_load_balance(sched->sched_ctx);
    task = scheduler_next(sched->sched_ctx);
    if (task) {
      task_switch_to(sched, task);
    } else {
      printf("sched-%d, no more tasks, sleep 1s\n", sched->id);
      assert(sched->current == sched->idle);
      assert(sched->current->state == TASK_STATE_RUNNING);
      sleep(1);
    }
  }
  return NULL;
}

/*
 * idle task per scheduler
 */
static task_t *task_create_idle()
{
  task_t *task = calloc(1, sizeof(task_t));
  if (!task) {
    errno = ENOMEM;
    return NULL;
  }

  task->state = TASK_STATE_RUNNING;
  task_context_save(&task->context);

  return task;
}

/*
 * initialize 'index' scheduler(pthread)
 */
static void init_proc(int index)
{
  task_t *task = task_create_idle();
  schedulers[index].current = task;
  schedulers[index].idle = task;
  schedulers[index].id = index;
  schedulers[index].sched_ctx = scheduler_get(index);
  schedulers[index].gc = NULL;
}

/*
 * initialize main scheduler(pthread)
 */
static void init_main_proc(void)
{
  init_proc(0);
  pthread_key_create(&local_pthread_key, NULL);
  pthread_setspecific(local_pthread_key, &schedulers[0]);
}

/*
 * task scheduler intialization, call it firstly
 */
int task_scheduler_init(int num_threads)
{
  if (sched_state != SCHED_STATE_NONE) {
    errno = EINVAL;
    return -1;
  }

  if (scheduler_init(num_threads)) {
    return -1;
  }

  num_pthreads = num_threads;
  schedulers = calloc(num_threads, sizeof(task_scheduler_t));
  assert(schedulers);
  pthreads = calloc(num_threads, sizeof(pthread_t));
  assert(pthreads);

  //main processor
  init_main_proc();
  pthreads[0] = pthread_self();

  for (int i = 1; i < num_threads; i++) {
    init_proc(i);
    pthread_create(&pthreads[i], NULL, pthread_routine, &schedulers[i]);
  }

  return 0;
}

/*
 * create an task and run it
 */
task_t *task_create(task_attr_t *attr, routine_t routine, void *arg)
{
  task_t *task = calloc(1, sizeof(task_t));
  if (!task) {
    errno = ENOMEM;
    return NULL;
  }

  task->routine = routine;
  task->arg = arg;
  task->state = TASK_STATE_READY;
  task->id = task_idgen++;
  task_context_init(&task->context, attr->stacksize, task_go_routine, task);

  scheduler_schedule(current_scheduler()->sched_ctx, task);
  //printf("thread %lu created\n", thread->id);
  return task;
}

int task_yield(void)
{
  task_scheduler_t *sched = current_scheduler();
  scheduler_load_balance(sched->sched_ctx);
  task_t *task = scheduler_next(sched->sched_ctx);
  if (!task) {
    task_t *current = sched->current;
    if (current->state == TASK_STATE_DONE ||
        current->state == TASK_STATE_WAITING) {
      task = sched->idle;
      printf("idle state:%d\n", task->state);
      task_switch_to(sched, task);
    }
  } else {
    task_switch_to(sched, task);
  }
  return 0;
}

int task_join(task_t *task, void **result)
{
  if (result) *result = NULL;

  if (task->join_state == TASK_DETACHED) {
    printf("task cannot be joined.\n");
    return -1;
  }

  int old_state = atomic_set((int *)&task->join_state, TASK_WAIT_TO_JOIN);
  if (old_state == TASK_JOINABLE) {
    // need to wait until other task finished
    task_scheduler_t *sched = current_scheduler();
    task_t *current = sched->current;
    current->state = TASK_STATE_WAITING;
    task->join_task = current;
    printf("task-%lu is not finished\n", task->id);
    task_yield();
  } else if (old_state == TASK_WAIT_FOR_JOINER) {
    // other task is finished
    printf("task-%lu is finished\n", task->id);
    if (result) *result = task->result;
  } else {
    //it's TASK_WAIT_TO_JOIN or TASK_DETACHED - that's an error!!
    assert(0);
  }
  return 0;
}

int task_detach(task_t *task);

/*
 * get current task(like pthread_self())
 */
task_t *task_self(void)
{
  return current_scheduler()->current;
}
