/*
 * This file is part of the koala-lang project, under the MIT License.
 *
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 */

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

/* `Type` type */
TypeObjectRef type_type;
/* `Any` type */
TypeObjectRef any_type;
/* `Field` type */
TypeObjectRef field_type;
/* `Method` type */
TypeObjectRef method_type;

typedef struct _LroInfo {
    TypeObjectRef type;
    /* index of virtual table */
    int index;
} LroInfo, *LroInfoRef;

static HashMapRef mtbl_create(void)
{
    HashMapRef mtbl = mm_alloc_ptr(mtbl);
    hashmap_init(mtbl, NULL);
    return mtbl;
}

static inline HashMapRef __get_mtbl(TypeObjectRef type)
{
    HashMapRef mtbl = type->mtbl;
    if (!mtbl) {
        mtbl = mtbl_create();
        type->mtbl = mtbl;
    }
    return mtbl;
}

static int lro_find(VectorRef lro, TypeObjectRef type)
{
    LroInfoRef item;
    vector_foreach(item, lro, {
        if (item->type == type) return 1;
    });
    return 0;
}

static inline VectorRef __get_lro(TypeObjectRef type)
{
    VectorRef vec = type->lro;
    if (!vec) {
        vec = vector_create(sizeof(LroInfo));
        type->lro = vec;
    }
    return vec;
}

static void build_one_lro(TypeObjectRef type, TypeObjectRef one)
{
    if (!one) return;

    VectorRef vec = __get_lro(type);

    LroInfo lro = { NULL, -1 };
    LroInfoRef item;
    vector_foreach(item, one->lro, {
        if (!lro_find(vec, item->type)) {
            lro.type = item->type;
            vector_push_back(vec, &lro);
        }
    });

    if (!lro_find(vec, one)) {
        lro.type = one;
        vector_push_back(vec, &lro);
    }
}

static void build_lro(TypeObjectRef type)
{
    /* add Any type */
    build_one_lro(type, any_type);

    /* add first class or trait */
    build_one_lro(type, type->base);

    /* add traits */
    TypeObjectRef *trait;
    vector_foreach(trait, type->traits, { build_one_lro(type, *trait); });

    /* add self */
    build_one_lro(type, type);
}

static void inherit_methods(TypeObjectRef type)
{
    VectorRef lro = __get_lro(type);
    int size = vector_size(lro);
    HashMapRef mtbl = __get_mtbl(type);
    VectorRef vec;
    MethodObjectRef *m;
    LroInfoRef item;
    for (int i = size - 2; i >= 0; i--) {
        /* omit type self */
        item = vector_get_ptr(lro, i);
        vec = item->type->methods;
        vector_foreach(m, vec, { hashmap_put_absent(mtbl, &(*m)->entry); });
    }
}

static VectorRef __get_vtbl(TypeObjectRef type, int index)
{
    VectorRef vec = type->vtbl;
    if (!vec) {
        vec = vector_create(PTR_SIZE);
        type->vtbl = vec;
    }

    VectorRef vtbl = NULL;
    vector_get(vec, index, &vtbl);
    if (!vtbl) {
        vtbl = vector_create(PTR_SIZE);
        vector_set(vec, index, &vtbl);
    }
    return vtbl;
}

static int vtbl_same(VectorRef v1, int len1, VectorRef v2)
{
    if (vector_size(v2) != len1) return 0;

    LroInfoRef item1, item2;
    for (int i = 0; i <= len1; i++) {
        item1 = vector_get_ptr(v1, i);
        item2 = vector_get_ptr(v2, i);
        if (!item1 || !item2) return 0;
        if (item1->type != item2->type) return 0;
    }
    return 1;
}

static void build_vtbl_one(uintptr_t *slots, int index, VectorRef methods,
                           HashMapRef mtbl)
{
    MethodObjectRef *m;
    MethodObjectRef n;
    HashMapEntry *e;
    vector_foreach(m, methods, {
        n = *m;
        e = hashmap_get(mtbl, &n->entry);
        assert(e);
        n = CONTAINER_OF(e, MethodObject, entry);
        *(slots + index++) = (uintptr_t)n;
    });
}

