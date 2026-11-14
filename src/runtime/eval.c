/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <math.h>
#include <pthread.h>
#include "bytesobj.h"
#include "dictobj.h"
#include "excobj.h"
#include "jit/jit.h"
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

static TValue vm_tag_mappings[11] = {
    bool_value(0),
    bool_value(1),
    nil_value,
    float64_value(0.0),
    float64_value(-0.0),
    float64_value(NAN),
    float64_value(INFINITY),
    float64_value(-INFINITY),
};

#define TAG_VALUE(tag) (vm_tag_mappings[(tag)])

void do_int_cast(TValue *regs, int rd, int rs, int mode, int dst_ti);
void do_float_cast(TValue *regs, int rd, int rs, int mode, int dst_ti);
void do_int_to_float(TValue *regs, int rd, int rs, int mode, int dst_ti);
void do_float_to_int(TValue *regs, int rd, int rs, int mode, int dst_ti);

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

        case INTERN_MAP: {
            /* values hold pairwise (k0, v0, k1, v1, ...) */
            ASSERT((count % 2) == 0);
            Object *dobj = kl_new_dict();
            for (int i = 0; i < count; i += 2) {
                kl_dict_add(dobj, &values[i], &values[i + 1]);
            }
            return dobj;
        }

        case INTERN_SLICE: {
            ASSERT(count == 3);
            return kl_new_slice(values);
        }

        case INTERN_BYTES: {
            ASSERT(count == 1);
            BytesObject *b = (BytesObject *)values[0].obj;
            ASSERT(IS_BYTES(b));
            return kl_bytes_from_data(b->data + b->offset, b->size);
        }

        default: {
            UNREACHABLE();
            return NULL;
        }
    }
}
static TValue kl_call_cfunc(Object *code, TValue *args, int nargs);

/* JIT entry guard; defined with the other JIT runtime helpers at the end of file. */
static inline int jit_params_ok(TValue *regs, int nargs);

/* baseline JIT: count a call and compile the code once when hot */
static inline void jit_count_call(CodeObject *co)
{
    if (!co->jit && !co->jit_tried) {
        if (++co->jit_hotness >= kl_jit_threshold()) {
            co->jit = kl_jit_compile(co);
            co->jit_tried = 1;
        }
    }
}

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

    cf->tailcalls = 0;
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

    CodeObject *co = (CodeObject *)code;

    /* build a call frame */
    CallFrame *cf = _new_frame(ks, co);
    cf->ks = ks;

    /* copy arguments */
    memcpy(cf->locals, args, sizeof(TValue) * nargs);

    /* baseline JIT: compile once when hot, then dispatch to native code */
    jit_count_call(co);

    TValue result;
    if (co->jit && jit_params_ok(cf->locals, nargs)) {
        /* link the frame exactly like _eval_frame does: the raise path
           (_raise_exc_fmt) and kl_call_slot read ks->cf while native code runs */
        cf->back = ks->cf;
        ks->cf = cf;
        co->jit->fn(cf->locals, &result, ks);
        /* direct self calls in native code leave stack_top behind */
        ks->stack_top = cf->locals + cf->nlocals;
        if (result.tag == JIT_TAG_TAILCALL) kl_jit_tail_continue(&result);
        ks->cf = cf->back;
    } else {
        /* eval the call frame */
        result = _eval_frame(ks, cf);
    }

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
    ASSERT(!cfunc->not_impl);
    if (cfunc->is_static) {
        /* static native method: caller passes no 'self', pass the owner type as self */
        TValue tp_val = obj_value(owner);
        return cfunc->func(&tp_val, args, nargs);
    }
    ASSERT(nargs >= 1);
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

    // TODO: when load module, need to call slot func
    if (ks->cf) _incr_stack_top(ks);

    if (IS_CFUNC(fn)) {
        ret = kl_call_cfunc(fn, args, nargs);
    } else {
        ASSERT(ks->cf && IS_CODE(fn));
        ret = kl_call_code(fn, args, nargs);
    }

    if (ks->cf) _decr_stack_top(ks);

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

/* ===========================================================================
 * JIT runtime helpers (v1). Grouped here at the end of the file. The only JIT
 * touch-points above are: the jit/jit.h include, the jit_params_scalar forward
 * declaration, and the compile-once / native-dispatch block inside kl_call_code.
 * =========================================================================== */

