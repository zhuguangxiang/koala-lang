
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
X(OP_NOP, FORMAT_Op, "nop")

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
X(OP_MOVE, FORMAT_RxRx, "move")

/**
 * OP_LOAD_INT_IMM — load typed integer immediate (12-bit immediate)
 *
 * FORMAT_R_TI_Imm12:
 *     | op:8 | rd:8 | ti:4 | imm:12 |
 *
 * Fields:
 *     ti       — 4-bit type info:
 *              bit[3]   : integer flag (must be 1 for integer types)
 *              bit[2]   : sign (0 = signed, 1 = unsigned)
 *              bit[1:0] : width selector:
 *              0 = 8-bit
 *              1 = 16-bit
 *              2 = 32-bit
 *              3 = 64-bit
 *
 * Details:
 *    Loads a 12-bit signed integer immediate into rd, with the width specified by ti.
 */
X(OP_LOAD_INT_IMM, FORMAT_R_TI_Imm12, "load_int_imm")

 /**
 * OP_LOAD_UINT_IMM — load typed unsigned integer immediate (12-bit immediate)
 *
 * FORMAT_R_TI_Imm12:
 *     | op:8 | rd:8 | ti:4 | imm:12 |
 *
 * Fields:
 *     ti: the same as OP_LOAD_INT_IMM
 *
 * Details:
 *    Loads a 12-bit unsigned integer immediate into rd, with the width specified by ti.
 */
X(OP_LOAD_UINT_IMM, FORMAT_R_TI_Imm12, "load_uint_imm")

/**
 * OP_LOAD_TAG — load small tagged constant
 *
 * FORMAT_RxTag:
 *     | op:8 | ---:4 | rd:12 | imm:8 |
 *
 * Details:
 *     Loads a small tagged constant into rd. The imm field encodes:
 *         - boolean values(0, 1)
 *         - none / null(2)
 *         - special float values (e.g., +0.0(3), -0.0(4), NaN(5), -inf(6), +inf(7))
 *     This avoids constant-pool lookup for common values.
 */
X(OP_LOAD_TAG, FORMAT_RxTag, "load_tag")

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
X(OP_LOADK, FORMAT_RIdx2, "loadk")

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
X(OP_INT_ADD, FORMAT_RRR, "int.add")

/**
 * OP_INT_ADD_IMM — integer addition with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs + imm, where imm is an 8-bit signed immediate.
 */
X(OP_INT_ADD_IMM, FORMAT_RRImm, "int.add_imm")

/**
 * OP_INT_SUB — integer subtraction
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs - rt.
 */
X(OP_INT_SUB, FORMAT_RRR, "int.sub")

/**
 * OP_INT_SUB_IMM — integer subtraction with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs - imm, where imm is an 8-bit signed immediate.
 */
X(OP_INT_SUB_IMM, FORMAT_RRImm, "int.sub_imm")

/**
 * OP_INT_MUL — integer multiplication
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs * rt.
 */
X(OP_INT_MUL, FORMAT_RRR, "int.mul")

/**
 * OP_INT_MUL_IMM — integer multiplication with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs * imm, where imm is an 8-bit signed immediate.
 */
X(OP_INT_MUL_IMM, FORMAT_RRImm, "int.mul_imm")

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
X(OP_INT_DIV, FORMAT_RRR, "int.div")

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
X(OP_INT_DIV_IMM, FORMAT_RRImm, "int.div_imm")

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
X(OP_INT_MOD, FORMAT_RRR, "int.mod")

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
X(OP_INT_MOD_IMM, FORMAT_RRImm, "int.mod_imm")

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
X(OP_INT_AND, FORMAT_RRR, "int.and")

/**
 * OP_INT_AND_IMM — integer bitwise AND with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs & imm, where imm is an 8-bit signed immediate.
 */
X(OP_INT_AND_IMM, FORMAT_RRImm, "int.and_imm")

/**
 * OP_INT_OR — integer bitwise OR
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs | rt.
 */
X(OP_INT_OR, FORMAT_RRR, "int.or")

/**
 * OP_INT_OR_IMM — integer bitwise OR with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs | imm, where imm is an 8-bit signed immediate.
 */
X(OP_INT_OR_IMM, FORMAT_RRImm, "int.or_imm")

/**
 * OP_INT_XOR — integer bitwise XOR
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs ^ rt.
 */
X(OP_INT_XOR, FORMAT_RRR, "int.xor")

/**
 * OP_INT_XOR_IMM — integer bitwise XOR with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs ^ imm, where imm is an 8-bit signed immediate.
 */
X(OP_INT_XOR_IMM, FORMAT_RRImm, "int.xor_imm")

/**
 * OP_INT_SHL — integer left shift
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs << rt. Only the low 5 bits of rt are used.
 */
X(OP_INT_SHL, FORMAT_RRR, "int.shl")

/**
 * OP_INT_SHL_IMM — integer left shift with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs << imm, where imm is an 8-bit unsigned shift amount.
 */
X(OP_INT_SHL_IMM, FORMAT_RRImm, "int.shl_imm")

/**
 * OP_INT_SHR — integer arithmetic right shift
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs >> rt (arithmetic shift). Only the low 5 bits of rt are used.
 */
X(OP_INT_SHR, FORMAT_RRR, "int.shr")

/**
 * OP_INT_SHR_IMM — integer arithmetic right shift with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs >> imm (arithmetic shift), where imm is an 8-bit unsigned shift amount.
 */
X(OP_INT_SHR_IMM, FORMAT_RRImm, "int.shr_imm")

/*---------------------------------------------------------------+
 |  Integer Comparison Operations                                |
 +---------------------------------------------------------------*/

/**
 * OP_INT_EQ — integer equality comparison
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = (rs == rt).
 */
X(OP_INT_EQ, FORMAT_RRR, "int.eq")

/**
 * OP_INT_EQ_IMM — integer equality comparison with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = (rs == imm), where imm is an 8-bit signed immediate.
 */
X(OP_INT_EQ_IMM, FORMAT_RRImm, "int.eq_imm")

/**
 * OP_INT_NE — integer inequality comparison
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = (rs != rt).
 */
X(OP_INT_NE, FORMAT_RRR, "int.ne")

/**
 * OP_INT_NE_IMM — integer inequality comparison with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = (rs != imm), where imm is an 8-bit signed immediate.
 */
X(OP_INT_NE_IMM, FORMAT_RRImm, "int.ne_imm")

/**
 * OP_INT_LT — integer less-than comparison
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = (rs < rt), using signed integer comparison.
 */
X(OP_INT_LT, FORMAT_RRR, "int.lt")

/**
 * OP_INT_LT_IMM — integer less-than comparison with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = (rs < imm), where imm is an 8-bit signed immediate.
 */
X(OP_INT_LT_IMM, FORMAT_RRImm, "int.lt_imm")

/**
 * OP_INT_LE — integer less-or-equal comparison
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = (rs <= rt), using signed integer comparison.
 */
X(OP_INT_LE, FORMAT_RRR, "int.le")

/**
 * OP_INT_LE_IMM — integer less-or-equal comparison with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = (rs <= imm), where imm is an 8-bit signed immediate.
 */
X(OP_INT_LE_IMM, FORMAT_RRImm, "int.le_imm")

/**
 * OP_INT_GT — integer greater-than comparison
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = (rs > rt), using signed integer comparison.
 */
X(OP_INT_GT, FORMAT_RRR, "int.gt")

/**
 * OP_INT_GT_IMM — integer greater-than comparison with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = (rs > imm), where imm is an 8-bit signed immediate.
 */
X(OP_INT_GT_IMM, FORMAT_RRImm, "int.gt_imm")

/**
 * OP_INT_GE — integer greater-or-equal comparison
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = (rs >= rt), using signed integer comparison.
 */
X(OP_INT_GE, FORMAT_RRR, "int.ge")

/**
 * OP_INT_GE_IMM — integer greater-or-equal comparison with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = (rs >= imm), where imm is an 8-bit signed immediate.
 */
X(OP_INT_GE_IMM, FORMAT_RRImm, "int.ge_imm")

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
X(OP_INT_NEG, FORMAT_RxRx, "int.neg")

