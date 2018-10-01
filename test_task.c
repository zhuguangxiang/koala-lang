
#include "task_scheduler.h"
#include <unistd.h>


void *f1(void *p)
{
  int* value = (int*)p;
  *value += 1;
  task_scheduler_t *curr = current_scheduler();
  printf("%s: 1 value : %d\n", curr->id, *value);
  task_yield();
  *value += 1;
  curr = current_scheduler();
  printf("%s: 2 value : %d\n", curr->id, *value);
  //task_t *crr = current_task();
  //ucontext_t *uctx = crr->context.ctx_stack_ptr;
  //uctx->uc_link = curr->idle->context.ctx_stack_ptr;
  //printf("%p\n", uctx->uc_link);
  return NULL;
}

int main(int argc, char *argv[])
{
  task_scheduler_init();
  int value = 100;
  int value2 = 200;
  task_create(f1, &value2);
  task_create(f1, &value);
  sleep(3);
    printf("h1\n");
    task_yield();
    printf("h2\n");
    assert(value == 101);
    printf("h3\n");
    task_yield();
    printf("h4\n");
    assert(value == 102);
    int count = 0;
    while (count++ < 5) {
      sleep(2);
      printf("main thread\n");
    }
  return 0;
}
