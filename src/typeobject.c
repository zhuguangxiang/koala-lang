/*===-- typeobject.c - Type Object --------------------------------*- C -*-===*\
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) 2020 James, https://github.com/zhuguangxiang                 *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This file implements the Koala 'TypeObject' object.                        *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "fieldobject.h"
#include "gc.h"
#include "methodobject.h"
#include "mm.h"
#include <stdio.h>

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
    struct mnode *n = mm_alloc(sizeof(*node));
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

int type_add_fielddef(TypeObject *type, FieldDef *def)
{
    Object *obj = field_new(def->name);
    return mtbl_add(type->mtbl, def->name, obj);
}

int type_add_methdef(TypeObject *type, MethodDef *def)
{
    Object *obj = cmethod_new(def);
    return mtbl_add(type->mtbl, def->name, obj);
}

/*------------ `Type` object ------------*/

/* type's type */
static TypeObject *type_type;

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

void init_type_type(void)
{
    TypeObject *type = type_new("Type");

    /* reset ->type as self */
    GcObject *gcobj = (GcObject *)type - 1;
    gcobj->type = type;

    /* set `Type` as final class */
    type->flags = TP_FLAGS_CLASS + TP_FLAGS_FINAL;

    /* set global variable `type_type` */
    type_type = type;
}
