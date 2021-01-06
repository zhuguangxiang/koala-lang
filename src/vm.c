/*===-- vm.c - Koala Virtual Machine ------------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This file implements koala virtual machine.                                *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "vm.h"
#include "codeobject.h"
#include "opcode.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OPCODE(op, argc)  \
    {                     \
        op, argc, #op + 3 \
    }

static struct opcode {
    uint8_t op;
    uint8_t argc;
    char *str;
} opcodes[] = {
    OPCODE(OP_HALT, 0),
    OPCODE(OP_POP_TOP, 0),
    OPCODE(OP_DUP, 0),
    OPCODE(OP_SWAP, 0),

    OPCODE(OP_CONST_BYTE, 1),
    OPCODE(OP_CONST_INT16, 2),
    OPCODE(OP_CONST_INT32, 4),
    OPCODE(OP_CONST_INT, 8),
    OPCODE(OP_CONST_FLOAT, 8),
    OPCODE(OP_CONST_CHAR, 4),
    OPCODE(OP_CONST_TRUE, 0),
    OPCODE(OP_CONST_FALSE, 0),
    OPCODE(OP_CONST_STRING, 4),

    OPCODE(OP_LOAD, 1),
    OPCODE(OP_LOAD_0, 0),
    OPCODE(OP_LOAD_1, 0),
    OPCODE(OP_LOAD_2, 0),
    OPCODE(OP_LOAD_3, 0),

    OPCODE(OP_STORE, 1),
    OPCODE(OP_STORE_0, 0),
    OPCODE(OP_STORE_1, 0),
    OPCODE(OP_STORE_2, 0),
    OPCODE(OP_STORE_3, 0),

    OPCODE(OP_ADD, 0),
    OPCODE(OP_SUB, 0),
    OPCODE(OP_MUL, 0),
    OPCODE(OP_DIV, 0),
    OPCODE(OP_MOD, 0),
    OPCODE(OP_POW, 0),
    OPCODE(OP_NEG, 0),

    OPCODE(OP_GT, 0),
    OPCODE(OP_GE, 0),
    OPCODE(OP_LT, 0),
    OPCODE(OP_LE, 0),
    OPCODE(OP_EQ, 0),
    OPCODE(OP_NE, 0),

    OPCODE(OP_AND, 0),
    OPCODE(OP_OR, 0),
    OPCODE(OP_NOT, 0),

    OPCODE(OP_BIT_AND, 0),
    OPCODE(OP_BIT_OR, 0),
    OPCODE(OP_BIT_XOR, 0),
    OPCODE(OP_BIT_NOT, 0),
    OPCODE(OP_BIT_LSHIFT, 0),
    OPCODE(OP_BIT_RSHIFT, 0),

    OPCODE(OP_INPLACE_ADD, 0),
    OPCODE(OP_INPLACE_SUB, 0),
    OPCODE(OP_INPLACE_MUL, 0),
    OPCODE(OP_INPLACE_DIV, 0),
    OPCODE(OP_INPLACE_MOD, 0),
    OPCODE(OP_INPLACE_POW, 0),
    OPCODE(OP_INPLACE_AND, 0),
    OPCODE(OP_INPLACE_OR, 0),
    OPCODE(OP_INPLACE_XOR, 0),
    OPCODE(OP_INPLACE_LSHIFT, 0),
    OPCODE(OP_INPLACE_RSHIFT, 0),
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

#define NEXT_OP() (assert(pc < codesize), codes[pc++])

#define TOP()   *(top - 1)
#define POP()   *(--top)
#define PUSH(v) (*top++ = (v))

#define NEXT_BYTE() (assert(pc < codesize), codes[pc++])

#define CHECK()                                \
    if (top < base || top >= ks->stack_last) { \
        printf("error: stack overlfow\n");     \
    }

#define SET_LOCAL(i, v) *(ci->func + 1 + i) = (v)
#define GET_LOCAL(i)    *(ci->func + 1 + i)

void eval(KoalaState *ks)
{
    CallInfo *ci = ks->ci;

    /* calculate stack */
    TValueRef *base = ci->top;
    TValueRef *top = ks->top;

    /* opcode array */
    Object *cobj = ci->func->_v.obj;
    uint8_t *codes = ((CodeObject *)cobj)->codes;
    int codesize = ((CodeObject *)cobj)->size;

    /* code pc */
    int pc = ci->saved_pc;

    /* current opcode */
    uint8_t op;

    /* temperary variables */
    TValueRef x, y;
    int iarg;

    while (1) {
        op = NEXT_OP();
        CHECK();
        switch (op) {
            case OP_POP_TOP: {
                x = POP();
                break;
            }
            case OP_DUP: {
                x = TOP();
                PUSH(x);
                break;
            }
            case OP_SWAP: {
                x = POP();
                y = POP();
                PUSH(x);
                PUSH(y);
                break;
            }
            case OP_CONST_BYTE: {
                int8_t bval = NEXT_BYTE();
                setbyteval(&x, bval);
                PUSH(x);
                break;
            }
            case OP_LOAD: {
                abort();
                break;
            }
            case OP_LOAD_0: {
                x = GET_LOCAL(0);
                PUSH(x);
                break;
            }
            case OP_LOAD_1: {
                x = GET_LOCAL(1);
                PUSH(x);
                break;
            }
            case OP_STORE: {
                iarg = NEXT_BYTE();
                x = POP();
                SET_LOCAL(iarg, x);
                break;
            }
            case OP_ADD: {
                x = POP();
                y = POP();
                if (x._t.tag == 1 && y._t.tag == 1) {
                    if (x._t.kind == TYPE_BYTE && y._t.kind == TYPE_BYTE) {
                        x._v.bval += y._v.bval;
                        PUSH(x);
                    }
                }
                else {
                }
                break;
            }
            case OP_SUB: {
                break;
            }
            case OP_HALT: {
                abort();
            }
            default: {
                printf("error: unrecognized opcode\n");
                abort();
            }
        }
    }
}

DLLEXPORT KoalaState *kl_new_state(void)
{
    KoalaState *ks;

    /* create new koalastate */
    ks = mm_alloc(sizeof(*ks));

    /* initialize stack  */
    ks->stack = mm_alloc(MAX_STACK_SIZE * sizeof(TValueRef));
    ks->stacksize = MAX_STACK_SIZE;
    ks->stack_last = ks->stack + ks->stacksize;
    ks->top = ks->stack;

    /* initialize ci */
    CallInfo *ci = &ks->base_ci;
    ci->back = NULL;
    ci->func = ks->top;
    /* `func` entry for this `ci` */
    setnilval(ks->top);
    ks->top++;
    ci->top = ks->top;

    ks->ci = ci;
    ks->nci = 1;

    return ks;
}

static void __new_call(KoalaState *ks, int loc)
{
    if (ks->nci >= MAX_CALL_DEPTH) {
        printf("error: stack overflow(too many call frame)\n");
        abort();
    }

    CallInfo *ci = mm_alloc(sizeof(CallInfo));
    ci->back = ks->ci;
    ci->func = ks->top - loc - 1;
    ci->top = ks->top;
    ks->ci = ci;
    ks->nci++;
}

void kl_do_call(KoalaState *ks, int argc)
{
    __new_call(ks, argc);
    eval(ks);
}

#ifdef __cplusplus
}
#endif
