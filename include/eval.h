/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_EVAL_H_
#define _KOALA_EVAL_H_

#include "lldq.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* forward declaration */
struct _KoalaState;
struct _ThreadState;

/* call stack frame information */
typedef struct _CallFrame {
    /* call stack back frame */
    struct _CallFrame *back;
    /* point back to KoalaState */
    struct _KoalaState *ks;

    /* code for this call */
    Object *code;

    /* module */
    Object *module;

    /* locals + cells + frees */
    int local_size;
    /* value stack size */
    int stack_size;

    /* kwargs */
    Object *kwargs;

    /* value stack pointer */
    Value *stack;
    /* locals and value stack */
    Value *local_stack;

    /* gc roots(c code) */
    Vector gcroots;
} CallFrame;

/* per koala thread */
typedef struct _KoalaState {
    /* link to _ThreadState or _GlobalState */
    LLDqNode link;
    /* point to _ThreadState */
    struct _ThreadState *ts;
    /* top call stack frame */
    CallFrame *cf;
    /* free call frame list */
    CallFrame *free_cf_list;
    /* exception */
    Object *exc;
    /* depth of call frames */
    int depth;
    /* stack size */
    int stack_size;
    /* base stack pointer */
    Value *base_stack_ptr;
    /* stack pointer */
    Value *stack_ptr;
} KoalaState;

Value kl_eval_func(Object *func, Value *args, int nargs, Object *kwargs);

KoalaState *ks_new(void);
void ks_free(KoalaState *ks);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_EVAL_H_ */
