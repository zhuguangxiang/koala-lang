/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "fieldobject.h"

#ifdef __cplusplus
extern "C" {
#endif

TypeObject field_type = {
    OBJECT_HEAD_INIT(&type_type),
    .name = "Field",
    .call = NULL,
};

Object *kl_field_from_member(MemberDef *member)
{
    int sz = sizeof(FieldObject) + sizeof(MemberDef *);
    FieldObject *field = gc_alloc_p(sz);
    INIT_OBJECT_HEAD(field, &field_type);
    field->kind = FIELD_KIND_MEMBER;
    MemberDef **p = (MemberDef **)(field + 1);
    *p = member;
    return (Object *)field;
}

Object *kl_field_from_getset(GetSetDef *getset)
{
    int sz = sizeof(FieldObject) + sizeof(GetSetDef *);
    FieldObject *field = gc_alloc_p(sz);
    INIT_OBJECT_HEAD(field, &field_type);
    field->kind = FIELD_KIND_GETSET;
    GetSetDef **p = (GetSetDef **)(field + 1);
    *p = getset;
    return (Object *)field;
}

Object *kl_field_from_var(VarInfo *var)
{
    int sz = sizeof(FieldObject) + sizeof(VarInfo);
    FieldObject *field = gc_alloc_p(sz);
    INIT_OBJECT_HEAD(field, &field_type);
    field->kind = FIELD_KIND_VAR;
    VarInfo *p = (VarInfo *)(field + 1);
    *p = *var;
    return (Object *)field;
}

Object *kl_field_from_value(ValueInfo *val)
{
    int sz = sizeof(FieldObject) + sizeof(ValueInfo);
    FieldObject *field = gc_alloc_p(sz);
    INIT_OBJECT_HEAD(field, &field_type);
    field->kind = FIELD_KIND_VAR;
    ValueInfo *p = (ValueInfo *)(field + 1);
    *p = *val;
    return (Object *)field;
}

#ifdef __cplusplus
}
#endif
