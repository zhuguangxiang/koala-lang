/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "core.h"
#include "util/hash.h"

#ifdef __cplusplus
extern "C" {
#endif

static FieldNode *_add_field(HashMap *m, char *name, TypeDesc *desc)
{
    FieldNode *f = mm_alloc_obj(f);
    f->name = name;
    f->kind = MNODE_FIELD_KIND;
    hashmap_entry_init(f, str_hash(f->name));
    f->desc = desc;
    if (hashmap_put_absent(m, f)) {
        mm_free(f);
        return null;
    }
    return f;
}

static FuncNode *_add_func(HashMap *m, char *name, TypeDesc *desc)
{
    FuncNode *fn = mm_alloc_obj(fn);
    fn->name = name;
    hashmap_entry_init(fn, str_hash(name));
    fn->desc = desc;
    if (hashmap_put_absent(m, fn)) {
        mm_free(fn);
        return null;
    }
    return fn;
}

static ProtoNode *_add_proto(HashMap *m, char *name, TypeDesc *desc)
{
    ProtoNode *pn = mm_alloc_obj(pn);
    pn->name = name;
    pn->kind = MNODE_IFUNC_KIND;
    hashmap_entry_init(pn, str_hash(name));
    pn->desc = desc;
    if (hashmap_put_absent(m, pn)) {
        mm_free(pn);
        return null;
    }
    return pn;
}

static TypeNode *_add_type(HashMap *m, char *name, TypeInfo *type)
{
    TypeNode *tn = mm_alloc_obj(tn);
    tn->name = name;
    tn->kind = MNODE_TYPE_KIND;
    hashmap_entry_init(tn, str_hash(name));
    tn->type = type;
    if (hashmap_put_absent(m, tn)) {
        mm_free(tn);
        return null;
    }
    return tn;
}

static VarNode *_add_var(HashMap *m, char *name, TypeDesc *desc)
{
    VarNode *vn = mm_alloc_obj(vn);
    vn->name = name;
    vn->kind = MNODE_VAR_KIND;
    hashmap_entry_init(vn, str_hash(name));
    vn->desc = desc;
    if (hashmap_put_absent(m, vn)) {
        mm_free(vn);
        return null;
    }
    return vn;
}

// Create TypeInfo, build LRO and virtual tables

typedef struct _LroInfo {
    TypeInfo *type;
    /* index of virtual table */
    int index;
} LroInfo;

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
    extern TypeInfo any_type;

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
    FuncNode **fn;
    FuncNode *node;
    LroInfo *item;
    for (int i = size - 2; i >= 0; i--) {
        /* omit type self */
        item = vector_get_ptr(lro, i);
        vec = item->type->methods;
        vector_foreach(fn, vec, {
            node = _add_func(mtbl, (*fn)->name, (*fn)->desc);
            if (!node) continue;
            node->kind = (*fn)->kind;
            node->ptr = (*fn)->ptr;
            node->inherit = 1;
            node->slot = -1;
        });
    }
}

static int vtbl_same(Vector *v1, int len1, Vector *v2)
{
    if (vector_size(v2) != len1) return 0;

    LroInfo *item1, *item2;
    for (int i = 0; i <= len1; i++) {
        item1 = vector_get_ptr(v1, i);
        item2 = vector_get_ptr(v2, i);
        if (!item1 || !item2) return 0;
        if (item1->type != item2->type) return 0;
    }
    return 1;
}

static VTable *build_0_slot_vtbl(TypeInfo *type, HashMap *mtbl)
{
    Vector *lro = __get_lro(type);
    LroInfo *item;
    Vector *vec;

    /* calculae length */
    int len = __get_mtbl(type)->count;

    /* build virtual table */
    VTable *vtbl = mm_alloc(sizeof(VTable) + sizeof(FuncNode *) * len);
    vtbl->head = 0;
    vtbl->num = len;
    int index = 0;
    FuncNode **m;
    HashMapEntry *e;
    FuncNode *fn;
    vector_foreach(item, lro, {
        vec = item->type->methods;
        vector_foreach(m, vec, {
            e = hashmap_get(mtbl, &(*m)->entry);
            assert(e);
            fn = (FuncNode *)e;
            if (fn->slot == -1) {
                fn->slot = index;
                vtbl->func[index++] = fn;
            }
        });
    });

    return vtbl;
}

