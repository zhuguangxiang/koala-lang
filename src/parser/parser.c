/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
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

void do_klass_typeparams(ParserStateRef ps, char *name)
{
    ps->in_angle = 0;
}

void show_error_detail(ParserStateRef ps, int row, int col)
{
    // clang-format off
    printf("%5d | %s\n", row, SBUF_STR(ps->linebuf));
    printf("      | %*s" RED_COLOR(^) "\n", col, "");
    // clang-format on
}

static void parse_import(ParserStateRef ps, StmtRef stmt)
{
}

static void parse_constdecl(ParserStateRef ps, StmtRef stmt)
{
}

static void parse_vardecl(ParserStateRef ps, StmtRef stmt)
{
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
