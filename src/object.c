/*===-- typeobject.c - Type Object --------------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This file implements the Koala `Any` and `Type` object.                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "object.h"
#include <dlfcn.h>

#ifdef __cplusplus
extern "C" {
#endif

/* object in meta table */
struct mnode {
    /* entry need at top */
    HashMapEntry entry;
    /* object name */
    const char *name;
    /* object pointer */
    void *obj;
};

static int mnode_equal(void *e1, void *e2)
{
    struct mnode *n1 = e1;
    struct mnode *n2 = e2;
    return (n1 == n2) || !strcmp(n1->name, n2->name);
}

static struct mnode *mnode_new(const char *name, void *obj)
{
    struct mnode *n = mm_alloc(sizeof(*n));
    n->name = name;
    hashmap_entry_init(n, strhash(name));
    n->obj = obj;
    return n;
}

static void mnode_free(void *e, void *arg)
{
    struct mnode *n = e;
    printf("debug: `%s` is freed.", n->name);
    mm_free(n);
}

static void mnode_show(void *e, void *arg)
{
    struct mnode *n = e;
    printf("%s(%p)\n", n->name, n->obj);
}

static void mnode_free(void *e, void *arg)
{
    struct mnode *n = e;
    printf("free: %s\n", n->name);
}

HashMap *mtbl_create(void)
{
    HashMap *mtbl = mm_alloc(sizeof(*mtbl));
    hashmap_init(mtbl, mnode_equal);
    return mtbl;
}

int mtbl_add(HashMap *mtbl, const char *name, void *obj)
{
    struct mnode *n = mnode_new(name, obj);
    int ret = hashmap_put_absent(mtbl, n);
    if (ret) { printf("error: duplicated object `%s` in meta-table\n", name); }
    return ret;
}

void *mtbl_find(HashMap *mtbl, const char *name)
{
    struct mnode key = { .name = name };
    hashmap_entry_init(&key, strhash(name));
    struct mnode *find = hashmap_get(mtbl, &key);
    if (find) { return find->obj; }
    return NULL;
}

void mtbl_destroy(HashMap *mtbl)
{
    hashmap_fini(mtbl, mnode_free, NULL);
}

void mtbl_show(HashMap *mtbl)
{
    if (!mtbl) return;
    hashmap_visit(mtbl, mnode_show, NULL);
}

/* `Type` type */
TypeObject *type_type;
/* `Any` type */
TypeObject *any_type;

struct lro_info {
    TypeObject *type;
    Vector *mro;
};

static inline HashMap *__get_mtbl(TypeObject *type)
{
    HashMap *mtbl = type->mtbl;
    if (!mtbl) {
        mtbl = mtbl_create();
        type->mtbl = mtbl;
    }
    return mtbl;
}

static int lro_find(Vector *vec, struct lro_info *lro)
{
    struct lro_info *item;
    vector_foreach(item, vec)
    {
        if (item->type == lro->type) return 1;
    }
    return 0;
}

static inline Vector *__get_lro(TypeObject *type)
{
    Vector *vec = type->lro;
    if (!vec) {
        vec = vector_new(sizeof(struct lro_info));
        type->lro = vec;
    }
    return vec;
}

static void lro_build_one(TypeObject *type, TypeObject *base)
{
    Vector *vec = __get_lro(type);

    struct lro_info lro = { NULL, NULL };
    struct lro_info *item;
    vector_foreach(item, base->lro)
    {
        if (!lro_find(vec, item)) {
            lro.type = item->type;
            vector_push_back(vec, &lro);
        }
    }

    lro.type = base;
    if (!lro_find(vec, &lro)) { vector_push_back(vec, &lro); }
}

static void lro_build(TypeObject *type)
{
    /* add any type */
    lro_build_one(type, any_type);

    /* add first base type */
    if (type->base) lro_build_one(type, type->base);

    /* add trait types */
    TypeObject **item;
    vector_foreach(item, type->traits)
    {
        lro_build_one(type, *item);
    }

    /* add self type */
    lro_build_one(type, type);
}

static void mro_build_one(Vector *vec, TypeObject *self)
{
    if (vector_empty(vec)) return;

    Object *find;
    MethodObject **mobj;
    vector_foreach(mobj, vec)
    {
        find = mtbl_find(self->mtbl, (*mobj)->name);
        assert(find);
        vector_push_back(self->mro, &find);
    }
}

static void mro_build(TypeObject *type)
{
    /* new type->mro */
    Vector *mro = type->mro;
    if (!mro) {
        mro = vector_new(sizeof(void *));
        type->mro = mro;
    }

    /* iterate lro types */
    struct lro_info *item;
    vector_foreach(item, type->lro)
    {
        mro_build_one(item->type->methods, type);
    }
}

static void lro_build_inherit(TypeObject *type)
{
    struct lro_info *item;
    vector_foreach_reverse(item, type->lro)
    {
        if (item->type == type) continue;

        /* method */
        MethodObject **mobj;
        vector_foreach(mobj, item->type->methods)
        {
            mtbl_add(__get_mtbl(type), (*mobj)->name, (Object *)*mobj);
        }
        /* fields */
    }
}

