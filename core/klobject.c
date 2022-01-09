/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "eval.h"
#include "util/buffer.h"
#include "util/log.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
trait Any {
    func __hash__() int;
    func __cmp__(other Any) int;
    func __class__() Class;
    func __str__() string;
}
*/

KlType any_type = {
    .name = "any",
    .flags = TP_TRAIT,
};

static int kl_any_hash(KlState *ks)
{
    // KlValue self = kl_pop_value(ks);
    // int32 hash = mem_hash(&self, sizeof(uintptr));
    // kl_push_int32(ks, hash);
    return 1;
}

static int kl_any_equal(KlState *ks)
{
    // KlValue self = kl_pop_value(ks);
    // KlValue o = kl_pop_value(ks);
    // int eq = (self.obj == o.obj);
    // kl_push_bool(ks, eq);
    return 1;
}

static int kl_any_class(KlState *ks)
{
    printf("any class called()\n");
    return 0;
}

static int kl_any_str(KlState *ks)
{
    // char buf[64];
    // TypeInfo *type = __GET_TYPE(self);
    // snprintf(buf, sizeof(buf) - 1, "%.32s@%lx", type->name, self);
    // return string_new(buf);
    printf("any str called()\n");
    return 0;
}

static void init_any_type(void)
{
    MethodDef methods[] = {
        /* DO NOT change the order */
        /* clang-format off */
        { "__hash__",  null, "i32",     kl_any_hash  },
        { "__eq__",    "A",  "b",       kl_any_equal },
        { "__class__", null, "LClass;", kl_any_class },
        { "__str__",   null, "s",       kl_any_str   },
        { null },
        /* clang-format on */
    };

    type_add_methdefs(&any_type, methods);
    type_ready(&any_type);
    // mod_add_type("/", &any_type);
}

KlField *mtbl_add_field(HashMap *m, char *name)
{
    KlField *fd = mm_alloc_obj(fd);
    fd->name = name;
    hashmap_entry_init(fd, str_hash(name));
    if (hashmap_put_absent(m, fd)) {
        mm_free(fd);
        return null;
    }
    fd->kind = KL_FIELD_NODE;
    return fd;
}

KlFunc *mtbl_add_func(HashMap *m, char *name)
{
    KlFunc *fn = mm_alloc_obj(fn);
    fn->name = name;
    hashmap_entry_init(fn, str_hash(name));
    if (hashmap_put_absent(m, fn)) {
        mm_free(fn);
        return null;
    }
    fn->kind = KL_FUNC_NODE;
    return fn;
}

KlProto *mtbl_add_ifunc(HashMap *m, char *name)
{
    KlProto *pt = mm_alloc_obj(pt);
    pt->name = name;
    hashmap_entry_init(pt, str_hash(name));
    if (hashmap_put_absent(m, pt)) {
        mm_free(pt);
        return null;
    }
    pt->kind = KL_IFUNC_NODE;
    return pt;
}

int mtbl_add_type(HashMap *m, KlType *ty)
{
    hashmap_entry_init(ty, str_hash(ty->name));
    if (hashmap_put_absent(m, ty)) {
        return -1;
    }
    return 0;
}

int mtbl_equal_func(void *m1, void *m2)
{
    KlNode *n1 = m1;
    KlNode *n2 = m2;
    return !strcmp(n1->name, n2->name);
}

HashMap *type_get_mtbl(KlType *type)
{
    HashMap *mtbl = type->mtbl;
    if (!mtbl) {
        mtbl = mm_alloc_obj(mtbl);
        hashmap_init(mtbl, mtbl_equal_func);
        type->mtbl = mtbl;
    }
    return mtbl;
}

typedef struct _LroInfo {
    KlType *type;
    /* index of virtual table */
    int index;
} LroInfo;

static int lro_find(Vector *lro, KlType *type)
{
    LroInfo *item;
    vector_foreach(item, lro, {
        if (item->type == type) return 1;
    });
    return 0;
}

static inline Vector *__get_lro(KlType *type)
{
    Vector *vec = type->lro;
    if (!vec) {
        vec = vector_create(sizeof(LroInfo));
        type->lro = vec;
    }
    return vec;
}

static void build_one_lro(KlType *type, KlType *one)
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

static void build_lro(KlType *type)
{
    /* add Any type */
    build_one_lro(type, &any_type);

    /* add first class or trait */
    build_one_lro(type, type->base);

    /* add traits */
    KlType **trait;
    vector_foreach(trait, type->traits, { build_one_lro(type, *trait); });

    /* add self */
    build_one_lro(type, type);
}

static void inherit_methods(KlType *type)
{
    Vector *lro = __get_lro(type);
    int size = vector_size(lro);
    HashMap *mtbl = type_get_mtbl(type);
    Vector *vec;
    KlFunc **fn;
    KlFunc *node;
    LroInfo *item;
    /* omit type self */
    for (int i = size - 2; i >= 0; i--) {
        item = vector_get_ptr(lro, i);
        vec = item->type->methods;
        vector_foreach(fn, vec, {
            node = mtbl_add_func(mtbl, (*fn)->name);
            if (!node) continue;
            node->desc = (*fn)->desc;
            node->kind = (*fn)->kind;
            node->ptr = (*fn)->ptr;
            node->owner = item->type;
            node->inherit = 1;
            node->slot = -1;
        });
    }
}

