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

/* do call */
void kl_do_call(KoalaState *ks);

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

/* stack push functions */

static inline void kl_push_nil(KoalaState *ks)
{
    ++ks->top;
    check_stack(ks);
    setnilval(ks->top);
}

static inline void kl_push_byte(KoalaState *ks, int8_t bval)
{
    ++ks->top;
    check_stack(ks);
    setbyteval(ks->top, bval);
}

static inline void kl_push_int(KoalaState *ks, int64_t ival)
{
    ++ks->top;
    check_stack(ks);
    setintval(ks->top, ival);
}

static inline void kl_push_float(KoalaState *ks, double fval)
{
    ++ks->top;
    check_stack(ks);
    setfltval(ks->top, fval);
}

static inline void kl_push_bool(KoalaState *ks, int8_t zval)
{
    ++ks->top;
    check_stack(ks);
    setboolval(ks->top, zval);
}

static inline void kl_push_char(KoalaState *ks, int32_t cval)
{
    ++ks->top;
    check_stack(ks);
    setcharval(ks->top, cval);
}

static inline void kl_push_str(KoalaState *ks, const char *s)
{
    ++ks->top;
    check_stack(ks);
    Object *obj = NULL; // string_new(s);
    setstrval(ks->top, obj);
}

static inline void kl_push_func(KoalaState *ks, Object *meth)
{
    ++ks->top;
    check_stack(ks);
    setobjval(ks->top, NULL, meth);
}

/* stack pop functions */

static inline int8_t kl_pop_byte(KoalaState *ks)
{
    TValueRef v = *ks->top;
    if (v._t.tag != 1 && v._t.kind != TYPE_BYTE) {
        printf("error: not `byte` type.\n");
        abort();
    }
    --ks->top;
    check_stack(ks);
    return v._v.bval;
}

static inline int64_t kl_pop_int(KoalaState *ks)
{
    TValueRef v = *ks->top;
    if (v._t.tag != 1 && v._t.kind != TYPE_INT) {
        printf("error: not `int` type.\n");
        abort();
    }
    --ks->top;
    check_stack(ks);
    return v._v.ival;
}

static inline double kl_pop_float(KoalaState *ks)
{
    TValueRef v = *ks->top;
    if (v._t.tag != 1 && v._t.kind != TYPE_FLOAT) {
        printf("error: not `float` type.\n");
        abort();
    }
    --ks->top;
    check_stack(ks);
    return v._v.fval;
}

static inline int8_t kl_pop_bool(KoalaState *ks)
{
    TValueRef v = *ks->top;
    if (v._t.tag != 1 && v._t.kind != TYPE_BOOL) {
        printf("error: not `bool` type.\n");
        abort();
    }
    --ks->top;
    check_stack(ks);
    return v._v.zval;
}

static inline int32_t kl_pop_char(KoalaState *ks)
{
    TValueRef v = *ks->top;
    if (v._t.tag != 1 && v._t.kind != TYPE_CHAR) {
        printf("error: not `char` type.\n");
        abort();
    }
    --ks->top;
    check_stack(ks);
    return v._v.cval;
}

static inline const char *kl_pop_str(KoalaState *ks)
{
    TValueRef v = *ks->top;
    if (v._t.tag != 1 && v._t.kind != TYPE_STR) {
        printf("error: not `String` type.\n");
        abort();
    }
    --ks->top;
    check_stack(ks);
    return NULL; // string_tocstr(v->_v.obj);
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_VM_H_ */