static VTable *build_nth_slot_vtbl(int nth, TypeInfo *type, HashMap *mtbl)
{
    /* calculae length */
    VTable *vtbl0 = type->vtbl[0];
    int len = vtbl0->num;

    /* build virtual table */
    VTable *vtbl = mm_alloc(sizeof(VTable) + sizeof(FuncNode *) * len);
    vtbl->head = -(nth * PTR_SIZE);
    vtbl->num = len;

    FuncNode **fn = vtbl0->func;
    HashMapEntry *e;
    for (int i = 0; i < len; i++) {
        e = hashmap_get(mtbl, &fn[i]->entry);
        assert(e);
        vtbl->func[i] = (FuncNode *)e;
    }

    return vtbl;
}

static void build_vtbl(TypeInfo *type)
{
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
            if (vtbl_same(lro, i, item->type->lro)) item->index = 0;
        }
    });

    /* calculate number of slots */
    int num_slot = 2;
    vector_foreach(item, lro, {
        if (item->index == -1) num_slot++;
    });

    VTable **slots = mm_alloc(sizeof(VTable *) * num_slot);
    HashMap *mtbl = __get_mtbl(type);

    /* build #0 slot virtual table */
    slots[0] = build_0_slot_vtbl(type, mtbl);

    /* build other traits virtual tables */
    int j = 0;
    vector_foreach(item, lro, {
        if (item->index == -1) {
            item->index = ++j;
            slots[j] = build_nth_slot_vtbl(j, item->type, mtbl);
            assert(j < num_slot);
        }
    });

    /* update virtual table */
    VTable *vt;
    for (int i = 0; i <= j; i++) {
        vt = slots[i];
        vt->data = (j - i + 1) * PTR_SIZE;
        vt->type = type;
    }

    /* set virtual table */
    type->num_vtbl = num_slot - 1;
    type->vtbl = slots;
}

TypeInfo *type_new(char *name, int flags)
{
    TypeInfo *tp = mm_alloc_obj(tp);
    tp->name = name;
    tp->flags = flags;
    return tp;
}

int type_set_base(TypeInfo *type, TypeInfo *base)
{
    if (type->base) return -1;
    type->base = base;
    return 0;
}

int type_add_typeparam(TypeInfo *type, char *name, TypeInfo *bound)
{
    Vector *tps = type->params;
    if (!tps) {
        tps = vector_create(sizeof(TypeParam));
        type->params = tps;
    }

    TypeParam tp;
    strcpy(tp.name, name);
    tp.bound = bound;
    vector_push_back(tps, &tp);
    return 0;
}

int type_add_trait(TypeInfo *type, TypeInfo *trait)
{
    Vector *traits = type->traits;
    if (!traits) {
        traits = vector_create(PTR_SIZE);
        type->traits = traits;
    }
    vector_push_back(traits, &trait);
    return 0;
}

int type_add_field(TypeInfo *ty, char *name, TypeDesc *desc)
{
    FieldNode *fld = _add_field(__get_mtbl(ty), name, desc);
    if (!fld) return -1;
    fld->offset = 0;
    return 0;
}

int type_add_kfunc(TypeInfo *ty, char *name, TypeDesc *desc, CodeInfo *code)
{
    FuncNode *fn = _add_func(__get_mtbl(ty), name, desc);
    if (!fn) return -1;
    fn->kind = MNODE_KFUNC_KIND;
    fn->ptr = (uintptr)code;
    fn->slot = -1;
    return 0;
}

int type_add_cfunc(TypeInfo *ty, char *name, TypeDesc *desc, void *ptr)
{
    FuncNode *fn = _add_func(__get_mtbl(ty), name, desc);
    if (!fn) return -1;
    fn->kind = MNODE_CFUNC_KIND;
    fn->ptr = (uintptr)ptr;
    fn->slot = -1;
    Vector *vec = ty->methods;
    if (!vec) {
        vec = vector_create(PTR_SIZE);
        ty->methods = vec;
    }
    vector_push_back(vec, &fn);
    return 0;
}

int type_add_ifunc(TypeInfo *ty, char *name, TypeDesc *desc)
{
    ProtoNode *pn = _add_proto(__get_mtbl(ty), name, desc);
    if (!pn) return -1;
    return 0;
}

void type_ready(TypeInfo *type)
{
    build_lro(type);
    inherit_methods(type);
    build_vtbl(type);
}

