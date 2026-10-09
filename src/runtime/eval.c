/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <math.h>
#include <pthread.h>
#include "excobj.h"
#include "listobj.h"
#include "modobj.h"
#include "opcode_only.h"
#include "rangeobj.h"
#include "tupleobj.h"

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
    vm_tag_mappings[2] = nil_value;
    vm_tag_mappings[3] = float64_value(0.0);
    vm_tag_mappings[4] = float64_value(-0.0);
    vm_tag_mappings[5] = float64_value(NAN);
    vm_tag_mappings[6] = float64_value(INFINITY);
    vm_tag_mappings[7] = float64_value(-INFINITY);
}

#define TAG_VALUE(tag) (vm_tag_mappings[(tag)])

#include "do_cast.h"

static Object *do_build_intern(TValue *values, InternTag tag, int count)
{
    switch (tag) {
        case INTERN_TUPLE: {
            return kl_new_tuple(values, count);
        }

        case INTERN_RANGE: {
            ASSERT(count == 3);
            return kl_new_range(values);
        }

        case INTERN_LIST: {
            return kl_list_from_array(values, count);
        }

        case INTERN_SLICE: {
            ASSERT(count == 3);
            return kl_new_slice(values);
        }

        default: {
            UNREACHABLE();
            return NULL;
        }
    }
}

static TValue do_nil_check(TValue *val)
{
    if (!is_nil(val)) return nil_value;

    raise_exc_str("forced unwrap (`!`) of a nil value");
    return error_value;
}

static TValue kl_call_cfunc(Object *code, TValue *args, int nargs);

TValue do_print(TValue *args, int nargs);

/* clang-format off */

// save pc for traceback
#define SAVE_PC() do { \
    ptrdiff_t off = pc - codes; \
    ASSERT(off >= 0); \
    cf->pc = (uint32_t)off - 1; \
} while (0)

// [Op:8] [A:8] [B:8] [C:8]
// [Op:8] [A:12] [B:12]
// [Op:8] [A:8] [B:16]

#define I_OP(i)  ((i) >> 24)

#define I_VAL(i, shr, mask)   (((i) >> (shr)) & ((1 << (mask)) - 1))
#define I_SVAL(i, shr, mask)  (int##mask##_t)I_VAL(i, shr, mask)

#define CP(i) (const_pool + (i))
#define ENTRY(i) entry_table[i]
#define IMPORT_ENTRY(i) (import_table + (i))
#define TYPE(i) (*(types + (i)))

#define OP_NYI(op) do { \
    fprintf(stderr, "Fatal: Opcode %d (%s) is Not Yet Implemented\n", op, #op); \
    abort(); \
} while (0)

#define CHECK_REG_ID(id) ASSERT((id) < max_regs)
#define CHECK_IS_INT(id) ASSERT(is_int(regs + (id)))
#define CHECK_IS_UINT(id) ASSERT(is_uint(regs + (id)))
#define CHECK_IS_FLOAT(id) ASSERT(is_float(regs + (id)))
#define CHECK_IS_BOOL(id) ASSERT(is_bool(regs + (id)))
#define CHECK_TYPE_INDEX(idx) ASSERT((idx) >= 0 && (idx) < types_size)

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

