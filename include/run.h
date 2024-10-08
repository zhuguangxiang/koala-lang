/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_RUN_H_
#define _KOALA_RUN_H_

#include <pthread.h>
#include "eval.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/* per pthread, thread local storage */
typedef struct _ThreadState {
    /* local running KoalaState list */
    LLDeque run_list;
    /* current running KoalaState in this thread */
    KoalaState *current;
    /* id */
    size_t id;
    /* steal count */
    size_t steal_count;
    /* pthread id */
    pthread_t pid;
    /* state flag */
    int state;
#define TS_RUNNING 0
#define TS_DONE    1
#define TS_WAIT    2 /* waiting for available KoalaState */
#define TS_GC_STW  3 /* waiting for gc has more memory */
} ThreadState;

void kl_init(int argc, char *argv[]);
void kl_run_file(const char *filename);
void kl_fini(void);

/* get current thread */
extern __thread ThreadState *__ts;

/* all loaded modules */
extern HashMap _gs_modules;

/* get current koala state  */
static inline KoalaState *__ks(void) { return __ts->current; }

int check_all_threads_stw(void);
int enum_all_roots(Queue *que);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_RUN_H_ */