/* Baseline JIT entry guard: run native code when every argument is a value the
 * compiled opcodes can handle — a scalar (int/uint/bool/float) or a reference
 * (object / interface). The bytecode is statically typed, so this only rejects
 * stray error/no-value tags; it is a safety net, not a type check. */
static inline int jit_params_ok(TValue *regs, int nargs)
{
    for (int i = 0; i < nargs; i++) {
        TValue *v = regs + i;
        if (!is_int(v) && !is_uint(v) && !is_bool(v) && !is_float(v) &&
            !is_ref(v) && !is_intf(v)) return 0;
    }
    return 1;
}

/* JIT ordinary-call trampoline (see jit.h). A native function is only entered via
 * kl_call_code, which leaves ks->stack_top at caller_regs + nlocals -- exactly the
 * outgoing arg area -- and the native body never moves it, so the callee frame
 * lands here just as the interpreter's OP_CALL does. */
TValue kl_jit_call_helper(TValue *args, TValue *out, Object *fn, int nargs,
                          uint32_t pc)
{
    KoalaState *ks = __ks();
    ASSERT(ks->stack_top == args);
    /* the calling frame (from kl_call_code or a JIT direct call) describes its
       registers exactly: the GC walks [cf->locals, cf->locals + nlocals) */
    ASSERT(args == ks->cf->locals + ks->cf->nlocals);
    /* the calling JIT frame is linked by kl_call_code; record the call site
       for traceback, mirroring the interpreter's SAVE_PC */
    ks->cf->pc = pc;
    TValue ret;
    if (IS_CFUNC(fn)) {
        ret = kl_call_cfunc(fn, args, nargs);
    } else {
        ret = kl_call_code(fn, NULL, 0);
    }
    if (out) *out = ret;
    /* the native error epilogue cannot trace itself; record the JIT frame
       here, mirroring _eval_frame's error path */
    if (is_error(&ret)) trace_here(ks->cf);
    return ret;
}

TValue kl_jit_call_dyn(TValue *args, TValue *out, TValue *callee, int index, int nargs,
                       uint32_t pc)
{
    Object *fn;
    if (!callee) {
        ASSERT(is_intf(args));
        IntfTable *itab = args->itab;
        ASSERT(index < itab->num_funcs);
        fn = itab->methods[index];
    } else {
        if (!is_ref(callee) || !callee->obj || (!IS_CODE(callee->obj) && !IS_CFUNC(callee->obj))) {
            KoalaState *ks = __ks();
            ks->cf->pc = pc;
            raise_exc_str("function value is not callable");
            trace_here(ks->cf);
            if (out) *out = error_value;
            return error_value;
        }
        fn = callee->obj;
    }
    return kl_jit_call_helper(args, out, fn, nargs, pc);
}

TValue kl_jit_tail_call(TValue *args, TValue *out, Object *fn, TValue *callee, int index,
                        uint64_t site)
{
    KoalaState *ks = __ks();
    CallFrame *cf = ks->cf;
    int nargs = (int)(site & 0xFF);
    ASSERT(ks->stack_top == args);
    cf->pc = (uint32_t)(site >> 8);

    if (callee) {
        if (!is_ref(callee) || !callee->obj || (!IS_CODE(callee->obj) && !IS_CFUNC(callee->obj))) {
            raise_exc_str("function value is not callable");
            goto error;
        }
        fn = callee->obj;
    } else if (!fn) {
        ASSERT(is_intf(args));
        IntfTable *itab = args->itab;
        ASSERT(index < itab->num_funcs);
        fn = itab->methods[index];
    }

    if (IS_CFUNC(fn)) {
        *out = kl_call_cfunc(fn, args, nargs);
        if (is_error(out)) goto error;
        return *out;
    }

    /* reuse this frame, as the interpreter's OP_TAIL_CALL does */
    ASSERT(IS_CODE(fn));
    CodeObject *co = (CodeObject *)fn;
    memmove(cf->locals, args, sizeof(TValue) * nargs);
    cf->code = co;
    Object *owner = co->owner;
    cf->module = IS_MODULE(owner) ? owner : ((TypeObject *)owner)->module;
    cf->nlocals = co->cs.nlocals;
    cf->tailcalls++;
    ks->stack_top = cf->locals + cf->nlocals;
    ASSERT(ks->stack_top <= ks->stack_base + ks->stack_size);
    jit_count_call(co);
    *out = (TValue){ .tag = JIT_TAG_TAILCALL, .ival = nargs };
    return *out;

error:
    trace_here(cf);
    *out = error_value;
    return error_value;
}

