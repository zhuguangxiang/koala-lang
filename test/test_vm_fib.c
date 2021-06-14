/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include <assert.h>
#include <time.h>
#include "util/mm.h"
#include "vm/opcode.h"
#include "vm/vm.h"

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
        OP_I32_JMP_CMPKGT, 0, 1, 1, 0,
        OP_RET,
        OP_PUSH_I32_SUBK, 0, 1,
        OP_CALL, 1, 0, 0,
        OP_SAVE_RET, 1,
        OP_PUSH_I32_SUBK, 0, 2,
        OP_CALL, 1, 0, 0,
        OP_I32_ADD_RET, 0, 1,
        OP_RET,
    };

    /* clang-format on */

    KoalaState ks = { 0 };
    ks.ci = &ks.base_ci;
    ks.nci = 1;
    ks.stack = mm_alloc(320 * sizeof(StkVal));
    ks.stack_end = ks.stack + 320;

    int stacksize = 3;
    CallInfo *ci = ks.ci;
    ci->code = codes;
    ci->base = ks.stack;
    ci->top = ci->base + stacksize - 1;
    ci->savedpc = codes;
    ci->relinfo = (uintptr)codes;

    ks.top = ci->top;

    ci->base[0] = 40;
    // time_t start, end;
    clock_t start, end;
    // time(&start);
    start = clock();
    koala_execute(&ks, ci);
    // time(&end);
    end = clock();
    printf("k-fib:%ld, %lf\n", ci->base[0], difftime(end, start));
}

static int fib(int n)
{
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int main(int argc, char *argv[])
{
    clock_t start, end;
    start = clock();
    int r = fib(40);
    end = clock();
    printf("c-fib:%d, %lf\n", r, difftime(end, start));
    test_fib();
    return 0;
}

#ifdef __cplusplus
}
#endif
