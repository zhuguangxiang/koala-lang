/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_JIT_H_
#define _KOALA_JIT_H_

#include "object.h"

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
 * Native signature: void fn(TValue *regs, TValue *out)
 *   regs - the caller's register file (cf->locals), nlocals TValues
 *   out  - where the returned TValue is written
 */
typedef struct _JitEntry {
    void (*fn)(TValue *regs, TValue *out);
    void *mem;       /* executable mapping (mmap) */
    size_t mem_size; /* mapping size in bytes */
} JitEntry;

/* Attempt to compile a CodeObject. Only a module's own local free functions are
 * eligible (see CodeObject.owner and num_funcs). Returns NULL if not eligible
 * (method/imported/stdlib code, unsupported opcode, etc.). The result is cached
 * by the caller on CodeObject.jit. */
JitEntry *kl_jit_compile(CodeObject *code);

/* Release a compiled entry (munmap). Safe with NULL. */
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
TValue kl_jit_call_helper(TValue *args, TValue *out, Object *fn, int nargs,
                          uint32_t pc);

/* Compile-time access to the small tagged-constant table (OP_LOAD_TAG/RET_TAG):
 * the JIT embeds the returned TValue's bits as immediates. */
TValue kl_jit_tag_value(int tag);

/* Shift-count check for the register-form shifts (OP_INT/UINT_SHL/SHR). Mirrors
 * the interpreter's kl_check_shift_count + SAVE_PC + trace: on an invalid count
 * it records cf->pc and the traceback entry, then returns 0 so the native code
 * branches to the error epilogue. Returns 1 when the count is valid. */
int kl_jit_shift_check(TValue *count, uint32_t pc);

/* Float comparison for JIT-compiled code (op: 0=EQ 1=NE 2=LT 3=LE 4=GT 5=GE).
 * Mirrors the interpreter's C-operator semantics (incl. NaN); returns 0/1. */
int64_t kl_jit_float_cmp(double a, double b, int op);

/* Generic numeric protocol ops (OP_NUM_*). Arithmetic dispatches via the slot
 * table; comparisons mirror the interpreter's primitive fast paths first. Both
 * take the call-site pc for traceback and return the result TValue (tag in rax,
 * payload in rdx), setting TAG_ERROR on failure. */
TValue kl_jit_num_arith(TValue *a, TValue *b, int slot, uint32_t pc);
TValue kl_jit_num_cmp(TValue *a, TValue *b, int slot, uint32_t pc);

/* A group: object / container / string / IO. Slot-dispatch helpers mirror the
 * interpreter's kl_call_* family; all set cf->pc and trace on error, returning
 * TAG_ERROR on failure. kl_jit_new/build_intern construct objects. */
TValue kl_jit_slot3(TValue *a0, TValue *a1, TValue *a2, int nargs, int slot,
                    uint32_t pc);
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