static void lro_update_base_mro(struct lro_info *item, TypeObject *self)
{
    TypeObject *trait = item->type;
    Vector *mro = vector_new(sizeof(void *));
    Object *obj;
    MethodObject **mobj;
    vector_foreach(mobj, trait->mro)
    {
        obj = mtbl_find(self->mtbl, (*mobj)->name);
        assert(obj);
        vector_push_back(mro, &obj);
    }
    assert(!item->mro);
    item->mro = mro;
}

static void lro_update_mro(TypeObject *type)
{
    Vector *vec = type->lro;
    int len = vector_size(vec);
    struct lro_info *item;

    /* `Any` */
    item = vector_get_ptr(vec, 0);
    item->mro = type->mro;

    /* first super type(class or trait) */
    TypeObject *base = type->base;
    while (base) {
        struct lro_info *item;
        vector_foreach(item, vec)
        {
            if (item->type == base) {
                item->mro = type->mro;
                break;
            }
        }
        base = base->base;
    }

    /* `Self` */
    item = vector_get_ptr(vec, len - 1);
    item->mro = type->mro;

    /* other super type(tratis) */
    vector_foreach(item, vec)
    {
        if (item->mro) continue;
        lro_update_base_mro(item, type);
    }
}

void *__alloc_meta_object(int size)
{
    void **obj = mm_alloc(sizeof(void *) + size);
    *obj = type_type;
    return (void *)(obj + 1);
}

void type_ready(TypeObject *type)
{
    /* trait must not be final */
    if (type_is_trait(type)) assert(!type_is_final(type));

    /* module, enumeration must be final */
    if (type_is_mod(type) || type_is_enum(type)) assert(type_is_final(type));

    /* build lro */
    lro_build(type);

    /* build inheritance */
    lro_build_inherit(type);

    /* build mro */
    mro_build(type);

    /* update lro with mro */
    lro_update_mro(type);
}

void type_show(TypeObject *type)
{
    printf("%s(%p):\n", type->name, type);

    int cnt;
    char buf[64];
    if (type_is_class(type))
        cnt = sprintf(buf, "TP_FLAGS_CLASS");
    else if (type_is_trait(type))
        cnt = sprintf(buf, "TP_FLAGS_TRAIT");
    else if (type_is_enum(type))
        cnt = sprintf(buf, "TP_FLAGS_ENUM");
    else if (type_is_mod(type))
        cnt = sprintf(buf, "TP_FLAGS_MOD");
    else
        assert(0);

    if (type_is_pub(type)) cnt += sprintf(buf + cnt, " | TP_FLAGS_PUB");
    if (type_is_final(type)) cnt += sprintf(buf + cnt, " | TP_FLAGS_FINAL");

    printf("flags: %s\n", buf);

    printf("->type: %p\n", obj_get_type(type));
    printf("->vtbl: %p\n", type->vtbl);

    /* show lro */
    printf("lro:\n");
    struct lro_info *item;
    vector_foreach(item, type->lro)
    {
        if (i < len - 1)
            printf("%s(%p) <- ", item->type->name, item->mro);
        else
            printf("%s(%p)", item->type->name, item->mro);
    }
    printf("\n");

    /* show lro methods */
    printf("mro:%p\n", type->mro);
    MethodObject **mobj;
    vector_foreach(mobj, type->mro)
    {
        printf("%s(%p)\n", (*mobj)->name, *mobj);
    }

    /* show meta table */
    printf("mbrs:\n");
    mtbl_show(type->mtbl);
    printf("------============------\n");
}

void type_append_base(TypeObject *type, TypeObject *base)
{
    if (!type->base) {
        type->base = base;
        return;
    }

    Vector *vec = type->traits;
    if (!vec) {
        vec = vector_new(sizeof(TypeObject *));
        type->traits = vec;
    }

    vector_push_back(vec, &base);
}

int type_add_fielddef(TypeObject *type, FieldDef *def)
{
    Object *obj = NULL; // field_new(def->name);
    return mtbl_add(__get_mtbl(type), def->name, obj);
}

int type_add_methdef(TypeObject *type, MethodDef *def)
{
    Object *obj = cmethod_new(def);
    Vector *vec = type->methods;
    if (!vec) {
        vec = vector_new(sizeof(void *));
        type->methods = vec;
    }
    vector_push_back(vec, &obj);
    return mtbl_add(__get_mtbl(type), def->name, obj);
}

/* `Field` type */
TypeObject *field_type;

static void init_field_type(void)
{
    /* `Field` methods */
    MethodDef defs[] = {
        { "__str__", NULL, "s", "kl_type_name" },
        { NULL },
    };
    type_add_methdefs(type_type, defs);

    /* field_type ready */
    type_ready(field_type);

    /* show field_type */
    type_show(field_type);
}

Object *field_new(const char *name)
{
    return NULL;
}

/* `Method` type */
TypeObject *method_type;

static void init_method_type(void)
{
    /* `Method` methods */
    MethodDef defs[] = {
        { "__str__", NULL, "s", "kl_type_name" },
        { NULL },
    };
    type_add_methdefs(type_type, defs);

    /* method_type ready */
    type_ready(method_type);

    /* show method_type */
    type_show(method_type);
}

