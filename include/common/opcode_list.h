
// clang-format off

/* opcode_list.h — single source of truth */

X(OP_NOP,               FORMAT_Op,      "| op:8 | ------------------:24 |    ; nop")

X(OP_MOVE,              FORMAT_AxBx,    "| op:8 | Ax(rd):12 | Bx(rs):12   |    ; rd = rs")
X(OP_MOVE_FLT_S,        FORMAT_AxBx,    "| op:8 | Ax(rd):12 | Bx(kind):12 |    ; rd = float_special[kind]")
X(OP_MOVE_BOOL,         FORMAT_AxBx,    "| op:8 | Ax(rd):12 | Bx(flag):12 |    ; rd = (flag & 1) ? true : false")
X(OP_MOVE_NONE,         FORMAT_Ax,      "| op:8 | ------:12 | Ax(rd):12   |    ; rd = none")
X(OP_MOVE_INT_IMM,      FORMAT_ABxx,    "| op:8 | A(rd):8   | Bx(imm):16  |    ; rd = imm")
X(OP_LOADK,             FORMAT_ABxx,    "| op:8 | A(rd):8   | Bx(idx):16  |    ; rd = CP[idx]")

X(OP_CONST_INT,         FORMAT_ABxx,    "| op:8 | A(rd):8   | Bx(imm):16  |    ; rd = imm")
X(OP_CONST_LOAD,        FORMAT_ABxx,    "| op:8 | A(rd):8   | Bx(idx):16  |    ; rd = CP[idx]")
X(OP_CONST_FLT_S,       FORMAT_AxBx,    "| op:8 | Ax(rd):12 | Bx(kind):12 |    ; rd = float_special[kind]")
X(OP_CONST_BOOL,        FORMAT_AxBx,    "| op:8 | Ax(rd):12 | Bx(flag):12 |    ; rd = (flag & 1) ? true : false")
X(OP_CONST_NONE,        FORMAT_Ax,      "| op:8 | ------:12 | Ax(rd):12   |    ; rd = none")