/**
 * OP_INT_NOT — integer bitwise NOT
 *
 * FORMAT_RxRx:
 *     | op:8 | rd:12 | rs:12 |
 *
 * Details:
 *     Computes rd = ~rs (bitwise complement).
 */
X(OP_INT_NOT, FORMAT_RxRx, "int.not")

/*---------------------------------------------------------------+
 |  Unsigned Integer Operations                                  |
 +---------------------------------------------------------------*/

/**
 * OP_UINT_ADD_IMM — unsigned integer addition with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs + imm, where imm is an 8-bit unsigned immediate.
 */
X(OP_UINT_ADD_IMM, FORMAT_RRImm, "uint.add_imm")

/**
 * OP_UINT_SUB_IMM — unsigned integer subtraction with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs - imm, where imm is an 8-bit unsigned immediate.
 */
X(OP_UINT_SUB_IMM, FORMAT_RRImm, "uint.sub_imm")

/**
 * OP_UINT_MUL_IMM — unsigned integer multiplication with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs * imm, where imm is an 8-bit unsigned immediate.
 */
X(OP_UINT_MUL_IMM, FORMAT_RRImm, "uint.mul_imm")

/**
 * OP_UINT_DIV — unsigned integer division
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs / rt using unsigned division semantics.
 */
X(OP_UINT_DIV, FORMAT_RRR, "uint.div")

/**
 * OP_UINT_DIV_IMM — unsigned integer division with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs / imm using unsigned division semantics.
 */
X(OP_UINT_DIV_IMM, FORMAT_RRImm, "uint.div_imm")

/**
 * OP_UINT_MOD — unsigned integer modulo
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs % rt using unsigned modulo semantics.
 */
X(OP_UINT_MOD, FORMAT_RRR, "uint.mod")

/**
 * OP_UINT_MOD_IMM — unsigned integer modulo with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs % imm using unsigned modulo semantics.
 */
X(OP_UINT_MOD_IMM, FORMAT_RRImm, "uint.mod_imm")

/**
 * OP_UINT_AND_IMM — unsigned integer bitwise AND with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs & imm, where imm is an 8-bit unsigned immediate.
 */
X(OP_UINT_AND_IMM, FORMAT_RRImm, "uint.and_imm")

/**
 * OP_UINT_OR_IMM — unsigned integer bitwise OR with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs | imm, where imm is an 8-bit unsigned immediate.
 */
X(OP_UINT_OR_IMM, FORMAT_RRImm, "uint.or_imm")

/**
 * OP_UINT_XOR_IMM — unsigned integer bitwise XOR with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs ^ imm, where imm is an 8-bit unsigned immediate.
 */
X(OP_UINT_XOR_IMM, FORMAT_RRImm, "uint.xor_imm")

/**
 * OP_UINT_SHL_IMM — unsigned integer left shift with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs << imm, where imm is an 8-bit unsigned shift amount.
 */
X(OP_UINT_SHL_IMM, FORMAT_RRImm, "uint.shl_imm")

/**
 * OP_UINT_SHR — unsigned logical right shift
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs >> rt (logical shift). High bits are filled with zero.
 */
X(OP_UINT_SHR, FORMAT_RRR, "uint.shr")

/**
 * OP_UINT_SHR_IMM — unsigned logical right shift with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = rs >> imm (logical shift). High bits are filled with zero.
 */
X(OP_UINT_SHR_IMM, FORMAT_RRImm, "uint.shr_imm")

/*---------------------------------------------------------------+
 |  Unsigned Integer Comparison Operations                       |
 +---------------------------------------------------------------*/

/**
 * OP_UINT_EQ_IMM — unsigned integer equality comparison with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = (rs == imm), where imm is an 8-bit unsigned immediate.
 */
X(OP_UINT_EQ_IMM, FORMAT_RRImm, "uint.eq_imm")

/**
 * OP_UINT_NE_IMM — unsigned integer inequality comparison with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = (rs != imm), where imm is an 8-bit unsigned immediate.
 */
X(OP_UINT_NE_IMM, FORMAT_RRImm, "uint.ne_imm")

/**
 * OP_UINT_LT — unsigned less-than comparison
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = (rs < rt) using unsigned comparison.
 */
X(OP_UINT_LT, FORMAT_RRR, "uint.lt")

/**
 * OP_UINT_LT_IMM — unsigned less-than comparison with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = (rs < imm) using unsigned comparison.
 */
X(OP_UINT_LT_IMM, FORMAT_RRImm, "uint.lt_imm")

/**
 * OP_UINT_LE — unsigned less-or-equal comparison
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = (rs <= rt) using unsigned comparison.
 */
X(OP_UINT_LE, FORMAT_RRR, "uint.le")

/**
 * OP_UINT_LE_IMM — unsigned less-or-equal comparison with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = (rs <= imm) using unsigned comparison.
 */
X(OP_UINT_LE_IMM, FORMAT_RRImm, "uint.le_imm")

/**
 * OP_UINT_GT — unsigned greater-than comparison
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = (rs > rt) using unsigned comparison.
 */
X(OP_UINT_GT, FORMAT_RRR, "uint.gt")

/**
 * OP_UINT_GT_IMM — unsigned greater-than comparison with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = (rs > imm) using unsigned comparison.
 */
X(OP_UINT_GT_IMM, FORMAT_RRImm, "uint.gt_imm")

/**
 * OP_UINT_GE — unsigned greater-or-equal comparison
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = (rs >= rt) using unsigned comparison.
 */
X(OP_UINT_GE, FORMAT_RRR, "uint.ge")

/**
 * OP_UINT_GE_IMM — unsigned greater-or-equal comparison with immediate
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Details:
 *     Computes rd = (rs >= imm) using unsigned comparison.
 */
X(OP_UINT_GE_IMM, FORMAT_RRImm, "uint.ge_imm")

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
X(OP_FLOAT_ADD, FORMAT_RRR, "float.add")

/**
 * OP_FLOAT_SUB — floating-point subtraction
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs - rt.
 */
X(OP_FLOAT_SUB, FORMAT_RRR, "float.sub")

/**
 * OP_FLOAT_MUL — floating-point multiplication
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs * rt.
 */
X(OP_FLOAT_MUL, FORMAT_RRR, "float.mul")

/**
 * OP_FLOAT_DIV — floating-point division
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = rs / rt.
 */
X(OP_FLOAT_DIV, FORMAT_RRR, "float.div")

/**
 * OP_FLOAT_MOD — floating-point modulo
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Computes rd = fmod(rs, rt) using IEEE 754 semantics.
 */
X(OP_FLOAT_MOD, FORMAT_RRR, "float.mod")

/**
 * OP_FLOAT_EQ — floating‑point compare (EQ)
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     rd = 1  if rs == rt
 *     rd = 0  otherwise
 *
 * Notes:
 *     - If either operand is NaN, the comparison is false → rd = 0
 *     - +0.0 and -0.0 are considered equal
 */
X(OP_FLOAT_EQ, FORMAT_RRR, "float.eq")

/**
 * OP_FLOAT_NE — floating‑point compare (NE)
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     rd = 1  if rs != rt
 *     rd = 0  otherwise
 *
 * Notes:
 *     - If either operand is NaN, the comparison is true → rd = 1
 *     - +0.0 and -0.0 are considered equal (so NE = 0)
 */
X(OP_FLOAT_NE, FORMAT_RRR, "float.ne")

/**
 * OP_FLOAT_LT — floating‑point compare (LT)
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     rd = 1  if rs < rt
 *     rd = 0  otherwise
 *
 * Notes:
 *     - If either operand is NaN, the comparison is false → rd = 0
 */
X(OP_FLOAT_LT, FORMAT_RRR, "float.lt")

/**
 * OP_FLOAT_LE — floating‑point compare (LE)
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     rd = 1  if rs <= rt
 *     rd = 0  otherwise
 *
 * Notes:
 *     - If either operand is NaN, the comparison is false → rd = 0
 *     - +0.0 <= -0.0 and -0.0 <= +0.0 are both true
 */
X(OP_FLOAT_LE, FORMAT_RRR, "float.le")

