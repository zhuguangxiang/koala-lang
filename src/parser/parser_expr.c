/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "log.h"
#include "parser.h"
#include "utf8.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NOLOG
/* clang-format off */
#define log_type_spec(ts) do {        \
    BUF(buf);                           \
    type_spec_print(ts, &buf);          \
    log_info("  '%s'", BUF_STR(buf));   \
    FINI_BUF(buf);                      \
} while (0)
/* clang-format on */
#else
#define log_type_spec(ts) ((void *)(ts))
#endif

static void parse_ident(ParserState *ps, Expr *exp)
{
    IdentExpr *id_exp = (IdentExpr *)exp;
    Ident *id = &id_exp->id;
    Symbol *sym = find_symbol(ps, id);
    if (!sym) {
        kl_error(id->loc, "'%s' is not found", id->name);
        return;
    }

    if (sym->kind == SYM_VAR) {
        if (sym->status == SYM_UNRESOLVED) {
            parse_stmt(ps, sym->arg);
            exp->ts = sym->ts;
        } else if (sym->status == SYM_RESOLVING) {
            kl_error(id->loc, "circular reference detected for '%s'", id->name);
            return;
        } else {
            // do nothing
        }
    }

    if (sym->kind == SYM_FUNC) {
        FuncSymbol *fn_sym = (FuncSymbol *)sym;
        if (!fn_sym->ts) {
            log_info("update func '%s' proto", fn_sym->name);
            fn_sym->ts = func_type_spec_from_arginfo(fn_sym->params, fn_sym->ret);
        }
    }

    exp->ts = sym->ts;
    exp->sym = sym;

    log_info("ident resolved: %s", sym->name);
    log_type_spec(sym->ts);
}

static void parse_under(ParserState *ps, Expr *exp)
{
    kl_error(exp->loc, "'_' cannot be used in this context.");
}

static void parse_lit_int(ParserState *ps, LitExpr *lit)
{
    /* expected type from lhs */
    TypeSpec *ts = lit->expected;
    if (!ts) {
        lit->len = 8;
        lit->ival = (uint64_t)lit->ival_128;
        return;
    }

    if (ts->kind != TYPE_INT) {
        return;
    }

    __int128 val = lit->ival_128;
    int width = ts->int_flt_info.width;
    int sign = ts->int_flt_info.sign;

    // update literal integer's type as expected type
    lit->ts = ts;
    lit->sign = sign;
    lit->len = width;
    lit->ival = (uint64_t)val;

    __uint128_t phys_max = (width == 8) ? (__uint128_t)0xFFFFFFFFFFFFFFFFULL
                                        : ((__uint128_t)1 << (width * 8)) - 1;

    if (lit->bit_mode) {
        /* non-decimal literals */
        if (val < 0) {
            __int128 min_s = -((__int128)1 << (width * 8 - 1));
            if (val < min_s || sign == 0) {
                kl_error(lit->loc, "Negative hex literal %s overflows '%s%d' range",
                         lit->orginal, (sign ? "int" : "uint"), width * 8);
                return;
            }
        } else {
            if ((__uint128_t)val > phys_max) {
                kl_error(lit->loc, "Bit pattern exceeds the bit width of '%s%d'",
                         (sign ? "int" : "uint"), width * 8);
                return;
            }
        }
        // notes: allow 0xFF to be assigned to int8 as -1.
    } else {
        __int128 min_limit, max_limit;
        if (sign) {
            max_limit = (__int128)(phys_max >> 1); // 2^(n-1) - 1
            min_limit = -(max_limit + 1);          // -2^(n-1)
        } else {
            min_limit = 0;
            max_limit = (__int128)phys_max;
        }

        if (val < min_limit || val > max_limit) {
            kl_error(lit->loc, "Value %s overflows '%s%d' range", lit->orginal,
                     (sign ? "int" : "uint"), width * 8);
            return;
        }
    }
}

static void parse_lit_float(ParserState *ps, LitExpr *lit)
{
    /* expected type from lhs */
    // TypeDesc *desc = lit->expected;
    // if (!desc) return;
}

static void parse_none(ParserState *ps, LitExpr *lit)
{
    if (lit->ctx != EXPR_CTX_LOAD) {
        kl_error(lit->loc, "none is readonly.");
        return;
    }

    // TypeDesc *expected = lit->expected;
    // if (expected) {
    //     if (expected->kind != TYPE_OPTIONAL_KIND) {
    //         kl_error(lit->loc, "expected an optional type.");
    //         return;
    //     }
    //     // lit->desc = expected;
    // }
}

static void parse_literal(ParserState *ps, Expr *exp)
{
    if (exp->ctx != EXPR_CTX_LOAD) {
        kl_error(exp->loc, "literal is readonly.");
        return;
    }

    LitExpr *lit = (LitExpr *)exp;
    switch (lit->which) {
        case LIT_EXPR_INT: {
            log_info("literal integer:%s", lit->orginal);
            parse_lit_int(ps, lit);
            ASSERT(lit->ts);
            lit->sym = get_symbol_by_id(lit->ts->sym_id);
            ASSERT(lit->sym);
            break;
        }
        case LIT_EXPR_FLT: {
            log_info("literal float: %lf", lit->fval);
            parse_lit_float(ps, lit);
            ASSERT(lit->ts);
            lit->sym = get_symbol_by_id(lit->ts->sym_id);
            ASSERT(lit->sym);
            break;
        }
        case LIT_EXPR_BOOL: {
            log_info("literal bool: %s", lit->bval ? "true" : "false");
            ASSERT(lit->ts);
            lit->sym = get_symbol_by_id(lit->ts->sym_id);
            ASSERT(lit->sym);
            break;
        }
        case LIT_EXPR_STR: {
            log_info("literal string '%s'", lit->sval);
            if (check_utf8(lit->sval, lit->len) < 0) {
                kl_error(exp->loc, "invalid utf8 string");
            }
            ASSERT(lit->ts);
            lit->sym = get_symbol_by_id(lit->ts->sym_id);
            ASSERT(lit->sym);
            break;
        }
        case LIT_EXPR_NONE: {
            log_info("literal null");
            parse_none(ps, lit);
            break;
        }
        default: {
            UNREACHABLE();
            break;
        }
    }
}

static Symbol *get_current_klass(ParserState *ps)
{
    ParserScope *sc = ps->scope;
    while (sc) {
        if (sc->kind == SCOPE_CLASS) {
            return sc->sym;
        }
        sc = sc->next;
    }
    return NULL;
}

static void parse_self(ParserState *ps, Expr *exp)
{
    ASSERT(exp->ctx == EXPR_CTX_LOAD);

    Symbol *sym = get_current_klass(ps);
    if (!sym || sym->kind != SYM_CLASS) {
        kl_error(exp->loc, "'self' can only be used in class.");
        return;
    }

    exp->ts = ((KlassSymbol *)sym)->instance_ts;
    exp->sym = sym;
    log_info("'self' resolved as class '%s'", sym->name);
    log_type_spec(exp->ts);
}

static void parse_list(ParserState *ps, Expr *exp)
{
    ListExpr *list_exp = (ListExpr *)exp;

    Vector *tp_args = vector_create_ptr();
    Expr *e;
    vector_foreach(e, list_exp->vec) {
        e->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, e);
        if (!e->ts) return;
        vector_push_back(tp_args, &e->ts);
    }

    TypeSpec *infer_ts = find_lub(tp_args);
    log_info("infer list type:");
    log_type_spec(infer_ts);

    vector_clear(tp_args);
    vector_push_back(tp_args, &infer_ts);

    Symbol *origin = stbl_get(ps->module->builtin, "list");
    InstanceSymbol *inst_sym = find_or_add_instance(ps->module->stbl, origin, tp_args);
    inst_sym->arg = infer_ts; // save infered tuple type for later use

    exp->ts = inst_sym->instance_ts;
    exp->sym = (Symbol *)inst_sym;
    vector_destroy(tp_args);

    log_info("list type resolved:");
    log_type_spec(exp->ts);
}

static void parse_tuple(ParserState *ps, Expr *exp)
{
    TupleExpr *tuple_exp = (TupleExpr *)exp;

    Vector *tp_args = vector_create_ptr();
    Expr *e;
    vector_foreach(e, tuple_exp->vec) {
        e->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, e);
        if (!e->ts) return;
        vector_push_back(tp_args, &e->ts);
    }

    TypeSpec *infer_ts = find_lub(tp_args);
    log_info("infer tuple type:");
    log_type_spec(infer_ts);

    Symbol *origin = stbl_get(ps->module->builtin, "tuple");
    InstanceSymbol *inst_sym = find_or_add_instance(ps->module->stbl, origin, tp_args);
    inst_sym->arg = infer_ts; // save infered tuple type for later use

    exp->ts = inst_sym->instance_ts;
    exp->sym = (Symbol *)inst_sym;
    log_info("tuple type resolved:");
    log_type_spec(exp->ts);
}

