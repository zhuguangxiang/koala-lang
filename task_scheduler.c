
#include "task_scheduler.h"
#include <pthread.h>
#include <unistd.h>

static int sched_shutdown = 0;
static int sched_state = SCHED_STATE_NONE;
static int nr_scheds = DEFAULT_SCHEDULER_NR;
static task_scheduler_t *schedulers;
static pthread_t *threads;
static pthread_key_t thread_local_key;
pthread_mutex_t mutex;
int stolen = 0;

static void load_balance(task_scheduler_t *scheduler)
{
  if (&schedulers[0] == scheduler) {
    printf("main thread\n");
    return;
  }
  task_scheduler_t *sched = &schedulers[0];
  struct list_head *first = list_first(&sched->list);
  if (first) {
    if (stolen) {
      struct list_head *h = list_first(&scheduler->list);
      if (h) {
        list_del(h);
        list_add_tail(h, &sched->list);
      }
      return;
    }

    stolen = 1;
    task_t *tsk = container_of(first, task_t, node);
    assert(tsk != sched->idle);
    assert(tsk != sched->current);
    printf("balance\n");
    list_del(first);
    list_add_tail(first, &scheduler->list);
    printf("set uc_link\n");
  } else {
    printf("no need balance\n");
  }
}

static task_t *next_task(task_scheduler_t *scheduler)
{
  struct list_head *first = list_first(&scheduler->list);
  if (first) return container_of(first, task_t, node);
  else return NULL;
}

static void switch_to(task_scheduler_t *scheduler, task_t *to)
{
  task_t *from = scheduler->current;
  printf("idle? %d\n", from == scheduler->idle);
  printf("state: %d\n", from->state);
  if (from->state == TASK_STATE_RUNNING) {
    from->state = TASK_STATE_READY;
    if (from != scheduler->idle)
      list_add_tail(&from->node, &scheduler->list);
  } else if (from->state == TASK_STATE_DONE) {
    printf("down\n");
  } else {
    assert(0);
  }

  list_del(&to->node);
  assert(to->state == TASK_STATE_READY);
  to->state = TASK_STATE_RUNNING;

  scheduler->current = to;
  pthread_mutex_unlock(&mutex);
  task_context_swap(&from->context, &to->context);
}

static void switch_to_idle(task_scheduler_t *scheduler)
{
  task_t *from = scheduler->current;
  if (from == scheduler->idle) {
    printf("current is idle\n");
    assert(from->state == TASK_STATE_RUNNING);
    pthread_mutex_unlock(&mutex);
    return;
  }

  printf("to idle\n");
  if (from->state == TASK_STATE_RUNNING) {
    from->state = TASK_STATE_READY;
    list_add_tail(&from->node, &scheduler->list);
  } else if (from->state == TASK_STATE_DONE) {
    printf("task is finished\n");
  } else {
    assert(0);
  }
  scheduler->current = scheduler->idle;
  scheduler->current->state = TASK_STATE_RUNNING;
  pthread_mutex_unlock(&mutex);
  task_context_swap(&from->context, &scheduler->current->context);
}

task_scheduler_t *current_scheduler(void)
{
  return (task_scheduler_t *)pthread_getspecific(thread_local_key);
}

void scheduler_do_yield(task_scheduler_t *scheduler)
{
    pthread_mutex_lock(&mutex);
    task_t *tsk = next_task(scheduler);
    if (tsk) {
      switch_to(scheduler, tsk);
    } else {
      //FIXME
      printf("no more task waiting to run\n");
      switch_to_idle(scheduler);
    }
}

static void *task_entry_wrapper(void *para)
{
  pthread_setspecific(thread_local_key, para);
  task_scheduler_t *scheduler = (task_scheduler_t *)para;
  //getcontext(scheduler->idle->context.ctx_stack_ptr);
  printf("idle ctx:%p\n", scheduler->idle->context.ctx_stack_ptr);

  while (!sched_shutdown) {
    pthread_mutex_lock(&mutex);
    load_balance(scheduler);
    task_t *tsk = next_task(scheduler);
    if (tsk) {
      switch_to(scheduler, tsk);
      //scheduler->current = scheduler->idle;
    } else {
      //FIXME
      printf("no more task, switch to idle\n");
      pthread_mutex_unlock(&mutex);
      sleep(2);
    }
  }

  return NULL;
}

int task_scheduler_init(void)
{
  if (sched_state != SCHED_STATE_NONE) {
    errno = EINVAL;
    return -1;
  }

  pthread_mutex_init(&mutex, 0);

  threads = calloc(nr_scheds, sizeof(*threads));
  assert(threads);

  schedulers = malloc(nr_scheds * sizeof(*schedulers));
  assert(schedulers);

  schedulers[0].current = task_create_idle();
  schedulers[0].idle = schedulers[0].current;
  schedulers[0].id = "main";
  init_list_head(&schedulers[0].list);

  sched_state = SCHED_STATE_STARTED;

  for (int i = 1; i < nr_scheds; i++) {
    schedulers[i].id = "pthread";
    schedulers[i].current = task_create_idle();
    schedulers[i].idle = schedulers[i].current;
    init_list_head(&schedulers[i].list);
  }

  pthread_key_create(&thread_local_key, NULL);
  pthread_setspecific(thread_local_key, &schedulers[0]);

  for (int i = 1; i < nr_scheds; i++) {
    pthread_create(&threads[i], NULL, task_entry_wrapper, &schedulers[i]);
  }

  return 0;
}
