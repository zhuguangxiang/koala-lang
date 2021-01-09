/*===-- koala.h - Koala Language C Interfaces ---------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This header declares koala c interfaces.                                   *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_H_
#define _KOALA_H_

#include "stringobject.h"
#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* init koala */
void kl_init(void);

/* fini koala */
void kl_fini(void);

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
    ++ks->top;
    check_stack(ks);
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
    Object *obj = string_new(s);
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
    if (v->_t.tag != 1 && v->_t.kind != TYPE_BYTE) {
        printf("error: not `byte` type.\n");
        abort();
    }
    --ks->top;
    check_stack(ks);
    return v->_v.bval;
}

static inline int64_t kl_pop_int(KoalaState *ks)
{
    TValueRef *v = *ks->top;
    if (v->_t.tag != 1 && v->_t.kind != TYPE_INT) {
        printf("error: not `int` type.\n");
        abort();
    }
    --ks->top;
    check_stack(ks);
    return v->_v.ival;
}

static inline double kl_pop_float(KoalaState *ks)
{
    TValueRef *v = *ks->top;
    if (v->_t.tag != 1 && v->_t.kind != TYPE_FLOAT) {
        printf("error: not `float` type.\n");
        abort();
    }
    --ks->top;
    check_stack(ks);
    return v->_v.fval;
}

static inline int8_t kl_pop_bool(KoalaState *ks)
{
    TValueRef *v = *ks->top;
    if (v->_t.tag != 1 && v->_t.kind != TYPE_BOOL) {
        printf("error: not `bool` type.\n");
        abort();
    }
    --ks->top;
    check_stack(ks);
    return v->_v.zval;
}

static inline int32_t kl_pop_char(KoalaState *ks)
{
    TValueRef *v = *ks->top;
    if (v->_t.tag != 1 && v->_t.kind != TYPE_CHAR) {
        printf("error: not `char` type.\n");
        abort();
    }
    --ks->top;
    check_stack(ks);
    return v->_v.cval;
}

static inline const char *kl_pop_str(KoalaState *ks)
{
    TValueRef v = *ks->top;
    if (v->_t.tag != 1 && v->_t.kind != TYPE_STR) {
        printf("error: not `String` type.\n");
        abort();
    }
    --ks->top;
    check_stack(ks);
    return string_tocstr(v->_v.obj);
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_H_ */