static void check_kw_arg(ParserState *ps, Vector *params, KeyWordExpr *kw, int i__)
{
    log_info("check keyword argument: '%s' at %d", kw->key.name, i__);

    int param_size = vector_size(params);
    for (int i = i__; i < param_size; i++) {
        ArgInfo *arg = vector_get(params, i);
        if (!strcmp(arg->name, kw->key.name)) {
            if (!type_spec_compatible(arg->ts, kw->value->ts)) {
                kl_error_incompatible_type(kw->loc, arg->ts, kw->value->ts);
            }
            return;
        }
    }

    kl_error(kw->loc, "unexpected keyword argument: '%s'", kw->key.name);
}

static void check_dfl_param(ParserState *ps, Vector *params, Vector *exprs, int i__,
                            int j__)
{
    int expr_size = vector_size(exprs);
    for (int j = j__; j < expr_size; j++) {
        Expr *e = vector_get(exprs, j);
        ArgInfo *arg = vector_get(params, i__);
        if (!e) {
            // use default values for the rest parameters
            log_info("kw-param: %s, no value passed, skip the rest.", arg->name);
            return;
        }

        if (e->kind == EXPR_KW_KIND) {
            KeyWordExpr *kw = (KeyWordExpr *)e;
            log_info("kw-arg: '%s', check kw-arg", kw->key.name);
            check_kw_arg(ps, params, kw, i__);
        } else {
            if (!arg) {
                kl_error(e->loc, "too many positional arguments in function call.");
                return;
            }
            log_info("kw-param: '%s', check value passed by positional argument",
                     arg->name);
            if (!type_spec_compatible(arg->ts, e->ts)) {
                kl_error_incompatible_type(e->loc, arg->ts, e->ts);
            }
            ++i__;
        }
    }
}

static void check_valist_arg(ParserState *ps, TypeSpec *expected, Vector *exprs, int i__,
                             int *next_j)
{
    TypeSpec *arg_ts;
    int expr_size = vector_size(exprs);
    for (int j = i__; j < expr_size; j++) {
        Expr *e = vector_get(exprs, j);

        if (e->kind == EXPR_KW_KIND) {
            *next_j = j;
            return;
        }

        log_info("var-arg: check value passed by positional argument");

        arg_ts = e->ts;

        if ((e->kind == EXPR_BANG_KIND) && (match_sequence(e->ts, NULL, &arg_ts))) {
            log_info("var-arg: arg is unpacked by '!', check unpacked type:");
            log_type_spec(arg_ts);
        }

        if (!type_spec_compatible(expected, arg_ts)) {
            kl_error_incompatible_type(e->loc, expected, arg_ts);
        }
    }
    *next_j = expr_size;
}

static void check_call_args(Vector *params, Vector *exprs, ParserState *ps, Loc fn_loc)
{
    Expr *e1;
    vector_foreach(e1, exprs) {
        Expr *e2;
        vector_foreach(e2, exprs) {
            if (e1 == e2) continue;
            if (e1->kind == EXPR_KW_KIND && e2->kind == EXPR_KW_KIND) {
                KeyWordExpr *kw1 = (KeyWordExpr *)e1;
                KeyWordExpr *kw2 = (KeyWordExpr *)e2;
                if (strcmp(kw1->key.name, kw2->key.name) == 0) {
                    kl_error(kw2->loc, "duplicate keyword argument: '%s'", kw2->key.name);
                    return;
                }
            }
        }
    }

    int param_size = vector_size(params);
    int i__ = 0;
    int j__ = 0;
    while (i__ < param_size) {
        ArgInfo *arg = vector_get(params, i__);
        // 1. kw-param: pass value with keyword argument, e.g. foo(x=10)
        // 2. pass value only, e.g. foo(10)
        // 3. skip it, e.g. foo() for foo(x=10)
        if (arg->dfl_val_idx > 0) {
            check_dfl_param(ps, params, exprs, i__, j__);
            return;
        }

        Expr *e = vector_get(exprs, j__++);

        if (!type_is_valist(arg->ts)) {
            // required param, caller must pass value
            if (!e) {
                kl_error(fn_loc, "too few arguments in function call.");
                return;
            }

            if (e->kind == EXPR_KW_KIND) {
                KeyWordExpr *kw = (KeyWordExpr *)e;
                kl_error(
                    kw->loc,
                    "require a positional argument, but got a keyword argument: '%s'",
                    kw->key.name);
            }

            log_info("param '%s' is positional argument", arg->name);

            if (e->kind == EXPR_LITERAL_KIND) {
                log_info("arg is literal, parse literal expr again with expected type:");
                log_type_spec(e->ts);
                e->ctx = EXPR_CTX_LOAD;
                e->expected = arg->ts;
                parser_visit_expr(ps, e);
                if (!e->ts) return;
                log_info("after parsing literal expr, arg type is:");
                log_type_spec(e->ts);
            }

            if (!type_spec_compatible(arg->ts, e->ts)) {
                kl_error_incompatible_type(e->loc, arg->ts, e->ts);
            }

            ++i__;
        } else {
            // no more arguments passed.
            if (!e) return;

            if (e->kind == EXPR_KW_KIND) {
                log_info("skip var-arg param '%s' check and go to next kw-arg check",
                         arg->name);
                KeyWordExpr *kw = (KeyWordExpr *)e;
                check_kw_arg(ps, params, kw, i__ + 1);
            } else {
                // var-arg, caller can pass 0 or more values
                TypeSpec *src = arg->ts->va_list.src;
                if (type_is_any(src)) {
                    log_info(
                        "param '%s' is var-arg of type 'any', no check for var-arg type",
                        arg->name);
                } else {
                    int next_j = 0;
                    check_valist_arg(ps, src, exprs, i__, &next_j);
                    ++i__;
                    j__ = next_j;
                }
            }
        }
    }
}

static void parse_type(ParserState *ps, Expr *exp)
{
    TypeSpec *ts = exp->ts;
    ts = resolve_type(ps, ts);
    if (!ts) return;
    if (!check_type(ps, ts)) return;
    ASSERT(ts->sym_id >= 0);
    exp->sym = get_symbol_by_id(ts->sym_id);
    ASSERT(exp->sym->kind == SYM_CLASS);
    // update expr type as symbol type
    // int -> exp->ts is type type, symbol is int
    // Foo -> exp->ts is type type, symbol is Foo
    exp->ts = exp->sym->ts;
    log_info("type '%s' is resolved as: ", exp->sym->name);
    log_type_spec(exp->ts);
    return;
}

static TypeSpec *instance_type_spec(TypeSpec *ts, KlassSymbol *origin,
                                    InstanceSymbol *sym, ParserState *ps)
{
    Vector *kls_tps = &origin->tps;
    Vector *tp_args = sym->tp_args;

    TypeSpec *inst_ts;
    if (ts->kind == TYPE_GENERIC_VAR) {
        TypeParamSymbol *tp_sym = vector_get(kls_tps, ts->generic_var.index);
        ASSERT(tp_sym);
        if (!strcmp(tp_sym->name, ts->generic_var.name)) {
            if (tp_sym->which == TP_INFER) {
                ASSERT(sym->arg);
                inst_ts = sym->arg;
                log_info(
                    "generic var '%s' is inferred as '%s' for instance specialization",
                    ts->generic_var.name, inst_ts->signature);
            } else {
                log_info(
                    "generic var '%s' matches klass's tp, use tp_args to get instance "
                    "type",
                    ts->generic_var.name);
                inst_ts = vector_get(tp_args, ts->generic_var.index);
            }
        } else {
            // generic var must be defined in method's tps, not klass's tps.
            log_info("generic var '%s' does not match klass's tp, keep it.",
                     ts->generic_var.name);
            inst_ts = ts;
        }
    } else if (ts->kind == TYPE_GENERIC_REF) {
        Vector *_tp_args = vector_create_ptr();
        TypeSpec *_ts;
        vector_foreach(_ts, ts->generic_ref.args) {
            TypeSpec *inst_arg = instance_type_spec(_ts, origin, sym, ps);
            vector_push_back(_tp_args, &inst_arg);
        }
        Symbol *_sym = get_symbol_by_id(ts->sym_id);
        InstanceSymbol *inst_sym = find_or_add_instance(ps->module->stbl, _sym, _tp_args);
        inst_ts = inst_sym->instance_ts;
        vector_destroy(_tp_args);
    } else if (type_is_valist(ts)) {
        TypeSpec *src = ts->va_list.src;
        if (src->kind == TYPE_GENERIC_VAR) {
            TypeParamSymbol *tp_sym = vector_get(kls_tps, src->generic_var.index);
            ASSERT(tp_sym);
            if (!strcmp(tp_sym->name, src->generic_var.name)) {
                TypeSpec *inst_src_ts;
                if (tp_sym->which == TP_INFER) {
                    ASSERT(sym->arg);
                    inst_src_ts = sym->arg;
                    log_info(
                        "var-arg generic var '%s' is inferred as '%s' for instance "
                        "specialization",
                        src->generic_var.name, inst_src_ts->signature);
                } else {
                    log_info(
                        "var-arg generic var '%s' matches klass's tp, use tp_args to get "
                        "instance type",
                        src->generic_var.name);
                    inst_src_ts = vector_get(tp_args, src->generic_var.index);
                    log_info("var-arg generic var '%s' is resolved as '%s'",
                             src->generic_var.name, inst_src_ts->signature);
                }
                // update var-arg type as var-arg of instance type.
                inst_ts = va_list_type_spec_intern(inst_src_ts);
            } else {
                // generic var must be defined in method's tps, not klass's tps.
                log_info("var-arg generic var '%s' does not match klass's tp, keep it.",
                         src->generic_var.name);
                inst_ts = ts;
            }
        } else {
            log_info("var-arg source type is not generic var, keep it.");
            inst_ts = ts;
        }
    } else {
        inst_ts = ts;
    }
    return inst_ts;
}

