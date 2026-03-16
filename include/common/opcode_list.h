
// clang-format off

/* opcode_list.h — single source of truth */

X(OP_NOP,                   FORMAT_Op,      "[op:8][---:24]", "nop")
X(OP_MOVE,                  FORMAT_AxBx,    "[op:8][Rd:12][Rs:12]",     "Rd = Rs")

X(OP_CONST_INT_IMM,         FORMAT_AxBx,    "[op:8][Rd:12][Imm:12]",    "Rd = Imm")
X(OP_LOADK,                 FORMAT_AxBx,    "[op:8][Rd:12][Idx:12]",    "Rd = CP[Idx]")
X(OP_LOADK_SPECIAL,         FORMAT_AxBx,    "[op:8][Rd:12][id:12]",     "Rd = special_const(id)")

X(OP_INT_ADD,               FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs + Rt")
X(OP_INT_SUB,               FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs - Rt")
X(OP_INT_MUL,               FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs * Rt")
X(OP_INT_DIV,               FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs / Rt")
X(OP_INT_MOD,               FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs % Rt")
X(OP_INT_NEG,               FORMAT_ABC,     "[op:8][Rd:8][Rs:8][--:8]", "Rd = -Rs")

X(OP_INT_AND,               FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs & Rt")
X(OP_INT_OR,                FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs | Rt")
X(OP_INT_XOR,               FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs ^ Rt")
X(OP_INT_NOT,               FORMAT_ABC,     "[op:8][Rd:8][Rs:8][--:8]", "Rd = ~Rs")
X(OP_INT_SHL,               FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs << Rt")
X(OP_INT_SHR,               FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs >> Rt")

X(OP_INT_CMP_EQ,            FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs == Rt")
X(OP_INT_CMP_NE,            FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs != Rt")
X(OP_INT_CMP_LT,            FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs < Rt")
X(OP_INT_CMP_GT,            FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs > Rt")
X(OP_INT_CMP_LE,            FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs <= Rt")
X(OP_INT_CMP_GE,            FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs >= Rt")

X(OP_INT_ADD_IMM,           FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Imm:8]", "Rd = Rs + Imm")
X(OP_INT_SUB_IMM,           FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Imm:8]", "Rd = Rs - Imm")
X(OP_INT_MUL_IMM,           FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Imm:8]", "Rd = Rs * Imm")
X(OP_INT_DIV_IMM,           FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Imm:8]", "Rd = Rs / Imm")
X(OP_INT_MOD_IMM,           FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Imm:8]", "Rd = Rs % Imm")

X(OP_INT_AND_IMM,           FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Imm:8]", "Rd = Rs & Imm")
X(OP_INT_OR_IMM,            FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Imm:8]", "Rd = Rs | Imm")
X(OP_INT_XOR_IMM,           FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Imm:8]", "Rd = Rs ^ Imm")
X(OP_INT_SHL_IMM,           FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Imm:8]", "Rd = Rs << Imm")
X(OP_INT_SHR_IMM,           FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Imm:8]", "Rd = Rs >> Imm")

X(OP_INT_CMP_EQ_IMM,        FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Imm:8]", "Rd = Rs == Imm")
X(OP_INT_CMP_NE_IMM,        FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Imm:8]", "Rd = Rs != Imm")
X(OP_INT_CMP_LT_IMM,        FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Imm:8]", "Rd = Rs < Imm")
X(OP_INT_CMP_GT_IMM,        FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Imm:8]", "Rd = Rs > Imm")
X(OP_INT_CMP_LE_IMM,        FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Imm:8]", "Rd = Rs <= Imm")
X(OP_INT_CMP_GE_IMM,        FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Imm:8]", "Rd = Rs >= Imm")

X(OP_FLOAT_ADD,             FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs + Rt")
X(OP_FLOAT_SUB,             FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs - Rt")
X(OP_FLOAT_MUL,             FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs * Rt")
X(OP_FLOAT_DIV,             FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs / Rt")
X(OP_FLOAT_MOD,             FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs % Rt")
X(OP_FLOAT_NEG,             FORMAT_ABC,     "[op:8][Rd:8][Rs:8][--:8]", "Rd = -Rs")

X(OP_FLOAT_CMPL,            FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = (Rs < Rt) ? -1 : ((Rs == Rt) ? 0 : 1)")
X(OP_FLOAT_CMPG,            FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = (Rs > Rt) ? 1 : ((Rs == Rt) ? 0 : -1)")

X(OP_UINT_CMP_LT,           FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = (Rs < Rt) ? 1 : 0")
X(OP_UINT_CMP_LE,           FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = (Rs <= Rt) ? 1 : 0")
X(OP_UINT_CMP_GT,           FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = (Rs > Rt) ? 1 : 0")
X(OP_UINT_CMP_GE,           FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = (Rs >= Rt) ? 1 : 0")

X(OP_FLOAT_TO_INT,          FORMAT_AxBx,    "[op:8][Rd:12][Rs:12]", "Rd = (int)Rs")
X(OP_INT_TO_FLOAT,          FORMAT_AxBx,    "[op:8][Rd:12][Rs:12]", "Rd = (float)Rs")

X(OP_LAND,                  FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs && Rt")
X(OP_LOR,                   FORMAT_ABC,     "[op:8][Rd:8][Rs:8][Rt:8]", "Rd = Rs || Rt")
X(OP_LNOT,                  FORMAT_AxBx,    "[op:8][Rd:12][Rs:12]",     "Rd = !Rs")

X(OP_JMP,                   FORMAT_ABxx,    "[op:8][0:8][Offset:16]", "PC += Offset")
X(OP_JMP_TRUE,              FORMAT_ABxx,    "[op:8][A:8][Offset:16]", "PC += Offset if R(A) is true")
X(OP_JMP_FALSE,             FORMAT_ABxx,    "[op:8][A:8][Offset:16]", "PC += Offset if R(A) is false")
X(OP_JMP_NONE,              FORMAT_ABxx,    "[op:8][A:8][Offset:16]", "PC += Offset if R(A) is None")
X(OP_JMP_NOT_NONE,          FORMAT_ABxx,    "[op:8][A:8][Offset:16]", "PC += Offset if R(A) is not None")

X(OP_JMP_CMP_EQ,            FORMAT_ABC,     "[op:8][A:8][B:8][Offset:8]", "PC += Offset if R(A) == R(B)")
X(OP_JMP_CMP_NE,            FORMAT_ABC,     "[op:8][A:8][B:8][Offset:8]", "PC += Offset if R(A) != R(B)")
X(OP_JMP_CMP_LT,            FORMAT_ABC,     "[op:8][A:8][B:8][Offset:8]", "PC += Offset if R(A) < R(B)")
X(OP_JMP_CMP_GT,            FORMAT_ABC,     "[op:8][A:8][B:8][Offset:8]", "PC += Offset if R(A) > R(B)")
X(OP_JMP_CMP_LE,            FORMAT_ABC,     "[op:8][A:8][B:8][Offset:8]", "PC += Offset if R(A) <= R(B)")
X(OP_JMP_CMP_GE,            FORMAT_ABC,     "[op:8][A:8][B:8][Offset:8]", "PC += Offset if R(A) >= R(B)")

X(OP_JMP_INT_CMP_EQ,        FORMAT_ABC,     "[op:8][A:8][B:8][Offset:8]", "PC += Offset if R(A) == R(B)")
X(OP_JMP_INT_CMP_NE,        FORMAT_ABC,     "[op:8][A:8][B:8][Offset:8]", "PC += Offset if R(A) != R(B)")
X(OP_JMP_INT_CMP_LT,        FORMAT_ABC,     "[op:8][A:8][B:8][Offset:8]", "PC += Offset if R(A) < R(B)")
X(OP_JMP_INT_CMP_GT,        FORMAT_ABC,     "[op:8][A:8][B:8][Offset:8]", "PC += Offset if R(A) > R(B)")
X(OP_JMP_INT_CMP_LE,        FORMAT_ABC,     "[op:8][A:8][B:8][Offset:8]", "PC += Offset if R(A) <= R(B)")
X(OP_JMP_INT_CMP_GE,        FORMAT_ABC,     "[op:8][A:8][B:8][Offset:8]", "PC += Offset if R(A) >= R(B)")

X(OP_JMP_INT_CMP_EQ_IMM,    FORMAT_ABC,     "[op:8][A:8][imm:8][offset:8]", "PC += Offset if R(A) == Imm")
X(OP_JMP_INT_CMP_NE_IMM,    FORMAT_ABC,     "[op:8][A:8][imm:8][offset:8]", "PC += Offset if R(A) != Imm")
X(OP_JMP_INT_CMP_LT_IMM,    FORMAT_ABC,     "[op:8][A:8][imm:8][offset:8]", "PC += Offset if R(A) < Imm")
X(OP_JMP_INT_CMP_GT_IMM,    FORMAT_ABC,     "[op:8][A:8][imm:8][offset:8]", "PC += Offset if R(A) > Imm")
X(OP_JMP_INT_CMP_LE_IMM,    FORMAT_ABC,     "[op:8][A:8][imm:8][offset:8]", "PC += Offset if R(A) <= Imm")
X(OP_JMP_INT_CMP_GE_IMM,    FORMAT_ABC,     "[op:8][A:8][imm:8][offset:8]", "PC += Offset if R(A) >= Imm")

X(OP_JMP_UINT_CMP_EQ,       FORMAT_ABC,     "[op:8][A:8][B:8][Offset:8]", "PC += Offset if R(A) == R(B)")
X(OP_JMP_UINT_CMP_NE,       FORMAT_ABC,     "[op:8][A:8][B:8][Offset:8]", "PC += Offset if R(A) != R(B)")
X(OP_JMP_UINT_CMP_LT,       FORMAT_ABC,     "[op:8][A:8][B:8][Offset:8]", "PC += Offset if R(A) < R(B)")
X(OP_JMP_UINT_CMP_LE,       FORMAT_ABC,     "[op:8][A:8][B:8][Offset:8]", "PC += Offset if R(A) <= R(B)")
X(OP_JMP_UINT_CMP_GT,       FORMAT_ABC,     "[op:8][A:8][B:8][Offset:8]", "PC += Offset if R(A) > R(B)")
X(OP_JMP_UINT_CMP_GE,       FORMAT_ABC,     "[op:8][A:8][B:8][Offset:8]", "PC += Offset if R(A) >= R(B)")

X(OP_PUSH,                  FORMAT_AxBx,    "[op:8][0:12][Rs:12]")
X(OP_PUSH_INT_IMM,          FORMAT_ABxx,    "[op:8][0:8][Imm:16]")
X(OP_PUSH_CONST,            FORMAT_AxBx,    "[op:8][0:12][Idx:12]")
X(OP_PUSH_RELOC,            FORMAT_ABxx,    "[op:8][0:8][Offset:16]")
X(OP_PUSH_2,                FORMAT_AxBx,    "[op:8][Rs1:12][Rs2:12]")
X(OP_PUSH_NONE,             FORMAT_ABC,     "[op:8][0:8][0:8][0:8]")
X(OP_PUSH_TRUE,             FORMAT_ABC,     "[op:8][0:8][0:8][0:8]")
X(OP_PUSH_FALSE,            FORMAT_ABC,     "[op:8][0:8][0:8][0:8]")
X(OP_PUSH_EMPTY_STR,        FORMAT_ABC,     "[op:8][0:8][0:8][0:8]")
X(OP_PUSH_EMPTY_LIST,       FORMAT_ABC,     "[op:8][0:8][0:8][0:8]")
X(OP_PUSH_EMPTY_DICT,       FORMAT_ABC,     "[op:8][0:8][0:8][0:8]")

X(OP_CALL,                  FORMAT_ABC,     "[op:8][Rd:8][imm:8][offset:8]")
X(OP_CALL_KW,               FORMAT_ABC,     "[op:8][Rd:8][imm:8][offset:8]")
X(OP_CALL_DYNAMIC,          FORMAT_ABC,     "[op:8][Rd:8][imm:8][offset:8]")
X(OP_CALL_DYNAMIC_KW,       FORMAT_ABC,     "[op:8][Rd:8][imm:8][offset:8]")

X(OP_RETURN,                FORMAT_AxBx,    "[op:8][0:12][Rs:12]",      "return R(Rs)")
X(OP_RETURN_NONE,           FORMAT_ABC,     "[op:8][0:8][0:8][0:8]",    "return None")

X(OP_AS,                    FORMAT_Ax,      "")
X(OP_IS,                    FORMAT_Ax,      "")

X(OP_GET_GLOBAL,            FORMAT_AxBx,    "[op:8][Rd:12][offset:12]")
X(OP_SET_GLOBAL,            FORMAT_AxBx,    "[op:8][Rd:12][offset:12]")

X(OP_GET_FIELD,             FORMAT_ABC,     "[op:8][Rd:12][Rs:12][offset:8]",   "Rd = R(Rs).field[offset]")
X(OP_SET_FIELD,             FORMAT_ABC,     "[op:8][Rd:12][Rs:12][offset:8]",   "R(Rs).field[offset] = R(Rd)")

X(OP_CAST_INTF,              FORMAT_ABC,    "[op:8][Rd:8][Rs:8][Idx:8]",    "Rd = (intf)Rs")

X(OP_SUBSCR_LOAD,            FORMAT_Ax,     "")
X(OP_SUBSCR_STORE,           FORMAT_Ax,     "")

X(OP_GET_ITER,               FORMAT_Ax,     "")
X(OP_ITER_NEXT,              FORMAT_Ax,     "")

X(OP_BINARY_ADD,             FORMAT_Ax,     "")
X(OP_BINARY_SUB,             FORMAT_Ax,     "")
X(OP_BINARY_MUL,             FORMAT_Ax,     "")
X(OP_BINARY_DIV,             FORMAT_Ax,     "")
X(OP_BINARY_MOD,             FORMAT_Ax,     "")
X(OP_UNARY_NEG,              FORMAT_Ax,     "")

X(OP_BINARY_AND,             FORMAT_Ax,     "")
X(OP_BINARY_OR,              FORMAT_Ax,     "")
X(OP_BINARY_XOR,             FORMAT_Ax,     "")
X(OP_UNARY_NOT,              FORMAT_Ax,     "")
X(OP_BINARY_SHL,             FORMAT_Ax,     "")
X(OP_BINARY_SHR,             FORMAT_Ax,     "")

X(OP_BINARY_CMP_EQ,          FORMAT_ABC,   "[op:8][Rd:8][Rs:18][0:8]",  "Rd = (Rs == Rt) ? 1 : 0")
X(OP_BINARY_CMP_NE,          FORMAT_ABC,   "[op:8][Rd:8][Rs:18][0:8]",  "Rd = (Rs != Rt) ? 1 : 0")
X(OP_BINARY_CMP_LT,          FORMAT_ABC,   "[op:8][Rd:8][Rs:18][0:8]",  "Rd = (Rs < Rt) ? 1 : 0" )
X(OP_BINARY_CMP_GT,          FORMAT_ABC,   "[op:8][Rd:8][Rs:18][0:8]",  "Rd = (Rs > Rt) ? 1 : 0" )
X(OP_BINARY_CMP_LE,          FORMAT_ABC,   "[op:8][Rd:8][Rs:18][0:8]",  "Rd = (Rs <= Rt) ? 1 : 0")
X(OP_BINARY_CMP_GE,          FORMAT_ABC,   "[op:8][Rd:8][Rs:18][0:8]",  "Rd = (Rs >= Rt) ? 1 : 0")

X(OP_RAISE,                  FORMAT_Ax,     "")
X(OP_WIDE,                   FORMAT_Ax,     "")

X(OP_IR_LOCAL,               FORMAT_Ax,     "")
X(OP_IR_PHI,                 FORMAT_Ax,     "")
X(OP_IR_JMP_COND,            FORMAT_Ax,     "")

// clang-format on
