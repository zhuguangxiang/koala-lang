/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "codeobject.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeObject *code_type;

Object *code_new(int nloc, uint8_t *codes, int size)
{
    CodeObject *cobj = alloc_meta_object(CodeObject);
    cobj->codes = codes;
    cobj->size = size;
    cobj->nloc = nloc;
    return (Object *)cobj;
}

void init_code_type(void)
{
    /* `Code` is public final class */
    code_type = type_new_class("Code");
    type_set_public_final(code_type);

    /* code_type ready */
    type_ready(code_type);

    /* show code_type */
    type_show(code_type);
}

#ifdef __cplusplus
}
#endif
