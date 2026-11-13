/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_JIT_PRINTER_H_
#define _KOALA_JIT_PRINTER_H_

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * IR-style dump of a successfully compiled function, mirroring the compiler's
 * klr printer (func @name {, %bbN: blocks, 8-space indented insns, ;; notes).
 * Each bytecode instruction is shown with decoded operands and annotated with
 * its native offset, byte count and emitted machine-code bytes. The trailing
 * error epilogue (entered on a TAG_ERROR call result) is dumped after the body.
 *
 *   name     - function name (CodeObject.cs.name)
 *   insns    - bytecode words, n of them (already offset by start_pc)
 *   pc_map   - pc -> native code offset
 *   code     - emitted machine code buffer
 *   code_len - total emitted bytes
 *   err_off  - native offset of the error epilogue (end of the body)
 */
void jit_print_code(const char *name, const uint32_t *insns, int n,
                    const size_t *pc_map, const uint8_t *code, size_t code_len,
                    size_t err_off, FILE *fp);

/*
 * One-line bail/skip report for a function that was NOT compiled.
 * pc >= 0 points at the offending instruction; pc < 0 means a structural
 * reason with no specific instruction (e.g. non-module owner).
 */
void jit_print_bail(const char *name, int pc, uint32_t insn, const char *reason,
                    FILE *fp);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_JIT_PRINTER_H_ */