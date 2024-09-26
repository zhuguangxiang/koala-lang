/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "eval.h"
#include "cfuncobject.h"
#include "codeobject.h"
#include "dictobject.h"
#include "exception.h"
#include "mm.h"
#include "moduleobject.h"
#include "opcode.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------DATA-----------------------------------*/

/* max stack size */
#define MAX_STACK_SIZE (16 * 64 * 1024)

/* max call depth, stop for this limit */
#define MAX_CALL_DEPTH 10000

/*-------------------------------------API-----------------------------------*/

static void _copy_arguments(CallFrame *cf, Value *args, int nargs)
{
    ASSERT(cf->local_size >= nargs);

    Value *p = cf->local_stack;
    int total = cf->local_size + cf->stack_size;

    for (int i = 0; i < nargs; i++) {
        *(p + i) = *(args + i);
    }

    for (int i = nargs; i < total; i++) {
        (p + i)->tag = 0;
    }
}

static CallFrame *_new_frame(KoalaState *ks, CodeObject *code)
{
    CallFrame *cf = (CallFrame *)ks->stack_top_ptr;
    ks->stack_top_ptr = ks->stack_top_ptr + sizeof(*cf);

    cf->code = code;
    cf->module = code->module;
    int nlocals = code->cs.nlocals;
    int stack_size = code->cs.stack_size;
    cf->local_size = nlocals;
    cf->stack_size = stack_size;
    cf->stack = cf->local_stack + nlocals;
    ks->stack_top_ptr += sizeof(Value) * (nlocals + stack_size);
    ASSERT(ks->stack_top_ptr <= ks->base_stack_ptr + ks->stack_size);

    return cf;
}

static void _pop_frame(KoalaState *ks, CallFrame *cf)
{
    /* shrink stack */
    ks->stack_top_ptr -= sizeof(*cf) + sizeof(Value) * (cf->stack_size + cf->local_size);
    ASSERT(ks->stack_top_ptr >= ks->base_stack_ptr);
}

KoalaState *ks_new(void)
{
    int msize = sizeof(KoalaState) + MAX_STACK_SIZE;
    KoalaState *ks = mm_alloc(msize);
    lldq_node_init(&ks->link);
    ks->ts = __ts;
    ks->trace_stacks = NULL;
    ks->stack_top_ptr = ks->base_stack_ptr;
    ks->stack_size = MAX_STACK_SIZE;
    return ks;
}

void ks_free(KoalaState *ks)
{
    if (!ks) return;
    ASSERT(!ks->cf);
    ASSERT(ks->trace_stacks == NULL);
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

#define PUSH_INT(v) ((top->tag = VAL_TAG_INT, top->ival = (v)), top++)

#define GET_LOCAL(i)    ({ ASSERT((i) < nlocals); (locals + (i)); })
#define SET_LOCAL(i, v) (ASSERT((i) < nlocals), SET(locals + (i), v))
#define SET_INT_LOCAL(i, v) do { \
    Value *loc = GET_LOCAL(i); \
    loc->tag = VAL_TAG_INT; \
    loc->ival = (v); \
} while (0)

#define DISPATCH() goto dispatch;

/* clang-format on */

static Object *_get_symbol(CallFrame *cf, int m_idx, int o_idx)
{
    ModuleObject *m = (ModuleObject *)cf->module;
    if (m_idx != 0) {
        m = module_get_reloc((Object *)m, m_idx);
    }

    Object *r = module_get_symbol((Object *)m, o_idx);
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
    // Object *kwds = NULL;
    // int kwds_offset = tp->kwds_offset;
    // if (kwds_offset > 0) {
    //     /* object has default kwargs */
    //     kwds = kl_new_dict();
    // }

    Value r = func(obj, args, nargs);
    *result = r;
}

static void _eval_frame(KoalaState *ks, CallFrame *cf, Value *result)
{
    CodeObject *code = (CodeObject *)cf->code;
    uint8_t *first_inst = (uint8_t *)code->cs.insns; // Bytes_Buf(code->codes);
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

            case OP_JMP_INT_CMP_LT_IMM8: {
                int A = NEXT_REG();
                int imm = NEXT_INT8();
                int off = NEXT_INT16();
                Value *ra = GET_LOCAL(A);
                ASSERT(IS_INT(ra));
                if (ra->ival < imm) {
                    // absolute offset
                    next_inst = first_inst + off;
                }
                DISPATCH();
            }

            case OP_JMP_INT_CMP_GE_IMM8: {
                int A = NEXT_REG();
                int imm = NEXT_INT8();
                int off = NEXT_INT16();
                Value *ra = GET_LOCAL(A);
                ASSERT(IS_INT(ra));
                if (ra->ival >= imm) {
                    // absolute offset
                    next_inst = first_inst + off;
                }
                DISPATCH();
            }

            case OP_INT_ADD: {
                int A = NEXT_REG();
                int B = NEXT_REG();
                int C = NEXT_REG();

                Value *rb = GET_LOCAL(B);
                Value *rc = GET_LOCAL(C);
                ASSERT(IS_INT(rb) && IS_INT(rc));
                int64_t r = rb->ival + rc->ival;
                SET_INT_LOCAL(A, r);
                DISPATCH();
            }

            case OP_INT_SUB_IMM8: {
                int A = NEXT_REG();
                int B = NEXT_REG();
                int imm = NEXT_INT8();
                Value *rb = GET_LOCAL(B);
                ASSERT(IS_INT(rb));
                int64_t r = rb->ival - imm;
                SET_INT_LOCAL(A, r);
                DISPATCH();
            }

            case OP_PUSH: {
                int A = NEXT_REG();
                Value *ra = GET_LOCAL(A);
                PUSH(ra);
                DISPATCH();
            }

            case OP_PUSH_IMM8: {
                int imm = NEXT_INT8();
                PUSH_INT(imm);
                DISPATCH();
            }

            case OP_CALL: {
                int mod_idx = NEXT_INT8();
                int func_idx = NEXT_INT8();
                int nargs = NEXT_INT8();
                int A = NEXT_REG();
                Object *callable = _get_symbol(cf, mod_idx, func_idx);
                ASSERT(callable);
                Value *ra = GET_LOCAL(A);
                _call_function(callable, cf->stack, nargs, cf, ra);
                if (IS_ERROR(ra)) {
                    ASSERT(_exc_occurred(ks));
                    goto error;
                }
                SHRINK(nargs);
                DISPATCH();
            }

            case OP_RETURN: {
                int A = NEXT_REG();
                Value *ra = GET_LOCAL(A);
                SET(result, ra);
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

Value kl_eval_code(Object *code, Value *args, int nargs)
{
    KoalaState *ks = __ks();

    /* build a call frame */
    CallFrame *cf = _new_frame(ks, (CodeObject *)code);

    /* copy arguments */
    _copy_arguments(cf, args, nargs);

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
