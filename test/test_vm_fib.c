/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include <assert.h>
#include <time.h>
#include "koala.h"
#include "opcode.h"
#include "util/mm.h"
#include "vm.h"

#if 0
    OP_PUSH_I32_ADD,        /* A  B             R(++top) = R(A) + R(B)      */
    OP_PUSH_I32_SUB,        /* A  B             R(++top) = R(A) - R(B)      */
    OP_PUSH_I32_MUL,        /* A  B  C          R(A) = R(B) * R(C)          */
    OP_PUSH_I32_DIV,             /* A  B  C          R(A) = R(B) / R(C)          */
    OP_PUSH_I32_MOD,             /* A  B  C          R(A) = R(B) % R(C)          */
    OP_PUSH_I32_NEG,             /* A  B             R(A) = -R(B)                */
    OP_PUSH_I32_AND,             /* A  B  C          R(A) = R(B) & R(C)          */
    OP_PUSH_I32_OR,              /* A  B  C          R(A) = R(B) | R(C)          */
    OP_PUSH_I32_XOR,             /* A  B  C          R(A) = R(B) ^ R(C)          */
    OP_PUSH_I32_SHL,             /* A  B  C          R(A) = R(B) << R(C)         */
    OP_PUSH_I32_SHR,             /* A  B  C          R(A) = R(B) >> R(C)         */
    OP_PUSH_I32_USHR,            /* A  B  C          R(A) = R(B) >>> R(C)        */

    OP_PUSH_I32_ADDK,            /* A  B  K(1)       R(A) = R(B) + (u8)K         */
    OP_PUSH_I32_SUBK,            /* A  B  K(1)       R(A) = R(B) - (u8)K         */
    OP_PUSH_I32_MULK,            /* A  B  K(1)       R(A) = R(B) * (u8)K         */
    OP_PUSH_I32_DIVK,            /* A  B  K(1)       R(A) = R(B) / (u8)K         */
    OP_PUSH_I32_MODK,            /* A  B  K(1)       R(A) = R(B) % (u8)K         */
    OP_PUSH_I32_ANDK,            /* A  B  K(1)       R(A) = R(B) & (u8)K         */
    OP_PUSH_I32_ORK,             /* A  B  K(1)       R(A) = R(B) | (u8)K         */
    OP_PUSH_I32_XORK,            /* A  B  K(1)       R(A) = R(B) ^ (u8)K         */
    OP_PUSH_I32_SHLK,            /* A  B  K(1)       R(A) = R(B) << (u8)K        */
    OP_PUSH_I32_SHRK,            /* A  B  K(1)       R(A) = R(B) >> (u8)K        */
    OP_PUSH_I32_USHRK,           /* A  B  K(1)       R(A) = R(B) >>> (u8)K       */

    OP_PUSH_I32_ADDK,       /* A  K(1)          R(++top) = R(A) + (u8)K     */
    OP_PUSH_I32_SUBK,       /* A  K(1)          R(++top) = R(A) - (u8)K     */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
func fib(n int32) int32 {
    if n <= 1 return n
    return fib(n - 1) + fib(n - 2)
}
*/

/*
    # 0: n
    # 1: tmp
    # 2: tmp
    # 3: tmp
    OP_I32_CMP_CONST 1, 0, 1
    OP_JMP_GT 1, $.L0
    OP_RET_VAL 0 ##-> opt as OP_RET, if A == 0
.L0:
    OP_I32_SUB_CONST 2, 0, 1
    OP_PUSH 2
    OP_CALL 1, fib
    OP_SAVE_RET 2
    OP_I32_SUB_CONST 3, 0, 2
    OP_PUSH 3
    OP_CALL 1, fib
    OP_SAVE_RET 3
    OP_I32_ADD 0, 2, 3
    OP_RET_VAL 0 ##-> opt as OP_RET, if A == 0
*/

