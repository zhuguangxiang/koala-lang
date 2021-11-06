/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_VM_H_
#define _KOALA_VM_H_

#include "object.h"
#include "util/list.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CALL_DEPTH 64
#define MAX_STACK_SIZE 4096

/* forward declaration */

typedef struct _KlState KlState;
typedef struct _KlFrame KlFrame;
typedef struct _KlGlobal KlGlobal;
typedef int (*KlCFunc)(KlState *);

/* func call frame */
struct _KlFrame {
    /* back call frame */
    KlFrame *back;
    /* prev call frame */
    KlFrame *prev;
    /* base pointer */
    KlValue *base;
    /* top pointer */
    KlValue *top;
    /* func pointer */
    KlFunc *func;
    /* saved pc */
    int pc;
};

/* task state */
struct _KlState {
    /* linked in global */
    List ksnode;
    /* call frame stack */
    KlFrame *cf;
    /* frame depth */
    int depth;

    /* stack size */
    int stack_size;
    /* stack base */
    KlValue *base;
    /* first free slot */
    KlValue *top;
    /* last free slot */
    KlValue *last;

    /* first call frame */
    KlFrame base_cf;
};

/* global state, shared by all tasks */
struct _KlGlobal {
    HashMap pkgs;
    List states;
};

/* initial koala state */
void kl_init_state(KlState *ks);

/* call koala(c) function */
void kl_eval_func(KlState *ks, KlFunc *func);

/* finalize koala state */
void kl_fini_state(KlState *ks);

static inline void check_stack(KlState *ks)
{
    if (ks->top < ks->cf->top) {
        printf("error: stack overflow(down)\n");
        abort();
    }

    if (ks->top >= ks->last) {
        printf("error: stack overflow(up)\n");
        abort();
    }
}

/* stack push functions */

static inline void kl_push_int8(KlState *ks, int8 val)
{
    ++ks->top;
    check_stack(ks);
    KlValue _val = { val, int8_vtbl };
    *ks->top = _val;
}

static inline void kl_push_int16(KlState *ks, int16 val)
{
    ++ks->top;
    check_stack(ks);
    KlValue _val = { val, int16_vtbl };
    *ks->top = _val;
}

static inline void kl_push_int32(KlState *ks, int32 val)
{
    ++ks->top;
    check_stack(ks);
    KlValue _val = { val, int32_vtbl };
    *ks->top = _val;
}

static inline void kl_push_int64(KlState *ks, int64 val)
{
    ++ks->top;
    check_stack(ks);
    KlValue _val = { val, int64_vtbl };
    *ks->top = _val;
}

static inline void kl_push_bool(KlState *ks, int8 val)
{
    ++ks->top;
    check_stack(ks);
    KlValue _val = { val, bool_vtbl };
    *ks->top = _val;
}

/* stack pop functions */

static inline int8 kl_pop_int8(KlState *ks)
{
    KlValue _val = *ks->top;
    --ks->top;
    check_stack(ks);
    return (int8)_val.value;
}

static inline int16 kl_pop_int16(KlState *ks)
{
    KlValue _val = *ks->top;
    --ks->top;
    check_stack(ks);
    return (int16)_val.value;
}

static inline int32 kl_pop_int32(KlState *ks)
{
    KlValue _val = *ks->top;
    --ks->top;
    check_stack(ks);
    return (int32)_val.value;
}

static inline int64 kl_pop_int64(KlState *ks)
{
    KlValue _val = *ks->top;
    --ks->top;
    check_stack(ks);
    return (int64)_val.value;
}

static inline int32 kl_pop_char(KlState *ks)
{
    return kl_pop_int32(ks);
}

static inline KlValue kl_pop_value(KlState *ks)
{
    KlValue _val = *ks->top;
    --ks->top;
    check_stack(ks);
    return _val;
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_VM_H_ */