static Vector *build_instance_params(Vector *params, KlassSymbol *origin,
                                     InstanceSymbol *sym, ParserState *ps)
{
    Vector *inst_params = vector_create_ptr();
    ArgInfo *arg;
    vector_foreach(arg, params) {
        if (!arg) continue;
        TypeSpec *ts = instance_type_spec(arg->ts, origin, sym, ps);
        ArgInfo *inst_arg = mm_alloc_obj(inst_arg);
        inst_arg->name = arg->name;
        inst_arg->ts = ts;
        inst_arg->dfl_val_idx = arg->dfl_val_idx;
        vector_push_back(inst_params, &inst_arg);
    }
    return inst_params;
}

static inline int func_has_infer_tp(FuncSymbol *fn_sym)
{
    if (vector_size(&fn_sym->tps) <= 0) return 0;

    TypeParamSymbol *tp_sym = vector_get(&fn_sym->tps, 0);
    if (tp_sym->which != TP_INFER) return 0;
    return 1;
}

static Vector *get_func_real_params(FuncSymbol *fn_sym, Vector *_tp_args)
{
    Vector *real_params = vector_create_ptr();
    ArgInfo *arg;
    vector_foreach(arg, fn_sym->params) {
        if (!arg) continue;
        TypeSpec *ts = arg->ts;
        if (ts->kind == TYPE_GENERIC_VAR) {
            TypeSpec *_ts = vector_get(_tp_args, ts->generic_var.index);
            ASSERT(_ts);
            ASSERT(!strcmp(fn_sym->name, ts->generic_var.owner));
            ts = _ts;
        }

        ArgInfo *real_arg = mm_alloc_obj(real_arg);
        real_arg->name = arg->name;
        real_arg->ts = ts;
        real_arg->dfl_val_idx = arg->dfl_val_idx;
        vector_push_back(real_params, &real_arg);
    }
    return real_params;
}

static TypeSpec *get_func_real_ret(FuncSymbol *fn_sym, Vector *_tp_args)
{
    TypeSpec *ret = fn_sym->ret;
    ASSERT(!strcmp(fn_sym->name, ret->generic_var.owner));
    if (ret->kind == TYPE_GENERIC_VAR) {
        TypeSpec *_ts = vector_get(_tp_args, ret->generic_var.index);
        ret = _ts;
    }
    return ret;
}

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

static Vector *infer_tp_from_new(KlassSymbol *cls_sym, FuncSymbol *fn_sym,
                                 CallExpr *call_exp, ParserState *ps)
{
    Vector *result = vector_create_ptr();
    Vector *tps = &cls_sym->tps;
    Vector *params = fn_sym->params;
    Vector *call_args = call_exp->args;

    HashMap map;
    hashmap_init(&map, (HashMapEqualFunc)__tpinfo_eq__);

    ArgInfo *arg;
    vector_foreach(arg, params) {
        if (!arg) continue;
        if (arg->dfl_val_idx > 0) {
            // default value param, skip it and the rest params.
            log_info("param '%s' is key-word parameter, skip the rest.", arg->name);
            break;
        }

        TypeSpec *ts = arg->ts;

        Expr *e = vector_get(call_args, i__);
        if (!e) {
            // no more arguments passed, report error.
            if (type_is_valist(ts)) {
                // var-arg can be empty, break the loop and check the rest params.
                log_info(
                    "param '%s' is var-arg, no more arguments passed, skip the rest.",
                    arg->name);
                break;
            } else {
                kl_error(call_exp->loc,
                         "expected at least %d arguments in __init__ call, but %d got.",
                         vector_size(params), i__);
                hashmap_fini(&map, __tpinfo_free__, NULL);
                vector_destroy(result);
                return NULL;
            }
        }

        TypeSpec *real = e->ts;
        ASSERT(real);

        if (ts->kind == TYPE_GENERIC_VAR) {
            log_info("param '%s' is generic var '%s'", arg->name, ts->generic_var.name);
            ASSERT(!strcmp(ts->generic_var.owner, cls_sym->name));

            TpInfo *info = mm_alloc_obj(info);
            info->name = ts->generic_var.name;
            info->real = real;
            hashmap_entry_init(info, str_hash(info->name));
            void *r = hashmap_put(&map, info);
            ASSERT(!r);
        } else if (ts->kind == TYPE_GENERIC_REF) {
            NYI();
        } else if (ts->kind == TYPE_VA_LIST) {
            // var-arg must be last one parameter.
            log_info("param '%s' is var-arg", arg->name);

            // TODO: guard code
            if (i__ != (vector_size(params) - 1)) {
                // after i__ are all kw-args.
                for (int j = i__ + 1; j < vector_size(params); j++) {
                    ArgInfo *next_arg = vector_get(params, j);
                    ASSERT(next_arg->dfl_val_idx > 0);
                }
            }

            Vector *var_arg_types = vector_create_ptr();
            for (int j = i__; j < vector_size(call_args); j++) {
                Expr *e = vector_get(call_args, j);
                if (!e) continue;

                if (e->kind == EXPR_KW_KIND) {
                    KeyWordExpr *kw = (KeyWordExpr *)e;
                    log_info("var-arg '%s' is passed as keyword argument '%s', skip",
                             arg->name, kw->key.name);
                    continue;
                }

                TypeSpec *arg_ts;
                if ((e->kind == EXPR_BANG_KIND) &&
                    (match_sequence(e->ts, NULL, &arg_ts))) {
                    vector_push_back(var_arg_types, &arg_ts);
                    log_info(
                        "var-arg '%s' is passed with bang operator(sequence[T]), "
                        "sequence's type arg is added to var-arg types",
                        arg->name);
                } else {
                    vector_push_back(var_arg_types, &e->ts);
                    log_info(
                        "var-arg '%s' is passed without bang operator, argument type is "
                        "added to var-arg types",
                        arg->name);
                }
            }

            TypeSpec *src = ts->va_list.src;
            if (src->kind == TYPE_GENERIC_VAR) {
                log_info("var-arg type is generic var '%s'", src->generic_var.name);
                ASSERT(!strcmp(src->generic_var.owner, cls_sym->name));
                TypeParamSymbol *tp_sym = vector_get(tps, src->generic_var.index);
                ASSERT(tp_sym);

                if (tp_sym->which == TP_INFER) {
                    log_info("generic var '%s' is inferred", src->generic_var.name);
                    NYI();
                } else {
                    log_info("generic var '%s' is normal", src->generic_var.name);
                    real = find_lub(var_arg_types);
                    ASSERT(real);
                }

                TpInfo *info = mm_alloc_obj(info);
                info->name = src->generic_var.name;
                info->real = real;
                hashmap_entry_init(info, str_hash(info->name));
                void *r = hashmap_put(&map, info);
                ASSERT(!r);
            } else {
                NYI();
            }
        } else {
            if (ts != real) {
                kl_error_incompatible_type(e->loc, ts, real);
                hashmap_fini(&map, __tpinfo_free__, NULL);
                vector_destroy(result);
                return NULL;
            }
        }
    }

    TypeParamSymbol *tp_sym;
    vector_foreach(tp_sym, tps) {
        if (!tp_sym) continue;
        TpInfo key = { .name = tp_sym->name };
        hashmap_entry_init(&key, str_hash(key.name));
        TpInfo *info = hashmap_get(&map, &key);
        if (!info) {
            kl_error(call_exp->loc, "cannot infer type parameter '%s' for class '%s'.",
                     tp_sym->name, cls_sym->name);
            hashmap_fini(&map, __tpinfo_free__, NULL);
            vector_destroy(result);
            return NULL;
        }
        log_info("inferred type parameter '%s' for class '%s':", tp_sym->name,
                 cls_sym->name);
        log_type_spec(info->real);
        vector_push_back(result, &info->real);
    }

    hashmap_fini(&map, __tpinfo_free__, NULL);
    return result;
}

