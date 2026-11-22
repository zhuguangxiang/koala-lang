/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "gc.h"
#include "listobj.h"
#include "log.h"
#include "modobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static TypeObject *_value_typeof(int tag)
{
    static TypeObject *_types_mapping[] = {
        [TAG_NIL] = &nil_type,         [TAG_ERROR] = &exc_type,       [TAG_BOOL] = &bool_type,
        [TAG_INT8] = &int8_type,       [TAG_INT16] = &int16_type,     [TAG_INT32] = &int32_type,
        [TAG_INT64] = &int64_type,     [TAG_UINT8] = &uint8_type,     [TAG_UINT16] = &uint16_type,
        [TAG_UINT32] = &uint32_type,   [TAG_UINT64] = &uint64_type,   [TAG_FLOAT16] = &float16_type,
        [TAG_FLOAT32] = &float32_type, [TAG_FLOAT64] = &float64_type,
    };

    ASSERT(tag < COUNT_OF(_types_mapping));
    return _types_mapping[tag];
}

Object *kl_new_global(char *name, int index, Object *m)
{
    GlobalObject *global = gc_alloc_perm_obj(global, 0);
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

/*
 * Find the itable of `tp` that implements the trait `name`. The name is the bare
 * trait name, exactly as write_meta_intf_entry() stores it (KlcIntfEntry carries no
 * module). `name` need not be 0-terminated: constant-pool strings are not.
 *
 * tp->itables lists every trait the type implements, inherited ones included, so
 * this also finds a parent trait. Returns NULL when the trait is not implemented.
 */
IntfTable *kl_tp_find_itab(TypeObject *tp, char *name, size_t len)
{
    IntfTable *item;
    vector_foreach_ptr(item, &tp->itables) {
        if (!item) continue;
        /* itab->name is a 0-terminated atom, `name` may not be */
        if (strlen(item->name) != len) continue;
        if (memcmp(item->name, name, len) != 0) continue;
        return item;
    }
    return NULL;
}

Object *kl_type_find(TypeObject *tp, char *name)
{
    Object *obj = stbl_find_obj(&tp->members, name);
    if (!obj) {
        log_error("type '%s' has no member named '%s'", tp->name, name);
    }
    return obj;
}

static void instance_mark(Object *obj)
{
    InstObject *inst = (InstObject *)obj;
    for (size_t i = 0; i < inst->_nfields; i++) {
        gc_mark_value(&inst->fields[i]);
    }
}

/* static types only; heap types (TP_FLAGS_HEAP) are released by the gc hook */
static Vector all_static_types;

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

    if (!(tp->flags & TP_FLAGS_HEAP)) {
        if (!all_static_types.obj_size) vector_init_ptr(&all_static_types);
        vector_push_back(&all_static_types, &tp);
    }
}

TypeObject *kl_new_type(char *name, int flags)
{
    TypeObject *tp = gc_alloc_perm_obj(tp, 0);
    INIT_OBJECT_HEAD(tp, &type_type, 0);
    tp->name = name;
    tp->flags = flags | TP_FLAGS_HEAP;
    // add gc mark
    tp->_mark = instance_mark;
    kl_init_type(tp);
    return tp;
}

Object *kl_new_instance(struct _TypeObject *tp)
{
    // 1. If the type has a custom allocation function, use it.
    if (tp->_alloc) return tp->_alloc(tp);

    // 2. Calculate the memory size for the instance based on objsize or number of fields.
    size_t nfields = vector_size(&tp->fields);

    size_t msize;

    if (tp->objsize > 0) {
        // 2.1 If objsize is specified, use it as the memory size for the instance.
        msize = tp->objsize;
    } else {
        // 2.2 Otherwise, calculate the memory size based on the number of fields.
        msize = sizeof(InstObject) + sizeof(TValue) * nfields;
    }

    Object *obj = gc_alloc(msize);
    INIT_OBJECT_HEAD(obj, tp, nfields);
    return obj;
}

void kl_free_instance(Object *obj)
{
    /* run the finalizer first: it releases native resources (fds, child processes) */
    FreeFunc fini = OB_TYPE(obj)->_free;
    if (fini) fini(obj);
    mm_free(obj);
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

static TValue _type_fmt(TValue *self, TValue *args, int nargs)
{
    TypeObject *tp = SELF_AS(type_type);

    ASSERT(nargs == 1);
    Formatter *fmt = kl_arg_obj_as(0, fmt_type);

    const char *s = kl_mo_path(tp->module);

    if (str_equal(s, "std/builtin")) {
        buf_write_str(&fmt->buf, "<class '");
        buf_write_str(&fmt->buf, tp->name);
        buf_write_str(&fmt->buf, "'>");
    } else {
        buf_write_str(&fmt->buf, "<class '");
        buf_write_str(&fmt->buf, s);
        buf_write_str(&fmt->buf, ".");
        buf_write_str(&fmt->buf, tp->name);
        buf_write_str(&fmt->buf, "'>");
    }

    return no_value;
}

static TValue _type_to_str(TValue *self, TValue *args, int nargs)
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

    TValue *items = VECTOR_ITEMS(&vec, TValue);
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

    TValue *items = VECTOR_ITEMS(&vec, TValue);
    int size = vector_size(&vec);
    Object *tobj = kl_list_from_array(items, size);
    TValue val = obj_value(tobj);
    vector_fini(&vec);
    return val;
}

static MethodDef type_methods[] = {
    { "to_str", _type_to_str },   { "fmt", _type_fmt }, { "name", _type_name },
    { "methods", _type_methods }, { "lro", _type_lro }, { NULL },
};

/* idempotent: zeroes the owned fields so a second pass is a no-op */
static void type_clear(TypeObject *tp)
{
    IntfTable *itab;
    vector_foreach_ptr(itab, &tp->itables) {
        mm_free(itab->methods);
        mm_free(itab->parents);
    }
    vector_fini(&tp->itables);
    vector_fini(&tp->fields);
    vector_fini(&tp->methods);
    stbl_fini(&tp->members);
    memset(&tp->members, 0, sizeof(tp->members));
    mm_free(tp->slots);
    tp->slots = NULL;
}

static void type_free_hook(Object *ob) { type_clear((TypeObject *)ob); }

/* static types are never gc-freed; clean every registered type before gc_fini */
void kl_fini_static_types(void)
{
    TypeObject *tp;
    vector_foreach(tp, &all_static_types) {
        type_clear(tp);
    }
    vector_fini(&all_static_types);
}

TypeObject type_type = {
    ._type = &type_type,
    .name = "type",
    .flags = TP_FLAGS_CLASS,
    .methdefs = type_methods,
    ._free = type_free_hook,
};

#ifdef __cplusplus
}
#endif
