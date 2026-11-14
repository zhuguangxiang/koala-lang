/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_JIT_H_
#define _KOALA_JIT_H_

#include "object.h"
#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Baseline (Sparkplug-style) x86-64 JIT, v1.
 *
 * A compiled function mirrors the interpreter's semantics directly on the
 * TValue register file (cf->locals), but with dispatch, instruction decode and
 * type checks removed and branches lowered to native jumps. See DESIGN.md.
 *
 * Native signature: void fn(TValue *regs, TValue *out, KoalaState *ks)
 *   regs - the caller's register file (cf->locals), nlocals TValues
 *   out  - where the returned TValue is written (never NULL)
 *   ks   - the running KoalaState (its stack_top is moved by direct self calls)
 */

typedef struct _JitEntry {
    void (*fn)(TValue *regs, TValue *out, KoalaState *ks);
    void *mem;       /* code start in the code arena (== fn) */
    size_t mem_size; /* code size in bytes */
} JitEntry;

/* Attempt to compile a CodeObject. Only a module's own local free functions are
 * eligible (see CodeObject.owner and num_funcs). Returns NULL if not eligible
 * (method/imported/stdlib code, unsupported opcode, etc.). The result is cached
 * by the caller on CodeObject.jit. */
JitEntry *kl_jit_compile(CodeObject *code);

/* Release a compiled entry. The code itself stays in the arena (never
 * reclaimed, DESIGN §16). Safe with NULL. */
void kl_jit_free(JitEntry *e);

/* Runtime call helper invoked by JIT-compiled code for an ordinary OP_CALL
 * (flag=0). Mirrors the interpreter: C functions go through kl_call_cfunc, code
 * objects through kl_call_code (which itself dispatches native vs interpreter).
 * Sets the calling JIT frame's cf->pc to the call site (the interpreter's
 * SAVE_PC) and, when the callee raised, records that frame via trace_here --
 * the native error epilogue cannot do either itself.
 *   args  - outgoing arg area (caller_regs + nlocals, == ks->stack_top here)
 *   out   - where to write the result TValue, or NULL for a void call (rd=0xFFF)
 *   fn    - resolved call target (ENTRY(payload))
 *   nargs - argument count
 *   pc    - absolute bytecode pc of the OP_CALL instruction
 * Returns the callee's TValue (tag in rax, payload in rdx) so the JIT code can
 * check TAG_ERROR even for a void call. */
TValue kl_jit_call_helper(TValue *args, TValue *out, Object *fn, int nargs, uint32_t pc);

/* OP_CALL flag 1 / 2: resolve the callee at run time like the interpreter, then
 * kl_jit_call_helper. callee == NULL: flag 1, the trait method `index` of the
 * receiver args[0]'s itable; else flag 2, the function value in that register
 * ("function value is not callable" otherwise). */
TValue kl_jit_call_dyn(TValue *args, TValue *out, TValue *callee, int index, int nargs,
                       uint32_t pc);

/* OP_TAIL_CALL to another function (flag 8|x): fn for x=0, the trait method
 * `index` of args[0] for x=1 (callee == NULL), the function value *callee for
 * x=2. site = (pc << 8) | nargs. A native callee is called and its result
 * returned (written to *out). A code callee reuses the current frame like the
 * interpreter (args moved to the locals, cf->code/module/nlocals replaced):
 * *out = {JIT_TAG_TAILCALL, nargs} and the native code returns, so the entry
 * (kl_call_code / the interpreter's OP_TAIL_CALL) continues the frame with the
 * new code without growing the C stack. Errors are traced; TAG_ERROR is returned. */
#define JIT_TAG_TAILCALL 4
TValue kl_jit_tail_call(TValue *args, TValue *out, Object *fn, TValue *callee, int index,
                        uint64_t site);

/* A JIT -> JIT direct call's callee returned JIT_TAG_TAILCALL in *out: run the
 * tail-called code in the linked frame ks->cf to the end (natively, or in the
 * interpreter when it is not compiled), leaving the frame linked. */
void kl_jit_tail_continue(TValue *out);
KoalaState *kl_jit_refill_cf(KoalaState *ks);

/* A direct self call returned an error: record the calling level's frame at the
 * call site (pc), as the interpreter's error path does for each frame. */
void kl_jit_self_error(uint32_t pc);

/* Raise an error at pc and trace the frame: JIT_RAISE_DIV_ZERO ("division by
 * zero"), JIT_RAISE_STACK_OVERFLOW ("stack overflow", a direct self call that
 * would run past the VM or native stack) or JIT_RAISE_NIL_UNWRAP (`x!` of nil).
 * The native code then takes the error epilogue. */
#define JIT_RAISE_DIV_ZERO       0
#define JIT_RAISE_STACK_OVERFLOW 1
#define JIT_RAISE_NIL_UNWRAP     2
void kl_jit_raise_at(int what, uint32_t pc);

/* OP_MAKE_INTF: *dst = *src with the itable idx of src's type (no error). */
void kl_jit_make_intf(TValue *dst, TValue *src, int idx);

/* OP_IS (is_as=0): bool (typeof(src) == tp); OP_AS (is_as=1): src or nil. */
TValue kl_jit_is_as(TValue *src, TypeObject *tp, int is_as);

/* Compile-time access to the small tagged-constant table (OP_LOAD_TAG/RET_TAG):
 * the JIT embeds the returned TValue's bits as immediates. */
TValue kl_jit_tag_value(int tag);

/* Shift-count check for the register-form shifts (OP_INT/UINT_SHL/SHR). Mirrors
 * the interpreter's kl_check_shift_count + SAVE_PC + trace: on an invalid count
 * it records cf->pc and the traceback entry, then returns 0 so the native code
 * branches to the error epilogue. Returns 1 when the count is valid. */
int kl_jit_shift_check(TValue *count, uint32_t pc);

/* Generic numeric protocol ops (OP_NUM_*). Arithmetic dispatches via the slot
 * table; comparisons mirror the interpreter's primitive fast paths first. Both
 * take the call-site pc for traceback and return the result TValue (tag in rax,
 * payload in rdx), setting TAG_ERROR on failure. */
TValue kl_jit_num_arith(TValue *a, TValue *b, int slot, uint32_t pc);
TValue kl_jit_num_cmp(TValue *a, TValue *b, int slot, uint32_t pc);

/* A group: object / container / string / IO. Slot-dispatch helpers mirror the
 * interpreter's kl_call_* family; all set cf->pc and trace on error, returning
 * TAG_ERROR on failure. kl_jit_new/build_intern construct objects. */
TValue kl_jit_slot3(TValue *a0, TValue *a1, TValue *a2, int nargs, int slot, uint32_t pc);
TValue kl_jit_get_item_imm(TValue *obj, int64_t idx, uint32_t pc);
TValue kl_jit_set_item_imm(TValue *obj, int64_t idx, TValue *val, uint32_t pc);
TValue kl_jit_hash(TValue *obj, uint32_t pc);
TValue kl_jit_fmt(TValue *obj, TValue *fmt, uint32_t pc);
TValue kl_jit_print(int nargs, uint32_t pc);
TValue kl_jit_new(TypeObject *tp);
TValue kl_jit_build_intern(int tag, int count);

/* Runtime switches (env: KOALA_JIT=0 disables, KOALA_JIT_THRESHOLD=<n>). */
int kl_jit_enabled(void);
uint32_t kl_jit_threshold(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_JIT_H_ */
