/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeInfo pkg_type = {
    .name = "Package",
    .flags = TP_CLASS | TP_FINAL,
};

void init_package_type(void)
{
    type_ready(&pkg_type);
    type_show(&pkg_type);
}

INIT_FUNC_LEVEL_1(init_package_type);

#ifdef __cplusplus
}
#endif
