/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

// clang-format off

/* 'Class' type */
TypeInfo class_type = {
    .name = "Type",
    .flags = TF_CLASS | TF_PUB | TF_FINAL,
};

/*
static MethodDef class_methods[] = {
    { "get_name",   NULL, "s",                class_get_name   },
    { "get_module", NULL, "LModule;",         class_get_module },
    { "get_lro",    NULL, "LArray(LClass;);", class_get_lro    },
    { "get_mbrs",   NULL, "LArray(A);",       class_get_mbrs   },
    { "get_field",  "s",  "LField;",          class_get_field  },
    { "get_method", "s",  "LMethod;",         class_get_method },
    { "to_string",  NULL, "s",                class_to_string  },
    { NULL }
};
*/

/* `Field` type */
TypeInfo field_type = {
    .name = "Field",
    .flags = TF_CLASS | TF_PUB | TF_FINAL,
};

/*
static MethodDef field_methods[] = {
    { "set",       "AA", NULL, field_set },
    { "get",       "A",  "A",  field_get },
    { "to_string", NULL, "s",  field_to_string },
    { NULL }
};
*/

/* `Method` type */
TypeInfo method_type = {
    .name = "Method",
    .flags = TF_CLASS | TF_PUB | TF_FINAL,
};

/*
static MethodDef method_methods[] = {
    { "invoke",    "A...", NULL, method_invoke    },
    { "to_string", NULL,   "s",  method_to_string },
    { NULL }
};
*/

/* `Any` type */
TypeInfo any_type = {
    .name  = "Any",
    .flags = TF_TRAIT | TF_PUB,
};

int32 any_hash_code(ObjectRef self)
{
    return (int32)mem_hash(&self, sizeof(void *));
}

int8 any_equals(ObjectRef self, ObjectRef other)
{
    if (self->type != other->type) return 0;
    return self == other;
}

ObjectRef any_get_class(ObjectRef self)
{
    return NULL;
}

ObjectRef any_to_string(ObjectRef self)
{
    char buf[64];
    TypeInfo *type = self->type;
    snprintf(buf, sizeof(buf) - 1, "%.32s@%x", type->name, PTR2INT(self));
    return NULL;
}

/* DON'T change the order */
static MethodDef any_methods[] = {
    { "__hash__", NULL, "i32",     any_hash  },
    { "__eq__",   "A",  "b",       any_equal },
    { "__type__", NULL, "LClass;", any_type  },
    { "__str__",  NULL, "s",       any_tostr },
    { NULL }
};

// clang-format on

#ifdef __cplusplus
}
#endif
