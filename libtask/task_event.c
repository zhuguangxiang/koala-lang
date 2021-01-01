/*===-- task_event.c - Event For Coroutine ------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This file implements coroutine's events.                                   *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#define _POSIX_C_SOURCE 199309L
#include "task_event.h"
#include "task_timer.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

static int eventfd;
static int timerfd;

typedef void (*event_func_t)(void *);

void event_poll(void)
{
    struct epoll_event events[64];
    const int count = epoll_wait(eventfd, events, 64, -1);
    if (count < 0) {
        if (errno == EINTR) {
            // interrupted, just try again later (could be gdb'ing etc)
            return;
        }
        abort();
    }

    for (int i = 0; i < count; i++) {
        const int fd = events[i].data.fd;
        if (fd == timerfd) {
            uint64_t timer_count = 0;
            int ret = read(timerfd, &timer_count, sizeof(timer_count));
            if (ret != sizeof(timer_count)) {
                assert(errno == EWOULDBLOCK || errno == EAGAIN);
                continue;
            }
            timer_poll(timer_count);
        }
    }
}

void init_event(void)
{
    timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    assert(timerfd >= 0);
    struct itimerspec in = {};
    in.it_interval.tv_nsec = TIME_RESOLUTION_MS * 1000000;
    in.it_value.tv_nsec = TIME_RESOLUTION_MS * 1000000;
    int ret = timerfd_settime(timerfd, 0, &in, NULL);
    assert(!ret);

    eventfd = epoll_create(1);
    assert(eventfd >= 0);
    struct epoll_event e = {};
    e.events = EPOLLIN | EPOLLET;
    e.data.fd = timerfd;
    ret = epoll_ctl(eventfd, EPOLL_CTL_ADD, timerfd, &e);
    assert(!ret);
}

#ifdef __cplusplus
}
#endif
