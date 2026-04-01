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
        &none_type,  &exc_type,   &bool_type,  &int_type,   &int_type,
        &int_type,   &int_type,   &int_type,   &int_type,   &int_type,
        &float_type, &float_type, &float_type, &float_type,
    };

    ASSERT(tag < COUNT_OF(_types_mapping));
    return _types_mapping[tag];
}

/*
Get the type of a value. For reference values, return the type of the referenced object.
For primitive values, return the corresponding type based on the tag.
*/
TypeObject *kl_typeof(TValue *val)
{
    if (is_ref(val)) {
        Object *obj = to_ref(val);
        return OB_TYPE(obj);
    }
    return _value_typeof(val->tag);
}

/*---------------------------------------------------------------------------+
 |  Type(meta) type definition                                               |
 +---------------------------------------------------------------------------*/

static TValue type_str(TValue *self, TValue *args, int nargs)
{
    TypeObject *tp = kl_typeof(self);
    const char *s = kl_mo_path(tp->module);
    Object *ret;
    if (str_eq(s, "std/builtin")) {
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
    .flags = TP_FLAGS_CLASS,
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
    TValue val = obj_value(fn);
    val = kl_do_call_no_arg(&val);
    return to_int64(&val);
}

static TValue slot_tp_richcmp(TValue *self, TValue *other, int op)
{
    TypeObject *tp = kl_typeof(self);
    Object *fn = slots(SLOT_EQ + op);
    TValue val = obj_value(fn);
    TValue args[] = { *self, *other };
    return kl_do_call(&val, args, 2);
}

static TValue slot_tp_str(TValue *self)
{
    TypeObject *tp = kl_typeof(self);
    Object *fn = slots(SLOT_STR);
    TValue val = obj_value(fn);
    return kl_do_call_no_arg(&val);
}

static TValue slot_tp_call(TValue *self, TValue *args, int nargs)
{
    TypeObject *tp = kl_typeof(self);
    Object *fn = slots(SLOT_CALL);
    TValue val = obj_value(fn);
    return kl_do_call(&val, args, nargs);
}

typedef struct _SlotDef {
    char *name;
    int offset;
    void *func;
    SlotId id;
} SlotDef;

#define TPSLOT(NAME, SLOT, FUNC, ID) \
    { NAME, offsetof(TypeObject, SLOT), (void *)(FUNC), ID }

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
    Object *_m = tp->module;
    ModuleObject *m = (ModuleObject *)_m;

    // initialization some fields
    vector_init_ptr(&tp->fields);
    vector_init_ptr(&tp->methods);
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
            log_info("added method '%s'(not_impl) to class/trait '%s'", def->name,
                     tp->name);
        }
        vector_push_back(&tp->methods, &cfunc);
        stbl_add_obj(&tp->members, def->name, cfunc);

        // bind to slots[]
        for (SlotDef *slot = slotdefs; slot->name; slot++) {
            if (str_eq(def->name, slot->name)) {
                log_info("binding method '%s' to slots[%d] of class/trait '%s'",
                         def->name, slot->id, tp->name);

                tp->slots[slot->id] = cfunc;

                void **field = (void **)((char *)tp + slot->offset);
                // if the type has implemented this slot function, do not override it.
                if (*field == NULL) *field = slot->func;
            }
        }

        ++def;
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
