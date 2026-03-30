/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_EVAL_H_
#define _KOALA_EVAL_H_

#include "codeobject.h"
#include "lldq.h"

#ifdef __cplusplus
extern "C" {
#endif

/* forward declaration */
struct _KoalaState;
struct _ThreadState;
struct _ShadowStack;

/* call stack frame information */
typedef struct _CallFrame {
    /* call stack back frame */
    struct _CallFrame *back;
    /* point back to KoalaState */
    struct _KoalaState *ks;

    /* code for this call */
    CodeObject *code;

    /* module */
    Object *module;

    /* number of locals(include parameters) */
    int local_size;
    /* stack size = temporaries for call to pass arguments */
    int stack_size;

    /* locals */
    Value *locals;
    /* stack pointer */
    Value *stack;
} CallFrame;

/* per koala thread */
typedef struct _KoalaState {
    /* link to _ThreadState or _GlobalState */
    LLDqNode link;
    /* point to _ThreadState */
    struct _ThreadState *ts;

    /* builtin module */
    Object *bltin;
    /* sys module */
    Object *sym;

    /* exception */
    Object *exc;

    /* trace(shadow) stack */
    struct _ShadowStack *shadow_stacks;

    /* top call stack frame */
    CallFrame *cf;

    /* depth of call frames */
    int depth;

    /* stack size */
    int stack_size;

    /* base stack pointer */
    Value *stack_base;

    /* stack top pointer */
    Value *stack_top;

    /* call frame cache */
    CallFrame *cf_cache;
} KoalaState;

Value kl_eval_code(Value *self, Value *args, int nargs, Object *names);

KoalaState *kl_new_ks(void);
void kl_free_ks(KoalaState *ks);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_EVAL_H_ */
