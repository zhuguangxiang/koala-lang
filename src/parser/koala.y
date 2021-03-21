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
#include "color.h"

#define yyerror(loc, ps, scanner, msg) ((void)0)

#define line(loc) ((loc).first_line)
#define column(loc) ((loc).first_column)

#define yy_error(loc, fmt, ...) printf("%s:%d:%d: " RED_COLOR(%s) fmt "\n", \
    ps->filename, line(loc), column(loc), "error: ", ##__VA_ARGS__)
    // printf("%5d | %s\n", line(loc), ps->linebuf); 
    // printf("      | " RED_COLOR(%*s) "\n", column(loc), "^")

%}

%union {
    int64_t ival;
    double fval;
    int cval;
    char *sval;
}

%token IMPORT
%token CONST
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
%token FLOAT32
%token FLOAT64
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
%token OP_MATCH

%token DOTDOTDOT
%token DOTDOTLESS
%token FAT_ARROW

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

%token L_ANGLE_ARGS
%token L_SHIFT
%token R_ANGLE_SHIFT

%token <ival> INT_LITERAL
%token <fval> FLOAT_LITERAL
%token <sval> STRING_LITERAL
%token <sval> ID
%token INVALID

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
    : import_stmt
    {
        if (ps->interactive) {
            ps->more = 0;
        }
    }
    | expr ';'
    {
        if (ps->interactive) {
            ps->more = 0;
        }
    }
    | const_decl
    {
        if (ps->interactive) {
            ps->more = 0;
        }
    }
    | PUB const_decl
    {
        if (ps->interactive) {
            ps->more = 0;
        }
    }
    | var_decl
    {
        if (ps->interactive) {
            ps->more = 0;
        }
    }
    | PUB var_decl
    {
        if (ps->interactive) {
            ps->more = 0;
        }
    }
    | free_var_decl
    {
        if (ps->interactive) {
            ps->more = 0;
        }
    }
    | assignment
    {
        if (ps->interactive) {
            ps->more = 0;
        }
    }
    | if_stmt
    {
        if (ps->interactive) {
            ps->more = 0;
        }
    }
    | while_stmt
    {
        if (ps->interactive) {
            ps->more = 0;
        }
    }
    | for_stmt
    {
        if (ps->interactive) {
            ps->more = 0;
        }
    }
    | match_stmt
    {
        if (ps->interactive) {
            ps->more = 0;
        }
    }
    | ';'
    {

    }
    | func_decl
    {
        if (ps->interactive) {
            ps->more = 0;
        }
    }
    | PUB func_decl
    {
        if (ps->interactive) {
            ps->more = 0;
        }
    }
    | type_decl
    {
        if (ps->interactive) {
            ps->more = 0;
        }
    }
    | PUB type_decl
    {
        if (ps->interactive) {
            ps->more = 0;
        }
    }
    | INVALID
    {
        if (ps->interactive) {
            ps->more = 0;
        }
    }
    | error {
        if (ps->interactive) {
            ps->more = 0;
        }
        printf("syntax error\n");
        yyclearin;
        yyerrok;

    }
    ;

import_stmt
    : IMPORT STRING_LITERAL ';'
    {
    }
    | IMPORT ID STRING_LITERAL ';'
    {
    }
    | IMPORT '.' STRING_LITERAL ';'
    {
    }
    | IMPORT '{' id_as_list '}' STRING_LITERAL ';'
    {
    }
    ;

id_as_list
    : ID
    {
    }
    | ID AS ID
    {
    }
    | id_as_list ',' ID
    {
    }
    | id_as_list ',' ID AS ID
    {
    }
    ;

const_decl
    : CONST ID '=' expr ';'
    | CONST ID type '=' expr ';'
    ;

var_decl
    : VAR ID type ';'
    | VAR ID '=' expr ';'
    | VAR ID type '=' expr ';'
    | VAR error
    {
        yy_error(@2, "expected identifier");
        yyerrok;
    }
    | VAR ID error
    {
        yy_error(@3, "expected TYPE or '='");
        yyerrok;
    }
    | VAR ID '=' error
    {
        yy_error(@4, "expected expression");
        yyerrok;
    }
    | VAR ID type '=' error
    {
        yy_error(@4, "expected expression");
        yyerrok;
    }
    ;

free_var_decl
    : ID FREE_ASSIGN expr ';'
    {
    }
    | PUB ID FREE_ASSIGN expr ';'
    ;

assignment
    : primary_expr assignop expr ';'
    {
    }
    ;

assignop
    : '='
    {
    }
    | PLUS_ASSIGN
    {
    }
    | MINUS_ASSIGN
    {
    }
    | MULT_ASSIGN
    {
    }
    | DIV_ASSIGN
    {
    }
    | MOD_ASSIGN
    {
    }
    | AND_ASSIGN
    {
    }
    | OR_ASSIGN
    {
    }
    | XOR_ASSIGN
    {
    }
    | SHL_ASSIGN
    {

    }
    | SHR_ASSIGN {}
    ;

