
// clang-format off

/*---------------------------------------------------------------+
 |  Koala VM Opcode List (X-Macro Source of Truth)               |
 |                                                               |
 |  Each opcode entry:                                           |
 |      X(OPCODE_NAME, FORMAT_xxx)                               |
 |                                                               |
 |  Full English comments are placed above each X() entry.       |
 +---------------------------------------------------------------*/

/**
 * OP_NOP — no operation
 *
 * FORMAT_Op:
 *     | op:8 | ------------------------:24 |
 *
 * Details:
 *     Does nothing. Used as padding or alignment.
 */
X(OP_NOP, FORMAT_Op)

/*---------------------------------------------------------------+
 |  Move and Constant Instructions                               |
 +---------------------------------------------------------------*/

/**
 * OP_MOVE — register-to-register move
 *
 * FORMAT_RxRx:
 *     | op:8 | rd:12 | rs:12 |
 *
 * Details:
 *     Copies the value from register rs into register rd.
 */
X(OP_MOVE, FORMAT_RxRx)

/**
 * OP_LOAD_INT_IMM — load immediate integer
 *
 * FORMAT_RImm2:
 *     | op:8 | rd:8 | imm:16 |
 *
 * Details:
 *     Loads a 16-bit signed integer immediate into register rd.
 */
X(OP_LOAD_INT_IMM, FORMAT_RImm2)

/**
 * OP_LOAD_TAG — load small tagged constant
 *
 * FORMAT_RxImm:
 *     | op:8 | ---:4 | rd:12 | imm:8 |
 *
 * Details:
 *     Loads a small tagged constant into rd. The imm field encodes:
 *         - boolean values
 *         - special float values (e.g., +0.0, -0.0, NaN, -inf, +inf)
 *         - none / null
 *     This avoids constant-pool lookup for common values.
 */
X(OP_LOAD_TAG, FORMAT_RxImm)

/**
 * OP_LOADK — load constant from constant pool
 *
 * FORMAT_RIdx2:
 *     | op:8 | rd:8 | idx:16 |
 *
 * Details:
 *     Loads a constant from the module's constant pool:
 *         rd = CP[idx]
 *     Used for strings, floats, arrays, objects, and other large constants.
 */
X(OP_LOADK, FORMAT_RIdx2)

/*---------------------------------------------------------------+
 |  Integer Arithmetic Operations                                |
 +---------------------------------------------------------------*/

/**
 * OP_INT_ADD — integer addition
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs + rt.
 */
X(OP_INT_ADD, FORMAT_RRR)

/**
 * OP_INT_ADD_IMM — integer addition with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs + imm, where imm is an 8-bit signed immediate.
 */
X(OP_INT_ADD_IMM, FORMAT_RRImm)

/**
 * OP_INT_SUB — integer subtraction
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs - rt.
 */
X(OP_INT_SUB, FORMAT_RRR)

/**
 * OP_INT_SUB_IMM — integer subtraction with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs - imm, where imm is an 8-bit signed immediate.
 */
X(OP_INT_SUB_IMM, FORMAT_RRImm)

/**
 * OP_INT_MUL — integer multiplication
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs * rt.
 */
X(OP_INT_MUL, FORMAT_RRR)

/**
 * OP_INT_MUL_IMM — integer multiplication with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs * imm, where imm is an 8-bit signed immediate.
 */
X(OP_INT_MUL_IMM, FORMAT_RRImm)

/**
 * OP_INT_DIV — integer division
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs / rt. Division-by-zero behavior follows the VM's
 *     integer arithmetic rules.
 */
X(OP_INT_DIV, FORMAT_RRR)

/**
 * OP_INT_DIV_IMM — integer division with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs / imm, where imm is an 8-bit signed immediate.
 *     Division-by-zero behavior follows the VM's integer arithmetic rules.
 */
X(OP_INT_DIV_IMM, FORMAT_RRImm)

/**
 * OP_INT_MOD — integer modulo
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs % rt. Modulo-by-zero behavior follows the VM's
 *     integer arithmetic rules.
 */
X(OP_INT_MOD, FORMAT_RRR)

/**
 * OP_INT_MOD_IMM — integer modulo with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs % imm, where imm is an 8-bit signed immediate.
 *     Modulo-by-zero behavior follows the VM's integer arithmetic rules.
 */
X(OP_INT_MOD_IMM, FORMAT_RRImm)

/*---------------------------------------------------------------+
 |  Integer Bitwise and Shift Operations                         |
 +---------------------------------------------------------------*/

/**
 * OP_INT_AND — integer bitwise AND
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs & rt.
 */
X(OP_INT_AND, FORMAT_RRR)

