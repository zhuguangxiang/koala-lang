/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <math.h>
#include "modobj.h"
#include "opcode.h"
#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* max call depth, stop for this limit */
#define MAX_CALL_DEPTH 10000

static TValue vm_tag_mappings[11];

void init_tag_mappings(void)
{
    vm_tag_mappings[0] = bool_value(0);
    vm_tag_mappings[1] = bool_value(1);
    vm_tag_mappings[2] = none_value;
    vm_tag_mappings[3] = float64_value(0.0);
    vm_tag_mappings[4] = float64_value(-0.0);
    vm_tag_mappings[5] = float64_value(NAN);
    vm_tag_mappings[6] = float64_value(INFINITY);
    vm_tag_mappings[7] = float64_value(-INFINITY);
}

#define TAG_VALUE(tag) (vm_tag_mappings[(tag)])

/* clang-format off */

// [Op:8] [A:8] [B:8] [C:8]
// [Op:8] [A:12] [B:12]
// [Op:8] [A:8] [B:16]

#define I_OP(i)  ((i) >> 24)

#define I_VAL(i, shr, mask)   (((i) >> (shr)) & ((1 << (mask)) - 1))
#define I_SVAL(i, shr, mask)  (int##mask##_t)I_VAL(i, shr, mask)

#define CP(i) (const_pool + (i))
#define ENTRY(i) (entry_table + (i))
#define IMPORT_ENTRY(i) (import_table + (i))

#define SHRINK(n)   ({ top -= (n); ASSERT(top >= ks->stack_top); })

#define OP_NYI(op) do { \
    fprintf(stderr, "Fatal: Opcode %d (%s) is Not Yet Implemented\n", op, #op); \
    abort(); \
} while (0)

#ifndef NDEBUG
    #define CHECK_REG_ID(id) ASSERT((id) < max_regs)
#else
    #define CHECK_REG_ID(id) ((void)0)
#endif

/* clang-format on */

#undef USE_COMPUTED_GOTOS

#ifdef USE_COMPUTED_GOTOS

/* map TARGET to a local label address */
#define TARGET(op) TARGET_##op:

/* direct threaded dispatch: fetch next instruction and jump immediately */
#define DISPATCH() \
    do { \
        inst = *pc++; \
        op = I_OP(inst); \
        goto *opcode_targets[op]; \
    } while (0)

#else

/* standard case label for switch-based dispatch */
#define TARGET(op) case op:

/* jump back to the loop head for the next iteration */
#define DISPATCH() goto dispatch;

#endif

__attribute__((aligned(64))) static TValue _eval_frame(KoalaState *ks, CallFrame *cf)
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

#ifdef USE_COMPUTED_GOTOS
    /* Jump table must be local to capture &&TARGET_ labels */
#include "opcode_targets.h"
#endif

main_loop:
    for (;;) {
    dispatch:
        inst = *pc++;
        op = I_OP(inst);

#ifdef USE_COMPUTED_GOTOS
        /* initial jump into the instruction stream */
        goto *opcode_targets[op];
#else
        switch (op)
#endif
        {
#include "vm_ops.h"

#ifndef USE_COMPUTED_GOTOS
            default: {
                UNREACHABLE();
                break;
            }
#endif
        } /* end of switch or computed goto block */
    } /* end of main_loop */

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