/* JIT direct calls pop frames from ks->cf_cache; push a fresh one when empty.
 * Returns ks. */
KoalaState *kl_jit_refill_cf(KoalaState *ks)
{
    CallFrame *cf = mm_alloc_obj(cf);
    cf->back = ks->cf_cache;
    ks->cf_cache = cf;
    return ks;
}

void kl_jit_tail_continue(TValue *out)
{
    KoalaState *ks = __ks();
    CallFrame *cf = ks->cf;
    while (out->tag == JIT_TAG_TAILCALL) {
        /* a tail call replaced the frame's code: continue it natively when
           possible, else in the interpreter (which links the frame itself) */
        CodeObject *co = cf->code;
        if (!co->jit || !jit_params_ok(cf->locals, (int)out->ival)) {
            ks->cf = cf->back;
            *out = _eval_frame(ks, cf);
            ks->cf = cf;
            return;
        }
        co->jit->fn(cf->locals, out, ks);
        ks->stack_top = cf->locals + cf->nlocals;
    }
}

void kl_jit_self_error(uint32_t pc)
{
    KoalaState *ks = __ks();
    ks->cf->pc = pc;
    trace_here(ks->cf);
}

void kl_jit_raise_at(int what, uint32_t pc)
{
    KoalaState *ks = __ks();
    ks->cf->pc = pc;
    if (what == JIT_RAISE_DIV_ZERO)
        raise_exc_str("division by zero");
    else if (what == JIT_RAISE_NIL_UNWRAP)
        raise_exc_str("forced unwrap (`!`) of a nil value");
    else
        raise_exc_str("stack overflow");
    trace_here(ks->cf);
}

void kl_jit_make_intf(TValue *dst, TValue *src, int idx)
{
    TypeObject *tp = kl_typeof(src);
    TValue v = *src;
    v.itab = vector_get_ptr(&tp->itables, idx);
    *dst = v;
}

TValue kl_jit_is_as(TValue *src, TypeObject *tp, int is_as)
{
    TypeObject *src_tp = kl_typeof(src);
    ASSERT(src_tp && tp);
    if (!is_as) return bool_value(src_tp == tp);
    if (src_tp == tp) return *src;
    return nil_value;
}

TValue kl_jit_tag_value(int tag)
{
    ASSERT(tag >= 0 && tag < (int)(sizeof(vm_tag_mappings) / sizeof(vm_tag_mappings[0])));
    return vm_tag_mappings[tag];
}

int kl_jit_shift_check(TValue *count, uint32_t pc)
{
    KoalaState *ks = __ks();
    if (kl_check_shift_count(count)) return 1;
    /* invalid count: kl_check_shift_count raised; mirror SAVE_PC + trace */
    ks->cf->pc = pc;
    trace_here(ks->cf);
    return 0;
}

/* Generic numeric protocol arithmetic (OP_NUM_ADD/SUB/...): dispatch through the
 * slot table exactly like the interpreter's kl_call_add/etc. Records cf->pc and
 * traces the JIT frame on error, like kl_jit_call_helper. */
TValue kl_jit_num_arith(TValue *a, TValue *b, int slot, uint32_t pc)
{
    KoalaState *ks = __ks();
    ks->cf->pc = pc;
    TValue args[] = { *a, *b };
    TValue ret = kl_call_slot(args, 2, slot);
    if (is_error(&ret)) trace_here(ks->cf);
    return ret;
}

/* Generic numeric comparison (OP_NUM_EQ/NE/LT/...): mirror the interpreter's
 * primitive fast paths (int/float/uint, plus bool for eq/ne) before falling back
 * to the slot table. Builtins have no cmp slot, so the fast path is required for
 * correctness, not just speed. */