/*
    # 0: n
    # 1: tmp
    # 2: tmp
    OP_I32_CMP_CONST 1, 0, 1
    OP_JMP_GT 1, $.L0
    OP_RET_VAL 0 ##-> opt as OP_RET, if A == 0
.L0:
    OP_I32_SUB_CONST 1, 0, 1
    OP_PUSH 1
    OP_CALL 1, fib
    OP_SAVE_RET 1
    OP_I32_SUB_CONST 2, 0, 2
    OP_PUSH 2
    OP_CALL 1, fib
    OP_SAVE_RET 2
    OP_I32_ADD 0, 1, 2
    OP_RET_VAL 0 ##-> opt as OP_RET, if A == 0
*/

#if 0
#define OP_I32_JMP_CMPKGT 0
#define OP_RET            1
#define OP_SAVE_RET       2
#define OP_I32_ADD_RET    3
#define OP_PUSH_I32_SUBK  4
#define OP_CALL           5
#endif

void test_fib(void)
{
    /* clang-format off */
    /*
    uint8 codes[] = {
        OP_I32_CMP_CONST, 1, 0, 1,
        OP_JMP_GT, 1, 1, 0,
        OP_RET,
        OP_I32_SUB_CONST, 2, 0, 1,
        OP_PUSH, 2,
        OP_CALL, 1, 0, 0,
        OP_SAVE_RET, 2,
        OP_I32_SUB_CONST, 3, 0, 2,
        OP_PUSH, 3,
        OP_CALL, 1, 0, 0,
        OP_SAVE_RET, 3,
        OP_I32_ADD, 0, 2, 3,
        OP_RET,
    };

    uint8 codes[] = {
        OP_I32_CMP_CONST, 1, 0, 1,
        OP_JMP_GT, 1, 1, 0,
        OP_RET,
        OP_I32_SUB_CONST, 1, 0, 1,
        OP_PUSH, 1,
        OP_CALL, 1, 0, 0,
        OP_SAVE_RET, 1,
        OP_I32_SUB_CONST, 0, 0, 2,
        OP_PUSH, 0,
        OP_CALL, 1, 0, 0,
        OP_SAVE_RET, 0,
        OP_I32_ADD, 0, 1, 0,
        OP_RET,
    };

    */

    uint8 codes[] = {
        OP_CONST_I8 1, 1
        OP_JMP_I32_CMPGT, 2, 1, 0,
        OP_RET_VALUE, 0,
        OP_CONST_I8 1, 1
        OP_I32_SUB, -1, 0, 1,
        // OP_PUSH, 1,
        //OP_I32_SUBK_PUSH, 0, 1,
        OP_CALL, 0, 0, 1,
        OP_POP, 1,
        OP_CONST_I8 1, 2
        OP_I32_SUB, 2, 0, 1,
        // OP_PUSH, 2,
        //OP_I32_SUBK_PUSH, 0, 2,
        OP_CALL, 0, 0, 1,
        OP_POP, 2,
        OP_I32_ADD, 0, 1, 2,
        OP_RET_VALUE, 0,
    };

    /* clang-format on */

    KlState ks = { 0 };
    ks.cf = &ks.base_cf;
    ks.depth = 1;
    ks.base = mm_alloc(320 * sizeof(KlValue));
    ks.last = ks.base + 320;

    KlFrame *cf = ks.cf;
    cf->base = ks.base - 1;
    cf->top = cf->base - 1;
    ks.top = cf->top;

    kl_push_int32(&ks, 40);
    KlCode code = {
        .codes = codes,
        .size = sizeof(codes),
    };

    KlFunc fn = {
        .kind = MNODE_KFUNC_KIND,
        .ptr = (uintptr)&code,
    };

    // time_t start, end;
    clock_t start, end;
    // time(&start);
    start = clock();
    kl_eval_func(&ks, &fn);
    // time(&end);
    end = clock();
    int res = kl_pop_int32(&ks);
    printf("k-fib:%d, %lf\n", res, difftime(end, start));
}

int fib(int n)
{
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int main(int argc, char *argv[])
{
    kl_init();
    clock_t start, end;
    start = clock();
    int r = fib(40);
    end = clock();
    signed char v = 0xFE;
    printf("v&0x80=%x\n", v & 0x80);
    printf("c-fib:%d, %lf\n", r, difftime(end, start));
    test_fib();
    kl_fini();
    return 0;
}

#ifdef __cplusplus
}
#endif
