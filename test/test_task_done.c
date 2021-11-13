/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "task/task.h"

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

    int count = 10;
    while (count-- > 0) {
        // printf("[proc-%u]No more tasks\n", current_pid());
        sleep(1);
        task_yield();
    }

    fini_procs();

    return 0;
}
