/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_JIT_DISASM_H_
#define _KOALA_JIT_DISASM_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Disassemble one instruction of the x86-64 subset emitted by emit.c.
 * Intel syntax (e.g. "mov rax, [rbx+0x8]").
 *
 *   p      : first byte of the instruction
 *   avail  : bytes available at p
 *   off    : absolute offset of this instruction in the code buffer, used to
 *            resolve rel32 jump targets
 *   out    : receives the NUL-terminated mnemonic + operands
 *   outsz  : size of out
 *   jtarget: if non-NULL, set to the native target offset for jmp/jcc, or
 *            SIZE_MAX for any other instruction
 *
 * Returns the instruction length in bytes (always >= 1). Unrecognized bytes
 * yield "(bad)" so the caller can still make progress.
 */
size_t jit_disasm_one(const uint8_t *p, size_t avail, size_t off, char *out,
                      size_t outsz, size_t *jtarget);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_JIT_DISASM_H_ */