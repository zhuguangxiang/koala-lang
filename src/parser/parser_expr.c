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
    IdentExpr *e = (IdentExpr *)exp;
    Ident *id = &((IdentExpr *)exp)->id;
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
    exp->desc = DESC_INCREF_GET(sym->desc);
    id->sym = sym;
    exp->sym = sym;
    exp->ir_val = sym->ir_val;
}

static void parse_lit_int(ParserState *ps, LitExpr *lit)
{
    /* expected type from lhs */
    TypeDesc *desc = lit->expected;
    int64_t val = lit->ival;
    if (val >= INT8_MIN && val <= INT8_MAX) {
        lit->len = 1;
    } else if (val >= INT16_MIN && val <= INT16_MAX) {
        lit->len = 2;
    } else if (val >= INT32_MIN && val <= INT32_MAX) {
        lit->len = 4;
    } else {
        lit->len = 8;
    }
}

static void parse_lit_float(ParserState *ps, LitExpr *lit)
{
    /* expected type from lhs */
    TypeDesc *desc = lit->expected;
    if (!desc) return;
}

static void parse_none(ParserState *ps, LitExpr *lit)
{
    if (lit->ctx != EXPR_CTX_LOAD) {
        kl_error(lit->loc, "none is readonly.");
        return;
    }

    TypeDesc *expected = lit->expected;
    if (expected) {
        if (expected->kind != TYPE_OPTIONAL_KIND) {
            kl_error(lit->loc, "expected an optional type.");
            return;
        }
        lit->desc = expected;
    }
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
            exp->ir_val = klr_const_int(lit->ival);
            break;
        }
        case LIT_EXPR_FLT: {
            parse_lit_float(ps, lit);
            exp->ir_val = klr_const_float(lit->fval);
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
            exp->ir_val = klr_const_str(lit->sval, lit->len);
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

static void parse_call(ParserState *ps, Expr *exp)
{
    CallExpr *call = (CallExpr *)exp;
    Expr *lhs = call->lhs;
    lhs->ctx = EXPR_CTX_CALL;
    parser_visit_expr(ps, call->lhs);
    if (!lhs->desc) return;
    exp->desc = DESC_INCREF_GET(lhs->desc);

    int size = vector_size(call->args);
    KlrValue *args[size + 1];

    Expr **arg_p;
    Expr *arg;
    vector_foreach(arg_p, call->args) {
        arg = *arg_p;
        arg->ctx = EXPR_CTX_LOAD;
        parser_visit_expr(ps, arg);
        if (!arg->desc) return;
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
        NULL,// parse_type,                      /* TYPE       */
        NULL,
        parse_call,                      /* CALL       */
        // parse_attr,                      /* ATTR       */
        // parse_tuple_get,                 /* TUPLE_GET  */
        // parse_index,                     /* INDEX      */
        // parse_unary,                     /* UNARY      */
        // parse_binary,                    /* BINARY     */
        // parse_range,                     /* RANGE      */
        // parse_is_expr,                   /* IS         */
        // parse_as_expr,                   /* AS         */
        NULL,                 /* OPT        */
        NULL,                   /* OPT_NOT    */
    };
    /* clang-format on */

    handlers[exp->kind](ps, exp);

    if (!exp->desc) {
        kl_error(exp->loc, "cannot resolve expr's type.");
    }
}

#ifdef __cplusplus
}
#endif
