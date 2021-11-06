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

static TypeInfo option_type = {
    .name = "Option",
    .flags = TF_ENUM,
};

void init_option_type(void)
{
    MethodDef option_methods[] = {
        /* clang-format off */
        /* clang-format on */
    };

    type_add_methdefs(&option_type, option_methods);
    type_ready(&option_type);
    type_show(&option_type);
    pkg_add_type(root_pkg, &option_type);
}

#ifdef __cplusplus
}
#endif
