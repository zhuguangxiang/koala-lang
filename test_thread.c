
#include <unistd.h>
#include "thread.h"

void task1_func(struct task *self)
{
  struct thread *thread;
  int i = 0;
  while (i++ < 20) {
    thread = task_owner_thread(self);
    printf("task1 is running %d times in %s\n", i, thread->name);
    task_sleep(self, 1);
    thread = task_owner_thread(self);
    printf("task1 is running more, yield and continue in %s\n", thread->name);
    task_yield(self);
  }
}

void task2_func(struct task *self)
{
  struct thread *thread;
  int i = 0;
  while (i++ < 20) {
    thread = task_owner_thread(self);
    printf("task2 is running %d times in %s\n", i, thread->name);
    task_sleep(self, 1);
    thread = task_owner_thread(self);
    printf("task2 is running more, yield and continue in %s\n", thread->name);
  }
}

void task3_func(struct task *self)
{
  struct thread *thread;
  int i = 0;
  while (i++ < 20) {
    thread = task_owner_thread(self);
    printf("task3 is running %d times in %s\n", i, thread->name);
    task_sleep(self, 2);
    thread = task_owner_thread(self);
    printf("task3 is running more, yield and continue in %s\n", thread->name);
    task_yield(self);
  }
}

void task4_func(struct task *self)
{
  struct thread *thread;
  int i = 0;
  while (i++ < 20) {
    thread = task_owner_thread(self);
    printf("task4 is running %d times in %s\n", i, thread->name);
    task_sleep(self, 2);
    thread = task_owner_thread(self);
    printf("task4 is running more, yield and continue in %s\n", thread->name);
  }
}

void task5_func(struct task *self)
{
  struct thread *thread;
  int i = 0;
  while (i++ < 20) {
    thread = task_owner_thread(self);
    printf("task5 is running %d times in %s\n", i, thread->name);
    task_sleep(self, 3);
    thread = task_owner_thread(self);
    printf("task5 is running more, yield and continue in %s\n", thread->name);
    task_yield(self);
  }
}

void task6_func(struct task *self)
{
  struct thread *thread;
  int i = 0;
  while (i++ < 20) {
    thread = task_owner_thread(self);
    printf("task6 is running %d times in %s\n", i, thread->name);
    task_sleep(self, 3);
    thread = task_owner_thread(self);
    printf("task6 is running more, yield and continue in %s\n", thread->name);
  }
}

extern struct scheduler sched;

int main(int argc, char *argv[])
{
  UNUSED_PARAMETER(argc);
  UNUSED_PARAMETER(argv);

  sched_init();

  struct task task1, task2, task3, task4, task5, task6;
  task_init(&task1, "task1", PRIO_LOW, task1_func, NULL);
  task_init(&task2, "task2", PRIO_LOW, task2_func, NULL);
  task_init(&task3, "task3", PRIO_NORMAL, task3_func, NULL);
  task_init(&task4, "task4", PRIO_NORMAL, task4_func, NULL);
  task_init(&task5, "task5", PRIO_HIGH, task5_func, NULL);
  task_init(&task6, "task6", PRIO_HIGH, task6_func, NULL);
  schedule();

  thread_forever();
  return 0;
}
