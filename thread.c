
#include "debug.h"
#include "thread.h"
#include <unistd.h>
#include <signal.h>
#include <errno.h>

struct scheduler sched;

static inline void init_task_ucontext(ucontext_t *ctx)
{
  getcontext(ctx);
  ctx->uc_link = NULL;
  ctx->uc_stack.ss_sp = malloc(TASK_STACK_SIZE);
  ctx->uc_stack.ss_size = TASK_STACK_SIZE;
  ctx->uc_stack.ss_flags = 0;
}

static inline void free_task_ucontext(ucontext_t *ctx)
{
  free(ctx->uc_stack.ss_sp);
}

void task_wrapper(struct task *tsk)
{
  tsk->run(tsk);
  task_exit(tsk);
}

int task_init(struct task *tsk, short prio, task_func run, void *arg)
{
  init_list_head(&tsk->link);
  init_task_ucontext(&tsk->ctx);
  makecontext(&tsk->ctx, (void (*)())task_wrapper, 1, tsk);
  tsk->state = STATE_READY;
  tsk->prio = prio;
  tsk->run = run;
  tsk->arg = arg;
  tsk->thread = NULL;
  pthread_mutex_lock(&sched.lock);
  tsk->id = ++sched.idgen;
  list_add_tail(&tsk->link, &sched.readylist[tsk->prio]);
  pthread_mutex_unlock(&sched.lock);
  return 0;
}

static void task_add_sleeplist(struct task *tsk)
{
  struct task *t;
  list_for_each_entry(t, &sched.sleeplist, link) {
    if (tsk->sleep < t->sleep)
      break;
  }
  __list_add(&tsk->link, t->link.prev, &t->link);
}

void task_sleep(struct task *tsk, int second)
{
  assert(tsk->state == STATE_RUNNING);
  assert(list_unlinked(&tsk->link));
  tsk->state = STATE_SUSPEND;
  tsk->sleep = second;
  pthread_mutex_lock(&sched.lock);
  task_add_sleeplist(tsk);
  pthread_mutex_unlock(&sched.lock);
  struct thread *thread = tsk->thread;
  assert(thread != NULL);
  swapcontext(&tsk->ctx, &thread->ctx);
}

void task_exit(struct task *tsk)
{
  assert(tsk->state == STATE_RUNNING);
  assert(list_unlinked(&tsk->link));
  tsk->state = STATE_DEAD;
  struct thread *thread = tsk->thread;
  assert(thread != NULL);
  setcontext(&thread->ctx);
}

void task_yield(struct task *tsk)
{
  assert(tsk->state == STATE_RUNNING);
  assert(list_unlinked(&tsk->link));
  tsk->state = STATE_READY;
  pthread_mutex_lock(&sched.lock);
  list_add_tail(&tsk->link, &sched.readylist[tsk->prio]);
  pthread_mutex_unlock(&sched.lock);
  struct thread *thread = tsk->thread;
  assert(thread != NULL);
  swapcontext(&tsk->ctx, &thread->ctx);
}

void task_suspend(struct task *tsk, int timeout)
{
  assert(tsk->state == STATE_RUNNING);
  assert(list_unlinked(&tsk->link));
  tsk->state = STATE_SUSPEND;
  pthread_mutex_lock(&sched.lock);
  list_add_tail(&tsk->link, &sched.readylist[tsk->prio]);
  pthread_mutex_unlock(&sched.lock);
  struct thread *thread = tsk->thread;
  assert(thread != NULL);
  swapcontext(&tsk->ctx, &thread->ctx);
}

static struct list_head *next_node(void)
{
  struct list_head *node;
  for (int i = 0; i < NR_PRIORITY; i++) {
    node = list_first(sched.readylist + i);
    if (node != NULL) return node;
  }
  return NULL;
}

static void *task_thread_func(void *arg)
{
  struct thread *thread = arg;
  struct task *tsk;
  struct list_head *node;

  while (1) {
    pthread_mutex_lock(&sched.lock);
    while ((node = next_node()) == NULL) {
      pthread_cond_wait(&sched.cond, &sched.lock);
    }
    list_del(node);
    pthread_mutex_unlock(&sched.lock);
    tsk = container_of(node, struct task, link);
    thread->current = tsk;
    tsk->thread = thread;
    tsk->state = STATE_RUNNING;
    swapcontext(&thread->ctx, &tsk->ctx);
    tsk->thread = NULL;
  }

  return NULL;
}

void timer_signal_handler(int v)
{
  sched.clock++;
  int ret = timer_getoverrun(sched.timer);
  if (ret < 0) {
    debug_error("errno:%d\n", errno);
    return;
  }
  sched.clock = sched.clock + ret;
  sem_post(&sched.timer_sem);
}

static void *timer_thread_func(void *arg)
{
  while (1) {
    sem_wait(&sched.timer_sem);

    uint64 clock = sched.clock;
    uint64 tick = sched.tick;
    if (tick >= clock) {
      debug_warn("tick >= clock\n");
      return NULL;
    }

    struct list_head *p, *n;
    struct task *tsk;

    pthread_mutex_lock(&sched.lock);
    list_for_each_safe(p, n, &sched.sleeplist) {
      tsk = container_of(p, struct task, link);
      tsk->sleep -= (clock - tick);
      if (tsk->sleep <= 0) {
        list_del(&tsk->link);
        tsk->state = STATE_READY;
        list_add_tail(&tsk->link, &sched.readylist[tsk->prio]);
      } else {
        break;
      }
    }
    pthread_cond_broadcast(&sched.cond);
    pthread_mutex_unlock(&sched.lock);

    sched.tick = clock;
  }

  return NULL;
}

void sched_timer_init(void)
{
  sem_init(&sched.timer_sem, 0, 0);
  pthread_create(&sched.timer_thread, NULL, timer_thread_func, NULL);

  struct sigaction sa;
  sa.sa_flags = 0;
  sa.sa_handler = timer_signal_handler;
  //sigemptyset(&sa.sa_mask);
  sigaction(SIGUSR1, &sa, NULL);

  struct sigevent se;
  //memset(&se, 0, sizeof(se));
  se.sigev_notify = SIGEV_SIGNAL;
  se.sigev_signo = SIGUSR1;
  timer_create(CLOCK_REALTIME, &se, &sched.timer);

  struct itimerspec ts;
  ts.it_interval.tv_sec = 1;
  ts.it_interval.tv_nsec = 0;
  ts.it_value = ts.it_interval;
  timer_settime(&sched.timer, 0, &ts, NULL);
}

void sched_init(void)
{
  for (int i = 0; i < NR_PRIORITY; i++)
    init_list_head(&sched.readylist[i]);
  init_list_head(&sched.suspendlist);
  init_list_head(&sched.sleeplist);
  pthread_mutex_init(&sched.lock, NULL);
  pthread_cond_init(&sched.cond, NULL);
  sched.idgen = 0;
  sched_timer_init();
}

void schedule(void)
{
  struct thread *thread;
  for (int i = 0; i < NR_THREADS; i++) {
    thread = sched.threads + i;
    sprintf(thread->name, "cpu-%d", i);
    pthread_create(&thread->id, NULL, task_thread_func, thread);
  }
}

void thread_forever(void)
{
  while (1) sleep(60);
}
