/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/* prologue */

%{

#include "buffer.h"
#include "parser.h"
#include "koala_yacc.h"

#define yyerror(loc, ps, scanner, msg) ((void)0)

#define line(loc) ((loc).first_line)
#define last_line(loc) ((loc).last_line)
#define col(loc) ((loc).first_column)
#define last_col(loc) ((loc).last_column)

#define loc(l) (Loc){line(l), col(l), last_line(l), last_col(l)}
#define lloc(l, r) (Loc){line(l), col(l), last_line(r), last_col(r)}

static int need_clear(int token)
{
    int tokens[] = {
        ';', IMPORT, LET, VAR, FUNC,
        IF, WHILE, FOR, MATCH, CLASS,
        TRAIT, RETURN, FINAL
    };

    for (int i = 0; i < COUNT_OF(tokens); i++) {
        if (tokens[i] == token) return 0;
    }
    return 1;
}

#define yy_clear_ok if (need_clear(ps->token)) yyclearin; yyerrok

%}

%union {
    char *sval;
    __int128 ival;
    __float128 fval;
    Stmt *stmt;
    Expr *expr;
    Type *type;
    Vector *vec;
}

%token IMPORT
%token LET
%token VAR
%token FUNC
%token CLASS
%token TRAIT
%token IF
%token GUARD
%token ELSE
%token WHILE
%token FOR
%token MATCH
%token CASE
%token BREAK
%token CONTINUE
%token RETURN
%token IN
%token AS
%token IS
%token PUBLIC
%token FINAL

%token SELF
%token SUPER
%token TRUE
%token FALSE
%token NONE

%token INT8
%token INT16
%token INT32
%token INT64
%token FLOAT32
%token FLOAT64
%token BOOL
%token CHAR
%token STRING
%token OBJECT
%token ARRAY
%token MAP
%token TUPLE
%token SET
%token TYPE
%token RANGE
%token BYTES

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
%token USHR_ASSIGN

%token DOTDOTDOT
%token DOTDOTEQ

%token L_SHIFT
%token R_SHIFT
%token R_USHIFT
%token MOD_DOC
%token DOC

%type<stmt> import_stmt
%type<stmt> top_stmt
%type<stmt> let_decl
%type<stmt> var_decl
%type<stmt> free_var_decl
%type<stmt> assignment
%type<stmt> return_stmt
%type<stmt> jump_stmt
%type<stmt> if_stmt
%type<stmt> elseif_stmt
%type<stmt> while_stmt
%type<stmt> for_stmt
%type<stmt> local

%type<expr> expr
%type<expr> or_expr
%type<expr> is_expr
%type<expr> as_expr
%type<expr> in_expr
%type<expr> range_expr
%type<expr> and_expr
%type<expr> bit_or_expr
%type<expr> bit_xor_expr
%type<expr> bit_and_expr
%type<expr> equality_expr
%type<expr> relation_expr
%type<expr> shift_expr
%type<expr> add_expr
%type<expr> multi_expr
%type<expr> unary_expr
%type<expr> primary_expr
%type<expr> call_expr
%type<expr> dot_expr
%type<expr> index_expr
%type<expr> slice_expr
%type<expr> atom_expr
%type<expr> array_expr
%type<expr> map_expr
%type<expr> map
%type<expr> atom
%type<expr> tuple_expr

%type<type> optional_type
%type<type> type_enum
%type<type> type
%type<type> array_type
%type<type> map_type
%type<type> tuple_type
%type<type> klass_type
%type<type> atom_type

%type<vec> import_stmts
%type<vec> top_stmts
%type<vec> optional_type_list
%type<vec> block
%type<vec> local_list
%type<vec> map_list
%type<vec> expr_list

%token<sval> ID
%token<ival> INT_LITERAL
%token<fval> FLOAT_LITERAL
%token STRING_LITERAL

%type<ival> assign_operator

%locations
%parse-param {ParserState *ps}
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
    : top_stmts
    {
        yyparse_module(ps, NULL, $1);
    }
    | import_stmts top_stmts
    {
        yyparse_module(ps, $1, $2);
    }
    | MOD_DOC top_stmts
    {
        yyparse_module(ps, NULL, $2);
    }
    | MOD_DOC import_stmts top_stmts
    {
        yyparse_module(ps, $2, $3);
    }
    ;