static void parse_call(ParserState *ps, Expr *exp)
{
    CallExpr *call = (CallExpr *)exp;

    Expr *lhs = call->lhs;
    lhs->ctx = EXPR_CTX_CALL;
    parser_visit_expr(ps, call->lhs);
    if (!lhs->ts) return;

    Expr *arg;
    vector_foreach(arg, call->args) {
        if (!arg) continue;
        arg->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, arg);
        if (!arg->ts) return;
    }

    Symbol *lhs_sym = lhs->sym;
    Vector *params = NULL;
    if (lhs_sym->kind == SYM_VAR) {
        TypeSpec *ts = lhs_sym->ts;

        log_info("call lhs is variable of type:");
        log_type_spec(ts);

        if (ts->kind == TYPE_PROTO) {
            log_info("call lhs is proto variable.");
            // proto variable call
            exp->ts = ts->proto_type.ret;
            params = ts->proto_type.args;
        } else if (ts->kind == TYPE_KLASS) {
            Symbol *_sym = get_symbol_by_id(ts->sym_id);
            if (!_sym) {
                UNREACHABLE();
                kl_error(lhs->loc, "'%s' is not callable", lhs_sym->name);
                return;
            }

            if (_sym->kind != SYM_CLASS) {
                kl_error(lhs->loc, "'%s' is not callable", lhs_sym->name);
                return;
            }

            // constructor call
            KlassSymbol *cls_sym = (KlassSymbol *)_sym;
            Symbol *call_fn_sym = stbl_get(cls_sym->stbl, "__call__");
            if (!call_fn_sym) {
                kl_error(lhs->loc, "'%s' is not callable.", lhs_sym->name);
                return;
            }
            exp->ts = ((FuncSymbol *)call_fn_sym)->ret;
            params = ((FuncSymbol *)call_fn_sym)->params;
        } else {
            UNREACHABLE();
        }
    } else if (lhs_sym->kind == SYM_CLASS) {
        // constructor call without type parameters, e.g. Foo(100)
        KlassSymbol *cls_sym = (KlassSymbol *)lhs_sym;
        log_info("call lhs is class '%s' without type parameters", cls_sym->name);

        Symbol *_fn_sym = stbl_get(cls_sym->stbl, "__init__");
        if (!_fn_sym) {
            kl_error(lhs->loc, "class '%s' has no constructor.", lhs_sym->name);
            return;
        }

        if (!strcmp(cls_sym->name, "tuple")) {
            log_info("infer tuple type parameters from __init__ arguments.");

            Vector *tp_args = vector_create_ptr();
            Expr *arg;
            vector_foreach(arg, call->args) {
                if (!arg) continue;
                vector_push_back(tp_args, &arg->ts);
            }

            InstanceSymbol *inst_sym =
                find_or_add_instance(ps->module->stbl, lhs_sym, tp_args);
            vector_destroy(tp_args);

            Symbol *_fn = stbl_get(inst_sym->stbl, "__init__");
            if (!_fn) {
                KlassSymbol *origin = (KlassSymbol *)inst_sym->origin;
                // params
                Vector *inst_params = build_instance_params(
                    ((FuncSymbol *)_fn_sym)->params, origin, inst_sym, ps);

                _fn = stbl_add_func(inst_sym->stbl, "__init__", no_type_spec(),
                                    inst_params, _fn_sym->flags);
            }
            _fn_sym = _fn;

            // func call type is instance type
            exp->ts = inst_sym->instance_ts;
            params = ((FuncSymbol *)_fn_sym)->params;
        } else {
            if (vector_size(&cls_sym->tps) > 0) {
                log_info(
                    "class '%s' has type parameters, try to infer them from __init__ "
                    "arguments.",
                    cls_sym->name);

                Vector *tp_args;

                if (vector_empty(call->args)) {
                    log_info("no arguments passed to __init__, set all tp as any");
                    tp_args = vector_create_ptr();
                    TypeSpec *any_ts = any_type_spec();
                    for (int i = 0; i < vector_size(&cls_sym->tps); i++) {
                        vector_push_back(tp_args, &any_ts);
                    }
                } else {
                    tp_args =
                        infer_tp_from_new(cls_sym, ((FuncSymbol *)_fn_sym), call, ps);
                }

                if (!tp_args) {
                    kl_error(lhs->loc, "failed to infer type parameters for class '%s'.",
                             cls_sym->name);
                    return;
                }

                InstanceSymbol *inst_sym =
                    find_or_add_instance(ps->module->stbl, lhs_sym, tp_args);

                vector_destroy(tp_args);

                Symbol *_fn = stbl_get(inst_sym->stbl, "__init__");
                if (!_fn) {
                    KlassSymbol *origin = (KlassSymbol *)inst_sym->origin;
                    // params
                    Vector *inst_params = build_instance_params(
                        ((FuncSymbol *)_fn_sym)->params, origin, inst_sym, ps);

                    _fn = stbl_add_func(inst_sym->stbl, "__init__", no_type_spec(),
                                        inst_params, _fn_sym->flags);
                }
                _fn_sym = _fn;

                // func call type is instance type
                exp->ts = inst_sym->instance_ts;
                params = ((FuncSymbol *)_fn_sym)->params;
            } else {
                log_info("class '%s' has no type parameters.", cls_sym->name);
                // func call type is instance type
                exp->ts = cls_sym->instance_ts;
                // exp->sym = cls_sym;
                params = ((FuncSymbol *)_fn_sym)->params;
            }
        }
    } else if (lhs_sym->kind == SYM_FUNC || lhs_sym->kind == SYM_INTF) {
        FuncSymbol *fn_sym = (FuncSymbol *)lhs_sym;
        if (func_has_infer_tp(fn_sym)) {
            Vector *_tp_args = infer_func_tp(fn_sym, call->args, ps);
            if (!_tp_args) {
                kl_error(lhs->loc, "failed to infer type parameters for function '%s'.",
                         fn_sym->name);
                return;
            }

            log_info("inferred type parameters for function '%s':", fn_sym->name);
            TypeSpec *_tp_arg;
            vector_foreach(_tp_arg, _tp_args) {
                log_type_spec(_tp_arg);
            }
            params = get_func_real_params(fn_sym, _tp_args);
            exp->ts = get_func_real_ret(fn_sym, _tp_args);
            vector_destroy(_tp_args);
        } else {
            if (lhs->ts->kind == TYPE_OPTIONAL) {
                log_info("call lhs is optional of proto.");
                // optional proto function call
                if (fn_sym->ret->kind == TYPE_OPTIONAL) {
                    exp->ts = fn_sym->ret;
                } else {
                    exp->ts = optional_type_spec_intern(fn_sym->ret);
                }
            } else if (lhs->ts->kind == TYPE_PROTO) {
                log_info("call lhs is proto.");
                // proto function call
                exp->ts = fn_sym->ret;
            }
            params = fn_sym->params;
        }
    } else if (lhs_sym->kind == SYM_INSTANCE) {
        // constructor call with type parameters, e.g. Foo[int](100)
        InstanceSymbol *inst_sym = (InstanceSymbol *)lhs_sym;

        log_info("call lhs is instance of class '%s' with type parameters:",
                 lhs_sym->name);

        KlassSymbol *origin = (KlassSymbol *)inst_sym->origin;
        Symbol *init_fn_sym = stbl_get(inst_sym->stbl, "__init__");
        if (!init_fn_sym) {
            Symbol *_fn_sym = stbl_get(origin->stbl, "__init__");
            if (!_fn_sym) {
                kl_error(lhs->loc, "class '%s' has no constructor.", origin->name);
                return;
            }

            // params
            Vector *inst_params = build_instance_params(((FuncSymbol *)_fn_sym)->params,
                                                        origin, inst_sym, ps);

            init_fn_sym =
                stbl_add_func(inst_sym->stbl, "__init__", no_type_spec(), inst_params, 0);
        }

        if (!strcmp(origin->name, "tuple")) {
            log_info("check __init__ args with specialized tuple type parameters.");
            if (vector_size(inst_sym->tp_args) != vector_size(call->args)) {
                kl_error(
                    lhs->loc,
                    "tuple instance expects %d type arguments, but %d were provided.",
                    vector_size(inst_sym->tp_args), vector_size(call->args));
                return;
            }

            for (int i = 0; i < vector_size(inst_sym->tp_args); i++) {
                TypeSpec *tp_arg = vector_get(inst_sym->tp_args, i);
                Expr *arg = vector_get(call->args, i);
                if (!type_spec_compatible(tp_arg, arg->ts)) {
                    kl_error(arg->loc,
                             "tuple instance argument type is not compatible with "
                             "specialized type parameter.");
                    return;
                }
            }
        }

        // func call type is instance type
        exp->ts = inst_sym->instance_ts;
        params = ((FuncSymbol *)init_fn_sym)->params;
    } else {
        UNREACHABLE();
    }

    check_call_args(params, call->args, ps, lhs->loc);

    // TODO:handle for builtin type(int, float, str etc) calls
    if (exp->ts->kind == TYPE_INT) {
    } else if (exp->ts->kind == TYPE_FLOAT) {
    } else {
    }
}

static TypeSpec *opt_dot_type(TypeSpec *ts, int opt_or_bang)
{
    if (opt_or_bang == DOT_OPTIONAL && ts->kind != TYPE_OPTIONAL) {
        return optional_type_spec_intern(ts);
    }
    return ts;
}

static void copy_tps(Vector *dst, Vector *src)
{
    TypeParamSymbol *tp_sym;
    vector_foreach(tp_sym, src) {
        if (!tp_sym) continue;
        TypeParamSymbol *new_tp_sym = mm_alloc_obj(new_tp_sym);
        new_tp_sym->name = tp_sym->name;
        new_tp_sym->which = tp_sym->which;
        vector_push_back(dst, &new_tp_sym);
    }
}

