/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "log.h"
#include "parser.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _TpInfo {
    HashMapEntry hnode;
    char *name;
    TypeSpec *real;
} TpInfo;

static int __tpinfo_eq__(const TpInfo *a, const TpInfo *b)
{
    char *a_name = a->name;
    char *b_name = b->name;

    ASSERT(a_name);
    ASSERT(b_name);
    return !strcmp(a_name, b_name);
}

static void __tpinfo_free__(void *info, void *arg) { mm_free(info); }

static void init_tpinfo_map(HashMap *map) { hashmap_init(map, (HashMapEqualFunc)__tpinfo_eq__); }

static void fini_tpinfo_map(HashMap *map) { hashmap_fini(map, __tpinfo_free__, NULL); }

static int add_tpinfo(HashMap *map, char *name, TypeSpec *real)
{
    TpInfo *info = mm_alloc_obj(info);
    info->name = name;
    info->real = real;
    hashmap_entry_init(info, str_hash(name));
    void *old = hashmap_put(map, info);
    ASSERT(!old);
    return 0;
}

static TpInfo *get_tpinfo(HashMap *map, char *name)
{
    TpInfo key = { .name = name };
    hashmap_entry_init(&key, str_hash(name));
    void *ret = hashmap_get(map, &key);
    return (TpInfo *)ret;
}

static int infer_tp_from_type(HashMap *map, TypeSpec *param_ts, TypeSpec *arg_ts,
                              char *tp_owner_name, Loc loc, ParserState *ps);

Vector *infer_tp_from_call(FuncSymbol *fn_sym, KlassSymbol *cls_sym, CallExpr *call_exp,
                           ParserState *ps)
{
    Vector *tps = cls_sym ? &cls_sym->tps : &fn_sym->tps;
    char *tp_owner_name = cls_sym ? cls_sym->name : fn_sym->name;

    // no type parameters to infer, return empty vector
    if (vector_empty(tps)) return NULL;

    HashMap map;
    init_tpinfo_map(&map);

    Vector *result = vector_create_ptr();

    Vector *params = fn_sym->params;
    Vector *call_args = call_exp->args;

    ArgInfo *arg;
    vector_foreach(arg, params) {
        if (!arg) continue;

        if (arg->has_dfl_val) {
            // default value param, skip it and the rest params.
            // param with default value MUST not be generic type.
            log_info("param '%s' is key-word parameter, skip the rest.", arg->name);
            break;
        }

        TypeSpec *ts = arg->ts;

        Expr *exp = call_arg_for_param(call_args, i__, arg->name);
        if (!exp) {
            // no more arguments passed
            if (type_is_valist(ts)) {
                // var-arg can be empty, break the loop and check the rest params.
                log_info("param '%s' is var-arg, no more arguments passed, skip the rest.",
                         arg->name);
                break;
            } else {
                kl_error(call_exp->loc, "expected at least %d arguments in '%s' call, but %d got.",
                         vector_size(params), fn_sym->name, i__);
                goto error;
            }
        }

        /* a lambda argument is resolved from the parameter type after inference,
           so it has no type yet and cannot contribute one; when a type parameter
           is left uninferred the loop below reports it */
        if (!exp->ts) {
            log_info("argument %d has no type yet, skip it for inference.", i__);
            continue;
        }

        if (ts->kind == TYPE_GENERIC_VAR) {
            log_info("param '%s' is generic var '%s'", arg->name, ts->generic_var.name);
            ASSERT(str_equal(ts->generic_var.owner, tp_owner_name));

            TpInfo *info = get_tpinfo(&map, ts->generic_var.name);
            if (info) {
                if (!type_spec_compatible(info->real, exp->ts)) {
                    kl_error(call_exp->loc, "generic type '%s' is inferred as '%s', but got '%s'",
                             info->name, info->real->signature, exp->ts->signature);
                    goto error;
                }
            } else {
                add_tpinfo(&map, ts->generic_var.name, exp->ts);
            }
            continue;
        }

        if (ts->kind == TYPE_GENERIC_REF) {
            log_info("param '%s' is generic ref '%s'", arg->name, ts->signature);
            if (!infer_tp_from_generic_ref(&map, ts, exp->ts, tp_owner_name, call_exp->loc, ps))
                goto error;
            continue;
        }

        if (ts->kind == TYPE_PROTO) {
            log_info("param '%s' is proto '%s'", arg->name, ts->signature);
            if (!infer_tp_from_type(&map, ts, exp->ts, tp_owner_name, call_exp->loc, ps))
                goto error;
            continue;
        }

        if (ts->kind == TYPE_VA_LIST) {
            // var-arg must be last one parameter.
            log_info("param '%s' is var-arg", arg->name);
            NYI();
        }
    }

    TypeParamSymbol *tp_sym;
    vector_foreach(tp_sym, tps) {
        if (!tp_sym) continue;
        TpInfo *info = get_tpinfo(&map, tp_sym->name);
        if (!info) {
            kl_error(call_exp->loc, "cannot infer type parameter '%s' for class '%s'.",
                     tp_sym->name, tp_owner_name);
            goto error;
        }
        // TODO: check if info->real is compatible with tp_sym->bound
        log_info("inferred type parameter '%s' for func '%s':", tp_sym->name, tp_owner_name);
        log_type_spec(info->real);
        vector_push_back(result, &info->real);
    }

    fini_tpinfo_map(&map);
    return result;

error:
    fini_tpinfo_map(&map);
    vector_destroy(result);
    return NULL;
}

