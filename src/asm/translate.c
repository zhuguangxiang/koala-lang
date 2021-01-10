/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "kl_llvm.h"
#include "opcode.h"

#define NEXT_OP() codes[pc++]

#define POP(v)  vector_pop_back(stack, &(v))
#define PUSH(v) vector_push_back(stack, &(v))

#define SET_LOCAL(i, v) vector_set(locals, i, &(v))

#define GET_LOCAL(i, v)          \
    vector_get(locals, i, &(v)); \
    PUSH(v)

/* x = x + y */
#define _add(x, y)                                               \
    POP(x);                                                      \
    POP(y);                                                      \
    x.llvalue = LLVMBuildAdd(builder, x.llvalue, y.llvalue, ""); \
    PUSH(x)

/* return x */
#define _ret(x) \
    POP(x);     \
    LLVMBuildRet(builder, x.llvalue)

void translate(kl_llvm_ctx_t *ctx)
{
    kl_llvm_func_t *fn = ctx->func;
    LLVMBuilderRef builder = ctx->builder;
    CodeObject *co = fn->co;
    int codesize = co->size;
    uint8_t *codes = co->codes;
    Vector *locals = &fn->locals;
    Vector *stack = &ctx->stack;
    int pc = 0;
    uint8_t op;
    kl_llvm_value_t x, y;
    LLVMValueRef val;

    while (pc < codesize) {
        op = NEXT_OP();
        switch (op) {
            case OP_DUP: {
                break;
            }
            case OP_SWAP: {
                break;
            }
            case OP_CONST_BYTE: {
                break;
            }
            case OP_LOAD: {
                break;
            }
            case OP_LOAD_0: {
                GET_LOCAL(0, x);
                break;
            }
            case OP_LOAD_1: {
                GET_LOCAL(1, x);
                break;
            }
            case OP_STORE: {
                break;
            }
            case OP_ADD: {
                _add(x, y);
                break;
            }
            case OP_SUB: {
                break;
            }
            case OP_CALL: {
                break;
            }
            case OP_RETURN_VALUE: {
                _ret(x);
                break;
            }
            default: {
                printf("error: unrecognized opcode\n");
                abort();
            }
        }
    }
}
