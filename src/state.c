/*===-- state.c - Koala Global & Thread State ---------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This header implements interfaces of global & thread state.                *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "state.h"

#ifdef __cplusplus
extern "C" {
#endif

DLLEXPORT KoalaState *koala_newstate(void)
{
    KoalaState *ks;
    CallInfo *ci;

    /* create new koalastate */
    ks = mm_alloc(sizeof(*ks));

    /* initialize stack  */
    ks->stacksize = STACK_MINIMUM_SIZE;
    ks->stack = mm_alloc(ks->stacksize * sizeof(kl_value_t));
    ks->stack_last = ks->stack + ks->stacksize;
    ks->top = ks->stack;

    /* initialize first ci */
    ci = &ks->base_ci;
    ci->back = NULL;
    ci->func = ks->top;
    setnilval(ks->top); /* `func` entry for this `ci` */
    ++ks->top;
    ci->top = ks->top;

    ks->ci = ci;
    ks->nci = 1;

    return ks;
}

#define check_stack(ks)                       \
    {                                         \
        if (ks->top < ks->ci->top) {          \
            printf("stack overflow(down)\n"); \
            abort();                          \
        }                                     \
        if (ks->top >= ks->stack_last) {      \
            printf("stack overflow(up)\n");   \
            abort();                          \
        }                                     \
    }

DLLEXPORT void koala_pushnil(KoalaState *ks)
{
    setnilval(ks->top);
    ++ks->top;
    check_stack(ks);
}

DLLEXPORT void koala_pushbyte(KoalaState *ks, int8_t bval)
{
    setbyteval(ks->top, bval);
    ++ks->top;
    check_stack(ks);
}

DLLEXPORT void koala_pushint(KoalaState *ks, int64_t ival)
{
    setintval(ks->top, ival);
    ++ks->top;
    check_stack(ks);
}

DLLEXPORT void koala_pushfloat(KoalaState *ks, double fval)
{
    setfltval(ks->top, fval);
    ++ks->top;
    check_stack(ks);
}

DLLEXPORT void koala_pushbool(KoalaState *ks, int8_t zval)
{
    setboolval(ks->top, zval);
    ++ks->top;
    check_stack(ks);
}

DLLEXPORT void koala_pushchar(KoalaState *ks, int32_t cval)
{
    setcharval(ks->top, cval);
    ++ks->top;
    check_stack(ks);
}

DLLEXPORT void koala_pushstr(KoalaState *ks, const char *s)
{
    Object *obj = string_new(s);
    setstrval(ks->top, obj);
    ++ks->top;
    check_stack(ks);
}

static kl_value_t *index2value(KoalaState *ks, int idx)
{
    kl_value_t *val = NULL;
    CallInfo *ci = ks->ci;

    if (idx > 0) {
        /* index by ci->func */
        val = ci->func + idx;
        if (val >= ci->top) {
            printf("error: invalid index: %d\n", idx);
            abort();
        }
    }
    else {
        /* non-positive index by ks->top */
        val = ks->top + idx;
        if (val < ci->top) {
            printf("error: invalid index: %d\n", idx);
            abort();
        }
    }
    return val;
}

DLLEXPORT int8_t koala_tobyte(KoalaState *ks, int idx)
{
    const kl_value_t *val = index2value(ks, idx);
    return val->bval;
}

DLLEXPORT int64_t koala_toint(KoalaState *ks, int idx)
{
    const kl_value_t *val = index2value(ks, idx);
    return val->ival;
}

DLLEXPORT double koala_tofloat(KoalaState *ks, int idx)
{
    const kl_value_t *val = index2value(ks, idx);
    return val->fval;
}

DLLEXPORT int8_t koala_tobool(KoalaState *ks, int idx)
{
    const kl_value_t *val = index2value(ks, idx);
    return val->bval;
}

DLLEXPORT int32_t koala_tochar(KoalaState *ks, int idx)
{
    const kl_value_t *val = index2value(ks, idx);
    return val->cval;
}

DLLEXPORT const char *koala_tostr(KoalaState *ks, int idx)
{
    const kl_value_t *val = index2value(ks, idx);
    return string_tocstr(val->obj);
}

DLLEXPORT int koala_gettop(KoalaState *ks)
{
    return ks->top - ks->ci->top;
}

#ifdef __cplusplus
}
#endif
