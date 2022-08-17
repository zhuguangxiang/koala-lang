/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2022-2032 James <zhuguangxiang@gmail.com>
 */

#ifndef _KOALA_EVAL_H_
#define _KOALA_EVAL_H_

#include "func.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

/* forward declaration */
typedef uintptr_t StkVal;
typedef struct _KlFrame KlFrame;
typedef struct _KlState KlState;
typedef struct _KlGlobal KlGlobal;

/* func call frame */
struct _KlFrame {
    /* backtrace call frame */
    KlFrame *back;
    /* previous call frame */
    KlFrame *prev;
    /* function */
    KlFunc *func;
    /* stack info */
    StkVal *base;
    StkVal *top;
};

/* per 'thread' state */
struct _KlState {
    /* linked in global state */
    List ksnode;
    /* call frame */
    KlFrame *cf;
    /* call depth */
    int depth;

    /* stack info */
    StkVal *base;
    StkVal *last;
    StkVal *top;

    /* first call frame */
    KlFrame base_cf;
};

/* global state, shared by all threads */
struct _KlGlobal {
    /* all thread state */
    List states;
};

void kl_evaluate(KlState *ks, KlFrame *kf);
void kl_do_call(KlState *ks, KlFunc *func);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_EVAL_H_ */
