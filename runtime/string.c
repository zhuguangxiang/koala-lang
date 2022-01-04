/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

KlTypeInfo string_type = {
    .name = "string",
    .flags = TP_CLASS | TP_FINAL,
};

int kl_string_push(KlState *ks)
{
    KlValue val = kl_pop_value(ks);
    int32 ch = kl_pop_char(ks);
    return 0;
}

void init_string_type(void)
{
    MethodDef methods[] = {
        { "push", "c", null, kl_string_push },
        { null },
    };

    type_add_methdefs(&string_type, methods);
    type_ready(&string_type);
    pkg_add_type("/", &string_type);
}

KlTypeInfo char_type = {
    .name = "char",
    .flags = TP_CLASS | TP_FINAL,
};

void init_char_type(void)
{
    type_ready(&char_type);
    pkg_add_type("/", &char_type);
}

INIT_LEVEL_2(init_string_type);
INIT_LEVEL_2(init_char_type);

#ifdef __cplusplus
}
#endif
