/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_TASK_TIMER_H_
#define _KOALA_TASK_TIMER_H_

#include <stdint.h>
#include "util/binheap.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TIME_RESOLUTION_MS 10 // ms

/* timer callback, if it is timeout */
typedef void (*timer_func_t)(void *);

/* timer structure */
typedef struct task_timer {
    BinHeapEntry entry;
    uint64_t tmo;
    timer_func_t func;
    void *arg;
} task_timer_t;

/* initialize timer system */
void init_timer(void);

/* finalize timer system */
void fini_timer(void);

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
