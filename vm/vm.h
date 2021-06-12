/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_VM_H_
#define _KOALA_VM_H_

#include "util/common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _CallInfo CallInfo;
typedef struct _KoalaState KoalaState;
typedef uintptr StkVal;

struct _CallInfo {
    // next callinfo
    CallInfo *next;
    // back callinfo
    CallInfo *prev;
    // top stack
    StkVal *top;
    // stack base
    StkVal *base;
    // byte code
    uint8 *code;
    /* code index */
    uint8 *savedpc;
    /* relocations */
    uintptr relinfo;
    // gc stack
    void *gc_stack[0];
};

struct _KoalaState {
    // call depth
    int nci;
    // call info
    CallInfo *ci;

    // stack top
    StkVal *top;
    // stack base
    StkVal *stack;
    // stack end
    StkVal *stack_end;

    // first call info
    CallInfo base_ci;
};

void koala_execute(KoalaState *ks, CallInfo *ci);
void eval(KoalaState *ks);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_VM_H_ */
