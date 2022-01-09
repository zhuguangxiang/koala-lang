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

TypeInfo array_type = {
    .name = "Array",
    .flags = TP_CLASS | TP_FINAL,
};

static void init_array_type(void)
{
    type_ready(&array_type);
    type_show(&array_type);
}

INIT_FUNC_LEVEL_1(init_array_type);

#ifdef __cplusplus
}
#endif