/**
 * OP_INT_AND_IMM — integer bitwise AND with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs & imm, where imm is an 8-bit signed immediate.
 */
X(OP_INT_AND_IMM, FORMAT_RRImm)

/**
 * OP_INT_OR — integer bitwise OR
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs | rt.
 */
X(OP_INT_OR, FORMAT_RRR)

/**
 * OP_INT_OR_IMM — integer bitwise OR with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs | imm, where imm is an 8-bit signed immediate.
 */
X(OP_INT_OR_IMM, FORMAT_RRImm)

/**
 * OP_INT_XOR — integer bitwise XOR
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs ^ rt.
 */
X(OP_INT_XOR, FORMAT_RRR)

/**
 * OP_INT_XOR_IMM — integer bitwise XOR with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs ^ imm, where imm is an 8-bit signed immediate.
 */
X(OP_INT_XOR_IMM, FORMAT_RRImm)

/**
 * OP_INT_SHL — integer left shift
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs << rt. Only the low 5 bits of rt are used.
 */
X(OP_INT_SHL, FORMAT_RRR)

/**
 * OP_INT_SHL_IMM — integer left shift with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs << imm, where imm is an 8-bit unsigned shift amount.
 */
X(OP_INT_SHL_IMM, FORMAT_RRImm)

/**
 * OP_INT_SHR — integer arithmetic right shift
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs >> rt (arithmetic shift). Only the low 5 bits of rt are used.
 */
X(OP_INT_SHR, FORMAT_RRR)

/**
 * OP_INT_SHR_IMM — integer arithmetic right shift with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs >> imm (arithmetic shift), where imm is an 8-bit unsigned shift amount.
 */
X(OP_INT_SHR_IMM, FORMAT_RRImm)

/*---------------------------------------------------------------+
 |  Integer Comparison Operations                                |
 +---------------------------------------------------------------*/

/**
 * OP_INT_CMPEQ — integer equality comparison
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = (rs == rt).
 */
X(OP_INT_CMPEQ, FORMAT_RRR)

/**
 * OP_INT_CMPEQ_IMM — integer equality comparison with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = (rs == imm), where imm is an 8-bit signed immediate.
 */
X(OP_INT_CMPEQ_IMM, FORMAT_RRImm)

/**
 * OP_INT_CMPNE — integer inequality comparison
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = (rs != rt).
 */
X(OP_INT_CMPNE, FORMAT_RRR)

/**
 * OP_INT_CMPNE_IMM — integer inequality comparison with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = (rs != imm), where imm is an 8-bit signed immediate.
 */
X(OP_INT_CMPNE_IMM, FORMAT_RRImm)

/**
 * OP_INT_CMPLT — integer less-than comparison
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = (rs < rt), using signed integer comparison.
 */
X(OP_INT_CMPLT, FORMAT_RRR)

/**
 * OP_INT_CMPLT_IMM — integer less-than comparison with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = (rs < imm), where imm is an 8-bit signed immediate.
 */
X(OP_INT_CMPLT_IMM, FORMAT_RRImm)

/**
 * OP_INT_CMPLE — integer less-or-equal comparison
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = (rs <= rt), using signed integer comparison.
 */
X(OP_INT_CMPLE, FORMAT_RRR)

/**
 * OP_INT_CMPLE_IMM — integer less-or-equal comparison with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = (rs <= imm), where imm is an 8-bit signed immediate.
 */
X(OP_INT_CMPLE_IMM, FORMAT_RRImm)

/**
 * OP_INT_CMPGT — integer greater-than comparison
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = (rs > rt), using signed integer comparison.
 */
X(OP_INT_CMPGT, FORMAT_RRR)

/**
 * OP_INT_CMPGT_IMM — integer greater-than comparison with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = (rs > imm), where imm is an 8-bit signed immediate.
 */
X(OP_INT_CMPGT_IMM, FORMAT_RRImm)

/**
 * OP_INT_CMPGE — integer greater-or-equal comparison
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = (rs >= rt), using signed integer comparison.
 */
X(OP_INT_CMPGE, FORMAT_RRR)

/**
 * OP_INT_CMPGE_IMM — integer greater-or-equal comparison with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = (rs >= imm), where imm is an 8-bit signed immediate.
 */
X(OP_INT_CMPGE_IMM, FORMAT_RRImm)

/*---------------------------------------------------------------+
 |  Unary Integer Operations                                     |
 +---------------------------------------------------------------*/

/**
 * OP_INT_NEG — integer negation
 *
 * FORMAT_RxRx:
 *     | op:8 | rd:12 | rs:12 |
 *
 * Details:
 *     Computes rd = -rs.
 */
X(OP_INT_NEG, FORMAT_RxRx)

