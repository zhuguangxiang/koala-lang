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

        Expr *exp = vector_get(call_args, i__);
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
            NYI();
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
    return 0;
}

static inline int tps_are_generic(Vector *tp_args)
{
    TypeSpec *ts;
    vector_foreach(ts, tp_args) {
        if (!ts) continue;
        if (ts_is_generic(ts)) return 1;
    }
    return 0;
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
            log_info("updated instance base '%s' for '%s'", base_ts->klass_type.name,
                     mangled_name);
        }

        if (_tp_args != tp_args) {
            vector_destroy(_tp_args);
        }
    }

    return inst_sym;
}

static TypeSpec *inst_type_spec(TypeSpec *ts, Vector *tp_args, HashMap *stbl, ParserModule *pm)
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
            TypeSpec *inst_arg = inst_type_spec(_arg_ts, tp_args, stbl, pm);
            ASSERT(inst_arg);
            vector_push_back(_tp_args, &inst_arg);
        }
        Symbol *sym = get_symbol_by_id(ts->sym_id);
        InstanceSymbol *inst_sym = find_or_add_instance(stbl, sym, _tp_args, pm);
        arg_ts = inst_sym->instance_ts;
    } else {
        arg_ts = ts;
    }
    return arg_ts;
}

Symbol *find_or_add_func_instance(FuncSymbol *origin, Vector *tp_args, HashMap *stbl,
                                  ParserState *ps)
{
    char *mangled_name = mangle_func_name(origin->name, tp_args);
    Symbol *sym = stbl_get(stbl, mangled_name);
    if (sym) {
        log_info("found existing func instance symbol '%s'", mangled_name);
        ASSERT(sym->kind == SYM_INSTANCE_FUNC || sym->kind == SYM_FUNC);
        return sym;
    }

    if (origin->flags & SYM_FLAGS_EXT) {
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
        TypeSpec *arg_ts = inst_type_spec(arg->ts, tp_args, stbl, ps->pm);
        log_info("func instance %dth-arg type: '%s'", i__, arg_ts->signature);
        vector_push_back(real_arg_types, &arg_ts);
    }

    TypeSpec *ret_type = inst_type_spec(origin->ret, tp_args, stbl, ps->pm);
    log_info("func instance ret type: '%s'", ret_type->signature);

    sym = stbl_add_func_instance(stbl, origin, mangled_name, real_arg_types, ret_type);
    return sym;
}

void update_specialized_func(FuncSymbol *origin, char *name, Vector *tp_args, ParserState *ps)
{
    ParserModule *pm = ps->pm;
    HashMap *stbl = pm->stbl;
    Symbol *sym = stbl_get(stbl, name);
    ASSERT(sym);

    Vector *arg_types = vector_create_ptr();
    Vector *arg_infos = vector_create_ptr();

    ArgInfo *arg;
    vector_foreach(arg, origin->params) {
        if (!arg) continue;
        TypeSpec *arg_ts = inst_type_spec(arg->ts, tp_args, stbl, pm);
        log_info("specialized func %dth-arg type: '%s'", i__, arg_ts->signature);
        vector_push_back(arg_types, &arg_ts);

        log_info("specialized func %dth-arg info: '%s'", i__, arg->name);
        ArgInfo *_param = mm_alloc_obj(_param);
        _param->name = arg->name;
        _param->ts = arg_ts;
        vector_push_back(arg_infos, &_param);
    }

    TypeSpec *ret_type = inst_type_spec(origin->ret, tp_args, stbl, pm);
    log_info("specialized func ret type: '%s'", ret_type->signature);
    FuncSymbol *fn_sym = (FuncSymbol *)sym;
    fn_sym->ret = ret_type;
    fn_sym->params = arg_infos;
    fn_sym->ts = func_type_spec(arg_types, ret_type);
}

#ifdef __cplusplus
}
#endif
