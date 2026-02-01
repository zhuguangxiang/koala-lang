/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
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

    exp->ts = sym->ts;
    exp->sym = sym;

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

    log_debug("ident resolved: %s", sym->name);
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

    unsigned __int128 phys_max = (width == 8) ? (unsigned __int128)0xFFFFFFFFFFFFFFFFULL
                                              : ((unsigned __int128)1 << (width * 8)) - 1;

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
            if ((unsigned __int128)val > phys_max) {
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
            min_limit = -(max_limit + 1); // -2^(n-1)
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

    // update literal integer's type as expected type
    lit->ts = ts;
    lit->sign = sign;
    lit->len = width;
    lit->ival = (uint64_t)val;
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
            break;
        }
        case LIT_EXPR_FLT: {
            log_info("literal float");
            parse_lit_float(ps, lit);
            break;
        }
        case LIT_EXPR_BOOL: {
            log_info("literal bool");
            // do nothing
            break;
        }
        case LIT_EXPR_STR: {
            log_info("literal string '%s'", lit->sval);
            if (check_utf8(lit->sval, lit->len) < 0) {
                kl_error(exp->loc, "invalid utf8 string");
            }
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

static void parse_self(ParserState *ps, Expr *exp) {}

static void check_call_args(Vector *params, Vector *exprs, ParserState *ps, Loc fn_loc)
{
    Expr *e;
    ArgInfo *arg;
    vector_foreach(arg, params) {
        if (!arg) continue;
        e = vector_get_object(exprs, i__);
        // if arg has default value, the caller can pass value, kw-arg or skip it
        if (arg->dfl_val_idx > 0) {
            if (!e) {
                // skip this arg, use default value
                log_debug("[check_call_args] kw-arg: %s, no value passed, skip left.",
                          arg->name);
                return;
            }

            if (e->kind == EXPR_KW_KIND) {
                // positional arg
                int size = vector_size(exprs);
                for (int i = i__; i < size; i++) {
                    e = vector_get_object(exprs, i);
                    ASSERT(e->kind == EXPR_KW_KIND);
                    KeyWordExpr *kw = (KeyWordExpr *)e;
                    if (strcmp(kw->key.name, arg->name) != 0) {
                        continue;
                    }
                    log_debug(
                        "[check_call_args] kw-arg: '%s', pass kw-arg, check kw-value "
                        "type compatible",
                        arg->name);
                    if (!type_spec_compatible(arg->ts, e->ts)) {
                        kl_error(e->loc, "argument type is not compatible.");
                        return;
                    }
                }
            } else {
                log_debug(
                    "[check_call_args] kw-arg: '%s', pass value only, check value type "
                    "compatible",
                    arg->name);
                if (!type_spec_compatible(arg->ts, e->ts)) {
                    kl_error(e->loc, "argument type is not compatible.");
                    return;
                }
            }
        } else {
            if (!e) {
                kl_error(fn_loc, "too few arguments in function call.");
                return;
            }

            if (!type_spec_compatible(arg->ts, e->ts)) {
                kl_error(e->loc, "argument type is not compatible.");
                return;
            }
            log_debug("[check_call_args] arg: '%s' type is compatible", arg->name);
            log_debug("lhs:");
            log_type_spec(arg->ts);
            log_debug("rhs:");
            log_type_spec(e->ts);
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
    log_debug("type '%s' is resolved as: ", exp->sym->name);
    log_type_spec(exp->ts);
    return;
}

static TypeSpec *instance_type_spec(TypeSpec *ts, Vector *tp_args, ParserState *ps)
{
    TypeSpec *inst_ts;
    if (ts->kind == TYPE_GENERIC_VAR) {
        inst_ts = vector_get_object(tp_args, ts->generic_var.index);
    } else if (ts->kind == TYPE_SPECIALIZED) {
        Symbol *sym = get_symbol_by_id(ts->sym_id);
        sym = find_or_add_instance(ps->stbl, sym, tp_args);
        ASSERT(sym->kind == SYM_INSTANCE);
        InstanceSymbol *inst_sym = (InstanceSymbol *)sym;
        inst_ts = inst_sym->instance_ts;
    } else {
        inst_ts = ts;
    }
    return inst_ts;
}

static Vector *build_instance_params(Vector *params, Vector *tp_args, ParserState *ps)
{
    Vector *inst_params = vector_create_ptr();
    ArgInfo *arg;
    vector_foreach(arg, params) {
        if (!arg) continue;
        TypeSpec *ts = instance_type_spec(arg->ts, tp_args, ps);
        ArgInfo *inst_arg = mm_alloc_obj(inst_arg);
        inst_arg->name = arg->name;
        inst_arg->ts = ts;
        inst_arg->dfl_val_idx = arg->dfl_val_idx;
        vector_push_back(inst_params, &inst_arg);
    }
    return inst_params;
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

        log_debug("call lhs is variable of type:");
        log_type_spec(ts);

        if (ts->kind == TYPE_PROTO) {
            log_debug("call lhs is proto variable.");
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
        // constructor call
        KlassSymbol *cls_sym = (KlassSymbol *)lhs_sym;
        Symbol *init_fn_sym = stbl_get(cls_sym->stbl, "__init__");
        if (!init_fn_sym) {
            kl_error(lhs->loc, "class '%s' has no constructor.", lhs_sym->name);
            return;
        }
        // func call type is instance type
        exp->ts = cls_sym->instance_ts;
        // exp->sym = cls_sym;
        params = ((FuncSymbol *)init_fn_sym)->params;
    } else if (lhs_sym->kind == SYM_FUNC || lhs_sym->kind == SYM_INTF) {
        FuncSymbol *fn_sym = (FuncSymbol *)lhs_sym;
        exp->ts = fn_sym->ret;
        params = fn_sym->params;
    } else if (lhs_sym->kind == SYM_INSTANCE) {
        InstanceSymbol *inst_sym = (InstanceSymbol *)lhs_sym;
        Symbol *init_fn_sym = stbl_get(inst_sym->stbl, "__init__");
        if (!init_fn_sym) {
            KlassSymbol *origin = (KlassSymbol *)inst_sym->origin;
            Symbol *_init_fn_sym = stbl_get(origin->stbl, "__init__");
            if (!_init_fn_sym) {
                kl_error(lhs->loc, "class '%s' has no constructor.", origin->name);
                return;
            }

            // params
            Vector *inst_params = build_instance_params(
                ((FuncSymbol *)_init_fn_sym)->params, inst_sym->tp_args, ps);

            init_fn_sym = stbl_add_func(inst_sym->stbl, "__init__", NULL, no_type_spec(),
                                        inst_params, 0, NULL, NULL);
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
    // // codegen

    // ParserScope *sc = ps->scope;
    // KlrValue *ir_val = lhs->ir_val;
    // KlrBuilder bldr;
    // klr_builder_end(&bldr, sc->bb);

    // KlrValue *ret = klr_build_call(&bldr, ir_val, args, size, "");
    // exp->ir_val = ret;
}

static void parse_dot(ParserState *ps, Expr *exp)
{
    DotExpr *dot = (DotExpr *)exp;

    Expr *lhs = dot->lhs;
    lhs->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, lhs);
    if (!lhs->ts) return;

    if (type_is_optional(lhs->ts)) {
        kl_error(lhs->loc, "optional cannot use dot operator.");
        return;
    }

    Symbol *lhs_ts_sym = get_symbol_by_id(lhs->ts->sym_id);
    if (!lhs_ts_sym) {
        kl_error(lhs->loc, "type is not found.");
        return;
    }

    HashMap *lhs_stbl = lhs_ts_sym->stbl;
    Ident *ident = &dot->id;
    Symbol *sym = stbl_get(lhs_stbl, ident->name);
    if (sym) {
        exp->ts = sym->ts;
        exp->sym = sym;
        log_debug("dot member resolved: %s", sym->name);
        if (sym->kind == SYM_FUNC) {
            log_info("ret type is:");
            log_type_spec(((FuncSymbol *)sym)->ret);
        } else {
            log_type_spec(sym->ts);
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
                    instance_type_spec(origin_var_sym->ts, inst_sym->tp_args, ps);
                Symbol *inst_var_sym = stbl_add_var(lhs_stbl, origin_var_sym->name, ts,
                                                    origin_var_sym->flags);
                exp->ts = inst_var_sym->ts;
                exp->sym = inst_var_sym;
                log_debug("dot member resolved: %s", inst_var_sym->name);
                log_type_spec(inst_var_sym->ts);
                return;
            } else if (sym->kind == SYM_FUNC) {
                log_info("found func '%s' from origin klass '%s'.", ident->name,
                         origin->name);
                // method of instance
                FuncSymbol *origin_fn_sym = (FuncSymbol *)sym;

                // params
                Vector *inst_params =
                    build_instance_params(origin_fn_sym->params, inst_sym->tp_args, ps);
                // return type
                TypeSpec *ret_ts =
                    instance_type_spec(origin_fn_sym->ret, inst_sym->tp_args, ps);

                // create function symbol for instance method
                Symbol *inst_fn_sym =
                    stbl_add_func(lhs_stbl, origin_fn_sym->name, NULL, ret_ts,
                                  inst_params, origin_fn_sym->flags, NULL, NULL);
                TypeSpec *fn_ts = func_type_spec_from_arginfo(inst_params, ret_ts);
                inst_fn_sym->ts = fn_ts;
                exp->ts = fn_ts;
                exp->sym = inst_fn_sym;
                log_debug("dot member resolved: %s", inst_fn_sym->name);
                log_type_spec(inst_fn_sym->ts);
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

static void parse_index(ParserState *ps, Expr *exp)
{
    IndexExpr *index = (IndexExpr *)exp;

    Expr *lhs = index->lhs;
    lhs->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, lhs);
    if (!lhs->ts) return;

    if (lhs->ts->kind == TYPE_TYPE) {
        // generic types
        KlassSymbol *kls_sym = (KlassSymbol *)lhs->sym;
        if (kls_sym->kind != SYM_CLASS) {
            kl_error(lhs->loc, "type '%s' is not a class type.", lhs->sym->name);
            return;
        }

        int tp_size = vector_size(kls_sym->tps);
        if (tp_size != vector_size(index->vec)) {
            kl_error(exp->loc,
                     "type '%s' expects %d type arguments, but %d were provided.",
                     kls_sym->name, tp_size, vector_size(index->vec));
            return;
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

            TypeParamSymbol *tp_sym =
                (TypeParamSymbol *)vector_get_object(kls_sym->tps, i__);
            TypeSpec *bound_ts;
            vector_foreach(bound_ts, tp_sym->bound) {
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
            vector_push_back(tp_args, &arg_ts);
        }

        // create or find instance symbol(List<int>)
        Symbol *inst_sym = find_or_add_instance(ps->stbl, (Symbol *)kls_sym, tp_args);
        exp->ts = inst_sym->ts;
        exp->sym = inst_sym;
        log_debug("generic type instance created/got: %s", inst_sym->name);
        log_type_spec(inst_sym->ts);
    } else {
        kl_error(lhs->loc, "only generic types support type arguments.");
        return;
    }
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

static void parse_binary(ParserState *ps, Expr *exp)
{
    BinaryExpr *bin = (BinaryExpr *)exp;
    BiOpKind op = bin->op;
    Expr *lhs = bin->lhs;
    Expr *rhs = bin->rhs;

    lhs->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, lhs);
    if (!lhs->ts) return;

    if (type_is_optional(lhs->ts) && (op != BINARY_EQ && op != BINARY_NEQ)) {
        Symbol *opt_sym = lhs->sym;
        if (opt_sym && opt_sym->kind == SYM_SHADOW_VAR) {
            // check shadow variable null state
            ShadowVarSymbol *shadow_sym = (ShadowVarSymbol *)opt_sym;
            log_info("  note: symbol '%s' is a shadow variable.", opt_sym->name);
            log_info("  value is null: %s", shadow_sym->is_null ? "true" : "false");
            if (shadow_sym->is_null) {
                kl_error(
                    bin->op_loc,
                    "optional type cannot be used with '%s' operator when value is null.",
                    get_binary_op_str(op));
                return;
            }
        }

        kl_error(bin->op_loc, "optional type cannot be used with '%s' operator.",
                 get_binary_op_str(op));
        return;
    }

    rhs->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, rhs);
    if (!rhs->ts) return;

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
            kl_error(
                bin->op_loc,
                "cannot compare optional type with non-optional type for '%s' operator.",
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

    if (sym->kind != SYM_CLASS) {
        kl_error(bin->op_loc, "type is not a class type.");
        return;
    }

    char *op_name = get_binary_op_name(op);
    HashMap *stbl = ((KlassSymbol *)sym)->stbl;
    Symbol *fn = stbl_get(stbl, op_name);
    if (!fn) {
        kl_error(bin->op_loc, "operator '%s' is not defined for this type.",
                 get_binary_op_str(op));
        return;
    }

    Vector *args = ((FuncSymbol *)fn)->params;
    if (vector_size(args) != 1) {
        kl_error(bin->op_loc, "expected %d argument for operator '%s', but got 1.",
                 vector_size(args), get_binary_op_str(op));
        return;
    }

    ArgInfo *arg_info = vector_get_object(args, 0);
    ASSERT(arg_info);

    TypeSpec *arg_ts = arg_info->ts;
    if (arg_ts != rhs->ts) {
        kl_error(bin->op_loc, "argument type mismatch for operator '%s'.",
                 get_binary_op_str(op));
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

    log_info("binary operator '%s' resolved.", get_binary_op_str(op));
    log_type_spec(exp->ts);
}

static void parse_keyword(ParserState *ps, Expr *exp)
{
    KeyWordExpr *kw = (KeyWordExpr *)exp;
    Expr *value = kw->value;
    value->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, value);
    if (!value->ts) return;
    exp->ts = value->ts;
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
        [EXPR_TYPE_KIND]    = parse_type,
        [EXPR_CALL_KIND]    = parse_call,
        [EXPR_DOT_KIND]     = parse_dot,
        [EXPR_INDEX_KIND]   = parse_index,
        [EXPR_BINARY_KIND]  = parse_binary,
        [EXPR_KW_KIND]      = parse_keyword,
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