/**
 * OP_INT_NOT — integer bitwise NOT
 *
 * FORMAT_RxRx:
 *     | op:8 | rd:12 | rs:12 |
 *
 * Details:
 *     Computes rd = ~rs (bitwise complement).
 */
X(OP_INT_NOT, FORMAT_RxRx)

/*---------------------------------------------------------------+
 |  Unsigned Integer Operations                                  |
 +---------------------------------------------------------------*/

/**
 * OP_UINT_DIV — unsigned integer division
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs / rt using unsigned division semantics.
 */
X(OP_UINT_DIV, FORMAT_RRR)

/**
 * OP_UINT_DIV_IMM — unsigned integer division with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs / imm using unsigned division semantics.
 */
X(OP_UINT_DIV_IMM, FORMAT_RRImm)

/**
 * OP_UINT_MOD — unsigned integer modulo
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs % rt using unsigned modulo semantics.
 */
X(OP_UINT_MOD, FORMAT_RRR)

/**
 * OP_UINT_MOD_IMM — unsigned integer modulo with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs % imm using unsigned modulo semantics.
 */
X(OP_UINT_MOD_IMM, FORMAT_RRImm)

/**
 * OP_UINT_SHR — unsigned logical right shift
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs >> rt (logical shift). High bits are filled with zero.
 */
X(OP_UINT_SHR, FORMAT_RRR)

/**
 * OP_UINT_SHR_IMM — unsigned logical right shift with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs >> imm (logical shift). High bits are filled with zero.
 */
X(OP_UINT_SHR_IMM, FORMAT_RRImm)

/*---------------------------------------------------------------+
 |  Unsigned Integer Comparison Operations                       |
 +---------------------------------------------------------------*/

/**
 * OP_UINT_CMPLT — unsigned less-than comparison
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = (rs < rt) using unsigned comparison.
 */
X(OP_UINT_CMPLT, FORMAT_RRR)

/**
 * OP_UINT_CMPLT_IMM — unsigned less-than comparison with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = (rs < imm) using unsigned comparison.
 */
X(OP_UINT_CMPLT_IMM, FORMAT_RRImm)

/**
 * OP_UINT_CMPLE — unsigned less-or-equal comparison
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = (rs <= rt) using unsigned comparison.
 */
X(OP_UINT_CMPLE, FORMAT_RRR)

/**
 * OP_UINT_CMPLE_IMM — unsigned less-or-equal comparison with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = (rs <= imm) using unsigned comparison.
 */
X(OP_UINT_CMPLE_IMM, FORMAT_RRImm)

/**
 * OP_UINT_CMPGT — unsigned greater-than comparison
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = (rs > rt) using unsigned comparison.
 */
X(OP_UINT_CMPGT, FORMAT_RRR)

/**
 * OP_UINT_CMPGT_IMM — unsigned greater-than comparison with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = (rs > imm) using unsigned comparison.
 */
X(OP_UINT_CMPGT_IMM, FORMAT_RRImm)

/**
 * OP_UINT_CMPGE — unsigned greater-or-equal comparison
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = (rs >= rt) using unsigned comparison.
 */
X(OP_UINT_CMPGE, FORMAT_RRR)

/**
 * OP_UINT_CMPGE_IMM — unsigned greater-or-equal comparison with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = (rs >= imm) using unsigned comparison.
 */
X(OP_UINT_CMPGE_IMM, FORMAT_RRImm)

/*---------------------------------------------------------------+
 |  Floating-Point Operations                                    |
 +---------------------------------------------------------------*/

/**
 * OP_FLOAT_ADD — floating-point addition
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs + rt.
 */
X(OP_FLOAT_ADD, FORMAT_RRR)

/**
 * OP_FLOAT_SUB — floating-point subtraction
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs - rt.
 */
X(OP_FLOAT_SUB, FORMAT_RRR)

/**
 * OP_FLOAT_MUL — floating-point multiplication
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs * rt.
 */
X(OP_FLOAT_MUL, FORMAT_RRR)

/**
 * OP_FLOAT_DIV — floating-point division
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs / rt.
 */
X(OP_FLOAT_DIV, FORMAT_RRR)

/**
 * OP_FLOAT_MOD — floating-point modulo
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = fmod(rs, rt) using IEEE 754 semantics.
 */
X(OP_FLOAT_MOD, FORMAT_RRR)

/**
 * OP_FLOAT_CMPL — floating-point compare (CMPL)
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     rd = -1 if rs < rt
 *     rd =  0 if rs == rt
 *     rd =  1 if rs > rt
 *     rd = -1 if either operand is NaN
 */
X(OP_FLOAT_CMPL, FORMAT_RRR)