static int get_generic_var_index(HashMap *stbl, TypeSpec *ts)
{
    ASSERT(ts->kind == TYPE_GENERIC_VAR);
    Symbol *sym = stbl_get(stbl, ts->generic_var.owner);
    ASSERT(sym);
    ASSERT(sym->kind == SYM_CLASS || sym->kind == SYM_TRAIT);

    Symbol *_sym = stbl_get(sym->stbl, ts->generic_var.name);
    ASSERT(_sym && _sym->kind == SYM_TYPE_PARAM);
    TypeParamSymbol *tp = (TypeParamSymbol *)_sym;
    ASSERT(tp->index >= 0);
    return tp->index;
}

static Symbol *_get_symbol_by_name(char *pkg, char *name, ParserModule *pm)
{
    HashMap *stbl = pm->stbl;
    if (pkg) {
        Symbol *pkg_sym = stbl_get(pm->imported, pkg);
        if (!pkg_sym) {
            PkgSymbol *_sym = import_package(pm, pkg);
            if (!_sym) {
                fprintf(stderr, "failed to import package: %s", pkg);
                return NULL;
            }
            pkg_sym = (Symbol *)_sym;
        }
        stbl = pkg_sym->stbl;
    }
    return stbl_get(stbl, name);
}

static InstanceSymbol *__instance_type_spec(HashMap *stbl, TypeSpec *ts, Vector *tp_args,
                                            ParserModule *pm)
{
    ASSERT(ts->kind == TYPE_GENERIC_REF);

    Symbol *origin_sym = get_symbol_by_id(ts->sym_id);
    if (!origin_sym) {
        origin_sym = stbl_get(stbl, ts->generic_ref.name);
    }

    if (!origin_sym) {
        origin_sym = _get_symbol_by_name(ts->generic_ref.pkg, ts->generic_ref.name, pm);
        if (!origin_sym) {
            fprintf(stderr, "type '%s.%s' is not found.\n", ts->generic_ref.pkg,
                    ts->generic_ref.name);
            return NULL;
        }
    }

    ASSERT(origin_sym->kind == SYM_CLASS || origin_sym->kind == SYM_TRAIT ||
           origin_sym->kind == SYM_INSTANCE);

    Vector *base_tp_args = vector_create_ptr();
    TypeSpec *spec_arg_ts;
    TypeSpec *arg_ts;
    vector_foreach(arg_ts, ts->generic_ref.args) {
        if (!arg_ts) continue;
        if (arg_ts->kind == TYPE_GENERIC_VAR) {
            if (arg_ts->generic_var.index < 0) {
                arg_ts->generic_var.index = get_generic_var_index(stbl, arg_ts);
            }
            spec_arg_ts = vector_get(tp_args, arg_ts->generic_var.index);
        } else if (arg_ts->kind == TYPE_GENERIC_REF) {
            // nested generic_ref type
            InstanceSymbol *spec_arg_sym = __instance_type_spec(stbl, arg_ts, tp_args, pm);
            spec_arg_ts = spec_arg_sym->instance_ts;
        } else {
            ASSERT(arg_ts->kind != TYPE_UNRESOLVED);
            spec_arg_ts = arg_ts;
        }
        vector_push_back(base_tp_args, &spec_arg_ts);
    }

    InstanceSymbol *inst_sym = find_or_add_instance(stbl, origin_sym, base_tp_args, pm);
    vector_destroy(base_tp_args);
    return inst_sym;
}

static inline int ts_is_generic(TypeSpec *ts)
{
    ASSERT(ts);
    if (ts->kind == TYPE_GENERIC_VAR) return 1;
    if (ts->kind == TYPE_GENERIC_REF) return 1;
    if (ts->kind == TYPE_OPTIONAL && ts->opt.src) return ts_is_generic(ts->opt.src);
    return 0;
}

int tps_are_generic(Vector *tp_args)
{
    TypeSpec *ts;
    vector_foreach(ts, tp_args) {
        if (!ts) continue;
        if (ts_is_generic(ts)) return 1;
    }
    return 0;
}

static Symbol *_unwrap_origin_symbol(Symbol *sym)
{
    while (sym) {
        if (sym->kind == SYM_INSTANCE) {
            sym = ((InstanceSymbol *)sym)->origin;
        } else if (sym->kind == SYM_IMPORTED) {
            sym = ((ImportedSymbol *)sym)->origin;
        } else {
            break;
        }
    }
    return sym;
}

/*
 * Resolves the origin class/trait symbol of a TYPE_GENERIC_REF typespec.
 * The sym_id of a generic_ref may point to an instance symbol (created by
 * find_or_add_instance), or it may be unset (e.g. loaded from klc).
 */
static Symbol *generic_ref_origin_sym(TypeSpec *ts, ParserModule *pm)
{
    ASSERT(ts && ts->kind == TYPE_GENERIC_REF);

    Symbol *sym = get_symbol_by_id(ts->sym_id);
    if (!sym) sym = _get_symbol_by_name(ts->generic_ref.pkg, ts->generic_ref.name, pm);

    sym = _unwrap_origin_symbol(sym);

    if (!sym) return NULL;
    if (sym->kind != SYM_CLASS && sym->kind != SYM_TRAIT) return NULL;
    return sym;
}

