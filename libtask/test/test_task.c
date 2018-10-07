
#include "task.h"
#include <stdio.h>
#include <unistd.h>

void *f1(void *p)
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
  *value += 1;
  curr = task_self();
  printf("[%d-%lu]: %d\n", current_scheduler()->id, curr->id, *value);
  *value += 1;
  curr = task_self();
  printf("[%d-%lu]: %d\n", current_scheduler()->id, curr->id, *value);
  *value += 1;
  curr = task_self();
  printf("[%d-%lu]: %d\n", current_scheduler()->id, curr->id, *value);
  return NULL;
}

int main(int argc, char *argv[])
{
  task_scheduler_init(2);
  int value1 = 100;
  int value2 = 200;
  int value3 = 300;
  int value4 = 400;
  int value5 = 500;
  int value6 = 600;
  task_attr_t attr = {.stacksize = 1 * 1024 * 1024};
  task_create(&attr, f1, &value1);
  task_create(&attr, f1, &value2);
  task_create(&attr, f1, &value3);
  task_create(&attr, f1, &value4);
  task_create(&attr, f1, &value5);
  task_create(&attr, f1, &value6);
  task_yield();
  task_yield();
  int count = 0;
  while (count++ < 5) {
    sleep(1);
    task_t *curr = task_self();
    printf("main thread...%d, %lu\n", current_scheduler()->id, curr->id);
  }
  return 0;
}
