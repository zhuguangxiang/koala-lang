/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_RUN_H_
#define _KOALA_RUN_H_

#include <pthread.h>
#include "eval.h"

#ifdef __cplusplus
extern "C" {
#endif

void kl_init(int argc, const char *argv[]);
void kl_run(const char *filename);
void kl_fini(void);

extern int __nthreads;
extern pthread_key_t __local_key;

/* get current thread  */
static inline ThreadState *__ts(void)
{
    return pthread_getspecific(__local_key);
}

/* get current koala state  */
static inline KoalaState *__ks(void) { return __ts()->current; }

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_RUN_H_ */