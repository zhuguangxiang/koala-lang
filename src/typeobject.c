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

#include "fieldobject.h"
#include "methodobject.h"

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
    Object *obj;
};

static int mnode_equal(void *e1, void *e2)
{
    struct mnode *n1 = e1;
    struct mnode *n2 = e2;
    return (n1 == n2) || !strcmp(n1->name, n2->name);
}

static struct mnode *mnode_new(const char *name, Object *obj)
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
    printf("%s\n", n->name);
}

static inline HashMap *mtbl_create(void)
{
    HashMap *mtbl = mm_alloc(sizeof(*mtbl));
    hashmap_init(mtbl, mnode_equal);
    return mtbl;
}

static int mtbl_add(HashMap *mtbl, const char *name, Object *obj)
{
    struct mnode *n = mnode_new(name, obj);
    int ret = hashmap_put_absent(mtbl, n);
    if (ret) { printf("error: duplicated object `%s` in meta-table\n", name); }
    return ret;
}

static Object *mtbl_remove(HashMap *mtbl, const char *name)
{
    Object *obj = NULL;
    struct mnode key = { .name = name };
    struct mnode *old = hashmap_remove(mtbl, &key);
    if (old) {
        obj = old->obj;
        mnode_free(old, NULL);
    }
    else {
        printf("warn:  `%s` not exist.\n", name);
    }
    return obj;
}

static void mtbl_show(HashMap *mtbl)
{
    if (!mtbl) return;
    hashmap_visit(mtbl, mnode_show, NULL);
}

/* `Type` type */
static TypeObject *type_type;

/* `Any` type */
static TypeObject *any_type;

static int lro_find(Vector *vec, TypeObject *type)
{
    TypeObject *item;
    int idx;
    vector_foreach(item, idx, vec)
    {
        if (item == type) return 1;
    }
    return 0;
}

static inline Vector *__get_lro(TypeObject *type)
{
    Vector *vec = type->lro;
    if (!vec) {
        vec = vector_new(sizeof(void *));
        type->lro = vec;
    }
    return vec;
}

static void lro_build_one(TypeObject *type, TypeObject *base)
{
    Vector *vec = __get_lro(type);

    TypeObject *item;
    int idx;
    vector_foreach(item, idx, base->lro)
    {
        if (!lro_find(vec, item)) { vector_push_back(vec, &item); }
    }

    if (!lro_find(vec, base)) { vector_push_back(vec, &base); }
}

static void lro_build(TypeObject *type)
{
    /* add any type */
    lro_build_one(type, any_type);

    /* add base types */
    TypeObject *item;
    int idx;
    vector_foreach(item, idx, type->bases)
    {
        lro_build_one(type, item);
    }

    /* add self type */
    lro_build_one(type, type);
}

static void vtbl_build(TypeObject *type)
{
}

TypeObject *__type_new(const char *name)
{
    void **obj = mm_alloc(sizeof(void **) + sizeof(TypeObject));
    *obj = type_type;
    TypeObject *type = (TypeObject *)(obj + 1);
    type->name = name;
    return type;
}

void type_ready(TypeObject *type)
{
    /* trait must not be final */
    if (type_is_trait(type)) assert(!type_is_final(type));

    /* module, enumeration must be final */
    if (type_is_mod(type) || type_is_enum(type)) assert(type_is_final(type));

    /* build lro */
    lro_build(type);

    /* build vtbl */
    vtbl_build(type);
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

    printf("flags:%s\n", buf);

    printf("->type: %p\n", obj_get_type(type));
    printf("->vtbl: %p\n", type->vtbl);

    /* show lro */
    TypeObject *item;
    int idx;
    int vecsize = vector_size(type->lro);
    vector_foreach(item, idx, type->lro)
    {
        if (idx < vecsize - 1)
            printf("%s <- ", item->name);
        else
            printf("%s", item->name);
    }
    printf("\n");

    /* show meta table */
    printf("mbrs:\n");
    mtbl_show(type->mtbl);
}

void type_append_base(TypeObject *type, TypeObject *base)
{
    Vector *vec = type->bases;
    if (!vec) {
        vec = vector_new(sizeof(TypeObject *));
        type->bases = vec;
    }

    vector_push_back(vec, &base);
}

static inline HashMap *__get_mtbl(TypeObject *type)
{
    HashMap *mtbl = type->mtbl;
    if (!mtbl) {
        mtbl = mtbl_create();
        type->mtbl = mtbl;
    }
    return mtbl;
}

int type_add_fielddef(TypeObject *type, FieldDef *def)
{
    Object *obj = NULL; // field_new(def->name);
    return mtbl_add(__get_mtbl(type), def->name, obj);
}

int type_add_methdef(TypeObject *type, MethodDef *def)
{
    Object *obj = NULL; // cmethod_new(def);
    return mtbl_add(__get_mtbl(type), def->name, obj);
}

/*
  >
  > s := "hello"
  > type := s.__type__()
  > print(type.__name__())
  > "String"
  >
 */
static Object *kl_type_name(Object *self)
{
    TypeObject *type = (TypeObject *)self;
    return NULL;
}

static void init_any_type(void)
{
    /* `Any` methods */
    MethodDef defs[] = {
        { "__hash__", "i", NULL, "kl_type_name" },
        { "__eq__", "A", "b", "" },
        { "__type__", NULL, "L.Type;", "" },
        { "__str__", NULL, "s", "" },
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

    /* initialize `Any` type */
    init_any_type();

    /* initialize `Type` type */
    init_type_type();
}

#ifdef __cplusplus
}
#endif
