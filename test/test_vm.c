/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include <assert.h>
#include "util/mm.h"
#include "vm/opcode.h"
#include "vm/vm.h"

/*
    func add(a int8, b int8) int8 {
        return a + b
    }

    add(1, 2)

    stack[0] = a,
    stack[1] = b,
    stack[0] = a + b,
*/

// little endian
#define VAL_I8(val) (val) & 0xFF
#define VAL_U8(val) (val) & 0xFF
#define VAL_I32(val)                                         \
    (val) & 0xFF, ((val) >> 8) & 0xFF, ((val) >> 16) & 0xFF, \
        ((val) >> 24) & 0xFF

void test_opcode(void)
{
    uint8 codes[] = {
        OP_I8K, 0, VAL_I8(-3), OP_I32_ADDK, 0, 0, VAL_U8(254), OP_RET,
    };

    KoalaState ks;
    ks.ci = &ks.base_ci;
    ks.nci = 1;
    ks.stack = mm_alloc(32 * sizeof(StkVal));
    ks.stack_end = ks.stack + 32;

    int stacksize = 5;
    CallInfo *ci = ks.ci;
    ci->code = codes;
    ci->base = ks.stack;
    ci->top = ks.stack + stacksize;
    ci->savedpc = codes;
    ks.top = ci->top;

    koala_execute(&ks, ci);
    assert((int32)ci->base[0] == 251);

    /*
    // ci->base[0] = 10;
    // ci->base[1] = 20;
    // koala_execute(&ks, ci);
    // assert(ci->base[0] == 30);

    ci->base[0] = 100;
    ci->base[1] = 100;
    koala_execute(&ks, ci);
    assert(ci->base[0] == 200);
    */
}

int main(int argc, char *argv[])
{
    test_opcode();
    return 0;
}
