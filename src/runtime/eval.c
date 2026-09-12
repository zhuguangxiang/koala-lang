/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <math.h>
#include <pthread.h>
#include "except.h"
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
    vm_tag_mappings[2] = none_value;
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
#define ENTRY(i) (entry_table + (i))
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
    TValue *const_pool = VECTOR_ITEMS(&m->const_pool, TValue);
    ImportEntry *import_table = VECTOR_ITEMS(&m->import_table, ImportEntry);
    FuncEntry *entry_table = VECTOR_ITEMS(&m->func_entries, FuncEntry);
    TypeObject **types = VECTOR_ITEMS(&m->types, TypeObject *);

#ifndef NDEBUG
    int entry_size = vector_size(&m->func_entries);
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

TValue kl_eval_code(TValue *self, TValue *args, int nargs)
{
    KoalaState *ks = __ks();

    Object *code = to_obj(self);

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

void kl_run_main(Object *_m)
{
    ModuleObject *m = (ModuleObject *)_m;

    if (m->main) {
        TValue val = obj_value(m->main);
        val = kl_do_call(&val, NULL, 0);
        if (is_error(&val)) {
            Object *exc = pop_exc();
            ASSERT(exc);
            print_exc_and_free(exc);
        }
    }
}

void kl_run_init(Object *_m)
{
    ModuleObject *m = (ModuleObject *)_m;

    if (m->__init__) {
        TValue val = obj_value(m->__init__);
        val = kl_do_call(&val, NULL, 0);
        if (is_error(&val)) {
            Object *exc = pop_exc();
            ASSERT(exc);
            print_exc_and_free(exc);
        }
    }
}

static void print_test_progress(int err, int index, int total)
{
    static char results[4096]; // store all test results
    results[index] = err ? 'F' : '.';

    const int window = 50; // number of symbols to show
    int start = 0;

    // compute sliding window start
    if (total > window) {
        if (index + 1 <= window)
            start = 0;
        else
            start = (index + 1) - window;
    }

    printf("\r[");
    for (int i = start; i < start + window && i < total; i++) {
        char c = results[i];
        if (i > index) {
            printf(" "); // not executed yet
        } else if (c == 'F') {
            printf("\x1b[31mF\x1b[0m"); // red F
        } else {
            printf("\x1b[32m.\x1b[0m"); // green dot
        }
    }
    printf("]  %d/%d", index + 1, total);

    if (index + 1 == total) printf("\n");

    fflush(stdout);
}

int kl_run_tests(Object *_m)
{
    ModuleObject *m = (ModuleObject *)_m;

    Vector *tests = &m->tests;
    int total = vector_size(tests);

    printf("\nRunning %d tests in file '%s.kl'\n\n", total, m->path);

    // 1. run test cases, print progress and save result

    TestCase *_case;
    vector_foreach_ptr(_case, tests) {
        TValue val = obj_value(_case->co);

        long long start_ns = now_ns();
        val = kl_do_call(&val, NULL, 0);
        long long end_ns = now_ns();

        _case->elapsed_ns = end_ns - start_ns;

        int err = is_error(&val);

        // if there is no error, mark the test case as passed
        if (!err) {
            if (_case->expect_panic) {
                // expected a panic but the test passed without error
                _case->exc = NULL;
                _case->passed = 0;
                print_test_progress(1, i__, total);
            } else {
                _case->exc = NULL;
                _case->passed = 1;
                print_test_progress(0, i__, total);
            }
            continue;
        }

        Object *exc = pop_exc();
        ASSERT(exc);

        if (_case->expect_panic) {
            char *exc_str = kl_exc_get_msg(exc);
            if (str_equal(exc_str, _case->msg)) {
                // eat this exception as it matches the expected panic
                kl_free_exc(exc);
                _case->exc = NULL;
                _case->passed = 1;
                print_test_progress(0, i__, total);
                continue;
            }
            // fall through to unexpected exception handling
        }

        // unexpected exception
        print_test_progress(1, i__, total);
        _case->exc = exc;
        _case->passed = 0;
    }

    // 2. calculate total time and pass count

    long long total_ns = 0;
    int pass = 0;
    vector_foreach_ptr(_case, tests) {
        total_ns += _case->elapsed_ns;

        if (_case->passed) {
            ASSERT(_case->exc == NULL);
            pass++;
            continue;
        }

        if (_case->exc) {
            print_exc_and_free(_case->exc);
            _case->exc = NULL;
        } else {
            // no exception object, but the test failed for some other reason
            if (isatty(1)) {
                printf(
                    "\n\x1b[31mError:\x1b[0m Test '%s' failed.\n  Expected Exception:\n    %s\n  "
                    "But no exception was raised.\n",
                    _case->name, _case->msg);
                printf(
                    "\nHint: It seems this feature was recently implemented, but the negative\n"
                    "  test case was not updated.\n");
            } else {
                printf(
                    "\nError: Test '%s' failed.\n  Expected Exception:\n    %s\n  But no exception "
                    "was raised.\n",
                    _case->name, _case->msg);
                printf(
                    "\nHint: It seems this feature was recently implemented, but the negative\n"
                    "  test case was not updated.\n");
            }
        }
    }

    // 3. print result
    double total_ms = (double)total_ns / 1e6;
    int failed = total - pass;
    printf("\nResult: %d passed, %d failed in %.3f ms\n", pass, failed, total_ms);
    if (failed == 0) printf("\nAll tests passed.\n");

    return failed;
}

#ifdef __cplusplus
}
#endif