// clang-format off
__attribute__((always_inline, aligned(64)))
static inline TValue _eval_frame(KoalaState *ks, CallFrame *cf)
// clang-format on
{
    /* push frame */
    cf->back = ks->cf;
    // cf->ks = ks;
    ks->cf = cf;
    // ++ks->depth;

    // ASSERT(ks->depth <= MAX_CALL_DEPTH);

ext_tailcall:
    ModuleObject *m = (ModuleObject *)cf->module;
    TValue *const_pool = VECTOR_ITEMS(&m->const_pool, TValue);
    ImportEntry *import_table = VECTOR_ITEMS(&m->import_table, ImportEntry);
    Object **entry_table = VECTOR_ITEMS(&m->funcs, Object *);
    TypeObject **types = VECTOR_ITEMS(&m->types, TypeObject *);

#ifndef NDEBUG
    int entry_size = vector_size(&m->funcs);
    int types_size = vector_size(&m->types);
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

    TValue result = nil_value;
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
    trace_here(cf);
    result = error_value;

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

TValue kl_call_code(Object *code, TValue *args, int nargs)
{
    KoalaState *ks = __ks();

    ASSERT(IS_CODE(code));

    /* build a call frame */
    CallFrame *cf = _new_frame(ks, (CodeObject *)code);
    cf->ks = ks;

    /* copy arguments */
    memcpy(cf->locals, args, sizeof(TValue) * nargs);

    /* eval the call frame */
    TValue result = _eval_frame(ks, cf);

    /* pop frame to free list */
    _pop_frame(ks, cf);

    return result;
}

static inline void _incr_stack_top(KoalaState *ks)
{
    int max_call_args = ks->cf->code->cs.max_call_args;
    ks->stack_top += max_call_args;
}

static inline void _decr_stack_top(KoalaState *ks)
{
    int max_call_args = ks->cf->code->cs.max_call_args;
    ks->stack_top -= max_call_args;
}

static TValue kl_call_cfunc(Object *code, TValue *args, int nargs)
{
    ASSERT(IS_CFUNC(code));
    CFuncObject *cfunc = (CFuncObject *)code;
    Object *owner = cfunc->owner;
    if (IS_MODULE(owner) || cfunc->not_impl) {
        TValue fn_val = obj_value(code);
        return cfunc->func(&fn_val, args, nargs);
    }

    ASSERT(IS_TYPE(owner, &type_type));
    ASSERT(nargs >= 1);
    ASSERT(!cfunc->not_impl);
    return cfunc->func(args, args + 1, nargs - 1);
}

TValue kl_call_slot(TValue *args, int nargs, int slotid)
{
    ASSERT(slotid < SLOT_MAX);

    TValue *self = &args[0];
    TypeObject *tp = kl_typeof(self);
    ASSERT(tp);

    Object *fn = tp->slots[slotid];
    if (!fn) {
        raise_exc_fmt("%s: slot %d is not implmented", tp->name, slotid);
        return error_value;
    }

    TValue ret;

    KoalaState *ks = __ks();

    _incr_stack_top(ks);

    if (IS_CFUNC(fn)) {
        ret = kl_call_cfunc(fn, args, nargs);
    } else {
        ASSERT(IS_CODE(fn));
        ret = kl_call_code(fn, args, nargs);
    }

    _decr_stack_top(ks);

    return ret;
}

static Object *_get_intf_func(TValue *intf, int func_idx)
{
    ASSERT(is_intf(intf));
    IntfTable *itab = intf->itab;
    ASSERT(itab);
    ASSERT(func_idx >= 0 && func_idx < itab->num_funcs);
    return itab->methods[func_idx];
}

TValue kl_call_intf(TValue *args, int nargs, int intf_idx)
{
    ASSERT(nargs >= 1);
    TValue *self = &args[0];
    Object *fn = _get_intf_func(self, intf_idx);
    if (!fn) {
        raise_exc_fmt("intf %d is not implemented", intf_idx);
        return error_value;
    }

    if (IS_CFUNC(fn)) {
        return kl_call_cfunc(fn, args, nargs);
    } else {
        ASSERT(IS_CODE(fn));
        return kl_call_code(fn, args, nargs);
    }
}

void kl_run_main(Object *_m)
{
    ModuleObject *m = (ModuleObject *)_m;

    if (!m->main) return;

    TValue val = kl_call_code(m->main, NULL, 0);
    if (is_error(&val)) {
        Object *exc = pop_exc();
        ASSERT(exc);
        print_exc_and_free(exc);
    }
}

void kl_run_init(Object *_m)
{
    ModuleObject *m = (ModuleObject *)_m;

    if (!m->__init__) return;

    TValue val = kl_call_code(m->__init__, NULL, 0);
    if (is_error(&val)) {
        Object *exc = pop_exc();
        ASSERT(exc);
        print_exc_and_free(exc);
    }
}

#ifdef __cplusplus
}
#endif