/*
 * Searches 'bases' for a type whose origin matches 'origin' and returns the
 * type arguments of it. The returned vector is borrowed from the symbol
 * table, do not free it.
 */
static Vector *find_matched_tp_args_in_bases(Symbol *origin, Vector *bases, ParserState *ps)
{
    if (!bases) return NULL;

    TypeSpec *base_ts;
    vector_foreach(base_ts, bases) {
        if (!base_ts) continue;
        Vector *ret = find_matched_tp_args(origin, base_ts, ps);
        if (ret) return ret;
    }
    return NULL;
}

/*
 * Finds the type arguments of 'arg_ts' (or one of its base types) which
 * correspond to the 'origin' class/trait. The returned vector is borrowed
 * from the symbol table, do not free it.
 */
Vector *find_matched_tp_args(Symbol *origin, TypeSpec *arg_ts, ParserState *ps)
{
    if (!arg_ts) return NULL;

    Symbol *sym = get_symbol_by_id(arg_ts->sym_id);
    if (!sym && arg_ts->kind == TYPE_GENERIC_REF) {
        sym = _get_symbol_by_name(arg_ts->generic_ref.pkg, arg_ts->generic_ref.name, ps->pm);
    }
    if (sym && sym->kind == SYM_IMPORTED) sym = ((ImportedSymbol *)sym)->origin;
    if (!sym) return NULL;

    if (arg_ts->kind == TYPE_GENERIC_REF) {
        if (sym->kind == SYM_INSTANCE) {
            InstanceSymbol *inst_sym = (InstanceSymbol *)sym;
            if (inst_sym->origin && inst_sym->origin->id == origin->id) {
                // e.g. param 'Iterator[T]' matches arg 'Iterator[int]'
                return arg_ts->generic_ref.args;
            }
            // specialized instance, e.g. 'MyIter[int]', search its bases
            return find_matched_tp_args_in_bases(origin, inst_sym->bases, ps);
        }
        if ((sym->kind == SYM_CLASS || sym->kind == SYM_TRAIT) && sym->id != origin->id) {
            return find_matched_tp_args_in_bases(origin, &((KlassSymbol *)sym)->bases, ps);
        }
        return NULL;
    }

    if (sym->kind == SYM_INSTANCE) {
        InstanceSymbol *inst_sym = (InstanceSymbol *)sym;
        if (inst_sym->origin && inst_sym->origin->id == origin->id) {
            // e.g. param 'Iterator[T]' matches arg 'Iterator[int]'
            return inst_sym->tp_args;
        }
        // specialized instance, e.g. 'MyIter[int]', search its bases
        return find_matched_tp_args_in_bases(origin, inst_sym->bases, ps);
    }

    if (sym->kind == SYM_CLASS || sym->kind == SYM_TRAIT) {
        if (sym->id == origin->id) {
            // bare origin type, e.g. 'self' inside 'class dict[K, V]': its type
            // arguments are the class's own type parameters.
            KlassSymbol *kls = (KlassSymbol *)sym;
            if (vector_empty(&kls->tps)) return NULL;
            Vector *self_args = vector_create_ptr();
            TypeParamSymbol *tp_sym;
            vector_foreach(tp_sym, &kls->tps) {
                TypeSpec *gv = generic_var_type_spec(tp_sym->name, tp_sym->index, tp_sym->id,
                                                     tp_sym->owner->name);
                vector_push_back(self_args, &gv);
            }
            return self_args;
        }
        // normal class, e.g. 'Foo' which inherits 'Iterator[int]'
        return find_matched_tp_args_in_bases(origin, &((KlassSymbol *)sym)->bases, ps);
    }

    return NULL;
}

/*
 * Matches a parameter type 'param_ts' against the actual argument type 'arg_ts',
 * binding every type parameter of 'tp_owner_name' that occurs in 'param_ts'.
 *
 * A type parameter may live only inside a function-typed parameter, e.g. the 'U'
 * of 'fn func(T) U' in map[T, U]. Such a parameter is a TYPE_PROTO, so its args
 * and return type are matched structurally against the argument's proto type to
 * infer the bindings. Generic vars, generic refs and nested protos are handled
 * recursively; a concrete type carries nothing to infer.
 */
