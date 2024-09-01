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

static void free_optional_type_list(Vector *vec)
{
    Type **item;
    vector_foreach(item, vec) {
        free_type(*item);
    }
    vector_destroy(vec);
}

static void free_expr_list(Vector *vec)
{
    Expr **item;
    vector_foreach(item, vec) {
        expr_free(*item);
    }
    vector_destroy(vec);
}

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
%type<stmt> func_decl
%type<stmt> field_decl
%type<stmt> prefix_field_decl
%type<stmt> prefix_method_decl
%type<stmt> trait_decl
%type<stmt> proto_decl
%type<stmt> prefix_proto_decl

%type<expr> expr
%type<expr> or_expr
%type<expr> is_expr
%type<expr> as_expr
%type<expr> in_expr
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
%type<expr> anony_expr

%type<type> optional_type
%type<type> type
%type<type> array_type
%type<type> map_type
%type<type> set_type
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
%type<vec> optional_expr_list
%type<vec> param_list
%type<vec> id_type_arg_list
%type<vec> kw_arg_list
%type<vec> klass_list
%type<vec> class_members_or_empty
%type<vec> field_list
%type<vec> method_list
%type<vec> trait_members_or_empty
%type<vec> proto_list
%type<vec> type_param_decl_list

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
    : IMPORT id_list semi
    {
        // $$ = stmt_from_import($2, NULL);
        // stmt_set_loc($$, lloc(@1, @3));
    }
    | IMPORT id_list AS ID semi
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
        $$ = $1;
    }
    | prefix var_decl semi
    {
        $$ = $2;
    }
    | free_var_decl semi
    {
        $$ = $1;
    }
    | prefix free_var_decl semi
    {
        $$ = $2;
    }
    | func_decl
    {
        $$ = NULL;
    }
    | prefix func_decl
    {
        $$ = NULL;
    }
    | class_decl
    {
        $$ = NULL;
    }
    | class_prefix class_decl
    {
        $$ = NULL;
    }
    | trait_decl
    {
        $$ = NULL;
    }
    | prefix trait_decl
    {
        $$ = NULL;
    }
    | assignment semi
    {
        $$ = $1;
    }
    | expr semi
    {
        $$ = stmt_from_expr($1);
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
    | match_stmt
    {
        $$ = NULL;
    }
    | semi
    {
        $$ = NULL;
    }
    | error {
        kl_error(loc(@1), "syntax error.");
        yyclearin; yyerrok;
        $$ = NULL;
    }
    ;


semi
    : ';'
    ;

class_prefix
    : PUBLIC
    | FINAL
    | PUBLIC FINAL
    | docs
    | docs PUBLIC
    | docs FINAL
    | docs PUBLIC FINAL
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
        $$ = optional_type($1);
        type_set_loc($$, lloc(@1, @2));
    }
    ;

type
    : array_type
    {
        $$ = $1;
    }
    | map_type
    {
        $$ = $1;
    }
    | tuple_type
    {
        $$ = $1;
    }
    | set_type
    {
        $$ = $1;
    }
    | klass_type
    {
        $$ = $1;
    }
    | atom_type
    {
        $$ = $1;
    }
    ;

array_type
    : ARRAY '[' optional_type ']'
    {
        $$ = array_type($3);
        type_set_loc($$, lloc(@1, @4));
    }
    | ARRAY
    {
        $$ = array_type(NULL);
        type_set_loc($$, loc(@1));
    }
    | ARRAY '[' error
    {
        kl_error(loc(@3), "expected type.");
        yy_clear_ok;
        $$ = NULL;
    }
    | ARRAY '[' optional_type error
    {
        free_type($3);
        kl_error(loc(@4), "expected ']'.");
        yy_clear_ok;
        $$ = NULL;
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
    | MAP '[' error
    {

    }
    | MAP '[' type  error
    {

    }
    | MAP '[' type ',' error
    {

    }
    | MAP '[' type ',' optional_type error
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
    | TUPLE '[' error
    {

    }
    | TUPLE '[' optional_type_list error
    {

    }
    ;

set_type
    : SET '[' type ']'
    {

    }
    | SET
    {

    }
    | SET '[' error
    {

    }
    | SET '[' type error
    {

    }
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
        kl_error(loc(@3), "expected and identifer.");
        yy_clear_ok;
        $$ = NULL;
    }
    | ID '[' error
    {
        kl_error(loc(@3), "expected type or type-list.");
        yy_clear_ok;
        $$ = NULL;
    }
    | ID '[' optional_type_list error
    {
        kl_error(loc(@4), "expected ']'.");
        yy_clear_ok;
        $$ = NULL;
    }
    | ID '.' ID '[' error
    {
        kl_error(loc(@5), "expected type or type-list.");
        yy_clear_ok;
        $$ = NULL;
    }
    | ID '.' ID '[' optional_type_list error
    {
        kl_error(loc(@6), "expected ']'.");
        yy_clear_ok;
        $$ = NULL;
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
        NIY();
    }
    | BYTES
    {
        NIY();
    }
    | TYPE
    {
        NIY();
    }
    | RANGE
    {
        NIY();
    }
    ;