static void parse_dot(ParserState *ps, Expr *exp)
{
    DotExpr *dot = (DotExpr *)exp;
    int opt_or_bang = dot->opt_or_bang;

    Expr *lhs = dot->lhs;
    lhs->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, lhs);
    if (!lhs->ts) return;

    if (type_is_optional(lhs->ts)) {
        if (opt_or_bang == DOT_BANG) {
            // force unwrap, error if nil
            log_info("force unwrap optional type.");
            TypeSpec *src_ts = lhs->ts->opt.src;
            ASSERT(src_ts);
            lhs->ts = src_ts;
        } else if (opt_or_bang == DOT_OPTIONAL) {
            // safe unwrap, don't change lhs type and pass next
            log_info("safe unwrap optional type.");
            TypeSpec *src_ts = lhs->ts->opt.src;
            ASSERT(src_ts);
        } else {
            kl_error(lhs->loc, "optional cannot use dot operator.");
            return;
        }
    } else {
        Symbol *lhs_sym = lhs->sym;
        if (lhs_sym && lhs_sym->kind == SYM_SHADOW_VAR) {
            ShadowVarSymbol *shadow_sym = (ShadowVarSymbol *)lhs_sym;
            if (!shadow_sym->is_null) {
                if (opt_or_bang == DOT_OPTIONAL) {
                    kl_warn(lhs->loc,
                            "variable '%s' is non-nullable, but '?.' operator is used, "
                            "which is redundant.",
                            shadow_sym->name);
                } else if (opt_or_bang == DOT_BANG) {
                    kl_warn(lhs->loc,
                            "variable '%s' is non-nullable, but '!.' operator is used, "
                            "which is redundant.",
                            shadow_sym->name);
                }
            } else {
                UNREACHABLE();
            }
        } else {
            if (opt_or_bang == DOT_OPTIONAL) {
                kl_error(lhs->loc, "only optional type can use optional dot operator.");
                return;
            } else if (opt_or_bang == DOT_BANG) {
                kl_error(lhs->loc, "only optional type can use bang dot operator.");
                return;
            }
        }
    }

    int sym_id = lhs->ts->sym_id;
    if (type_is_optional(lhs->ts)) {
        log_info("unwrap optional type for member access.");
        ASSERT(lhs->ts->opt.src);
        ASSERT(lhs->ts->opt.src->sym_id >= 0);
        sym_id = lhs->ts->opt.src->sym_id;
    }

    Symbol *lhs_ts_sym = get_symbol_by_id(sym_id);
    if (!lhs_ts_sym) {
        kl_error(lhs->loc, "type symbol is not found.");
        return;
    }

    HashMap *lhs_stbl = lhs_ts_sym->stbl;
    Ident *ident = &dot->id;
    Symbol *sym = stbl_get(lhs_stbl, ident->name);
    if (sym) {
        exp->ts = opt_dot_type(sym->ts, opt_or_bang);
        exp->sym = sym;
        log_info("dot member resolved: %s", sym->name);
        if (sym->kind == SYM_FUNC) {
            log_info("ret type is:");
            log_type_spec(((FuncSymbol *)sym)->ret);
        } else {
            log_type_spec(exp->ts);
        }
        return;
    }

    // instance field/method from origin klass
    if (lhs_ts_sym->kind == SYM_INSTANCE) {
        log_info("try to find instance member from origin klass.");
        InstanceSymbol *inst_sym = (InstanceSymbol *)lhs_ts_sym;
        KlassSymbol *origin = (KlassSymbol *)inst_sym->origin;
        sym = stbl_get(origin->stbl, ident->name);
        if (sym) {
            // field/method of instance
            if (sym->kind == SYM_VAR) {
                log_info("found field '%s' from origin klass '%s'.", ident->name,
                         origin->name);
                VarSymbol *origin_var_sym = (VarSymbol *)sym;
                TypeSpec *ts =
                    instance_type_spec(origin_var_sym->ts, origin, inst_sym, ps);
                Symbol *inst_var_sym = stbl_add_var(lhs_stbl, origin_var_sym->name, ts,
                                                    origin_var_sym->flags);
                exp->ts = opt_dot_type(inst_var_sym->ts, opt_or_bang);
                exp->sym = inst_var_sym;
                log_info("dot member resolved: %s", inst_var_sym->name);
                log_type_spec(exp->ts);
                return;
            } else if (sym->kind == SYM_FUNC) {
                log_info("found func '%s' from origin klass '%s'.", ident->name,
                         origin->name);
                // method of instance
                FuncSymbol *origin_fn_sym = (FuncSymbol *)sym;

                // params
                Vector *inst_params =
                    build_instance_params(origin_fn_sym->params, origin, inst_sym, ps);

                // return type
                TypeSpec *ret_ts =
                    instance_type_spec(origin_fn_sym->ret, origin, inst_sym, ps);

                // create function symbol for instance method
                Symbol *inst_fn_sym = stbl_add_func(lhs_stbl, origin_fn_sym->name, ret_ts,
                                                    inst_params, origin_fn_sym->flags);
                inst_fn_sym->parent = inst_sym;
                // copy method's tps to instance method
                copy_tps(&((FuncSymbol *)inst_fn_sym)->tps, &origin_fn_sym->tps);
                TypeSpec *fn_ts = func_type_spec_from_arginfo(inst_params, ret_ts);
                inst_fn_sym->ts = fn_ts;
                exp->ts = opt_dot_type(fn_ts, opt_or_bang);
                exp->sym = inst_fn_sym;
                log_info("dot member resolved: %s", inst_fn_sym->name);
                log_type_spec(exp->ts);
                return;
            } else {
                // nothing
                UNREACHABLE();
            }
        }
    }

    kl_error(ident->loc, "'%s' is not a member of '%s'", ident->name, lhs_ts_sym->name);
    return;
}

static void parse_index_load(ParserState *ps, Symbol *lhs_sym, IndexExpr *index)
{
    Expr *lhs = index->lhs;

    Symbol *__fn_sym = stbl_get(lhs_sym->stbl, "__getitem__");

    if (lhs_sym->kind == SYM_INSTANCE) {
        if (!__fn_sym) {
            // try to find __getitem__ from origin klass
            InstanceSymbol *inst_sym = (InstanceSymbol *)lhs_sym;
            KlassSymbol *origin = (KlassSymbol *)inst_sym->origin;
            __fn_sym = stbl_get(origin->stbl, "__getitem__");
            if (!__fn_sym) {
                kl_error(lhs->loc, "type '%s' is not subscriptable (missing __getitem__)",
                         lhs_sym->name);
                return;
            }

            // params
            Vector *inst_params = build_instance_params(((FuncSymbol *)__fn_sym)->params,
                                                        origin, inst_sym, ps);
            // return
            TypeSpec *ret_ts =
                instance_type_spec(((FuncSymbol *)__fn_sym)->ret, origin, inst_sym, ps);

            // create function symbol for instance __getitem__
            Symbol *inst_fn_sym = stbl_add_func(lhs_sym->stbl, "__getitem__", ret_ts,
                                                inst_params, __fn_sym->flags);

            // copy __getitem__'s tps to instance __getitem__
            copy_tps(&((FuncSymbol *)inst_fn_sym)->tps, &((FuncSymbol *)__fn_sym)->tps);
            TypeSpec *fn_ts = func_type_spec_from_arginfo(inst_params, ret_ts);
            inst_fn_sym->ts = fn_ts;
            inst_fn_sym->parent = inst_sym;
            __fn_sym = inst_fn_sym;
        }
    } else if (lhs_sym->kind == SYM_CLASS || lhs_sym->kind == SYM_TRAIT) {
        // do nothing, __getitem__ is defined on class/trait type itself, no need to
        // create new symbol for it.
    } else {
        kl_error(lhs->loc, "type '%s' is not subscriptable (missing __getitem__)",
                 lhs_sym->name);
        return;
    }

    if (!__fn_sym) {
        kl_error(lhs->loc, "type '%s' is not subscriptable (missing __getitem__)",
                 lhs_sym->name);
        return;
    }

    FuncSymbol *fn_sym = (FuncSymbol *)__fn_sym;

    if (func_has_infer_tp(fn_sym)) {
        Vector *_tp_args = infer_func_tp(fn_sym, index->vec, ps);
        if (!_tp_args) {
            kl_error(lhs->loc, "failed to infer type parameters for function '%s'.",
                     fn_sym->name);
            return;
        }

        Vector *params = get_func_real_params(fn_sym, _tp_args);
        check_call_args(params, index->vec, ps, lhs->loc);
        index->ts = get_func_real_ret(fn_sym, _tp_args);
        index->sym = get_symbol_by_id(index->ts->sym_id);
        vector_destroy(_tp_args);
    } else {
        index->ts = fn_sym->ret;
        index->sym = get_symbol_by_id(index->ts->sym_id);
        check_call_args(fn_sym->params, index->vec, ps, lhs->loc);
    }

    log_info("index load resolved to __getitem__:");
    log_type_spec(index->ts);
}