static int infer_tp_from_type(HashMap *map, TypeSpec *param_ts, TypeSpec *arg_ts,
                              char *tp_owner_name, Loc loc, ParserState *ps)
{
    if (!param_ts || !arg_ts) return 1;

    if (param_ts->kind == TYPE_GENERIC_VAR) {
        ASSERT(str_equal(param_ts->generic_var.owner, tp_owner_name));

        TpInfo *info = get_tpinfo(map, param_ts->generic_var.name);
        if (info) {
            if (!type_spec_compatible(info->real, arg_ts)) {
                kl_error(loc, "generic type '%s' is inferred as '%s', but got '%s'", info->name,
                         info->real->signature, arg_ts->signature);
                return 0;
            }
        } else {
            add_tpinfo(map, param_ts->generic_var.name, arg_ts);
        }
        return 1;
    }

    if (param_ts->kind == TYPE_GENERIC_REF) {
        return infer_tp_from_generic_ref(map, param_ts, arg_ts, tp_owner_name, loc, ps);
    }

    if (param_ts->kind == TYPE_PROTO) {
        // a non-proto argument is a type mismatch, reported later by check_call_args()
        if (arg_ts->kind != TYPE_PROTO) return 1;

        Vector *param_args = param_ts->proto_type.args;
        Vector *arg_args = arg_ts->proto_type.args;
        int num_args = vector_size(param_args);
        if (num_args != vector_size(arg_args)) return 1;

        for (int i = 0; i < num_args; i++) {
            if (!infer_tp_from_type(map, vector_get(param_args, i), vector_get(arg_args, i),
                                    tp_owner_name, loc, ps))
                return 0;
        }
        return infer_tp_from_type(map, param_ts->proto_type.ret, arg_ts->proto_type.ret,
                                  tp_owner_name, loc, ps);
    }

    /* concrete type: compatibility is checked later by check_call_args() */
    return 1;
}

/*
 * Binds the type parameters found in the arguments of 'param_ts' (a
 * TYPE_GENERIC_REF) to the corresponding types in 'matched_args'.
 */
static int bind_tp_from_matched_args(HashMap *map, TypeSpec *param_ts, Vector *matched_args,
                                     char *tp_owner_name, Loc loc, ParserState *ps)
{
    Vector *param_args = param_ts->generic_ref.args;
    int num_params = vector_size(param_args);
    int num_matched = vector_size(matched_args);

    if (num_params != num_matched) {
        kl_error(loc, "type '%s' has %d type parameter(s), but %d type argument(s) matched.",
                 param_ts->signature, num_params, num_matched);
        return 0;
    }

    for (int i = 0; i < num_params; i++) {
        if (!infer_tp_from_type(map, vector_get(param_args, i), vector_get(matched_args, i),
                                tp_owner_name, loc, ps))
            return 0;
    }

    return 1;
}

/*
 * Infers type parameter bindings from a call argument whose corresponding
 * parameter type is a TYPE_GENERIC_REF, e.g. param 'Iterator[T]' with a class
 * argument which inherits 'Iterator[int]' infers 'T' as int.
 */
int infer_tp_from_generic_ref(HashMap *map, TypeSpec *param_ts, TypeSpec *arg_ts,
                              char *tp_owner_name, Loc loc, ParserState *ps)
{
    ASSERT(param_ts->kind == TYPE_GENERIC_REF);

    if (!tps_are_generic(param_ts->generic_ref.args)) {
        // closed generic ref, nothing to infer, its compatibility
        // is checked later by check_call_args().
        return 1;
    }

    Symbol *origin = generic_ref_origin_sym(param_ts, ps->pm);
    if (!origin) {
        kl_error(loc, "cannot resolve the origin type of '%s'.", param_ts->signature);
        return 0;
    }

    Vector *matched_args = find_matched_tp_args(origin, arg_ts, ps);
    if (!matched_args) {
        kl_error(loc, "cannot infer type parameter(s) of '%s': type '%s' does not match '%s'.",
                 tp_owner_name, arg_ts ? arg_ts->signature : "none", param_ts->signature);
        return 0;
    }

    log_info("param type '%s' matched arg type '%s' with %d type argument(s)", param_ts->signature,
             arg_ts->signature, vector_size(matched_args));

    return bind_tp_from_matched_args(map, param_ts, matched_args, tp_owner_name, loc, ps);
}

