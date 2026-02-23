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
        TRAIT, RETURN
    };

    for (int i = 0; i < COUNT_OF(tokens); i++) {
        if (tokens[i] == token) return 0;
    }
    return 1;
}

#define yy_clear_ok if (need_clear(ps->token)) yyclearin; yyerrok

static void free_optional_type_list(Vector *vec)
{
    TypeSpec **item;
    vector_foreach(item, vec) {
        type_spec_free(*item);
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

static void free_map_list(Vector *vec)
{
    Expr **item;
    vector_foreach(item, vec) {
        expr_free(*item);
    }
    vector_destroy(vec);
}

static void free_klass_list(Vector *vec)
{

}

static void free_tp_list(Vector *vec)
{

}

static void yyparse_module(ParserState *ps, Vector *stmts)
{
    Stmt *stmt;
    vector_foreach(stmt, stmts) {
        parse_top_stmt(ps, stmt);
    }
    vector_destroy(stmts);
}

%}

%union {
    char *sval;
    __int128 ival;
    __float128 fval;
    Stmt *stmt;
    Expr *expr;
    TypeSpec *type_spec;
    Vector *vec;
    PrefixFlags prefix_flags;
    Ident ident;
    TypeParamDecl *tpval;
    ParamDecl *param;
}

%token FROM
%token IMPORT
%token CONST
%token LET
%token VAR
%token FUNC
%token CLASS
%token TRAIT
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
%token INFER

%token TRUE
%token FALSE
%token NONE
%token PANIC

%token UINT8
%token UINT16
%token UINT32
%token UINT64
%token INT8
%token INT16
%token INT32
%token INT64

%token FLOAT16
%token FLOAT32
%token FLOAT64
%token BFLOAT16

%token BOOL
%token STRING
%token ANY
%token LIST
%token MAP
%token TUPLE
%token SET
%token TYPE
%token RANGE

%token AND
%token OR
%token NOT

%token EQ
%token NE
%token GE
%token LE

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
%token DOTDOTDOT

%token L_SHIFT
%token R_SHIFT
%token R_USHIFT
%token DOC

%token OPT_DEF
%token OPT_DOT
%token BANG_DOT

%type<stmt> import_stmt
%type<stmt> top_stmt
%type<stmt> const_decl
%type<stmt> let_decl
%type<stmt> var_decl
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
%type<stmt> method_decl
%type<stmt> trait_decl
%type<stmt> class_decl
%type<stmt> func_proto_decl
%type<stmt> trait_method

%type<expr> expr
%type<expr> or_expr
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
%type<expr> list_expr
%type<expr> map_expr
%type<expr> map
%type<expr> atom
%type<expr> tuple_expr
%type<expr> anony_expr
%type<expr> assign_left_expr

%type<type_spec> array_type
%type<type_spec> optional_type
%type<type_spec> param_type
%type<type_spec> type
%type<type_spec> list_type
%type<type_spec> map_type
%type<type_spec> set_type
%type<type_spec> tuple_type
%type<type_spec> klass_type
%type<type_spec> atom_type
%type<type_spec> union_type;
%type<type_spec> union_opt_type;
%type<type_spec> const_type

%type<vec> id_as_list
%type<vec> top_stmts
%type<vec> optional_type_list
%type<vec> block
%type<vec> local_list
%type<vec> map_list
%type<vec> expr_list
%type<vec> index_expr_list
%type<vec> param_list
%type<vec> id_type_arg_list
%type<vec> kw_arg_list
%type<vec> klass_list
%type<vec> extends
%type<vec> class_members_or_empty
%type<vec> field_list
%type<vec> method_list
%type<vec> trait_members_or_empty
%type<vec> trait_method_list
%type<vec> type_param_list
%type<vec> const_tp_list
%type<vec> tp_decl_list
%type<vec> call_arg_list
%type<vec> call_kw_arg_list

%token<sval> ID
%token<ival> INT_LITERAL
%token<fval> FLOAT_LITERAL
%token STRING_LITERAL
%type<prefix_flags> annotation
%type<prefix_flags> prefix
%type<prefix_flags> access
%type<ival> assign_operator
%type<ident> class_name
%type<ident> trait_name
%type<tpval> type_param_decl
%type<param> kw_arg

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
        yyparse_module(ps, $1);
    }
    ;

