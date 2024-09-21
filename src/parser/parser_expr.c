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

static void parse_lit_int(ParserState *ps, LitExpr *lit)
{
    /* expected type from lhs */
    TypeDesc *desc = lit->expected;
    if (!desc) return;
}

static void parse_lit_float(ParserState *ps, LitExpr *lit)
{
    /* expected type from lhs */
    TypeDesc *desc = lit->expected;
    if (!desc) return;
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
        default: {
            UNREACHABLE();
            break;
        }
    }
}

static void parse_none(ParserState *ps, Expr *exp)
{
    if (exp->ctx != EXPR_CTX_LOAD) {
        kl_error(exp->loc, "none is readonly.");
        return;
    }

    TypeDesc *expected = exp->expected;
}

void parse_expr(ParserState *ps, Expr *exp)
{
    if (!exp) return;

    /* if errors is greater than MAX_ERRORS, stop parsing */
    if (ps->errors >= MAX_ERRORS) return;

    /* clang-format off */
    static void (*handlers[])(ParserState *, Expr *) = {
        NULL,                            /* UNKNOWN    */
        NULL, // parse_ident,                     /* ID         */
        NULL, // parse_under,                     /* UNDER      */
        parse_literal,                   /* LITERAL    */
        parse_none,                         /* NONE       */
        // parse_self,                      /* SELF       */
        // parse_super,                     /* SUPER      */
        // parse_array_expr,                /* ARRAY      */
        // parse_map_expr,                  /* MAP        */
        // NULL,                            /* MAP_ENTRY  */
        // parse_tuple_expr,                /* TUPLE      */
        // parse_anony,                     /* ANONY      */
        // parse_type,                      /* TYPE       */
        // parse_call,                      /* CALL       */
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
