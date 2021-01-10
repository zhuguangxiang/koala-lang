/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "task.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void *hello(void *arg)
{
    uint64_t id = current_tid();
    uint64_t count = id + 10;
    printf("[proc-%u]task-%lu: hello, world\n", current_pid(), id);
    while (count-- > 0) {
        printf("[proc-%u]task-%lu: alive!!\n", current_pid(), id);
        task_sleep(500);
    }
    printf("[proc-%u]task-%lu: good bye\n", current_pid(), id);
    return NULL;
}

int main(int argc, char *argv[])
{
    init_procs(3);
    for (int i = 0; i < 5; i++) task_create(hello, NULL, NULL);

    int count = 20;
    while (count-- > 0) {
        // printf("[proc-%u]No more tasks\n", current_pid());
        sleep(1);
        task_yield();
    }

    fini_procs();

    return 0;
}
