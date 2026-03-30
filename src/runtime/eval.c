/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "module.h"
#include "object.h"
#include "opcode.h"
#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline TValue _do_call(TValue *callable, TValue *args, int nargs)
{
    TypeObject *tp = kl_typeof(callable);
    CallFunc call = tp->call;
    if (!call) {
        /* raise an error */
        raise_exc_fmt("'%s' is not callable", tp->name);
        return error_value;
    }

    return call(callable, args, nargs);
}

/* max call depth, stop for this limit */
#define MAX_CALL_DEPTH 10000

/* clang-format off */

// [Op:8] [A:8] [B:8] [C:8]
// [Op:8] [A:12] [B:12]
// [Op:8] [A:8] [B:16]

#define I_OP(i)  ((i) >> 24)

#define I_VAL(i, shr, mask)   (((i) >> (shr)) & ((1 << (mask)) - 1))

#define CP(i)    vector_get_ptr(const_pool, i)
#define RELOC(i) \
    ({ RelocEntry *rel = vector_get_ptr(rels, i); ASSERT(rel); rel->obj; })

#define PUSH(x)     ({ *top++ = (x); ASSERT(top <= ks->stack_base + ks->stack_size); })
#define POP()       ({ --top; ASSERT(top >= ks->stack_top); *top; })
#define SHRINK(n)   ({ top -= (n); ASSERT(top >= ks->stack_top); })

#define DISPATCH() goto dispatch;

/* clang-format on */

static TValue _eval_frame(KoalaState *ks, CallFrame *cf)
{
    CodeObject *code = cf->code;
    ModuleObject *m = (ModuleObject *)cf->module;
    Vector *const_pool = &m->const_pool;
    uint32_t *codes = m->codes;

    TValue *top = ks->stack_top;
    TValue *regs = cf->locals;

    uint32_t *pc = codes + code->cs.start_pc;

    TValue result = none_value;
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
                rd = I_VAL(inst, 12, 12);
                rs = I_VAL(inst, 0, 12);

                ASSERT(rd < cf->nlocals);
                ASSERT(rs < cf->nlocals);

                regs[rd] = regs[rs];
                DISPATCH();
            }

            case OP_LOAD_INT_IMM: {
                rd = I_VAL(inst, 16, 8);
                imm = I_VAL(inst, 0, 16);

                ASSERT(rd < cf->nlocals);

                regs[rd].tag = TAG_INT64;
                regs[rd].ival = imm;
                DISPATCH();
            }

            case OP_LOADK: {
                rd = I_VAL(inst, 16, 8);
                idx = I_VAL(inst, 0, 16);

                ASSERT(rd < cf->nlocals);

                TValue *val = CP(idx);
                regs[rd].tag = val->tag;
                regs[rd].ival = val->ival;
                DISPATCH();
            }

            case OP_PUSH: {
                rs = I_VAL(inst, 0, 12);

                ASSERT(rs < cf->nlocals);

                PUSH(regs[rs]);
                DISPATCH();
            }

            case OP_PUSH_INT_IMM: {
                imm = I_VAL(inst, 0, 16);
                PUSH(int64_value(imm));
                DISPATCH();
            }

            case OP_CALL: {
                int flg = I_VAL(inst, 20, 4);
                rd = I_VAL(inst, 8, 12);
                imm = I_VAL(inst, 0, 8);
                if (flg == 1) {
                    uint32_t index = *pc++;
                    ImportEntry *e = vector_get_ptr(&m->import_table, index);
                    ASSERT(e->kind == IMPORT_KIND_FUNC);
                    Object *target = e->address;
                    Object *m = kl_find_module(e->path);
                    target = kl_mo_find(m, e->name);
                    e->address = target;
                    ASSERT(target);
                    TValue val = obj_value(target);
                    _do_call(&val, top - imm, imm);
                    SHRINK(imm);
                } else {
                    NYI();
                }
                DISPATCH();
            }

            case OP_RET: {
                rs = I_VAL(inst, 0, 12);
                ASSERT(rs < cf->nlocals);
                result = regs[rs];
                goto done;
            }

            case OP_RET_VOID: {
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

    Object *owner = code->owner;
    if (IS_MODULE(owner)) {
        cf->module = owner;
    } else {
        ASSERT(IS_TYPE(owner, &type_type));
        cf->module = ((TypeObject *)owner)->module;
    }

    cf->nlocals = code->cs.nlocals;

    cf->locals = ks->stack_top;
    ks->stack_top += cf->nlocals;
    ASSERT(ks->stack_top <= ks->stack_base + ks->stack_size);
    return cf;
}

static void _pop_frame(KoalaState *ks, CallFrame *cf)
{
    /* shrink stack */
    ks->stack_top -= cf->nlocals;
    ASSERT(ks->stack_top >= ks->stack_base);

    cf->back = ks->cf_cache;
    ks->cf_cache = cf;
    // mm_free(cf);
}

TValue kl_eval_code(TValue *self, TValue *args, int nargs)
{
    KoalaState *ks = __ks();

    Object *code = to_obj(self);

    /* build a call frame */
    CallFrame *cf = _new_frame(ks, (CodeObject *)code);

    /* eval the call frame */
    TValue result = _eval_frame(ks, cf);

    /* pop frame to free list */
    _pop_frame(ks, cf);

    return result;
}

#ifdef __cplusplus
}
#endif
