/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "eval.h"
#include "cfuncobject.h"
#include "codeobject.h"
#include "dictobject.h"
#include "exception.h"
#include "mm.h"
#include "moduleobject.h"
#include "opcode2.h"
#include "shadowstack.h"
#include "tupleobject.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------DATA-----------------------------------*/

/* max stack size */
#define MAX_STACK_SIZE (64 * 1024)

/* max call depth, stop for this limit */
#define MAX_CALL_DEPTH 10000

/*-------------------------------------API-----------------------------------*/

static inline void _copy_arguments(CallFrame *cf, Value *args, int nargs)
{
    ASSERT(cf->local_size >= nargs);
    Value *p = cf->locals;
    for (int i = 0; i < nargs; i++) {
        *(p + i) = *(args + i);
    }
}

static CallFrame *_new_frame(KoalaState *ks, CodeObject *code)
{
    CallFrame *cf;

    if (ks->cf_cache) {
        cf = ks->cf_cache;
        ks->cf_cache = cf->back;
    } else {
        cf = mm_alloc_obj(cf);
    }

    cf->code = code;
    cf->module = code->module;
    cf->local_size = code->nlocals;
    cf->stack_size = code->max_nargs;

    /* TODO: grow stack if needed */

    cf->locals = ks->stack_top;
    cf->stack = cf->locals + cf->local_size;
    ks->stack_top = cf->locals + cf->local_size + cf->stack_size;
    ASSERT(ks->stack_top <= ks->stack_base + ks->stack_size);

    return cf;
}

static void _pop_frame(KoalaState *ks, CallFrame *cf)
{
    /* shrink stack */
    ks->stack_top -= (cf->local_size + cf->stack_size);
    ASSERT(ks->stack_top >= ks->stack_base);

    cf->back = ks->cf_cache;
    ks->cf_cache = cf;
    // mm_free(cf);
}

KoalaState *kl_new_ks(void)
{
    KoalaState *ks = mm_alloc_obj(ks);
    lldq_node_init(&ks->link);
    ks->ts = __ts;
    ks->stack_base = mm_alloc(sizeof(Value) * MAX_STACK_SIZE);
    ks->stack_top = ks->stack_base;
    ks->stack_size = MAX_STACK_SIZE;
    return ks;
}

void kl_free_ks(KoalaState *ks)
{
    if (!ks) return;
    ASSERT(!ks->cf);
    ASSERT(ks->shadow_stacks == NULL);
    mm_free(ks->stack_base);
    mm_free(ks);
}

/* clang-format off */

// [Op:8] [A:8] [B:8] [C:8]
// [Op:8] [A:12] [B:12]
// [Op:8] [A:8] [B:16]

#define I_OP(i)  ((i) >> 24)

#define I_A(i)   (((i) >> 16) & 0xFF)
#define I_B(i)   (((i) >> 8) & 0xFF)
#define I_C(i)   ((i) & 0xFF)

#define I_Ax(i)  (((i) >> 12) & 0x0FFF)
#define I_Bx(i)  ((i) & 0x0FFF)

#define I_Bxx(i) ((i) & 0xFFFF)

#define CP(i)    (*(Value *)vector_get_ptr(consts, i))
#define RELOC(i) \
    ({ RelocEntry *rel = vector_get_ptr(rels, i); ASSERT(rel); rel->obj; })

#define PUSH(x)     ({ *top++ = (x); ASSERT(top <= cf->stack + cf->stack_size); })
#define POP()       ({ --top; ASSERT(top >= cf->stack); *top; })
#define SHRINK(n)   ({ top -= (n); ASSERT(top >= cf->stack); })

#define DISPATCH() goto dispatch;

/* clang-format on */