#define JIT_NUM_CMP_CASES(X, Y)                      \
    switch (slot) {                                  \
        case SLOT_EQ: return bool_value((X) == (Y)); \
        case SLOT_NE: return bool_value((X) != (Y)); \
        case SLOT_LT: return bool_value((X) < (Y));  \
        case SLOT_LE: return bool_value((X) <= (Y)); \
        case SLOT_GT: return bool_value((X) > (Y));  \
        case SLOT_GE: return bool_value((X) >= (Y)); \
    }

TValue kl_jit_num_cmp(TValue *a, TValue *b, int slot, uint32_t pc)
{
    KoalaState *ks = __ks();
    ks->cf->pc = pc;
    if (is_int(a) && is_int(b)) {
        int64_t x = a->ival, y = b->ival;
        JIT_NUM_CMP_CASES(x, y);
    } else if (is_float(a) && is_float(b)) {
        double x = a->fval, y = b->fval;
        JIT_NUM_CMP_CASES(x, y);
    } else if (is_uint(a) && is_uint(b)) {
        uint64_t x = (uint64_t)a->ival, y = (uint64_t)b->ival;
        JIT_NUM_CMP_CASES(x, y);
    } else if (is_bool(a) && is_bool(b) && (slot == SLOT_EQ || slot == SLOT_NE)) {
        int64_t x = a->ival, y = b->ival;
        return bool_value(slot == SLOT_EQ ? x == y : x != y);
    }
    TValue args[] = { *a, *b };
    TValue ret = kl_call_slot(args, 2, slot);
    if (is_error(&ret)) trace_here(ks->cf);
    return ret;
}
#undef JIT_NUM_CMP_CASES

/* ---- A group: object / container / string / IO helpers ---- */

/* Generic slot call with up to 3 args copied from the register file. Mirrors the
 * interpreter's kl_call_get_item/set_item/len/contains/slice/str family. */
TValue kl_jit_slot3(TValue *a0, TValue *a1, TValue *a2, int nargs, int slot,
                    uint32_t pc)
{
    KoalaState *ks = __ks();
    ks->cf->pc = pc;
    TValue args[3];
    args[0] = *a0;
    if (nargs >= 2) args[1] = *a1;
    if (nargs >= 3) args[2] = *a2;
    TValue ret = kl_call_slot(args, nargs, slot);
    if (is_error(&ret)) trace_here(ks->cf);
    return ret;
}

TValue kl_jit_get_item_imm(TValue *obj, int64_t idx, uint32_t pc)
{
    KoalaState *ks = __ks();
    ks->cf->pc = pc;
    TValue index = int64_value(idx);
    TValue ret = kl_call_get_item(obj, &index);
    if (is_error(&ret)) trace_here(ks->cf);
    return ret;
}

TValue kl_jit_set_item_imm(TValue *obj, int64_t idx, TValue *val, uint32_t pc)
{
    KoalaState *ks = __ks();
    ks->cf->pc = pc;
    TValue index = int64_value(idx);
    TValue ret = kl_call_set_item(obj, &index, val);
    if (is_error(&ret)) trace_here(ks->cf);
    return ret;
}

TValue kl_jit_hash(TValue *obj, uint32_t pc)
{
    KoalaState *ks = __ks();
    ks->cf->pc = pc;
    unsigned int h = kl_call_hash(obj);
    if (ks->exc) { trace_here(ks->cf); return error_value; }
    return int64_value(h);
}

TValue kl_jit_fmt(TValue *obj, TValue *fmt, uint32_t pc)
{
    KoalaState *ks = __ks();
    ks->cf->pc = pc;
    kl_call_fmt(obj, (Formatter *)to_obj(fmt));
    if (ks->exc) { trace_here(ks->cf); return error_value; }
    return nil_value;
}

TValue kl_jit_print(int nargs, uint32_t pc)
{
    KoalaState *ks = __ks();
    ks->cf->pc = pc;
    TValue ret = do_print(ks->stack_top, nargs);
    if (is_error(&ret)) trace_here(ks->cf);
    return ret;
}

TValue kl_jit_new(TypeObject *tp)
{
    return obj_value(kl_new_instance(tp));
}

TValue kl_jit_build_intern(int tag, int count)
{
    Object *obj = do_build_intern(__ks()->stack_top, tag, count);
    return obj_value(obj);
}

#ifdef __cplusplus
}
#endif