/**
 * OP_FLOAT_GT — floating‑point compare (GT)
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     rd = 1  if rs > rt
 *     rd = 0  otherwise
 *
 * Notes:
 *     - If either operand is NaN, the comparison is false → rd = 0
 */
X(OP_FLOAT_GT, FORMAT_RRR, "float.gt")

/**
 * OP_FLOAT_GE — floating‑point compare (GE)
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     rd = 1  if rs >= rt
 *     rd = 0  otherwise
 *
 * Notes:
 *     - If either operand is NaN, the comparison is false → rd = 0
 *     - +0.0 >= -0.0 and -0.0 >= +0.0 are both true
 */
X(OP_FLOAT_GE, FORMAT_RRR, "float.ge")

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
X(OP_FLOAT_NEG, FORMAT_RxRx, "float.neg")

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
X(OP_LAND, FORMAT_RRR, "land")

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
X(OP_LOR, FORMAT_RRR, "lor")

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
X(OP_LNOT, FORMAT_RxRx, "lnot")

/*---------------------------------------------------------------+
 |  Reference Comparison Instructions                            |
 +---------------------------------------------------------------*/

/**
 * OP_REF_EQ — compare two references for equality
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Compares rs and rt for identity equality.
 *     Writes true to rd if equal, false otherwise.
 */
X(OP_REF_EQ, FORMAT_RRR, "ref.eq")

/**
 * OP_REF_NE — compare two references for inequality
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Details:
 *     Compares rs and rt for identity inequality.
 *     Writes true to rd if not equal, false otherwise.
 */
X(OP_REF_NE, FORMAT_RRR, "ref.ne")

/**
 * OP_REF_EQ_NULL — check if reference is null
 *
 * FORMAT_RxRx:
 *     | op:8 | rd:12 | rs:12 |
 *
 * Details:
 *     Writes true to rd if rs is null, false otherwise.
 */
X(OP_REF_EQ_NULL, FORMAT_RxRx, "ref.eq_null")

/**
 * OP_REF_NE_NULL — check if reference is not null
 *
 * FORMAT_RxRx:
 *     | op:8 | rd:12 | rs:12 |
 *
 * Details:
 *     Writes true to rd if rs is not null, false otherwise.
 */
X(OP_REF_NE_NULL, FORMAT_RxRx, "ref.ne_null")

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
X(OP_JMP, FORMAT_JMP, "jmp")

/**
 * OP_JMP_TRUE — conditional jump if true
 *
 * FORMAT_ROff2:
 *     | op:8 | rd:8 | offset:16 |
 *
 * Details:
 *     If rd is true (non-zero), pc += offset.
 */
X(OP_JMP_TRUE, FORMAT_ROff2, "jmp_true")

/**
 * OP_JMP_FALSE — conditional jump if false
 *
 * FORMAT_ROff2:
 *     | op:8 | rd:8 | offset:16 |
 *
 * Details:
 *     If rd is false (zero), pc += offset.
 */
X(OP_JMP_FALSE, FORMAT_ROff2, "jmp_false")

/**
 * OP_JMP_REF_EQ — conditional jump if rs == rt
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If rs == rt, pc += offset.
 */
X(OP_JMP_REF_EQ, FORMAT_RROff, "jmp_ref_eq")

/**
 * OP_JMP_REF_NE — conditional jump if rs != rt
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If rs != rt, pc += offset.
 */
X(OP_JMP_REF_NE, FORMAT_RROff, "jmp_ref_ne")

/**
 * OP_JMP_REF_EQ_NULL — conditional jump if null
 *
 * FORMAT_ROff2:
 *     | op:8 | rs:8 | offset:16 |
 *
 * Details:
 *     If rs is null, pc += offset.
 */
X(OP_JMP_REF_EQ_NULL, FORMAT_ROff2, "jmp_ref_eq_null")

/**
 * OP_JMP_REF_NE_NULL — conditional jump if not null
 *
 * FORMAT_ROff2:
 *     | op:8 | rs:8 | offset:16 |
 *
 * Details:
 *     If rs is not null, pc += offset.
 */
X(OP_JMP_REF_NE_NULL, FORMAT_ROff2, "jmp_ref_ne_null")

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
X(OP_JMP_INT_EQ, FORMAT_RROff, "jmp_int_eq")

/**
 * OP_JMP_INT_EQ_IMM — jump if equal (immediate)
 *
 * FORMAT_RImmOff:
 *     | op:8 | rs:8 | imm:8 | offset:8 |
 *
 * Details:
 *     If rs == imm, pc += offset.
 */
X(OP_JMP_INT_EQ_IMM, FORMAT_RImmOff, "jmp_int_eq_imm")

/**
 * OP_JMP_INT_NE — jump if not equal
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If rs != rt, pc += offset.
 */
X(OP_JMP_INT_NE, FORMAT_RROff, "jmp_int_ne")

/**
 * OP_JMP_INT_NE_IMM — jump if not equal (immediate)
 *
 * FORMAT_RImmOff:
 *     | op:8 | rs:8 | imm:8 | offset:8 |
 *
 * Details:
 *     If rs != imm, pc += offset.
 */
X(OP_JMP_INT_NE_IMM, FORMAT_RImmOff, "jmp_int_ne_imm")

/**
 * OP_JMP_INT_LT — jump if less-than
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If rs < rt, pc += offset.
 */
X(OP_JMP_INT_LT, FORMAT_RROff, "jmp_int_lt")

/**
 * OP_JMP_INT_LT_IMM — jump if less-than (immediate)
 *
 * FORMAT_RImmOff:
 *     | op:8 | rs:8 | imm:8 | offset:8 |
 *
 * Details:
 *     If rs < imm, pc += offset.
 */
X(OP_JMP_INT_LT_IMM, FORMAT_RImmOff, "jmp_int_lt_imm")

/**
 * OP_JMP_INT_LE — jump if less-or-equal
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If rs <= rt, pc += offset.
 */
X(OP_JMP_INT_LE, FORMAT_RROff, "jmp_int_le")

/**
 * OP_JMP_INT_LE_IMM — jump if less-or-equal (immediate)
 *
 * FORMAT_RImmOff:
 *     | op:8 | rs:8 | imm:8 | offset:8 |
 *
 * Details:
 *     If rs <= imm, pc += offset.
 */
X(OP_JMP_INT_LE_IMM, FORMAT_RImmOff, "jmp_int_le_imm")

/**
 * OP_JMP_INT_GT — jump if greater-than
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If rs > rt, pc += offset.
 */
X(OP_JMP_INT_GT, FORMAT_RROff, "jmp_int_gt")

/**
 * OP_JMP_INT_GT_IMM — jump if greater-than (immediate)
 *
 * FORMAT_RImmOff:
 *     | op:8 | rs:8 | imm:8 | offset:8 |
 *
 * Details:
 *     If rs > imm, pc += offset.
 */
X(OP_JMP_INT_GT_IMM, FORMAT_RImmOff, "jmp_int_gt_imm")

/**
 * OP_JMP_INT_GE — jump if greater-or-equal
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If rs >= rt, pc += offset.
 */
X(OP_JMP_INT_GE, FORMAT_RROff, "jmp_int_ge")

/**
 * OP_JMP_INT_GE_IMM — jump if greater-or-equal (immediate)
 *
 * FORMAT_RImmOff:
 *     | op:8 | rs:8 | imm:8 | offset:8 |
 *
 * Details:
 *     If rs >= imm, pc += offset.
 */
X(OP_JMP_INT_GE_IMM, FORMAT_RImmOff, "jmp_int_ge_imm")

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
X(OP_JMP_UINT_LT, FORMAT_RROff, "jmp_uint_lt")

/**
 * OP_JMP_UINT_LT_IMM — jump if unsigned less-than (immediate)
 *
 * FORMAT_RImmOff:
 *     | op:8 | rs:8 | imm:8 | offset:8 |
 *
 * Details:
 *     If rs < imm (unsigned), pc += offset.
 */
X(OP_JMP_UINT_LT_IMM, FORMAT_RImmOff, "jmp_uint_lt_imm")

/**
 * OP_JMP_UINT_LE — jump if unsigned less-or-equal
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If rs <= rt (unsigned), pc += offset.
 */