static Value _eval_frame(KoalaState *ks, CallFrame *cf)
{
    CodeObject *code = cf->code;
    ModuleObject *m = (ModuleObject *)cf->module;
    Vector *consts = &m->consts;
    Vector *rels = &m->rels;
    Value *top = cf->stack;
    Value *regs = cf->locals;

    uint32_t *pc = (uint32_t *)code->insns;
    Value result = none_value;
    register uint32_t inst;
    register OpCode op;
    register int rd, rs, rt, imm, idx, off;

    /* push frame */
    cf->back = ks->cf;
    cf->ks = ks;
    ks->cf = cf;
    ++ks->depth;

    ASSERT(ks->depth <= MAX_CALL_DEPTH);

main_loop:
    for (;;) {
    dispatch:
        inst = *pc++;
        op = I_OP(inst);
    dispatch_opcode:
        switch (op) {
            case OP_MOVE: {
                rd = I_Ax(inst);
                rs = I_Bx(inst);

                ASSERT(rd < cf->local_size);
                ASSERT(rs < cf->local_size);

                regs[rd] = regs[rs];
                DISPATCH();
            }

            case OP_CONST: {
                rd = I_Ax(inst);
                idx = I_Bx(inst);

                ASSERT(rd < cf->local_size);

                regs[rd] = CP(idx);
                DISPATCH();
            }

            case OP_CONST_INT_0: {
                rd = I_Ax(inst);

                ASSERT(rd < cf->local_size);

                regs[rd].tag = TAG_INT64;
                regs[rd].ival = 0;
                DISPATCH();
            }

            case OP_CONST_INT_IMM: {
                rd = I_A(inst);
                imm = I_Bxx(inst);

                ASSERT(rd < cf->local_size);

                regs[rd].tag = TAG_INT64;
                regs[rd].ival = imm;
                DISPATCH();
            }

            case OP_JMP_INT_CMP_LT_IMM: {
                rd = I_A(inst);
                imm = I_B(inst);
                off = I_C(inst);

                ASSERT(rd < cf->local_size);

                if (regs[rd].ival < imm) {
                    pc += off;
                }
                DISPATCH();
            }

            case OP_JMP_INT_CMP_GE_IMM: {
                rd = I_A(inst);
                imm = I_B(inst);
                off = I_C(inst);

                ASSERT(rd < cf->local_size);

                if (regs[rd].ival >= imm) {
                    pc += off;
                }
                DISPATCH();
            }

            case OP_INT_ADD: {
                rd = I_A(inst);
                rs = I_B(inst);
                rt = I_C(inst);

                ASSERT(rd < cf->local_size);
                ASSERT(rs < cf->local_size);
                ASSERT(rt < cf->local_size);

                ASSERT(regs[rs].tag == regs[rt].tag);
                ASSERT(regs[rs].tag == TAG_INT64 || regs[rs].tag == TAG_UINT64);

                regs[rd].ival = regs[rs].ival + regs[rt].ival;
                regs[rd].tag = regs[rs].tag;
                DISPATCH();
            }

            case OP_INT_SUB_IMM: {
                rd = I_A(inst);
                rs = I_B(inst);
                imm = I_C(inst);

                ASSERT(rd < cf->local_size);
                ASSERT(rs < cf->local_size);

                ASSERT(regs[rs].tag == TAG_INT64 || regs[rs].tag == TAG_UINT64);

                regs[rd].ival = regs[rs].ival - imm;
                regs[rd].tag = regs[rs].tag;
                DISPATCH();
            }

            case OP_PUSH: {
                rs = I_Bx(inst);

                ASSERT(rs < cf->local_size);

                PUSH(regs[rs]);
                DISPATCH();
            }

            case OP_ARG_INT_IMM: {
                imm = I_Bxx(inst);
                PUSH(int64_value(imm));
                DISPATCH();
            }

            case OP_CALL: {
                rd = I_A(inst);
                imm = I_B(inst);

                off = I_C(inst);
                Object *obj = RELOC(off);

                ASSERT(obj);
                Value callable = obj_value(obj);
                int nargs = imm;
                Value ret = object_call(&callable, cf->stack, nargs);
                if (is_error(&ret)) {
                    ASSERT(_exc_occurred(ks));
                    result = ret;
                    goto error;
                }

                ASSERT(rd < cf->local_size);

                regs[rd] = ret;
                SHRINK(nargs);
                DISPATCH();
            }

            case OP_CALL_KW: {
                rd = I_A(inst);
                imm = I_B(inst);
                off = I_C(inst);
                Object *obj = RELOC(off);
                ASSERT(obj);
                Value callable = obj_value(obj);
                Value val = POP();
                Object *names = to_obj(&val);
                ASSERT(IS_TUPLE(names));
                int nargs = imm - TUPLE_LEN(names);
                ASSERT(nargs >= 0);
                Value ret = object_call_kw(&callable, cf->stack, nargs, names);
                if (is_error(&ret)) {
                    ASSERT(_exc_occurred(ks));
                    result = ret;
                    goto error;
                }

                ASSERT(rd < cf->local_size);

                regs[rd] = ret;
                SHRINK(nargs);
                DISPATCH();
            }

            case OP_RETURN: {
                rs = I_Bx(inst);

                ASSERT(rs < cf->local_size);

                result = regs[rs];
                goto done;
            }

            case OP_RETURN_NONE: {
                result = none_value;
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
    kl_trace_here(cf);

    /* finish the loop as we have an error. */
done:
    /* pop frame */
    ks->cf = cf->back;
    --ks->depth;

    return result;
}

Value kl_eval_code(Value *self, Value *args, int nargs, Object *names)
{
    KoalaState *ks = __ks();

    Object *code = to_obj(self);

    /* build a call frame */
    CallFrame *cf = _new_frame(ks, (CodeObject *)code);

    /* copy arguments */
    _copy_arguments(cf, args, nargs);

    /* eval the call frame */
    Value result = _eval_frame(ks, cf);

    /* pop frame to free list */
    _pop_frame(ks, cf);

    return result;
}

#ifdef __cplusplus
}
#endif