InstanceSymbol *find_or_add_instance(HashMap *stbl, Symbol *origin, Vector *tp_args,
                                     ParserModule *pm)
{
    ASSERT(origin->kind == SYM_CLASS || origin->kind == SYM_TRAIT || origin->kind == SYM_INSTANCE);

    if (origin->kind == SYM_INSTANCE) {
        // inherited instance, e.g. List[int] is an inherited instance of List[T]
        InstanceSymbol *origin_inst = (InstanceSymbol *)origin;
        origin = origin_inst->origin;
    }

    char *mangled_name = mangle_type_name(origin->name, tp_args);
    Symbol *sym = stbl_get(stbl, mangled_name);
    if (sym) {
        log_info("found existing instance symbol '%s'(generic=%d)", mangled_name,
                 sym->flags & SYM_FLAGS_GENERIC ? 1 : 0);
        ASSERT(sym->kind == SYM_INSTANCE);
        return (InstanceSymbol *)sym;
    }

    int generic = tps_are_generic(tp_args);

    if (generic) {
        log_info("added generic instance symbol '%s' for '%s'", mangled_name, origin->name);
        TypeSpec *instance_ts = generic_ref_type_spec(origin->path, origin->name, tp_args, -1);
        sym = stbl_add_instance(stbl, origin, mangled_name, tp_args, instance_ts);
    } else {
        log_info("added instance symbol '%s' for '%s'", mangled_name, origin->name);
        TypeSpec *instance_ts = klass_type_spec(origin->path, mangled_name);
        sym = stbl_add_instance(stbl, origin, mangled_name, tp_args, instance_ts);
    }

    InstanceSymbol *inst_sym = (InstanceSymbol *)sym;
    KlassSymbol *kls_sym = (KlassSymbol *)origin;

    // set instance bases
    if (!vector_empty(&kls_sym->bases)) {
        log_info("updating instance bases for '%s'", mangled_name);

        Vector *_tp_args = tp_args;

        if (!strcmp(origin->name, "tuple")) {
            // tuple instance
            // compute ...T for bases, methods parameters or return type
            log_info("handling tuple instance '%s'", mangled_name);
            TypeSpec *infer_ts = find_lub(tp_args);
            _tp_args = vector_create_ptr();
            inst_sym->arg = infer_ts; // pass infer type to instance symbol for later use
            vector_push_back(_tp_args, &infer_ts);
            log_info("tuple instance inferred type: '%s'", infer_ts->signature);
        }

        inst_sym->bases = vector_create_ptr();
        TypeSpec *base_ts;
        vector_foreach(base_ts, &kls_sym->bases) {
            if (!base_ts) continue;
            if (base_ts->kind == TYPE_KLASS) {
                vector_push_back(inst_sym->bases, &base_ts);
            } else if (base_ts->kind == TYPE_MANGLED) {
                Vector *__tp_args = vector_create_ptr();
                TypeSpec *_base_ts;
                vector_foreach(_base_ts, base_ts->mangled.args) {
                    Symbol *_origin = get_symbol_by_id(_base_ts->sym_id);
                    InstanceSymbol *_base_sym = find_or_add_instance(stbl, _origin, _tp_args, pm);
                    _base_ts = _base_sym->instance_ts;
                    vector_push_back(__tp_args, &_base_ts);
                    ASSERT(_base_ts->kind == TYPE_KLASS || _base_ts->kind == TYPE_GENERIC_REF);
                }
                Symbol *__origin = get_symbol_by_id(base_ts->sym_id);
                InstanceSymbol *__base_sym = find_or_add_instance(stbl, __origin, __tp_args, pm);
                base_ts = __base_sym->instance_ts;
                vector_push_back(inst_sym->bases, &base_ts);
                ASSERT(base_ts->kind == TYPE_KLASS || base_ts->kind == TYPE_GENERIC_REF);
            } else {
                // handle generic_ref base class/trait
                ASSERT(base_ts->kind == TYPE_GENERIC_REF);
                // specialize base class/trait
                InstanceSymbol *base_sym = __instance_type_spec(stbl, base_ts, _tp_args, pm);
                base_ts = base_sym->instance_ts;
                vector_push_back(inst_sym->bases, &base_ts);
                ASSERT(base_ts->kind == TYPE_KLASS || base_ts->kind == TYPE_GENERIC_REF);
            }
            log_info("updated instance base '%s' for '%s'", base_ts->klass_type.name, mangled_name);
        }

        if (_tp_args != tp_args) {
            vector_destroy(_tp_args);
        }
    }

    return inst_sym;
}

TypeSpec *type_spec_specialize(TypeSpec *ts, Vector *tp_args, HashMap *stbl, ParserModule *pm)
{
    TypeSpec *arg_ts;
    if (ts->kind == TYPE_GENERIC_VAR) {
        arg_ts = vector_get(tp_args, ts->generic_var.index);
        ASSERT(arg_ts);
    } else if (ts->kind == TYPE_GENERIC_REF) {
        Vector *_tp_args = vector_create_ptr();
        TypeSpec *_arg_ts;
        vector_foreach(_arg_ts, ts->generic_ref.args) {
            if (!_arg_ts) continue;
            // recursively instantiate the generic_ref args
            TypeSpec *inst_arg = type_spec_specialize(_arg_ts, tp_args, stbl, pm);
            ASSERT(inst_arg);
            vector_push_back(_tp_args, &inst_arg);
        }
        Symbol *sym = get_symbol_by_id(ts->sym_id);
        InstanceSymbol *inst_sym = find_or_add_instance(stbl, sym, _tp_args, pm);
        arg_ts = inst_sym->instance_ts;
    } else if (ts->kind == TYPE_OPTIONAL) {
        TypeSpec *inst_inner = type_spec_specialize(ts->opt.src, tp_args, stbl, pm);
        arg_ts = optional_type_spec_intern(inst_inner);
    } else if (ts->kind == TYPE_VA_LIST) {
        TypeSpec *inst_src = type_spec_specialize(ts->va_list.src, tp_args, stbl, pm);
        arg_ts = va_list_type_spec_intern(inst_src);
    } else if (ts->kind == TYPE_UNION) {
        Vector *inst_args = vector_create_ptr();
        TypeSpec *_arg_ts;
        vector_foreach(_arg_ts, ts->union_type.args) {
            TypeSpec *inst_arg = type_spec_specialize(_arg_ts, tp_args, stbl, pm);
            vector_push_back(inst_args, &inst_arg);
        }
        arg_ts = union_type_spec_intern(inst_args);
    } else if (ts->kind == TYPE_PROTO) {
        Vector *args = vector_create_ptr();
        TypeSpec *arg;
        vector_foreach(arg, ts->proto_type.args) {
            TypeSpec *inst_arg = type_spec_specialize(arg, tp_args, stbl, pm);
            vector_push_back(args, &inst_arg);
        }
        TypeSpec *ret = type_spec_specialize(ts->proto_type.ret, tp_args, stbl, pm);
        arg_ts = func_type_spec_intern(args, ret);
    } else {
        arg_ts = ts;
    }
    return arg_ts;
}

