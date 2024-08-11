/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "log.h"
#include "parser.h"

#ifdef __cplusplus
extern "C" {
#endif

static void parse_lit_int(ParserState *ps, LitExpr *lit)
{
    /* expected type from lhs */
    TypeDesc *desc = lit->expected;
    if (!desc) return;

    int64_t val = lit->ival;
    NumberDesc *num = (NumberDesc *)desc;
    switch (num->width) {
        case 8: {
            if (val > INT8_MAX || val < INT8_MIN) {
                kl_error(lit->loc, "Number %ld is out of Int8 range", val);
            } else {
                // update expr's type
                lit->desc = (TypeDesc *)&int8_desc;
            }
            break;
        }
        case 16: {
            if (val > INT16_MAX || val < INT16_MIN) {
                kl_error(lit->loc, "Number %ld is out of Int16 range", val);
            } else {
                // update expr's type
            }
            break;
        }
        case 32: {
            if (val > INT32_MAX || val < INT32_MIN) {
                kl_error(lit->loc, "Number %ld is out of Int32 range", val);
            } else {
                // update expr's type
            }
            break;
        }
        case 64: {
            // do nothing: the default type of literal integer is int64
            break;
        }
        default: {
            break;
        }
    }
}

static void parse_lit_float(ParserState *ps, LitExpr *lit)
{
    /* expected type from lhs */
    TypeDesc *desc = lit->expected;
    if (!desc) return;

    // double val = lit->fval;
    NumberDesc *num = (NumberDesc *)desc;
    switch (num->width) {
        case 32: {
            // update expr's type
            lit->desc = (TypeDesc *)&float32_desc;
            break;
        }
        case 64: {
            // do nothing: the default type of literal float is int64
            break;
        }
        default: {
            break;
        }
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
        // case LIT_EXPR_STR: {
        //     exp->type = expr_type_dync(desc_from_str());
        //     break;
        // }
        default: {
            UNREACHABLE();
            break;
        }
    }
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