static void parse_index_store(ParserState *ps, Symbol *lhs_sym, IndexExpr *index)
{
    Expr *lhs = index->lhs;
    Symbol *__fn_sym = stbl_get(lhs_sym->stbl, "__setitem__");

    if (lhs_sym->kind == SYM_INSTANCE) {
        if (!__fn_sym) {
            // try to find __setitem__ from origin klass
            InstanceSymbol *inst_sym = (InstanceSymbol *)lhs_sym;
            KlassSymbol *origin = (KlassSymbol *)inst_sym->origin;
            __fn_sym = stbl_get(origin->stbl, "__setitem__");
            if (!__fn_sym) {
                kl_error(lhs->loc, "type '%s' is not subscriptable (missing __setitem__)",
                         lhs_sym->name);
                return;
            }

            // params
            Vector *inst_params = build_instance_params(((FuncSymbol *)__fn_sym)->params,
                                                        origin, inst_sym, ps);
            // return
            TypeSpec *ret_ts =
                instance_type_spec(((FuncSymbol *)__fn_sym)->ret, origin, inst_sym, ps);

            // create function symbol for instance __setitem__
            Symbol *inst_fn_sym = stbl_add_func(lhs_sym->stbl, "__setitem__", ret_ts,
                                                inst_params, __fn_sym->flags);

            // copy __setitem__'s tps to instance __setitem__
            copy_tps(&((FuncSymbol *)inst_fn_sym)->tps, &((FuncSymbol *)__fn_sym)->tps);
            TypeSpec *fn_ts = func_type_spec_from_arginfo(inst_params, ret_ts);
            inst_fn_sym->ts = fn_ts;
            inst_fn_sym->parent = inst_sym;
            __fn_sym = inst_fn_sym;
        }
    } else if (lhs_sym->kind == SYM_CLASS || lhs_sym->kind == SYM_TRAIT) {
        // do nothing, __setitem__ is defined on class/trait type itself, no need to
        // create new symbol for it.
    } else {
        kl_error(lhs->loc, "type '%s' is not subscriptable (missing __setitem__)",
                 lhs_sym->name);
        return;
    }

    if (!__fn_sym) {
        kl_error(lhs->loc, "type '%s' is not subscriptable (missing __setitem__)",
                 lhs_sym->name);
        return;
    }

    FuncSymbol *fn_sym = (FuncSymbol *)__fn_sym;

    Vector *args = vector_create_ptr();

    ASSERT(vector_size(index->vec) == 1);
    Expr *e = vector_get(index->vec, 0);
    vector_push_back(args, &e);

    ASSERT(index->arg);
    vector_push_back(args, &index->arg);

    check_call_args(fn_sym->params, args, ps, lhs->loc);

    vector_destroy(args);

    index->ts = e->ts;
    index->sym = get_symbol_by_id(index->ts->sym_id);

    log_info("index store resolved to __setitem__:");
    log_type_spec(index->ts);
}

static void parse_slice_load(ParserState *ps, Symbol *lhs_sym, IndexExpr *index)
{
    Expr *lhs = index->lhs;

    Symbol *__fn_sym = stbl_get(lhs_sym->stbl, "__getslice__");

    if (lhs_sym->kind == SYM_INSTANCE) {
        if (!__fn_sym) {
            // try to find __getslice__ from origin klass
            InstanceSymbol *inst_sym = (InstanceSymbol *)lhs_sym;
            KlassSymbol *origin = (KlassSymbol *)inst_sym->origin;
            __fn_sym = stbl_get(origin->stbl, "__getslice__");
            if (!__fn_sym) {
                kl_error(lhs->loc,
                         "type '%s' is not subscriptable (missing __getslice__)",
                         lhs_sym->name);
                return;
            }

            // params
            Vector *inst_params = build_instance_params(((FuncSymbol *)__fn_sym)->params,
                                                        origin, inst_sym, ps);
            // return
            TypeSpec *ret_ts =
                instance_type_spec(((FuncSymbol *)__fn_sym)->ret, origin, inst_sym, ps);

            // create function symbol for instance __getslice__
            Symbol *inst_fn_sym = stbl_add_func(lhs_sym->stbl, "__getslice__", ret_ts,
                                                inst_params, __fn_sym->flags);

            // copy __getslice__'s tps to instance __getslice__
            copy_tps(&((FuncSymbol *)inst_fn_sym)->tps, &((FuncSymbol *)__fn_sym)->tps);
            TypeSpec *fn_ts = func_type_spec_from_arginfo(inst_params, ret_ts);
            inst_fn_sym->ts = fn_ts;
            inst_fn_sym->parent = inst_sym;
            __fn_sym = inst_fn_sym;
        }
    } else if (lhs_sym->kind == SYM_CLASS || lhs_sym->kind == SYM_TRAIT) {
        // do nothing, __getslice__ is defined on class/trait type itself, no need to
        // create new symbol for it.
    } else {
        kl_error(lhs->loc, "type '%s' is not subscriptable (missing __getslice__)",
                 lhs_sym->name);
        return;
    }

    if (!__fn_sym) {
        kl_error(lhs->loc, "type '%s' is not subscriptable (missing __getslice__)",
                 lhs_sym->name);
        return;
    }

    FuncSymbol *fn_sym = (FuncSymbol *)__fn_sym;

    index->ts = fn_sym->ret;
    index->sym = get_symbol_by_id(index->ts->sym_id);
    check_call_args(fn_sym->params, index->vec, ps, lhs->loc);

    log_info("index load resolved to __getslice__:");
    log_type_spec(index->ts);
}

static void parse_index_new_type(ParserState *ps, IndexExpr *index)
{
    Expr *lhs = index->lhs;
    KlassSymbol *kls_sym = (KlassSymbol *)lhs->sym;

    // generic types, e.g. List[int], Dict[str, int]

    if (kls_sym->kind != SYM_CLASS) {
        kl_error(lhs->loc, "type '%s' is not a class type.", lhs->sym->name);
        return;
    }

    int tp_size = vector_size(&kls_sym->tps);
    if (tp_size != vector_size(index->vec)) {
        if (!strcmp(kls_sym->name, "tuple")) {
            log_info(
                "tuple is inferred type parameter, skip type argument number check.");
        } else {
            kl_error(lhs->loc,
                     "type '%s' expects %d type arguments, but %d were provided.",
                     kls_sym->name, tp_size, vector_size(index->vec));
            return;
        }
    }

    Vector *tp_args = vector_create_ptr();
    Expr *arg;
    vector_foreach(arg, index->vec) {
        if (!arg) continue;
        arg->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, arg);
        if (!arg->ts) return;

        ASSERT(arg->ts->kind == TYPE_TYPE);

        TypeSpec *arg_ts;
        Symbol *arg_sym = arg->sym;
        if (arg_sym->kind == SYM_CLASS || arg_sym->kind == SYM_TRAIT) {
            arg_ts = ((KlassSymbol *)arg->sym)->instance_ts;
        } else if (arg_sym->kind == SYM_INSTANCE) {
            arg_ts = ((InstanceSymbol *)arg->sym)->instance_ts;
        } else {
            kl_error(arg->loc, "type argument must be a class/trait type.");
            return;
        }

        TypeParamSymbol *tp_sym = vector_get(&kls_sym->tps, i__);
        if (tp_sym) {
            TypeSpec *bound_ts;
            vector_foreach(bound_ts, &tp_sym->bound) {
                if (!bound_ts) continue;
                if (!type_spec_compatible(bound_ts, arg_ts)) {
                    kl_error(arg->loc,
                             "type argument '%s' is not compatible with bound type.",
                             arg_sym->name);
                    log_info("bound type is: ");
                    log_type_spec(bound_ts);
                    return;
                }
            }
        }
        vector_push_back(tp_args, &arg_ts);
    }

    // create or find instance symbol(List<int>)
    InstanceSymbol *inst_sym =
        find_or_add_instance(ps->module->stbl, (Symbol *)kls_sym, tp_args);
    vector_destroy(tp_args);
    index->ts = inst_sym->ts;
    index->sym = (Symbol *)inst_sym;
    log_info("generic type instance created/got: %s", inst_sym->name);
    log_type_spec(inst_sym->ts);
}

static void parse_index(ParserState *ps, Expr *exp)
{
    IndexExpr *index = (IndexExpr *)exp;
    Expr *lhs = index->lhs;
    lhs->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, lhs);
    if (!lhs->ts) return;

    if (lhs->ts->kind == TYPE_TYPE) {
        parse_index_new_type(ps, index);
        return;
    }

    if (vector_size(index->vec) != 1) {
        kl_error(lhs->loc, "only single index is supported.");
        return;
    }

    Expr *arg;
    vector_foreach(arg, index->vec) {
        if (!arg) continue;
        arg->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, arg);
        if (!arg->ts) return;
    }

    if (lhs->sym->kind == SYM_VAR) {
        TypeSpec *ts = lhs->sym->ts;
        Symbol *ts_sym = get_symbol_by_id(ts->sym_id);
        if (index->ctx == EXPR_CTX_LOAD) {
            if (arg->kind == EXPR_SLICE_KIND) {
                parse_slice_load(ps, ts_sym, index);
            } else {
                parse_index_load(ps, ts_sym, index);
            }
        } else if (index->ctx == EXPR_CTX_STORE) {
            if (arg->kind == EXPR_SLICE_KIND) {
                // parse_slice_store(ps, ts_sym, index);
            } else {
                parse_index_store(ps, ts_sym, index);
            }
        } else {
            ASSERT(index->ctx == EXPR_CTX_LOAD_STORE);
        }
    } else {
        Symbol *lhs_sym = lhs->sym;
        if (index->ctx == EXPR_CTX_LOAD) {
            if (arg->kind == EXPR_SLICE_KIND) {
                parse_slice_load(ps, lhs_sym, index);
            } else {
                parse_index_load(ps, lhs_sym, index);
            }
        } else if (index->ctx == EXPR_CTX_STORE) {
            if (arg->kind == EXPR_SLICE_KIND) {
                // parse_slice_store(ps, lhs_sym, index);
            } else {
                parse_index_store(ps, lhs_sym, index);
            }
        } else {
            ASSERT(index->ctx == EXPR_CTX_LOAD_STORE);
        }
    }
}

