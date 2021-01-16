/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "opcode.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OPCODE(op, argc, asm_str) \
    {                             \
        op, argc, asm_str         \
    }

static struct opcode {
    uint8_t op;
    uint8_t argc;
    char *str;
} opcodes[] = {
    OPCODE(OP_HALT, 0, "halt"),
    OPCODE(OP_POP_TOP, 0, "pop_top"),
    OPCODE(OP_DUP, 0, "dup"),
    OPCODE(OP_SWAP, 0, "swap"),

    OPCODE(OP_CONST_INT8, 2, "const_int8"),
    OPCODE(OP_CONST_INT16, 2, "const_int16"),
    OPCODE(OP_CONST_INT32, 2, "const_int32"),
    OPCODE(OP_CONST_INT64, 2, "const_int32"),
    OPCODE(OP_CONST_FLOAT32, 2, "const_float32"),
    OPCODE(OP_CONST_FLOAT64, 2, "const_float64"),
    OPCODE(OP_CONST_TRUE, 0, "const_true"),
    OPCODE(OP_CONST_FALSE, 0, "const_false"),
    OPCODE(OP_CONST_CHAR, 2, "const_char"),
    OPCODE(OP_CONST_STRING, 2, "const_string"),

    OPCODE(OP_LOAD, 1, "load"),
    OPCODE(OP_LOAD_0, 0, "load0"),
    OPCODE(OP_LOAD_1, 0, "load1"),
    OPCODE(OP_LOAD_2, 0, "load2"),
    OPCODE(OP_LOAD_3, 0, "load3"),

    OPCODE(OP_STORE, 1, "store"),
    OPCODE(OP_STORE_0, 0, "store"),
    OPCODE(OP_STORE_1, 0, "store1"),
    OPCODE(OP_STORE_2, 0, "store2"),
    OPCODE(OP_STORE_3, 0, "store3"),

    OPCODE(OP_ADD, 0, "add"),
    OPCODE(OP_SUB, 0, "sub"),
    OPCODE(OP_MUL, 0, "mul"),
    OPCODE(OP_DIV, 0, "div"),
    OPCODE(OP_MOD, 0, "mod"),
    OPCODE(OP_POW, 0, "pow"),
    OPCODE(OP_NEG, 0, "neg"),

    OPCODE(OP_GT, 0, "gt"),
    OPCODE(OP_GE, 0, "ge"),
    OPCODE(OP_LT, 0, "lt"),
    OPCODE(OP_LE, 0, "le"),
    OPCODE(OP_EQ, 0, "eq"),
    OPCODE(OP_NE, 0, "ne"),

    OPCODE(OP_AND, 0, "and"),
    OPCODE(OP_OR, 0, "or"),
    OPCODE(OP_NOT, 0, "not"),

    OPCODE(OP_BIT_AND, 0, "bit_and"),
    OPCODE(OP_BIT_OR, 0, "bit_or"),
    OPCODE(OP_BIT_XOR, 0, "bit_xor"),
    OPCODE(OP_BIT_NOT, 0, "bit_not"),
    OPCODE(OP_BIT_LSHIFT, 0, "bit_lshift"),
    OPCODE(OP_BIT_RSHIFT, 0, "bit_rshift"),

    OPCODE(OP_INPLACE_ADD, 0, "inplace_add"),
    OPCODE(OP_INPLACE_SUB, 0, "inplace_sub"),
    OPCODE(OP_INPLACE_MUL, 0, "inplace_mul"),
    OPCODE(OP_INPLACE_DIV, 0, "inplace_div"),
    OPCODE(OP_INPLACE_MOD, 0, "inplace_mod"),
    OPCODE(OP_INPLACE_POW, 0, "inplace_pow"),
    OPCODE(OP_INPLACE_AND, 0, "inplace_and"),
    OPCODE(OP_INPLACE_OR, 0, "inplace_or"),
    OPCODE(OP_INPLACE_XOR, 0, "inplace_xor"),
    OPCODE(OP_INPLACE_LSHIFT, 0, "inplace_lshift"),
    OPCODE(OP_INPLACE_RSHIFT, 0, "inplace_rshift"),

    OPCODE(OP_PRINT, 0, "print"),
    OPCODE(OP_CALL, 1, "call"),
    OPCODE(OP_RETURN, 0, "return"),
    OPCODE(OP_RETURN_VALUE, 0, "return_value"),
};

char *opcode_str(uint8_t op)
{
    struct opcode *opcode;
    for (int i = 0; i < COUNT_OF(opcodes); i++) {
        opcode = opcodes + i;
        if (opcode->op == op) { return opcode->str; }
    }
    abort();
}

uint8_t opcode_argc(uint8_t op)
{
    struct opcode *opcode;
    for (int i = 0; i < COUNT_OF(opcodes); i++) {
        opcode = opcodes + i;
        if (opcode->op == op) { return opcode->argc; }
    }
    abort();
}

#ifdef __cplusplus
}
#endif
