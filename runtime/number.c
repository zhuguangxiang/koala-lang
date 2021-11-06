/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "util/hash.h"
#include "vm.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeInfo int8_type = {
    .name = "int8",
    .flags = TP_CLASS | TP_FINAL,
};

TypeInfo int16_type = {
    .name = "int16",
    .flags = TP_CLASS | TP_FINAL,
};

TypeInfo int32_type = {
    .name = "int32",
    .flags = TP_CLASS | TP_FINAL,
};

TypeInfo int64_type = {
    .name = "int64",
    .flags = TP_CLASS | TP_FINAL,
};

TypeInfo float32_type = {
    .name = "float32",
    .flags = TP_CLASS | TP_FINAL,
};

TypeInfo float64_type = {
    .name = "float64",
    .flags = TP_CLASS | TP_FINAL,
};

TypeInfo bool_type = {
    .name = "bool",
    .flags = TP_CLASS | TP_FINAL,
};

VTable *int8_vtbl;
VTable *int16_vtbl;
VTable *int32_vtbl;
VTable *int64_vtbl;
VTable *float32_vtbl;
VTable *float64_vtbl;
VTable *bool_vtbl;

static int kl_number_class(KlState *ks)
{
    // KlValue self = kl_pop_value(ks);
    // int32 hash = mem_hash(&self, sizeof(uintptr));
    // kl_push_int32(ks, hash);
    return 0;
}

static int kl_number_str(KlState *ks)
{
    // KlValue self = kl_pop_value(ks);
    // int32 hash = mem_hash(&self, sizeof(uintptr));
    // kl_push_int32(ks, hash);
    return 0;
}

void init_number_types(void)
{
    MethodDef methods[] = {
        /* clang-format off */
        { "__class__", null, "LClass;", kl_number_class },
        { "__str__",   null, "s",       kl_number_str   },
        { null },
        /* clang-format on */
    };

    type_add_methdefs(&int8_type, methods);
    type_add_methdefs(&int16_type, methods);
    type_add_methdefs(&int32_type, methods);
    type_add_methdefs(&int64_type, methods);
    // type_add_methdefs(&float32_type, methods);
    // type_add_methdefs(&float64_type, methods);
    type_add_methdefs(&bool_type, methods);

    type_ready(&int8_type);
    type_ready(&int16_type);
    type_ready(&int32_type);
    type_ready(&int64_type);
    type_ready(&float32_type);
    type_ready(&float64_type);
    type_ready(&bool_type);

    int8_vtbl = int8_type.vtbl[0];
    int16_vtbl = int16_type.vtbl[0];
    int32_vtbl = int32_type.vtbl[0];
    int64_vtbl = int64_type.vtbl[0];
    float32_vtbl = float32_type.vtbl[0];
    float64_vtbl = float64_type.vtbl[0];
    bool_vtbl = bool_type.vtbl[0];

    type_show(&int8_type);
    type_show(&int16_type);
    type_show(&int32_type);
    type_show(&int64_type);
    type_show(&float32_type);
    type_show(&float64_type);
    type_show(&bool_type);
}

INIT_FUNC_LEVEL_1(init_number_types);

#ifdef __cplusplus
}
#endif
