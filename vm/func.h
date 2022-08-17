/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2022-2032 James <zhuguangxiang@gmail.com>
 */

#ifndef _KOALA_FUNC_H_
#define _KOALA_FUNC_H_

#include "opcode.h"
#include "typedesc.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* forward declaration */
typedef struct _KlFunc KlFunc;
typedef struct _KlLocal KlLocal;

/* koala function */
struct _KlFunc {
    /* func name */
    char *name;
    /* type */
    TypeDesc *proto;
    /* number of parameters */
    int num_params;
    /* parameter size */
    int param_size;
    /* return size */
    int ret_size;
    /* byte codes */
    int code_size;
    uint8_t *codes;
    /* stack size(align=8) */
    int stack_size;
    /* local info */
    int num_locals;
    KlLocal *locals;
};

/* local variable info */
struct _KlLocal {
    /* local name */
    char *name;
    /* local type */
    TypeDesc *desc;
    /* local offset(align=8) */
    int offset;
    /* local size(align=8) */
    int size;
};

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_FUNC_H_ */
