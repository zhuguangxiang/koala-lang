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
%token ENUM
%token IF
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
%token DOC

%type<stmt> top_decl
%type<stmt> let_decl

%type<expr> expr
%type<expr> or_expr
%type<expr> is_expr
%type<expr> as_expr
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
%type<expr> atom_expr
%type<expr> array_expr
%type<expr> map_expr
%type<expr> map
%type<expr> atom
%type<expr> tuple_expr

%type<type> type
%type<type> array_type
%type<type> map_type
%type<type> tuple_type
%type<type> klass_type
%type<type> atom_type

%type<vec> type_list
%type<vec> map_list
%type<vec> expr_list

%token<sval> ID
%token<ival> INT_LITERAL
%token<fval> FLOAT_LITERAL
%token STRING_LITERAL

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
    : top_decl
    | program top_decl
    ;

top_decl
    : let_decl semi
    {
        yyparse_top_decl(ps, $1);
    }
    | prefix let_decl semi
    {
        // $$ = $2;
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
    : ARRAY '[' type ']'
    {
        // $$ = NULL; // expr_type_array(lloc(@1, @4), $3);
    }
    ;

map_type
    : MAP '[' type ',' type ']'
    {
        // $$ = NULL; //expr_type_map(lloc(@1, @6), $3, $5);
    }
    ;

tuple_type
    : '(' type_list ',' type ')'
    {
        // vector_push_back($2, &$4);
        // $$ = expr_type_tuple(lloc(@1, @5), $2);
    }
    | '(' error
    {
        // kl_error(loc(@2), "expected type-list.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | '(' type_list error
    {
        // kl_error(loc(@3), "expected ','.");
        // yy_clear_ok;
        // $$ = NULL;
    }
    | '(' type_list ',' error
    {
        // kl_error(loc(@4), "expected type or ')'.");
        // yy_clear_ok;
        // $$ = NULL;
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
    | ID '[' type_list ']'
    {
        // Ident pkg = {NULL, {0, 0, 0, 0}, NULL};
        // Ident name = {$1, loc(@1), NULL};
        // $$ = expr_type_klass(lloc(@1, @4), pkg, name, $3);
    }
    | ID '.' ID '[' type_list ']'
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
    | ID '[' type_list error
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
    | ID '.' ID '[' type_list error
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
    ;

type_list
    : type
    {
        // $$ = vector_create_ptr();
        // vector_push_back($$, &$1);
    }
    | type_list ',' type
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
    | LET ID type '=' expr
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
    | LET ID type error
    {
        // kl_error(loc(@4), "expected '='.");
        // yy_clear_ok;
        $$ = NULL;
    }
    | LET ID type '=' error
    {
        // kl_error(loc(@5), "expected an expression.");
        // yy_clear_ok;
        $$ = NULL;
    }
    ;

expr
    : or_expr
    {
        $$ = $1;
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
    | primary_expr '.' INT_LITERAL
    {
        // $$ = expr_from_tuple_get($1, $3, loc(@3));
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
    : primary_expr '[' expr_list ',' expr ']'
    {
        // Foo[Bar, Baz]
        // $$ = expr_from_index($1, $3);
        // expr_set_loc($$, lloc(@1, @4));
    }
    | primary_expr '[' expr ']'
    {
        // Foo[100]
        // Foo["hello"]
        // Foo[Bar]
    }
    | primary_expr '[' error
    {
        // kl_error(loc(@3), "expected expr-type-list.");
        // yy_clear_ok;
        // $$ = NULL;
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
    | atom_type
    {
        // $$ = expr_from_type($1);
        // expr_set_loc($$, loc(@1));
    }
    ;

atom
    : ID
    {
        // Ident id = {$1, loc(@1)};
        // $$ = expr_from_ident(id, NULL);
        // expr_set_loc($$, loc(@1));
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
    | SELF
    {
        // $$ = expr_from_self();
        // expr_set_loc($$, loc(@1));
    }
    | SUPER
    {
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
    | array_type
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
    | map_type
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

%%

/* epilogue */
