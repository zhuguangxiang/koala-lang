
#include "task.h"
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

void *func2(void *p)
{
  volatile int* value = (int*)p;
  *value += 1;
  task_t *curr = current_task();
  printf("[%d-%lu]: %d\n", current_processor()->id, curr->id, *value);
  task_yield();
  *value += 1;
  curr = current_task();
  printf("[%d-%lu]: %d\n", current_processor()->id, curr->id, *value);
  task_yield();
  *value += 1;
  curr = current_task();
  printf("[%d-%lu]: %d\n", current_processor()->id, curr->id, *value);
  task_yield();
  *value += 1;
  curr = current_task();
  printf("[%d-%lu]: %d\n", current_processor()->id, curr->id, *value);
  task_yield();
  *value += 1;
  curr = current_task();
  printf("[%d-%lu]: %d\n", current_processor()->id, curr->id, *value);
  task_yield();
  *value += 1;
  curr = current_task();
  printf("[%d-%lu]: %d\n", current_processor()->id, curr->id, *value);
  printf(">>>%lu\n", (intptr_t)(*(int **)value));
  return *(int **)value;
}

task_t *t2;

void *func1(void *p)
{
  task_attr_t attr = {.stacksize = 1 * 1024 * 1024};
  uint64_t val = 0;
  task_join(t2, (void **)&val);
  printf("task-%lu is joined, val:%lu\n", t2->id, val);

  int* value = (int*)p;
  *value += 1;
  task_t *curr = current_task();
  printf("[%d-%lu]: %d\n", current_processor()->id, curr->id, *value);
  task_yield();
  *value += 1;
  curr = current_task();
  printf("[%d-%lu]: %d\n", current_processor()->id, curr->id, *value);
  task_yield();
  *value += 1;
  curr = current_task();
  printf("[%d-%lu]: %d\n", current_processor()->id, curr->id, *value);
  *value += 1;
  curr = current_task();
  printf("[%d-%lu]: %d\n", current_processor()->id, curr->id, *value);
  *value += 1;
  curr = current_task();
  printf("[%d-%lu]: %d\n", current_processor()->id, curr->id, *value);
  *value += 1;
  curr = current_task();
  printf("[%d-%lu]: %d\n", current_processor()->id, curr->id, *value);
  printf("<<<%lu\n", (intptr_t)(*(int **)value));
  return *(int **)value;
}

int main(int argc, char *argv[])
{
  task_init_procs(3);
  int value1 = 100;
  int value2 = 200;
  task_attr_t attr = {.stacksize = 1 * 1024 * 1024};
  task_t *t = task_create(&attr, func1, &value1);
  t2 = task_create(&attr, func2, &value2);

  sleep(1);

  int count = 0;
  while (count++ < 3) {
    task_t *curr = current_task();
    printf("main processor...[%d, %lu]\n", current_processor()->id, curr->id);
    sleep(2);
  }
  return 0;
}
