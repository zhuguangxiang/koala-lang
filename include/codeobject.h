/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_CODE_OBJECT_H_
#define _KOALA_CODE_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _CodeObject {
    FUNCTION_HEAD
    /* name */
    char *name;
    /* file name */
    char *filename;
    /* line table(addr2line) */
    void *line_table;
    /* number of positional parameters(must be passed), not include KW parameters */
    int nargs;
    /* all locals, include parameters(pos-args&kw-args) */
    int nlocals;
    /* max number of call arguments, the callframe size is nlocals + max_nargs */
    int max_nargs;
    /* instructions */
    char *insns;
} CodeObject;

extern TypeObject code_type;
#define IS_CODE(ob) IS_TYPE((ob), &code_type)

Object *kl_new_code(char *name, Object *m, TypeObject *cls);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CODE_OBJECT_H_ */
