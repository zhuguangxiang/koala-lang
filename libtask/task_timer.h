/*===-- task_timer.h - Timer For Coroutine ------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This header declares coroutine's timer structures and interfaces.          *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_TASK_TIMER_H_
#define _KOALA_TASK_TIMER_H_

#include "binheap.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TIME_RESOLUTION_MS 10 // ms

/* timer callback, if it is timeout */
typedef void (*timer_func_t)(void *);

/* timer structure */
typedef struct task_timer {
    binheap_entry_t entry;
    uint64_t tmo;
    timer_func_t func;
    void *arg;
} task_timer_t;

/* initialize timer system */
void init_timer(void);

/* run one timer */
void timer_start(task_timer_t *tm, uint64_t tmo, timer_func_t func, void *arg);

/* stop one timer */
void timer_stop(task_timer_t *tm);

/* timer epoll function */
void timer_poll(uint64_t trigger);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TASK_TIMER_H_ */