/**
 * OP_FLOAT_CMPG — floating-point compare (CMPG)
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     rd = -1 if rs < rt
 *     rd =  0 if rs == rt
 *     rd =  1 if rs > rt
 *     rd =  1 if either operand is NaN
 */
X(OP_FLOAT_CMPG, FORMAT_RRR)

/*---------------------------------------------------------------+
 |  Floating-Point Unary Operations                              |
 +---------------------------------------------------------------*/

/**
 * OP_FLOAT_NEG — floating-point negation
 *
 * FORMAT_RxRx:
 *     | op:8 | rd:12 | rs:12 |
 *
 * Details:
 *     Computes rd = -rs.
 */
X(OP_FLOAT_NEG, FORMAT_RxRx)

/*---------------------------------------------------------------+
 |  Numeric Conversion Instructions                              |
 +---------------------------------------------------------------*/

/**
 * OP_FLT_TO_INT — convert float to int
 *
 * FORMAT_RxRx:
 *     | op:8 | rd:12 | rs:12 |
 *
 * Details:
 *     Converts rs (float) to an integer value and stores it in rd.
 *     Semantics follow C-style cast: truncation toward zero.
 */
X(OP_FLT_TO_INT, FORMAT_RxRx)

/**
 * OP_INT_TO_FLT — convert int to float
 *
 * FORMAT_RxRx:
 *     | op:8 | rd:12 | rs:12 |
 *
 * Details:
 *     Converts rs (int) to a floating-point value and stores it in rd.
 */
X(OP_INT_TO_FLT, FORMAT_RxRx)

/*---------------------------------------------------------------+
 |  Boolean Logical Operations                                   |
 +---------------------------------------------------------------*/

/**
 * OP_LAND — logical AND
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = (rs && rt).
 *     Operands must be boolean values (0 or 1).
 */
X(OP_LAND, FORMAT_RRR)

/**
 * OP_LOR — logical OR
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = (rs || rt).
 *     Operands must be boolean values (0 or 1).
 */
X(OP_LOR, FORMAT_RRR)

/**
 * OP_LNOT — logical NOT
 *
 * FORMAT_RxRx:
 *     | op:8 | rd:12 | rs:12 |
 *
 * Details:
 *     Computes rd = !rs.
 *     Operand must be a boolean value (0 or 1).
 */
X(OP_LNOT, FORMAT_RxRx)

/*---------------------------------------------------------------+
 |  Jump and Branch Instructions                                 |
 +---------------------------------------------------------------*/

/**
 * OP_JMP — unconditional jump
 *
 * FORMAT_JMP:
 *     | op:8 | -----:8 | offset:16 |
 *
 * Details:
 *     Unconditionally jumps: pc += offset.
 */
X(OP_JMP, FORMAT_JMP)

/**
 * OP_JMP_TRUE — conditional jump if true
 *
 * FORMAT_ROff2:
 *     | op:8 | rd:8 | offset:16 |
 *
 * Details:
 *     If rd is true (non-zero), pc += offset.
 */
X(OP_JMP_TRUE, FORMAT_ROff2)

/**
 * OP_JMP_FALSE — conditional jump if false
 *
 * FORMAT_ROff2:
 *     | op:8 | rd:8 | offset:16 |
 *
 * Details:
 *     If rd is false (zero), pc += offset.
 */
X(OP_JMP_FALSE, FORMAT_ROff2)

/*---------------------------------------------------------------+
 |  Fused Integer Compare + Jump Instructions                    |
 +---------------------------------------------------------------*/

/**
 * OP_JMP_INT_EQ — jump if equal
 *
 * FORMAT_RRImm:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If rs == rt, pc += offset.
 */
X(OP_JMP_INT_EQ, FORMAT_RRImm)

/**
 * OP_JMP_INT_EQ_IMM — jump if equal (immediate)
 *
 * FORMAT_RImmOff:
 *     | op:8 | rs:8 | imm:8 | offset:8 |
 *
 * Details:
 *     If rs == imm, pc += offset.
 */
X(OP_JMP_INT_EQ_IMM, FORMAT_RImmOff)

/**
 * OP_JMP_INT_NE — jump if not equal
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If rs != rt, pc += offset.
 */
X(OP_JMP_INT_NE, FORMAT_RROff)

/**
 * OP_JMP_INT_NE_IMM — jump if not equal (immediate)
 *
 * FORMAT_RImmOff:
 *     | op:8 | rs:8 | imm:8 | offset:8 |
 *
 * Details:
 *     If rs != imm, pc += offset.
 */
X(OP_JMP_INT_NE_IMM, FORMAT_RImmOff)

