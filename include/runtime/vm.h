/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_VM_H_
#define _KOALA_VM_H_

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
    CodeObject *code;

    /* module */
    Object *module;

    /* program counter for this call */
    uint32_t pc;

    /* number of locals(include parameters) */
    int nlocals;

    /* locals */
    TValue *locals;
} CallFrame;

/* per koala thread */
typedef struct _KoalaState {
    /* top call stack frame */
    CallFrame *cf;
    /* depth of call frames */
    int depth;

    /* stack size */
    int stack_size;
    /* base stack pointer */
    TValue *stack_base;

    /* stack top pointer */
    TValue *stack_top;

    /* call frame cache */
    CallFrame *cf_cache;

    /* builtin module */
    Object *builtin;
    /* sys module */
    Object *sys;

    /* exception */
    Object *exc;

    /* point to _ThreadState */
    struct _ThreadState *ts;
} KoalaState;

/* per pthread, thread local storage */
typedef struct _ThreadState {
    /* current running KoalaState in this thread */
    KoalaState *current;
} ThreadState;

/* get current thread */
extern __thread ThreadState *__ts;

/* get current koala state  */
static inline KoalaState *__ks(void) { return __ts->current; }

KoalaState *kl_new_ks(void);
void kl_free_ks(KoalaState *ks);

void _raise_exc_fmt(KoalaState *ks, char *fmt, ...);
void _raise_exc_str(KoalaState *ks, char *str);
#define _exc_occurred(ks) ((ks)->exc != NULL)
void kl_trace_here(CallFrame *cf);

void _print_exc(KoalaState *ks);

/* clang-format off */

#define print_exc() do {     \
    KoalaState *ks = __ks(); \
    _print_exc(ks);          \
} while (0)

#define raise_exc_fmt(fmt, args...) do { \
    KoalaState *ks = __ks();             \
    _raise_exc_fmt(ks, fmt, args);       \
} while(0)

#define raise_exc_str(str) do { \
    KoalaState *ks = __ks();    \
    _raise_exc_str(ks, str);    \
} while(0)

#define exc_occurred() ({       \
    KoalaState *ks = __ks();    \
    _exc_occurred(ks);          \
})

/* clang-format on */

void kl_trace_back(KoalaState *ks);

void init_builtin_module(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_VM_H_ */
