/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.call
 */

#include "atom.h"
#include "cfuncobject.h"
#include "exception.h"
#include "log.h"
#include "stringobject.h"
#include "tupleobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static TypeObject *mapping[] = {
    &none_type,   &exc_type,   &bool_type,  &int8_type,   &int16_type,
    &int32_type,  &int64_type, &uint8_type, &uint16_type, &uint32_type,
    &uint64_type, NULL,        NULL,        NULL,         NULL,
};

TypeObject *object_typeof(Value *val)
{
    if (is_value(val)) {
        ASSERT(val->tag >= 0 && val->tag < COUNT_OF(mapping));
        TypeObject *tp = mapping[val->tag];
        ASSERT(tp);
        return tp;
    } else {
        return OB_TYPE(to_obj(val));
    }
}

Value object_tostr(Value *self)
{
    TypeObject *tp = object_typeof(self);
    ASSERT(tp->str);
    return tp->str(self);
}

Value object_call(Value *self, Value *args, int nargs, Object *names)
{
    TypeObject *tp = object_typeof(self);
    ASSERT(tp);
    CallFunc call = tp->call;
    if (!call) {
        /* raise an error */
        raise_exc_fmt("'%s' is not callable", tp->name);
        return error_value;
    }

    return call(self, args, nargs, names);
}

Object *object_lookup(Value *obj, char *name)
{
    TypeObject *tp = object_typeof(obj);
    Object *fn = sym_tbl_find(&tp->map, name, strlen(name));
    return fn;
}

typedef struct _VTableMapEntry {
    HashMapEntry hnode;
    TypeObject *tp;
    VTable *vtbl;
} VTableMapEntry;

static int __vtbl_map_eq__(void *e1, void *e2)
{
    VTableMapEntry *n1 = e1;
    VTableMapEntry *n2 = e2;
    return n1->tp == n2->tp;
}

static int tp_exists(Vector *vec, TypeObject *tp)
{
    TypeObject *existing_tp;
    vector_foreach(existing_tp, vec) {
        if (!existing_tp) continue;
        if (existing_tp == tp) {
            return 1;
        }
    }
    return 0;
}

/* compute primary inheritance path */
static void type_compute_pip(TypeObject *tp)
{
    Vector *pip = &tp->pip;
    if (vector_size(pip) > 0) return;

    TypeObject *any_tp;

    TypeObject *base_tp = vector_get(&tp->bases, 0);
    if (!base_tp) {
        any_tp = &any_type;
        if (!tp_exists(pip, any_tp)) {
            vector_push_back(pip, &any_tp);
        }
        // add itself
        if (!tp_exists(pip, tp)) {
            vector_push_back(pip, &tp);
        }
        return;
    }

    // compute base's pip first
    type_compute_pip(base_tp);

    any_tp = &any_type;
    if (!tp_exists(pip, any_tp)) {
        vector_push_back(pip, &any_tp);
    }

    // inherit from base's pip
    TypeObject *_tp;
    vector_foreach(_tp, &base_tp->pip) {
        if (!_tp) continue;
        // skip any type in pip
        if (_tp == &any_type) continue;

        if (!tp_exists(pip, _tp)) {
            vector_push_back(pip, &_tp);
        }
    }

    // add itself
    if (!tp_exists(pip, tp)) {
        vector_push_back(pip, &tp);
    }
}

static void type_compute_lro(TypeObject *tp)
{
    Vector *lro = &tp->lro;
    if (vector_size(lro) > 0) return;

    TypeObject *base_tp;
    vector_foreach(base_tp, &tp->bases) {
        if (!base_tp) continue;

        // compute base's lro first
        type_compute_lro(base_tp);
    }

    TypeObject *any_tp = &any_type;
    vector_push_back(lro, &any_tp);

    vector_foreach(base_tp, &tp->bases) {
        if (!base_tp) continue;

        // inherit from base's lro
        TypeObject *tp;
        vector_foreach(tp, &base_tp->lro) {
            if (!tp) continue;

            // check duplication
            if (!tp_exists(lro, tp)) {
                vector_push_back(lro, &tp);
            }
        }
    }

    // add self
    if (!tp_exists(lro, tp)) {
        vector_push_back(lro, &tp);
    }
}

static void type_compute_scm(TypeObject *tp)
{
    Vector *scm = &tp->scm;
    if (vector_size(scm) > 0) return;

    TypeObject *base_tp;
    vector_foreach(base_tp, &tp->lro) {
        if (!base_tp) continue;

        if (!tp_exists(&tp->pip, base_tp)) {
            vector_push_back(scm, &base_tp);
        }
    }
}