static int union_has_exact_member(TypeSpec *union_ts, TypeSpec *ts)
{
    TypeSpec *member;
    vector_foreach(member, union_ts->union_type.args) {
        if (type_spec_equal_exact(member, ts)) return 1;
    }

    return 0;
}

/*
 * A class parameter that is a union is valid when it takes everything the trait's parameter
 * declares:
 *
 *     trait Collection[T] { func __contains__(item T) bool }
 *
 *     class bytes : Collection[uint8] {
 *         pub func __contains__(value uint8 | bytes) bool {}    // ok, it takes a uint8
 *     }
 *
 *     trait P { func f(x int | str) }
 *
 *     class A : P { pub func f(x int | str | bool) {} }         // ok, it takes both members
 *     class B : P { pub func f(x int | bool) {} }               // error, it does not take a str
 */
static int union_param_takes_trait_param(TypeSpec *union_ts, TypeSpec *trait_ts)
{
    if (trait_ts->kind != TYPE_UNION) return union_has_exact_member(union_ts, trait_ts);

    TypeSpec *member;
    vector_foreach(member, trait_ts->union_type.args) {
        if (!union_has_exact_member(union_ts, member)) return 0;
    }

    return 1;
}

static int intf_param_matches(TypeSpec *expected, TypeSpec *actual)
{
    if (actual->kind == TYPE_UNION) return union_param_takes_trait_param(actual, expected);

    return type_spec_equal_exact(expected, actual);
}

static void intf_param_error(FuncSymbol *fn, KlassSymbol *trait, int index, TypeSpec *expected,
                             ParserState *ps)
{
    ArgInfo *arg = vector_get(fn->params, index);

    BUF(expected_buf);
    type_spec_print(expected, &expected_buf);
    BUF(actual_buf);
    type_spec_print(arg->ts, &actual_buf);

    kl_error(fn->loc,
             "method '%s' does not match trait '%s': parameter %d expected '%s', but got '%s'.",
             fn->name, trait->name, index + 1, BUF_STR(expected_buf), BUF_STR(actual_buf));

    FINI_BUF(expected_buf);
    FINI_BUF(actual_buf);
}

static void intf_return_error(FuncSymbol *fn, KlassSymbol *trait, TypeSpec *expected,
                              ParserState *ps)
{
    BUF(expected_buf);
    type_spec_print(expected, &expected_buf);
    BUF(actual_buf);
    type_spec_print(fn->ret, &actual_buf);

    kl_error(fn->loc,
             "method '%s' does not match trait '%s': return type expected '%s', but got '%s'.",
             fn->name, trait->name, BUF_STR(expected_buf), BUF_STR(actual_buf));

    FINI_BUF(expected_buf);
    FINI_BUF(actual_buf);
}

/*
 * Check a class method against the trait method it implements. Parameters and the return type are
 * invariant, with `T` bound to the type argument of the class:
 *
 *     trait Iterable[T] { func iter(step = 1) Iterator[T] }
 *
 *     class Ok : Iterable[uint8] {
 *         pub func iter(step = 1) Iterator[uint8] { ... }
 *     }
 *
 *     class Wrong : Iterable[uint8] {
 *         pub func iter(step = 1) Iterator[int] { ... }    // error: return type
 *     }
 */
static void check_intf_method(KlassSymbol *kls, KlassSymbol *trait, FuncSymbol *trait_fn,
                              Symbol *method, Vector *tp_args, ParserState *ps)
{
    /* the trait's own default, or a method the class does not implement */
    if (method->kind == SYM_INHERITED || method == (Symbol *)trait_fn) return;

    if (method->kind != SYM_FUNC) {
        kl_error(method->loc,
                 "'%s' of class '%s' is not a method, but trait '%s' declares it as one.",
                 method->name, kls->name, trait->name);
        return;
    }

    FuncSymbol *fn = (FuncSymbol *)method;

    if (fn->flags & SYM_FLAGS_STATIC) {
        kl_error(fn->loc, "method '%s' does not match trait '%s': it must not be static.", fn->name,
                 trait->name);
        return;
    }

    /* an `infer` type parameter is deduced by the compiler, not declared by the trait */
    if (!vector_empty(&trait_fn->tps) || !vector_empty(&fn->tps)) return;

    int size = vector_size(trait_fn->params);
    if (size != vector_size(fn->params)) {
        kl_error(fn->loc,
                 "method '%s' does not match trait '%s': expected %d parameters, but got %d.",
                 fn->name, trait->name, size, vector_size(fn->params));
        return;
    }

    HashMap *stbl = ps->pm->stbl;

    for (int i = 0; i < size; i++) {
        ArgInfo *trait_arg = vector_get(trait_fn->params, i);
        ArgInfo *arg = vector_get(fn->params, i);

        TypeSpec *expected = type_spec_specialize(trait_arg->ts, tp_args, stbl, ps->pm);
        if (!intf_param_matches(expected, arg->ts)) {
            intf_param_error(fn, trait, i, expected, ps);
        }
    }

    TypeSpec *expected_ret = type_spec_specialize(trait_fn->ret, tp_args, stbl, ps->pm);
    if (!type_spec_equal_exact(expected_ret, fn->ret)) {
        intf_return_error(fn, trait, expected_ret, ps);
    }
}