X(OP_JMP_UINT_LE, FORMAT_RROff, "jmp_uint_le")

/**
 * OP_JMP_UINT_LE_IMM — jump if unsigned less-or-equal (immediate)
 *
 * FORMAT_RImmOff:
 *     | op:8 | rs:8 | imm:8 | offset:8 |
 *
 * Details:
 *     If rs <= imm (unsigned), pc += offset.
 */
X(OP_JMP_UINT_LE_IMM, FORMAT_RImmOff, "jmp_uint_le_imm")

/**
 * OP_JMP_UINT_GT — jump if unsigned greater-than
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If rs > rt (unsigned), pc += offset.
 */
X(OP_JMP_UINT_GT, FORMAT_RROff, "jmp_uint_gt")

/**
 * OP_JMP_UINT_GT_IMM — jump if unsigned greater-than (immediate)
 *
 * FORMAT_RImmOff:
 *     | op:8 | rs:8 | imm:8 | offset:8 |
 *
 * Details:
 *     If rs > imm (unsigned), pc += offset.
 */
X(OP_JMP_UINT_GT_IMM, FORMAT_RImmOff, "jmp_uint_gt_imm")

/**
 * OP_JMP_UINT_GE — jump if unsigned greater-or-equal
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If rs >= rt (unsigned), pc += offset.
 */
X(OP_JMP_UINT_GE, FORMAT_RROff, "jmp_uint_ge")

/**
 * OP_JMP_UINT_GE_IMM — jump if unsigned greater-or-equal (immediate)
 *
 * FORMAT_RImmOff:
 *     | op:8 | rs:8 | imm:8 | offset:8 |
 *
 * Details:
 *     If rs >= imm (unsigned), pc += offset.
 */
X(OP_JMP_UINT_GE_IMM, FORMAT_RImmOff, "jmp_uint_ge_imm")

/*---------------------------------------------------------------+
 |  Fused Float Compare + Jump Instructions                      |
 +---------------------------------------------------------------*/

/**
 * OP_JMP_FLOAT_EQ — jump if floating-point equal
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If (rs == rt) in floating‑point comparison, pc += offset.
 *
 * Notes:
 *     - If either operand is NaN, comparison is false → no jump
 *     - +0.0 and -0.0 are considered equal
 */
X(OP_JMP_FLOAT_EQ, FORMAT_RROff, "jmp_float_eq")

/**
 * OP_JMP_FLOAT_NE — jump if floating-point not equal
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If (rs != rt) in floating‑point comparison, pc += offset.
 *
 * Notes:
 *     - If either operand is NaN, comparison is true → jump
 *     - +0.0 and -0.0 are considered equal (so NE = false)
 */
X(OP_JMP_FLOAT_NE, FORMAT_RROff, "jmp_float_ne")

/**
 * OP_JMP_FLOAT_LT — jump if floating-point less-than
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If (rs < rt), pc += offset.
 *
 * Notes:
 *     - If either operand is NaN, comparison is false → no jump
 */
X(OP_JMP_FLOAT_LT, FORMAT_RROff, "jmp_float_lt")

/**
 * OP_JMP_FLOAT_LE — jump if floating-point less-or-equal
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If (rs <= rt), pc += offset.
 *
 * Notes:
 *     - If either operand is NaN, comparison is false → no jump
 *     - +0.0 <= -0.0 and -0.0 <= +0.0 are both true
 */
X(OP_JMP_FLOAT_LE, FORMAT_RROff, "jmp_float_le")

/**
 * OP_JMP_FLOAT_GT — jump if floating-point greater-than
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If (rs > rt), pc += offset.
 *
 * Notes:
 *     - If either operand is NaN, comparison is false → no jump
 */
X(OP_JMP_FLOAT_GT, FORMAT_RROff, "jmp_float_gt")

/**
 * OP_JMP_FLOAT_GE — jump if floating-point greater-or-equal
 *
 * FORMAT_RROff:
 *     | op:8 | rs:8 | rt:8 | offset:8 |
 *
 * Details:
 *     If (rs >= rt), pc += offset.
 *
 * Notes:
 *     - If either operand is NaN, comparison is false → no jump
 *     - +0.0 >= -0.0 and -0.0 >= +0.0 are both true
 */
X(OP_JMP_FLOAT_GE, FORMAT_RROff, "jmp_float_ge")

/*---------------------------------------------------------------+
 |  Unified Call Instruction                                     |
 +---------------------------------------------------------------*/

/**
 * OP_CALL — unified call instruction family
 *
 * FORMAT_CALL:
 *     | op:8 | flag:4 | A(ret-reg):12 | B(nargs):8 |
 *     | payload (32-bit)                           |
 *
 * Details:
 *     A unified call instruction format. The 'flag' field determines
 *     the call subtype:
 *
 *         flag = 0  →  direct call within the same module
 *         flag = 1  →  external function call (import-index)
 *         flag = 2  →  interface method call (the same module)
 *         flag = 3  →  interface method call (external, import-index)
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
X(OP_CALL, FORMAT_CALL, "call")

/**
 * OP_TAIL_CALL — tail-call self (only call the current function)
 *
 * FORMAT_CALL:
 *     | op:8 | flag:4 | A(ret-reg):12 | B(nargs):8 |
 *
 * Encoding rules:
 *     - flag must be 0
 *     - A(ret-reg) must be 0xFFF (no return value)
 *     - B(nargs) is optional debug info; VM does not use it
 *
 * Details:
 *     OP_TAIL_CALL performs a tail-recursive call to the *current* function.
 *     It cannot be used for:
 *         - external calls
 *         - interface method calls
 *         - calling other functions in the same module
 *
 *     No payload word follows this instruction.
 *     The instruction is a single 32-bit word.
 *
 * VM behavior:
 *     - reuses the current stack frame (no new frame is created)
 *     - does not return to the caller
 *     - jumps to the beginning of the current function body
 *     - acts as a terminator instruction (no fallthrough)
 *
 * Summary:
 *     OP_TAIL_CALL is a compact, single-word terminator instruction
 *     dedicated to self tail recursion elimination.
 */
X(OP_TAIL_CALL, FORMAT_CALL, "tail_call")

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
X(OP_RET, FORMAT_Rx, "ret")

/**
 * OP_RET_INT_IMM — return typed integer immediate (16-bit immediate)
 *
 * FORMAT_TI_Imm2:
 *     | op:8 | ---:4 | ti:4 | imm:16 |
 *
 * Fields:
 *     ti: the same as OP_LOAD_INT_IMM
 *
 * Details:
 *     Returns a 16-bit signed integer immediate to the caller.
 */
X(OP_RET_INT_IMM, FORMAT_TI_Imm2, "ret_int_imm")

/**
 * OP_RET_UINT_IMM — return typed unsigned integer immediate (16-bit immediate)
 *
 * FORMAT_TI_Imm2:
 *     | op:8 | ---:4 | ti:4 | imm:16 |
 *
 * Fields:
 *     ti: the same as OP_LOAD_INT_IMM
 *
 * Details:
 *     Returns a 16-bit unsigned integer immediate to the caller.
 */
X(OP_RET_UINT_IMM, FORMAT_TI_Imm2, "ret_uint_imm")

/**
 * OP_LOAD_TAG — return small tagged constant
 *
 * FORMAT_Tag:
 *     | op:8 | ---:16 | imm:8 |
 *
 * Details:
 *     Returns a small tagged constant into rd. The imm field encodes:
 *         - boolean values(0, 1)
 *         - none / null(2)
 *         - special float values (e.g., +0.0(3), -0.0(4), NaN(5), -inf(6), +inf(7))
 *     This avoids constant-pool lookup for common values.
 */
X(OP_RET_TAG, FORMAT_Tag, "ret_tag")

/**
 * OP_RET_CONST — return constant pool entry
 *
 * FORMAT_Idx2:
 *     | op:8 | ---:8 | index:16 |
 *
 * Details:
 *     Returns CP[idx] to the caller.
 */
X(OP_RET_CONST, FORMAT_Idx2, "ret_const")

/**
 * OP_RET_VOID — return void
 *
 * FORMAT_Op:
 *     | op:8 | ------------------------:24 |
 *
 * Details:
 *     Returns void (no value) to the caller.
 */