return_stmt
    : RETURN ';'
    {
    }
    | RETURN expr ';'
    {
    }
    ;

    jump_stmt
    : BREAK ';'
    {
    }
    | CONTINUE ';'
    {
    }
    ;

block
    : '{' local_list '}'
    {
    }
    | '{' expr '}'
    {
    }
    | '{' '}'
    {

    }
    ;

local_list
    : local
    {

    }
    | local_list local
    {
    }
    ;

local
    : expr ';'
    {
    }
    | var_decl
    {
    }
    | free_var_decl
    {
    }
    | assignment
    {
    }
    | if_stmt
    {
    }
    | while_stmt
    {
    }
    | for_stmt
    {
    }
    | match_stmt
    {
    }
    | return_stmt
    {
    }
    | jump_stmt
    {
    }
    | block
    {
    }
    | ';'
    {
    }
    ;

if_stmt
    : IF expr block elseif
    ;

elseif
    : %empty
    | ELSE block
    | ELSE if_stmt
    ;

while_stmt
    : WHILE expr block
    | WHILE block
    ;

for_stmt
    : FOR expr IN expr block
    ;

match_stmt
    : MATCH expr '{' match_clauses '}'
    ;

match_clauses
    : match_clause
    | match_clauses match_clause
    ;

match_clause
    : match_pattern_list FAT_ARROW match_block match_tail
    ;

match_pattern_list
    : match_pattern
    | match_pattern_list ',' match_pattern
    ; 

match_pattern
    : expr
    | IN range_expr
    | IS type
    ;

match_tail
    : ';'
    | ','
    ;

match_block
    : block
    | expr
    ;

func_decl
    : FUNC name '(' param_list ')' type block
    {
    }
    | FUNC name '(' param_list ')' block
    {
    }
    | FUNC name '(' ')' type block
    {
    }
    | FUNC name '(' ')' block
    {
    }
    ;

proto_decl
    : FUNC name '(' param_list ')' type ';'
    {
    }
    | FUNC name '(' param_list ')' ';'
    {
    }
    | FUNC name '(' ')' type ';'
    {
    }
    | FUNC name '(' ')' ';'
    {
    }
    ;

param_list
    : id_type_list
    {
    }
    | id_varg
    {
    }
    | id_type_list ',' id_varg
    {
    }
    ;

id_type_list
    : ID type
    {
    }
    | id_type_list ',' ID type
    {
    }
    ;

id_varg
    : ID DOTDOTDOT
    {
    }
    | ID DOTDOTDOT no_array_type
    {
    }
    ;

type_decl
    : class_decl
    | CONST class_decl
    | trait_decl
    | enum_decl
    | CONST enum_decl
    ;

class_decl
    : CLASS name extends '{' class_members '}'
    | CLASS name extends '{' '}'
    | CLASS name extends ';'
    ;

trait_decl
    : TRAIT name extends '{' trait_members '}'
    | TRAIT name extends '{' '}'
    | TRAIT name extends ';'
    ;

enum_decl
    : ENUM name extends '{' enum_members '}'
    | ENUM name extends ';'
    ;

name
    : ID
    {
    }
    | ID '<' type_param_decl_list '>'
    {
    }
    ;

type_param_decl_list
    : type_param_decl
    | type_param_decl_list ',' type_param_decl
    ;

type_param_decl
    : ID
    | ID ':' type_param_bound_list
    ;

type_param_bound_list
    : klass_type
    {
        // trait, not class
    }
    | type_param_bound_list '&' klass_type
    ;

extends
    : %empty
    | ':' klass_list
    ;

klass_list
    : klass_type
    | klass_list ',' klass_type
    ;

class_members
    : class_member
    | class_members class_member
    ;

class_member
    : ID type ';'
    | ID type '=' expr ';'
    | func_decl
    | ';'
    ;

trait_members
    : trait_member
    | trait_members trait_member
    ;

trait_member
    : func_decl
    | proto_decl
    | ';'
    ;

enum_members
    : enum_labels ','
    | enum_labels ';'
    | enum_labels ',' enum_methods
    | enum_labels ';' enum_methods
    ;

enum_labels
    : enum_label
    {
    }
    | enum_labels ',' enum_label
    {
    }
    ;

enum_label
    : ID
    | ID '(' type_list ')'
    | ID '=' INT_LITERAL
    ;

enum_methods
    : func_decl ';'
    {
    }
    | enum_methods func_decl ';'
    {
    }
    ;

expr
    : cond_expr
    | or_expr
    | range_expr
    | is_expr
    | as_expr
    | match_expr
    ;

