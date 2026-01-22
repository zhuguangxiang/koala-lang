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
#define print_type_spec(ts) do {        \
    BUF(buf);                           \
    type_spec_print(ts, &buf);          \
    log_info("  '%s'", BUF_STR(buf));   \
    FINI_BUF(buf);                      \
} while (0)
/* clang-format on */
#else
#define print_type_spec(ts) ((void *)(ts))
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
    log_debug("ident resolved: %s", sym->name);
    print_type_spec(sym->ts);
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
            parse_lit_int(ps, lit);
            break;
        }
        case LIT_EXPR_FLT: {
            parse_lit_float(ps, lit);
            break;
        }
        case LIT_EXPR_BOOL: {
            // do nothing
            break;
        }
        case LIT_EXPR_STR: {
            if (check_utf8(lit->sval, lit->len) < 0) {
                kl_error(exp->loc, "invalid utf8 string");
            }
            break;
        }
        case LIT_EXPR_NONE: {
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

            log_debug("[check_call_args] arg: '%s' check type compatible", arg->name);
            if (!type_spec_compatible(arg->ts, e->ts)) {
                kl_error(e->loc, "argument type is not compatible.");
                return;
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
    log_debug("type '%s' is resolved as: ", exp->sym->name);
    print_type_spec(exp->ts);
    return;
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

    if (ps->errors > 0) return;

    Symbol *lhs_sym = lhs->sym;
    Vector *params = NULL;
    if (lhs_sym->kind == SYM_VAR) {
        TypeSpec *ts = lhs_sym->ts;

        log_debug("call lhs is variable of type:");
        print_type_spec(ts);

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
    } else if (lhs_sym->kind == SYM_FUNC || lhs_sym->kind == SYM_PROTO) {
        FuncSymbol *fn_sym = (FuncSymbol *)lhs_sym;
        exp->ts = fn_sym->ret;
        params = fn_sym->params;
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

static void parse_dot(ParserState *ps, Expr *exp) {}

static void parse_index(ParserState *ps, Expr *exp)
{
    IndexExpr *index = (IndexExpr *)exp;

    Expr *lhs = index->lhs;
    lhs->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, lhs);
    if (!lhs->ts) return;

    // Expr *idx = index->index;
    // idx->ctx = EXPR_CTX_LOAD;
    // parser_visit_expr(ps, idx);
    // if (!idx->ts) return;
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
        default:
            return "unknown_op";
    }
}

static char *get_binary_op_char(BiOpKind op)
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
        default:
            UNREACHABLE();
            return "";
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

    rhs->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, rhs);
    if (!rhs->ts) return;

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
                 get_binary_op_char(op));
        return;
    }

    Vector *args = ((FuncSymbol *)fn)->params;
    if (vector_size(args) != 1) {
        kl_error(bin->op_loc, "invalid operator '%s' definition.",
                 get_binary_op_char(op));
        return;
    }

    ArgInfo *arg_info = vector_get_object(args, 0);
    if (!arg_info) {
        kl_error(bin->op_loc, "invalid operator '%s' definition.",
                 get_binary_op_char(op));
        return;
    }

    TypeSpec *arg_ts = arg_info->ts;
    if (arg_ts != rhs->ts) {
        kl_error(bin->op_loc, "invalid operator '%s' definition.",
                 get_binary_op_char(op));
        return;
    }
    exp->ts = lhs->ts;
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
    static void (*handlers[])(ParserState *, Expr *) = {
        NULL,                            /* UNKNOWN    */
        parse_ident,                     /* ID         */
        parse_under,                     /* UNDER      */
        parse_literal,                   /* LITERAL    */
        parse_self,                      /* SELF       */
        NULL,                            /* ARRAY      */
        NULL,                            /* MAP        */
        NULL,                            /* MAP_ENTRY  */
        NULL,                            /* TUPLE      */
        NULL,                            /* SET        */
        NULL,                            /* ANONY      */
        parse_type,                      /* TYPE       */
        parse_call,                      /* CALL       */
        parse_dot,                       /* DOT        */
        parse_index,                     /* INDEX      */
        NULL,                            /* SLICE      */
        NULL,                            /* UNARY      */
        parse_binary,                    /* BINARY     */
        NULL,                            /* RANGE      */
        parse_keyword,                   /* KW         */
        NULL,                            /* IS         */
        NULL,                            /* AS         */
        NULL,                            /* IN         */
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