X(OP_INT_ADD,           FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs + rt")
X(OP_INT_ADD_IMM,       FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = rs + imm")
X(OP_INT_SUB,           FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs - rt")
X(OP_INT_SUB_IMM,       FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = rs - imm")
X(OP_INT_MUL,           FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs * rt")
X(OP_INT_MUL_IMM,       FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = rs * imm")
X(OP_INT_DIV,           FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs / rt")
X(OP_INT_DIV_IMM,       FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = rs / imm")
X(OP_INT_MOD,           FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs % rt")
X(OP_INT_MOD_IMM,       FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = rs % imm")

X(OP_INT_AND,           FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs & rt")
X(OP_INT_AND_IMM,       FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = rs & imm")
X(OP_INT_OR,            FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs | rt")
X(OP_INT_OR_IMM,        FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = rs | imm")
X(OP_INT_XOR,           FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs ^ rt")
X(OP_INT_XOR_IMM,       FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = rs ^ imm")
X(OP_INT_SHL,           FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs << rt")
X(OP_INT_SHL_IMM,       FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = rs << imm")
X(OP_INT_SHR,           FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs >> rt")
X(OP_INT_SHR_IMM,       FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = rs >> imm")

X(OP_INT_CMPEQ,         FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs == rt")
X(OP_INT_CMPEQ_IMM,     FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = rs == imm")
X(OP_INT_CMPNE,         FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs != rt")
X(OP_INT_CMPNE_IMM,     FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = rs != imm")
X(OP_INT_CMPLT,         FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs < rt")
X(OP_INT_CMPLT_IMM,     FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = rs < imm")
X(OP_INT_CMPGT,         FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs > rt")
X(OP_INT_CMPGT_IMM,     FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = rs > imm")
X(OP_INT_CMPLE,         FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs <= rt")
X(OP_INT_CMPLE_IMM,     FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = rs <= imm")
X(OP_INT_CMPGE,         FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs >= rt")
X(OP_INT_CMPGE_IMM,     FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = rs >= imm")

X(OP_INT_NEG,           FORMAT_AxBx,    "| op:8 | Ax(rd):12 | Bx(rs):12 |    ; rd = -rs")
X(OP_INT_NOT,           FORMAT_AxBx,    "| op:8 | Ax(rd):12 | Bx(rs):12 |    ; rd = ~rs")

X(OP_UINT_DIV,          FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs / rt")
X(OP_UINT_DIV_IMM,      FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = rs / imm")
X(OP_UINT_MOD,          FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs % rt")
X(OP_UINT_MOD_IMM,      FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = rs % imm")
X(OP_UINT_SHR,          FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs >> rt")
X(OP_UINT_SHR_IMM,      FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = rs >> imm")

X(OP_UINT_CMPLT,        FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = (rs < rt) ? 1 : 0")
X(OP_UINT_CMPLT_IMM,    FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = (rs < imm) ? 1 : 0")
X(OP_UINT_CMPLE,        FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = (rs <= rt) ? 1 : 0")
X(OP_UINT_CMPLE_IMM,    FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = (rs <= imm) ? 1 : 0")
X(OP_UINT_CMPGT,        FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = (rs > rt) ? 1 : 0")
X(OP_UINT_CMPGT_IMM,    FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = (rs > imm) ? 1 : 0")
X(OP_UINT_CMPGE,        FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = (rs >= rt) ? 1 : 0")
X(OP_UINT_CMPGE_IMM,    FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(imm):8 |    ; rd = (rs >= imm) ? 1 : 0")

X(OP_FLOAT_ADD,         FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs + rt")
X(OP_FLOAT_SUB,         FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs - rt")
X(OP_FLOAT_MUL,         FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs * rt")
X(OP_FLOAT_DIV,         FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs / rt")
X(OP_FLOAT_MOD,         FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs % rt")
X(OP_FLOAT_CMPL,        FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = (rs < rt) ? -1 : ((rs == rt) ? 0 : 1)")
X(OP_FLOAT_CMPG,        FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = (rs > rt) ? 1 : ((rs == rt) ? 0 : -1)")

X(OP_FLOAT_NEG,         FORMAT_AxBx,    "| op:8 | Ax(rd):12 | Bx(rs):12 |    ; rd = -rs")

X(OP_FLT_TO_INT,        FORMAT_AxBx,    "| op:8 | Ax(rd):12 | Bx(rs):12 |    ; rd = (int)rs")
X(OP_INT_TO_FLT,        FORMAT_AxBx,    "| op:8 | Ax(rd):12 | Bx(rs):12 |    ; rd = (float)rs")

X(OP_LAND,              FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs && rt")
X(OP_LOR,               FORMAT_ABC,     "| op:8 | A(rd):8 | B(rs):8 | C(rt):8  |    ; rd = rs || rt")

X(OP_LNOT,              FORMAT_AxBx,    "| op:8 | Ax(rd):12 | Bx(rs):12 |    ; rd = !rs")

X(OP_JMP,               FORMAT_Axx,     "| op:8 | -----:8 | Axx(offset):16 |    ; pc += offset")
X(OP_JMP_TRUE,          FORMAT_ABxx,    "| op:8 | A(rd):8 | Bxx(offset):16 |    ; pc += offset if rd is true")
X(OP_JMP_FALSE,         FORMAT_ABxx,    "| op:8 | A(rd):8 | Bxx(offset):16 |    ; pc += offset if rd is false")

/* fused jmp+cmp */
X(OP_JMP_INT_EQ,        FORMAT_ABC,     "| op:8 | A(rs):8 | B(rt):8 | C(offset):8  |    ; pc += offset if rs == rt")
X(OP_JMP_INT_EQ_IMM,    FORMAT_ABC,     "| op:8 | A(rs):8 | B(imm):8 | C(offset):8 |    ; pc += offset if rs == imm")
X(OP_JMP_INT_NE,        FORMAT_ABC,     "| op:8 | A(rs):8 | B(rt):8 | C(offset):8  |    ; pc += offset if rs != rt")
X(OP_JMP_INT_NE_IMM,    FORMAT_ABC,     "| op:8 | A(rs):8 | B(imm):8 | C(offset):8 |    ; pc += offset if rs != imm")
X(OP_JMP_INT_LT,        FORMAT_ABC,     "| op:8 | A(rs):8 | B(rt):8 | C(offset):8  |    ; pc += offset if rs < rt")
X(OP_JMP_INT_LT_IMM,    FORMAT_ABC,     "| op:8 | A(rs):8 | B(imm):8 | C(offset):8 |    ; pc += offset if rs < imm")
X(OP_JMP_INT_GT,        FORMAT_ABC,     "| op:8 | A(rs):8 | B(rt):8 | C(offset):8  |    ; pc += offset if rs > rt")
X(OP_JMP_INT_GT_IMM,    FORMAT_ABC,     "| op:8 | A(rs):8 | B(imm):8 | C(offset):8 |    ; pc += offset if rs > imm")
X(OP_JMP_INT_LE,        FORMAT_ABC,     "| op:8 | A(rs):8 | B(rt):8 | C(offset):8  |    ; pc += offset if rs <= rt")
X(OP_JMP_INT_LE_IMM,    FORMAT_ABC,     "| op:8 | A(rs):8 | B(imm):8 | C(offset):8 |    ; pc += offset if rs <= imm")
X(OP_JMP_INT_GE,        FORMAT_ABC,     "| op:8 | A(rs):8 | B(rt):8 | C(offset):8  |    ; pc += offset if rs >= rt")
X(OP_JMP_INT_GE_IMM,    FORMAT_ABC,     "| op:8 | A(rs):8 | B(imm):8 | C(offset):8 |    ; pc += offset if rs >= imm")

X(OP_JMP_UINT_LT,       FORMAT_ABC,     "| op:8 | A(rs):8 | B(rt):8  | C(offset):8 |    ; pc += offset if rs < rt")
X(OP_JMP_UINT_LT_IMM,   FORMAT_ABC,     "| op:8 | A(rs):8 | B(imm):8 | C(offset):8 |    ; pc += offset if rs < imm")
X(OP_JMP_UINT_LE,       FORMAT_ABC,     "| op:8 | A(rs):8 | B(rt):8  | C(offset):8 |    ; pc += offset if rs <= rt")
X(OP_JMP_UINT_LE_IMM,   FORMAT_ABC,     "| op:8 | A(rs):8 | B(imm):8 | C(offset):8 |    ; pc += offset if rs <= imm")
X(OP_JMP_UINT_GT,       FORMAT_ABC,     "| op:8 | A(rs):8 | B(rt):8  | C(offset):8 |    ; pc += offset if rs > rt")
X(OP_JMP_UINT_GT_IMM,   FORMAT_ABC,     "| op:8 | A(rs):8 | B(imm):8 | C(offset):8 |    ; pc += offset if rs > imm")
X(OP_JMP_UINT_GE,       FORMAT_ABC,     "| op:8 | A(rs):8 | B(rt):8  | C(offset):8 |    ; pc += offset if rs >= rt")
X(OP_JMP_UINT_GE_IMM,   FORMAT_ABC,     "| op:8 | A(rs):8 | B(imm):8 | C(offset):8 |    ; pc += offset if rs >= imm")

X(OP_PUSH,              FORMAT_Ax,      "| op:8 | Ax(rs):12   | ---:12 |    ; push register")
X(OP_PUSH_INT_IMM,      FORMAT_Axx,     "| op:8 | Axx(imm):16 | ---:8  |    ; push int immediate")
X(OP_PUSH_CONST,        FORMAT_Axx,     "| op:8 | Axx(idx):16 | ---:8  |    ; push constant")
X(OP_PUSH_FLT_S,        FORMAT_Ax,      "| op:8 | Ax(kind):12 | ---:12 |    ; push float immediate")
X(OP_PUSH_BOOL,         FORMAT_Ax,      "| op:8 | Ax(flag):12 | ---:12 |    ; push boolean")
X(OP_PUSH_NONE,         FORMAT_Op,      "| op:8 | -----------------:24 |    ; push none")

/* call_direct */
X(OP_CALL,              FORMAT_ABC,     "| op:8 | A(ret):8 | B(nargs):8 | C(offset):8 |    ; directly call function")

X(OP_RET,               FORMAT_Ax,      "| op:8 | Ax(rs):12   | ---:12 |    ; return register")
X(OP_RET_INT_IMM,       FORMAT_Axx,     "| op:8 | Axx(imm):16 | ---:8  |    ; return int immediate")
X(OP_RET_FLT_S,         FORMAT_Ax,      "| op:8 | Ax(kind):12 | ---:12 |    ; return float special")
X(OP_RET_BOOL,          FORMAT_Ax,      "| op:8 | Ax(flag):12 | ---:12 |    ; return boolean")
X(OP_RET_NONE,          FORMAT_Op,      "| op:8 | -----------------:24 |    ; return none")
X(OP_RET_VOID,          FORMAT_Op,      "| op:8 | -----------------:24 |    ; return void")

X(OP_GET_GLOBAL,        FORMAT_AxBx,    "")
X(OP_SET_GLOBAL,        FORMAT_AxBx,    "")

X(OP_GET_FIELD,         FORMAT_ABC,     "")
X(OP_SET_FIELD,         FORMAT_ABC,     "")

X(OP_CAST_INTF,         FORMAT_ABC,     "")

X(OP_SUBSCR_LOAD,       FORMAT_Ax,      "")
X(OP_SUBSCR_STORE,      FORMAT_Ax,      "")

X(OP_GET_ITER,          FORMAT_Ax,      "")
X(OP_ITER_NEXT,         FORMAT_Ax,      "")

X(OP_AS,                FORMAT_Ax,      "")
X(OP_IS,                FORMAT_Ax,      "")

X(OP_WIDE,              FORMAT_ABC,     "| op:8 | A(imm):8 | B(imm):8 | C(imm):8 |    ; wide next insn")

X(OP_RAISE,             FORMAT_Op,      "| op:8 | -----------------:24 |    ; raise exception")

/* ir-only pseudo instructions */

X(OP_BINARY_ADD,        FORMAT_IR,      "; ir-only pseudo instruction")
X(OP_BINARY_SUB,        FORMAT_IR,      "; ir-only pseudo instruction")
X(OP_BINARY_MUL,        FORMAT_IR,      "; ir-only pseudo instruction")
X(OP_BINARY_DIV,        FORMAT_IR,      "; ir-only pseudo instruction")
X(OP_BINARY_MOD,        FORMAT_IR,      "; ir-only pseudo instruction")

X(OP_BINARY_AND,        FORMAT_IR,      "; ir-only pseudo instruction")
X(OP_BINARY_OR,         FORMAT_IR,      "; ir-only pseudo instruction")
X(OP_BINARY_XOR,        FORMAT_IR,      "; ir-only pseudo instruction")

X(OP_BINARY_SHL,        FORMAT_IR,      "; ir-only pseudo instruction")
X(OP_BINARY_SHR,        FORMAT_IR,      "; ir-only pseudo instruction")

X(OP_BINARY_CMPEQ,      FORMAT_IR,      "; ir-only pseudo instruction")
X(OP_BINARY_CMPNE,      FORMAT_IR,      "; ir-only pseudo instruction")
X(OP_BINARY_CMPLT,      FORMAT_IR,      "; ir-only pseudo instruction")
X(OP_BINARY_CMPGT,      FORMAT_IR,      "; ir-only pseudo instruction")
X(OP_BINARY_CMPLE,      FORMAT_IR,      "; ir-only pseudo instruction")
X(OP_BINARY_CMPGE,      FORMAT_IR,      "; ir-only pseudo instruction")

X(OP_UNARY_NEG,         FORMAT_IR,      "; ir-only pseudo instruction")
X(OP_UNARY_NOT,         FORMAT_IR,      "; ir-only pseudo instruction")

X(OP_IR_LOCAL,          FORMAT_IR,      "; ir-only pseudo instruction")
X(OP_IR_CALL,           FORMAT_IR,      "; ir-only pseudo instruction")
X(OP_IR_JMP_COND,       FORMAT_IR,      "; ir-only pseudo instruction")
X(OP_IR_PHI,            FORMAT_IR,      "; ir-only pseudo instruction")

// clang-format on
