/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_DISPTAB_H_
#define _KOALA_DISPTAB_H_

#ifdef __cplusplus
extern "C" {
#endif

#define vm_dispatch(x) goto *disptab[x];
#define vm_case(l)     L_##l:
#define vm_break       vm_dispatch(NEXT_OP());

static const void *const disptab[] = {
    &&L_OP_JMP_I32_CMPGTK, &&L_OP_RET_VALUE, &&L_OP_I32_SUBK, &&L_OP_I32_ADD,
    &&L_OP_PUSH,           &&L_OP_POP,       &&L_OP_CALL,
};

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_DISPTAB_H_ */