static void parse_slice(ParserState *ps, Expr *exp)
{
    SliceExpr *slice = (SliceExpr *)exp;

    Expr *start = slice->start;
    if (start) {
        start->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, start);
        if (!start->ts) return;
        if (start->ts->kind != TYPE_INT) {
            kl_error(start->loc, "slice start index must be of int type.");
            return;
        }
    }

    Expr *stop = slice->stop;
    if (stop) {
        stop->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, stop);
        if (!stop->ts) return;
        if (stop->ts->kind != TYPE_INT) {
            kl_error(stop->loc, "slice stop index must be of int type.");
            return;
        }
    }

    Expr *step = slice->step;
    if (step) {
        step->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, step);
        if (!step->ts) return;
        if (step->ts->kind != TYPE_INT) {
            kl_error(step->loc, "slice step index must be of int type.");
            return;
        }
    }

    exp->ts = klass_type_spec(NULL, "slice");
    exp->sym = get_symbol_by_id(exp->ts->sym_id);
}

static char *get_binary_op_name(BiOpKind op)
{
    switch (op) {
        case BINARY_ADD:
            return "__add__";
        case BINARY_SUB:
            return "__sub__";
        case BINARY_MUL:
            return "__mul__";
        case BINARY_DIV:
            return "__div__";
        case BINARY_MOD:
            return "__mod__";
        case BINARY_SHL:
            return "__lsh__";
        case BINARY_SHR:
            return "__rsh__";

        case BINARY_BIT_AND:
            return "__bitand__";
        case BINARY_BIT_OR:
            return "__bitor__";
        case BINARY_BIT_XOR:
            return "__bitxor__";

        case BINARY_LT:
            return "__lt__";
        case BINARY_LE:
            return "__le__";
        case BINARY_GT:
            return "__gt__";
        case BINARY_GE:
            return "__ge__";
        case BINARY_EQ:
            return "__eq__";
        case BINARY_NEQ:
            return "__neq__";

        case BINARY_AND:
            return "__and__";
        case BINARY_OR:
            return "__or__";
        default:
            return "unknown_op";
    }
}

static char *get_binary_op_str(BiOpKind op)
{
    switch (op) {
        case BINARY_ADD:
            return "+";
        case BINARY_SUB:
            return "-";
        case BINARY_MUL:
            return "*";
        case BINARY_DIV:
            return "/";
        case BINARY_MOD:
            return "%";
        case BINARY_SHL:
            return "<<";
        case BINARY_SHR:
            return ">>";
        case BINARY_BIT_AND:
            return "&";
        case BINARY_BIT_OR:
            return "|";
        case BINARY_BIT_XOR:
            return "^";
        case BINARY_LT:
            return "<";
        case BINARY_LE:
            return "<=";
        case BINARY_GT:
            return ">";
        case BINARY_GE:
            return ">=";
        case BINARY_EQ:
            return "==";
        case BINARY_NEQ:
            return "!=";
        case BINARY_AND:
            return "&&";
        case BINARY_OR:
            return "||";
        default:
            UNREACHABLE();
            return "";
    }
}

static int binary_op_iscmp(BiOpKind op)
{
    switch (op) {
        case BINARY_GT:
        case BINARY_GE:
        case BINARY_LT:
        case BINARY_LE:
        case BINARY_EQ:
        case BINARY_NEQ:
            return 1;
        default:
            return 0;
    }
}

static BiOpKind bin_op_reverse(BiOpKind op)
{
    BiOpKind neg_op = 0;
    switch (op) {
        case BINARY_EQ:
            neg_op = BINARY_NEQ;
            break;
        case BINARY_NEQ:
            neg_op = BINARY_EQ;
            break;
        case BINARY_LT:
            neg_op = BINARY_GE;
            break;
        case BINARY_LE:
            neg_op = BINARY_GT;
            break;
        case BINARY_GT:
            neg_op = BINARY_LE;
            break;
        case BINARY_GE:
            neg_op = BINARY_LT;
            break;
        default:
            UNREACHABLE();
            break;
    }
    return neg_op;
}

static void parse_unary(ParserState *ps, Expr *exp)
{
    UnaryExpr *unary = (UnaryExpr *)exp;
    UnOpKind op = unary->op;

    Expr *e = unary->exp;
    e->ctx = EXPR_CTX_LOAD;

    // optimize unary '!' on comparison operator
    if (op == UNARY_NOT) {
        int count = 1;
        while (e->kind == EXPR_UNARY_KIND) {
            UnaryExpr *inner_unary = (UnaryExpr *)e;
            if (inner_unary->op != UNARY_NOT) {
                // restore to original expression
                e = unary->exp;
                break;
            }
            // double negation elimination
            e = inner_unary->exp;
            count++;
        }

        if (e->kind == EXPR_BINARY_KIND) {
            BinaryExpr *bin = (BinaryExpr *)e;
            if (binary_op_iscmp(bin->op)) {
                if (count % 2 == 1) {
                    // change comparison operator to its negation
                    bin->op = bin_op_reverse(bin->op);
                    log_info(
                        "optimize unary '!'(%d) on comparison operator to its "
                        "negation.",
                        count);
                } else {
                    log_info(
                        "optimize unary '!'(%d) on comparison operator, double "
                        "negation "
                        "eliminated.",
                        count);
                }
            }
        }
    }

    parser_visit_expr(ps, e);
    if (!e->ts) return;

    if (op == UNARY_PLUS) {
        if (e->ts->kind != TYPE_INT && e->ts->kind != TYPE_FLOAT) {
            kl_error(unary->op_loc, "unary '+' operator requires int or float type.");
            return;
        }
    } else if (op == UNARY_NEG) {
        if (e->ts->kind != TYPE_INT && e->ts->kind != TYPE_FLOAT) {
            kl_error(unary->op_loc, "unary '-' operator requires int or float type.");
            return;
        }
    } else if (op == UNARY_BIT_NOT) {
        if (e->ts->kind != TYPE_INT) {
            kl_error(unary->op_loc, "unary '~' operator requires int type.");
            return;
        }
    } else if (op == UNARY_NOT) {
        if (e->ts->kind != TYPE_BOOL) {
            kl_error(unary->op_loc, "unary '!' operator requires bool type.");
            return;
        }
        exp->ts = bool_type_spec();
        log_info("unary '!' operator resolved.");
        log_type_spec(exp->ts);
    } else {
        UNREACHABLE();
    }
}

static void parse_binary(ParserState *ps, Expr *exp)
{
    BinaryExpr *bin = (BinaryExpr *)exp;
    BiOpKind op = bin->op;
    Expr *lhs = bin->lhs;
    Expr *rhs = bin->rhs;

    lhs->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, lhs);
    if (lhs->ts) {
        TypeSpec *orig_ts = lhs->ts;
        if (lhs->ts->kind == TYPE_INT) {
            if (lhs->ts->int_flt_info.sign) {
                lhs->ts = int64_type_spec();
            } else {
                lhs->ts = uint64_type_spec();
            }
            log_info("promote integer type from %s%d to %s",
                     orig_ts->int_flt_info.sign ? "int" : "uint",
                     orig_ts->int_flt_info.width * 8,
                     lhs->ts->int_flt_info.sign ? "int64" : "uint64");
        }
    }

    rhs->ctx = EXPR_CTX_LOAD;
    rhs->expected = lhs->ts;
    parser_visit_expr(ps, rhs);

    if (!lhs->ts || !rhs->ts) return;

    if (type_is_optional(lhs->ts) && (op != BINARY_EQ && op != BINARY_NEQ)) {
        Symbol *opt_sym = lhs->sym;
        if (opt_sym && opt_sym->kind == SYM_SHADOW_VAR) {
            // check shadow variable null state
            ShadowVarSymbol *shadow_sym = (ShadowVarSymbol *)opt_sym;
            log_info("  note: symbol '%s' is a shadow variable.", opt_sym->name);
            log_info("  value is null: %s", shadow_sym->is_null ? "true" : "false");
            if (shadow_sym->is_null) {
                kl_error(bin->op_loc,
                         "optional type cannot be used with '%s' operator when value "
                         "is null.",
                         get_binary_op_str(op));
                return;
            }
        }

        kl_error(bin->op_loc, "optional type cannot be used with '%s' operator.",
                 get_binary_op_str(op));
        return;
    }

    if (op == BINARY_EQ || op == BINARY_NEQ) {
        // special handling for optional and null comparison
        if (type_is_optional(lhs->ts) && type_is_optional(rhs->ts)) {
            if (expr_is_literal_null(lhs) || expr_is_literal_null(rhs)) {
                // allow optional type compared with null literal
                exp->ts = bool_type_spec();
                log_info("binary operator '%s' resolved.", get_binary_op_str(op));
                log_type_spec(exp->ts);
                return;
            }

            if (!type_spec_compatible(lhs->ts, rhs->ts)) {
                kl_error(lhs->loc,
                         "two optional types are not compatible for '%s' operator.",
                         get_binary_op_str(op));
                log_info("lhs type:");
                log_type_spec(lhs->ts);
                log_info("rhs type:");
                log_type_spec(rhs->ts);
            } else {
                exp->ts = bool_type_spec();
                log_info("binary operator '%s' resolved.", get_binary_op_str(op));
                log_type_spec(exp->ts);
            }
            return;
        }

        if (type_is_optional(lhs->ts) && !type_is_optional(rhs->ts)) {
            kl_error(bin->op_loc,
                     "cannot compare optional type with non-optional type for '%s' "
                     "operator.",
                     get_binary_op_str(op));
            return;
        }

        if (!type_is_optional(lhs->ts) && type_is_optional(rhs->ts)) {
            kl_error(bin->op_loc,
                     "cannot compare non-optional type with optional type for '%s' "
                     "operator.",
                     get_binary_op_str(op));
            return;
        }

        // fall through to normal operator resolution
    }

    Symbol *sym = get_symbol_by_id(lhs->ts->sym_id);
    if (!sym) {
        kl_error(bin->op_loc, "type is not found.");
        return;
    }

    char *op_name = get_binary_op_name(op);
    HashMap *stbl = ((KlassSymbol *)sym)->stbl;
    Symbol *fn = stbl_get(stbl, op_name);
    if (!fn) {
        kl_error(bin->op_loc, "operator '%s' is not defined in type '%s'.",
                 get_binary_op_str(op), sym->name);
        return;
    }

    Vector *args = ((FuncSymbol *)fn)->params;
    if (vector_size(args) != 1) {
        kl_error(bin->op_loc,
                 "expected %d argument for operator '%s' of type '%s', but got 1.",
                 vector_size(args), get_binary_op_str(op), sym->name);
        return;
    }

    ArgInfo *arg_info = vector_get(args, 0);
    ASSERT(arg_info);

    TypeSpec *arg_ts = arg_info->ts;
    if (!type_spec_compatible(arg_ts, rhs->ts)) {
        kl_error(bin->op_loc, "argument type mismatch for operator '%s' of type '%s'.",
                 get_binary_op_str(op), sym->name);
        log_info("expected type:");
        log_type_spec(arg_ts);
        log_info("actual type:");
        log_type_spec(rhs->ts);
        return;
    }

    if (binary_op_iscmp(op))
        exp->ts = bool_type_spec();
    else
        exp->ts = lhs->ts;

    log_info("binary operator '%s' for type '%s' resolved.", get_binary_op_str(op),
             sym->name);
    log_type_spec(exp->ts);
}