X(OP_RET_VOID, FORMAT_Op, "ret_void")

/*---------------------------------------------------------------+
 |  Integer/Float cast Instructions                              |
 +---------------------------------------------------------------*/

/**
 * OP_INT_CAST — integer cast with different mode on overflow
 *
 * FORMAT_RR_TI_MODE:
 *     | op:8 | rd:8 | rs:8 | dst_ti:6 | mode:2 |
 *
 * dst_ti:
 *      0b001000 = i8
 *      0b001001 = i16
 *      0b001010 = i32
 *      0b001011 = i64
 *      0b001100 = u8
 *      0b001101 = u16
 *      0b001110 = u32
 *      0b001111 = u64
 *
 * mode:
 *      0 = trap on overflow
 *      1 = wrap on overflow
 *      2 = saturate (future)
 *      3 = reserved
 */
X(OP_INT_CAST, FORMAT_RR_TI_MODE, "int.cast")

/**
 * OP_FLOAT_CAST — float cast with overflow mode
 *
 * FORMAT_RR_TI_MODE:
 *     | op:8 | rd:8 | rs:8 | dst_ti:6 | mode:2 |
 *
 * dst_ti:
 *     0b010001 = f16
 *     0b010010 = f32
 *     0b010011 = f64
 *     ... reserved
 *
 * mode:
 *     0 = trap
 *     1 = ieee (default IEEE754 behavior)
 *     2 = saturate (future)
 *     3 = reserved
 */
X(OP_FLOAT_CAST, FORMAT_RR_TI_MODE, "float.cast")

/**
 * OP_FLOAT_TO_INT — convert float to int
 *
 * FORMAT_RR_TI_MODE:
 *     | op:8 | rd:8 | rs:8 | dst_ti:6 | mode:2 |
 *
 * dst_ti:
 *      0b001000 = i8
 *      0b001001 = i16
 *      0b001010 = i32
 *      0b001011 = i64
 *      0b001100 = u8
 *      0b001101 = u16
 *      0b001110 = u32
 *      0b001111 = u64
 *
 * mode:
 *      0 = trap on overflow
 *      1 = wrap on overflow
 *      2 = saturate (future)
 *      3 = reserved
 */
X(OP_FLOAT_TO_INT, FORMAT_RR_TI_MODE, "float_to_int")

/**
 * OP_INT_TO_FLOAT — convert int to float
 *
 * FORMAT_RR_TI_MODE:
 *     | op:8 | rd:8 | rs:8 | dst_ti:6 | mode:2 |
 *
 * dst_ti:
 *     0b010001 = f16
 *     0b010010 = f32
 *     0b010011 = f64
 *     ... reserved
 *
 * mode:
 *     0 = trap
 *     1 = ieee (default IEEE754 behavior)
 *     2 = saturate (future)
 *     3 = reserved
 */
X(OP_INT_TO_FLOAT, FORMAT_RR_TI_MODE, "int_to_float")

/*---------------------------------------------------------------+
 |  New object Instructions                                      |
 +---------------------------------------------------------------*/

/**
 * OP_NEW — allocate object of a local type (same module)
 *
 * FORMAT_NEW_LOCAL:
 *     | op:8 | dst:8 | type_index(local):16 |
 *
 * Details:
 *     Allocates an object whose type metadata is defined inside
 *     the same module. The 16‑bit type_index refers to the module’s
 *     local type‑metadata array (constructed at load time, no patch).
 *
 *     R[dst] = alloc(local_type[type_index]).
 */
X(OP_NEW, FORMAT_RIdx2, "new")

/**
 * OP_NEW_EXT — allocate object of an external type
 *
 * FORMAT_NEW_EXT:
 *     | op:8 | dst:8 | import_index:16 |
 *
 * Details:
 *     Allocates an object whose type metadata originates from
 *     another module. The 16‑bit import_index refers to an entry
 *     in the module’s import table.
 *
 *     Loader resolves the external type and fills the import entry
 *     with the final TypeObject*.
 *
 *     R[dst] = alloc(import_table[import_index].type).
 */
X(OP_NEW_EXT, FORMAT_RIdx2, "new_ext")

/**
 * OP_BUILD_INTERN — Build builtin object
 *
 * FORMAT_RTagImm:
 *     | op:8 | rd:8 | tag:8 | nargs:8 |
 *
 */
X(OP_BUILD_INTERN, FORMAT_RTagImm, "build_intern")

/*---------------------------------------------------------------+
 |  Global Variable Access Instructions                           |
 +---------------------------------------------------------------*/

/**
 * OP_GLOBAL_GET — load global variable
 *
 * FORMAT_AxBx:
 *     | op:8 | dst:12 | global-index:12 |
 *
 * Details:
 *     Loads the value of a global variable into register dst.
 *     The global-index refers to the module's global table.
 *     Globals are resolved at module load time and stored in
 *     Module.globals[].
 */
X(OP_GLOBAL_GET, FORMAT_RxIdx12, "global.get")

/**
 * OP_GLOBAL_SET — store global variable
 *
 * FORMAT_AxBx:
 *     | op:8 | src:12 | global-index:12 |
 *
 * Details:
 *     Stores the value in register src into a global variable.
 *     The global-index refers to the module's global table.
 *     Writes may trigger GC barriers depending on the value type.
 */
X(OP_GLOBAL_SET, FORMAT_RxIdx12, "global.set")


/**
 * OP_GLOBAL_GET_EXT — load global variable
 *
 * FORMAT_AxBx:
 *     | op:8 | dst:8 | imported-index:16 |
 *
 * Details:
 *     Loads the value of a external global variable into register dst.
 *     The imported-index refers to the imported-table.
 */
X(OP_GLOBAL_GET_EXT, FORMAT_RIdx2, "global.get_ext")

/**
 * OP_GLOBAL_SET_EXT — store global variable
 *
 * FORMAT_AxBx:
 *     | op:8 | src:8 | imported-index:16 |
 *
 * Details:
 *     Stores the value in register src into a global variable.
 *     The imported-index refers to the imported-table.
 *     Writes may trigger GC barriers depending on the value type.
 */
X(OP_GLOBAL_SET_EXT, FORMAT_RIdx2, "global.set_ext")

/*---------------------------------------------------------------+
 |  Field Access Instructions                                    |
 +---------------------------------------------------------------*/

/**
 * OP_GET_FIELD — load field from object (same module)
 *
 * FORMAT_ABC:
 *     | op:8 | dst:8 | src:8 | imm(field-offset):8 |
 *
 * Details:
 *     Loads a field from an object using a compile-time constant
 *     field offset. Used only for fields defined in the same module.
 *     R[dst] = *(R[src] + field-offset).
 */
X(OP_GET_FIELD, FORMAT_RRImm, "get_field")

/**
 * OP_SET_FIELD — store field into object (same module)
 *
 * FORMAT_ABC:
 *     | op:8 | dst:8 | src:8 | imm(field-offset):8 |
 *
 * Details:
 *     Stores a value into a field using a compile-time constant
 *     field offset. Used only for fields defined in the same module.
 *     Writes may trigger GC barriers depending on the value type.
 *     *(R[dst] + field-offset) = R[src].
 */
X(OP_SET_FIELD, FORMAT_RRImm, "set_field")

/**
 * OP_GET_FIELD_EXT — load field from external class
 *
 * FORMAT_ABC:
 *     | op:8 | dst:8 | src:8 | imm(import-index):8 |
 *
 * Details:
 *     Loads a field defined in another module. The import-index
 *     refers to an ImportEntry of kind IMPORT_FIELD. The loader
 *     resolves the field offset and fills ImportEntry.resolved.field_offset.
 */
X(OP_GET_FIELD_EXT, FORMAT_RRImm, "get_field_ext")

/**
 * OP_SET_FIELD_EXT — store field into external class
 *
 * FORMAT_ABC:
 *     | op:8 | dst:8 | src:8 | imm(import-index):8 |
 *
 * Details:
 *     Stores a value into a field defined in another module. The
 *     import-index refers to an ImportEntry of kind IMPORT_FIELD.
 *     The loader resolves the field offset and fills
 *     ImportEntry.resolved.field_offset.
 */
