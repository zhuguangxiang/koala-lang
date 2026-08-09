/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "atom.h"
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

static char *mangle_type_name(char *base_name, Vector *tp_args)
{
    BUF(buf);
    buf_write_str(&buf, base_name);

    if (vector_size(tp_args) > 0) {
        buf_write_char(&buf, '<');
        TypeSpec *ts;
        vector_foreach(ts, tp_args) {
            type_spec_to_str(ts, &buf);
        }
        buf_write_char(&buf, '>');
    }

    char *mangled_name = atom_str(BUF_STR(buf));
    FINI_BUF(buf);
    return mangled_name;
}

static TypeSpec *inst_type_spec(TypeSpec *ts, Vector *tp_args, HashMap *stbl)
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
            TypeSpec *inst_arg = inst_type_spec(_arg_ts, tp_args, stbl);
            ASSERT(inst_arg);
            vector_push_back(_tp_args, &inst_arg);
        }
        Symbol *sym = get_symbol_by_id(ts->sym_id);
        InstanceSymbol *inst_sym = find_or_add_instance(stbl, sym, _tp_args);
        arg_ts = inst_sym->instance_ts;
    } else {
        arg_ts = ts;
    }
    return arg_ts;
}

Symbol *find_or_add_func_instance(FuncSymbol *origin, Vector *tp_args, HashMap *stbl)
{
    char *mangled_name = mangle_type_name(origin->name, tp_args);
    Symbol *sym = stbl_get(stbl, mangled_name);
    if (sym) {
        log_info("found existing func instance symbol '%s'", mangled_name);
        ASSERT(sym->kind == SYM_INSTANCE_FUNC);
        return sym;
    }

    log_info("added func instance symbol '%s' for '%s'", mangled_name, origin->name);

    Vector *real_arg_types = vector_create_ptr();

    ArgInfo *arg;
    vector_foreach(arg, origin->params) {
        if (!arg) continue;
        TypeSpec *arg_ts = inst_type_spec(arg->ts, tp_args, stbl);
        log_info("func instance %dth-arg type: '%s'", i__, arg_ts->signature);
        vector_push_back(real_arg_types, &arg_ts);
    }

    TypeSpec *ret_type = inst_type_spec(origin->ret, tp_args, stbl);
    log_info("func instance ret type: '%s'", ret_type->signature);

    sym = stbl_add_func_instance(stbl, origin, mangled_name, real_arg_types, ret_type);
    return sym;
}

#ifdef __cplusplus
}
#endif