/**
 * OP_JMP_INT_LT — jump if less-than
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If rs < rt, pc += offset.
 */
X(OP_JMP_INT_LT, FORMAT_RROff)

/**
 * OP_JMP_INT_LT_IMM — jump if less-than (immediate)
 *
 * FORMAT_RImmOff:
 *     | op:8 | rs:8 | imm:8 | offset:8 |
 *
 * Details:
 *     If rs < imm, pc += offset.
 */
X(OP_JMP_INT_LT_IMM, FORMAT_RImmOff)

/**
 * OP_JMP_INT_LE — jump if less-or-equal
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If rs <= rt, pc += offset.
 */
X(OP_JMP_INT_LE, FORMAT_RROff)

/**
 * OP_JMP_INT_LE_IMM — jump if less-or-equal (immediate)
 *
 * FORMAT_RImmOff:
 *     | op:8 | rs:8 | imm:8 | offset:8 |
 *
 * Details:
 *     If rs <= imm, pc += offset.
 */
X(OP_JMP_INT_LE_IMM, FORMAT_RImmOff)

/**
 * OP_JMP_INT_GT — jump if greater-than
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If rs > rt, pc += offset.
 */
X(OP_JMP_INT_GT, FORMAT_RROff)

/**
 * OP_JMP_INT_GT_IMM — jump if greater-than (immediate)
 *
 * FORMAT_RImmOff:
 *     | op:8 | rs:8 | imm:8 | offset:8 |
 *
 * Details:
 *     If rs > imm, pc += offset.
 */
X(OP_JMP_INT_GT_IMM, FORMAT_RImmOff)

/**
 * OP_JMP_INT_GE — jump if greater-or-equal
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If rs >= rt, pc += offset.
 */
X(OP_JMP_INT_GE, FORMAT_RROff)

/**
 * OP_JMP_INT_GE_IMM — jump if greater-or-equal (immediate)
 *
 * FORMAT_RImmOff:
 *     | op:8 | rs:8 | imm:8 | offset:8 |
 *
 * Details:
 *     If rs >= imm, pc += offset.
 */
X(OP_JMP_INT_GE_IMM, FORMAT_RImmOff)

/*---------------------------------------------------------------+
 |  Fused Unsigned Integer Compare + Jump Instructions           |
 +---------------------------------------------------------------*/

/**
 * OP_JMP_UINT_LT — jump if unsigned less-than
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If rs < rt (unsigned), pc += offset.
 */
X(OP_JMP_UINT_LT, FORMAT_RROff)

/**
 * OP_JMP_UINT_LT_IMM — jump if unsigned less-than (immediate)
 *
 * FORMAT_RImmOff:
 *     | op:8 | rs:8 | imm:8 | offset:8 |
 *
 * Details:
 *     If rs < imm (unsigned), pc += offset.
 */
X(OP_JMP_UINT_LT_IMM, FORMAT_RImmOff)

/**
 * OP_JMP_UINT_LE — jump if unsigned less-or-equal
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If rs <= rt (unsigned), pc += offset.
 */
X(OP_JMP_UINT_LE, FORMAT_RROff)

/**
 * OP_JMP_UINT_LE_IMM — jump if unsigned less-or-equal (immediate)
 *
 * FORMAT_RImmOff:
 *     | op:8 | rs:8 | imm:8 | offset:8 |
 *
 * Details:
 *     If rs <= imm (unsigned), pc += offset.
 */
X(OP_JMP_UINT_LE_IMM, FORMAT_RImmOff)

/**
 * OP_JMP_UINT_GT — jump if unsigned greater-than
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If rs > rt (unsigned), pc += offset.
 */
X(OP_JMP_UINT_GT, FORMAT_RROff)

/**
 * OP_JMP_UINT_GT_IMM — jump if unsigned greater-than (immediate)
 *
 * FORMAT_RImmOff:
 *     | op:8 | rs:8 | imm:8 | offset:8 |
 *
 * Details:
 *     If rs > imm (unsigned), pc += offset.
 */
X(OP_JMP_UINT_GT_IMM, FORMAT_RImmOff)

/**
 * OP_JMP_UINT_GE — jump if unsigned greater-or-equal
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If rs >= rt (unsigned), pc += offset.
 */
X(OP_JMP_UINT_GE, FORMAT_RROff)

/**
 * OP_JMP_UINT_GE_IMM — jump if unsigned greater-or-equal (immediate)
 *
 * FORMAT_RImmOff:
 *     | op:8 | rs:8 | imm:8 | offset:8 |
 *
 * Details:
 *     If rs >= imm (unsigned), pc += offset.
 */
X(OP_JMP_UINT_GE_IMM, FORMAT_RImmOff)