static void parse_keyword(ParserState *ps, Expr *exp)
{
    KeyWordExpr *kw = (KeyWordExpr *)exp;
    Expr *e = kw->value;
    e->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, e);
    if (!e->ts) return;
    exp->ts = e->ts;
}

static void parse_bang(ParserState *ps, Expr *exp)
{
    BangExpr *bang = (BangExpr *)exp;
    Expr *e = bang->exp;
    e->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, e);
    if (!e->ts) return;

    TypeSpec *it_ts;

    if (!type_is_optional(e->ts)) {
        Symbol *sym = e->sym;
        if (sym->kind == SYM_SHADOW_VAR) {
            // check shadow variable null state
            ShadowVarSymbol *shadow_sym = (ShadowVarSymbol *)sym;
            log_info("  note: symbol '%s' is a shadow variable.", sym->name);
            log_info("  value is null: %s", shadow_sym->is_null ? "true" : "false");
            if (shadow_sym->is_null) {
                kl_error(bang->loc,
                         "bang operator cannot be applied when value is null.");
                return;
            } else {
                kl_warn(bang->loc,
                        "bang operator applied on non-nullable variable('%s').",
                        sym->name);
                log_info("bang operator resolved on shadow variable, value is not null.");
                log_type_spec(exp->ts);
                exp->ts = e->ts;
                exp->sym = e->sym;
                return;
            }
        } else if (match_sequence(e->ts, &it_ts, NULL)) {
            exp->ts = it_ts;
            exp->sym = get_symbol_by_id(exp->ts->sym_id);
            log_info("update bang expr type as sequence type, unwrap sequence object.");
            log_type_spec(exp->ts);
            return;
        } else {
            kl_error(bang->loc,
                     "only optional type/Sequence's subtype can use bang operator.");
            return;
        }
    }

    exp->ts = e->ts->opt.src;
    exp->sym = e->sym;
    log_info("bang operator resolved, unwrap optional type.");
    log_type_spec(exp->ts);
}

static int check_type_cast(TypeSpec *src, TypeSpec *target, ParserState *ps)
{
    if (src == target) {
        log_info(
            "type cast is valid, source and target type are the same. cast is safe.");
        return 1;
    }

    if (target == any_type_spec()) {
        log_info("type cast is valid, target type is 'any'. cast is safe.");
        return 1;
    }

    Symbol *src_sym = get_symbol_by_id(src->sym_id);
    ASSERT(src_sym->kind == SYM_CLASS || src_sym->kind == SYM_INSTANCE ||
           src_sym->kind == SYM_TRAIT);
    Vector *bases = &((KlassSymbol *)src_sym)->bases;

    TypeSpec *ts;
    vector_foreach(ts, bases) {
        if (!ts) continue;
        if (ts == target) {
            log_info("type cast is valid, '%s' is a subtype of '%s'. cast is safe.",
                     src->signature, target->signature);
            return 1;
        }

        Symbol *base_sym = get_symbol_by_id(ts->sym_id);
        ASSERT(base_sym->kind == SYM_CLASS || base_sym->kind == SYM_INSTANCE ||
               base_sym->kind == SYM_TRAIT);
        if (check_type_cast(ts, target, ps)) return 1;
    }

    log_info(
        "checking type cast from '%s' to '%s', not found in base types. \nMaybe throw "
        "exception at runtime.",
        src->signature, target->signature);
    return 0;
}

static void parse_as(ParserState *ps, Expr *exp)
{
    AsExpr *as = (AsExpr *)exp;
    Expr *e = as->exp;
    e->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, e);
    if (!e->ts) return;

    TypeSpec *target_ts = as->type;
    Symbol *target_sym = get_symbol_by_id(target_ts->sym_id);
    if (!target_sym) {
        kl_error(as->loc, "type is not found.");
        return;
    }

    if (target_sym->kind != SYM_CLASS && target_sym->kind != SYM_TRAIT) {
        kl_error(as->loc, "target type of 'as' operator must be a class or trait type.");
        return;
    }

    as->safe_cast = check_type_cast(e->ts, target_ts, ps);

    exp->ts = target_ts;
    exp->sym = target_sym;
    log_info("'as' operator resolved, cast type to '%s'.", target_ts->signature);
}

static void parse_is(ParserState *ps, Expr *exp)
{
    IsExpr *is = (IsExpr *)exp;
    Expr *e = is->exp;
    e->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, e);
    if (!e->ts) return;

    TypeSpec *target_ts = is->type;
    Symbol *target_sym = get_symbol_by_id(target_ts->sym_id);
    if (!target_sym) {
        kl_error(is->loc, "type is not found.");
        return;
    }

    if (target_sym->kind != SYM_CLASS && target_sym->kind != SYM_TRAIT) {
        kl_error(is->loc, "target type of 'is' operator must be a class or trait type.");
        return;
    }

    is->result = check_type_cast(e->ts, target_ts, ps);

    exp->ts = bool_type_spec();
    exp->sym = get_symbol_by_id(exp->ts->sym_id);
    log_info("'is' operator resolved, cast type to '%s' is '%s'.", target_ts->signature,
             is->result ? "true" : "unknown (maybe false at runtime)");
}

void parser_visit_expr(ParserState *ps, Expr *exp)
{
    if (!exp) return;

    /* if errors is greater than MAX_ERRORS, stop parsing */
    if (ps->errors >= MAX_ERRORS) return;

    /* clang-format off */
    static void (*handlers[EXPR_MAX_KIND])(ParserState *, Expr *) = {
        [EXPR_ID_KIND]      = parse_ident,
        [EXPR_UNDER_KIND]   = parse_under,
        [EXPR_LITERAL_KIND] = parse_literal,
        [EXPR_SELF_KIND]    = parse_self,
        [EXPR_LIST_KIND]    = parse_list,
        [EXPR_TUPLE_KIND]   = parse_tuple,
        [EXPR_TYPE_KIND]    = parse_type,
        [EXPR_CALL_KIND]    = parse_call,
        [EXPR_DOT_KIND]     = parse_dot,
        [EXPR_INDEX_KIND]   = parse_index,
        [EXPR_SLICE_KIND]   = parse_slice,
        [EXPR_UNARY_KIND]   = parse_unary,
        [EXPR_BINARY_KIND]  = parse_binary,
        [EXPR_KW_KIND]      = parse_keyword,
        [EXPR_BANG_KIND]    = parse_bang,
        [EXPR_AS_KIND]      = parse_as,
        [EXPR_IS_KIND]      = parse_is,
    };
    /* clang-format on */

    handlers[exp->kind](ps, exp);

    if (!exp->ts) {
        if (ps->errors == 0) {
            kl_error(exp->loc, "cannot resolve expr's type.");
        }
    }
}

#ifdef __cplusplus
}
#endif
