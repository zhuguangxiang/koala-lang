/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.call
 */

#include "atom.h"
#include "cfuncobject.h"
#include "exception.h"
#include "stringobject.h"
#include "tupleobject.h"

#ifdef __cplusplus
extern "C" {
#endif

static TypeObject *mapping[] = {
    // &none_type, &exc_type,   &bool_type,  &int_type,   &int_type,
    // &int_type,  &int_type,   &int_type,   &int_type,   &int_type,
    // &int_type,  &float_type, &float_type, &float_type, &float_type,
};

TypeObject *object_typeof(Value *val)
{
    TypeObject *tp = mapping[val->tag];
    if (tp) return tp;
    return OB_TYPE(to_obj(val));
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

    // add method to type
    MethodDef *def = tp->methdefs;
    while (def && def->name) {
        Object *cfunc = kl_new_cfunc(def, tp->module, tp);
        sym_tbl_add(&tp->map, def->name, strlen(def->name), cfunc);
        ++def;
    }

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
