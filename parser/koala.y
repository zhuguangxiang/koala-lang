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

/* prologue */

%{

#include "parser.h"
#include "color.h"
#include "koala_yacc.h"
#include "koala_lex.h"

#define yyerror(loc, ps, scanner, msg) ((void)0)

#define line(loc) ((loc).first_line)
#define col_1st(loc) ((loc).first_column)
#define col_last(loc) ((loc).last_column)
#define loc(l) (l).first_line, (l).first_column

#define IDENT(_s, _l) (Ident){_s, loc(_l)}

void show_error_detail(ParserStateRef ps, int row, int col);

#define yy_error(loc, fmt, ...) printf("%s:%d:%d: " RED_COLOR(%s) fmt "\n", \
    ps->filename, line(loc), col_1st(loc), "error: ", ##__VA_ARGS__);

int cmd_add_var(ParserStateRef ps, Ident id);
void cmd_eval_stmt(ParserStateRef ps, StmtRef stmt);

%}

%union {
    int64_t ival;
    double fval;
    int cval;
    char *sval;
    AssignOpKind assignop;
    ExprRef expr;
    StmtRef stmt;
    TypeRef type;
    VectorRef list;
    VectorRef typelist;
    VectorRef stmtlist;
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
%token INVALID

%token <ival> INT_LITERAL
%token <fval> FLOAT_LITERAL
%token <cval> CHAR_LITERAL
%token <sval> STRING_LITERAL
%token <sval> ID

%type <stmt> const_decl
%type <stmt> var_decl
%type <stmt> free_var_decl
%type <assignop> assignop
%type <stmt> assignment
%type <stmt> return_stmt
%type <stmt> jump_stmt
%type <stmt> block
%type <stmt> local
%type <stmtlist> local_list

%type <expr> expr
%type <expr> cond_expr
%type <expr> cond_c_expr
%type <expr> range_expr
%type <expr> is_expr
%type <expr> as_expr
%type <expr> or_expr
%type <expr> and_expr
%type <expr> bit_or_expr
%type <expr> bit_xor_expr
%type <expr> bit_and_expr
%type <expr> equality_expr
%type <expr> relation_expr
%type <expr> shift_expr
%type <expr> add_expr
%type <expr> multi_expr
%type <expr> unary_expr
%type <expr> primary_expr
%type <expr> atom_expr

%type <type> type
%type <type> atom_type
%type <type> klass_type
%type <type> func_type
%type <typelist> type_varg_list
%type <typelist> type_list

%locations
%parse-param {ParserStateRef ps}
%parse-param {void *scanner}
%define api.pure full
%lex-param {void *scanner}
%code provides {
  int yylex(YYSTYPE *yylval_param, YYLTYPE *yylloc, void *yyscanner);
}

%start program

/* grammar rules */

%%

program
    : %empty
    | program stmt
    ;

stmt
    : import_stmt
    {
        if (ps->cmd) {
            ps->more = 0;
        }
    }
    | expr ';'
    {
        StmtRef s = stmt_from_expr($1);
        stmt_set_loc(s, loc(@1));
        if (ps->cmd) {
            ps->more = 0;
            cmd_eval_stmt(ps, s);
            free_stmt(s);
        }
    }
    | const_decl
    {
        if (ps->cmd) {
            ps->more = 0;
            free_stmt($1);
        }
    }
    | PUB const_decl
    {
        if (ps->cmd) {
            ps->more = 0;
            free_stmt($2);
        }
    }
    | var_decl
    {
        if (ps->cmd) {
            ps->more = 0;
            cmd_add_var(ps, ((VarDeclStmtRef)$1)->name);
            cmd_eval_stmt(ps, $1);
            free_stmt($1);
        }
    }
    | PUB var_decl
    {
        if (ps->cmd) {
            ps->more = 0;
            free_stmt($2);
        }
    }
    | free_var_decl
    {
        if (ps->cmd) {
            ps->more = 0;
            free_stmt($1);
        }
    }
    | PUB free_var_decl
    {
        if (ps->cmd) {
            ps->more = 0;
            free_stmt($2);
        }
    }
    | assignment
    {
        if (ps->cmd) {
            ps->more = 0;
        }
    }
    | if_stmt
    {
        if (ps->cmd) {
            ps->more = 0;
        }
    }
    | while_stmt
    {
        if (ps->cmd) {
            ps->more = 0;
        }
    }
    | for_stmt
    {
        if (ps->cmd) {
            ps->more = 0;
        }
    }
    | match_stmt
    {
        if (ps->cmd) {
            ps->more = 0;
        }
    }
    | ';'
    {
        if (ps->cmd) {
            ps->more = 0;
        }
    }
    | func_decl
    {
        if (ps->cmd) {
            ps->more = 0;
        }
    }
    | PUB func_decl
    {
        if (ps->cmd) {
            ps->more = 0;
        }
    }
    | type_decl
    {
        if (ps->cmd) {
            ps->more = 0;
        }
    }
    | PUB type_decl
    {
        if (ps->cmd) {
            ps->more = 0;
        }
    }
    | INVALID
    {
        if (ps->cmd) {
            ps->more = 0;
        }
        printf("invalid token\n");
    }
    | expr error
    {
        printf("expr: expected ;\n");
        free_expr($1);
        yyerrok;
    }
    | error {
        if (ps->cmd) {
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
    | IMPORT error
    {
        yyclearin;
        yyerrok;
    }
    | IMPORT ID error
    {

    }
    | IMPORT '.' error
    {

    }
    | IMPORT STRING_LITERAL error
    | IMPORT ID STRING_LITERAL error
    | IMPORT '.' STRING_LITERAL error
    | IMPORT '{' error '}' STRING_LITERAL ';'
    {
        printf("import error\n");
    }
    | IMPORT '{' id_as_list error
    {
    }
    | IMPORT '{' id_as_list '}' error
    {
    }
    | IMPORT '{' id_as_list '}' STRING_LITERAL error
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
    {
        Ident name = IDENT($2, @2);
        $$ = stmt_from_constdecl(&name, NULL, $4);
        stmt_set_loc($$, loc(@1));
    }
    | CONST ID type '=' expr ';'
    {
        Ident name = IDENT($2, @2);
        $$ = stmt_from_constdecl(&name, $3, $5);
        stmt_set_loc($$, loc(@1));
    }
    ;

var_decl
    : VAR ID type ';'
    {
        Ident name = IDENT($2, @2);
        $$ = stmt_from_vardecl(&name, $3, NULL);
        stmt_set_loc($$, loc(@1));
    }
    | VAR ID '=' expr ';'
    {
        Ident name = IDENT($2, @2);
        $$ = stmt_from_vardecl(&name, NULL, $4);
        stmt_set_loc($$, loc(@1));
    }
    | VAR ID type '=' expr ';'
    {
        Ident name = IDENT($2, @2);
        $$ = stmt_from_vardecl(&name, $3, $5);
        stmt_set_loc($$, loc(@1));
    }
    | VAR error
    {
        printf("%d-%d\n", loc(@2));
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
        yy_error(@4, "expected expression2, %d\n", yychar);
        if (yychar != FOR) {
            yyclearin;
        }
        yyerrok;
    }
    | VAR ID '=' expr error
    {
        yy_error(@5, "expected ';' or '\\n', %d\n", yychar);
        if (yychar != FOR) {
            yyclearin;
        }
        yyclearin;
    }
    | VAR ID type '=' error
    {
        yy_error(@4, "expected expression3");
        yyerrok;
    }
    ;

free_var_decl
    : ID FREE_ASSIGN expr ';'
    {
        Ident name = IDENT($1, @1);
        $$ = stmt_from_vardecl(&name, NULL, $3);
        stmt_set_loc($$, loc(@1));
    }
    ;

assignment
    : primary_expr assignop expr ';'
    {
        $$ = stmt_from_assign($2, $1, $3);
        stmt_set_loc($$, loc(@1));
    }
    ;

assignop
    : '='
    {
        $$ = OP_ASSIGN;
    }
    | PLUS_ASSIGN
    {
        $$ = OP_PLUS_ASSIGN;
    }
    | MINUS_ASSIGN
    {
        $$ = OP_MINUS_ASSIGN;
    }
    | MULT_ASSIGN
    {
        $$ = OP_MULT_ASSIGN;
    }
    | DIV_ASSIGN
    {
        $$ = OP_DIV_ASSIGN;
    }
    | MOD_ASSIGN
    {
        $$ = OP_MOD_ASSIGN;
    }
    | AND_ASSIGN
    {
        $$ = OP_AND_ASSIGN;
    }
    | OR_ASSIGN
    {
        $$ = OP_OR_ASSIGN;
    }
    | XOR_ASSIGN
    {
        $$ = OP_XOR_ASSIGN;
    }
    | SHL_ASSIGN
    {
        $$ = OP_ASSIGN;
    }
    | SHR_ASSIGN
    {
        $$ = OP_ASSIGN;
    }
    ;

return_stmt
    : RETURN ';'
    {
        $$ = stmt_from_ret(NULL);
        stmt_set_loc($$, loc(@1));
    }
    | RETURN expr ';'
    {
        $$ = stmt_from_ret($2);
        stmt_set_loc($$, loc(@1));
    }
    ;

jump_stmt
    : BREAK ';'
    {
        $$ = stmt_from_break();
        stmt_set_loc($$, loc(@1));
    }
    | CONTINUE ';'
    {
        $$ = stmt_from_continue();
        stmt_set_loc($$, loc(@1));
    }
    ;

block
    : '{' local_list '}'
    {
        $$ = stmt_from_block($2);
        stmt_set_loc($$, loc(@1));
    }
    | '{' expr '}'
    {
        VectorRef vec = vector_create(sizeof(void *));
        StmtRef s = stmt_from_expr($2);
        stmt_set_loc(s, loc(@2));
        vector_push_back(vec, &s);
        $$ = stmt_from_block(vec);
        stmt_set_loc($$, loc(@1));
    }
    | '{' '}'
    {
        $$ = stmt_from_block(NULL);
        stmt_set_loc($$, loc(@1));
    }
    ;

local_list
    : local
    {
        $$ = vector_create(sizeof(void *));
        vector_push_back($$, $1);
    }
    | local_list local
    {
        $$ = $1;
        vector_push_back($$, $2);
    }
    ;

local
    : expr ';'
    {
        $$ = stmt_from_expr($1);
        stmt_set_loc($$, loc(@1));
    }
    | var_decl
    {
        $$ = $1;
    }
    | free_var_decl
    {
        $$ = $1;
    }
    | assignment
    {
        $$ = $1;
    }
    | if_stmt
    {
        $$ = NULL;
    }
    | while_stmt
    {
        $$ = NULL;
    }
    | for_stmt
    {
        $$ = NULL;
    }
    | match_stmt
    {
        $$ = NULL;
    }
    | return_stmt
    {
        $$ = $1;
    }
    | jump_stmt
    {
        $$ = $1;
    }
    | block
    {
        $$ = $1;
    }
    | ';'
    {
        $$ = NULL;
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
    | MATCH error
    {
        yy_error(@2, "match error\n");
        yyerrok;
    }
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
    : %empty
    | block
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
    | ID DOTDOTDOT atom_type
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
    {
        $$ = $1;
    }
    | as_expr
    {
        $$ = $1;
    }
    | range_expr
    {
        $$ = $1;
    }
    ;

cond_expr
    : cond_c_expr
    {
        $$ = $1;
    }
    | cond_c_expr '?' expr ':' expr
    {
        $$ = NULL;
    }
    ;

cond_c_expr
    : or_expr
    {
        $$ = $1;
    }
    | is_expr
    {
        $$ = $1;
    }
    ;

range_expr
    : or_expr DOTDOTDOT or_expr
    {
        $$ = NULL;
    }
    | or_expr DOTDOTLESS or_expr
    {
        $$ = NULL;
    }
    ;

is_expr
    : primary_expr IS type
    {
        $$ = NULL;
    }
    ;

as_expr
    : primary_expr AS type
    {
        $$ = NULL;
    }
    ;

or_expr
    : and_expr
    {
        $$ = $1;
    }
    | or_expr OR and_expr
    {
        $$ = expr_from_binary(BINARY_OR, $1, $3);
        expr_set_loc($$, loc(@1));
    }
    ;

and_expr
    : bit_or_expr
    {
        $$ = $1;
    }
    | and_expr AND bit_or_expr
    {
        $$ = expr_from_binary(BINARY_AND, $1, $3);
        expr_set_loc($$, loc(@1));
    }
    ;

bit_or_expr
    : bit_xor_expr
    {
        $$ = $1;
    }
    | bit_or_expr '|' bit_xor_expr
    {
        $$ = expr_from_binary(BINARY_BIT_OR, $1, $3);
        expr_set_loc($$, loc(@1));
    }
    ;

bit_xor_expr
    : bit_and_expr
    {
        $$ = $1;
    }
    | bit_xor_expr '^' bit_and_expr
    {
        $$ = expr_from_binary(BINARY_BIT_XOR, $1, $3);
        expr_set_loc($$, loc(@1));
    }
    ;

bit_and_expr
    : equality_expr
    {
        $$ = $1;
    }
    | bit_and_expr '&' equality_expr
    {
        $$ = expr_from_binary(BINARY_BIT_AND, $1, $3);
        expr_set_loc($$, loc(@1));
    }
    ;

equality_expr
    : relation_expr
    {
        $$ = $1;
    }
    | equality_expr EQ relation_expr
    {
        $$ = expr_from_binary(BINARY_EQ, $1, $3);
        expr_set_loc($$, loc(@1));
    }
    | equality_expr NE relation_expr
    {
        $$ = expr_from_binary(BINARY_NE, $1, $3);
        expr_set_loc($$, loc(@1));
    }
    ;

relation_expr
    : shift_expr
    {
        $$ = $1;
    }
    | relation_expr '<' shift_expr
    {
        $$ = expr_from_binary(BINARY_LT, $1, $3);
        expr_set_loc($$, loc(@1));
    }
    | relation_expr '>' shift_expr
    {
        $$ = expr_from_binary(BINARY_GT, $1, $3);
        expr_set_loc($$, loc(@1));
    }
    | relation_expr LE shift_expr
    {
        $$ = expr_from_binary(BINARY_LE, $1, $3);
        expr_set_loc($$, loc(@1));
    }
    | relation_expr GE shift_expr
    {
        $$ = expr_from_binary(BINARY_GE, $1, $3);
        expr_set_loc($$, loc(@1));
    }
    ;

shift_expr
    : add_expr
    {
        $$ = $1;
    }
    | shift_expr R_ANGLE_SHIFT '>' add_expr
    {
        $$ = expr_from_binary(BINARY_RSHIFT, $1, $4);
        expr_set_loc($$, loc(@1));
    }
    | shift_expr L_SHIFT add_expr
    {
        $$ = expr_from_binary(BINARY_LSHIFT, $1, $3);
        expr_set_loc($$, loc(@1));
    }
    ;

add_expr
    : multi_expr
    {
        $$ = $1;
    }
    | add_expr '+' multi_expr
    {
        $$ = expr_from_binary(BINARY_ADD, $1, $3);
        expr_set_loc($$, loc(@1));
    }
    | add_expr '-' multi_expr
    {
        $$ = expr_from_binary(BINARY_SUB, $1, $3);
        expr_set_loc($$, loc(@1));
    }
    ;

multi_expr
    : unary_expr
    {
        $$ = $1;
    }
    | multi_expr '*' unary_expr
    {
        $$ = expr_from_binary(BINARY_MULT, $1, $3);
        expr_set_loc($$, loc(@1));
    }
    | multi_expr '/' unary_expr
    {
        $$ = expr_from_binary(BINARY_DIV, $1, $3);
        expr_set_loc($$, loc(@1));
    }
    | multi_expr '%' unary_expr
    {
        $$ = expr_from_binary(BINARY_MOD, $1, $3);
        expr_set_loc($$, loc(@1));
    }
    ;

unary_expr
    : primary_expr
    {
        $$ = $1;
    }
    | '+' unary_expr
    {
        $$ = $2;
    }
    | '-' unary_expr
    {
        $$ = expr_from_unary(UNARY_NEG, $2);
        expr_set_loc($$, loc(@1));
    }
    | '~' unary_expr
    {
        $$ = expr_from_unary(UNARY_BIT_NOT, $2);
        expr_set_loc($$, loc(@1));
    }
    | NOT unary_expr
    {
        $$ = expr_from_unary(UNARY_NOT, $2);
        expr_set_loc($$, loc(@1));
    }
    ;

primary_expr
    : call_expr
    {

    }
    | dot_expr
    {

    }
    | index_expr
    {

    }
    | angle_expr
    {

    }
    | atom_expr
    {
        $$ = $1;
    }
    | array_object_expr
    {

    }
    | map_object_expr
    {

    }
    | tuple_object_expr
    {

    }
    | anony_object_expr
    {

    }
    | new_map_expr
    {

    }
    | new_base_expr
    {

    }
    ;

call_expr
    : primary_expr '(' ')'
    | primary_expr '(' expr_list ')'
    | primary_expr '(' expr_list ';' ')'
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
        $$ = expr_from_ident($1);
        expr_set_loc($$, loc(@1));
    }
    | '_'
    {
        $$ = expr_from_under();
        expr_set_loc($$, loc(@1));
    }
    | INT_LITERAL
    {
        $$ = expr_from_int64($1);
        expr_set_loc($$, loc(@1));
    }
    | FLOAT_LITERAL
    {
        $$ = expr_from_float64($1);
        expr_set_loc($$, loc(@1));
    }
    | CHAR_LITERAL
    {
        $$ = expr_from_char($1);
        expr_set_loc($$, loc(@1));
    }
    | STRING_LITERAL
    {
        $$ = expr_from_str($1);
        expr_set_loc($$, loc(@1));
    }
    | TRUE
    {
        $$ = expr_from_bool(1);
        expr_set_loc($$, loc(@1));
    }
    | FALSE
    {
        $$ = expr_from_bool(0);
        expr_set_loc($$, loc(@1));
    }
    | NIL
    {
        $$ = expr_from_nil();
        expr_set_loc($$, loc(@1));
    }
    | SELF
    {
        $$ = expr_from_self();
        expr_set_loc($$, loc(@1));
    }
    | SUPER
    {
        $$ = expr_from_super();
        expr_set_loc($$, loc(@1));
    }
    | '(' expr ')'
    {
        $$ = $2;
    }
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
    : atom_type
    {
        $$ = $1;
    }
    | '[' type ']'
    {
        $$ = NULL;
    }
    | '[' type ':' type ']'
    {
        $$ = NULL;
    }
    | '(' type_list ')'
    {
        $$ = NULL;
    }
    ;

atom_type
    : INT8
    {
        $$ = ast_type_int8();
        type_set_loc($$, loc(@1));
    }
    | INT16
    {
        $$ = ast_type_int8();
        type_set_loc($$, loc(@1));
    }
    | INT32
    {
        $$ = ast_type_int8();
        type_set_loc($$, loc(@1));
    }
    | INT64
    {
        $$ = ast_type_int8();
        type_set_loc($$, loc(@1));
    }
    | FLOAT32
    {
        $$ = ast_type_int8();
        type_set_loc($$, loc(@1));
    }
    | FLOAT64
    {
        $$ = ast_type_int8();
        type_set_loc($$, loc(@1));
    }
    | BOOL
    {
        $$ = ast_type_int8();
        type_set_loc($$, loc(@1));
    }
    | CHAR
    {
        $$ = ast_type_int8();
        type_set_loc($$, loc(@1));
    }
    | STRING
    {
        $$ = ast_type_int8();
        type_set_loc($$, loc(@1));
    }
    | ANY
    {
        $$ = ast_type_int8();
        type_set_loc($$, loc(@1));
    }
    | klass_type
    {
        $$ = $1;
    }
    | func_type
    {
        $$ = $1;
    }
    ;

klass_type
    : ID
    {
        Ident name = IDENT($1, @1);
        $$ = ast_type_klass(NULL, &name);
        type_set_loc($$, loc(@1));
    }
    | ID '.' ID
    {
        Ident pkg = IDENT($1, @1);
        Ident name = IDENT($3, @3);
        $$ = ast_type_klass(&pkg, &name);
        type_set_loc($$, loc(@1));
    }
    | ID L_ANGLE_ARGS type_list r_angle
    {
        Ident name = IDENT($1, @1);
        $$ = ast_type_klass(NULL, &name);
        type_set_typeparams($$, $3);
        type_set_loc($$, loc(@1));
    }
    | ID '.' ID L_ANGLE_ARGS type_list r_angle
    {
        Ident pkg = IDENT($1, @1);
        Ident name = IDENT($3, @3);
        $$ = ast_type_klass(&pkg, &name);
        type_set_typeparams($$, $5);
        type_set_loc($$, loc(@1));
    }
    ;

r_angle
    : '>'
    | R_ANGLE_SHIFT
    ;

func_type
    : FUNC '(' type_varg_list ')' type
    {
        $$ = ast_type_proto($5, $3);
        type_set_loc($$, loc(@1));
    }
    | FUNC '(' type_varg_list ')'
    {
        $$ = ast_type_proto(NULL, $3);
        type_set_loc($$, loc(@1));
    }
    | FUNC '(' ')' type
    {
        $$ = ast_type_proto($4, NULL);
        type_set_loc($$, loc(@1));
    }
    | FUNC '(' ')'
    {
        $$ = ast_type_proto(NULL, NULL);
        type_set_loc($$, loc(@1));
    }
    ;

type_varg_list
    : type_list
    {
        $$ = $1;
    }
    | type_list ',' DOTDOTDOT
    {
        $$ = $1;
        TypeRef _t = ast_type_valist(NULL);
        type_set_loc(_t, loc(@3));
        vector_push_back($$, &_t);
    }
    | type_list ',' DOTDOTDOT atom_type
    {
        $$ = $1;
        TypeRef _t = ast_type_valist($4);
        type_set_loc(_t, loc(@3));
        vector_push_back($$, &_t);
    }
    ;

type_list
    : type
    {
        $$ = vector_create(sizeof(void *));
        vector_push_back($$, &$1);
    }
    | type_list ',' type
    {
        $$ = $1;
        vector_push_back($$, &$3);
    }
    ;

%%

/* epilogue */
