/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2022-2032 James <zhuguangxiang@gmail.com>
 */

#define vm_dispatch(x) goto *disptab[x];
#define vm_case(l)     L_##l:
#define vm_break       vm_dispatch(NEXT_OP());

static const void *const disptab[] = {
    &&L_OP_HALT,
    &&L_OP_I32_CONST,
    &&L_OP_I32_ADD,
    &&L_OP_I32_SUB,
    &&L_OP_LOCAL_GET,
    &&L_OP_LOCAL_SET,
    &&L_OP_JMP_INT32_CMPGT,
    &&L_OP_CALL,
    &&L_OP_RETURN,
    &&L_OP_RETURN_VALUE,
    &&L_OP_I32_CONST_0,
    &&L_OP_I32_CONST_1,
    &&L_OP_I32_CONST_2,
};
