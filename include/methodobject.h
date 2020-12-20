/*===-- methodobject.h - Method Object ----------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This header declares the Koala `Method` object.                            *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_METHOD_OBJECT_H_
#define _KOALA_METHOD_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* method kind */
typedef enum {
    KFUNC_KIND = 1,
    CFUNC_KIND,
    PROTO_KIND,
} method_kind_t;

/* `Method` object layout */
typedef struct MethodObject {
    OBJECT_HEAD
    /* method name */
    const char *name;
    /* cfunc, kcode or intf */
    method_kind_t kind;
    /* cfunc or kcode */
    void *ptr;
} MethodObject;

void init_method_type(void);
Object *cmethod_new(MethodDef *def);
Object *method_new(char *name, Object *code);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_METHOD_OBJECT_H_ */
