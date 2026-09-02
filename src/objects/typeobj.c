/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "listobj.h"
#include "log.h"
#include "modobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static TypeObject *_value_typeof(int tag)
{
    static TypeObject *_types_mapping[] = {
        [TAG_NONE] = &none_type,     [TAG_ERROR] = &exc_type,     [TAG_BOOL] = &bool_type,
        [TAG_INT8] = &int_type,      [TAG_INT16] = &int_type,     [TAG_INT32] = &int_type,
        [TAG_INT64] = &int_type,     [TAG_UINT8] = &int_type,     [TAG_UINT16] = &int_type,
        [TAG_UINT32] = &int_type,    [TAG_UINT64] = &int_type,    [TAG_FLOAT16] = &float_type,
        [TAG_FLOAT32] = &float_type, [TAG_FLOAT64] = &float_type,
    };

    ASSERT(tag < COUNT_OF(_types_mapping));
    return _types_mapping[tag];
}

Object *kl_new_global(char *name, int index, Object *m)
{
    GlobalObject *global = mm_alloc_obj(global);
    INIT_OBJECT_HEAD(global, &global_type, 0);
    global->name = name;
    global->index = index;
    global->module = m;
    return (Object *)global;
}

TypeObject global_type = {
    ._type = &type_type,
    .name = "global",
    .flags = TP_FLAGS_CLASS,
};

/*
Get the type of a value. For reference values, return the type of the referenced object.
For primitive values, return the corresponding type based on the tag.
*/
TypeObject *kl_typeof(TValue *val)
{
    if (is_intf(val)) {
        IntfTable *itab = val->itab;
        return itab->tp;
    }

    if (is_ref(val)) {
        Object *obj = to_obj(val);
        return OB_TYPE(obj);
    }
    return _value_typeof(val->tag);
}

Object *kl_type_find(TypeObject *tp, char *name)
{
    Object *obj = stbl_find_obj(&tp->members, name);
    if (!obj) {
        log_error("type '%s' has no member named '%s'", tp->name, name);
    }
    return obj;
}

void kl_init_type(TypeObject *tp)
{
    if (!tp || tp->flags & TP_FLAGS_READY) return;

    vector_init(&tp->itables, sizeof(IntfTable));
    vector_init_ptr(&tp->fields);
    vector_init_ptr(&tp->methods);
    stbl_init(&tp->members);

    /* slots: allocate SLOT_MAX entries */
    tp->slots = mm_alloc(sizeof(Object *) * SLOT_MAX);
    ASSERT(tp->slots);
}

TypeObject *kl_new_type(char *name, int flags)
{
    TypeObject *tp = mm_alloc_obj(tp);
    INIT_OBJECT_HEAD(tp, &type_type, 0);
    tp->name = name;
    tp->flags = flags;
    kl_init_type(tp);
    return tp;
}

Object *kl_new_instance(struct _TypeObject *tp)
{
    size_t nfields = vector_size(&tp->fields);
    int msize = sizeof(InstObject) + sizeof(TValue) * nfields;
    msize += tp->priv_size;
    Object *obj = mm_alloc(msize);
    INIT_OBJECT_HEAD(obj, tp, nfields);
    return obj;
}

int kl_tp_add_field(TypeObject *tp, char *name, Object *field)
{
    vector_push_back(&tp->fields, &field);
    stbl_add_obj(&tp->members, name, field);
    return 0;
}

int kl_tp_add_method(TypeObject *tp, char *name, int slotid, Object *meth)
{
    if (slotid >= 0) {
        ASSERT(slotid < SLOT_MAX);
        /* Install the method into its designated slot position */
        ASSERT(tp->slots[slotid] == NULL);
        tp->slots[slotid] = meth;
    }

    /* Append regular methods */
    vector_push_back(&tp->methods, &meth);
    /* Register the method in the member table for name-based lookup */
    stbl_add_obj(&tp->members, name, meth);
    return 0;
}

/*---------------------------------------------------------------------------+
 |  Type(meta) type definition                                               |
 +---------------------------------------------------------------------------*/

static TValue _type_str(TValue *self, TValue *args, int nargs)
{
    TypeObject *tp = SELF_AS(type_type);
    const char *s = kl_mo_path(tp->module);
    Object *ret;
    if (str_equal(s, "std/builtin")) {
        ret = kl_new_fmt_str("<class '%s'>", tp->name);
    } else {
        ret = kl_new_fmt_str("<class '%s.%s'>", s, tp->name);
    }
    return obj_value(ret);
}

static TValue _type_name(TValue *self, TValue *args, int nargs)
{
    TypeObject *tp = SELF_AS(type_type);
    const char *s = kl_mo_path(tp->module);
    Object *ret;
    if (str_equal(s, "std/builtin")) {
        ret = kl_new_fmt_str("%s", tp->name);
    } else {
        ret = kl_new_fmt_str("%s.%s", s, tp->name);
    }
    return obj_value(ret);
}

static TValue _type_methods(TValue *self, TValue *args, int nargs)
{
    TypeObject *tp = SELF_AS(type_type);

    Vector vec;
    vector_init(&vec, sizeof(TValue));

    Object *ob;
    vector_foreach(ob, &tp->methods) {
        if (!ob) continue;
        char *name;
        if (IS_CFUNC(ob)) {
            CFuncObject *cfunc = (CFuncObject *)ob;
            name = cfunc->name;
        } else {
            ASSERT(IS_CODE(ob));
            CodeObject *code = (CodeObject *)ob;
            name = code->cs.name;
        }
        char *_name = strchr(name, '$');
        if (_name) name = _name + 1;
        TValue val = kl_val_str(name);
        vector_push_back(&vec, &val);
    }

    TValue *items = VECTOR_RAW(&vec, TValue);
    int size = vector_size(&vec);
    Object *tobj = kl_list_from_array(items, size);
    TValue val = obj_value(tobj);
    vector_fini(&vec);
    return val;
}

static TValue _type_lro(TValue *self, TValue *args, int nargs)
{
    TypeObject *tp = SELF_AS(type_type);

    Vector vec;
    vector_init(&vec, sizeof(TValue));

    IntfTable *itab;
    vector_foreach_ptr(itab, &tp->itables) {
        TValue val = kl_val_str(itab->name);
        vector_push_back(&vec, &val);
    }

    TValue *items = VECTOR_RAW(&vec, TValue);
    int size = vector_size(&vec);
    Object *tobj = kl_list_from_array(items, size);
    TValue val = obj_value(tobj);
    vector_fini(&vec);
    return val;
}

static MethodDef type_methods[] = {
    { "__str__", _type_str },
    { "name", _type_name },
    { "methods", _type_methods },
    { "lro", _type_lro },
    { NULL },
};

TypeObject type_type = {
    ._type = &type_type,
    .name = "type",
    .flags = TP_FLAGS_CLASS,
    .methdefs = type_methods,
};

#ifdef __cplusplus
}
#endif