X(OP_SET_FIELD_EXT, FORMAT_RRImm, "set_field_ext")

/*---------------------------------------------------------------+
 |  Conditional Move Instruction                                 |
 +---------------------------------------------------------------*/

 /**
 * OP_MOVE_TRUE — conditional move
 *
 * FORMAT_RRR:
 *     | op:8 | dst:8 | cond:8 | src:8 |
 *
 * Details:
 *     If R[cond] is non-zero, R[dst] = R[src].
 *     Otherwise, R[dst] is unchanged.
 *
 * Semantics:
 *     if (R[cond] != 0)
 *         R[dst] = R[src];
 */
X(OP_MOVE_TRUE, FORMAT_RRR, "move_true")

/*---------------------------------------------------------------+
 |  Type Casting & Interface Casting Instructions                |
 +---------------------------------------------------------------*/

/**
 * OP_MAKE_INTF — cast object to interface
 *
 * FORMAT_RRImm:
 *     | op:8 | dst:8 | src:8 | intf-index: 8 |
 *
 * Details:
 *     Attempts to cast object 'obj' to the interface identified by
 *     intf-table-index. No error happens.
 *
 *     R[dst] receives a new interface object constructed from the
 *     impl-entry of 'obj' that implements the target interface.
 */
X(OP_MAKE_INTF, FORMAT_RRImm, "make_intf")

/**
 * OP_UPCAST_INTF — upcast interface to a parent interface
 *
 * FORMAT_RRImm:
 *     | op:8 | dst:8 | src:8 | parent-index:8 |
 *
 * Details:
 *     Produces a new interface in 'dst' by selecting the parent
 *     interface implementation entry from:
 *
 *         src.impl_entry.parents[parent-index]
 *
 *     This is a purely structural upcast inside the impl-entry tree.
 *     No error happens.
 *
 * Notes:
 *    - parent-index is an 8-bit offset into impl-entry.parents[].
 */
X(OP_UPCAST_INTF, FORMAT_RRImm, "upcast_intf")

/**
 * OP_DOWNCAST_INTF — cast interface to a concrete class or another interface
 *
 * FORMAT_Op:
 *     | op:8 | dst:12 | src:12 |
 *     |  type-index:32         |
 *
 * Details:
 *     Attempts to cast the interface-object 'src' to the target type
 *     identified by type-index (index into IRModule.type_table).
 *
 *     If the target is a class:
 *         - succeeds only if src.impl_entry.klass == target-class
 *         - dst receives the underlying class-object
 *
 *     If the target is a trait:
 *         - succeeds only if the underlying class implements the trait
 *         - dst receives a new interface constructed from the
 *           corresponding impl-entry
 *
 *     Otherwise, the operation traps.
 *
 * Notes:
 *     - CHECKED OP (may trap).
 *     - type-index is a global index (16/32 bits recommended).
 */
X(OP_DOWNCAST_INTF, FORMAT_Op, "downcast_intf")

/*---------------------------------------------------------------+
 |  Sequence Protocol Instructions                               |
 +---------------------------------------------------------------*/

/**
 * OP_SEQ_GET — load an element from a sequence
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Semantics:
 *     rd = rs[rt]
 *
 * Description:
 *     Loads the element at index stored in register `rt` from the
 *     sequence object in register `rs`.
 *
 * Behavior:
 *     - `rt` must contain an integer index
 *     - Bounds checking is performed at runtime
 *     - The loaded TValue is written into `rd`
 *     - No write barrier is required (read-only)
 */
X(OP_SEQ_GET, FORMAT_RRR, "seq.get")

/**
 * OP_SEQ_SET — store an element into a sequence
 *
 * FORMAT_RRR:
 *     | op:8 | rs:8 | rv:8 | rt:8 |
 *
 * Semantics:
 *     rs[rt] = rv
 *
 * Description:
 *     Stores the value in register `rv` into the sequence object in
 *     register `rs` at index stored in register `rt`.
 *
 * Sequence objects:
 *     - Koala classes (objects with layout: sizeof(Base) + TValue[])
 *     - Native types that declare themselves as sequences and use a
 *       contiguous TValue[] layout.
 *
 * Behavior:
 *     - `rt` must contain an integer index
 *     - Bounds checking is performed at runtime
 *     - A write barrier is applied when storing into the sequence object
 *
 * Notes:
 *     - This opcode is the fast-path for `obj[index] = value` when the
 *       compiler determines that `obj` is a sequence.
 */
X(OP_SEQ_SET, FORMAT_RRR, "seq.set")

/**
 * OP_SEQ_GET_IMM — load an element using an immediate index
 *
 * FORMAT_RRImm:
 *     | op:8 | rd:8 | rs:8 | imm:8 |
 *
 * Semantics:
 *     rd = rs[imm]
 *
 * Description:
 *     Loads the element at constant index `imm` from the sequence object
 *     in register `rs`.
 *
 * Behavior:
 *     - `rs` must be a sequence object
 *     - `imm` is an signed 8-bit immediate index
 *     - Bounds checking is performed at runtime
 *     - The loaded TValue is written into `rd`
 *     - No write barrier is required (read-only)
 */
X(OP_SEQ_GET_IMM, FORMAT_RRImm, "seq.get_imm")

/**
 * OP_SEQ_SET_IMM — store an element using an immediate index
 *
 * FORMAT_RRImm:
 *     | op:8 | rs:8 | rv:8 | imm:8 |
 *
 * Semantics:
 *     rs[imm] = rv
 *
 * Description:
 *     Stores the value in register `rv` into the sequence object in
 *     register `rs` at constant index `imm`.
 *
 * Behavior:
 *     - `rs` must be a sequence object
 *     - `imm` is an signed 8-bit immediate index
 *     - Bounds checking is performed at runtime
 *     - A write barrier is applied when storing into the sequence object
 */
X(OP_SEQ_SET_IMM, FORMAT_RRImm, "seq.set_imm")

/**
 * OP_SEQ_GET_SLICE — load an object using a slice index
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rt:8 |
 *
 * Semantics:
 *     rd = rs[rt]   // rt is a slice object
 *
 * Description:
 *     Loads an object from the container in register `rs` using the
 *     slice descriptor in register `rt` as the index. The slice object
 *     describes (start, end, step) and is interpreted according to the
 *     container's indexing semantics.
 *
 * Behavior:
 *     - rt is a slice object
 *     - The loaded TValue is written into rd
 */
X(OP_SEQ_GET_SLICE, FORMAT_RRR, "seq.get_slice")

/**
 * OP_SEQ_SET_SLICE — store into a slice using a slice index
 *
 * FORMAT_RRR:
 *     | op:8 | rs:8 | rt:8 | rv:8 |
 *
 * Semantics:
 *     rs[rt] = rv   // rt is a slice object
 *
 * Description:
 *     Stores elements from register `rv` into the slice of the container
 *     in register `rs`, where the slice descriptor is in register `rt`.
 *
 * Behavior:
 *     - rt is a slice object
 *     - rv is an iterable of compatible element type
 */
X(OP_SEQ_SET_SLICE, FORMAT_RRR, "seq.set_slice")

/*---------------------------------------------------------------+
 |  Map Protocol Instructions                                    |
 +---------------------------------------------------------------*/

/**
 * OP_MAP_GET — load value from a map via key lookup
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rk:8 |
 *
 * Semantics:
 *     rd = rs[rk]
 *
 * Description:
 *     Loads the value associated with key in register rk
 *     from the map in register rs.
 *
 * Behavior:
 *     - rs must be a map object
 *     - rk is a TValue key (string/int/tuple/etc.)
 *     - Performs a key lookup according to the mapping's semantics.
 *     - panic if the key is not present
 */
X(OP_MAP_GET, FORMAT_RRR, "map.get")

/**
 * OP_MAP_SET — store value into a map via key lookup
 *
 * FORMAT_RRR:
 *     | op:8 | rs:8 | rk:8 | rv:8 |
 *
 * Semantics:
 *     rs[rk] = rv
 *
 * Description:
 *     Stores the value in register rv into the map in register rs
 *     under key in register rk.
 *
 * Behavior:
 *     - rs must be a map object
 *     - rk is a TValue key
 *     - Performs hash lookup and inserts or updates the entry
 *     - Write barrier is applied when storing into map
 */