static void build_vtbl(TypeObjectRef type)
{
    /* first class or trait is the same virtual table, at 0 */
    VectorRef lro = __get_lro(type);
    TypeObjectRef base = type;
    LroInfoRef item;
    while (base) {
        vector_foreach(item, lro, {
            if (item->type == base) {
                item->index = 0;
                break;
            }
        });
        base = base->base;
    }

    /* vtbl of 'Any' is also at 0 */
    item = vector_first_ptr(lro);
    item->index = 0;

    /* update some traits indexes */
    vector_foreach_reverse(item, lro, {
        if (item->index == -1) {
            if (vtbl_same(lro, i, item->type->lro)) item->index = 0;
        }
    });

    /* calculate number of slots */
    int num_slot = 2;
    vector_foreach(item, lro, {
        if (item->index == -1) num_slot++;
    });

    uintptr_t **slots = mm_alloc(sizeof(uintptr_t *) * num_slot);
    assert(slots);

    HashMapRef mtbl = __get_mtbl(type);
    VectorRef vtbl = __get_vtbl(type, 0);

    /* calculae #0 slot length */
    int n0 = 1;
    VectorRef vec;
    vector_foreach(item, lro, {
        vec = item->type->methods;
        n0 += vector_size(vec);
    });

    /* build #0 slot virtual table */
    uintptr_t *n0slot = mm_alloc(sizeof(uintptr_t) * n0);
    slots[0] = n0slot;
    int index = 0;
    vector_foreach(item, lro, {
        vec = item->type->methods;
        build_vtbl_one(n0slot, index, vec, mtbl);
        index += sizeof(vec);
    });

    /* build other traits virtual tables */
    int j = 0;
    int nx;
    uintptr_t *nxslot;
    vector_foreach(item, lro, {
        if (item->index == -1) {
            item->index = ++j;
            vtbl = __get_vtbl(type, j);
            vec = __get_vtbl(item->type, 0);
            nx = vector_size(vec) + 1;
            nxslot = mm_alloc(sizeof(uintptr_t) * nx);
            build_vtbl_one(nxslot, 0, vec, mtbl);
            slots[j] = nxslot;
            assert(j < num_slot);
        }
    });
}

TypeObjectRef kl_type_new(char *path, char *name, TPFlags flags,
                          VectorRef params, TypeObjectRef base,
                          VectorRef traits)
{
    TypeObjectRef tp = mm_alloc_ptr(tp);
    // tp->type = type_type;
    tp->name = name;
    tp->flags = flags;
    tp->params = params;
    tp->base = base;
    tp->traits = traits;
    return tp;
}

void kl_add_field(TypeObjectRef type, ObjectRef field)
{
}

void kl_add_method(TypeObjectRef type, ObjectRef meth)
{
}

void kl_type_ready(TypeObjectRef type)
{
    build_lro(type);
    inherit_methods(type);
    build_vtbl(type);
}

void kl_init_types(void)
{
    TypeObjectRef any_tp;
    TypeObjectRef type_tp;

    any_tp = kl_type_new_simple("std", "Any", TF_TRAIT | TF_PUB);
    type_tp = kl_type_new_simple("std", "Type", TF_CLASS | TF_PUB | TF_FINAL);

    // any_tp->type = type_tp;
    // type_tp->type = type_tp;

    any_type = any_tp;
    type_type = type_tp;
}

void kl_fini_types(void)
{
}

void kl_type_show(TypeObjectRef type)
{
    if (kl_is_pub(type)) printf("pub ");

    if (kl_is_final(type)) printf("final ");

    if (kl_is_class(type))
        printf("class ");
    else if (kl_is_trait(type))
        printf("trait ");
    else if (kl_is_enum(type))
        printf("enum ");
    else if (kl_is_mod(type))
        printf("module ");
    else
        assert(0);

    printf("%s:\n", type->name);

    /* show lro */
    printf("lro:\n");
    LroInfoRef item;
    vector_foreach(item, type->lro, {
        if (i < len - 1)
            printf("%s <- ", item->type->name);
        else
            printf("%s", item->type->name);
    });
    printf("\n");
    printf("----============----\n");
}

#ifdef __cplusplus
}
#endif
