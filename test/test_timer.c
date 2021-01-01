
#include "task.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void *hello(void *arg)
{
    printf("[proc-%u]task-%lu: hello, world\n", current_pid(), current_tid());
    while (1) {
        printf("[proc-%u]task-%lu: alive!!\n", current_pid(), current_tid());
        task_sleep(1000);
    }
    printf("[proc-%u]task-%lu: good bye\n", current_pid(), current_tid());
    return NULL;
}

int main(int argc, char *argv[])
{
    init_procs(2);
    for (int i = 0; i < 5; i++) task_create(hello, NULL, NULL);

    sleep(1);
    task_yield();

    while (1) {
        printf("[proc-%u]No more tasks\n", current_pid());
        sleep(1);
        task_yield();
    }
    return 0;
}
