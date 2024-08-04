/**
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_CFUNC_OBJECT_H_
#define _KOALA_CFUNC_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _CFuncObject {
    OBJECT_HEAD
    /* cfunc definition */
    MethodDef *def;
    /* module */
    Object *mod;
    /* class, can be null */
    TypeObject *cls;
    /* object call */
    CallFunc call;
} CFuncObject;

extern TypeObject cfunc_type;
#define IS_CFUNC(ob) IS_TYPE((ob), &cfunc_type)

#define METH_NOARG    0x0001
#define METH_ONE      0x0002
#define METH_KEYWORDS 0x0004
#define METH_FASTCALL 0x0008

typedef Value (*CFunc)(Value *);
typedef Value (*CFuncOne)(Value *, Value *);
typedef Value (*CFuncFast)(Value *, Value *, int);
typedef Value (*CFuncFastWithKeywords)(Value *, Value *, int, Object *);

Object *kl_new_cfunc(MethodDef *def, Object *mod, TypeObject *cls);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CFUNC_OBJECT_H_ */
