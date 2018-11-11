
#ifndef _KOALA_KTHREAD_H_
#define _KOALA_KTHREAD_H_

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>
#include <ucontext.h>
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STATE_READY    1
#define STATE_RUNNING  2
#define STATE_SUSPEND  3
#define STATE_DEAD     4

#define PRIO_HIGH    0
#define PRIO_NORMAL  1
#define PRIO_LOW     2

#define NAME_SIZE  16
#define NR_PRIORITY 3
#define NR_THREADS  1
#define TASK_STACK_SIZE 8192

struct task {
  char name[NAME_SIZE];
  struct list_head link;
  short state;
  short prio;
  uint64 sleep;
  uint64 id;
  ucontext_t ctx;
  void *thread;
  void (*run)(struct task *);
  void *arg;
};

struct locker {
  pthread_mutex_t lock;
  int locked;
  struct list_head wait_list;
};

struct thread {
  char name[NAME_SIZE];
  pthread_t id;
  ucontext_t ctx;
  struct task *current;
  int runsize;
  struct list_head runlist;
  pthread_mutex_t lock;
  pthread_cond_t cond;
};

struct scheduler {
  struct list_head readylist[NR_PRIORITY];
  struct list_head suspendlist;
  struct list_head sleeplist;
  uint64 idgen;
  sem_t timer_sem;
  timer_t timer;
  uint64 tick;
  uint64 clock;
  pthread_t timer_thread;
  pthread_t mover_thread;
  pthread_mutex_t lock;
  pthread_cond_t cond;
  pthread_mutex_t sleeplock;
  struct thread threads[NR_THREADS];
};

typedef void (*task_func)(struct task *);
int task_init(struct task *tsk, char *name, short prio,
              task_func run, void *arg);
void task_sleep(struct task *tsk, int second);
void task_exit(struct task *tsk);
void task_yield(struct task *tsk);
void task_suspend(struct task *tsk, int second);
void sched_traverse(task_func visit);
void sched_init(void);
void schedule(void);
void thread_forever(void);
void locker_lock(struct locker *locker);
void locker_unlock(struct locker *locker);

#define task_owner_thread(tsk) ((struct thread *)((tsk)->thread))

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_KTHREAD_H_ */
