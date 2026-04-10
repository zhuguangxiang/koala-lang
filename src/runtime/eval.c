/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "modobj.h"
#include "opcode.h"
#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* max call depth, stop for this limit */
#define MAX_CALL_DEPTH 10000

/* clang-format off */

// [Op:8] [A:8] [B:8] [C:8]
// [Op:8] [A:12] [B:12]
// [Op:8] [A:8] [B:16]

#define I_OP(i)  ((i) >> 24)

#define I_VAL(i, shr, mask)   (((i) >> (shr)) & ((1 << (mask)) - 1))

#define CP(i) (const_pool + (i))
#define ENTRY(i) (entry_table + (i))
#define IMPORT_ENTRY(i) (import_table + (i))

#define PUSH(x)     ({ *top++ = (x); has_push = 1; ASSERT(top <= ks->stack_base + ks->stack_size); })
#define SHRINK(n)   ({ top -= (n); ASSERT(top >= ks->stack_top); })

#define DISPATCH() goto dispatch;

/* clang-format on */

static TValue _eval_frame(KoalaState *ks, CallFrame *cf)
{
    /* push frame */
    cf->back = ks->cf;
    // cf->ks = ks;
    ks->cf = cf;
    // ++ks->depth;

    // ASSERT(ks->depth <= MAX_CALL_DEPTH);

ext_tailcall:
    ModuleObject *m = (ModuleObject *)cf->module;
    TValue *const_pool = VECTOR_RAW(&m->const_pool, TValue);
    ImportEntry *import_table = VECTOR_RAW(&m->import_table, ImportEntry);
    FuncEntry *entry_table = VECTOR_RAW(&m->func_entries, FuncEntry);
#ifndef NDEBUG
    int entry_size = vector_size(&m->func_entries);
#endif
    uint32_t *codes = m->codes;

local_tailcall:
    // TValue *top = ks->stack_top;
    TValue *regs = cf->locals;
    CodeObject *code = cf->code;
    uint32_t *pc = codes + code->cs.start_pc;

#ifndef NDEBUG
    int max_regs = cf->nlocals + code->cs.max_call_args;
#endif

    TValue result = none_value;
    register uint32_t inst;
    register OpCode op;
    register int rd, rs, rt, imm, idx, off;
    // int has_push = 0;

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

                ASSERT(rd < max_regs);
                ASSERT(rs < max_regs);

                regs[rd] = regs[rs];
                DISPATCH();
            }

            case OP_LOAD_INT_IMM: {
                rd = I_VAL(inst, 16, 8);
                imm = I_VAL(inst, 0, 16);

                ASSERT(rd < max_regs);

                regs[rd].tag = TAG_INT64;
                regs[rd].ival = imm;
                DISPATCH();
            }

            case OP_LOADK: {
                rd = I_VAL(inst, 16, 8);
                idx = I_VAL(inst, 0, 16);

                ASSERT(rd < max_regs);

                TValue *val = CP(idx);
                regs[rd].tag = val->tag;
                regs[rd].ival = val->ival;
                DISPATCH();
            }

            case OP_INT_ADD: {
                rd = I_VAL(inst, 16, 8);
                rs = I_VAL(inst, 8, 8);
                rt = I_VAL(inst, 0, 8);

                ASSERT(rd < max_regs);
                ASSERT(rs < max_regs);
                ASSERT(rt < max_regs);
                ASSERT(regs[rs].tag == regs[rt].tag);
                ASSERT(regs[rs].tag == TAG_INT64 || regs[rs].tag == TAG_UINT64);

                regs[rd].ival = regs[rs].ival + regs[rt].ival;
                regs[rd].tag = regs[rs].tag;
                DISPATCH();
            }

            case OP_INT_ADD_IMM: {
                rd = I_VAL(inst, 16, 8);
                rs = I_VAL(inst, 8, 8);
                imm = I_VAL(inst, 0, 8);

                ASSERT(rd < max_regs);
                ASSERT(rs < max_regs);
                ASSERT(regs[rs].tag == TAG_INT64 || regs[rs].tag == TAG_UINT64);

                regs[rd].ival = regs[rs].ival + imm;
                regs[rd].tag = regs[rs].tag;
                DISPATCH();
            }

            case OP_INT_SUB_IMM: {
                rd = I_VAL(inst, 16, 8);
                rs = I_VAL(inst, 8, 8);
                imm = I_VAL(inst, 0, 8);

                ASSERT(rd < max_regs);
                ASSERT(rs < max_regs);
                ASSERT(regs[rs].tag == TAG_INT64 || regs[rs].tag == TAG_UINT64);

                regs[rd].ival = regs[rs].ival - imm;
                regs[rd].tag = regs[rs].tag;
                DISPATCH();
            }

            case OP_INT_CMPLT_IMM: {
                rd = I_VAL(inst, 16, 8);
                rs = I_VAL(inst, 8, 8);
                imm = I_VAL(inst, 0, 8);

                ASSERT(rd < max_regs);
                ASSERT(rs < max_regs);
                ASSERT(regs[rs].tag == TAG_INT64);

                regs[rd].ival = regs[rs].ival < imm;
                regs[rd].tag = TAG_BOOL;
                DISPATCH();
            }

            case OP_JMP_FALSE: {
                rs = I_VAL(inst, 16, 8);
                off = I_VAL(inst, 0, 16);

                ASSERT(rs < max_regs);
                ASSERT(regs[rs].tag == TAG_BOOL);

                if (regs[rs].bval == 0) {
                    pc += off;
                }
                DISPATCH();
            }

            case OP_JMP_INT_NE_IMM: {
                rs = I_VAL(inst, 16, 8);
                imm = I_VAL(inst, 8, 8);
                off = I_VAL(inst, 0, 8);

                ASSERT(rs < max_regs);

                if (regs[rs].ival != imm) {
                    pc += off;
                }
                DISPATCH();
            }

            case OP_JMP_INT_GE_IMM: {
                rs = I_VAL(inst, 16, 8);
                imm = I_VAL(inst, 8, 8);
                off = I_VAL(inst, 0, 8);

                ASSERT(rs < max_regs);

                if (regs[rs].ival >= imm) {
                    pc += off;
                }
                DISPATCH();
            }

            case OP_CALL: {
                int flg = I_VAL(inst, 20, 4);
                rd = I_VAL(inst, 8, 12);
                imm = I_VAL(inst, 0, 8);

                if (flg == 1) {
                    uint32_t index = *pc++;
                    ImportEntry *e = IMPORT_ENTRY(index);
                    ASSERT(e->kind == IMPORT_KIND_FUNC);
                    Object *target = e->address;
                    ASSERT(target);
                    TValue val = obj_value(target);
                    TValue ret = kl_do_call(&val, ks->stack_top, imm);
                    if (rd != 0xFFFu) {
                        ASSERT(rd < max_regs);
                        regs[rd] = ret;
                    }
                    // if (has_push) {
                    //     SHRINK(imm);
                    //     has_push = 0;
                    // }
                    DISPATCH();
                }

                int32_t local_index = *(int32_t *)pc++;
                // uint32_t *target_pc = pc + local_index;
                // ASSERT(target_pc < codes + code->cs.code_size);
                // uint32_t f_idx = *target_pc;
                ASSERT(local_index >= 0 && local_index < entry_size);
                FuncEntry *e = ENTRY(local_index);
                Object *obj = e->obj;
                TValue ret;

                if (IS_CFUNC(obj)) {
                    CFuncObject *cfunc = (CFuncObject *)obj;
                    TValue val = obj_value(obj);
                    NativeFunc func = cfunc->func;
                    ret = func(&val, ks->stack_top, imm);
                } else {
                    ASSERT(IS_CODE(e->obj));
                    TValue val = obj_value(e->obj);
                    ret = kl_eval_code(&val, NULL, 0);
                }

                if (rd != 0xFFFu) {
                    ASSERT(rd < max_regs);
                    regs[rd] = ret;
                }
                DISPATCH();
            }

            case OP_TAIL_CALL: {
                int flg = I_VAL(inst, 20, 4);
                // imm = I_VAL(inst, 0, 8);

                // if (flg == 1) {
                //     // external function call
                //     NYI();
                //     goto ext_tailcall;
                // }

                if (flg == 3) {
                    pc = codes + code->cs.start_pc;
                    goto main_loop;
                }

                // local function call
                int32_t local_index = *(int32_t *)pc++;
                ASSERT(local_index >= 0 && local_index < entry_size);
                FuncEntry *e = ENTRY(local_index);
                Object *obj = e->obj;

                if (obj == (Object *)cf->code) {
                    pc = codes + code->cs.start_pc;
                    goto main_loop;
                }

                TValue ret;
                if (IS_CFUNC(obj)) {
                    // CFuncObject *cfunc = (CFuncObject *)obj;
                    // TValue val = obj_value(obj);
                    // NativeFunc func = cfunc->func;
                    // ret = func(&val, regs, imm);
                    // // TODO: tail call does not have return value.
                    // // if (rd != 0xFFFu) {
                    // //     ASSERT(rd < cf->nlocals);
                    // //     regs[rd] = ret;
                    // // }
                    // SHRINK(imm);
                    DISPATCH();
                } else {
                    ASSERT(IS_CODE(e->obj));
                    cf->code = (CodeObject *)e->obj;
                    cf->nlocals = cf->code->cs.nlocals;
                    ks->stack_top = cf->locals + cf->nlocals;
                    goto local_tailcall;
                }
            }

            case OP_RET: {
                rs = I_VAL(inst, 0, 12);
                ASSERT(rs < max_regs);
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
    // --ks->depth;

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

void kl_run_module(Object *_m)
{
    ModuleObject *m = (ModuleObject *)_m;

    if (m->__init__) {
        TValue val = obj_value(m->__init__);
        kl_do_call(&val, NULL, 0);
    }

    if (m->main) {
        TValue val = obj_value(m->main);
        kl_do_call(&val, NULL, 0);
    }
}

#ifdef __cplusplus
}
#endif