/*---------------------------------------------------------------+
 |  Stack Push Instructions (used for call only)                 |
 +---------------------------------------------------------------*/

/**
 * OP_PUSH — push register value onto the stack
 *
 * FORMAT_Rx:
 *     | op:8 | ---:12 | Rx(rs):12 |
 *
 * Details:
 *     Pushes the value of register rs onto the VM stack.
 */
X(OP_PUSH, FORMAT_Rx)

/**
 * OP_PUSH_INT_IMM — push integer immediate onto the stack
 *
 * FORMAT_Imm2:
 *     | op:8 | ---:8 | imm16:16 |
 *
 * Details:
 *     Pushes a 16-bit signed integer immediate onto the VM stack.
 */
X(OP_PUSH_INT_IMM, FORMAT_Imm2)

/**
 * OP_PUSH_TAG — push small tagged value (bool / special float / none)
 *
 * FORMAT_Imm2:
 *     | op:8 | ---:8 | imm:16 |
 *
 * Details:
 *     Pushes a small tagged value (boolean, special float, or none) onto the stack.
 */
X(OP_PUSH_TAG, FORMAT_Imm2)

/**
 * OP_PUSH_CONST — push constant pool entry onto the stack
 *
 * FORMAT_Idx2:
 *     | op:8 | ---:8 | index:16 |
 *
 * Details:
 *     Pushes CP[idx] onto the VM stack.
 */
X(OP_PUSH_CONST, FORMAT_Idx2)

/*---------------------------------------------------------------+
 |  Unified Call Instruction                                     |
 +---------------------------------------------------------------*/

/**
 * OP_CALL — unified call instruction family
 *
 * FORMAT_CALL:
 *     31                                           0
 *     | op:8 | flag:4 | A(ret-reg):12 | B(nargs):8 |
 *     | payload (32-bit)                           |
 *
 * Details:
 *     A unified call instruction format. The 'flag' field determines
 *     the call subtype:
 *
 *         flag = 0  →  direct call within the same module
 *         flag = 1  →  external function call (import-index)
 *         flag = 2  →  interface method call (intf-id + method-slot)
 *
 *     The second 32-bit word (payload) is interpreted differently
 *     depending on the flag:
 *
 *         flag = 0 (direct call):
 *             payload = relative-offset (signed 32-bit)
 *
 *         flag = 1 (external call):
 *             payload = import-index (unsigned 32-bit)
 *
 *         flag = 2 (interface call):
 *             payload = (intf-id:16 | method-slot:16)
 *
 *     This unified encoding reduces opcode count and keeps all call
 *     instructions consistent while still supporting:
 *         - intra-module direct calls (fast PC-relative)
 *         - cross-module calls via import table
 *         - interface dynamic dispatch via TypeInfo
 */
X(OP_CALL,              FORMAT_CALL)

/*---------------------------------------------------------------+
 |  Return Instructions                                          |
 +---------------------------------------------------------------*/

/**
 * OP_RET — return register value
 *
 * FORMAT_Rx:
 *     | op:8 | ---:12 | Rx(rs):12 |
 *
 * Details:
 *     Returns the value in register rs to the caller.
 */
X(OP_RET, FORMAT_Rx)

/**
 * OP_RET_INT_IMM — return integer immediate
 *
 * FORMAT_Imm2:
 *     | op:8 | ---:8 | imm:16 |
 *
 * Details:
 *     Returns a 16-bit signed integer immediate to the caller.
 */
X(OP_RET_INT_IMM, FORMAT_Imm2)

/**
 * OP_RET_TAG — return small tagged value (bool / special float / none)
 *
 * FORMAT_Imm2:
 *     | op:8 | ---:8 | imm:16 |
 *
 * Details:
 *     Returns a small tagged value to the caller.
 */
X(OP_RET_TAG, FORMAT_Imm2)

/**
 * OP_RET_CONST — return constant pool entry
 *
 * FORMAT_Idx2:
 *     | op:8 | ---:8 | index:16 |
 *
 * Details:
 *     Returns CP[idx] to the caller.
 */
X(OP_RET_CONST, FORMAT_Idx2)

/**
 * OP_RET_VOID — return void
 *
 * FORMAT_Op:
 *     | op:8 | ------------------------:24 |
 *
 * Details:
 *     Returns void (no value) to the caller.
 */
X(OP_RET_VOID, FORMAT_Op)

/*---------------------------------------------------------------+
 |  Global Variable Access Instructions                           |
 +---------------------------------------------------------------*/

/**
 * OP_GLOBAL_GET — load global variable
 *
 * FORMAT_AxBx:
 *     | op:8 | Ax(dst):12 | Bx(global-index):12 |
 *
 * Details:
 *     Loads the value of a global variable into register dst.
 *     The global-index refers to the module's global table.
 *     Globals are resolved at module load time and stored in
 *     Module.globals[].
 */
