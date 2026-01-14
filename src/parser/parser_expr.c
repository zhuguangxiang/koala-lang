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

static void parse_ident(ParserState *ps, Expr *exp)
{
    IdentExpr *id_exp = (IdentExpr *)exp;
    Ident *id = &id_exp->id;
    Symbol *sym = find_symbol(ps, id);
    if (!sym) {
        kl_error(id->loc, "'%s' is not found", id->name);
        return;
    }
    if (exp->ctx == EXPR_CTX_CALL) {
        if (sym->kind != SYM_FUNC && sym->kind != SYM_PROTO) {
            kl_error(id->loc, "'%s' is not callable", id->name);
            return;
        }
    }
    exp->ts = sym->ts;
    exp->sym = sym;
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
    lit->ts->int_flt_info.width = width;
    lit->ts->int_flt_info.sign = sign;
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

static void check_call_args(Vector *params, Vector *exprs)
{
    ArgInfo **arg_p;
    ArgInfo *arg;
    vector_foreach(arg_p, params) {
        arg = *arg_p;
    }
}

static void parse_type(ParserState *ps, Expr *exp) { TypeExpr *texp = (TypeExpr *)exp; }

static void parse_call(ParserState *ps, Expr *exp)
{
    CallExpr *call = (CallExpr *)exp;

    Expr *lhs = call->lhs;
    lhs->ctx = EXPR_CTX_CALL;
    parser_visit_expr(ps, call->lhs);
    if (!lhs->ts) return;

    exp->ts = lhs->ts;

    int size = vector_size(call->args);
    KlrValue *args[size + 1];

    Expr **arg_p;
    Expr *arg;
    vector_foreach(arg_p, call->args) {
        arg = *arg_p;
        arg->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, arg);
        if (!arg->ts) return;
        args[i__] = arg->ir_val;
    }

    args[size] = NULL;

    if (ps->errors > 0) return;

    Symbol *lhs_sym = lhs->sym;
    FuncSymbol *fn_sym = NULL;
    if (lhs_sym->kind == SYM_FUNC) {
        fn_sym = (FuncSymbol *)lhs_sym;
        check_call_args(fn_sym->params, call->args);
        if (ps->errors > 0) return;
    }

    // codegen

    ParserScope *sc = ps->scope;
    KlrValue *ir_val = lhs->ir_val;
    KlrBuilder bldr;
    klr_builder_end(&bldr, sc->bb);

    KlrValue *ret = klr_build_call(&bldr, ir_val, args, size, "");
    exp->ir_val = ret;
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

static void parse_keyword(ParserState *ps, Expr *exp) {}

void parser_visit_expr(ParserState *ps, Expr *exp)
{
    if (!exp) return;

    /* if errors is greater than MAX_ERRORS, stop parsing */
    if (ps->errors >= MAX_ERRORS) return;

    /* clang-format off */
    static void (*handlers[])(ParserState *, Expr *) = {
        NULL,                            /* UNKNOWN    */
        parse_ident,                     /* ID         */
        NULL, // parse_under,                     /* UNDER      */
        parse_literal,                   /* LITERAL    */
        NULL,// parse_self,                      /* SELF       */
        NULL,// parse_super,                     /* SUPER      */
        NULL,// parse_array_expr,                /* ARRAY      */
        NULL,// parse_map_expr,                  /* MAP        */
        NULL,                            /* MAP_ENTRY  */
        NULL,// parse_tuple_expr,                /* TUPLE      */
        NULL,// parse_anony,                     /* ANONY      */
        NULL,
        parse_type,                      /* TYPE       */
        parse_call,                      /* CALL       */
        NULL,
        NULL, // parse_attr,                      /* ATTR       */
        NULL, // parse_tuple_get,                 /* TUPLE_GET  */
        NULL, // parse_index,                     /* INDEX      */
        NULL, // parse_unary,                     /* UNARY      */
        parse_binary,                    /* BINARY     */
        NULL, // parse_range,                     /* RANGE      */
        parse_keyword,                   /* KW         */
        // parse_is_expr,                   /* IS         */
        // parse_as_expr,                   /* AS         */
        NULL,                 /* OPT        */
        NULL,                   /* OPT_NOT    */
    };
    /* clang-format on */

    handlers[exp->kind](ps, exp);

    if (!exp->ts) {
        kl_error(exp->loc, "cannot resolve expr's type.");
    }
}

#ifdef __cplusplus
}
#endif
