
#include "task.h"
#include <stdio.h>
#include <stdlib.h>

void *hello(void *arg)
{
    printf("task-%lu: hello, world\n", current_task()->id);
    task_yield();
    return NULL;
}

int main(int argc, char *argv[])
{
    init_task_procs();
    task_create(hello, NULL);
    task_create(hello, NULL);
    task_create(hello, NULL);
    task_yield();

    /* !! NEVER go here !! */
    abort();
    return 0;
}