X(OP_GLOBAL_GET, FORMAT_Op)

/**
 * OP_GLOBAL_SET — store global variable
 *
 * FORMAT_AxBx:
 *     | op:8 | Ax(src):12 | Bx(global-index):12 |
 *
 * Details:
 *     Stores the value in register src into a global variable.
 *     The global-index refers to the module's global table.
 *     Writes may trigger GC barriers depending on the value type.
 */
X(OP_GLOBAL_SET, FORMAT_Op)

/*---------------------------------------------------------------+
 |  Field Access Instructions                                    |
 +---------------------------------------------------------------*/

/**
 * OP_FIELD_LOAD — load field from object (same module)
 *
 * FORMAT_ABC:
 *     | op:8 | A(dst):8 | B(obj):8 | C(field-off):8 |
 *
 * Details:
 *     Loads a field from an object using a compile-time constant
 *     field offset. Used only for fields defined in the same module.
 */
X(OP_FIELD_LOAD, FORMAT_Op)

/**
 * OP_FIELD_STORE — store field into object (same module)
 *
 * FORMAT_ABC:
 *     | op:8 | A(obj):8 | B(field-off):8 | C(src):8 |
 *
 * Details:
 *     Stores a value into a field using a compile-time constant
 *     field offset. Used only for fields defined in the same module.
 */
X(OP_FIELD_STORE, FORMAT_Op)

/**
 * OP_FIELD_LOAD_EXT — load field from external class
 *
 * FORMAT_ABC:
 *     | op:8 | A(dst):8 | B(obj):8 | C(import-index):8 |
 *
 * Details:
 *     Loads a field defined in another module. The import-index
 *     refers to an ImportEntry of kind IMPORT_FIELD. The loader
 *     resolves the field offset and fills ImportEntry.resolved.field_offset.
 */
X(OP_FIELD_LOAD_EXT, FORMAT_Op)

/**
 * OP_FIELD_STORE_EXT — store field into external class
 *
 * FORMAT_ABC:
 *     | op:8 | A(obj):8 | B(import-index):8 | C(src):8 |
 *
 * Details:
 *     Stores a value into a field defined in another module. The
 *     import-index refers to an ImportEntry of kind IMPORT_FIELD.
 *     The loader resolves the field offset and fills
 *     ImportEntry.resolved.field_offset.
 */
X(OP_FIELD_STORE_EXT, FORMAT_Op)

/*---------------------------------------------------------------+
 |  Type Casting & Interface Casting Instructions                |
 +---------------------------------------------------------------*/

/**
 * OP_CAST_INTF — cast object to interface
 *
 * FORMAT_ABC:
 *     | op:8 | A(dst):8 | B(obj):8 | C(intf-id):8 |
 *
 * Details:
 *     Attempts to cast object 'obj' to the interface identified by
 *     intf-id. If the object's TypeInfo implements the interface,
 *     the cast succeeds and dst = obj. Otherwise, an exception is
 *     raised (same semantics as OP_RAISE).
 *
 *     intf-id refers to a compile-time assigned interface index
 *     within the module's TypeInfo table.
 */
X(OP_CAST_INTF, FORMAT_Op)


/*---------------------------------------------------------------+
 |  Subscription (Indexing) Instructions                         |
 +---------------------------------------------------------------*/

/**
 * OP_SUBSCR_LOAD — load element via subscription
 *
 * FORMAT_Ax:
 *     | op:8 | ---:12 | Ax(rs):12 |
 *
 * Details:
 *     Loads the element at index stack[top] from container rs.
 *     The index is popped from the stack. The result is pushed.
 *
 *     Supported container types:
 *         - arrays
 *         - strings
 *         - user-defined indexable types (via metamethods or vtable)
 */
X(OP_SUBSCR_LOAD, FORMAT_Op)

/**
 * OP_SUBSCR_STORE — store element via subscription
 *
 * FORMAT_Ax:
 *     | op:8 | ---:12 | Ax(rs):12 |
 *
 * Details:
 *     Stores stack[top] into container rs at index stack[top-1].
 *     Pops both index and value.
 *
 *     Supported container types mirror OP_SUBSCR_LOAD.
 */
X(OP_SUBSCR_STORE, FORMAT_Op)

/*---------------------------------------------------------------+
 |  Iterator Protocol Instructions                               |
 +---------------------------------------------------------------*/

