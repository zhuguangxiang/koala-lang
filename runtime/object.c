/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "object.h"
#include "util/buffer.h"
#include "util/log.h"

#ifdef __cplusplus
extern "C" {
#endif

static KlField *_add_field(HashMap *m, char *name, TypeDesc *desc)
{
    KlField *fd = mm_alloc_obj(fd);
    fd->name = name;
    hashmap_entry_init(fd, str_hash(name));
    if (hashmap_put_absent(m, fd)) {
        mm_free(fd);
        return null;
    }
    fd->kind = MNODE_FIELD_KIND;
    fd->desc = desc;
    return fd;
}

static KlFunc *_add_func(HashMap *m, char *name, TypeDesc *desc)
{
    KlFunc *fn = mm_alloc_obj(fn);
    fn->name = name;
    hashmap_entry_init(fn, str_hash(name));
    if (hashmap_put_absent(m, fn)) {
        mm_free(fn);
        return null;
    }
    fn->desc = desc;
    return fn;
}

static KlProto *_add_ifunc(HashMap *m, char *name, TypeDesc *desc)
{
    KlProto *pt = mm_alloc_obj(pt);
    pt->name = name;
    hashmap_entry_init(pt, str_hash(name));
    if (hashmap_put_absent(m, pt)) {
        mm_free(pt);
        return null;
    }
    pt->kind = MNODE_IFUNC_KIND;
    pt->desc = desc;
    return pt;
}

static KlType *_add_type(HashMap *m, TypeInfo *type)
{
    KlType *ty = mm_alloc_obj(ty);
    ty->name = type->name;
    hashmap_entry_init(ty, str_hash(type->name));
    if (hashmap_put_absent(m, ty)) {
        mm_free(ty);
        return null;
    }
    ty->kind = MNODE_TYPE_KIND;
    ty->type = type;
    return ty;
}

static KlVar *_add_var(HashMap *m, char *name, TypeDesc *desc)
{
    KlVar *va = mm_alloc_obj(va);
    va->name = name;
    hashmap_entry_init(va, str_hash(name));
    if (hashmap_put_absent(m, va)) {
        mm_free(va);
        return null;
    }
    va->kind = MNODE_VAR_KIND;
    va->desc = desc;
    return va;
}

static int mn_equal(void *m1, void *m2)
{
    MNode *n1 = m1;
    MNode *n2 = m2;
    return !strcmp(n1->name, n2->name);
}

static inline HashMap *__get_mtbl(TypeInfo *type)
{
    HashMap *mtbl = type->mtbl;
    if (!mtbl) {
        mtbl = mm_alloc_obj(mtbl);
        hashmap_init(mtbl, mn_equal);
        type->mtbl = mtbl;
    }
    return mtbl;
}

typedef struct _LroInfo {
    TypeInfo *type;
    /* index of virtual table */
    int index;
} LroInfo;

static int lro_find(Vector *lro, TypeInfo *type)
{
    LroInfo *item;
    vector_foreach(item, lro, {
        if (item->type == type) return 1;
    });
    return 0;
}

static inline Vector *__get_lro(TypeInfo *type)
{
    Vector *vec = type->lro;
    if (!vec) {
        vec = vector_create(sizeof(LroInfo));
        type->lro = vec;
    }
    return vec;
}

static void build_one_lro(TypeInfo *type, TypeInfo *one)
{
    if (!one) return;

    Vector *vec = __get_lro(type);

    LroInfo lro = { null, -1 };
    LroInfo *item;
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

static void build_lro(TypeInfo *type)
{
    /* add Any type */
    build_one_lro(type, &any_type);

    /* add first class or trait */
    build_one_lro(type, type->base);

    /* add traits */
    TypeInfo **trait;
    vector_foreach(trait, type->traits, { build_one_lro(type, *trait); });

    /* add self */
    build_one_lro(type, type);
}

static void inherit_methods(TypeInfo *type)
{
    Vector *lro = __get_lro(type);
    int size = vector_size(lro);
    HashMap *mtbl = __get_mtbl(type);
    Vector *vec;
    KlFunc **fn;
    KlFunc *node;
    LroInfo *item;
    /* omit type self */
    for (int i = size - 2; i >= 0; i--) {
        item = vector_get_ptr(lro, i);
        vec = item->type->methods;
        vector_foreach(fn, vec, {
            node = _add_func(mtbl, (*fn)->name, (*fn)->desc);
            if (!node) continue;
            node->kind = (*fn)->kind;
            node->ptr = (*fn)->ptr;
            node->owner = item->type;
            node->inherit = 1;
            node->slot = -1;
        });
    }
}

static Vector *__get_fields(TypeInfo *ty)
{
    Vector *vec = ty->fields;
    if (!vec) {
        vec = vector_create_ptr();
        ty->fields = vec;
    }
    return vec;
}

static Vector *__get_methods(TypeInfo *ty)
{
    Vector *vec = ty->methods;
    if (!vec) {
        vec = vector_create_ptr();
        ty->methods = vec;
    }
    return vec;
}

static int vtbl_same(Vector *v1, Vector *v2)
{
    int len2 = vector_size(v2);
    LroInfo *item1, *item2;
    for (int i = 0; i <= len2; i++) {
        item1 = vector_get_ptr(v1, i);
        item2 = vector_get_ptr(v2, i);
        if (!item1 || !item2) return 0;
        if (item1->type != item2->type) return 0;
    }
    return 1;
}

static VTable *build_main_vtbl(TypeInfo *type, HashMap *mtbl)
{
    Vector *lro = __get_lro(type);
    LroInfo *item;
    Vector *vec;

    /* calc main vtbl length */
    /* terminated with null */
    int num = 1;

    vector_foreach(item, lro, { num += vector_size(item->type->methods); });

    /* build virtual table */
    VTable *vtbl = mm_alloc(sizeof(VTable) + sizeof(KlFunc *) * num);
    vtbl->num = num;

    int index = 0;
    KlFunc **m;
    HashMapEntry *e;
    KlFunc *fn;
    vector_foreach(item, lro, {
        vec = item->type->methods;
        vector_foreach(m, vec, {
            e = hashmap_get(mtbl, &(*m)->entry);
            assert(e);
            fn = (KlFunc *)e;
            if (fn->slot == -1) {
                fn->slot = index;
                vtbl->func[index++] = fn;
            }
        });
    });

    return vtbl;
}

static VTable *build_nth_vtbl(int nth, TypeInfo *type, HashMap *mtbl)
{
    /* calculae length */
    VTable *vtbl0 = type->vtbl[0];
    int len = vtbl0->num;

    /* build virtual table */
    VTable *vtbl = mm_alloc(sizeof(VTable) + sizeof(KlFunc *) * len);
    vtbl->num = len;

    KlFunc **fn = vtbl0->func;
    HashMapEntry *e;
    for (int i = 0; i < len - 1; i++) {
        e = hashmap_get(mtbl, &fn[i]->entry);
        assert(e);
        vtbl->func[i] = (KlFunc *)e;
    }

    return vtbl;
}

static void build_vtbl(TypeInfo *type)
{
    HashMap *mtbl = __get_mtbl(type);
    VTable **vtbl;

    /* first class or trait is the same virtual table, at #0 */
    Vector *lro = __get_lro(type);
    TypeInfo *base = type;
    LroInfo *item;
    while (base) {
        vector_foreach(item, lro, {
            if (item->type == base) {
                item->index = 0;
                break;
            }
        });
        base = base->base;
    }

    /* vtbl of 'Any' is also at #0 */
    item = vector_first_ptr(lro);
    item->index = 0;

    /* update some traits indexes, they are also at #0 */
    vector_foreach_reverse(item, lro, {
        if (item->index == -1) {
            if (i != vector_size(item->type->lro)) continue;
            if (vtbl_same(lro, item->type->lro)) item->index = 0;
        }
    });

    /* calculate number of slots */
    /* terminated with null */
    int num_vtbl = 2;
    vector_foreach(item, lro, {
        if (item->index == -1) num_vtbl++;
    });

    vtbl = mm_alloc(sizeof(VTable *) * num_vtbl);

    /* build #0(main) virtual table */
    vtbl[0] = build_main_vtbl(type, mtbl);

    /* build other traits virtual tables */
    int j = 0;
    vector_foreach(item, lro, {
        if (item->index == -1) {
            item->index = ++j;
            vtbl[j] = build_nth_vtbl(j, item->type, mtbl);
            assert(j < num_vtbl);
        }
    });

    /* update virtual table */
    VTable *vt;
    for (int i = 0; i <= j; i++) {
        vt = vtbl[i];
        vt->type = type;
    }

    /* set virtual table */
    type->num_vtbl = num_vtbl - 1;
    type->vtbl = vtbl;
}

int type_add_field(TypeInfo *ty, char *name, TypeDesc *desc)
{
    KlField *fd = _add_field(__get_mtbl(ty), name, desc);
    if (!fd) return -1;
    fd->offset = 0;
    vector_push_back(__get_fields(ty), &fd);
    return 0;
}

int type_add_kfunc(TypeInfo *ty, char *name, TypeDesc *desc, KlCode *code)
{
    KlFunc *fn = _add_func(__get_mtbl(ty), name, desc);
    if (!fn) return -1;
    fn->kind = MNODE_KFUNC_KIND;
    fn->owner = ty;
    fn->ptr = (uintptr)code;
    fn->slot = -1;
    vector_push_back(__get_methods(ty), &fn);
    return 0;
}

int type_add_cfunc(TypeInfo *ty, char *name, TypeDesc *desc, void *cfunc)
{
    KlFunc *fn = _add_func(__get_mtbl(ty), name, desc);
    if (!fn) return -1;
    fn->kind = MNODE_CFUNC_KIND;
    fn->owner = ty;
    fn->ptr = (uintptr)cfunc;
    fn->slot = -1;
    vector_push_back(__get_methods(ty), &fn);
    return 0;
}

int type_add_ifunc(TypeInfo *ty, char *name, TypeDesc *desc)
{
    KlProto *pt = _add_ifunc(__get_mtbl(ty), name, desc);
    if (!pt) return -1;
    vector_push_back(__get_methods(ty), &pt);
    return 0;
}

void type_ready(TypeInfo *type)
{
    build_lro(type);
    inherit_methods(type);
    build_vtbl(type);
}

#if !defined(NLOG)
void type_show(TypeInfo *type)
{
    if (TP_IS_FINAL(type)) log("final ");

    if (TP_IS_CLASS(type))
        log("class ");
    else if (TP_IS_TRAIT(type))
        log("trait ");
    else if (TP_IS_ENUM(type))
        log("enum ");
    else
        assert(0);

    log("%s {\n", type->name);

    /* show lro */
    log("lro:\n");
    LroInfo *item;
    vector_foreach_reverse(item, type->lro, {
        if (i == 0)
            log(" %s(%d)", item->type->name, item->index);
        else
            log("  %s(%d) ->", item->type->name, item->index);
    });
    log("\n\n");

    /* show fields */
    log("fields:\n");
    log("\n");

    /* show methods */
    log("methods:\n");
    KlFunc **m;
    BUF(buf);
    vector_foreach(m, type->methods, {
        desc_to_str((*m)->desc, &buf);
        log("  func %s%s\n", (*m)->name, BUF_STR(buf));
        RESET_BUF(buf);
    });
    FINI_BUF(buf);
    log("\n");

    /* show vtbl */
    KlFunc **fn;
    VTable **vtbl = type->vtbl;
    TypeInfo *owner;
    int i = 0;
    while (*vtbl) {
        log("vtbl[%d]:", i++);
        fn = (*vtbl)->func;
        while (*fn) {
            owner = (*fn)->owner;
            if (owner != type) {
                assert((*fn)->inherit);
                log("\n  %s.%s", owner->name, (*fn)->name);
            } else {
                log("\n  %s", (*fn)->name);
            }
            fn++;
        }
        ++vtbl;
        log("\n");
    }

    log("}\n\n");
}
#endif

void type_add_methdefs(TypeInfo *ty, MethodDef *def)
{
    TypeDesc *desc;
    MethodDef *fn = def;
    while (fn->name) {
        desc = str_to_proto(fn->ptype, fn->rtype);
        if (!fn->cfunc) {
            type_add_ifunc(ty, fn->name, desc);
        } else {
            type_add_cfunc(ty, fn->name, desc, fn->cfunc);
        }
        ++fn;
    }
}

int type_set_base(TypeInfo *type, TypeInfo *base)
{
    if (type->base) return -1;
    type->base = base;
    return 0;
}

int type_add_bound(TypeInfo *type, char *name, TypeInfo *bound)
{
    return 0;
}

int type_add_trait(TypeInfo *type, TypeInfo *trait)
{
    Vector *traits = type->traits;
    if (!traits) {
        traits = vector_create_ptr();
        type->traits = traits;
    }
    vector_push_back(traits, &trait);
    return 0;
}

#ifdef __cplusplus
}
#endif