/*
 * Walk one entry of a class's interface table. An inherited slot is skipped: the trait that
 * declares it has its own entry, which reports the mismatch once.
 */
static void check_intf_entry(KlassSymbol *kls, IntfEntry *entry, ParserState *ps)
{
    KlassSymbol *trait = (KlassSymbol *)entry->trait;

    Vector *tp_args = find_matched_tp_args(entry->trait, kls->instance_ts, ps);
    if (!vector_empty(&trait->tps) && !tp_args) return;

    int size = vector_size(trait->funcs);
    for (int i = 0; i < size; i++) {
        Symbol *slot = vector_get(trait->funcs, i);
        if (slot->kind == SYM_INHERITED) continue;

        Symbol *method = vector_get(&entry->methods, i);
        check_intf_method(kls, trait, (FuncSymbol *)slot, method, tp_args, ps);
    }
}

static TypeSpec *find_generic_trait(Vector *seen, Symbol *origin)
{
    TypeSpec *ts;
    vector_foreach(ts, seen) {
        InstanceSymbol *inst = (InstanceSymbol *)get_symbol_by_id(ts->sym_id);
        if ((Symbol *)inst->origin == origin) return ts;
    }

    return NULL;
}

/* Print an instance as it is written, `G[list[int64]]`, not by its signature `G<...>`. */
static void generic_trait_print(TypeSpec *ts, Buffer *buf)
{
    Symbol *sym = NULL;
    if (ts->sym_id >= 0) sym = get_symbol_by_id(ts->sym_id);

    if (!sym || sym->kind != SYM_INSTANCE) {
        type_spec_print(ts, buf);
        return;
    }

    InstanceSymbol *inst = (InstanceSymbol *)sym;
    buf_write_str(buf, inst->origin->name);
    buf_write_char(buf, '[');

    TypeSpec *arg;
    vector_foreach(arg, inst->tp_args) {
        if (i__ > 0) buf_write_str(buf, ", ");
        generic_trait_print(arg, buf);
    }

    buf_write_char(buf, ']');
}

/*
 * A class implements a generic trait with one list of type arguments, directly or through another
 * trait. The same list reached twice is the same trait:
 *
 *     trait G[T] { func g(x T) int }
 *     trait L[T] : G[T] {}
 *     trait R[T] : G[T] {}
 *
 *     class Ok : L[int] & R[int] {}          // ok, G[int] twice
 *     class Twice : G[int] & G[str] {}       // error
 *     class Inherited : L[int] & G[str] {}   // error
 */
static void check_generic_traits_once(KlassSymbol *kls, Vector *bases, Vector *seen,
                                      ParserState *ps)
{
    TypeSpec *base_ts;
    vector_foreach(base_ts, bases) {
        if (!base_ts) continue;

        Symbol *base_sym = get_symbol_by_id(base_ts->sym_id);
        if (!base_sym) continue;
        if (base_sym->kind == SYM_IMPORTED) base_sym = ((ImportedSymbol *)base_sym)->origin;

        if (base_sym->kind != SYM_INSTANCE) {
            check_generic_traits_once(kls, &((KlassSymbol *)base_sym)->bases, seen, ps);
            continue;
        }

        InstanceSymbol *inst = (InstanceSymbol *)base_sym;
        TypeSpec *other_ts = find_generic_trait(seen, (Symbol *)inst->origin);
        if (!other_ts) {
            vector_push_back(seen, &base_ts);
        } else if (get_symbol_by_id(other_ts->sym_id) != base_sym) {
            BUF(first_buf);
            generic_trait_print(other_ts, &first_buf);
            BUF(second_buf);
            generic_trait_print(base_ts, &second_buf);

            kl_error(
                kls->loc,
                "class '%s' implements trait '%s' with different type arguments: '%s' and '%s'.",
                kls->name, inst->origin->name, BUF_STR(first_buf), BUF_STR(second_buf));

            FINI_BUF(first_buf);
            FINI_BUF(second_buf);
        }

        if (inst->bases) check_generic_traits_once(kls, inst->bases, seen, ps);
    }
}

static int cmp_symbol_by_id(const void *a, const void *b)
{
    Symbol **sym1 = (Symbol **)a;
    Symbol **sym2 = (Symbol **)b;
    return (*sym1)->id - (*sym2)->id;
}

int check_intf_methods(ParserModule *pm)
{
    /* binding a type argument can add instances to the symbol table, so walk a copy */
    Vector classes = VECTOR_INIT_PTR;

    HashMapIter it;
    hashmap_iter_init(pm->stbl, &it, 0, 1);
    while (hashmap_iter_next(&it)) {
        Symbol *sym = (Symbol *)hashmap_iter_entry(&it);
        if (sym->kind == SYM_CLASS) vector_push_back(&classes, &sym);
    }

    /* report in the order of the declarations */
    vector_sort(&classes, cmp_symbol_by_id);

    int errors = 0;

    KlassSymbol *kls;
    vector_foreach(kls, &classes) {
        ParserState *ps = kls->ps;
        int errors_before = ps->errors;

        Vector seen = VECTOR_INIT_PTR;
        check_generic_traits_once(kls, &kls->bases, &seen, ps);
        vector_fini(&seen);

        /* one interface table entry cannot stand for two lists of type arguments */
        if (ps->errors == errors_before) {
            IntfEntry *entry;
            vector_foreach(entry, &kls->intf_table) {
                check_intf_entry(kls, entry, ps);
            }
        }

        errors += ps->errors - errors_before;
    }

    vector_fini(&classes);
    return errors;
}

