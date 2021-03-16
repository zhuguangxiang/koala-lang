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

/* prologue */

%{

#include "parser.h"

#define yyerror(loc, ps, scanner, msg) ((void)0)

#define line(loc) ((loc).first_line)
#define column(loc) ((loc).first_column)

%}

%union {
    int64_t ival;
    double fval;
    int cval;
    char *sval;
}

%token IMPORT
%token FINAL
%token PUB
%token VAR
%token FUNC
%token CLASS
%token TRAIT
%token ENUM
%token IF
%token ELSE
%token WHILE
%token FOR
%token MATCH
%token BREAK
%token CONTINUE
%token RETURN
%token IN
%token BY
%token AS
%token IS

%token SELF
%token SUPER
%token TRUE
%token FALSE
%token NIL

%token INT8
%token INT16
%token INT32
%token INT64
%token INT
%token FLOAT32
%token FLOAT64
%token FLOAT
%token BOOL
%token CHAR
%token STRING
%token ANY

%token AND
%token OR
%token NOT
%token EQ
%token NE
%token GE
%token LE

%token FREE_ASSIGN
%token PLUS_ASSIGN
%token MINUS_ASSIGN
%token MULT_ASSIGN
%token DIV_ASSIGN
%token MOD_ASSIGN
%token AND_ASSIGN
%token OR_ASSIGN
%token XOR_ASSIGN
%token SHL_ASSIGN
%token SHR_ASSIGN

%token L_SHIFT
%token R_ANGLE_SHIFT

%token <ival> INT_LITERAL
%token <fval> FLOAT_LITERAL
%token <sval> STRING_LITERAL
%token <sval> ID

%locations
%parse-param {ParserStateRef ps}
%parse-param {void *scanner}
%define api.pure full
%lex-param {void *scanner}
%code provides {
  int yylex(YYSTYPE *yylval_param, YYLTYPE *yylloc, void *yyscanner);
}

%start units

/* grammar rules */

%%

units
    : unit
    | units unit
    ;

unit
    : expr ';'
    {
        if (ps->interactive) {
            ps->more = 0;
        }
    }
    ;

expr
    : cond_expr
    ;

cond_expr
    : or_expr
    | or_expr '?' or_expr ':' or_expr
    ;

or_expr
    : and_expr
    | or_expr OR and_expr
    ;

and_expr
    : bit_or_expr
    | and_expr AND bit_or_expr
    ;

bit_or_expr
    : bit_xor_expr
    | bit_or_expr '|' bit_xor_expr
    ;

bit_xor_expr
    : bit_and_expr
    | bit_xor_expr '^' bit_and_expr
    ;

bit_and_expr
    : equality_expr
    | bit_and_expr '&' equality_expr
    ;

equality_expr
    : relation_expr
    | equality_expr EQ relation_expr
    | equality_expr NE relation_expr
    ;

relation_expr
    : shift_expr
    | relation_expr '<' shift_expr
    | relation_expr '>' shift_expr
    | relation_expr LE shift_expr
    | relation_expr GE shift_expr
    ;

shift_expr
    : add_expr
    | shift_expr R_ANGLE_SHIFT '>' add_expr
    | shift_expr L_SHIFT add_expr
    ;

add_expr
    : multi_expr
    | add_expr '+' multi_expr
    | add_expr '-' multi_expr
    ;

multi_expr
    : unary_expr
    | multi_expr '*' unary_expr
    | multi_expr '/' unary_expr
    | multi_expr '%' unary_expr
    ;

unary_expr
    : primary_expr
    | '+' unary_expr
    | '-' unary_expr
    | '~' unary_expr
    | NOT unary_expr
    ;

primary_expr
    : call_expr
    | dot_expr
    | index_expr
    | atom_expr
    ;

call_expr
    : primary_expr '(' ')'
    | primary_expr '(' expr_list ')'
    | primary_expr '(' expr_list ';' ')'
    ;

dot_expr
    : primary_expr '.' ID
    | primary_expr '.' INT_LITERAL
    ;

index_expr
    : primary_expr '[' expr ']'
    | primary_expr '[' slice_expr ']'
    ;

slice_expr
    : expr ':' expr
    | ':' expr
    | expr ':'
    | ':'
    ;

atom_expr
    : ID
    | '_'
    | INT_LITERAL
    {
        printf("integer:%ld(%d-%d)\n", $1, line(@1), column(@1));
    }
    | FLOAT_LITERAL
    {
        printf("float:%lf(%d-%d)\n", $1, line(@1), column(@1));
    }
    | STRING_LITERAL
    {
        printf("string:%s(%d-%d)\n", $1, line(@1), column(@1));
    }
    | TRUE
    | FALSE
    | NIL
    | SELF
    | SUPER
    | '(' expr ')'
    ;

expr_list
    : expr
    | expr_list ',' expr
    ;

%%

/* epilogue */
