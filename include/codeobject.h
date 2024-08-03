/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_CODE_OBJECT_H_
#define _KOALA_CODE_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _CodeObject {
    OBJECT_HEAD
    /* name */
    char *name;
    /* file name */
    char *filename;
    /* line table(addr2line) */
    void *line_table;
    /* positional arguments number */
    int nargs;
    /* all locals, include arguments */
    int nlocals;
    /* max number of call parameters */
    int stack_size;
    /* number of instructions */
    int num_insns;
    /* instructions */
    char *insns;
    /* constants */
    Vector consts;
    /* default keyword arguments */
    Object *kwargs;
} CodeObject;

extern TypeObject code_type;
#define IS_CODE(ob) IS_TYPE((ob), &code_type)

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CODE_OBJECT_H_ */