X(OP_MAP_SET, FORMAT_RRR, "map.set")

/*---------------------------------------------------------------+
 |  Container(Seq&Map) Common Instructions                       |
 +---------------------------------------------------------------*/

/**
 * OP_LEN — get the number of items in a container
 *
 * FORMAT_RxRx:
 *     | op:8 | rd:12 | rs:12 |
 *
 * Semantics:
 *     rd = len(rs)
 *
 * Description:
 *     Loads the number of items contained in the container object
 *     in register `rs` into register `rd`. This includes sequences,
 *     sets, maps, strings, bytes, and any type that implements the
 *     length protocol.
 */
X(OP_LEN, FORMAT_RxRx, "len")

/**
 * OP_CONTAINS — membership test for container objects
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | rs:8 | rv:8 |
 *
 * Semantics:
 *     rd = (rv in rs)
 *
 * Description:
 *     Evaluates whether the value in register `rv` is contained in
 *     the container object in register `rs`. The result (true or
 *     false) is written into register `rd`.
 *
 * Notes:
 *     - Lowered from IR `__contains__`
 *     - ISEL selects the appropriate implementation based on type:
 *       sequence, set, or map.
 */
X(OP_CONTAINS, FORMAT_RRR, "contains")

/*---------------------------------------------------------------+
 |  List related Instructions                                    |
 +---------------------------------------------------------------*/

/**
 * OP_LIST_PUSH — append a value to the end of a list
 *
 * FORMAT_RxRx:
 *     | op:8 | rs:12 | rv:12 |
 *
 * Semantics:
 *     append rv to list rs
 *
 * Description:
 *     Appends the value in register rv to the list in register rs.
 *     Performs capacity check and grows the list if necessary.
 *     Applies write barrier when storing into the list.
 *
 * Notes:
 *     - This is a high-frequency operation and must be a VM opcode.
 *     - IRGen emits this opcode for list.append(x) and list.push(x).
 *     - ISEL lowers this opcode directly without specialization.
 */
X(OP_LIST_PUSH, FORMAT_RxRx, "list.push")

/**
 * OP_LIST_POP — pop the last element from a list
 *
 * FORMAT_RxRx:
 *     | op:8 | rd:12 | rs:12 |
 *
 * Semantics:
 *     rd = list_pop(rs)
 *
 * Description:
 *     Removes and returns the last element of the list in register rs.
 *     This is an O(1) operation.
 *
 * Behavior:
 *     - rs must be a list object
 *     - If the list is empty, raises an IndexError
 *     - No shifting or reordering of elements is performed
 *
 * Notes:
 *     - This is a high-frequency operation and must be a VM opcode.
 *     - IRGen emits OP_CALL for list.pop().
 *     - ISEL lowers OP_CALL "__pop__" to OP_LIST_POP when rs is a list.
 */
X(OP_LIST_POP, FORMAT_RxRx, "list.pop")

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
X(OP_GET_ITER, FORMAT_Op, "get_iter")

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
X(OP_ITER_NEXT, FORMAT_Op, "iter.next")

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
X(OP_AS, FORMAT_Op, "as")

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
X(OP_IS, FORMAT_Op, "is")

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
X(OP_WIDE, FORMAT_WIDE, "wide")

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
X(OP_RAISE, FORMAT_Op, "raise")

/*---------------------------------------------------------------+
 |  Number Protocol Instructions                                 |
 +---------------------------------------------------------------*/

/**
 * OP_NUM_ADD — numeric addition via number protocol
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | ra:8 | rb:8 |
 *
 * Semantics:
 *     rd = a + b
 *
 * Description:
 *     Computes the numeric sum of ra and rb using the number protocol.
 *
 * Behavior:
 *     - Operands must implement __add__
 *
 * Types:
 *     - User-defined numeric types implementing __add__
 *
 * Notes:
 *     - Protocol-level addition; built-in int/float use dedicated opcodes
 */
X(OP_NUM_ADD, FORMAT_RRR, "num.add")

/**
 * OP_NUM_SUB — numeric subtraction via number protocol
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | ra:8 | rb:8 |
 *
 * Semantics:
 *     rd = a - b
 *
 * Description:
 *     Computes the numeric difference of ra and rb using the number protocol.
 *
 * Behavior:
 *     - Operands must implement __sub__
 *
 * Types:
 *     - User-defined numeric types implementing __sub__
 *
 * Notes:
 *     - Protocol-level subtraction; built-in int/float use dedicated opcodes
 */
X(OP_NUM_SUB, FORMAT_RRR, "num.sub")

/**
 * OP_NUM_MUL — numeric multiplication via number protocol
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | ra:8 | rb:8 |
 *
 * Semantics:
 *     rd = a * b
 *
 * Description:
 *     Computes the numeric product of ra and rb using the number protocol.
 *
 * Behavior:
 *     - Operands must implement __mul__
 *
 * Types:
 *     - User-defined numeric types implementing __mul__
 *
 * Notes:
 *     - Protocol-level multiplication; built-in int/float use dedicated opcodes
 */
X(OP_NUM_MUL, FORMAT_RRR, "num.mul")

/**
 * OP_NUM_DIV — numeric division via number protocol
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | ra:8 | rb:8 |
 *
 * Semantics:
 *     rd = a / b
 *
 * Description:
 *     Computes the numeric quotient of ra divided by rb using the number protocol.
 *
 * Behavior:
 *     - Division by zero is invalid
 *     - Operands must implement __div__
 *
 * Types:
 *     - User-defined numeric types implementing __div__
 *
 * Notes:
 *     - Protocol-level division; built-in int/float use dedicated opcodes
 */
X(OP_NUM_DIV, FORMAT_RRR, "num.div")

/**
 * OP_NUM_MOD — numeric modulo via number protocol
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | ra:8 | rb:8 |
 *
 * Semantics:
 *     rd = a % b
 *
 * Description:
 *     Computes the remainder of ra divided by rb using the number protocol.
 *
 * Behavior:
 *     - Modulo by zero is invalid
 *     - Operands must implement __mod__
 *
 * Types:
 *     - User-defined numeric types implementing __mod__
 *
 * Notes:
 *     - Protocol-level modulo; built-in int/float use dedicated opcodes
 */
X(OP_NUM_MOD, FORMAT_RRR, "num.mod")

/**
 * OP_NUM_AND — bitwise AND via number protocol
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | ra:8 | rb:8 |
 *
 * Semantics:
 *     rd = a & b
 *
 * Description:
 *     Performs bitwise AND on ra and rb using the number protocol.
 *
 * Behavior:
 *     - Operands must implement __and__
 *
 * Types:
 *     - User-defined numeric types implementing __and__
 *
 * Notes:
 *     - Protocol-level bitwise AND; built-in int/float use dedicated opcodes
 */
X(OP_NUM_AND, FORMAT_RRR, "num.and")

/**
 * OP_NUM_OR — bitwise OR via number protocol
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | ra:8 | rb:8 |
 *
 * Semantics:
 *     rd = a | b
 *
 * Description:
 *     Performs bitwise OR on ra and rb using the number protocol.
 *
 * Behavior:
 *     - Operands must implement __or__
 *
 * Types:
 *     - User-defined numeric types implementing __or__
 *
 * Notes:
 *     - Protocol-level bitwise OR; built-in int/float use dedicated opcodes
 */
X(OP_NUM_OR, FORMAT_RRR, "num.or")

/**
 * OP_NUM_XOR — bitwise XOR via number protocol
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | ra:8 | rb:8 |
 *
 * Semantics:
 *     rd = a ^ b
 *
 * Description:
 *     Performs bitwise XOR on ra and rb using the number protocol.
 *
 * Behavior:
 *     - Operands must implement __xor__
 *
 * Types:
 *     - User-defined numeric types implementing __xor__
 *
 * Notes:
 *     - Protocol-level bitwise XOR; built-in int/float use dedicated opcodes
 */
X(OP_NUM_XOR, FORMAT_RRR, "num.xor")

