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

/*
final class Method { }
*/

static TypeInfo method_type = {
    .name = "Method",
    .flags = TF_CLASS | TF_FINAL,
};

void init_method_type(void)
{
    MethodDef meth_methods[] = {
        /* clang-format off */
        /* clang-format on */
    };

    type_add_methdefs(&method_type, meth_methods);
    type_ready(&method_type);
    type_show(&method_type);
    pkg_add_type("/", &method_type);
}

#ifdef __cplusplus
}
#endif