static inline void *__get_cfunc(const char *name)
{
    return dlsym(NULL, name);
}

Object *cmethod_new(MethodDef *def)
{
    MethodObject *mobj = alloc_meta_object(MethodObject);
    mobj->name = def->name;
    mobj->kind = CFUNC_KIND;
    mobj->ptr = __get_cfunc(def->funcname);
    assert(mobj->ptr);
    return (Object *)mobj;
}

Object *method_new(char *name, Object *code)
{
    return NULL;
}

Object *module_new(const char *path)
{
    TypeObject *type = type_new_mod(path);
    type_set_public_final(type);
    ModuleObject *mobj = mm_alloc(sizeof(ModuleObject));
    obj_set_type(mobj, type);
    mobj->size = MODULE_MIN_VALUES_SIZE;
    mobj->values = mm_alloc(MODULE_MIN_VALUES_SIZE);
    return (Object *)mobj;
}

int module_add_type(Object *obj, TypeObject *type)
{
    TypeObject *_type = obj_get_type(obj);
    if (!type_is_mod(_type)) {
        printf("error: object '%s' is not module\n", _type->name);
        abort();
    }

    printf("add type '%s' into module '%s'\n", type->name, _type->name);
    return mtbl_add(__get_mtbl(_type), type->name, type);
}

int module_add_var(Object *obj, FieldObject *field)
{
    TypeObject *_type = obj_get_type(obj);
    if (!type_is_mod(_type)) {
        printf("error: object '%s' is not module\n", _type->name);
        abort();
    }

    printf("add field '%s' into module '%s'\n", field->name, _type->name);

    ModuleObject *mobj = (ModuleObject *)obj;
    if (_type->next_offset >= mobj->size) {
        printf("expand module '%s' values' memory\n", _type->name);
        int new_size = mobj->size << 1;
        void **new_values = mm_alloc(new_size);
        memcpy(new_values, mobj->values, mobj->size);
        mm_free(mobj->values);
        mobj->values = new_values;
        mobj->size = new_size;
    }

    field->offset = _type->next_offset;
    _type->next_offset += field->size;
    return mtbl_add(__get_mtbl(_type), field->name, field);
}

int module_add_func(Object *obj, MethodObject *meth)
{
    TypeObject *_type = obj_get_type(obj);
    if (!type_is_mod(_type)) {
        printf("error: object '%s' is not module\n", _type->name);
        abort();
    }

    printf("add method '%s' into module '%s'\n", meth->name, _type->name);
    return mtbl_add(__get_mtbl(_type), meth->name, meth);
}

void module_show(Object *obj)
{
    TypeObject *_type = obj_get_type(obj);
    if (!type_is_mod(_type)) {
        printf("error: object '%s' is not module\n", _type->name);
        abort();
    }

    printf("module '%s':\n", _type->name);
    mtbl_show(_type->mtbl);
}

DLLEXPORT Object *kl_type_name(Object *self)
{
    TypeObject *type = (TypeObject *)self;
    return NULL;
}

static void init_any_type(void)
{
    /* `Any` methods */
    MethodDef defs[] = {
        { "__hash__", "i", NULL, "kl_type_name" },
        { "__eq__", "A", "b", "kl_type_name" },
        { "__type__", NULL, "L.Type;", "kl_type_name" },
        { "__str__", NULL, "s", "kl_type_name" },
        { NULL },
    };
    type_add_methdefs(any_type, defs);

    /* any_type ready */
    type_ready(any_type);

    /* show any_type */
    type_show(any_type);
}

static void init_type_type(void)
{
    /* `Type` methods */
    MethodDef defs[] = {
        { "__name__", NULL, "s", "kl_type_name" },
        { "__mod__", NULL, "L.Module;", "kl_type_name" },
        { "__lro__", NULL, "L.Tuple;", "kl_type_name" },
        { "__mbrs__", NULL, "L.Array(s);", "kl_type_name" },
        { "getField", "s", "L.Field;", "kl_type_name" },
        { "getMethod", "s", "L.Method;", "kl_type_name" },
        { "__str__", NULL, "s", "kl_type_name" },
        { NULL },
    };
    type_add_methdefs(type_type, defs);

    /* type_type ready */
    type_ready(type_type);

    /* show any_type */
    type_show(type_type);
}

void init_core_types(void)
{
    /* `Type` is public final class */
    type_type = type_new_class("Type");
    type_set_public_final(type_type);

    /* set type_type->type as self */
    obj_set_type(type_type, type_type);

    /* 'Any' is public trait */
    any_type = type_new_trait("Any");
    type_set_public(any_type);

    /* `Method` is public final class */
    method_type = type_new_class("Method");
    type_set_public_final(method_type);

    /* `Field` is public final class */
    field_type = type_new_class("Field");
    type_set_public_final(field_type);

    /* initialize `Any` type */
    init_any_type();

    /* initialize `Type` type */
    init_type_type();

    /* initialize `Method` type */
    init_method_type();

    /* initialize `Field` type */
    init_field_type();
}

#ifdef __cplusplus
}
#endif
