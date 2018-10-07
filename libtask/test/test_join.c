
#include "task.h"
#include <stdio.h>
#include <unistd.h>

void *f2 (void *p)
{
  int* value = (int*)p;
  *value += 1;
  task_t *curr = task_self();
  printf("[%d-%lu]: %d\n", current_scheduler()->id, curr->id, *value);
  task_yield();
  *value += 1;
  curr = task_self();
  printf("[%d-%lu]: %d\n", current_scheduler()->id, curr->id, *value);
  task_yield();
  *value += 1;
  curr = task_self();
  printf("[%d-%lu]: %d\n", current_scheduler()->id, curr->id, *value);
  task_yield();
  *value += 1;
  curr = task_self();
  printf("[%d-%lu]: %d\n", current_scheduler()->id, curr->id, *value);
  task_yield();
  *value += 1;
  curr = task_self();
  printf("[%d-%lu]: %d\n", current_scheduler()->id, curr->id, *value);
  task_yield();
  *value += 1;
  curr = task_self();
  printf("[%d-%lu]: %d\n", current_scheduler()->id, curr->id, *value);
  return (void *)(*(int **)value);
}

void *f1(void *p)
{
  task_attr_t attr = {.stacksize = 1 * 1024 * 1024};
  int value3 = 300;
  task_t *t = task_create(&attr, f2, &value3);
  sleep(1);
  printf("start to join task-%lu\n", t->id);
  uint64_t val = 0;
  task_join(t, (void **)&val);
  printf("task-%lu is joined, val:%lu\n", t->id, val);

  int* value = (int*)p;
  *value += 1;
  task_t *curr = task_self();
  printf("[%d-%lu]: %d\n", current_scheduler()->id, curr->id, *value);
  task_yield();
  *value += 1;
  curr = task_self();
  printf("[%d-%lu]: %d\n", current_scheduler()->id, curr->id, *value);
  task_yield();
  *value += 1;
  curr = task_self();
  printf("[%d-%lu]: %d\n", current_scheduler()->id, curr->id, *value);
  *value += 1;
  curr = task_self();
  printf("[%d-%lu]: %d\n", current_scheduler()->id, curr->id, *value);
  *value += 1;
  curr = task_self();
  printf("[%d-%lu]: %d\n", current_scheduler()->id, curr->id, *value);
  *value += 1;
  curr = task_self();
  printf("[%d-%lu]: %d\n", current_scheduler()->id, curr->id, *value);
  return *(int **)value;
}

int main(int argc, char *argv[])
{
  task_scheduler_init(3);
  int value1 = 100;
  //int value2 = 200;
  task_attr_t attr = {.stacksize = 1 * 1024 * 1024};
  task_t *t = task_create(&attr, f1, &value1);
  sleep(2);
  int count = 0;
  while (count++ < 5) {
    task_t *curr = task_self();
    printf("main thread...%d, %lu\n", current_scheduler()->id, curr->id);
    sleep(1);
  }
  return 0;
}