#ifndef NOLOG
static void print_vtbl_info(TypeObject *tp)
{
    printf("vtbl info for class/trait '%s':", tp->name);

    TypeObject *_tp;

    printf("\n  pip:");
    vector_foreach(_tp, &tp->pip) {
        if (!_tp) continue;
        if (i__ != 0) printf(" -> ");
        printf("%s", _tp->name);
    }

    printf("\n  lro:");
    vector_foreach(_tp, &tp->lro) {
        if (!_tp) continue;
        if (i__ != 0) printf(" -> ");
        printf("%s", _tp->name);
    }

    printf("\n  scm:");
    vector_foreach(_tp, &tp->scm) {
        if (!_tp) continue;
        if (i__ != 0) printf(" -> ");
        printf("%s", _tp->name);
    }
    printf("\n");
}
#else
#define print_vtbl_info(tp) \
    do { \
    } while (0)
#endif

static int _type_ready(TypeObject *tp)
{
    vector_init_ptr(&tp->bases);
    vector_init_ptr(&tp->fields);
    vector_init_ptr(&tp->methods);
    vector_init_ptr(&tp->vtables);
    init_sym_tbl(&tp->map);
    vector_init_ptr(&tp->pip);
    vector_init_ptr(&tp->lro);
    vector_init_ptr(&tp->scm);
    hashmap_init(&tp->vtable_map, __vtbl_map_eq__);

    BaseDef *base = tp->basedefs;
    while (base && base->tp) {
        TypeObject *base_tp = base->tp;
        if (type_ready(base_tp)) {
            return -1;
        }
        vector_push_back(&tp->bases, &base_tp);
        ++base;
    }

    type_compute_pip(tp);
    type_compute_lro(tp);
    type_compute_scm(tp);
    print_vtbl_info(tp);

    // add method to type
    MethodDef *def = tp->methdefs;
    while (def && def->name) {
        Object *cfunc = kl_new_cfunc(def, tp->module, tp);
        vector_push_back(&tp->methods, &cfunc);
        sym_tbl_add(&tp->map, def->name, strlen(def->name), cfunc);
        log_info("added method '%s' to class/trait '%s'", def->name, tp->name);
        ++def;
    }

    VTable *main_vtbl = mm_alloc_obj(main_vtbl);
    main_vtbl->type = tp;
    vector_init_ptr(&main_vtbl->methods);
    tp->vtbl = main_vtbl;

    TypeObject *base_tp;

    // inherit methods from bases
    vector_foreach(base_tp, &tp->lro) {
        if (!base_tp) continue;
        if (base_tp == tp) continue;

        Object *meth;
        vector_foreach(meth, &base_tp->methods) {
            if (!meth) continue;
            if (IS_CFUNC(meth)) {
                CFuncObject *cfunc = (CFuncObject *)meth;
                char *name = cfunc->def->name;
                Object *r = sym_tbl_find(&tp->map, name, strlen(name));
                if (!r) {
                    log_info("inherited method '%s' from '%s' to '%s'", name,
                             base_tp->name, tp->name);
                    sym_tbl_add(&tp->map, name, strlen(name), meth);
                }
            } else {
                NYI();
            }
        }
    }

    // add direct base methods to primary vtable
    // [size - 1] = self
    // [size - 2] = direct base
    int base_index = vector_size(&tp->pip) - 2;
    base_tp = vector_get(&tp->pip, base_index);

    if (base_tp) {
        VTable *base_vtbl = base_tp->vtbl;
        if (base_vtbl) {
            Object *meth;
            vector_foreach(meth, &base_vtbl->methods) {
                if (!meth) continue;
                if (IS_CFUNC(meth)) {
                    CFuncObject *cfunc = (CFuncObject *)meth;
                    char *name = cfunc->def->name;
                    Object *r = sym_tbl_find(&tp->map, name, strlen(name));
                    ASSERT(r);
                    vector_push_back(&main_vtbl->methods, &r);
                    log_info("added method '%s' to main vtbl of '%s' at %d slot", name,
                             tp->name, vector_size(&main_vtbl->methods) - 1);
                } else {
                    NYI();
                }
            }
        }
    }

    // add self methods to primary vtable
    if (tp->flags & TP_FLAGS_TRAIT) {
        Object *meth;
        vector_foreach(meth, &tp->methods) {
            if (!meth) continue;
            if (IS_CFUNC(meth)) {
                CFuncObject *cfunc = (CFuncObject *)meth;
                if (cfunc->ready) {
                    log_info(
                        "method '%s' of trait '%s' is already ready, skip adding to main "
                        "vtbl",
                        cfunc->def->name, tp->name);
                    continue;
                }
                vector_push_back(&main_vtbl->methods, &meth);
                log_info("added method '%s' to main vtbl of '%s' at %d slot",
                         cfunc->def->name, tp->name,
                         vector_size(&main_vtbl->methods) - 1);
                cfunc->ready = 1;
            } else {
                NYI();
            }
        }
    }

    // add vtable mapping
    if (tp->flags & TP_FLAGS_CLASS) {
        vector_foreach(base_tp, &tp->pip) {
            if (!base_tp) continue;
            if (base_tp == tp) continue;
            VTableMapEntry *e = mm_alloc_obj(e);
            hashmap_entry_init(e, mem_hash(base_tp, PTR_SIZE));
            e->tp = base_tp;
            e->vtbl = main_vtbl;
            hashmap_put_only(&tp->vtable_map, e);
            log_info("added main vtable mapping for '%s' to '%s'", base_tp->name,
                     tp->name);
        }
    }

    return 0;
}

