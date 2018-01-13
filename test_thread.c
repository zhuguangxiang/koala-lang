
#include <unistd.h>
#include "thread.h"

void task1_func(struct task *self)
{
  while (1) {
    struct thread *thread = task_owner_thread(self);
    printf("task1 is running in %s\n", thread->name);
    task_sleep(self, 2);
    task_yield(self);
    printf("task1 is finished in %s\n", thread->name);
    task_sleep(self, 1);
  }
}

void task2_func(struct task *self)
{
  while (1) {
    struct thread *thread = task_owner_thread(self);
    printf("task2 is running in %s\n", thread->name);
    task_sleep(self, 2);
    task_yield(self);
    printf("task2 is finished in %s\n", thread->name);
    task_sleep(self, 1);
  }
}

void task3_func(struct task *self)
{
  while (1) {
    struct thread *thread = task_owner_thread(self);
    printf("task3 is running in %s\n", thread->name);
    task_sleep(self, 2);
    task_yield(self);
    printf("task3 is finished in %s\n", thread->name);
    task_sleep(self, 1);
  }
}

void task4_func(struct task *self)
{
  while (1) {
    struct thread *thread = task_owner_thread(self);
    printf("task4 is running in %s\n", thread->name);
    task_sleep(self, 2);
    task_yield(self);
    printf("task4 is finished in %s\n", thread->name);
    task_sleep(self, 1);
  }
}

extern struct scheduler sched;

int main(int argc, char *argv[])
{
  sched_init();

  struct task task1, task2, task3, task4;
  task_init(&task1, PRIO_NORMAL, task1_func, NULL);
  task_init(&task2, PRIO_NORMAL, task2_func, NULL);
  task_init(&task3, PRIO_NORMAL, task3_func, NULL);
  task_init(&task4, PRIO_NORMAL, task4_func, NULL);

  schedule();

  thread_forever();
  return 0;
}