optional_type_list
    : optional_type
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$1);
    }
    | optional_type_list ',' optional_type
    {
        $$ = $1;
        vector_push_back($$, &$3);
    }
    | optional_type_list ',' error
    {
        free_optional_type_list($$);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

let_decl
    : LET ID '=' expr
    {
        Ident id = {$2, loc(@2)};
        $$ = stmt_from_var_decl(id, NULL, 1, $4);
        stmt_set_loc($$, lloc(@1, @4));
    }
    | LET ID type '=' expr
    {
        Ident id = {$2, loc(@2)};
        $$ = stmt_from_var_decl(id, $3, 1, $5);
        stmt_set_loc($$, lloc(@1, @5));
    }
    | LET error
    {
        kl_error(loc(@2), "expected an identifier.");
        yy_clear_ok;
        $$ = NULL;
    }
    | LET ID error
    {
        kl_error(loc(@3), "expected type or '='.");
        yy_clear_ok;
        $$ = NULL;
    }
    | LET ID '=' error
    {
        kl_error(loc(@4), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | LET ID type error
    {
        free_type($3);
        kl_error(loc(@4), "expected '='.");
        yy_clear_ok;
        $$ = NULL;
    }
    | LET ID type '=' error
    {
        free_type($3);
        kl_error(loc(@5), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

var_decl
    : VAR ID optional_type
    {
        IDENT(id, $2, loc(@2));
        $$ = stmt_from_var_decl(id, $3, 0, NULL);
        stmt_set_loc($$, lloc(@1, @3));
    }
    | VAR ID optional_type '=' expr
    {
        IDENT(id, $2, loc(@2));
        $$ = stmt_from_var_decl(id, $3, 0, $5);
        stmt_set_loc($$, lloc(@1, @5));
    }
    | VAR ID '=' expr
    {
        IDENT(id, $2, loc(@2));
        $$ = stmt_from_var_decl(id, NULL, 0, $4);
        stmt_set_loc($$, lloc(@1, @4));
    }
    | VAR error
    {
        kl_error(loc(@2), "expected an identifer.");
        yy_clear_ok;
        $$ = NULL;
    }
    | VAR ID error
    {
        kl_error(loc(@3), "expected a type or '='.");
        yy_clear_ok;
        $$ = NULL;
    }
    | VAR ID optional_type error
    {
        free_type($3);
        kl_error(loc(@4), "expected ';' or '='.");
        yy_clear_ok;
        $$ = NULL;
    }
    | VAR ID '=' error
    {
        kl_error(loc(@4), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | VAR ID optional_type '=' error
    {
        free_type($3);
        kl_error(loc(@5), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

free_var_decl
    : ID FREE_ASSIGN expr
    {
        IDENT(id, $1, loc(@1));
        $$ = stmt_from_var_decl(id, NULL, 0, $3);
    }
    | ID FREE_ASSIGN error
    {
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

func_decl
    : FUNC name '(' param_list ')' optional_type block
    {

    }
    | FUNC name '(' param_list ')' block
    {

    }
    | FUNC name '(' ')' optional_type block
    {

    }
    | FUNC name '(' ')' block
    {

    }
    ;

param_list
    : id_type_arg_list
    {
        // $$ = $1;
    }
    | kw_arg_list
    {
        // $$ = vector_create(sizeof(ParamDecl));
        // vector_push_back($$, &$1);
    }
    | id_type_arg_list ',' kw_arg_list
    {
        // $$ = $1;
        // vector_push_back($$, &$3);
    }
    ;

id_type_arg_list
    : ID optional_type
    {
        // Ident id = {$1, loc(@1)};
        // ParamDecl param = {lloc(@1, @2), id, $2};
        // $$ = vector_create(sizeof(ParamDecl));
        // vector_push_back($$, &param);
    }
    | id_type_arg_list ',' ID optional_type
    {
        // $$ = $1;
        // Ident id = {$3, loc(@3)};
        // ParamDecl param = {lloc(@3, @4), id, $4};
        // vector_push_back($$, &param);
    }
    ;

kw_arg_list
    : ID '=' expr
    {

    }
    | ID optional_type '=' expr
    {

    }
    ;

class_decl
    : CLASS name extends '{' class_members_or_empty '}'
    ;

name
    : ID
    {
    }
    | ID '[' type_param_decl_list ']'
    {
    }
    ;

type_param_decl_list
    : type_param_decl
    {
        // $$ = vector_create(sizeof(TypeParamDecl));
        // vector_push_back($$, &$1);
    }
    | type_param_decl_list ',' type_param_decl
    {
        // $$ = $1;
        // vector_push_back($$, &$3);
    }
    ;

type_param_decl
    : ID
    | ID ':' klass_list
    ;

extends
    : %empty
    | ':' klass_list
    ;

klass_list
    : klass_type
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$1);
    }
    | klass_list '&' klass_type
    {
        $$ = $1;
        vector_push_back($$, &$3);
    }
    ;

class_members_or_empty
    : %empty
    {
        $$ = NULL;
    }
    | field_list
    {
        $$ = $1;
    }
    | method_list
    {
        $$ = $1;
    }
    | field_list method_list
    {
        $$ = $1;
        vector_concat($$, $2);
        vector_destroy($2);
    }
    ;

field_list
    : prefix_field_decl
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$1);
    }
    | field_list prefix_field_decl
    {
        $$ = $1;
        vector_push_back($$, &$2);
    }
    ;

prefix_field_decl
    : field_decl
    {
        $$ = $1;
    }
    | prefix field_decl
    {
        $$ = $2;
    }
    ;

field_decl
    : var_decl semi
    {
        // $$ = $1;
        // var_set_where($$, VAR_FIELD);
    }
    | let_decl semi
    {
        // $$ = $1;
        // var_set_where($$, VAR_FIELD);
    }
    | ID optional_type semi
    {

    }
    | ID optional_type '=' expr semi
    {

    }
    | ID '=' expr semi
    {

    }
    | ID error
    {
        // kl_error(loc(@3), "expected type or '='.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | ID optional_type error
    {
        // kl_error(loc(@4), "expected '='.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    ;

method_list
    : prefix_method_decl
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$1);
    }
    | method_list prefix_method_decl
    {
        $$ = $1;
        vector_push_back($$, &$2);
    }
    ;

prefix_method_decl
    : func_decl
    {
        $$ = $1;
    }
    | prefix func_decl
    {
        $$ = $2;
    }
    | proto_decl
    {

    }
    | prefix proto_decl
    {

    }
    ;

trait_decl
    : TRAIT name extends '{' trait_members_or_empty '}'
    {
        // $$ = stmt_from_type(STMT_TRAIT_KIND, $2.id, $2.tps, NULL, NULL);
        // stmt_set_loc($$, lloc(@1, @6));
    }
    ;

trait_members_or_empty
    : %empty
    {
        $$ = NULL;
    }
    | proto_list
    {
        $$ = $1;
    }
    ;

proto_list
    : prefix_proto_decl
    {

    }
    | proto_list prefix_proto_decl
    {

    }
    ;

prefix_proto_decl
    : proto_decl
    {
        $$ = $1;
    }
    | prefix proto_decl
    {
        $$ = $2;
    }
    ;

proto_decl
    : FUNC name '(' param_list ')' optional_type semi
    {
        $$ = NULL;
    }
    | FUNC name '(' param_list ')' semi
    {
        $$ = NULL;
    }
    | FUNC name '(' ')' optional_type semi
    {
        $$ = NULL;
    }
    | FUNC name '(' ')' semi
    {
        $$ = NULL;
    }
    ;

assignment
    : primary_expr assign_operator expr
    {
        $$ = stmt_from_assignment($2, $1, $3);
        stmt_set_loc($$, lloc(@1, @3));
    }
    | primary_expr assign_operator error
    {
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
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
    | match_stmt
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

match_stmt
    : MATCH expr '{' case_list '}'
    ;

case_list
    : case_stmt
    | case_list case_stmt
    ;

case_stmt
    : CASE case_pattern_list ':' case_block case_tail
    ;

case_pattern_list
    : case_pattern
    | case_pattern_list ',' case_pattern
    ;

case_pattern
    : expr
    | IN expr
    | IS type
    ;

case_block
    : block
    | expr
    | assignment
    | return_stmt
    ;

case_tail
    : %empty
    | semi
    | ','
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
    | or_expr IN error
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
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
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
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
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
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
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
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
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
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
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
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | equality_expr NE error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
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
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | relation_expr '>' error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | relation_expr LE error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | relation_expr GE error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expression.");
        yy_clear_ok;
        $$ = NULL;
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
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | shift_expr R_SHIFT error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | shift_expr L_SHIFT error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
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
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | add_expr '-' error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
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
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | multi_expr '/' error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | multi_expr '%' error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
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
        kl_error(loc(@2), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | '-' error
    {
        kl_error(loc(@2), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | '~' error
    {
        kl_error(loc(@2), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | NOT error
    {
        kl_error(loc(@2), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
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
        expr_free($1);
        kl_error(loc(@3), "expected expr, expr-list or ')'.");
        yy_clear_ok;
        $$ = NULL;
    }
    | primary_expr '(' expr_list error
    {
        expr_free($1);
        free_expr_list($3);
        kl_error(loc(@4), "expected ')'.");
        yy_clear_ok;
        $$ = NULL;
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
        expr_free($1);
        kl_error(loc(@3), "expected an identifer.");
        yy_clear_ok;
        $$ = NULL;
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
        expr_free($1);
        kl_error(loc(@3), "expected expr-list.");
        yy_clear_ok;
        $$ = NULL;
    }
    | primary_expr '[' optional_expr_list error
    {
        expr_free($1);
        free_expr_list($3);
        kl_error(loc(@4), "expected ']'.");
        yy_clear_ok;
        $$ = NULL;
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
    | ':' error
    {

    }
    | expr ':' error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

atom_expr
    : atom
    {
        $$ = $1;
    }
    | '(' expr ')'
    {
        $$ = $2;
    }
    | '(' error
    {
        kl_error(loc(@2), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | '(' expr error
    {
        expr_free($2);
        kl_error(loc(@3), "expected ')'.");
        yy_clear_ok;
        $$ = NULL;
    }
    | array_expr
    {
        $$ = $1;
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
    | anony_expr
    {

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
        $$ = expr_from_under();
        expr_set_loc($$, loc(@1));
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
        $$ = expr_from_lit_none();
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
    ;

array_expr
    : '[' expr_list ']'
    {
        // [1,2,3]
        // $$ = expr_from_array($2);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | ARRAY
    {

    }
    | '[' error
    {
        kl_error(loc(@2), "expected expr-list.");
        yy_clear_ok;
        $$ = NULL;
    }
    | '[' expr_list error
    {
        kl_error(loc(@3), "expected ',' or ']'.");
        yy_clear_ok;
        $$ = NULL;
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

anony_expr
    : FUNC '(' param_list ')' optional_type block
    {

    }
    | FUNC '(' param_list ')' block
    {

    }
    | FUNC '(' ')' optional_type block
    {

    }
    | FUNC '(' ')' block
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
    | dot_expr '?'
    {
    }
    | index_expr '?'
    {
    }
    | atom_expr '?'
    {
    }
    ;

optional_expr_list
    : optional_expr
    {

    }
    | optional_expr_list ',' optional_expr
    {
    }
    | optional_expr_list ',' error
    {

    }
    ;

%%

/* epilogue */
