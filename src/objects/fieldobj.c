/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------+
 |  Field type definition                                                    |
 +---------------------------------------------------------------------------*/

static TValue field_str(TValue *self, TValue *args, int nargs)
{
    FieldObject *o = (FieldObject *)to_obj(self);
    Object *s = kl_new_fmt_str("<field '%s'>", o->name);
    return obj_value(s);
}

static MethodDef field_methods[] = {
    { "__str__", field_str },
    { NULL },
};

TypeObject field_type = {
    ._type = &type_type,
    .name = "Field",
    .flags = TP_FLAGS_CLASS,
    .methdefs = field_methods,
};

Object *kl_new_field(char *name, int type, int offset, Object *owner)
{
    FieldObject *field = mm_alloc_obj(field);
    INIT_OBJECT_HEAD(field, &field_type, 0);
    field->name = name;
    field->owner = owner;
    field->type = type;
    field->index = offset;
    return (Object *)field;
}

#ifdef __cplusplus
}
#endif
