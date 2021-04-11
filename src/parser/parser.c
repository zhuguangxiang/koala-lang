/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2020-2021 James <zhuguangxiang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "parser.h"
#include "color.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Module, path as key */
static HashMap modules;

/* std module */
static Module _std_;

static int _mod_equal_(void *e1, void *e2)
{
    Module *m1 = e1;
    Module *m2 = e2;
    if (m1 == m2) return 1;
    if (m1->path == m2->path) return 1;
    return !strcmp(m1->path, m2->path);
}

static void _mod_free_(void *e, void *arg)
{
    Module *mod = e;
    printf("free module(parser) '%s'", mod->path);
    stbl_free(mod->stbl);
    MemFree(mod);
}

void init_parser(void)
{
    HashMapInit(&modules, _mod_equal_);
}

void fini_parser(void)
{
    HashMapFini(&modules, _mod_free_, NULL);
}

void do_klass_typeparams(ParserStateRef ps, char *name)
{
    ps->in_angle = 0;
}

void show_error_detail(ParserStateRef ps, int row, int col)
{
    // clang-format off
    /*
    printf("%5d | %s\n", row, BUF_STR(ps->linebuf));
    printf("      | %*s" RED_COLOR(^) "\n", col, "");
    */
    // clang-format on
}

static void parse_nil(ParserStateRef ps, ExprRef exp)
{
}

static void parse_self(ParserStateRef ps, ExprRef exp)
{
}

static void parse_super(ParserStateRef ps, ExprRef exp)
{
}

static void parse_int(ParserStateRef ps, ExprRef exp)
{
    ExprKRef k = (ExprKRef)exp;
    printf("parse integer '%ld'\n", k->ival);
}

static void parse_float(ParserStateRef ps, ExprRef exp)
{
}

static void parse_bool(ParserStateRef ps, ExprRef exp)
{
}

static void parse_char(ParserStateRef ps, ExprRef exp)
{
}

static void parse_str(ParserStateRef ps, ExprRef exp)
{
}

static void parse_ident(ParserStateRef ps, ExprRef exp)
{
    ExprIdRef id = (ExprIdRef)exp;
    printf("parse ident '%s'\n", id->name);
    SymbolRef sym = stbl_get(ps->mod->stbl, id->name);
    if (!sym) {
        printf("cannot find symbol '%s'\n", id->name);
        return;
    }

    if (sym->kind == SYM_VAR) {
        printf("symbol '%s' is variable\n", id->name);
    }
}

static void parse_under(ParserStateRef ps, ExprRef exp)
{
}

static void parse_unary(ParserStateRef ps, ExprRef exp)
{
}

static void parse_binary(ParserStateRef ps, ExprRef exp)
{
}

static void parse_ternary(ParserStateRef ps, ExprRef exp)
{
}

static void parse_attr(ParserStateRef ps, ExprRef exp)
{
}

static void parse_subscr(ParserStateRef ps, ExprRef exp)
{
}

static void parse_call(ParserStateRef ps, ExprRef exp)
{
}

static void parse_slice(ParserStateRef ps, ExprRef exp)
{
}

static void parse_dottuple(ParserStateRef ps, ExprRef exp)
{
}

static void parse_tuple(ParserStateRef ps, ExprRef exp)
{
}

static void parse_array(ParserStateRef ps, ExprRef exp)
{
}

static void parse_map(ParserStateRef ps, ExprRef exp)
{
}

static void parse_anony(ParserStateRef ps, ExprRef exp)
{
}

static void parse_is(ParserStateRef ps, ExprRef exp)
{
}

static void parse_as(ParserStateRef ps, ExprRef exp)
{
}

static void parse_range(ParserStateRef ps, ExprRef exp)
{
}

