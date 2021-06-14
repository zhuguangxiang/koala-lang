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
final class Package { }
*/

static TypeInfo pkg_type = {
    .name = "Package",
    .flags = TF_CLASS | TF_FINAL,
};

void init_package_type(void)
{
    MethodDef pkg_methods[] = {
        /* clang-format off */
        /* clang-format on */
    };

    type_add_methdefs(&pkg_type, pkg_methods);
    type_ready(&pkg_type);
    type_show(&pkg_type);
    pkg_add_type("/", &pkg_type);
}

#ifdef __cplusplus
}
#endif