static Vector *__get_fields(KlType *ty)
{
    Vector *vec = ty->fields;
    if (!vec) {
        vec = vector_create_ptr();
        ty->fields = vec;
    }
    return vec;
}

static Vector *__get_methods(KlType *ty)
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

static void build_main_vtbl(KlFuncTbl *vtbl, KlType *type, HashMap *mtbl)
{
    Vector *lro = __get_lro(type);
    LroInfo *item;
    Vector *vec;

    /* calc main vtbl length */
    /* terminated with null */
    int num = 1;

    vector_foreach(item, lro, { num += vector_size(item->type->methods); });

    /* build virtual table */
    KlFunc **ftbl = mm_alloc(sizeof(KlFunc *) * num);
    vtbl->num = num;
    vtbl->func = ftbl;

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
                ftbl[index++] = fn;
            }
        });
    });
}

static void build_nth_vtbl(KlFuncTbl *vtbl, int nth, KlType *type,
                           HashMap *mtbl)
{
    /* calculae length */
    KlFuncTbl *vtbl0 = type->vtbl;
    int num = vtbl0->num;

    /* build virtual table */
    KlFunc **ftbl = mm_alloc(sizeof(KlFunc *) * num);
    vtbl->num = num;
    vtbl->func = ftbl;

    KlFunc **fn = vtbl0->func;
    HashMapEntry *e;
    for (int i = 0; i < num - 1; i++) {
        e = hashmap_get(mtbl, &fn[i]->entry);
        assert(e);
        ftbl[i] = (KlFunc *)e;
    }
}

static void build_vtbl(KlType *type)
{
    HashMap *mtbl = type_get_mtbl(type);
    KlFuncTbl *vtbl;

    /* first class or trait is the same virtual table, at #0 */
    Vector *lro = __get_lro(type);
    KlType *base = type;
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

    vtbl = mm_alloc(sizeof(KlFuncTbl) * num_vtbl);

    /* build #0(main) virtual table */
    build_main_vtbl(vtbl, type, mtbl);

    /* build other traits virtual tables */
    int j = 0;
    vector_foreach(item, lro, {
        if (item->index == -1) {
            item->index = ++j;
            build_nth_vtbl(vtbl + j, j, item->type, mtbl);
            assert(j < num_vtbl);
        }
    });

    /* update virtual table */
    KlFuncTbl *vt;
    for (int i = 0; i <= j; i++) {
        vt = vtbl + i;
        vt->type = type;
    }

    /* set virtual table */
    type->num_vtbl = num_vtbl - 1;
    type->vtbl = vtbl;
}

int type_add_field(KlType *ty, char *name, TypeDesc *desc)
{
    HashMap *mtbl = type_get_mtbl(ty);
    KlField *fd = mtbl_add_field(mtbl, name);
    if (!fd) return -1;
    fd->desc = desc;
    fd->owner = ty;

    fd->offset = 0;
    vector_push_back(__get_fields(ty), &fd);
    return 0;
}

int type_add_kfunc(KlType *ty, char *name, TypeDesc *desc, KlCode *code)
{
    HashMap *mtbl = type_get_mtbl(ty);
    KlFunc *fn = mtbl_add_func(mtbl, name);
    if (!fn) return -1;
    fn->desc = desc;
    fn->owner = ty;

    fn->cfunc = 0;
    fn->ptr = (uintptr)code;
    fn->slot = -1;
    vector_push_back(__get_methods(ty), &fn);
    return 0;
}

int type_add_cfunc(KlType *ty, char *name, TypeDesc *desc, void *cfunc)
{
    HashMap *mtbl = type_get_mtbl(ty);
    KlFunc *fn = mtbl_add_func(mtbl, name);
    if (!fn) return -1;
    fn->desc = desc;
    fn->owner = ty;

    fn->cfunc = 1;
    fn->ptr = (uintptr)cfunc;
    fn->slot = -1;
    vector_push_back(__get_methods(ty), &fn);
    return 0;
}

int type_add_ifunc(KlType *ty, char *name, TypeDesc *desc)
{
    HashMap *mtbl = type_get_mtbl(ty);
    KlProto *pt = mtbl_add_ifunc(mtbl, name);
    if (!pt) return -1;
    pt->desc = desc;
    pt->owner = ty;

    pt->slot = 0;
    vector_push_back(__get_methods(ty), &pt);
    return 0;
}

void type_ready(KlType *type)
{
    build_lro(type);
    inherit_methods(type);
    build_vtbl(type);
}

#if !defined(NLOG)
void type_show(KlType *type)
{
    if (TP_IS_FINAL(type)) log("final ");

    if (TP_IS_CLASS(type)) {
        log("class ");
    } else if (TP_IS_TRAIT(type)) {
        log("trait ");
    } else if (TP_IS_ENUM(type)) {
        log("enum ");
    } else {
        return;
    }

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
    KlFuncTbl *vtbl = type->vtbl;
    KlType *owner;
    int i = 0;
    int j = 0;
    while (j < type->num_vtbl) {
        log("vtbl[%d]:", i++);
        fn = vtbl->func;
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
        ++j;
        log("\n");
    }

    log("}\n\n");
}
#endif

void type_add_methdefs(KlType *ty, MethodDef *def)
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

int type_set_base(KlType *type, KlType *base)
{
    if (type->base) return -1;
    type->base = base;
    return 0;
}

int type_add_bound(KlType *type, char *name, KlType *bound)
{
    return 0;
}

int type_add_trait(KlType *type, KlType *trait)
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
