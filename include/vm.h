/*===-- vm.h - Koala Virtual Machine ------------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This header declares koala virtual machine structures and interfaces.      *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_VM_H_
#define _KOALA_VM_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CALL_DEPTH 64
#define MAX_STACK_SIZE 4096

/* forward declaration */
typedef struct CallInfo CallInfo;
typedef struct KoalaState KoalaState;

/* `global state`, shared by all tasks */
typedef struct KoalaGlobal {
} KoalaGlobal;

/* call info */
struct CallInfo {
    /* back call info */
    CallInfo *back;
    /* top for func */
    TValueRef *top;
    /* func object */
    TValueRef *func;
    /* code pc */
    int saved_pc;
};

/* task state structure */
struct KoalaState {
    /* first free slot */
    TValueRef *top;

    /* call info */
    CallInfo *ci;
    /* call depth */
    int nci;

    /* stack size */
    int stacksize;
    /* stack base */
    TValueRef *stack;
    /* last free slot */
    TValueRef *stack_last;

    /* call info first level for c calling koala */
    CallInfo base_ci;
};

/* new koala state */
DLLEXPORT KoalaState *kl_new_state(void);

/* call koala code */
void kl_do_call(KoalaState *ks, int argc);

/* run klc */
void koala_run_file(const char *path);

#define setnilval(v) \
    (v)->_t.tag = 1; \
    (v)->_t.kind = TYPE_NIL

#define setbyteval(v, x)      \
    (v)->_t.tag = 1;          \
    (v)->_t.kind = TYPE_BYTE; \
    (v)->_v.bval = (x)

#define setintval(v, x)      \
    (v)->_t.tag = 1;         \
    (v)->_t.kind = TYPE_INT; \
    (v)->_v.ival = (x)

#define setfltval(v, x)        \
    (v)->_t.tag = 1;           \
    (v)->_t.kind = TYPE_FLOAT; \
    (v)->_v.fval = (x)

#define setboolval(v, x)      \
    (v)->_t.tag = 1;          \
    (v)->_t.kind = TYPE_BOOL; \
    (v)->_v.bval = (x)

#define setcharval(v, x)      \
    (v)->_t.tag = 1;          \
    (v)->_t.kind = TYPE_CHAR; \
    (v)->_v.cval = (x)

#define setstrval(v, x)      \
    (v)->_t.tag = 1;         \
    (v)->_t.kind = TYPE_STR; \
    (v)->_v.obj = (x)

#define setanyval(v, x)      \
    (v)->_t.tag = 1;         \
    (v)->_t.kind = TYPE_ANY; \
    (v)->_v.obj = (x)

#define setptrval(v, x)      \
    (v)->_t.tag = 1;         \
    (v)->_t.kind = TYPE_ANY; \
    (v)->_v.ptr = (x)

#define setobjval(v, _vtbl, _obj) \
    (v)->_t.vtbl = (_vtbl);       \
    (v)->_v.obj = (_obj)

static inline void check_stack(KoalaState *ks)
{
    if (ks->top < ks->ci->top) {
        printf("error: stack overflow(down)\n");
        abort();
    }
    if (ks->top >= ks->stack_last) {
        printf("error: stack overflow(up)\n");
        abort();
    }
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_VM_H_ */