/**
 * OP_GET_ITER — obtain iterator from object
 *
 * FORMAT_Ax:
 *     | op:8 | ---:12 | Ax(obj):12 |
 *
 * Details:
 *     Produces an iterator object for 'obj' and pushes it onto
 *     the stack. The iterator protocol is defined by TypeInfo:
 *
 *         - arrays → array iterator
 *         - strings → character iterator
 *         - user-defined types → __iter__ or vtable entry
 */
X(OP_GET_ITER, FORMAT_Op)

/**
 * OP_ITER_NEXT — advance iterator
 *
 * FORMAT_Ax:
 *     | op:8 | ---:12 | Ax(iter):12 |
 *
 * Details:
 *     Advances the iterator 'iter'. If iteration continues, pushes
 *     the next value and returns true. If iteration ends, pushes
 *     false.
 *
 *     This instruction is designed to support:
 *         - for-in loops
 *         - generator-like patterns
 *         - custom iterable types
 */
X(OP_ITER_NEXT, FORMAT_Op)

/*---------------------------------------------------------------+
 |  Type Testing & Safe Casting Instructions                     |
 +---------------------------------------------------------------*/

/**
 * OP_AS — safe cast
 *
 * FORMAT_Ax:
 *     | op:8 | ---:12 | Ax(obj):12 |
 *
 * Details:
 *     Performs a safe cast of obj to the target type encoded in
 *     the following instruction (usually OP_WIDE or a metadata
 *     operand). If the cast succeeds, pushes the casted value.
 *     If it fails, pushes 'none' instead of raising.
 *
 *     This is equivalent to C#'s "as" operator.
 */
X(OP_AS, FORMAT_Op)

/**
 * OP_IS — type test
 *
 * FORMAT_Ax:
 *     | op:8 | ---:12 | Ax(obj):12 |
 *
 * Details:
 *     Tests whether obj is of the target type encoded in the
 *     following instruction. Pushes true or false.
 *
 *     This is equivalent to C#'s "is" operator.
 */
X(OP_IS, FORMAT_Op)

/*---------------------------------------------------------------+
 |  Miscellaneous / Special Instructions                         |
 +---------------------------------------------------------------*/

/**
 * OP_WIDE — extend the next instruction's operand fields
 *
 * FORMAT_WIDE:
 *     | op:8 | imm:8 | imm:8 | imm:8 |
 *
 * Details:
 *     Extends the next instruction by providing high 8 bits for
 *     its A/B/C fields. The next instruction consumes these bits
 *     and forms 16-bit or 24-bit operands depending on its format.
 */
X(OP_WIDE, FORMAT_WIDE)

/**
 * OP_RAISE — raise an exception
 *
 * FORMAT_Op:
 *     | op:8 | ------------------------:24 |
 *
 * Details:
 *     Raises an exception. The VM unwinds the call stack until a
 *     handler is found or terminates execution if none exists.
 */
X(OP_RAISE, FORMAT_Op)


/*---------------------------------------------------------------+
 |  IR-Only Pseudo Instructions                                  |
 +---------------------------------------------------------------*/

/* Binary arithmetic (IR only) */
X(OP_BINARY_ADD,    FORMAT_IR)
X(OP_BINARY_SUB,    FORMAT_IR)
X(OP_BINARY_MUL,    FORMAT_IR)
X(OP_BINARY_DIV,    FORMAT_IR)
X(OP_BINARY_MOD,    FORMAT_IR)

/* Binary bitwise (IR only) */
X(OP_BINARY_AND,    FORMAT_IR)
X(OP_BINARY_OR,     FORMAT_IR)
X(OP_BINARY_XOR,    FORMAT_IR)

/* Binary shifts (IR only) */
X(OP_BINARY_SHL,    FORMAT_IR)
X(OP_BINARY_SHR,    FORMAT_IR)

/* Binary comparisons (IR only) */
X(OP_BINARY_CMPEQ,  FORMAT_IR)
X(OP_BINARY_CMPNE,  FORMAT_IR)
X(OP_BINARY_CMPLT,  FORMAT_IR)
X(OP_BINARY_CMPLE,  FORMAT_IR)
X(OP_BINARY_CMPGT,  FORMAT_IR)
X(OP_BINARY_CMPGE,  FORMAT_IR)

/* Unary ops (IR only) */
X(OP_UNARY_NEG,     FORMAT_IR)
X(OP_UNARY_NOT,     FORMAT_IR)

/* Structural IR ops */
X(OP_IR_LOCAL,      FORMAT_IR)
X(OP_IR_CALL,       FORMAT_IR)
X(OP_IR_JMP_COND,   FORMAT_IR)
X(OP_IR_PHI,        FORMAT_IR)

/* Non-executable data slot (pseudo-instruction) */
X(OP_DATA, FORMAT_DATA)

// clang-format on