cond_expr
    : cond_c_expr '?' expr ':' expr
    ;

cond_c_expr
    : equality_expr
    | is_expr
    | match_expr
    ;

range_expr
    : or_expr DOTDOTDOT or_expr
    | or_expr DOTDOTLESS or_expr
    ;

is_expr
    : primary_expr IS type
    ;

as_expr
    : primary_expr AS type
    ;

match_expr
    : primary_expr OP_MATCH or_expr
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
    | multi_expr '*' error
    {
        //ps->errors++;
        ps->more = 0;
        yyerrok;
        printf("expected expr\n");
    }
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
    | angle_expr
    | atom_expr
    | array_object_expr
    | map_object_expr
    | tuple_object_expr
    | anony_object_expr
    | new_map_expr
    | new_base_expr
    ;

call_expr
    : primary_expr '(' ')'
    | primary_expr '(' expr_list ')'
    | primary_expr '(' expr_list ';' ')'
    | primary_expr '(' error
    {
        printf("error, expected ) or expr list\n");
    }
    ;

dot_expr
    : primary_expr '.' ID
    {
        printf("object access\n");
    }
    | primary_expr '.' INT_LITERAL
    {
        printf("tuple access\n");
    }
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

angle_expr
    : primary_expr L_ANGLE_ARGS type_list r_angle
    {

    }
    ;

atom_expr
    : ID
    {
        printf("id:%s\n",$1);
    }
    | '_'
    | INT_LITERAL
    {
        printf("integer:%ld(%d-%d)\n", $1, line(@1), column(@1));
        //printf("has doc:%d\n", SBUF_LEN(ps->doc_buf));
    }
    | FLOAT_LITERAL
    {
        printf("float:%lf(%d-%d)\n", $1, line(@1), column(@1));
        //printf("has doc:%d\n", SBUF_LEN(ps->doc_buf));
    }
    | STRING_LITERAL
    {
        printf("string:%s(%d-%d)\n", $1, line(@1), column(@1));
        //printf("has doc:%d\n", SBUF_LEN(ps->doc_buf));
    }
    | TRUE
    | FALSE
    | NIL
    | SELF
    | SUPER
    | '(' expr ')'
    ;

array_object_expr
    : '[' expr_list ']'
    {
    }
    | '[' expr_list ',' ']'
    {
    }
    | '[' expr_list ';' ']'
    {
    }
    ;

map_object_expr
    : '{' mapentry_list '}'
    {
    }
    | '{' mapentry_list ',' '}'
    {
    }
    | '{' mapentry_list ';' '}'
    {
    }
    ;

mapentry_list
    : mapentry
    {
    }
    | mapentry_list ',' mapentry
    {
    }
    ;

mapentry
    : expr ':' expr
    {
    }
    ;

tuple_object_expr
    : '(' expr_list ',' ')'
    {
    }
    | '(' expr_list ',' expr ')'
    {
    }
    | '(' expr_list ',' expr ';' ')'
    {
    }
    ;

anony_object_expr
    : FUNC '(' param_list ')' type block
    {
    }
    | FUNC '(' param_list ')' block
    {
    }
    | FUNC '(' ')' type block
    {
    }
    | FUNC '(' ')' block
    {
    }
    ;

new_map_expr
    : '[' expr ':' expr ']'
    {
        printf("new map\n");
    }
    ;

new_base_expr
    : INT8
    | INT16
    | INT32
    | INT64
    | FLOAT32
    | FLOAT64
    | BOOL
    | CHAR
    | STRING
    | ANY
    ;

expr_list
    : expr
    | expr_list ',' expr
    ;

type
    : no_array_type
    | '[' type ']'
    | '[' type ':' type ']'
    | '(' type_list ')'
    ;

no_array_type
    : INT8
    | INT16
    | INT32
    | INT64
    | FLOAT32
    | FLOAT64
    | BOOL
    | CHAR
    | STRING
    | ANY
    | klass_type
    | func_type
    ;

klass_type
    : ID
    | ID '.' ID
    | ID L_ANGLE_ARGS type_list r_angle
    {
        printf("type param\n");
    }
    | ID '.' ID L_ANGLE_ARGS type_list r_angle
    ;

r_angle
    : '>'
    | R_ANGLE_SHIFT
    ;

func_type
    : FUNC '(' type_varg_list ')' type
    {

    }
    | FUNC '(' type_varg_list ')'
    {
    }
    | FUNC '(' ')' type
    {
    }
    | FUNC '(' ')'
    {
    }
    ;

type_varg_list
    : type_list
    {
    }
    | type_list ',' DOTDOTDOT
    {
    }
    | type_list ',' DOTDOTDOT no_array_type
    {
    }
    ;

type_list
    : type
    | type_list ',' type
    ;

%%

/* epilogue */
