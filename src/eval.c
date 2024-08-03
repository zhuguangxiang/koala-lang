/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "eval.h"
#include "cfuncobject.h"
#include "codeobject.h"
#include "dictobject.h"
#include "exception.h"
#include "funcobject.h"
#include "mm.h"
#include "moduleobject.h"
#include "opcode.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------DATA-----------------------------------*/

/* max stack size */
#define MAX_STACK_SIZE (64 * 1024)

/* max call depth, stop for this limit */
#define MAX_CALL_DEPTH 10000

/*-------------------------------------API-----------------------------------*/

static CallFrame *_new_frame(KoalaState *ks, FuncObject *func)
{
    CallFrame *cf;
    if (ks->free_cf_list) {
        cf = ks->free_cf_list;
        ks->free_cf_list = cf->back;
    } else {
        cf = mm_alloc_obj_fast(cf);
    }

    vector_init_ptr(&cf->gcroots);

    cf->code = func->code;
    cf->module = func->mod;

    CodeObject *code = (CodeObject *)func->code;
    int nlocals = code->nlocals;
    int stack_size = code->stack_size;
    cf->local_size = nlocals;
    cf->stack_size = stack_size;

    cf->local_stack = ks->stack_ptr;
    cf->stack = cf->local_stack + nlocals;
    ks->stack_ptr += (nlocals + stack_size);
    ASSERT(ks->stack_ptr < ks->base_stack_ptr + ks->stack_size);

    for (int i = 0; i < (nlocals + stack_size); i++) {
        (cf->local_stack + i)->tag = 0;
    }

    return cf;
}

static void _pop_frame(KoalaState *ks, CallFrame *cf)
{
    /* shrink stack */
    ks->stack_ptr -= (cf->stack_size + cf->local_size);
    ASSERT(ks->stack_ptr >= ks->base_stack_ptr);

    /* save to cached free list */
    cf->back = ks->free_cf_list;
    ks->free_cf_list = cf;
}

KoalaState *ks_new(void)
{
    KoalaState *ks = mm_alloc_obj(ks);
    lldq_node_init(&ks->link);
    ks->ts = __ts();

    ks->base_stack_ptr = mm_alloc_fast(MAX_STACK_SIZE * sizeof(Value));
    ks->stack_ptr = ks->base_stack_ptr;
    ks->stack_size = MAX_STACK_SIZE;

    return ks;
}

void ks_free(KoalaState *ks)
{
    ASSERT(!ks->cf);
    CallFrame *cf = ks->free_cf_list;
    CallFrame *next;
    while (cf) {
        next = cf->back;
        mm_free(cf);
        cf = next;
    }
    mm_free(ks->base_stack_ptr);
    mm_free(ks);
}

/* clang-format off */

#define NEXT_REG() ({        \
    uint8_t _v = *next_inst; \
    next_inst++;             \
    _v;                      \
})

#define NEXT_INT8() ({       \
    uint8_t _v = *next_inst; \
    next_inst++;             \
    (int8_t)_v;              \
})

#define NEXT_INT16() ({       \
    uint8_t _v1 = *next_inst; \
    next_inst++;              \
    uint8_t _v2 = *next_inst; \
    next_inst++;              \
    (int)((_v2 << 8) + _v1);  \
})

#define NEXT_OP() do {   \
    opcode = *next_inst; \
    next_inst++;         \
} while (0)

#define SET(x, y)   ((x)->tag = (y)->tag, (x)->obj = (y)->obj)
#define PUSH(x)     (SET(top, x), top++)
#define POP()       (*--top)
#define SHRINK(n)   (top -= (n))

#define GET_LOCAL(i)    ({ ASSERT((i) < nlocals); (locals + (i)); })
#define SET_LOCAL(i, v) (ASSERT((i) < nlocals), SET(locals + (i), v))
#define SET_INT_LOCAL(i, v) do { \
    Value *loc = GET_LOCAL(i); \
    loc->tag = VAL_TAG_INT; \
    loc->ival = (v); \
} while (0)

#define DISPATCH() goto dispatch;

/* clang-format on */

static Object *_get_symbol(CallFrame *cf, int mod_idx, int obj_idx)
{
    Object *m = module_get_reloc(cf->module, mod_idx);
    ASSERT(m && IS_MODULE(m));
    Object *r = module_get_symbol(m, obj_idx);
    ASSERT(r && object_is_callable(r));
    return r;
}