Symbol *find_or_add_func_instance(FuncSymbol *origin, Vector *tp_args, HashMap *stbl,
                                  ParserState *ps)
{
    // Type args with type variables (e.g. a generic body calling another generic func) are not a
    // concrete instantiation, so never routed to a specialized func: always the erased instance.
    // Keyed by the type-name form 'f<...>', which no declared func name can match.
    int generic = tps_are_generic(tp_args);

    // A method has no specialized func, and its instance is keyed by its class too, e.g.
    // 'Box.pass<int64>': by 'pass_int64' it would be a top-level func of that name, or the
    // instance of a method of the same name in another class.
    Symbol *parent = origin->parent;
    int is_method = parent && (parent->kind == SYM_CLASS || parent->kind == SYM_INSTANCE ||
                               parent->kind == SYM_TRAIT);

    char *mangled_name;
    if (is_method) {
        BUF(qualified);
        buf_write_str(&qualified, parent->name);
        buf_write_char(&qualified, '.');
        buf_write_str(&qualified, origin->name);
        mangled_name = mangle_type_name(BUF_STR(qualified), tp_args);
        FINI_BUF(qualified);
    } else if (generic) {
        mangled_name = mangle_type_name(origin->name, tp_args);
    } else {
        mangled_name = mangle_func_name(origin->name, tp_args);
    }

    Symbol *sym = stbl_get(stbl, mangled_name);
    if (sym) {
        log_info("found existing func instance symbol '%s'", mangled_name);
        ASSERT(sym->kind == SYM_INSTANCE_FUNC || sym->kind == SYM_FUNC);
        return sym;
    }

    if (!generic && !is_method && (origin->flags & SYM_FLAGS_EXT)) {
        // try to find mangled_name function from external library
        sym = find_ext_symbol(ps, origin->path, mangled_name);
        if (sym) {
            log_info("found existing external mangled func symbol '%s'", mangled_name);
            ASSERT(sym->kind == SYM_FUNC);
            return sym;
        }
        // fall back to add an instance function
    }

    log_info("added func instance symbol '%s' for '%s'", mangled_name, origin->name);

    Vector *real_arg_types = vector_create_ptr();

    ArgInfo *arg;
    vector_foreach(arg, origin->params) {
        if (!arg) continue;
        TypeSpec *arg_ts = type_spec_specialize(arg->ts, tp_args, stbl, ps->pm);
        log_info("func instance %dth-arg type: '%s'", i__, arg_ts->signature);
        vector_push_back(real_arg_types, &arg_ts);
    }

    TypeSpec *ret_type = type_spec_specialize(origin->ret, tp_args, stbl, ps->pm);
    log_info("func instance ret type: '%s'", ret_type->signature);

    sym = stbl_add_func_instance(stbl, origin, mangled_name, real_arg_types, ret_type);
    return sym;
}

void add_specialized_func(FuncSymbol *origin, Vector *tp_args_list, ParserState *ps)
{
    Vector *tp_args;
    vector_foreach(tp_args, tp_args_list) {
        char *mangled_name = mangle_func_name(origin->name, tp_args);
        stbl_add_func(ps->pm->stbl, mangled_name, NULL, NULL, origin->flags);
        log_info("added specialized func symbol '%s' for '%s'", mangled_name, origin->name);
    }
}

void update_specialized_func(FuncSymbol *origin, Vector *tp_args_list, ParserState *ps)
{
    ParserModule *pm = ps->pm;
    HashMap *stbl = pm->stbl;

    Vector *tp_args;
    vector_foreach(tp_args, tp_args_list) {
        char *name = mangle_func_name(origin->name, tp_args);
        Symbol *sym = stbl_get(stbl, name);
        ASSERT(sym);

        Vector *arg_types = vector_create_ptr();
        Vector *arg_infos = vector_create_ptr();

        ArgInfo *arg;
        vector_foreach(arg, origin->params) {
            if (!arg) continue;
            TypeSpec *arg_ts = type_spec_specialize(arg->ts, tp_args, stbl, pm);
            log_info("specialized func %dth-arg type: '%s'", i__, arg_ts->signature);
            vector_push_back(arg_types, &arg_ts);

            log_info("specialized func %dth-arg info: '%s'", i__, arg->name);
            ArgInfo *_param = mm_alloc_obj(_param);
            _param->name = arg->name;
            _param->ts = arg_ts;
            vector_push_back(arg_infos, &_param);
        }

        TypeSpec *ret_type = type_spec_specialize(origin->ret, tp_args, stbl, pm);
        log_info("specialized func ret type: '%s'", ret_type->signature);
        FuncSymbol *fn_sym = (FuncSymbol *)sym;
        fn_sym->ret = ret_type;
        fn_sym->params = arg_infos;
        fn_sym->ts = func_type_spec(arg_types, ret_type);
    }
}

#ifdef __cplusplus
}
#endif
