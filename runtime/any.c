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

/*
trait Any {
    func __hash__() int;
    func __cmp__(other Any) int;
    func __class__() Class;
    func __str__() string;
}
*/

TypeInfo any_type = {
    .name = "any",
    .flags = TP_TRAIT,
};

static int kl_any_hash(KlState *ks)
{
    KlValue self = kl_pop_value(ks);
    int32 hash = mem_hash(&self, sizeof(uintptr));
    kl_push_int32(ks, hash);
    return 1;
}

static int kl_any_equal(KlState *ks)
{
    KlValue self = kl_pop_value(ks);
    KlValue o = kl_pop_value(ks);
    int eq = self.value == o.value;
    kl_push_bool(ks, eq);
    return 1;
}

static int kl_any_class(KlState *ks)
{
    printf("any class called()\n");
    return 0;
}

static int kl_any_str(KlState *ks)
{
    // char buf[64];
    // TypeInfo *type = __GET_TYPE(self);
    // snprintf(buf, sizeof(buf) - 1, "%.32s@%lx", type->name, self);
    // return string_new(buf);
    printf("any str called()\n");
    return 0;
}

static void init_any_type(void)
{
    MethodDef methods[] = {
        /* DO NOT change the order */
        /* clang-format off */
        { "__hash__",  null, "i32",     kl_any_hash  },
        { "__eq__",    "A",  "b",       kl_any_equal },
        { "__class__", null, "LClass;", kl_any_class },
        { "__str__",   null, "s",       kl_any_str   },
        { null },
        /* clang-format on */
    };

    type_add_methdefs(&any_type, methods);
    type_ready(&any_type);
    type_show(&any_type);
}

INIT_FUNC_LEVEL_0(init_any_type);

#ifdef __cplusplus
}
#endif
