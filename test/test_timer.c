/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "common.h"
#include "task.h"
#include "task_timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void *hello(void *arg)
{
    printf("[proc-%u]task-%lu: hello, world\n", current_pid(), current_tid());
    while (1) {
        printf("[proc-%u]task-%lu: alive!!\n", current_pid(), current_tid());
        task_sleep(4000);
    }
    printf("[proc-%u]task-%lu: good bye\n", current_pid(), current_tid());
    return NULL;
}

void *tm1_hello(void *arg)
{
    printf("[proc-%u]task-%lu: hello in timer\n", current_pid(), current_tid());
    return NULL;
}

static void tm1_loop_callback(void *arg)
{
    task_timer_t *tm = arg;
    int count = PTR2INT(tm->arg);
    if (count < 10) {
        task_create(tm1_hello, NULL, NULL);
        count++;
        timer_start(tm, 2000, tm1_loop_callback, INT2PTR(count));
    }
}

int main(int argc, char *argv[])
{
    init_procs(3);

    // for (int i = 0; i < 2; i++) task_create(hello, NULL, NULL);

    task_timer_t tm1 = {};

    int count = 0;
    timer_start(&tm1, 2000, tm1_loop_callback, INT2PTR(count));

    int loop = 40;
    while (loop-- > 0) {
        sleep(1);
        task_yield();
    }

    fini_procs();
    return 0;
}