/**
 * OP_NUM_SHL — bitwise shift-left via number protocol
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | ra:8 | rb:8 |
 *
 * Semantics:
 *     rd = a << b
 *
 * Description:
 *     Shifts ra left by b bits using the number protocol.
 *
 * Behavior:
 *     - Shift amount must be non-negative
 *     - Operands must implement __shl__
 *
 * Types:
 *     - User-defined numeric types implementing __shl__
 *
 * Notes:
 *     - Protocol-level shift-left; built-in int/float use dedicated opcodes
 */
X(OP_NUM_SHL, FORMAT_RRR, "num.shl")

/**
 * OP_NUM_SHR — bitwise shift-right via number protocol
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | ra:8 | rb:8 |
 *
 * Semantics:
 *     rd = a >> b
 *
 * Description:
 *     Shifts ra right by b bits using the number protocol.
 *
 * Behavior:
 *     - Shift amount must be non-negative
 *     - Operands must implement __shr__
 *
 * Types:
 *     - User-defined numeric types implementing __shr__
 *
 * Notes:
 *     - Protocol-level shift-right; built-in int/float use dedicated opcodes
 */
X(OP_NUM_SHR, FORMAT_RRR, "num.shr")

/*---------------------------------------------------------------+
 |  Comparable & Equatable Protocol Instructions                 |
 +---------------------------------------------------------------*/

/**
 * OP_NUM_EQ — equality comparison via Equatable protocol
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | ra:8 | rb:8 |
 *
 * Semantics:
 *     rd = (a == b)
 *
 * Description:
 *     Compares ra and rb for equality using the Equatable protocol.
 *
 * Behavior:
 *     - Operands must implement __eq__
 *
 * Types:
 *     - User-defined types implementing __eq__
 *
 * Notes:
 *     - Returns boolean
 */
X(OP_NUM_EQ, FORMAT_RRR, "num.eq")

/**
 * OP_NUM_NE — inequality comparison via Equatable protocol
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | ra:8 | rb:8 |
 *
 * Semantics:
 *     rd = (a != b)
 *
 * Description:
 *     Compares ra and rb for inequality using the Equatable protocol.
 *
 * Behavior:
 *     - Operands must implement __ne__
 *
 * Types:
 *     - User-defined types implementing __ne__
 *
 * Notes:
 *     - Returns boolean
 */
X(OP_NUM_NE, FORMAT_RRR, "num.ne")

/**
 * OP_NUM_LT — less-than comparison via Comparable protocol
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | ra:8 | rb:8 |
 *
 * Semantics:
 *     rd = (a < b)
 *
 * Description:
 *     Compares ra and rb using less-than via the Comparable protocol.
 *
 * Behavior:
 *     - Operands must implement __lt__
 *
 * Types:
 *     - User-defined types implementing __lt__
 *
 * Notes:
 *     - Returns boolean
 */
X(OP_NUM_LT, FORMAT_RRR, "num.lt")

/**
 * OP_NUM_LE — less-or-equal comparison via Comparable protocol
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | ra:8 | rb:8 |
 *
 * Semantics:
 *     rd = (a <= b)
 *
 * Description:
 *     Compares ra and rb using <= via the Comparable protocol.
 *
 * Behavior:
 *     - Operands must implement __le__
 *
 * Types:
 *     - User-defined types implementing __le__
 *
 * Notes:
 *     - Returns boolean
 */
X(OP_NUM_LE, FORMAT_RRR, "num.le")

/**
 * OP_NUM_GT — greater-than comparison via Comparable protocol
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | ra:8 | rb:8 |
 *
 * Semantics:
 *     rd = (a > b)
 *
 * Description:
 *     Compares ra and rb using > via the Comparable protocol.
 *
 * Behavior:
 *     - Operands must implement __gt__
 *
 * Types:
 *     - User-defined types implementing __gt__
 *
 * Notes:
 *     - Returns boolean
 */
X(OP_NUM_GT, FORMAT_RRR, "num.gt")

/**
 * OP_NUM_GE — greater-or-equal comparison via Comparable protocol
 *
 * FORMAT_RRR:
 *     | op:8 | rd:8 | ra:8 | rb:8 |
 *
 * Semantics:
 *     rd = (a >= b)
 *
 * Description:
 *     Compares ra and rb using >= via the Comparable protocol.
 *
 * Behavior:
 *     - Operands must implement __ge__
 *
 * Types:
 *     - User-defined types implementing __ge__
 *
 * Notes:
 *     - Returns boolean
 */
X(OP_NUM_GE, FORMAT_RRR, "num.ge")

/**
 * OP_HASH — compute hash value via Hashable protocol
 *
 * FORMAT_RR:
 *     | op:8 | rd:12 | ra:12 |
 *
 * Semantics:
 *     rd = hash(a)
 *
 * Description:
 *     Computes the hash of operand `ra` using the Hashable protocol.
 *
 * Behavior:
 *     - Operand must implement __hash__
 *
 * Types:
 *     - User-defined types implementing __hash__
 *     - Built-in types with native hashing (int, str, bytes, etc.)
 *
 * Notes:
 *     - Returns an integer hash value
 *     - Generated by the compiler for the intrinsic `hash(obj)`
 */
X(OP_HASH, FORMAT_RxRx, "hash")

/**
 * OP_STR — convert object to string via Printable protocol
 *
 * FORMAT_RR:
 *     | op:8 | rd:12 | ra:12 |
 *
 * Semantics:
 *     rd = str(a)
 *
 * Description:
 *     Converts operand `ra` to its string representation using the
 *     Printable protocol.
 *
 * Behavior:
 *     - Operand must implement __str__
 *
 * Types:
 *     - User-defined types implementing __str__
 *     - Built-in types with native string conversion (int, bytes, etc.)
 *
 * Notes:
 *     - Returns a string
 *     - Generated by the compiler for the intrinsic `str(obj)`
 */
X(OP_STR, FORMAT_RxRx, "str")

/*---------------------------------------------------------------+
 |  IR-Only Pseudo Instructions                                  |
 +---------------------------------------------------------------*/

/* Binary arithmetic (IR only) */
X(OP_BINARY_ADD,    FORMAT_IR, "add")
X(OP_BINARY_SUB,    FORMAT_IR, "sub")
X(OP_BINARY_MUL,    FORMAT_IR, "mul")
X(OP_BINARY_DIV,    FORMAT_IR, "div")
X(OP_BINARY_MOD,    FORMAT_IR, "mod")

/* Binary bitwise (IR only) */
X(OP_BINARY_AND,    FORMAT_IR, "and")
X(OP_BINARY_OR,     FORMAT_IR, "or")
X(OP_BINARY_XOR,    FORMAT_IR, "xor")

/* Binary shifts (IR only) */
X(OP_BINARY_SHL,    FORMAT_IR, "shl")
X(OP_BINARY_SHR,    FORMAT_IR, "shr")

/* Binary comparisons (IR only) */
X(OP_BINARY_EQ,  FORMAT_IR, "eq")
X(OP_BINARY_NE,  FORMAT_IR, "ne")
X(OP_BINARY_LT,  FORMAT_IR, "lt")
X(OP_BINARY_LE,  FORMAT_IR, "le")
X(OP_BINARY_GT,  FORMAT_IR, "gt")
X(OP_BINARY_GE,  FORMAT_IR, "ge")

/* Unary ops (IR only) */
X(OP_UNARY_PLUS,    FORMAT_IR, "plus")
X(OP_UNARY_NEG,     FORMAT_IR, "neg")
X(OP_UNARY_NOT,     FORMAT_IR, "not")

/* Structural IR ops */
X(OP_IR_LOCAL,      FORMAT_IR, "local")
X(OP_IR_CALL,       FORMAT_IR, "call")
X(OP_IR_SELECT,     FORMAT_IR, "select")
X(OP_IR_JMP_COND,   FORMAT_IR, "branch")
X(OP_IR_CAST,       FORMAT_IR, "cast")
X(OP_IR_PHI,        FORMAT_IR, "phi")

/* Non-executable data slot (pseudo-instruction) */
X(OP_DATA, FORMAT_DATA, "data")

// clang-format on
