/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_CODE_OBJECT_H_
#define _KOALA_CODE_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* local variable info */
typedef struct locvar {
    char *name;
    TypeDesc *desc;
} locvar_t;

/* `Code` object layout */
typedef struct CodeObject {
    OBJECT_HEAD
    /* code name */
    char *name;
    /* code proto type */
    TypeDesc *proto;
    /* local variables */
    Vector locals;
    /* code size */
    int size;
    /* byte codes */
    uint8_t *codes;
} CodeObject;

/* initialize `Code` type */
void init_code_type(void);

/* new code object */
Object *code_new(char *name, TypeDesc *proto, uint8_t *codes, int size);

/* add local variable */
static inline void code_add_locvar(Object *obj, char *name, TypeDesc *type)
{
    CodeObject *cobj = (CodeObject *)obj;
    locvar_t locvar = { name, type };
    vector_push_back(&cobj->locals, &locvar);
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_CODE_OBJECT_H_ */