void type_show(TypeInfo *type)
{
    if (type_is_final(type)) printf("final ");

    if (type_is_class(type))
        printf("class ");
    else if (type_is_trait(type))
        printf("trait ");
    else if (type_is_enum(type))
        printf("enum ");
    else
        assert(0);

    printf("%s {\n", type->name);

    /* show lro */
    printf("lro:\n");
    LroInfo *item;
    vector_foreach(item, type->lro, {
        if (i == 0)
            printf("  %s(%d)", item->type->name, item->index);
        else
            printf(" <- %s(%d)", item->type->name, item->index);
    });
    printf("\n\n");

    /* show methods */
    printf("self-methods:\n");
    FuncNode **m;
    vector_foreach(m, type->methods,
                   { printf("  %s@%lx\n", (*m)->name, (*m)->ptr); });
    printf("\n");

    /* show vtbl */
    FuncNode **fn;
    VTable **vtbl = type->vtbl;
    int i = 0;
    while (*vtbl) {
        printf("vtbl[%d], head = %d, data = %d:\n", i++, (*vtbl)->head,
               (*vtbl)->data);
        fn = (*vtbl)->func;
        while (*fn) {
            printf("  %s@%lx(%d)\n", (*fn)->name, (*fn)->ptr, (*fn)->inherit);
            fn++;
        }
        ++vtbl;
        printf("\n");
    }

    printf("}\n\n");
}

int type_get_func_slot(TypeInfo *type, char *name)
{
    MNode key = { .name = name };
    hashmap_entry_init(&key, str_hash(name));
    FuncNode *fn = hashmap_get(type->mtbl, &key);
    return fn == null ? -1 : fn->slot;
}

FuncNode *object_get_func(objref obj, int offset)
{
    VTable *vtbl = __GET_VTBL(obj);
    assert(offset >= 0 && offset < vtbl->num);
    return vtbl->func[offset];
}

static HashMap *pkg_map;
static PkgInfo *root_pkg;

PkgInfo *new_pkg(char *path)
{
    PkgInfo *pkg = mm_alloc_obj(pkg);
    pkg->path = path;
    hashmap_entry_init(&pkg->entry, str_hash(path));
    hashmap_init(&pkg->map, mn_equal);
    return pkg;
}

static PkgInfo *__get_or_create_pkg(char *path)
{
}

void pkg_add_type(char *path, TypeInfo *type)
{
    _add_type(&pkg->map, type->name, type);
}

void pkg_add_var(char *path, char *name, TypeDesc *desc)
{
    _add_var(&pkg->map, name, desc);
}

void pkg_add_cfunc(char *path, char *name, TypeDesc *desc, void *ptr)
{
    FuncNode *fn = _add_func(&pkg->map, name, desc);
    if (!fn) return;
    fn->kind = MNODE_CFUNC_KIND;
    fn->ptr = (uintptr)ptr;
}

void pkg_add_kfunc(char *path, char *name, TypeDesc *desc, CodeInfo *code)
{
    FuncNode *fn = _add_func(&pkg->map, name, desc);
    if (!fn) return;
    fn->kind = MNODE_KFUNC_KIND;
    fn->ptr = (uintptr)code;
}

void root_add_type(TypeInfo *type)
{
    _add_type(&root_pkg->map, type->name, type);
}

void root_add_var(char *name, TypeDesc *desc)
{
    _add_var(&root_pkg->map, name, desc);
}

void root_add_cfunc(char *name, TypeDesc *desc, void *ptr)
{
    FuncNode *fn = _add_func(&root_pkg->map, name, desc);
    if (!fn) return;
    fn->kind = MNODE_CFUNC_KIND;
    fn->ptr = (uintptr)ptr;
}

void root_add_kfunc(char *name, TypeDesc *desc, CodeInfo *code)
{
    FuncNode *fn = _add_func(&root_pkg->map, name, desc);
    if (!fn) return;
    fn->kind = MNODE_KFUNC_KIND;
    fn->ptr = (uintptr)code;
}

static int pkg_equal(void *m1, void *m2)
{
    PkgInfo *pkg1 = m1;
    PkgInfo *pkg2 = m2;
    return !strcmp(pkg1->path, pkg2->path);
}

void init_core(void)
{
    pkg_map = mm_alloc_obj(pkg_map);
    hashmap_init(pkg_map, pkg_equal);
    root_pkg = new_pkg("/");
    hashmap_put_absent(pkg_map, root_pkg);
}

void fini_core(void)
{
}

#ifdef __cplusplus
}
#endif