import_stmts
    : import_stmt
    {
        $$ = vector_create_ptr();
        if ($1) vector_push_back($$, &$1);
    }
    | import_stmts import_stmt
    {
        $$ = $1;
        if ($2) vector_push_back($$, &$2);
    }
    ;

import_stmt
    : IMPORT STRING_LITERAL semi
    {
        // $$ = stmt_from_import($2, NULL);
        // stmt_set_loc($$, lloc(@1, @3));
    }
    | IMPORT STRING_LITERAL AS ID semi
    {
        // $$ = stmt_from_import($2, $4);
        // stmt_set_loc($$, lloc(@1, @5));
    }
    ;

top_stmts
    : top_stmt
    {
        $$ = vector_create_ptr();
        if ($1) vector_push_back($$, &$1);
    }
    | top_stmts top_stmt
    {
        $$ = $1;
        if ($2) vector_push_back($$, &$2);
    }
    ;

top_stmt
    : let_decl semi
    {
        $$ = $1;
    }
    | prefix let_decl semi
    {
        $$ = $2;
    }
    | var_decl semi
    {
        $$ = NULL;
    }
    | prefix var_decl semi
    {
        $$ = NULL;
    }
    | free_var_decl semi
    {
        $$ = NULL;
    }
    | assignment semi
    {
        $$ = NULL;
    }
    | expr semi
    {
        $$ = NULL;
    }
    | if_stmt
    {
        $$ = $1;
    }
    | while_stmt
    {
        $$ = NULL;
    }
    | for_stmt
    {
        $$ = NULL;
    }
    | semi
    {
        $$ = NULL;
    }
    | error {
        printf("error\n");
    }
    ;


semi
    : ';'
    ;

prefix
    : PUBLIC
    | docs
    | docs PUBLIC
    ;

docs
    : DOC
    | docs DOC
    ;

optional_type
    : type
    {
        $$ = $1;
    }
    | type '?'
    {
        $$ = $1;
        type_set_optional($$);
        type_set_loc($$, lloc(@1, @2));
    }
    | type_enum
    {

    }
    | '(' type_enum ')'
    {

    }
    | '(' type_enum ')' '?'
    {

    }
    ;

type_enum
    : type '|' type
    | type_enum '|' type
    ;

type
    : array_type
    {
        // $$ = $1;
    }
    | map_type
    {
        // $$ = $1;
    }
    | tuple_type
    {
        // $$ = $1;
    }
    | set_type
    {

    }
    | klass_type
    {
        // $$ = $1;
    }
    | atom_type
    {
        $$ = $1;
    }
    ;

array_type
    : ARRAY '[' optional_type ']'
    {
        // $$ = NULL; // expr_type_array(lloc(@1, @4), $3);
    }
    | ARRAY
    {

    }
    ;

map_type
    : MAP '[' type ',' optional_type ']'
    {
        // $$ = NULL; //expr_type_map(lloc(@1, @6), $3, $5);
    }
    | MAP
    {

    }
    ;

tuple_type
    : TUPLE '[' optional_type_list ']'
    {
        // vector_push_back($2, &$4);
        // $$ = expr_type_tuple(lloc(@1, @5), $2);
    }
    | TUPLE
    {

    }
    ;

set_type
    : SET '[' type ']'
    | SET
    ;

