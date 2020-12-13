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
#include "gc.h"
#include "methodobject.h"
#include "mm.h"
#include <stdio.h>

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

/* meta table */
struct kl_mtbl_t {
    HashMap map;
};

kl_mtbl_t *mtbl_create(void)
{
    kl_mtbl_t *mtbl = mm_alloc(sizeof(*mtbl));
    hashmap_init(&mtbl->map, mnode_equal);
    return mtbl;
}

int mtbl_add(kl_mtbl_t *mtbl, const char *name, Object *obj)
{
    struct mnode *n = mnode_new(name, obj);
    int ret = hashmap_put_absent(&mtbl->map, n);
    if (ret) { printf("error: duplicated object `%s` in meta-table\n", name); }
    return ret;
}

Object *mtbl_remove(kl_mtbl_t *mtbl, const char *name)
{
    Object *obj = NULL;
    struct mnode key = { .name = name };
    struct mnode *old = hashmap_remove(&mtbl->map, &key);
    if (old) {
        obj = old->obj;
        mnode_free(old, NULL);
    }
    else {
        printf("warn:  `%s` not exist.\n", name);
    }
    return obj;
}

void mtbl_show(kl_mtbl_t *mtbl)
{
    if (!mtbl) return;
    hashmap_visit(&mtbl->map, mnode_show, NULL);
}

int type_add_fielddef(TypeObject *type, FieldDef *def)
{
    Object *obj = NULL; // field_new(def->name);
    return mtbl_add(type->mtbl, def->name, obj);
}

int type_add_methdef(TypeObject *type, MethodDef *def)
{
    Object *obj = NULL; // cmethod_new(def);
    return mtbl_add(type->mtbl, def->name, obj);
}

/* `Type` type */
static TypeObject *type_type;

/* `Any` type */
static TypeObject *any_type;

TypeObject *type_new(const char *name)
{
    GcObject *gcobj = mm_alloc(sizeof(GcObject) + sizeof(TypeObject));
    gcobj->type = type_type;
    TypeObject *type = (TypeObject *)(gcobj + 1);
    type->name = name;
    vector_init(&type->lro, sizeof(TypeObject *));
    type->mtbl = mtbl_create();
    return type;
}

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

static void lro_build_one(TypeObject *type, TypeObject *base)
{
    Vector *vec = &type->lro;

    TypeObject *item;
    int idx;
    vector_foreach(item, idx, &base->lro)
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

void type_ready(TypeObject *type)
{
    /* trait must not be final */
    if (is_trait(type)) assert(!is_final(type));

    /* module, enumeration must be final */
    if (is_mod(type) || is_enum(type)) assert(is_final(type));

    /* build lro */
    lro_build(type);

    /* build virtual table */
    vtbl_build(type);
}

void type_set_type(TypeObject *type)
{
    GcObject *gcobj = (GcObject *)type - 1;
    gcobj->type = type_type;
}

void type_show(TypeObject *type)
{
    printf("%s(%p):\n", type->name, type);

    int cnt;
    char buf[64];
    if (is_class(type))
        cnt = sprintf(buf, "TP_FLAGS_CLASS");
    else if (is_trait(type))
        cnt = sprintf(buf, "TP_FLAGS_TRAIT");
    else if (is_enum(type))
        cnt = sprintf(buf, "TP_FLAGS_ENUM");
    else if (is_mod(type))
        cnt = sprintf(buf, "TP_FLAGS_MOD");
    else
        assert(0);

    if (is_pub(type)) cnt += sprintf(buf + cnt, " | TP_FLAGS_PUB");
    if (is_final(type)) cnt += sprintf(buf + cnt, " | TP_FLAGS_FINAL");

    printf("flags:%s\n", buf);

    GcObject *gcobj = (GcObject *)type - 1;

    printf("->type: %p\n", (TypeObject *)gcobj->type);
    printf("->vtbl: %p\n", (TypeObject *)gcobj->vtbl);

    /* show lro */
    TypeObject *item;
    int idx;
    int vecsize = vector_size(&type->lro);
    vector_foreach(item, idx, &type->lro)
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
    TypeObject *type = type_new("Any");

    /* set `Any` as public trait */
    type->flags = TP_FLAGS_TRAIT | TP_FLAGS_PUB;

    /* add `Method` of `Any` */
    MethodDef defs[] = {
        { "__hash__", "i", NULL, "kl_type_name" },
        { "__eq__", "A", "b", "" },
        { "__type__", NULL, "L.Type;", "" },
        { "__str__", NULL, "s", "" },
        { NULL },
    };
    type_add_methdefs(type, defs);

    /* set global variable `any_type` */
    any_type = type;

    /* any_type ready */
    type_ready(type);
}

void init_type_type(void)
{
    init_any_type();

    TypeObject *type = type_new("Type");

    /* reset ->type as self */
    GcObject *gcobj = (GcObject *)type - 1;
    gcobj->type = type;

    /* set `Type` as public final class */
    type->flags = TP_FLAGS_CLASS | TP_FLAGS_PUB | TP_FLAGS_FINAL;

    /* add `Method` of `Type` */
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
    type_add_methdefs(type, defs);

    /* set global variable `type_type` */
    type_type = type;

    /* type_type ready */
    type_ready(type);

    /* update `any_type` */
    type_set_type(any_type);

    /* show `Any` and `Type` */
    type_show(any_type);
    type_show(type_type);
}

#ifdef __cplusplus
}
#endif
