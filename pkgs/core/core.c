/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "core.h"

#ifdef __cplusplus
extern "C" {
#endif

static HashMap *all_pkgs;

TypeInfo any_type = {
    .name = "Any",
    .flags = TF_TRAIT,
};

/*------ type parameter bitmap ----------------------------------------------*/

int tp_size(uint32 tp_map, int index)
{
    static int __size__[] = { 0, PTR_SIZE, 1, 2, 4, 8, 2, 4, 8, 1, 4 };
    return __size__[tp_index(tp_map, index)];
}

/*------ generic type -------------------------------------------------------*/

/* var obj T, where the upper bound of T is Any */

int32 generic_any_hash(uintptr obj, int isref)
{
    if (!isref) {
        return (int32)mem_hash(&obj, sizeof(uintptr));
    } else {
        TypeInfo *type = ((Object *)obj)->type;
        int32 (*fn)(uintptr) = NULL; //(void *)type->vtbl[0][0];
        return fn(obj);
    }
}

bool generic_any_equal(uintptr obj, uintptr other, int isref)
{
    if (obj == other) return 1;

    if (isref) {
        TypeInfo *type = ((Object *)obj)->type;
        bool (*fn)(uintptr, uintptr) = NULL; //(void *)type->vtbl[0][1];
        return fn(obj, other);
    }

    return 0;
}

Object *generic_any_class(uintptr obj, int tpkind)
{
    return nil;
}

Object *generic_any_tostr(uintptr obj, int tpkind)
{
    return nil;
}

/*------ Any type -----------------------------------------------------------*/

int32 any_hash(Object *self)
{
    return (int32)mem_hash(&self, sizeof(void *));
}

bool any_equal(Object *self, Object *other)
{
    if (self->type != other->type) return 0;
    return self == other;
}

Object *any_class(Object *self)
{
    return class_new(self->type);
}

Object *any_tostr(Object *self)
{
    char buf[64];
    TypeInfo *type = self->type;
    snprintf(buf, sizeof(buf) - 1, "%.32s@%x", type->name, PTR2INT(self));
    return string_new(buf);
}

void init_any_type(void)
{
    // clang-format off
    /* DON'T change the order */
    MethodDef any_methods[] = {
        METHOD_DEF("__hash__", nil, "i32",     any_hash),
        METHOD_DEF("__eq__",   "A", "b",       any_equal),
        METHOD_DEF("__type__", nil, "LClass;", any_class),
        METHOD_DEF("__str__",  nil, "s",       any_tostr),
    };
    // clang-format on
    type_add_methods(&any_type, any_methods, COUNT_OF(any_methods));
    type_ready(&any_type);
    pkg_add_type("/", &any_type);
}

/*------ Member Node --------------------------------------------------------*/

static int mn_equal(void *m1, void *m2)
{
    MNode *n1 = m1;
    MNode *n2 = m2;
    return !strcmp(n1->name, n2->name);
}

static HashMap *mn_map_new(void)
{
    HashMap *m = mm_alloc_obj(m);
    hashmap_init(m, mn_equal);
    return m;
}

static FieldNode *_add_field(HashMap *m, char *name, TypeDesc *desc)
{
    FieldNode *f = mm_alloc_obj(f);
    f->name = name;
    f->kind = MNODE_FIELD_KIND;
    hashmap_entry_init(f, str_hash(f->name));
    f->desc = desc;
    if (hashmap_put_absent(m, f)) {
        mm_free(f);
        return nil;
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
        return nil;
    }
    return fn;
}

static ProtoNode *_add_proto(HashMap *m, char *name, TypeDesc *desc)
{
    ProtoNode *pn = mm_alloc_obj(pn);
    pn->name = name;
    pn->kind = MNODE_PROTO_KIND;
    hashmap_entry_init(pn, str_hash(name));
    pn->desc = desc;
    if (hashmap_put_absent(m, pn)) {
        mm_free(pn);
        return nil;
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
        return nil;
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
        return nil;
    }
    return vn;
}

/*------ type info ----------------------------------------------------------*/

// Create TypeInfo, build LRO and virtual tables

typedef struct _LroInfo LroInfo;

struct _LroInfo {
    TypeInfo *type;
    /* index of virtual table */
    int index;
};

static inline HashMap *__get_mtbl(TypeInfo *type)
{
    HashMap *mtbl = type->mtbl;
    if (!mtbl) {
        mtbl = mn_map_new();
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

    LroInfo lro = { nil, -1 };
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

static VirtTable *build_0_slot_vtbl(TypeInfo *type, HashMap *mtbl)
{
    Vector *lro = __get_lro(type);
    LroInfo *item;
    Vector *vec;

    /* calculae length */
    int length = 0;
    vector_foreach(item, lro, {
        vec = item->type->methods;
        length += vector_size(vec);
    });

    /* build virtual table */
    VirtTable *vtbl = mm_alloc(sizeof(VirtTable) + sizeof(uintptr) * length);
    vtbl->head = 0;
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

static VirtTable *build_nth_slot_vtbl(int nth, TypeInfo *type, HashMap *mtbl)
{
    /* calculae length */
    int length = 0;
    VirtTable *vtbl0 = type->vtbl[0];
    uintptr *ptr = (uintptr *)vtbl0->func;
    while (*ptr) {
        length++;
        ptr++;
    }

    /* build virtual table */
    VirtTable *vtbl = mm_alloc(sizeof(VirtTable) + sizeof(uintptr) * length);
    vtbl->head = -(nth * PTR_SIZE);
    ptr = (uintptr *)vtbl0->func;
    HashMapEntry *e;
    FuncNode *fn;
    int index = 0;
    while (*ptr) {
        e = hashmap_get(mtbl, &((FuncNode *)*ptr)->entry);
        assert(e);
        fn = (FuncNode *)e;
        vtbl->func[index++] = fn;
        ptr++;
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

    VirtTable **slots = mm_alloc(sizeof(VirtTable *) * num_slot);
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

    /* set virtual table */
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

int type_add_typeparam(TypeInfo *type, TypeParam *tp)
{
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
    fn->ptr = code;
    fn->slot = -1;
    return 0;
}

int type_add_cfunc(TypeInfo *ty, char *name, TypeDesc *desc, void *ptr)
{
    FuncNode *fn = _add_func(__get_mtbl(ty), name, desc);
    if (!fn) return -1;
    fn->kind = MNODE_CFUNC_KIND;
    fn->ptr = ptr;
    fn->slot = -1;
    Vector *vec = ty->methods;
    if (!vec) {
        vec = vector_create(PTR_SIZE);
        ty->methods = vec;
    }
    vector_push_back(vec, &fn);
    return 0;
}

int type_add_proto(TypeInfo *ty, char *name, TypeDesc *desc)
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
                   { printf("  %s@%p\n", (*m)->name, (*m)->ptr); });
    printf("\n");

    /* show vtbl */
    FuncNode **fn;
    VirtTable **vtbl = type->vtbl;
    int i = 0;
    while (*vtbl) {
        printf("vtbl[%d], head = %d:\n", i++, (*vtbl)->head);
        fn = (*vtbl)->func;
        while (*fn) {
            printf("  %s@%p\n", (*fn)->name, (*fn)->ptr);
            fn++;
        }
        ++vtbl;
        printf("\n");
    }

    printf("}\n\n");
}

void type_add_methods(TypeInfo *type, MethodDef *def, int size)
{
    MethodDef *m;
    for (int i = 0; i < size; i++) {
        m = def + i;
        type_add_cfunc(type, m->name, NULL, m->faddr);
    }
}

static PkgNode *_get_pkg(char *path)
{
    PkgNode key = { .name = path };
    hashmap_entry_init(&key, str_hash(path));
    return hashmap_get(all_pkgs, &key);
}

void pkg_add_type(char *path, TypeInfo *type)
{
    PkgNode *pkg = _get_pkg(path);
    _add_type(pkg->map, type->name, type);
}

void pkg_add_var(char *path, char *name, TypeDesc *desc)
{
    PkgNode *pkg = _get_pkg(path);
    _add_var(pkg->map, name, desc);
}

void pkg_add_cfunc(char *path, char *name, TypeDesc *desc, void *ptr)
{
    PkgNode *pkg = _get_pkg(path);
    FuncNode *fn = _add_func(pkg->map, name, desc);
    if (!fn) return;
    fn->kind = MNODE_CFUNC_KIND;
    fn->ptr = ptr;
}

void pkg_add_kfunc(char *path, char *name, TypeDesc *desc, CodeInfo *code)
{
    PkgNode *pkg = _get_pkg(path);
    FuncNode *fn = _add_func(pkg->map, name, desc);
    if (!fn) return;
    fn->kind = MNODE_KFUNC_KIND;
    fn->ptr = code;
}

void init_core_pkg(void)
{
    all_pkgs = mn_map_new();

    PkgNode *pkg = mm_alloc_obj(pkg);
    pkg->name = "/";
    hashmap_entry_init(pkg, str_hash(pkg->name));
    pkg->map = mn_map_new();

    hashmap_put_absent(all_pkgs, pkg);

    init_any_type();
    init_string_type();
    init_array_type();
    init_reflect_types();

    type_show(&any_type);
}

#ifdef __cplusplus
}
#endif