klass_type
    : ID
    {
        // Ident pkg = {NULL, {0, 0, 0, 0}, NULL};
        // Ident name = {$1, loc(@1), NULL};
        // $$ = expr_type_klass(loc(@1), pkg, name, NULL);
    }
    | ID '.' ID
    {
        // Ident pkg = {$1, loc(@1), NULL};
        // Ident name = {$3, loc(@3), NULL};
        // $$ = expr_type_klass(lloc(@1, @3), pkg, name, NULL);
    }
    | ID '[' optional_type_list ']'
    {
        // Ident pkg = {NULL, {0, 0, 0, 0}, NULL};
        // Ident name = {$1, loc(@1), NULL};
        // $$ = expr_type_klass(lloc(@1, @4), pkg, name, $3);
    }
    | ID '.' ID '[' optional_type_list ']'
    {
        // Ident pkg = {$1, loc(@1), NULL};
        // Ident name = {$3, loc(@3), NULL};
        // $$ = expr_type_klass(lloc(@1, @6), pkg, name, $5);
    }
    | ID '.' error
    {
        // kl_error(loc(@3), "expected ID.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | ID '[' error
    {
        // kl_error(loc(@3), "expected type or type-list.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | ID '[' optional_type_list error
    {
        // kl_error(loc(@4), "expected ']'.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | ID '.' ID '[' error
    {
        // kl_error(loc(@5), "expected type or type-list.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | ID '.' ID '[' optional_type_list error
    {
        // kl_error(loc(@6), "expected ']'.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

atom_type
    : INT8
    {
        $$ = int8_type();
        type_set_loc($$, loc(@1));
    }
    | INT16
    {
        $$ = int16_type();
        type_set_loc($$, loc(@1));
    }
    | INT32
    {
        $$ = int32_type();
        type_set_loc($$, loc(@1));
    }
    | INT64
    {
        $$ = int64_type();
        type_set_loc($$, loc(@1));
    }
    | FLOAT32
    {
        $$ = float32_type();
        type_set_loc($$, loc(@1));
    }
    | FLOAT64
    {
        $$ = float64_type();
        type_set_loc($$, loc(@1));
    }
    | BOOL
    {
        $$ = bool_type();
        type_set_loc($$, loc(@1));
    }
    | STRING
    {
        $$ = str_type();
        type_set_loc($$, loc(@1));
    }
    | OBJECT
    {
        $$ = object_type();
        type_set_loc($$, loc(@1));
    }
    | CHAR
    {
        $$ = NULL;
    }
    | BYTES
    {

    }
    | TYPE
    {

    }
    | RANGE
    {

    }
    ;

optional_type_list
    : optional_type
    {
        // $$ = vector_create_ptr();
        // vector_push_back($$, &$1);
    }
    | optional_type_list ',' optional_type
    {
        // $$ = $1;
        // vector_push_back($$, &$3);
    }
    ;

let_decl
    : LET ID '=' expr
    {
        Ident id = {$2, loc(@2)};
        $$ = stmt_from_var_decl(id, NULL, 1, 0, $4);
        stmt_set_loc($$, lloc(@1, @4));
    }
    | LET ID optional_type '=' expr
    {
        Ident id = {$2, loc(@2)};
        $$ = stmt_from_var_decl(id, $3, 1, 0, $5);
        stmt_set_loc($$, lloc(@1, @5));
    }
    | LET error
    {
        // kl_error(loc(@2), "expected an identifier.");
        // yy_clear_ok;
        $$ = NULL;
    }
    | LET ID error
    {
        // kl_error(loc(@3), "expected type or '='.");
        // yy_clear_ok;
        $$ = NULL;
    }
    | LET ID '=' error
    {
        // kl_error(loc(@4), "expected an expression.");
        // yy_clear_ok;
        $$ = NULL;
    }
    | LET ID optional_type error
    {
        // kl_error(loc(@4), "expected '='.");
        // yy_clear_ok;
        $$ = NULL;
    }
    | LET ID optional_type '=' error
    {
        // kl_error(loc(@5), "expected an expression.");
        // yy_clear_ok;
        $$ = NULL;
    }
    ;

var_decl
    : VAR ID type ';'
    {
        // IDENT(id, $2, @2);
        // TYPE(type, $3, @3);
        // $$ = stmt_from_vardecl(id, &type, NULL);
    }
    | VAR ID type '=' expr ';'
    {
        // IDENT(id, $2, @2);
        // TYPE(type, $3, @3);
        // $$ = stmt_from_vardecl(id, &type, $5);
    }
    | VAR ID '=' expr ';'
    {
        // IDENT(id, $2, @2);
        // $$ = stmt_from_vardecl(id, NULL, $4);
    }
    ;

free_var_decl
    : ID FREE_ASSIGN expr
    {
        // IDENT(id, $1, loc(@1));
        // $$ = stmt_from_vardecl(id, NULL, $3);
    }
    ;

assignment
    : primary_expr assign_operator expr
    {
        // $$ = stmt_from_assign($2, $1, $3);
    }
    ;

assign_operator
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
        $$ = OP_SHL_ASSIGN;
    }
    | SHR_ASSIGN
    {
        $$ = OP_SHR_ASSIGN;
    }
    | USHR_ASSIGN
    {
        $$ = OP_USHR_ASSIGN;
    }
    ;

return_stmt
    : RETURN
    {
        // $$ = stmt_from_return(NULL);
    }
    | RETURN expr
    {
        // $$ = stmt_from_return($2);
    }
    ;

jump_stmt
    : BREAK
    {
        // $$ = stmt_from_break(row(@1), col(@1));
    }
    | CONTINUE
    {
        // $$ = stmt_from_continue(row(@1), col(@1));
    }
    ;

block
    : '{' local_list '}'
    {
        $$ = $2;
    }
    | '{' '}'
    {
        $$ = vector_create_ptr();
    }
    ;

local_list
    : local
    {
        $$ = vector_create_ptr();
        if ($1) vector_push_back($$, &$1);
    }
    | local_list local
    {
        $$ = $1;
        if ($2) vector_push_back($$, &$2);
    }
    ;

local
    : expr semi
    {
        $$ = stmt_from_expr($1);
        stmt_set_loc($$, lloc(@1, @2));
    }
    | let_decl semi
    {
        $$ = $1;
    }
    | var_decl semi
    {
        $$ = NULL;
    }
    | free_var_decl semi
    {
        $$ = NULL;
    }
    | assignment semi
    {
        $$ = NULL;
    }
    | return_stmt semi
    {

    }
    | jump_stmt semi
    {

    }
    | block
    {
        $$ = stmt_from_block($1);
        stmt_set_loc($$, loc(@1));
    }
    | if_stmt
    {
        $$ = $1;
    }
    | while_stmt
    {

    }
    | for_stmt
    {

    }
    | semi
    {
        $$ = NULL;
    }
    ;

if_stmt
    : IF expr block elseif_stmt
    {
        $$ = stmt_from_if($2, $3, $4);
        stmt_set_loc($$, lloc(@1, @4));
    }
    | IF LET ID '=' expr block
    {
        IDENT(id, $3, loc(@3));
        $$ = stmt_from_if_let(&id, $5, $6);
        stmt_set_loc($$, lloc(@1, @6));
    }
    | GUARD LET ID '=' expr ELSE block
    {
        IDENT(id, $3, loc(@3));
        $$ = stmt_from_guard_let(&id, $5, $7);
        stmt_set_loc($$, lloc(@1, @7));
    }
    ;

elseif_stmt
    : %empty
    {
        $$ = NULL;
    }
    | ELSE block
    {
        $$ = stmt_from_block($2);
        stmt_set_loc($$, loc(@2));
    }
    | ELSE if_stmt
    {
        $$ = $2;
    }
    ;

while_stmt
    : WHILE expr block
    {
        // $$ = stmt_from_while($2, $3);
    }
    | WHILE block
    {
        // $$ = stmt_from_while(NULL, $2);
    }
    ;

for_stmt
    : FOR id_list IN expr block
    {

    }
    ;

id_list
    : ID
    | id_list ',' ID
    ;

expr
    : or_expr
    {
        $$ = $1;
    }
    | in_expr
    {
        $$ = NULL;
    }
    | is_expr
    {
        // $$ = $1;
    }
    | as_expr
    {
        // $$ = $1;
    }
    | range_expr
    {
        // $$ = $1;
    }
    ;

range_expr
    : or_expr DOTDOTDOT or_expr
    {
        // printf("...\n");
        // $$ = NULL;
    }
    | or_expr DOTDOTEQ or_expr
    {
        // printf("..<\n");
        // $$ = NULL;
    }
    | or_expr DOTDOTDOT error
    {
        // kl_error(loc(@3), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | or_expr DOTDOTEQ error
    {
        // kl_error(loc(@3), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

is_expr
    : or_expr IS type
    {
        // $$ = expr_from_is_expr($1, $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | or_expr IS error
    {
        // kl_error(loc(@3), "expected a type.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

as_expr
    : or_expr AS type
    {
        // $$ = expr_from_as_expr($1, $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | or_expr AS error
    {
        // kl_error(loc(@3), "expected a type.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

in_expr
    : or_expr IN or_expr
    ;

or_expr
    : and_expr
    {
        $$ = $1;
    }
    | or_expr OR and_expr
    {
        // $$ = expr_from_binary(BINARY_OR, loc(@2), $1, $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | or_expr OR error
    {
        // kl_error(loc(@3), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

and_expr
    : bit_or_expr
    {
        $$ = $1;
    }
    | and_expr AND bit_or_expr
    {
        // $$ = expr_from_binary(BINARY_AND, loc(@2), $1, $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | and_expr AND error
    {
        // kl_error(loc(@3), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

bit_or_expr
    : bit_xor_expr
    {
        $$ = $1;
    }
    | bit_or_expr '|' bit_xor_expr
    {
        // $$ = expr_from_binary(BINARY_BIT_OR, loc(@2), $1, $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | bit_or_expr '|' error
    {
        // kl_error(loc(@3), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

bit_xor_expr
    : bit_and_expr
    {
        $$ = $1;
    }
    | bit_xor_expr '^' bit_and_expr
    {
        // $$ = expr_from_binary(BINARY_BIT_XOR, loc(@2), $1, $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | bit_xor_expr '^' error
    {
        // kl_error(loc(@3), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

bit_and_expr
    : equality_expr
    {
        $$ = $1;
    }
    | bit_and_expr '&' equality_expr
    {
        // $$ = expr_from_binary(BINARY_BIT_AND, loc(@2), $1, $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | bit_and_expr '&' error
    {
        // kl_error(loc(@3), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

equality_expr
    : relation_expr
    {
        $$ = $1;
    }
    | equality_expr EQ relation_expr
    {
        // $$ = expr_from_binary(BINARY_EQ, loc(@2), $1, $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | equality_expr NE relation_expr
    {
        // $$ = expr_from_binary(BINARY_NE, loc(@2), $1, $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | equality_expr EQ error
    {
        // kl_error(loc(@3), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | equality_expr NE error
    {
        // kl_error(loc(@3), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

relation_expr
    : shift_expr
    {
        $$ = $1;
    }
    | relation_expr '<' shift_expr
    {
        // $$ = expr_from_binary(BINARY_LT, loc(@2), $1, $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | relation_expr '>' shift_expr
    {
        // $$ = expr_from_binary(BINARY_GT, loc(@2), $1, $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | relation_expr LE shift_expr
    {
        // $$ = expr_from_binary(BINARY_LE, loc(@2), $1, $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | relation_expr GE shift_expr
    {
        // $$ = expr_from_binary(BINARY_GE, loc(@2), $1, $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | relation_expr '<' error
    {
        // kl_error(loc(@3), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | relation_expr '>' error
    {
        // kl_error(loc(@3), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | relation_expr LE error
    {
        // kl_error(loc(@3), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | relation_expr GE error
    {
        // kl_error(loc(@3), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

shift_expr
    : add_expr
    {
        $$ = $1;
    }
    | shift_expr R_USHIFT add_expr
    {
        // $$ = expr_from_binary(BINARY_USHR, loc(@2), $1, $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | shift_expr R_SHIFT add_expr
    {
        // $$ = expr_from_binary(BINARY_SHR, loc(@2), $1, $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | shift_expr L_SHIFT add_expr
    {
        // $$ = expr_from_binary(BINARY_SHL, loc(@2), $1, $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | shift_expr R_USHIFT error
    {
        // kl_error(loc(@3), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | shift_expr R_SHIFT error
    {
        // kl_error(loc(@3), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | shift_expr L_SHIFT error
    {
        // kl_error(loc(@3), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

add_expr
    : multi_expr
    {
        $$ = $1;
    }
    | add_expr '+' multi_expr
    {
        // $$ = expr_from_binary(BINARY_ADD, loc(@2), $1, $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | add_expr '-' multi_expr
    {
        // $$ = expr_from_binary(BINARY_SUB, loc(@2), $1, $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | add_expr '+' error
    {
        // kl_error(loc(@3), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | add_expr '-' error
    {
        // kl_error(loc(@3), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

multi_expr
    : unary_expr
    {
        $$ = $1;
    }
    | multi_expr '*' unary_expr
    {
        // $$ = expr_from_binary(BINARY_MUL, loc(@2), $1, $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | multi_expr '/' unary_expr
    {
        // $$ = expr_from_binary(BINARY_DIV, loc(@2), $1, $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | multi_expr '%' unary_expr
    {
        // $$ = expr_from_binary(BINARY_MOD, loc(@2), $1, $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | multi_expr '*' error
    {
        // kl_error(loc(@3), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | multi_expr '/' error
    {
        // kl_error(loc(@3), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | multi_expr '%' error
    {
        // kl_error(loc(@3), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

unary_expr
    : primary_expr
    {
        $$ = $1;
    }
    | '+' unary_expr
    {
        // $$ = expr_from_unary(UNARY_PLUS, loc(@1), $2);
        // expr_set_loc($$, lloc(@1, @2));
    }
    | '-' unary_expr
    {
        // $$ = expr_from_unary(UNARY_NEG, loc(@1), $2);
        // expr_set_loc($$, lloc(@1, @2));
    }
    | '~' unary_expr
    {
        // $$ = expr_from_unary(UNARY_BIT_NOT, loc(@1), $2);
        // expr_set_loc($$, lloc(@1, @2));
    }
    | NOT unary_expr
    {
        // $$ = expr_from_unary(UNARY_NOT, loc(@1), $2);
        // expr_set_loc($$, lloc(@1, @2));
    }
    | '+' error
    {
        // kl_error(loc(@2), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | '-' error
    {
        // kl_error(loc(@2), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | '~' error
    {
        // kl_error(loc(@2), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | NOT error
    {
        // kl_error(loc(@2), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

primary_expr
    : call_expr
    {
        // $$ = $1;
    }
    | dot_expr
    {
        // $$ = $1;
    }
    | index_expr
    {
        // $$ = $1;
    }
    | atom_expr
    {
        $$ = $1;
    }
    ;

call_expr
    : primary_expr '(' ')'
    {
        // $$ = expr_from_call($1, NULL);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | primary_expr '(' expr_list ')'
    {
        // $$ = expr_from_call($1, $3);
        // expr_set_loc($$, lloc(@1, @4));
    }
    | primary_expr '(' error
    {
        // kl_error(loc(@3), "expected expr, expr-list or ')'.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | primary_expr '(' expr_list error
    {
        // kl_error(loc(@4), "expected ')'.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

dot_expr
    : primary_expr '.' ID
    {
        // Ident id = {$3, loc(@3)};
        // $$ = expr_from_attr($1, id);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | primary_expr '.' error
    {
        // kl_error(loc(@3), "expected ID or Index Number.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

index_expr
    : primary_expr '[' optional_expr_list ']'
    {
        // Foo[100]
        // Foo["hello"]
        // Foo[Bar]
        // FOO[Bar?]
        // Foo[Bar, Baz]
        // $$ = expr_from_index($1, $3);
        // expr_set_loc($$, lloc(@1, @4));
    }
    | primary_expr '[' slice_expr ']'
    {
        $$ = NULL;
    }
    | primary_expr '[' error
    {
        // kl_error(loc(@3), "expected expr-type-list.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

slice_expr
    : expr ':' expr
    {
        // $$.start = $1;
        // $$.end = $3;
    }
    | ':' expr
    {
        // $$.start = NULL;
        // $$.end = $2;
    }
    | expr ':'
    {
        // $$.start = $1;
        // $$.end = NULL;
    }
    | ':'
    {
        // $$.start = NULL;
        // $$.end = NULL;
    }
    ;

atom_expr
    : atom
    {
        $$ = $1;
    }
    | '(' expr ')'
    {
        // $$ = $2;
    }
    | '(' error
    {
        // kl_error(loc(@2), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | '(' expr error
    {
        // kl_error(loc(@3), "expected ')'.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | array_expr
    {
        // $$ = $1;
    }
    | map_expr
    {
        // $$ = $1;
    }
    | tuple_expr
    {
        // $$ = $1;
    }
    | set_expr
    {

    }
    | atom_type
    {
        // $$ = expr_from_type($1);
        // expr_set_loc($$, loc(@1));
    }
    ;

atom
    : ID
    {
        IDENT(id, $1, loc(@1));
        $$ = expr_from_ident(&id);
        expr_set_loc($$, loc(@1));
    }
    | '_'
    {
        // $$ = expr_from_under();
        // expr_set_loc($$, loc(@1));
    }
    | INT_LITERAL
    {
        if (errno != 0) {
            kl_error(loc(@1), "Number %s is out of int64 range", ps->sval);
            $$ = NULL;
            YYERROR;
        } else {
            $$ = expr_from_lit_int($1);
            expr_set_loc($$, loc(@1));
        }
    }
    | FLOAT_LITERAL
    {
        if (errno != 0) {
            kl_error(loc(@1), "Number %s is out of float64 range", ps->sval);
            $$ = NULL;
            YYERROR;
        } else {
            $$ = expr_from_lit_float($1);
            expr_set_loc($$, loc(@1));
        }
    }
    | STRING_LITERAL
    {
        $$ = expr_from_lit_str(&ps->sbuf);
        expr_set_loc($$, loc(@1));
    }
    | TRUE
    {
        $$ = expr_from_lit_bool(1);
        expr_set_loc($$, loc(@1));
    }
    | FALSE
    {
        $$ = expr_from_lit_bool(0);
        expr_set_loc($$, loc(@1));
    }
    | NONE
    {
        $$ = NULL;
    }
    | SELF
    {
        $$ = NULL;
        // $$ = expr_from_self();
        // expr_set_loc($$, loc(@1));
    }
    | SUPER
    {
        $$ = NULL;
        // $$ = expr_from_super();
        // expr_set_loc($$, loc(@1));
    }
    ;

array_expr
    : '[' expr_list ']'
    {
        // // [1,2,3]
        // $$ = expr_from_array_expr($2);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | ARRAY
    {

    }
    | '[' error
    {
        // kl_error(loc(@2), "expected expr-list.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | '[' expr_list error
    {
        // kl_error(loc(@3), "expected ',' or ']'.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

map_expr
    : '{' map_list '}'
    {
        // $$ = expr_from_map($2);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | MAP
    {
        // $$ = NULL;
    }
    | '{' error
    {
        // kl_error(loc(@2), "expected key:value pairs.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | '{' map_list error
    {
        // kl_error(loc(@3), "expected ',' or '}'.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

map_list
    : map
    {
        // $$ = vector_create_ptr();
        // vector_push_back($$, &$1);
    }
    | map_list ',' map
    {
        // $$ = $1;
        // vector_push_back($$, &$3);
    }
    | map_list ',' error
    {
        // kl_error(loc(@3), "expected map key-value expr.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

map
    : expr ':' expr
    {
        // $$ = expr_from_map_entry($1, $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | expr error
    {
        // kl_error(loc(@2), "expected ':'.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | expr ':' error
    {
        // kl_error(loc(@3), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

tuple_expr
    : '(' expr_list ',' ')'
    {
        // (1, "hello", [1,2,3])
        // vector_push_back($2, &$4);
        // $$ = expr_from_tuple($2);
        // expr_set_loc($$, lloc(@1, @5));
    }
    | '(' expr_list ',' expr ')'
    {
        // $$ = NULL;
    }
    | TUPLE
    {

    }
    | '(' expr_list ',' error
    {
        // kl_error(loc(@4), "expected an expression.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | '(' expr_list ',' expr error
    {
        // kl_error(loc(@5), "expected ')'.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

set_expr
    : '{' expr_list '}'
    {

    }
    | SET
    {

    }
    ;

expr_list
    : expr
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$1);
    }
    | expr_list ',' expr
    {
        $$ = $1;
        vector_push_back($$, &$3);
    }
    ;

optional_expr
    : expr
    {
    }
    | expr '?'
    {
    }
    ;

optional_expr_list
    : optional_expr
    {

    }
    | expr_list ',' optional_expr
    {
    }
    ;

%%

/* epilogue */