import_stmt
    : IMPORT STRING_LITERAL semi
    {
        // $$ = stmt_from_import($2, $4);
        // stmt_set_loc($$, lloc(@1, @5));
        $$ = NULL;
    }
    | IMPORT STRING_LITERAL AS ID semi
    {
        // $$ = stmt_from_import($2, $4);
        // stmt_set_loc($$, lloc(@1, @5));
        $$ = NULL;
    }
    | FROM STRING_LITERAL IMPORT id_as_list semi
    {
        $$ = NULL;
    }
    | IMPORT STRING_LITERAL error
    {
        $$ = NULL;
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
    : import_stmt
    {
        $$ = $1;
    }
    | const_decl semi
    {
        // $$ = $1;
        // if ($$) var_set_where($$, VAR_GLOBAL);
    }
    | prefix const_decl semi
    {
        // $$ = $2;
        // if ($$) var_set_where($$, VAR_GLOBAL);
    }
    | let_decl semi
    {
        $$ = $1;
        if ($$) var_set_where($$, VAR_GLOBAL);
    }
    | prefix let_decl semi
    {
        $$ = $2;
        if ($$) var_set_where($$, VAR_GLOBAL);
    }
    | var_decl semi
    {
        $$ = $1;
        if ($$) var_set_where($$, VAR_GLOBAL);
    }
    | prefix var_decl semi
    {
        $$ = $2;
        if ($$) var_set_where($$, VAR_GLOBAL);
    }
    | func_decl
    {
        $$ = $1;
    }
    | prefix func_decl
    {
        $$ = $2;
        stmt_set_prefix($$, $1);
    }
    | class_decl
    {
        $$ = $1;
    }
    | prefix class_decl
    {
        $$ = $2;
        stmt_set_prefix($$, $1);
    }
    | trait_decl
    {
        $$ = $1;
    }
    | prefix trait_decl
    {
        $$ = $2;
        stmt_set_prefix($$, $1);
    }
    | semi
    {
        $$ = NULL;
    }
    | prefix error
    {
        $$ = NULL;
    }
    | error {
        if (ps->errors == 0) {
            kl_error(loc(@1), "syntax error.");
        }
        yyclearin; yyerrok;
        $$ = NULL;
    }
    | type_alias semi
    {
        $$ = NULL;
    }
    | expr semi
    {
        $$ = stmt_from_expr($1);
        stmt_set_loc($$, loc(@1));
    }
    | assignment semi
    {
        $$ = $1;
        stmt_set_loc($$, loc(@1));
    }
    | if_stmt
    {
        $$ = $1;
    }
    | while_stmt
    {
        $$ = $1;
    }
    | for_stmt
    {
        $$ = NULL;
    }
    | match_stmt
    {
        $$ = NULL;
    }
    ;

type_alias
    : TYPE ID '=' type
    /* {
        // IDENT(id, $2, loc(@2));
        // $$ = stmt_from_type_alias(id, $4);
        // stmt_set_loc($$, lloc(@1, @4));
    } */
    | TYPE ID '=' anony_type
    ;

semi
    : ';'
    ;

prefix
    : DOC
    {
        memset(&$$, 0, sizeof($$));
        $$.doc.flag = 1;
    }
    | annotation
    {
        $$ = $1;
    }
    | access
    {
        $$ = $1;
    }
    | DOC annotation
    {
        $$ = $2;
        $$.doc.flag = 1;
    }
    | DOC access
    {
        $$ = $2;
        $$.doc.flag = 1;
    }
    | annotation access
    {
        $$ = $1;
        $$.pub = $2.pub;
    }
    | DOC annotation access
    {
        $$ = $2;
        $$.doc.flag = 1;
        $$.pub = $3.pub;
    }
    ;

access
    : PUBLIC
    {
        memset(&$$, 0, sizeof($$));
        $$.pub.flag = 1;
        $$.pub.loc = loc(@1);
    }
    ;

annotation
    : '@' ID semi
    {
        memset(&$$, 0, sizeof($$));
        AtFlag *at = &$$.at;
        at->flag.flag = 1;
        at->flag.loc = lloc(@1, @3);
        at->ident = $2;
        at->id_loc = loc(@2);
    }
    | '@' ID '(' ID ')' semi
    {
        memset(&$$, 0, sizeof($$));
        AtFlag *at = &$$.at;
        at->flag.flag = 1;
        at->flag.loc = lloc(@1, @3);
        at->ident = $2;
        at->id_loc = loc(@2);
        at->assoc_ident = $4;
        at->assoc_id_loc = loc(@4);
    }
    ;

optional_type
    : type
    {
        $$ = $1;
    }
    | type '?'
    {
        $$ = optional_type_spec($1);
        type_spec_loc($$, lloc(@1, @2));
    }
    | array_type
    {
        printf("array_type\n");
        $$ = NULL;
    }
    | anony_type
    {
        printf("anonymous func type\n");
        $$ = NULL;
    }
    ;

type
    : list_type
    {
        $$ = $1;
    }
    | map_type
    {
        $$ = NULL;
    }
    | tuple_type
    {
        $$ = $1;
    }
    | set_type
    {
        $$ = NULL;
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

anony_type
    : FUNC '(' optional_type_list ')' optional_type
    {
        printf("optional func-type\n");
    }
    | FUNC '(' optional_type_list ')'
    {
        printf("no return func-type\n");
    }
    | FUNC '(' ')' optional_type
    {
        printf("no parameter func-type\n");
    }
    | FUNC '(' ')'
    {
        printf("no return no parameter func-type\n");
    }
    ;

union_type
    : type '|' type
    {
        $$ = union_type_spec($1, $3);
    }
    | union_type '|' type
    {
        $$ = $1;
        if ($1) {
            union_type_spec_add_arg($1, $3);
        } else {
            // $$ = union_type_spec(NULL, $3);
            // error
        }
    }
    | type '|' error
    {
        // free_type($1);
        kl_error(loc(@3), "expected type.");
        yyclearin; yyerrok;
        $$ = NULL;
    }
    | union_type '|' error
    {
        kl_error(loc(@3), "expected type.");
        yyclearin; yyerrok;
        $$ = NULL;
    }
    ;

union_opt_type
    : union_type
    {
        $$ = $1;
    }
    | '(' union_type ')'
    {
        $$ = $2;
    }
    | '(' union_type ')' '?'
    {
        $$ = optional_type_spec($2);
        type_spec_loc($$, lloc(@1, @4));
    }
    ;

list_type
    : LIST '[' optional_type ']'
    {
        NAME_ID(id, "list", loc(@1));
        Vector *args = vector_create_ptr();
        vector_push_back(args, &$3);
        $$ = unresolved_type_spec(NULL, id, args);
        type_spec_loc($$, lloc(@1, @4));
    }
    | LIST '[' error
    {
        kl_error(loc(@3), "expected type.");
        yy_clear_ok;
        $$ = NULL;
    }
    | LIST '[' optional_type error
    {
        // free_type($3);
        kl_error(loc(@4), "expected ']'.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

array_type
    : '[' int_lit_list ']' type
    {

    }
    | '[' int_lit_list ']' type '?'
    {

    }
    | '[' ']' type
    {

    }
    | '[' ']' type '?'
    {

    }
    ;

int_lit_list
    : INT_LITERAL
    | int_lit_list ',' INT_LITERAL
    | int_lit_list ','
    ;

map_type
    : MAP '[' type ',' optional_type ']'
    {
        // $$ = map_type($3, $5);
        // type_set_loc($$, lloc(@1, @6));
    }
    | MAP '[' error
    {
        kl_error(loc(@3), "expected a type.");
        yy_clear_ok;
        $$ = NULL;
    }
    | MAP '[' type  error
    {
        // free_type($3);
        kl_error(loc(@4), "expected a ','.");
        yy_clear_ok;
        $$ = NULL;
    }
    | MAP '[' type ',' error
    {
        // free_type($3);
        kl_error(loc(@5), "expected a type.");
        yy_clear_ok;
        $$ = NULL;
    }
    | MAP '[' type ',' optional_type error
    {
        // free_type($3);
        // free_type($5);
        kl_error(loc(@6), "expected a ']'.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

tuple_type
    : TUPLE '[' optional_type_list ']'
    {
        $$ = tuple_type_spec($3);
        type_spec_loc($$, lloc(@1, @4));
    }
    | '(' optional_type_list ')'
    {
        $$ = tuple_type_spec($2);
        type_spec_loc($$, lloc(@1, @3));
    }
    | TUPLE '[' error
    {
        kl_error(loc(@3), "expected a type or type-list.");
        yy_clear_ok;
        $$ = NULL;
    }
    | TUPLE '[' optional_type_list error
    {
        free_optional_type_list($3);
        kl_error(loc(@4), "expected ']'.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

set_type
    : SET '[' optional_type ']'
    {
        // $$ = set_type($3);
        // type_set_loc($$, lloc(@1, @4));
    }
    | SET '[' error
    {
        kl_error(loc(@3), "expected a type.");
        yy_clear_ok;
        $$ = NULL;
    }
    | SET '[' type error
    {
        // free_type($3);
        kl_error(loc(@4), "expected ']'.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

klass_type
    : ID
    {
        NAME_ID(id, $1, loc(@1));
        $$ = unresolved_type_spec(NULL, id, NULL);
        type_spec_loc($$, loc(@1));
    }
    | ID '.' ID
    {
        MOD_ID(mod, $1, loc(@1));
        NAME_ID(id, $3, loc(@3));
        $$ = unresolved_type_spec(&mod, id, NULL);
        type_spec_loc($$, lloc(@1, @3));
    }
    | ID '[' optional_type_list ']'
    {
        NAME_ID(id, $1, loc(@1));
        $$ = unresolved_type_spec(NULL, id, $3);
        type_spec_loc($$, lloc(@1, @4));
    }
    | ID '.' ID '[' optional_type_list ']'
    {
        MOD_ID(mod, $1, loc(@1));
        NAME_ID(id, $3, loc(@3));
        $$ = unresolved_type_spec(&mod, id, $5);
        type_spec_loc($$, lloc(@1, @6));
    }
    | ID '.' error
    {
        kl_error(loc(@3), "expected and identifier.");
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
        free_optional_type_list($3);
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
        free_optional_type_list($5);
        kl_error(loc(@6), "expected ']'.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

atom_type
    : UINT8
    {
        $$ = uint8_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | UINT16
    {
        $$ = uint16_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | UINT32
    {
        $$ = uint32_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | UINT64
    {
        $$ = uint64_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | INT8
    {
        $$ = int8_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | INT16
    {
        $$ = int16_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | INT32
    {
        $$ = int32_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | INT64
    {
        $$ = int64_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | FLOAT16
    {
        $$ = float16_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | FLOAT32
    {
        $$ = float32_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | FLOAT64
    {
        $$ = float64_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | BFLOAT16
    {
        $$ = bfloat16_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | BOOL
    {
        $$ = bool_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | STRING
    {
        $$ = str_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | ANY
    {
        $$ = any_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | TYPE
    {
        $$ = type_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | RANGE
    {
        $$ = range_type_spec();
        type_spec_loc($$, loc(@1));
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

const_type
    : UINT8
    {
        $$ = uint8_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | UINT16
    {
        $$ = uint16_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | UINT32
    {
        $$ = uint32_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | UINT64
    {
        $$ = uint64_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | INT8
    {
        $$ = int8_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | INT16
    {
        $$ = int16_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | INT32
    {
        $$ = int32_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | INT64
    {
        $$ = int64_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | FLOAT16
    {
        $$ = float16_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | FLOAT32
    {
        $$ = float32_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | FLOAT64
    {
        $$ = float64_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | BFLOAT16
    {
        $$ = bfloat16_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | BOOL
    {
        $$ = bool_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | STRING
    {
        $$ = str_type_spec();
        type_spec_loc($$, loc(@1));
    }
    | tuple_type
    {
        $$ = $1;
    }
    ;

let_decl
    : LET ID '=' expr
    {
        Ident id = {$2, loc(@2)};
        $$ = stmt_from_var_decl(id, NULL, 1, $4);
        stmt_set_loc($$, lloc(@1, @4));
    }
    | LET ID optional_type '=' expr
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
    | LET ID optional_type error
    {
        // free_type($3);
        kl_error(loc(@4), "expected '='.");
        yy_clear_ok;
        $$ = NULL;
    }
    | LET ID optional_type '=' error
    {
        // free_type($3);
        kl_error(loc(@5), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

const_decl
    : CONST ID '=' expr
    {
        Ident id = {$2, loc(@2)};
        $$ = stmt_from_var_decl(id, NULL, 2, $4);
        stmt_set_loc($$, lloc(@1, @4));
    }
    | CONST ID const_type '=' expr
    {
        Ident id = {$2, loc(@2)};
        $$ = stmt_from_var_decl(id, $3, 2, $5);
        stmt_set_loc($$, lloc(@1, @5));
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
        kl_error(loc(@2), "expected an identifier.");
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
        // free_type($3);
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
        // free_type($3);
        kl_error(loc(@5), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

func_decl
    : func_proto_decl block
    {
        $$ = $1;
        ((FuncDeclStmt *)$$)->body = $2;
    }
    | func_proto_decl '=' expr semi
    {
        $$ = $1;
        Vector *block = vector_create_ptr();
        Stmt *s = stmt_from_expr($3);
        stmt_set_loc(s, loc(@3));
        vector_push_back(block, &s);
        ((FuncDeclStmt *)$$)->body = block;
    }
    ;

func_proto_decl
    : FUNC ID '(' param_list ')' optional_type
    {
        IDENT(id, $2, loc(@2));
        $$ = stmt_from_func_decl(id, $4, $6, NULL);
        stmt_set_loc($$, lloc(@1, @6));
    }
    | FUNC ID '(' param_list ')'
    {
        IDENT(id, $2, loc(@2));
        $$ = stmt_from_func_decl(id, $4, NULL, NULL);
        stmt_set_loc($$, lloc(@1, @5));
    }
    | FUNC ID '(' ')' optional_type
    {
        IDENT(id, $2, loc(@2));
        $$ = stmt_from_func_decl(id, NULL, $5, NULL);
        stmt_set_loc($$, lloc(@1, @5));
    }
    | FUNC ID '(' ')'
    {
        IDENT(id, $2, loc(@2));
        $$ = stmt_from_func_decl(id, NULL, NULL, NULL);
        stmt_set_loc($$, lloc(@1, @4));
    }
    | FUNC ID '[' tp_decl_list ']' '(' param_list ')' optional_type
    {
        IDENT(id, $2, loc(@2));
        $$ = stmt_from_func_decl(id, $7, $9, $4);
        stmt_set_loc($$, lloc(@1, @9));
    }
    | FUNC ID '[' tp_decl_list ']' '(' param_list ')'
    {
        IDENT(id, $2, loc(@2));
        $$ = stmt_from_func_decl(id, $7, NULL, $4);
        stmt_set_loc($$, lloc(@1, @8));
    }
    | FUNC ID '[' tp_decl_list ']' '(' ')' optional_type
    {
        IDENT(id, $2, loc(@2));
        $$ = stmt_from_func_decl(id, NULL, $8, $4);
        stmt_set_loc($$, lloc(@1, @8));
    }
    | FUNC ID '[' tp_decl_list ']' '(' ')'
    {
        IDENT(id, $2, loc(@2));
        $$ = stmt_from_func_decl(id, NULL, NULL, $4);
        stmt_set_loc($$, lloc(@1, @7));
    }
    | FUNC ID error
    {
        printf("func proto error1\n");
        $$ = NULL;
    }
    | FUNC ID '(' param_list ')' error
    {
        printf("func proto error2\n");
        $$ = NULL;
    }
    | FUNC ID '(' ')' error
    {
        $$ = NULL;
    }
    | FUNC ID '[' error
    {
        $$ = NULL;
    }
    | FUNC ID '[' tp_decl_list ']' error
    {
        $$ = NULL;
    }
    ;

param_list
    : id_type_arg_list
    {
        $$ = $1;
    }
    | kw_arg_list
    {
        $$ = $1;
    }
    | id_type_arg_list ',' kw_arg_list
    {
        $$ = $1;
        vector_concat($$, $3);
        vector_destroy($3);
    }
    ;

id_type_arg_list
    : ID param_type
    {
        Ident id = {$1, loc(@1)};
        ParamDecl *p = param_new(lloc(@1, @2), id, $2, NULL);
        $$ = vector_create_ptr();
        vector_push_back($$, &p);
    }
    | ID DOTDOTDOT
    {
        Ident id = {$1, loc(@1)};
        TypeSpec *ts = va_list_type_spec(NULL);
        type_spec_loc(ts, loc(@2));
        ParamDecl *p = param_new(lloc(@1, @2), id, ts, NULL);
        p->va_arg = 1;
        $$ = vector_create_ptr();
        vector_push_back($$, &p);
    }
    | ID DOTDOTDOT param_type
    {
        Ident id = {$1, loc(@1)};
        TypeSpec *ts = va_list_type_spec($3);
        type_spec_loc(ts, loc(@2));
        ParamDecl *p = param_new(lloc(@1, @2), id, ts, NULL);
        p->va_arg = 1;
        $$ = vector_create_ptr();
        vector_push_back($$, &p);
    }
    | id_type_arg_list ',' ID param_type
    {
        $$ = $1;
        Ident id = {$3, loc(@3)};
        ParamDecl *p = param_new(lloc(@3, @4), id, $4, NULL);
        vector_push_back($$, &p);
    }
    | id_type_arg_list ',' ID DOTDOTDOT
    {
        $$ = $1;
        Ident id = {$3, loc(@3)};
        TypeSpec *ts = va_list_type_spec(NULL);
        type_spec_loc(ts, loc(@4));
        ParamDecl *p = param_new(lloc(@3, @4), id, ts, NULL);
        p->va_arg = 1;
        vector_push_back($$, &p);
    }
    | id_type_arg_list ',' ID DOTDOTDOT param_type
    {
        $$ = $1;
        Ident id = {$3, loc(@3)};
        TypeSpec *ts = va_list_type_spec($5);
        type_spec_loc(ts, lloc(@4, @5));
        ParamDecl *p = param_new(lloc(@3, @5), id, ts, NULL);
        p->va_arg = 1;
        vector_push_back($$, &p);
    }
    ;

param_type
    : optional_type
    {
        $$ = $1;
    }
    | union_opt_type
    {
        $$ = $1;
    }
    ;

kw_arg_list
    : kw_arg
    {
        $$ = vector_create_ptr();
        if ($1) vector_push_back($$, &$1);
    }
    | kw_arg_list ',' kw_arg
    {
        $$ = $1;
        if ($3) vector_push_back($$, &$3);
    }
    ;

kw_arg
    : ID '=' expr
    {
        Ident id = {$1, loc(@1)};
        $$ = param_new(lloc(@1, @3), id, NULL, $3);
    }
    | ID optional_type '=' expr
    {
        Ident id = {$1, loc(@1)};
        $$ = param_new(lloc(@1, @4), id, $2, $4);
    }
    | ID union_opt_type '=' expr
    {
        Ident id = {$1, loc(@1)};
        $$ = param_new(lloc(@1, @4), id, $2, $4);
    }
    | ID '=' error
    {
        kl_error(loc(@3), "expected an expr.");
        yyclearin;
        yyerrok;
        $$ = NULL;
    }
    ;

class_decl
    : CLASS class_name extends '{' class_members_or_empty '}'
    {
        $$ = stmt_from_klass($2, NULL, $3, $5);
        stmt_set_loc($$, lloc(@1, @6));
    }
    | CLASS class_name '{' class_members_or_empty '}'
    {
        $$ = stmt_from_klass($2, NULL, NULL, $4);
        stmt_set_loc($$, lloc(@1, @5));
    }
    | CLASS class_name '[' tp_decl_list ']' extends '{' class_members_or_empty '}'
    {
        $$ = stmt_from_klass($2, $4, $6, $8);
        stmt_set_loc($$, lloc(@1, @9));
    }
    | CLASS class_name '[' tp_decl_list ']' '{' class_members_or_empty '}'
    {
        $$ = stmt_from_klass($2, $4, NULL, $7);
        stmt_set_loc($$, lloc(@1, @8));
    }
    | CLASS class_name '{' error '}'
    {
        kl_error(loc(@4), "expected field-decl or method-decl.");
        yy_clear_ok;
        yyclearin;
        $$ = NULL;
    }
    | CLASS class_name '[' error ']' extends '{' class_members_or_empty '}'
    {
        kl_error(loc(@4), "expected type-parameter decl-list.");
        yy_clear_ok;
        yyclearin;
        $$ = NULL;
    }
    | CLASS class_name '[' tp_decl_list ']' extends '{' error '}'
    {
        kl_error(loc(@8), "expected field-decl or method-decl.");
        yy_clear_ok;
        yyclearin;
        $$ = NULL;
    }
    | CLASS class_name '[' tp_decl_list ']' '{' error '}'
    {
        kl_error(loc(@7), "expected field-decl or method-decl.");
        yy_clear_ok;
        yyclearin;
        $$ = NULL;
    }
    | CLASS TUPLE '[' INFER ID ']' extends '{' class_members_or_empty '}'
    {
        Ident tp_id = {$5, lloc(@4, @5)};
        TypeParamDecl *tp = infer_type_param_new(lloc(@4, @5), tp_id);
        Vector *tp_list = vector_create_ptr();
        vector_push_back(tp_list, &tp);

        Ident id = {"tuple", loc(@2)};
        $$ = stmt_from_klass(id, tp_list, $7, $9);
        stmt_set_loc($$, lloc(@1, @9));
    }
    ;

class_name
    : ID
    {
        $$ = (Ident){$1, loc(@1)};
    }
    | UINT8
    {
        $$ = (Ident){"uint8", loc(@1)};
    }
    | UINT16
    {
        $$ = (Ident){"uint16", loc(@1)};
    }
    | UINT32
    {
        $$ = (Ident){"uint32", loc(@1)};
    }
    | UINT64
    {
        $$ = (Ident){"uint64", loc(@1)};
    }
    | INT8
    {
        $$ = (Ident){"int8", loc(@1)};
    }
    | INT16
    {
        $$ = (Ident){"int16", loc(@1)};
    }
    | INT32
    {
        $$ = (Ident){"int32", loc(@1)};
    }
    | INT64
    {
        $$ = (Ident){"int64", loc(@1)};
    }
    | FLOAT16
    {
        $$ = (Ident){"float16", loc(@1)};
    }
    | FLOAT32
    {
        $$ = (Ident){"float32", loc(@1)};
    }
    | FLOAT64
    {
        $$ = (Ident){"float64", loc(@1)};
    }
    | BFLOAT16
    {
        $$ = (Ident){"bfloat16", loc(@1)};
    }
    | STRING
    {
        $$ = (Ident){"str", loc(@1)};
    }
    | LIST
    {
        $$ = (Ident){"list", loc(@1)};
    }
    | MAP
    {
        $$ = (Ident){"dict", loc(@1)};
    }
    | SET
    {
        $$ = (Ident){"set", loc(@1)};
    }
    | RANGE
    {
        $$ = (Ident){"range", loc(@1)};
    }
    | TYPE
    {
        $$ = (Ident){"type", loc(@1)};
    }
    | BOOL
    {
        $$ = (Ident){"bool", loc(@1)};
    }
    ;

tp_decl_list
    : type_param_list ',' const_tp_list
    {
        vector_concat($1, $3);
        vector_destroy($3);
        $$ = $1;
    }
    | const_tp_list
    {
        $$ = $1;
    }
    | type_param_list
    {
        $$ = $1;
    }
    ;

const_tp_list
    : CONST ID const_type
    {
        Ident id = {$2, loc(@2)};
        TypeParamDecl *tp = const_type_param_new(lloc(@1, @3), id, $3);
        $$ = vector_create_ptr();
        vector_push_back($$, &tp);
    }
    | const_tp_list ',' CONST ID const_type
    {
        $$ = $1;
        Ident id = {$4, loc(@4)};
        TypeParamDecl *tp = const_type_param_new(lloc(@3, @4), id, $5);
        vector_push_back($$, &tp);
    }
    ;

type_param_list
    : type_param_decl
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$1);
    }
    | type_param_list ',' type_param_decl
    {
        $$ = $1;
        vector_push_back($$, &$3);
    }
    | type_param_list ',' error
    {
        free_tp_list($1);
        yyclearin;
        yyerrok;
        $$ = NULL;
    }
    ;

type_param_decl
    : ID
    {
        Ident id = {$1, loc(@1)};
        $$ = type_param_new(loc(@1), id, NULL);
    }
    | ID ':' klass_list
    {
        Ident id = {$1, loc(@1)};
        $$ = type_param_new(lloc(@1, @3), id, $3);
    }
    | INFER ID
    {
        Ident id = {$2, loc(@2)};
        $$ = infer_type_param_new(lloc(@1, @2), id);
    }
    | ID ':' error
    {
        kl_error(loc(@3), "expected up-bound.");
        yyclearin;
        yyerrok;
        $$ = NULL;
    }
    | ID error
    {
        kl_error(loc(@2), "expected ':' up-bound.");
        yyclearin;
        yyerrok;
        $$ = NULL;
    }
    ;

extends
    : ':' klass_list
    {
        $$ = $2;
    }
    | ':' error
    {
        kl_error(loc(@2), "expected class or trait-list");
        yyclearin;
        yyerrok;
        $$ = NULL;
    }
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
    | klass_list '&' error
    {
        free_klass_list($1);
        kl_error(loc(@3), "expected a type.");
        yyclearin;
        $$ = NULL;
    }
    | klass_list error
    {
        free_klass_list($1);
        kl_error(loc(@2), "expected '&' and type.");
        yyclearin;
        $$ = NULL;
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
        $$ = $1;
        var_set_where($$, VAR_FIELD);
    }
    | let_decl semi
    {
        $$ = $1;
        var_set_where($$, VAR_FIELD);
    }
    | LET ID optional_type semi
    {
        Ident id = {$2, loc(@2)};
        $$ = stmt_from_var_decl(id, $3, 1, NULL);
        stmt_set_loc($$, lloc(@1, @3));
        var_set_where($$, VAR_FIELD);
    }
    ;

method_list
    : method_decl
    {
        $$ = vector_create_ptr();
        if ($1) vector_push_back($$, &$1);
    }
    | method_list method_decl
    {
        $$ = $1;
        if ($2) vector_push_back($$, &$2);
    }
    ;

method_decl
    : func_decl
    {
        $$ = $1;
    }
    | prefix func_decl
    {
        $$ = $2;
        stmt_set_prefix($$, $1);
    }
    | semi
    {
        $$ = NULL;
    }
    ;

trait_decl
    : TRAIT trait_name '{' trait_members_or_empty '}'
    {
        $$ = stmt_from_trait($2, NULL, NULL, $4);
        stmt_set_loc($$, lloc(@1, @5));
    }
    | TRAIT trait_name extends '{' trait_members_or_empty '}'
    {
        $$ = stmt_from_trait($2, NULL, $3, $5);
        stmt_set_loc($$, lloc(@1, @6));
    }
    | TRAIT trait_name '[' type_param_list ']' '{' trait_members_or_empty '}'
    {
        $$ = stmt_from_trait($2, $4, NULL, $7);
        stmt_set_loc($$, lloc(@1, @8));
    }
    | TRAIT trait_name '[' type_param_list ']' extends '{' trait_members_or_empty '}'
    {
        $$ = stmt_from_trait($2, $4, $6, $8);
        stmt_set_loc($$, lloc(@1, @9));
    }
    ;

trait_name
    : ID
    {
        $$ = (Ident){$1, loc(@1)};
    }
    | ANY
    {
        $$ = (Ident){"any", loc(@1)};
    }
    ;

trait_members_or_empty
    : %empty
    {
        $$ = NULL;
    }
    | trait_method_list
    {
        $$ = $1;
    }
    ;

trait_method_list
    : trait_method
    {
        $$ = vector_create_ptr();
        if ($1) vector_push_back($$, &$1);
    }
    | trait_method_list trait_method
    {
        $$ = $1;
        if ($2) vector_push_back($$, &$2);
    }
    ;

trait_method
    : func_proto_decl semi
    {
        $$ = $1;
    }
    | prefix func_proto_decl semi
    {
        $$ = $2;
        stmt_set_prefix($$, $1);
    }
    | func_decl
    {
        $$ = $1;
    }
    | prefix func_decl
    {
        $$ = $2;
        stmt_set_prefix($$, $1);
    }
    | semi
    {
        $$ = NULL;
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
    | '{' return_stmt '}'
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$2);
    }
    | '{' jump_stmt '}'
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$2);
    }
    | '{' assignment '}'
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$2);
    }
    | '{' expr '}'
    {
        $$ = vector_create_ptr();
        Stmt *s = stmt_from_expr($2);
        stmt_set_loc(s, loc(@2));
        vector_push_back($$, &s);
    }
    | '{' local_list error
    {
        // free_local_list($2);
        kl_error(loc(@3), "expected '}'.");
        yy_clear_ok;
        $$ = NULL;
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
    | const_decl semi
    {
        // $$ = $1;
        // var_set_where($$, VAR_LOCAL);
    }
    | let_decl semi
    {
        $$ = $1;
        var_set_where($$, VAR_LOCAL);
    }
    | var_decl semi
    {
        $$ = $1;
        var_set_where($$, VAR_LOCAL);
    }
    | assignment semi
    {
        $$ = $1;
    }
    | return_stmt semi
    {
        $$ = $1;
    }
    | jump_stmt semi
    {
        $$ = $1;
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
        $$ = $1;
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
    ;

assignment
    : assign_left_expr assign_operator expr
    {
        $$ = stmt_from_assignment($2, $1, $3);
        stmt_set_loc($$, lloc(@1, @3));
    }
    | assign_left_expr assign_operator error
    {
        // expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

assign_left_expr
    : ID
    {
        IDENT(id, $1, loc(@1));
        $$ = expr_from_ident(&id);
        expr_set_loc($$, loc(@1));
    }
    | primary_expr '.' ID
    {
        IDENT(id, $3, loc(@3));
        $$ = expr_from_dot($1, &id, DOT_NORMAL);
        expr_set_loc($$, lloc(@1, @3));
    }
    | primary_expr BANG_DOT ID
    {
        IDENT(id, $3, loc(@3));
        $$ = expr_from_dot($1, &id, DOT_BANG);
        expr_set_loc($$, lloc(@1, @3));
    }
    | index_expr
    {
        $$ = $1;
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
    ;

return_stmt
    : RETURN
    {
        $$ = stmt_from_return(NULL);
        stmt_set_loc($$, loc(@1));
    }
    | RETURN expr
    {
        $$ = stmt_from_return($2);
        stmt_set_loc($$, lloc(@1, @2));
    }
    ;

jump_stmt
    : BREAK
    {
        $$ = stmt_from_break();
        stmt_set_loc($$, loc(@1));
    }
    | CONTINUE
    {
        $$ = stmt_from_continue();
        stmt_set_loc($$, loc(@1));
    }
    ;

if_stmt
    : IF expr block elseif_stmt
    {
        $$ = stmt_from_if($2, $3, $4);
        stmt_set_loc($$, lloc(@1, @4));
    }
    | IF LET ID '=' expr block elseif_stmt
    {
        IDENT(id, $3, loc(@3));
        $$ = stmt_from_if_let(&id, $5, $6, $7);
        stmt_set_loc($$, lloc(@1, @7));
    }
    | IF '(' LET ID '=' expr ')' block elseif_stmt
    {
        IDENT(id, $4, loc(@4));
        $$ = stmt_from_if_let(&id, $6, $8, $9);
        stmt_set_loc($$, lloc(@1, @9));
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
        $$ = stmt_from_while($2, $3);
        stmt_set_loc($$, lloc(@1, @3));
    }
    | WHILE LET ID '=' expr block
    {
        IDENT(id, $3, loc(@3));
        $$ = stmt_from_while_let(&id, $5, $6);
        stmt_set_loc($$, lloc(@1, @6));
    }
    | WHILE '(' LET ID '=' expr ')' block
    {
        IDENT(id, $4, loc(@4));
        $$ = stmt_from_while_let(&id, $6, $8);
        stmt_set_loc($$, lloc(@1, @8));
    }
    | WHILE block
    {
        $$ = stmt_from_while(NULL, $2);
        stmt_set_loc($$, lloc(@1, @2));
    }
    ;

for_stmt
    : FOR id_list IN expr block
    {

    }
    | FOR '(' id_list ')' IN expr block
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
        $$ = $1;
    }
    | or_expr DOTDOTDOT or_expr
    {
        $$ = NULL;
    }
    | or_expr OPT_DEF or_expr
    {
        // $$ = expr_from_is_expr($1, loc(@2), $3);
        // expr_set_loc($$, lloc(@1, @3));
    }
    | or_expr OPT_DEF error
    {
        // expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | or_expr IS type
    {
        $$ = expr_from_is_expr($1, loc(@2), $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    ;

in_expr
    : or_expr IN or_expr
    {
        $$ = expr_from_in_expr($1, loc(@2), $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | or_expr IN error
    {
        expr_free($1);
        kl_error(loc(@3), "expected a type.");
        yy_clear_ok;
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
        $$ = expr_from_binary(BINARY_OR, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
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
        $$ = expr_from_binary(BINARY_AND, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
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
        $$ = expr_from_binary(BINARY_BIT_OR, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
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
        $$ = expr_from_binary(BINARY_BIT_XOR, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
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
        $$ = expr_from_binary(BINARY_BIT_AND, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
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
        $$ = expr_from_binary(BINARY_EQ, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | equality_expr NE relation_expr
    {
        $$ = expr_from_binary(BINARY_NEQ, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
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
        $$ = expr_from_binary(BINARY_LT, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | relation_expr '>' shift_expr
    {
        $$ = expr_from_binary(BINARY_GT, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | relation_expr LE shift_expr
    {
        $$ = expr_from_binary(BINARY_LE, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | relation_expr GE shift_expr
    {
        $$ = expr_from_binary(BINARY_GE, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
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
        $$ = expr_from_binary(BINARY_USHR, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | shift_expr R_SHIFT add_expr
    {
        $$ = expr_from_binary(BINARY_SHR, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | shift_expr L_SHIFT add_expr
    {
        $$ = expr_from_binary(BINARY_SHL, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
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
        $$ = expr_from_binary(BINARY_ADD, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | add_expr '-' multi_expr
    {
        $$ = expr_from_binary(BINARY_SUB, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
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
        $$ = expr_from_binary(BINARY_MUL, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | multi_expr '/' unary_expr
    {
        $$ = expr_from_binary(BINARY_DIV, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | multi_expr '%' unary_expr
    {
        $$ = expr_from_binary(BINARY_MOD, loc(@2), $1, $3);
        expr_set_loc($$, lloc(@1, @3));
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
        $$ = expr_from_unary(UNARY_PLUS, loc(@1), $2);
        expr_set_loc($$, lloc(@1, @2));
    }
    | '-' unary_expr
    {
        $$ = expr_from_unary(UNARY_NEG, loc(@1), $2);
        expr_set_loc($$, lloc(@1, @2));
    }
    | '~' unary_expr
    {
        $$ = expr_from_unary(UNARY_BIT_NOT, loc(@1), $2);
        expr_set_loc($$, lloc(@1, @2));
    }
    | NOT unary_expr
    {
        $$ = expr_from_unary(UNARY_NOT, loc(@1), $2);
        expr_set_loc($$, lloc(@1, @2));
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
        $$ = $1;
    }
    | dot_expr
    {
        $$ = $1;
    }
    | index_expr
    {
        $$ = $1;
    }
    | atom_expr
    {
        $$ = $1;
    }
    | primary_expr NOT
    {
        // only optional type
        $$ = expr_from_bang($1);
        expr_set_loc($$, lloc(@1, @2));
    }
    | '[' expr_list ']' ID
    {

    }
    | '[' expr_list ']' ID '?'
    {

    }
    | '[' expr_list ']' atom_type
    {

    }
    | '[' expr_list ']' atom_type '?'
    {

    }
    | '[' expr_list ']' LIST
    {

    }
    | '[' expr_list ']' MAP
    {
    }
    | '[' expr_list ']' SET
    {
    }
    | '[' expr_list ']' TUPLE
    {
    }
    ;

call_expr
    : primary_expr '(' ')'
    {
        $$ = expr_from_call($1, NULL);
        expr_set_loc($$, lloc(@1, @3));
    }
    | primary_expr '(' call_arg_list ')'
    {
        $$ = expr_from_call($1, $3);
        expr_set_loc($$, lloc(@1, @4));
    }
    | primary_expr '(' error
    {
        expr_free($1);
        kl_error(loc(@3), "expected expr, expr-list or ')'.");
        yy_clear_ok;
        $$ = NULL;
    }
    | primary_expr '(' call_arg_list error
    {
        // expr_free($1);
        // free_call_arg_list($3);
        kl_error(loc(@4), "expected ')'.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

call_arg_list
    : expr_list
    {
        $$ = $1;
    }
    | call_kw_arg_list
    {
        $$ = $1;
    }
    | expr_list ',' call_kw_arg_list
    {
        $$ = $1;
        vector_concat($$, $3);
        vector_destroy($3);
    }
    ;

call_kw_arg_list
    : ID '=' expr
    {
        Ident id = {$1, loc(@1)};
        Expr *arg = expr_from_keyword(lloc(@1, @3), id, $3);
        $$ = vector_create_ptr();
        vector_push_back($$, &arg);
    }
    | call_kw_arg_list ',' ID '=' expr
    {
        Ident id = {$3, loc(@3)};
        Expr *arg = expr_from_keyword(lloc(@3, @5), id, $5);
        $$ = $1;
        vector_push_back($$, &arg);
    }
    ;

dot_expr
    : primary_expr '.' ID
    {
        IDENT(id, $3, loc(@3));
        $$ = expr_from_dot($1, &id, DOT_NORMAL);
        expr_set_loc($$, lloc(@1, @3));
    }
    | primary_expr '.' ID '?'
    {

    }
    | primary_expr '.' INT_LITERAL
    {
        // IDENT(id, $3, loc(@3));
        // $$ = expr_from_dot($1, &id);
        // expr_set_loc($$, lloc(@1, @3));
        $$ = NULL;
    }
    | primary_expr OPT_DOT ID
    {
        IDENT(id, $3, loc(@3));
        $$ = expr_from_dot($1, &id, DOT_OPTIONAL);
        expr_set_loc($$, lloc(@1, @3));
    }
    | primary_expr BANG_DOT ID
    {
        IDENT(id, $3, loc(@3));
        $$ = expr_from_dot($1, &id, DOT_BANG);
        expr_set_loc($$, lloc(@1, @3));
    }
    | primary_expr '.' AS '(' type ')'
    {
        $$ = expr_from_as_expr($1, loc(@3), $5);
        expr_set_loc($$, lloc(@1, @6));
    }
    | primary_expr '.' IS '(' type ')'
    {
        $$ = expr_from_is_expr($1, loc(@3), $5);
        expr_set_loc($$, lloc(@1, @6));
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
    : primary_expr '[' index_expr_list ']'
    {
        // Foo[1, 2]
        // Foo["hello"]
        // Foo[Bar]()
        // FOO[Bar?]()
        // Foo[Bar, Baz]()
        $$ = expr_from_index($1, $3);
        expr_set_loc($$, lloc(@1, @4));
    }
    | primary_expr '[' index_expr_list ']' '?'
    {

    }
    | primary_expr '[' error
    {
        expr_free($1);
        kl_error(loc(@3), "expected expr-list.");
        yy_clear_ok;
        $$ = NULL;
    }
    | primary_expr '[' index_expr_list error
    {
        // expr_free($1);
        // free_expr_list($3);
        kl_error(loc(@4), "expected ']'.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

index_expr_list
    : expr
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$1);
    }
    | slice_expr
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$1);
    }
    | index_expr_list ',' expr
    {
        $$ = $1;
        vector_push_back($$, &$3);
    }
    | index_expr_list ',' slice_expr
    {
        $$ = $1;
        vector_push_back($$, &$3);
    }
    ;

slice_expr
    : expr ':' expr ':' expr
    {
        $$ = expr_from_slice($1, $3, $5);
        expr_set_loc($$, lloc(@1, @5));
    }
    | ':' expr ':' expr
    {
        $$ = expr_from_slice(NULL, $2, $4);
        expr_set_loc($$, lloc(@1, @4));
    }
    | expr ':' ':' expr
    {
        $$ = expr_from_slice($1, NULL, $4);
        expr_set_loc($$, lloc(@1, @4));
    }
    | expr ':' expr ':'
    {
        $$ = expr_from_slice($1, $3, NULL);
        expr_set_loc($$, lloc(@1, @3));
    }
    | expr ':' expr
    {
        $$ = expr_from_slice($1, $3, NULL);
        expr_set_loc($$, lloc(@1, @3));
    }
    | ':' expr
    {
        $$ = expr_from_slice(NULL, $2, NULL);
        expr_set_loc($$, lloc(@1, @2));
    }
    | expr ':'
    {
        $$ = expr_from_slice($1, NULL, NULL);
        expr_set_loc($$, lloc(@1, @2));
    }
    | ':' ':' expr
    {
        $$ = expr_from_slice(NULL, NULL, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | ':' expr ':'
    {
        $$ = expr_from_slice(NULL, $2, NULL);
        expr_set_loc($$, lloc(@1, @3));
    }
    | expr ':' ':'
    {
        $$ = expr_from_slice($1, NULL, NULL);
        expr_set_loc($$, lloc(@1, @3));
    }
    | ':' ':'
    {
        $$ = expr_from_slice(NULL, NULL, NULL);
        expr_set_loc($$, loc(@1));
    }
    | ':'
    {
        $$ = expr_from_slice(NULL, NULL, NULL);
        expr_set_loc($$, loc(@1));
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
    | list_expr
    {
        $$ = $1;
    }
    | map_expr
    {
        $$ = NULL;
    }
    | tuple_expr
    {
        $$ = NULL;
    }
    | atom_type
    {
        $$ = expr_from_type($1);
        expr_set_loc($$, loc(@1));
    }
    | anony_expr
    {
        $$ = NULL;
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
            kl_error(loc(@1), "Number %s is out of int range", ps->sval);
            $$ = NULL;
            YYERROR;
        } else {
            $$ = expr_from_lit_int(ps->sval, $1, ps->sign, ps->bit_mode);
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
    | PANIC '(' expr ')'
    {
        $$ = expr_from_panic($3);
        expr_set_loc($$, lloc(@1, @4));
    }
    ;

list_expr
    : '[' expr_list ']'
    {
        // [1,2,3]
        $$ = expr_from_array($2);
        expr_set_loc($$, lloc(@1, @3));
    }
    | '[' expr_list semi ']'
    {

    }
    | '[' expr_list ',' ']'
    {

    }
    | '[' ']'
    {
        $$ = expr_from_array(NULL);
        expr_set_loc($$, lloc(@1, @2));
    }
    | LIST
    {
        TypeSpec *ty = klass_type_spec(NULL, "list");
        type_spec_loc(ty, loc(@1));
        $$ = expr_from_type(ty);
        expr_set_loc($$, loc(@1));
    }
    | SET
    {
        $$ = NULL;
    }
    | '[' error
    {
        kl_error(loc(@2), "expected expr-list.");
        yy_clear_ok;
        $$ = NULL;
    }
    | '[' expr_list error
    {
        free_expr_list($2);
        kl_error(loc(@3), "expected ',' or ']'.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

map_expr
    : '{' map_list '}'
    {
        $$ = expr_from_map($2);
        expr_set_loc($$, lloc(@1, @3));
    }
    | '{' ':' '}'
    {
        $$ = expr_from_map(NULL);
        expr_set_loc($$, lloc(@1, @3));
    }
    | '{' map_list semi '}'
    {

    }
    | '{' map_list ',' '}'
    {

    }
    | MAP
    {
        // Type *ty = map_type(NULL, NULL);
        // type_set_loc(ty, loc(@1));
        // $$ = expr_from_type(ty);
        // expr_set_loc($$, loc(@1));
    }
    | '{' error
    {
        kl_error(loc(@2), "expected key:value pairs.");
        yy_clear_ok;
        $$ = NULL;
    }
    | '{' map_list error
    {
        free_map_list($2);
        kl_error(loc(@3), "expected ',' or '}'.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

map_list
    : map
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$1);
    }
    | map_list ',' map
    {
        $$ = $1;
        vector_push_back($$, &$3);
    }
    | map_list ',' error
    {
        free_map_list($1);
        kl_error(loc(@3), "expected map key-value expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

map
    : expr ':' expr
    {
        $$ = expr_from_map_entry($1, $3);
        expr_set_loc($$, lloc(@1, @3));
    }
    | expr error
    {
        expr_free($1);
        kl_error(loc(@2), "expected ':'.");
        yy_clear_ok;
        $$ = NULL;
    }
    | expr ':' error
    {
        expr_free($1);
        kl_error(loc(@3), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    ;

tuple_expr
    : '(' expr_list ',' ')'
    {
        $$ = expr_from_tuple($2);
        expr_set_loc($$, lloc(@1, @4));
    }
    | '(' expr_list ',' expr ')'
    {
        // (1, "hello", [1,2,3])
        vector_push_back($2, &$4);
        $$ = expr_from_tuple($2);
        expr_set_loc($$, lloc(@1, @5));
    }
    | '(' ')'
    {
        printf("tuple ()\n");
    }
    | TUPLE
    {
        printf("tuple expr\n");
        // Type *ty = tuple_type(NULL);
        // type_set_loc(ty, loc(@1));
        // $$ = expr_from_type(ty);
        // expr_set_loc($$, loc(@1));
    }
    | '(' expr_list ',' error
    {
        free_expr_list($2);
        kl_error(loc(@4), "expected an expr.");
        yy_clear_ok;
        $$ = NULL;
    }
    | '(' expr_list ',' expr error
    {
        free_expr_list($2);
        expr_free($4);
        kl_error(loc(@5), "expected ')'.");
        yy_clear_ok;
        $$ = NULL;
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
    | FUNC error
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

%%

/* epilogue */