void parser_visit_expr(ParserStateRef ps, ExprRef exp)
{
    /* if errors is greater than MAX_ERRORS, stop parsing */
    if (ps->errors >= MAX_ERRORS) return;

    // clang-format off
    static void (*handlers[])(ParserStateRef, ExprRef) = {
        NULL,                   /* INVALID              */
        parse_nil,              /* NIL_KIND             */
        parse_self,             /* SELF_KIND            */
        parse_super,            /* SUPER_KIND           */
        parse_int,              /* INT_KIND             */
        parse_float,            /* FLOAT_KIND           */
        parse_bool,             /* BOOL_KIND            */
        parse_char,             /* CHAR_KIND            */
        parse_str,              /* STR_KIND             */
        parse_ident,            /* ID_KIND              */
        parse_under,            /* UNDER_KIND           */
        parse_unary,            /* UNARY_KIND           */
        parse_binary,           /* BINARY_KIND          */
        parse_ternary,          /* TERNARY_KIND         */
        parse_attr,             /* ATTRIBUTE_KIND       */
        parse_subscr,           /* SUBSCRIPT_KIND       */
        parse_call,             /* CALL_KIND            */
        parse_slice,            /* SLICE_KIND           */
        parse_dottuple,         /* DOT_TUPLE_KIND       */
        parse_tuple,            /* TUPLE_KIND           */
        parse_array,            /* ARRAY_KIND           */
        parse_map,              /* MAP_KIND             */
        parse_anony,            /* ANONY_KIND           */
        parse_is,               /* IS_KIND              */
        parse_as,               /* AS_KIND              */
        parse_range,            /* RANGE_KIND           */
    };
    // clang-format on

    handlers[exp->kind](ps, exp);
}

static void parse_import(ParserStateRef ps, StmtRef stmt)
{
}

static void parse_constdecl(ParserStateRef ps, StmtRef stmt)
{
}

static void parse_vardecl(ParserStateRef ps, StmtRef stmt)
{
    VarDeclStmt *s = (VarDeclStmt *)stmt;
    ExprRef exp = s->exp;
    TypeRef ty = s->ty;
    Ident *id = &s->name;
    Symbol *sym = stbl_get(ps->mod->stbl, id->name);
    assert(sym);
    if (exp) {
        // parse initial expr
        printf("parse var '%s' initial expr\n", id->name);
    }
}

static void parse_assign(ParserStateRef ps, StmtRef stmt)
{
}

static void parse_funcdecl(ParserStateRef ps, StmtRef stmt)
{
}

static void parse_return(ParserStateRef ps, StmtRef stmt)
{
}

static void parse_expr(ParserStateRef ps, StmtRef stmt)
{
    printf("parse expr\n");
    ExprStmtRef s = (ExprStmtRef)stmt;
    ExprRef exp = s->exp;
    exp->ctx = EXPR_CTX_LOAD;
    parser_visit_expr(ps, exp);
}

static void parse_block(ParserStateRef ps, StmtRef stmt)
{
}

static void parse_ifunc(ParserStateRef ps, StmtRef stmt)
{
}

static void parse_class(ParserStateRef ps, StmtRef stmt)
{
}

static void parse_trait(ParserStateRef ps, StmtRef stmt)
{
}

static void parse_enum(ParserStateRef ps, StmtRef stmt)
{
}

static void parse_break(ParserStateRef ps, StmtRef stmt)
{
}

static void parse_continue(ParserStateRef ps, StmtRef stmt)
{
}

static void parse_if(ParserStateRef ps, StmtRef stmt)
{
}

static void parse_while(ParserStateRef ps, StmtRef stmt)
{
}

static void parse_for(ParserStateRef ps, StmtRef stmt)
{
}

static void parse_match(ParserStateRef ps, StmtRef stmt)
{
}

void parse_stmt(ParserStateRef ps, StmtRef stmt)
{
    /* if errors is greater than MAX_ERRORS, stop parsing */
    if (ps->errors >= MAX_ERRORS) return;

    // clang-format off
    static void (*handlers[])(ParserStateRef, StmtRef) = {
        NULL,               /* INVALID          */
        parse_import,       /* IMPORT_KIND      */
        parse_constdecl,    /* CONST_KIND       */
        parse_vardecl,      /* VAR_KIND         */
        parse_assign,       /* ASSIGN_KIND      */
        parse_funcdecl,     /* FUNC_KIND        */
        parse_return,       /* RETURN_KIND      */
        parse_expr,         /* EXPR_KIND        */
        parse_block,        /* BLOCK_KIND       */
        parse_ifunc,        /* IFUNC_KIND       */
        parse_class,        /* CLASS_KIND       */
        parse_trait,        /* TRAIT_KIND       */
        parse_enum,         /* ENUM_KIND        */
        parse_break,        /* BREAK_KIND       */
        parse_continue,     /* CONTINUE_KIND    */
        parse_if,           /* IF_KIND          */
        parse_while,        /* WHILE_KIND       */
        parse_for,          /* FOR_KIND         */
        parse_match,        /* MATCH_KIND       */
    };
    // clang-format on

    handlers[stmt->kind](ps, stmt);
}

#ifdef __cplusplus
}
#endif
