/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

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
    INIT_OBJECT_HEAD(global, &global_type);
    global->name = name;
    global->index = index;
    global->module = m;
    return (Object *)global;
}

TypeObject global_type = {
    ._type = &type_type,
    .name = "global",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC,
};

/*
Get the type of a value. For reference values, return the type of the referenced object.
For primitive values, return the corresponding type based on the tag.
*/
TypeObject *kl_typeof(TValue *val)
{
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

TypeObject *kl_new_type(char *name, int flags)
{
    TypeObject *tp = mm_alloc_obj(tp);
    tp->_type = &type_type;
    tp->name = name;
    tp->flags = flags;
    vector_init_ptr(&tp->fields);
    vector_init_ptr(&tp->methods);
    vector_init(&tp->itables, sizeof(IntfTable));
    stbl_init(&tp->members);
    return tp;
}

Object *kl_new_instance(struct _TypeObject *tp)
{
    if (tp->alloc) {
        return tp->alloc(tp);
    }

    size_t nfields = vector_size(&tp->fields);
    int msize = sizeof(InstObject) + sizeof(TValue) * nfields;
    Object *obj = mm_alloc(msize);
    INIT_OBJECT_HEAD(obj, tp);
    InstObject *inst = (InstObject *)obj;
    inst->size = nfields;
    return obj;
}

/*---------------------------------------------------------------------------+
 |  Type(meta) type definition                                               |
 +---------------------------------------------------------------------------*/

static TValue type_str(TValue *self, TValue *args, int nargs)
{
    TypeObject *tp = kl_typeof(self);
    const char *s = kl_mo_path(tp->module);
    Object *ret;
    if (str_equal(s, "std/builtin")) {
        ret = kl_new_fmt_str("<class '%s'>", tp->name);
    } else {
        ret = kl_new_fmt_str("<class '%s.%s'>", s, tp->name);
    }
    return obj_value(ret);
}

static MethodDef type_methods[] = {
    { "__str__", type_str },
    { NULL },
};

TypeObject type_type = {
    ._type = &type_type,
    .name = "type",
    .flags = TP_FLAGS_CLASS | TP_FLAGS_PUBLIC,
    .methdefs = type_methods,
};

/*---------------------------------------------------------------------------+
 |  Slot definition                                                          |
 +---------------------------------------------------------------------------*/

#define slots(idx) tp->slots[idx]

static unsigned int slot_tp_hash(TValue *self)
{
    TypeObject *tp = kl_typeof(self);
    Object *fn = slots(SLOT_HASH);
    if (IS_CFUNC(fn)) {
        CFuncObject *cfn = (CFuncObject *)fn;
        TValue val = cfn->func(self, NULL, 0);
        return to_int64(&val);
    } else {
        TValue val = obj_value(fn);
        val = kl_do_call_one_arg(&val, self);
        return to_int64(&val);
    }
}

static TValue slot_tp_richcmp(TValue *self, TValue *other, int op)
{
    TypeObject *tp = kl_typeof(self);
    Object *fn = slots(SLOT_EQ + op);
    if (IS_CFUNC(fn)) {
        CFuncObject *cfn = (CFuncObject *)fn;
        return cfn->func(self, other, 1);
    } else {
        TValue val = obj_value(fn);
        TValue args[] = { *self, *other };
        return kl_do_call(&val, args, 2);
    }
}

static TValue slot_tp_str(TValue *self)
{
    TypeObject *tp = kl_typeof(self);
    Object *fn = slots(SLOT_STR);
    if (IS_CFUNC(fn)) {
        CFuncObject *cfn = (CFuncObject *)fn;
        return cfn->func(self, NULL, 0);
    } else {
        TValue val = obj_value(fn);
        return kl_do_call_one_arg(&val, self);
    }
}

static TValue slot_tp_call(TValue *self, TValue *args, int nargs)
{
    TypeObject *tp = kl_typeof(self);
    Object *fn = slots(SLOT_CALL);
    if (IS_CFUNC(fn)) {
        CFuncObject *cfn = (CFuncObject *)fn;
        return cfn->func(self, args, nargs);
    } else {
        TValue val = obj_value(fn);
        return kl_do_call(&val, args, nargs);
    }
}

typedef struct _SlotDef {
    char *name;
    int offset;
    void *func;
    SlotId id;
} SlotDef;

#define TPSLOT(NAME, SLOT, FUNC, ID) { NAME, offsetof(TypeObject, SLOT), (void *)(FUNC), ID }

static SlotDef slotdefs[] = {
    TPSLOT("__hash__", hash, slot_tp_hash, SLOT_HASH),
    TPSLOT("__eq__", cmp, slot_tp_richcmp, SLOT_EQ),
    TPSLOT("__ne__", cmp, slot_tp_richcmp, SLOT_NE),
    TPSLOT("__lt__", cmp, slot_tp_richcmp, SLOT_LT),
    TPSLOT("__le__", cmp, slot_tp_richcmp, SLOT_LE),
    TPSLOT("__gt__", cmp, slot_tp_richcmp, SLOT_GT),
    TPSLOT("__ge__", cmp, slot_tp_richcmp, SLOT_GE),
    TPSLOT("__str__", str, slot_tp_str, SLOT_STR),
    TPSLOT("__call__", call, slot_tp_call, SLOT_CALL),
    { NULL },
};

/*---------------------------------------------------------------------------+
 |  type init core implementation                                            |
 +---------------------------------------------------------------------------*/

int kl_init_type(TypeObject *tp)
{
    if (!tp || tp->flags & TP_FLAGS_READY) return 0;

    Object *_m = tp->module;
    ModuleObject *m = (ModuleObject *)_m;

    // initialization some fields
    vector_init_ptr(&tp->fields);
    vector_init_ptr(&tp->methods);
    vector_init(&tp->itables, sizeof(IntfTable));
    stbl_init(&tp->members);

    // initialize slots[]
    memset(tp->slots, 0, sizeof(tp->slots));

    // add method to type
    Object *cfunc;
    MethodDef *def = tp->methdefs;
    while (def && def->name) {
        if (def->cfunc) {
            cfunc = kl_new_cfunc(def->name, def->cfunc, (Object *)tp);
            kl_bind_func(_m, cfunc);
            log_info("added method '%s' to class/trait '%s'", def->name, tp->name);
        } else {
            cfunc = m->not_impl;
            log_info("added method '%s'(not_impl) to class/trait '%s'", def->name, tp->name);
        }
        vector_push_back(&tp->methods, &cfunc);
        stbl_add_obj(&tp->members, def->name, cfunc);

        // bind to slots[]
        for (SlotDef *slot = slotdefs; slot->name; slot++) {
            if (str_equal(def->name, slot->name)) {
                log_info("binding method '%s' to slots[%d] of class/trait '%s'", def->name,
                         slot->id, tp->name);

                tp->slots[slot->id] = cfunc;

                void **field = (void **)((char *)tp + slot->offset);
                // if the type has implemented this slot function, do not override it.
                if (*field == NULL) *field = slot->func;
            }
        }

        ++def;
    }

    // add member to type
    MemberDef *mdef = tp->membdefs;
    while (mdef && mdef->name) {
        Object *field = kl_new_field(mdef->name, mdef->type, mdef->offset, (Object *)tp);
        vector_push_back(&tp->fields, &field);
        stbl_add_obj(&tp->members, mdef->name, field);
        log_info("added field '%s' to class/trait '%s'", mdef->name, tp->name);
        ++mdef;
    }

    tp->flags |= TP_FLAGS_READY;
    return 0;
}

#ifdef __cplusplus
}
#endif
