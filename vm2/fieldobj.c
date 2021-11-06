/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* final class Field { } */

static TypeInfo field_type = {
    .name = "Field",
    .flags = TF_CLASS | TF_FINAL,
};

void init_field_type(void)
{
    MethodDef field_methods[] = {
        /* clang-format off */
        /* clang-format on */
    };

    type_add_methdefs(&field_type, field_methods);
    type_ready(&field_type);
    type_show(&field_type);
    pkg_add_type(root_pkg, &field_type);
}

#ifdef __cplusplus
}
#endif