int type_ready(TypeObject *tp)
{
    // The type is fully initialized.
    if (tp->flags & TP_FLAGS_READY) return 0;

    // The type is initializing, to prevent recursive ready calls
    if (tp->flags & TP_FLAGS_READYING) return 0;

    tp->flags |= TP_FLAGS_READYING;

    if (_type_ready(tp)) {
        tp->flags &= ~TP_FLAGS_READYING;
        return -1;
    }

    // mark the type is ready
    tp->flags |= TP_FLAGS_READY;
    tp->flags &= ~TP_FLAGS_READYING;
    return 0;
}

// static int get_name_index(Object *names, const char *name)
// {
//     Value *items = TUPLE_ITEMS(names);
//     int len = TUPLE_LEN(names);

//     for (int i = 0; i < len; i++) {
//         Object *sobj = to_obj(items + i);
//         ASSERT(IS_STR(sobj));
//         const char *s = STR_BUF(sobj);
//         if (!strcmp(s, name)) {
//             return i;
//         }
//     }

//     return -1;
// }

// /**
//  * Parse optional keyword arguments of this function.
//  *
//  * @return 0 successful, -1 error
//  *
//  * @param args The base pointer of arguments passed to this function.
//  * @param nargs The number of positional arguments passed to this function.
//  * @param names The tuple of keyword arguments' names passed to this function.
//  * @param npos The number of positional arguments defined by this function.
//  * @param kws The string array of acceptable keyword arguments' names defined by this
//  * function.
//  *
//  * @note
//  * The value of `nargs - npos` is the number of keyword arguments which are passed by
//  * positional arguments.
//  *
//  * It will be checked by compiler if there are arguments both have positional values
//  and
//  * keyword values.
//  */
// int kl_parse_kwargs(Value *args, int nargs, Object *names, int npos, const char **kws,
//                     ...)
// {
//     ASSERT(nargs >= npos);
//     Value *v;
//     va_list va_args;
//     va_start(va_args, kws);

//     // Parse keyword arguments which are passed by position.
//     for (int i = npos; i < nargs; i++) {
//         v = va_arg(va_args, Value *);
//         *v = *(args + i);
//         ++kws;
//     }

//     // Parse keyword arguments which are passed by keyword.
//     if (names) {
//         ASSERT(IS_TUPLE(names));
//         const char *kw;
//         while ((kw = *kws)) {
//             int i = get_name_index(names, kw);
//             v = va_arg(va_args, Value *);
//             if (i >= 0) {
//                 *v = *(args + nargs + i);
//             }
//             ++kws;
//         }
//     }

//     va_end(va_args);
//     return 0;
// }

/* object symbol entry */
typedef struct _SymEntry {
    HashMapEntry hnode;
    const char *key;
    int len;
    Object *obj;
} SymEntry;

static int _tbl_func_equal_(void *e1, void *e2)
{
    SymEntry *n1 = e1;
    SymEntry *n2 = e2;
    if (n1->len != n2->len) return 0;
    if (!strncmp(n1->key, n2->key, n1->len)) return 1;
    return 0;
}

void sym_tbl_add(HashMap *map, char *name, int len, Object *obj)
{
    SymEntry *e = mm_alloc_obj_fast(e);
    uint64_t hash = str_hash(name);
    hashmap_entry_init(e, hash);
    e->key = atom_nstr(name, len);
    e->len = len;
    e->obj = obj;
    int r = hashmap_put_absent(map, e);
    ASSERT(!r);
}

Object *sym_tbl_find(HashMap *map, char *name, int len)
{
    SymEntry entry = { .key = name, .len = len };
    uint64_t hash = mem_hash(name, len);
    hashmap_entry_init(&entry, hash);
    SymEntry *found = hashmap_get(map, &entry);
    return found ? found->obj : NULL;
}

void init_sym_tbl(HashMap *map) { hashmap_init(map, _tbl_func_equal_); }

Value intf_not_impl(Value *self)
{
    raise_exc_str("Interface not yet implemented");
    return error_value;
}

Value intf_not_impl_arg(Value *self, Value *arg)
{
    raise_exc_str("Interface not yet implemented");
    return error_value;
}

#ifdef __cplusplus
}
#endif