static void _call_function(Object *obj, Value *args, int nargs, CallFrame *cf,
                           Value *result)
{
    TypeObject *tp = OB_TYPE(obj);
    CallFunc func = tp->call;
    if (!func) {
        _raise_exc(cf->ks, "object is not callable");
        *result = ErrorValue;
    }

    /* process default key-value arguments */
    Object *kwargs = NULL;
    int kwargs_offset = tp->kwargs_offset;
    if (kwargs_offset > 0) {
        /* object has default kwargs */
        kwargs = kl_new_dict();
    }
    cf->kwargs = NULL;

    Value r = func(obj, args, nargs, kwargs);
    *result = r;
}

static void _eval_frame(KoalaState *ks, CallFrame *cf, Value *result)
{
    CodeObject *code = (CodeObject *)cf->code;
    uint8_t *first_inst = (uint8_t *)code->insns; // Bytes_Buf(code->codes);
    uint8_t *next_inst = first_inst;
    Value *top = cf->stack;
    Value *locals = cf->local_stack;
    int nlocals = cf->local_size;
    int opcode;

    /* push frame */
    cf->back = ks->cf;
    cf->ks = ks;
    ks->cf = cf;
    ++ks->depth;

    ASSERT(ks->depth <= MAX_CALL_DEPTH);

main_loop:
    for (;;) {
    dispatch:
        NEXT_OP();
    dispatch_opcode:
        switch (opcode) {
            case OP_CONST_INT_0: {
                DISPATCH();
            }

            case OP_JMP_INT_CMP_GE_IMM8: {
                int A = NEXT_REG();
                int imm = NEXT_INT8();
                int off = NEXT_INT16();
                Value *va = GET_LOCAL(A);
                ASSERT(IS_INT(va));
                if (va->ival >= imm) {
                    // absolute offset
                    next_inst = first_inst + off;
                }
                DISPATCH();
            }

            case OP_INT_ADD: {
                int A = NEXT_REG();
                int B = NEXT_REG();
                int C = NEXT_REG();

                Value *vb = GET_LOCAL(B);
                Value *vc = GET_LOCAL(C);
                ASSERT(IS_INT(vb) && IS_INT(vc));
                int64_t r = vb->ival + vc->ival;
                SET_INT_LOCAL(A, r);
                DISPATCH();
            }

            case OP_INT_SUB_IMM8: {
                int A = NEXT_REG();
                int B = NEXT_REG();
                int imm = NEXT_INT8();
                Value *vb = GET_LOCAL(B);
                ASSERT(IS_INT(vb));
                int64_t r = vb->ival - imm;
                SET_INT_LOCAL(A, r);
                DISPATCH();
            }

            case OP_PUSH: {
                int A = NEXT_REG();
                Value *va = GET_LOCAL(A);
                PUSH(va);
                DISPATCH();
            }

            case OP_CALL: {
                int mod_idx = NEXT_INT8();
                int func_idx = NEXT_INT8();
                int nargs = NEXT_INT8();
                int A = NEXT_REG();
                Object *callable = _get_symbol(cf, mod_idx, func_idx);
                ASSERT(callable);
                Value *va = GET_LOCAL(A);
                _call_function(callable, cf->stack, nargs, cf, va);
                if (IS_ERROR(va)) {
                    ASSERT(_exc_occurred(ks));
                    goto error;
                }
                SHRINK(nargs);
                DISPATCH();
            }

            case OP_RETURN: {
                int A = NEXT_REG();
                Value *r = GET_LOCAL(A);
                SET(result, r);
                goto done;
            }

            case OP_RETURN_NONE: {
                *result = NoneValue;
                goto done;
            }

            default: {
                UNREACHABLE();
                break;
            }
        } /* switch */

        /* This should never be reached here! */
        UNREACHABLE();
    } /* main_loop */

error:

    /* log traceback info */

    /* finish the loop as we have an error. */

done:

    /* pop frame */
    ks->cf = cf->back;
    --ks->depth;
}

Value kl_eval_func(Object *func, Value *args, int nargs, Object *kwargs)
{
    ASSERT(IS_FUNC(func));

    KoalaState *ks = __ks();

    /* build a call frame */
    CallFrame *cf = _new_frame(ks, (FuncObject *)func);

    /* copy arguments */
    ASSERT(cf->local_size >= nargs);
    for (int i = 0; i < nargs; i++) {
        *(cf->local_stack + i) = *(args + i);
    }
    cf->kwargs = kwargs;

    /* eval the call frame */
    Value result = { 0 };
    _eval_frame(ks, cf, &result);

    /* pop frame to free list */
    _pop_frame(ks, cf);

    return result;
}

#ifdef __cplusplus
}
#endif
