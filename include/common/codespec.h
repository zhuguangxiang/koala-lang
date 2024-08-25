/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * An Atom is a null-terminated string cached in internal hashmap.
 * With null-terminated, it's convenient to operate it like c-string.
 */

#ifndef _KOALA_CODE_SPEC_H_
#define _KOALA_CODE_SPEC_H_

#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _CodeSpec {
    /* name */
    const char *name;
    /* file name */
    const char *filename;
    /* line table(addr2line) */
    void *line_table;
    /* positional arguments number */
    int nargs;
    /* all locals, include arguments */
    int nlocals;
    /* max number of call parameters */
    int stack_size;
    /* size of instructions */
    int insns_size;
    /* instructions */
    const char *insns;
    /* constants */
    Vector consts;
    /* default keyword arguments */
    Vector default_kwargs;
} CodeSpec;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CODE_SPEC_H_ */
