/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

/* prologue */

%{

#include "parser.h"
#include "koala_yacc.h"

#define yyerror(loc, ps, scanner, msg) ((void)0)

#define row(loc) ((loc).first_line)
#define col(loc) ((loc).first_column)
#define col_last(loc) ((loc).last_column)
#define loc(l) (Loc){row(l), col(l)}

int keyword(int token)
{
    int keys[] = {
        PACKAGE, IMPORT, TYPE, LET, VAR, FUNC,
        IF, ELSE, WHILE, FOR, SWITCH, CLASS,
        INTERFACE, ENUM, RETURN, PUBLIC,
    };

    for (int i = 0; i < COUNT_OF(keys); i++) {
        if (keys[i] == token) return 1;
    }
    return 0;
}

#define yy_clearin_errok if (!keyword(ps->token)) yyclearin; yyerrok

%}

%union {
    int64_t ival;
    double fval;
    int cval;
    char *sval;
    AssignOpKind assignop;
    Expr *expr;
    Stmt *stmt;
    ExprType type;
    Vector *typelist;
    Vector *klasslist;
    Vector *type_param_list;
    Vector *list;
    Vector *exprlist;
    Vector *stmtlist;
    Ident_TypeParams id_typeparams;
    Ident_Type id_type;
    Vector *member_list;
}

%token PACKAGE
%token IMPORT
%token TYPE
%token LET
%token VAR
%token FUNC
%token CLASS
%token INTERFACE
%token ENUM
%token STRUCT
%token IF
%token ELSE
%token WHILE
%token FOR
%token SWITCH
%token CASE
%token BREAK
%token CONTINUE
%token RETURN
%token IN
%token AS
%token IS
%token PUBLIC
%token EXTERNC
%token INOUT
%token POINTER

%token SELF
%token SUPER
%token TRUE
%token FALSE
%token NULL_TK

%token UINT8
%token UINT16
%token UINT32
%token UINT64
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

%token SIZEOF

%token DOTDOTDOT
%token DOTDOTLESS

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
%token <cval> CHAR_LITERAL
%token <sval> STRING_LITERAL
%token <sval> ID

%type <stmt> type_alias_decl
%type <stmt> let_decl
%type <stmt> var_decl
%type <assignop> assignop
%type <stmt> assignment
%type <stmt> return_stmt
%type <stmt> jump_stmt
%type <list> block
%type <stmt> local
%type <stmtlist> local_list
%type <stmt> func_decl
%type <id_typeparams> name
%type <type_param_list> type_param_decl_list
%type <klasslist> klass_list
%type <klasslist> extends
%type <id_typeparams> type_param_decl
%type <list> param_list
%type <stmt> type_decl
%type <stmt> struct_decl
%type <member_list> struct_members
%type <id_typeparams> struct_name
%type <stmt> struct_member

%type <expr> expr
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
%type <expr> call_expr
%type <expr> atom_expr
%type <expr> dot_expr
%type <expr> index_expr
%type <expr> angle_expr
%type <expr> primitive_as_expr
%type <exprlist> expr_list

%type <type> type
%type <type> atom_type
%type <type> klass_type
%type <type> func_type
%type <typelist> type_list

/*
%destructor {
    printf("free expr auto\n");
    free_expr($$);
} <expr>
*/

%locations
%parse-param {ParserState *ps}
%parse-param {void *scanner}
%define api.pure full
//%define api.prefix {kl}
%lex-param {void *scanner}
%code provides {
    int yylex(YYSTYPE *yylval_param, YYLTYPE *yylloc, void *yyscanner);
}

%start program

/* grammar rules */

%%

program
    : package imports top_decls
    | imports top_decls
    | package top_decls
    | top_decls
    ;

imports
    : import_stmt
    | imports import_stmt
    ;

top_decls
    : top_decl
    | top_decls top_decl
    ;

package
    : PACKAGE ID ';'
    {
        parser_set_pkg_name(ps, $2);
    }
    | PACKAGE error
    {
        parser_error(loc(@2), "expected package name");
        yyerrok;
    }
    | PACKAGE ID error
    {
        parser_error(loc(@2), "expected ';'");
        yyerrok;
    }
    ;

top_decl
    : type_alias_decl
    {

    }
    | PUBLIC type_alias_decl
    {

    }
    | let_decl
    {
        parser_new_var(ps, $1);
        ((VarDeclStmt *)$1)->where = VAR_DECL_GLOBAL;
    }
    | PUBLIC let_decl
    {
        parser_new_var(ps, $2);
        ((VarDeclStmt *)$2)->where = VAR_DECL_GLOBAL;
        ((VarDeclStmt *)$2)->pub = 1;
    }
    | var_decl
    {
        parser_new_var(ps, $1);
        ((VarDeclStmt *)$1)->where = VAR_DECL_GLOBAL;
    }
    | PUBLIC var_decl
    {
        parser_new_var(ps, $2);
        ((VarDeclStmt *)$2)->where = VAR_DECL_GLOBAL;
        ((VarDeclStmt *)$2)->pub = 1;
    }
    | func_decl
    {
        //printf("func_decl in top\n");
        //parser_new_func(ps, $1);
    }
    | PUBLIC func_decl
    {

    }
    | type_decl
    {

    }
    | PUBLIC type_decl
    {

    }
    | at_expr_list type_decl
    {

    }
    | at_expr_list PUBLIC type_decl
    {
        printf("which?\n");
    }
    | extern_scope
    {
    }
    | ';'
    {
    }
    | error
    {
        parser_error(loc(@1), "illegal top definition");
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
        yy_clearin_errok;
    }
    | IMPORT STRING_LITERAL error
    {

    }
    | IMPORT ID error
    {

    }
    | IMPORT ID STRING_LITERAL error
    {

    }
    | IMPORT '.' error
    {

    }
    | IMPORT '.' STRING_LITERAL error
    {

    }
    | IMPORT '{' error
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
    | ID AS error
    {
    }
    | id_as_list ',' ID AS error
    {
    }
    ;

type_alias_decl
    : TYPE ID type ';'
    {

    }
    | TYPE error
    {

    }
    | TYPE ID error
    {

    }
    | TYPE ID type error
    {

    }
    ;

let_decl
    : LET ID '=' expr ';'
    {
        // printf("let-decl\n");
        Ident id = {$2, loc(@2), NULL};
        ExprType ty = {0};
        $$ = stmt_from_let_decl(id, ty, $4);
        $$->loc = loc(@1);
    }
    | LET ID type '=' expr ';'
    {
        Ident id = {$2, loc(@2), NULL};
        $$ = stmt_from_let_decl(id, $3, $5);
        $$->loc = loc(@1);
    }
    | LET error
    {
        parser_error(loc(@2), "expected identifier");
        yyerrok; $$ = NULL;
    }
    | LET ID error
    {
        parser_error(loc(@3), "expected TYPE or =");
        yyerrok; $$ = NULL;
    }
    | LET ID '=' error
    {
        parser_error(loc(@4), "expected expression");
        yyerrok; $$ = NULL;
    }
    | LET ID type error
    {
        parser_error(loc(@4), "expected =");
        yyerrok; $$ = NULL;
    }
    | LET ID type '=' error
    {
        parser_error(loc(@5), "expected expression");
        yyerrok; $$ = NULL;
    }
    | LET ID '=' expr error
    {
        parser_error(loc(@5), "expected ; or \\n");
        yyerrok; $$ = NULL;
    }
    | LET ID type '=' expr error
    {
        parser_error(loc(@6), "expected ; or \\n");
        yyerrok; $$ = NULL;
    }
    ;

var_decl
    : VAR ID type ';'
    {
        Ident id = {$2, loc(@2), NULL};
        $$ = stmt_from_var_decl(id, $3, NULL);
        $$->loc = loc(@1);
    }
    | VAR ID '=' expr ';'
    {
        Ident id = {$2, loc(@2), NULL};
        ExprType ty = {0};
        $$ = stmt_from_var_decl(id, ty, $4);
        $$->loc = loc(@1);
    }
    | VAR ID type '=' expr ';'
    {
        Ident id = {$2, loc(@2), NULL};
        $$ = stmt_from_var_decl(id, $3, $5);
        $$->loc = loc(@1);
    }
    | VAR error
    {
        parser_error(loc(@2), "expected identifier");
        yyerrok; $$ = NULL;
    }
    | VAR ID error
    {
        parser_error(loc(@3), "expected TYPE or =");
        yyerrok; $$ = NULL;
    }
    | VAR ID '=' error
    {
        parser_error(loc(@4), "expected expression");
        yyerrok; $$ = NULL;
    }
    | VAR ID type error
    {
        parser_error(loc(@4), "expected = or ;");
        yyerrok; $$ = NULL;
    }
    | VAR ID '=' expr error
    {
        parser_error(loc(@5), "expected ; or \\n");
        yyerrok; $$ = NULL;
    }
    | VAR ID type '=' error
    {
        parser_error(loc(@5), "expected expression");
        yyerrok; $$ = NULL;
    }
    | VAR ID type '=' expr error
    {
        parser_error(loc(@6), "expected ; or \\n");
        yyerrok; $$ = NULL;
    }
    ;

extern_scope
    : EXTERNC '{' extern_scope_decl_list '}' ';'
    | EXTERNC '{' extern_scope_decl_list ';' '}' ';'
    | EXTERNC error
    {
        printf("expected '{'\n");
    }
    | EXTERNC '{' error
    {
        printf("expected 'struct or function defined, or '}'\n");
    }
    | EXTERNC '{' extern_scope_decl_list error
    {
        printf("expected '}'\n");
    }
    | EXTERNC '{' extern_scope_decl_list '}' error
    {
        printf("expected '{2'\n");
    }
    ;

extern_scope_decl_list
    : extern_scope_decl
    | extern_scope_decl_list extern_scope_decl
    ;

extern_scope_decl
    : c_struct_decl
    | proto_decl
    | func_decl
    ;

c_struct_decl
    : STRUCT ID '{' c_struct_body_list '}' ';'
    | STRUCT ID '{' c_struct_body_list ';' '}' ';'
    | STRUCT error
    {
        printf("expected 'Ident'\n");
    }
    | STRUCT ID error
    {
        printf("expected struct's 1\n");
    }
    | STRUCT ID '{' error
    {
        printf("expected struct's body\n");
    }
    | STRUCT ID '{' c_struct_body_list error
    {
        printf("expected struct's 2\n");
    }
    | STRUCT ID '{' c_struct_body_list '}' error
    {
        printf("expected struct's 3\n");
    }
    ;

c_struct_body_list
    : c_struct_body
    | c_struct_body_list c_struct_body
    ;

c_struct_body
    : VAR ID type ';'
    {
        printf("var c_struct_body\n");
    }
    | VAR error
    {
        printf("expected c_struct_body\n");
    }
    | VAR ID error
    {
        printf("expected c_struct_body2\n");
    }
    | VAR ID type error
    {
        printf("expected c_struct_body3\n");
    }
    ;

assignment
    : primary_expr assignop expr ';'
    {
        //$$ = stmt_from_assign($2, loc(@2), $1, $3);
        //stmt_set_loc($$, loc(@1));
    }
    | primary_expr assignop error
    {
        // yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = NULL;
    }
    | primary_expr assignop expr error
    {
        // yy_errmsg(loc(@4), "expected ';' or '\\n'");
        yy_clearin_errok; $$ = NULL;
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
        $$ = OP_SHL_ASSIGN;
    }
    | SHR_ASSIGN
    {
        $$ = OP_SHR_ASSIGN;
    }
    ;

return_stmt
    : RETURN ';'
    {
        $$ = stmt_from_ret(NULL);
        $$->loc = loc(@1);
    }
    | RETURN expr ';'
    {
        $$ = stmt_from_ret($2);
        $$->loc = loc(@1);
    }
    | RETURN error
    {
        // yy_errmsg(loc(@2), "expected expression, ';' or '\\n'");
        yy_clearin_errok;
        $$ = NULL;
    }
    | RETURN expr error
    {
        // yy_errmsg(loc(@3), "expected ';' or '\\n'");
        yy_clearin_errok;
        $$ = NULL;
    }
    ;

jump_stmt
    : BREAK ';'
    {
        $$ = stmt_from_break();
        $$->loc = loc(@1);
    }
    | CONTINUE ';'
    {
        $$ = stmt_from_continue();
        $$->loc = loc(@1);
    }
    | BREAK error
    {
        // yy_errmsg(loc(@2), "expected ';' or '\\n'");
        yy_clearin_errok;
        $$ = NULL;
    }
    | CONTINUE error
    {
        // yy_errmsg(loc(@2), "expected ';' or '\\n'");
        yy_clearin_errok;
        $$ = NULL;
    }
    ;

block
    : '{' local_list '}'
    {
        $$ = $2;
    }
    | '{' expr '}'
    {
        Stmt *s = stmt_from_expr($2);
        s->loc = loc(@2);
        $$ = vector_create_ptr();
        vector_push_back($$, &s);
    }
    | '{' '}'
    {
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
        if ($2)  vector_push_back($$, &$2);
    }
    ;

local
    : expr ';'
    {
        $$ = stmt_from_expr($1);
        $$->loc = loc(@1);
    }
    | expr error
    {
        parser_error(loc(@2), "expected ; or \\n");
        yyerrok; $$ = NULL;
    }
    | let_decl
    {
        $$ = $1;
    }
    | var_decl
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
    | switch_stmt
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
        $$ = stmt_from_block($1);
        $$->loc = loc(@1);
    }
    | ';'
    {
        $$ = NULL;
    }
    ;

if_stmt
    : IF expr block elseif
    | IF error
    {
        printf("if error\n");
    }
    | IF expr error
    {
        printf("if expr error\n");
    }
    ;

elseif
    : %empty
    | ELSE block
    | ELSE if_stmt
    | ELSE error
    {
        printf("else error\n");
    }
    ;

while_stmt
    : WHILE expr block
    | WHILE block
    | WHILE error
    {
        printf("while error\n");
    }
    | WHILE expr error
    {
        printf("WHILE expr error\n");
    }
    ;

for_stmt
    : FOR expr IN expr block
    | FOR error
    {
        printf("for error\n");
    }
    | FOR expr IN error
    {
        printf("FOR expr IN error\n");
    }
    | FOR expr IN expr error
    {
        printf("FOR expr IN expr error\n");
    }
    ;

switch_stmt
    : SWITCH expr '{' switch_clauses '}'
    | SWITCH error
    {
        // yy_errmsg(loc(@2), "match expr error\n");
        yyerrok;
    }
    | SWITCH expr error
    {

    }
    | SWITCH expr '{' error
    {

    }
    | SWITCH expr '{' switch_clauses error
    {

    }
    ;

switch_clauses
    : switch_clause
    | switch_clauses switch_clause
    ;

switch_clause
    : CASE case_pattern_list ':' case_block case_tail
    | CASE error
    | CASE case_pattern_list error
    | CASE case_pattern_list ':' error
    ;

case_pattern_list
    : case_pattern
    | case_pattern_list ',' case_pattern
    ;

case_pattern
    : expr
    | IN range_expr
    | IS type
    | IN error
    | IS error
    ;

case_block
    : %empty
    | block
    | expr
    ;

case_tail
    : ';'
    | ','
    ;

func_decl
    : FUNC name '(' param_list ')' type block
    {
        printf("func decl\n");
        $$ = stmt_from_func_decl($2.id, $2.type_params, $4, &$6, $7);
        $$->loc = loc(@1);
    }
    | FUNC name '(' param_list ')' block
    {
        $$ = stmt_from_func_decl($2.id, $2.type_params, $4, NULL, $6);
        $$->loc = loc(@1);
    }
    | FUNC name '(' ')' type block
    {
        $$ = stmt_from_func_decl($2.id, $2.type_params, NULL, &$5, $6);
        $$->loc = loc(@1);
    }
    | FUNC name '(' ')' block
    {
        $$ = stmt_from_func_decl($2.id, $2.type_params, NULL, NULL, $5);
        $$->loc = loc(@1);
    }
    | FUNC error
    {
        // yy_errmsg(loc(@2), "expected identifier");
        yy_clearin_errok;
        $$ = NULL;
    }
    | FUNC name error
    {
        // yy_errmsg(loc(@3), "expected '('");
        yy_clearin_errok;
        $$ = NULL;
    }
    | FUNC name '(' error
    {
        // yy_errmsg(loc(@4), "expected declaration specifiers or ')'");
        yy_clearin_errok;
        $$ = NULL;
    }
    | FUNC name '(' param_list error
    {
        // yy_errmsg(loc(@5), "expected ')'");
        yy_clearin_errok;
        $$ = NULL;
    }
    | FUNC name '(' param_list ')' error
    {
        // yy_errmsg(loc(@6), "expected ';', 'TYPE' or 'BLOCK'");
        yy_clearin_errok;
        $$ = NULL;
    }
    | FUNC name '(' param_list ')' type error
    {
        // yy_errmsg(loc(@7), "expected ';' or 'BLOCK'");
        yy_clearin_errok;
        $$ = NULL;
    }
    | FUNC name '(' ')' error
    {
        // yy_errmsg(loc(@5), "expected ';', 'TYPE' or 'BLOCK'");
        yy_clearin_errok;
        $$ = NULL;
    }
    | FUNC name '(' ')' type error
    {
        // yy_errmsg(loc(@6), "expected ';' or 'BLOCK'");
        yy_clearin_errok;
        $$ = NULL;
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
    | FUNC name '(' type_list ')' type ';'
    {

    }
    | FUNC name '(' type_list ')' ';'
    {

    }
    | FUNC name '(' type_list error
    | FUNC name '(' type_list ')' error
    | FUNC name '(' type_list ')' type error
    ;

param_list
    : ID type
    {
        //IdentType idtype = {{$1, loc(@1), NULL}, $2};
        //$$ = vector_create(sizeof(IdentType));
        //vector_push_back($$, &idtype);
    }
    | ID INOUT type
    {

    }
    | param_list ',' ID type
    {
        $$ = $1;
    }
    | param_list ',' ID INOUT type
    {
        $$ = $1;
    }
    | param_list ',' error
    {

    }
    | param_list ',' ID error
    {

    }
    ;

type_decl
    : class_decl
    {
        $$ = NULL;
    }
    | interface_decl
    {
        $$ = NULL;
    }
    | enum_decl
    {
        $$ = NULL;
    }
    | struct_decl
    {
        $$ = $1;
    }
    ;

class_decl
    : CLASS name extends '{' class_members '}'
    | CLASS error
    | CLASS name error
    | CLASS name extends '{' error
    | CLASS name extends '{' class_members error
    ;

struct_decl
    : STRUCT struct_name extends '{' struct_members '}'
    {
        $$ = stmt_from_struct_decl($2.id, $2.type_params, $3, $5, 0);
    }
    | STRUCT error
    {
        parser_error(loc(@2), "expected identifer of structure");
        yyerrok;
        $$ = NULL;
    }
    | STRUCT struct_name error
    {
        parser_error(loc(@3), "expected ':' or '{'");
        yyerrok;
        $$ = NULL;
    }
    | STRUCT struct_name extends '{' error
    {
        parser_error(loc(@4), "expected fields or functions declaration in structure");
        yyerrok;
        $$ = NULL;
    }
    | STRUCT struct_name extends '{' struct_members error
    {
        parser_error(loc(@6), "expected '}' at end of structure");
        $$ = NULL;
    }
    ;

struct_name
    : name
    {
        $$ = $1;
    }
    | UINT8
    {
        Ident uint8_ident = {"UInt8", loc(@1), NULL};
        Ident_TypeParams id_tps = {uint8_ident, NULL, (Loc){0, 0}, 0};
        $$ = id_tps;
    }
    | UINT16
    {
        Ident uint8_ident = {"UInt8", loc(@1), NULL};
        Ident_TypeParams id_tps = {uint8_ident, NULL, (Loc){0, 0}, 0};
        $$ = id_tps;
    }
    | UINT32
    {
        Ident uint8_ident = {"UInt8", loc(@1), NULL};
        Ident_TypeParams id_tps = {uint8_ident, NULL, (Loc){0, 0}, 0};
        $$ = id_tps;
    }
    | UINT64
    {
        Ident uint8_ident = {"UInt8", loc(@1), NULL};
        Ident_TypeParams id_tps = {uint8_ident, NULL, (Loc){0, 0}, 0};
        $$ = id_tps;
    }
    | INT8
    {
        Ident uint8_ident = {"UInt8", loc(@1), NULL};
        Ident_TypeParams id_tps = {uint8_ident, NULL, (Loc){0, 0}, 0};
        $$ = id_tps;
    }
    | INT16
    {
        Ident uint8_ident = {"UInt8", loc(@1), NULL};
        Ident_TypeParams id_tps = {uint8_ident, NULL, (Loc){0, 0}, 0};
        $$ = id_tps;
    }
    | INT32
    {
        Ident uint8_ident = {"UInt8", loc(@1), NULL};
        Ident_TypeParams id_tps = {uint8_ident, NULL, (Loc){0, 0}, 0};
        $$ = id_tps;
    }
    | INT64
    {
        Ident uint8_ident = {"UInt8", loc(@1), NULL};
        Ident_TypeParams id_tps = {uint8_ident, NULL, (Loc){0, 0}, 0};
        $$ = id_tps;
    }
    | FLOAT32
    {
        Ident uint8_ident = {"UInt8", loc(@1), NULL};
        Ident_TypeParams id_tps = {uint8_ident, NULL, (Loc){0, 0}, 0};
        $$ = id_tps;
    }
    | FLOAT64
    {
        Ident uint8_ident = {"UInt8", loc(@1), NULL};
        Ident_TypeParams id_tps = {uint8_ident, NULL, (Loc){0, 0}, 0};
        $$ = id_tps;
    }
    | BOOL
    {
        Ident uint8_ident = {"UInt8", loc(@1), NULL};
        Ident_TypeParams id_tps = {uint8_ident, NULL, (Loc){0, 0}, 0};
        $$ = id_tps;
    }
    | CHAR
    {
        Ident uint8_ident = {"UInt8", loc(@1), NULL};
        Ident_TypeParams id_tps = {uint8_ident, NULL, (Loc){0, 0}, 0};
        $$ = id_tps;
    }
    | STRING
    {
        Ident uint8_ident = {"UInt8", loc(@1), NULL};
        Ident_TypeParams id_tps = {uint8_ident, NULL, (Loc){0, 0}, 0};
        $$ = id_tps;
    }
    | POINTER '<' ID '>'
    {
        Ident uint8_ident = {"UInt8", loc(@1), NULL};
        Ident_TypeParams id_tps = {uint8_ident, NULL, (Loc){0, 0}, 0};
        $$ = id_tps;
    }
    ;

struct_members
    : struct_member
    {
        $$ = vector_create_ptr();
        if ($1) vector_push_back($$, $1);
    }
    | struct_members struct_member
    {
        $$ = $1;
        if ($2) vector_push_back($$, $2);
    }
    ;

struct_member
    : var_decl
    {
        $$ = $1;
    }
    | PUBLIC var_decl
    {
        $$ = $2;
    }
    | let_decl
    {
        $$ = $1;
    }
    | PUBLIC let_decl
    {
        $$ = $2;
    }
    | func_decl
    {
        $$ = $1;
    }
    | PUBLIC func_decl
    {
        $$ = $2;
    }
    | ';'
    {
        $$ = NULL;
    }
    ;

interface_decl
    : INTERFACE name extends '{' interface_members '}'
    | INTERFACE error
    | INTERFACE name error
    | INTERFACE name extends '{' error
    | INTERFACE name extends '{' interface_members error
    ;

enum_decl
    : ENUM name extends '{' enum_members '}'
    | ENUM error
    | ENUM name error
    | ENUM name extends '{' error
    | ENUM name extends '{' enum_members error
    ;

name
    : ID
    {
        $$.id = (Ident){$1, loc(@1), NULL};
        $$.type_params = NULL;
        $$.loc = (Loc){0, 0};
        $$.error = 0;
    }
    | ID '<' type_param_decl_list '>'
    {
        $$.id = (Ident){$1, loc(@1), NULL};
        $$.type_params = $3;
        $$.loc = loc(@3);
        $$.error = 0;
    }
    | ID '<' error
    {
        $$.id = (Ident){$1, loc(@1), NULL};
        $$.type_params = NULL;
        $$.loc = loc(@3);
        $$.error = 1;
        parser_error(loc(@3), "illegal type parameter list");
        yyerrok;
    }
    ;

type_param_decl_list
    : type_param_decl
    {
        $$ = vector_create(sizeof(Ident_TypeParams));
        vector_push_back($$, &$1);
    }
    | type_param_decl_list ',' type_param_decl
    {
        $$ = $1;
        vector_push_back($$, &$3);
    }
    | type_param_decl_list ',' error
    {
        $$ = NULL;
        parser_error(loc(@3), "illegal type parameter");
        yyerrok;
    }
    ;

type_param_decl
    : ID
    {
        $$.id = (Ident){$1, loc(@1), NULL};
        $$.type_params = NULL;
        $$.loc = (Loc){0, 0};
        $$.error = 0;
    }
    | ID ':' klass_list
    {
        $$.id = (Ident){$1, loc(@1), NULL};
        $$.type_params = $3;
        $$.loc = loc(@3);
        $$.error = 0;
    }
    | ID error
    {
        $$.id = (Ident){$1, loc(@1), NULL};
        $$.type_params = NULL;
        $$.loc = loc(@2);
        $$.error = 1;
        parser_error(loc(@2), "expected type list");
        yyerrok;
    }
    ;

extends
    : %empty
    {
        $$ = NULL;
    }
    | ':' klass_list
    {
        $$ = $2;
    }
    | ':' error
    {
        $$ = NULL;
    }
    ;

klass_list
    : klass_type
    {
        $$ = vector_create(sizeof(ExprType));
        vector_push_back($$, &$1);
    }
    | klass_list '&' klass_type
    {
        $$ = $1;
        vector_push_back($$, &$3);
    }
    ;

class_members
    : class_member
    | at_expr_list class_member
    | class_members class_member
    | class_members at_expr_list class_member
    ;

at_expr_list
    : '@' ID ';'
    {
        printf("at-expr:%s\n", $2);
    }
    | at_expr_list '@' ID ';'
    ;

class_member
    : var_decl
    | PUBLIC var_decl
    | let_decl
    | PUBLIC let_decl
    | func_decl
    | PUBLIC func_decl
    | proto_decl
    | PUBLIC proto_decl
    | ';'
    ;

interface_members
    : interface_member
    | at_expr_list interface_member
    | interface_members interface_member
    | interface_members at_expr_list interface_member
    ;

interface_member
    : proto_decl
    | PUBLIC proto_decl
    | ';'
    {

    }
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
    | ID error
    | ID '(' error
    | ID '(' type_list error
    ;

enum_methods
    : enum_method
    {
    }
    | enum_methods enum_method
    {
    }
    ;

enum_method
    : func_decl
    | PUBLIC func_decl
    | at_expr_list func_decl
    | at_expr_list PUBLIC func_decl
    | ';'
    ;

expr
    : or_expr
    {
        $$ = $1;
    }
    | is_expr
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

range_expr
    : or_expr DOTDOTDOT or_expr
    {
        $$ = expr_from_range($1, RANGE_DOTDOTDOT, loc(@2), $3);
        $$->loc = loc(@1);
    }
    | or_expr DOTDOTLESS or_expr
    {
        $$ = expr_from_range($1, RANGE_DOTDOTLESS, loc(@2), $3);
        $$->loc = loc(@1);
    }
    | or_expr DOTDOTDOT error
    {
        parser_error(loc(@3), "expected expression");
        yyerrok;
        $$ = NULL;
    }
    | or_expr DOTDOTLESS error
    {
        parser_error(loc(@3), "expected expression");
        yyerrok;
        $$ = NULL;
    }
    ;

is_expr
    : or_expr IS type
    {
        $$ = expr_from_is($1, $3);
        $$->loc = loc(@1);
    }
    | or_expr IS error {
        parser_error(loc(@3), "expected 'TYPE'");
        yyerrok;
        $$ = NULL;
    }
    ;

as_expr
    : or_expr AS type
    {
        $$ = expr_from_as($1, $3);
        $$->loc = loc(@1);
    }
    | or_expr AS error {
        parser_error(loc(@3), "expected 'TYPE'");
        yyerrok;
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
        $$->loc = loc(@1);
    }
    | or_expr OR error
    {
        parser_error(loc(@3), "expected expression");
        yyerrok;
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
        $$->loc = loc(@1);;
    }
    | and_expr AND error
    {
        parser_error(loc(@3), "expected expression");
        yyerrok;
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
        $$->loc = loc(@1);;
    }
    | bit_or_expr '|' error
    {
        parser_error(loc(@3), "expected expression");
        yyerrok;
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
        $$->loc = loc(@1);;
    }
    | bit_xor_expr '^' error
    {
        parser_error(loc(@3), "expected expression");
        yyerrok;
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
        $$->loc = loc(@1);;
    }
    | bit_and_expr '&' error
    {
        parser_error(loc(@3), "expected expression");
        yyerrok;
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
        $$->loc = loc(@1);
    }
    | equality_expr NE relation_expr
    {
        $$ = expr_from_binary(BINARY_NE, loc(@2), $1, $3);
        $$->loc = loc(@1);
    }
    | equality_expr EQ error
    {
        parser_error(loc(@3), "expected expression");
        yyerrok;
        $$ = NULL;
    }
    | equality_expr NE error
    {
        parser_error(loc(@3), "expected expression");
        yyerrok;
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
        $$->loc = loc(@1);;
    }
    | relation_expr '>' shift_expr
    {
        $$ = expr_from_binary(BINARY_GT, loc(@2), $1, $3);
        $$->loc = loc(@1);;
    }
    | relation_expr LE shift_expr
    {
        $$ = expr_from_binary(BINARY_LE, loc(@2), $1, $3);
        $$->loc = loc(@1);;
    }
    | relation_expr GE shift_expr
    {
        $$ = expr_from_binary(BINARY_GE, loc(@2), $1, $3);
        $$->loc = loc(@1);;
    }
    | relation_expr '<' error
    {
        parser_error(loc(@3), "expected expression");
        yyerrok;
        $$ = NULL;
    }
    | relation_expr '>' error
    {
        parser_error(loc(@3), "expected expression");
        yyerrok;
        $$ = NULL;
    }
    | relation_expr LE error
    {
        parser_error(loc(@3), "expected expression");
        yyerrok;
        $$ = NULL;
    }
    | relation_expr GE error
    {
        parser_error(loc(@3), "expected expression");
        yyerrok;
        $$ = NULL;
    }
    ;

shift_expr
    : add_expr
    {
        $$ = $1;
    }
    | shift_expr R_ANGLE_SHIFT '>' add_expr
    {
        $$ = expr_from_binary(BINARY_SHR, loc(@2), $1, $4);
        $$->loc = loc(@1);;
    }
    | shift_expr L_SHIFT add_expr
    {
        $$ = expr_from_binary(BINARY_SHL, loc(@2), $1, $3);
        $$->loc = loc(@1);;
    }
    | shift_expr R_ANGLE_SHIFT '>' error
    {
        parser_error(loc(@4), "expected expression");
        yyerrok;
        $$ = NULL;
    }
    | shift_expr L_SHIFT error
    {
        parser_error(loc(@3), "expected expression");
        yyerrok;
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
        $$->loc = loc(@1);;
    }
    | add_expr '-' multi_expr
    {
        $$ = expr_from_binary(BINARY_SUB, loc(@2), $1, $3);
        $$->loc = loc(@1);;
    }
    | add_expr '+' error
    {
        parser_error(loc(@3), "expected expression");
        yyerrok;
        $$ = NULL;
    }
    | add_expr '-' error
    {
        parser_error(loc(@3), "expected expression");
        yyerrok;
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
        $$ = expr_from_binary(BINARY_MULT, loc(@2), $1, $3);
        $$->loc = loc(@1);
    }
    | multi_expr '/' unary_expr
    {
        $$ = expr_from_binary(BINARY_DIV, loc(@2), $1, $3);
        $$->loc = loc(@1);;
    }
    | multi_expr '%' unary_expr
    {
        $$ = expr_from_binary(BINARY_MOD, loc(@2), $1, $3);
        $$->loc = loc(@1);
    }
    | multi_expr '*' error
    {
        parser_error(loc(@3), "expected expression");
        yyerrok;
        $$ = NULL;
    }
     | multi_expr '/' error
    {
        parser_error(loc(@3), "expected expression");
        yyerrok;
        $$ = NULL;
    }
     | multi_expr '%' error
    {
        parser_error(loc(@3), "expected expression");
        yyerrok;
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
        $$->loc = loc(@1);
    }
    | '-' unary_expr
    {
        $$ = expr_from_unary(UNARY_NEG, loc(@1), $2);
        $$->loc = loc(@1);
    }
    | '~' unary_expr
    {
        $$ = expr_from_unary(UNARY_BIT_NOT, loc(@1), $2);
        $$->loc = loc(@1);
    }
    | NOT unary_expr
    {
        $$ = expr_from_unary(UNARY_NOT, loc(@1), $2);
        $$->loc = loc(@1);
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
    | angle_expr
    {
        $$ = $1;
    }
    | atom_expr
    {
        $$ = $1;
    }
    ;

call_expr
    : primary_expr '(' ')'
    {
        $$ = expr_from_call($1, NULL);
        $$->loc = loc(@1);
    }
    | primary_expr '(' expr_list ')'
    {
        $$ = expr_from_call($1, $3);
        $$->loc = loc(@1);
    }
    | primary_expr '(' expr_list ';' ')'
    {
        $$ = expr_from_call($1, $3);
        $$->loc = loc(@1);
    }
    | primary_expr '(' error
    {
        parser_error(loc(@3), "expected arguments or ')'");
        yyerrok;
        $$ = NULL;
    }
    | primary_expr '(' expr_list error
    {
        parser_error(loc(@4), "expected ')'");
        yyerrok;
        $$ = NULL;
    }
    ;

dot_expr
    : primary_expr '.' ID
    {
        Ident id = {$3, loc(@3), NULL};
        Expr *idexp = expr_from_ident(id);
        idexp->loc = loc(@3);
        $$ = expr_from_attr($1, idexp);
        $$->loc = loc(@1);
    }
    | primary_expr '.' INT_LITERAL
    {
        $$ = expr_from_tuple_access($1, $3);
        $$->loc = loc(@1);
    }
    | primary_expr '.' error
    {
        parser_error(loc(@3), "expected identifer or integer index");
        yyerrok;
        $$ = NULL;
    }
    ;

index_expr
    : primary_expr '[' expr ']'
    {
        $$ = expr_from_index($1, $3);
        $$->loc = loc(@1);
    }
    | primary_expr '[' error
    {
        parser_error(loc(@3), "expected expression");
        yyerrok;
        $$ = NULL;
    }
    | primary_expr '[' expr error
    {
        parser_error(loc(@4), "expected ']'");
        yyerrok;
        $$ = NULL;
    }
    ;

angle_expr
    : primary_expr L_ANGLE_ARGS type_list r_angle
    {
        $$ = expr_from_type_params($1, $3);
        $$->loc = loc(@1);
    }
    | primary_expr L_ANGLE_ARGS error
    {
        parser_error(loc(@3), "expected TYPE-LIST");
        yyerrok;
        $$ = NULL;
    }
    | primary_expr L_ANGLE_ARGS type_list error
    {
        parser_error(loc(@3), "expected '>'");
        yyerrok;
        $$ = NULL;
    }
    ;

atom_expr
    : ID
    {
        Ident id = {$1, loc(@1), NULL};
        $$ = expr_from_ident(id);
        $$->loc = loc(@1);
    }
    | '_'
    {
        $$ = expr_from_under();
        $$->loc = loc(@1);
    }
    | INT_LITERAL
    {
        $$ = expr_from_int($1);
        $$->loc = loc(@1);
    }
    | FLOAT_LITERAL
    {
        $$ = expr_from_float($1);
        $$->loc = loc(@1);
    }
    | CHAR_LITERAL
    {
        $$ = expr_from_char($1);
        $$->loc = loc(@1);
    }
    | STRING_LITERAL
    {
        $$ = expr_from_str($1);
        $$->loc = loc(@1);
    }
    | TRUE
    {
        $$ = expr_from_bool(1);
        $$->loc = loc(@1);
    }
    | FALSE
    {
        $$ = expr_from_bool(0);
        $$->loc = loc(@1);
    }
    | NULL_TK
    {
        $$ = expr_from_null();
        $$->loc = loc(@1);
    }
    | SELF
    {
        $$ = expr_from_self();
        $$->loc = loc(@1);
    }
    | SUPER
    {
        $$ = expr_from_super();
        $$->loc = loc(@1);
    }
    | POINTER
    {
        $$ = expr_from_pointer();
        $$->loc = loc(@1);
        ps->in_angle = 1;
    }
    | SIZEOF '(' type ')'
    {
        $$ = expr_from_sizeof($3);
        $$->loc = loc(@1);
    }
    | SIZEOF error
    {
        parser_error(loc(@2), "expected '('");
        yyerrok;
        $$ = NULL;
    }
    | SIZEOF '(' error
    {
        parser_error(loc(@3), "expected TYPE");
        yyerrok;
        $$ = NULL;
    }
    | SIZEOF '(' type error
    {
        parser_error(loc(@4), "expected ')'");
        yyerrok;
        $$ = NULL;
    }
    | '(' expr ')'
    {
        $$ = $2;
    }
    | '(' error
    {
        parser_error(loc(@2), "expected expression");
        yyerrok;
        $$ = NULL;
    }
    | '(' expr error
    {
        parser_error(loc(@3), "expected ')'");
        yyerrok;
        $$ = NULL;
    }
    | array_object_expr
    {
        assert(0);
    }
    | map_object_expr
    {
        assert(0);
    }
    | tuple_object_expr
    {
        assert(0);
    }
    | anony_object_expr
    {
        assert(0);
    }
    | primitive_as_expr
    {
        $$ = $1;
    }
    ;

primitive_as_expr
    : INT8 '(' expr ')'
    {
        ExprType ty = { 0 };
        ty.loc = loc(@1);
        ty.ty = desc_from_int8();
        $$ = expr_from_as($3, ty);
        $$->loc = loc(@1);
    }
    | INT16 '(' expr ')'
    {
        ExprType ty = { 0 };
        ty.loc = loc(@1);
        ty.ty = desc_from_int16();
        $$ = expr_from_as($3, ty);
        $$->loc = loc(@1);
    }
    | INT32 '(' expr ')'
    {
        ExprType ty = { 0 };
        ty.loc = loc(@1);
        ty.ty = desc_from_int32();
        $$ = expr_from_as($3, ty);
        $$->loc = loc(@1);
    }
    | INT64 '(' expr ')'
    {
        ExprType ty = { 0 };
        ty.loc = loc(@1);
        ty.ty = desc_from_int64();
        $$ = expr_from_as($3, ty);
        $$->loc = loc(@1);
    }
    | UINT8 '(' expr ')'
    {
        ExprType ty = { 0 };
        ty.loc = loc(@1);
        ty.ty = desc_from_uint8();
        $$ = expr_from_as($3, ty);
        $$->loc = loc(@1);
    }
    | UINT16 '(' expr ')'
    {
        ExprType ty = { 0 };
        ty.loc = loc(@1);
        ty.ty = desc_from_uint16();
        $$ = expr_from_as($3, ty);
        $$->loc = loc(@1);
    }
    | UINT32 '(' expr ')'
    {
        ExprType ty = { 0 };
        ty.loc = loc(@1);
        ty.ty = desc_from_uint32();
        $$ = expr_from_as($3, ty);
        $$->loc = loc(@1);
    }
    | UINT64 '(' expr ')'
    {
        ExprType ty = { 0 };
        ty.loc = loc(@1);
        ty.ty = desc_from_uint64();
        $$ = expr_from_as($3, ty);
        $$->loc = loc(@1);
    }
    | FLOAT32 '(' expr ')'
    {
        ExprType ty = { 0 };
        ty.loc = loc(@1);
        ty.ty = desc_from_float32();
        $$ = expr_from_as($3, ty);
        $$->loc = loc(@1);
    }
    | FLOAT64 '(' expr ')'
    {
        ExprType ty = { 0 };
        ty.loc = loc(@1);
        ty.ty = desc_from_float64();
        $$ = expr_from_as($3, ty);
        $$->loc = loc(@1);
    }
    | STRING '(' expr ')'
    {
        ExprType ty = { 0 };
        ty.loc = loc(@1);
        ty.ty = desc_from_str();
        $$ = expr_from_as($3, ty);
        $$->loc = loc(@1);
    }
    | CHAR '(' expr ')'
    {
        ExprType ty = { 0 };
        ty.loc = loc(@1);
        ty.ty = desc_from_char();
        $$ = expr_from_as($3, ty);
        $$->loc = loc(@1);
    }
    | BOOL '(' expr ')'
    {
        ExprType ty = { 0 };
        ty.loc = loc(@1);
        ty.ty = desc_from_bool();
        $$ = expr_from_as($3, ty);
        $$->loc = loc(@1);
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
    | '[' error
    {

    }
    | '[' expr_list error
    {

    }
    | '[' expr_list ',' error
    | '[' expr_list ';' error
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
    | '{' error
    {
        printf("map object expr error\n");
    }
    | '{' mapentry_list error
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
    | expr ':' error
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
    | FUNC '(' error
    {

    }
    | FUNC '(' param_list error
    {

    }
    | FUNC '(' ')' error
    {

    }
    | FUNC '(' param_list ')' error
    {

    }
    | FUNC '(' ')' type error
    {

    }
    | FUNC '(' param_list ')' type error
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

type
    : atom_type
    {
        $$ = $1;
    }
    | '[' type ';' INT_LITERAL ']'
    {
        $$.loc = (Loc){0, 0};
        $$.ty = desc_from_unk();
    }
    | '[' type ']'
    {
        $$.loc = loc(@1);
        $$.ty = desc_from_array($2.ty);
        $$.loc2 = loc(@2);
    }
    | '[' type ':' type ']'
    {
        $$.loc = loc(@1);
        $$.ty = desc_from_map($2.ty, $4.ty);
        $$.loc2 = loc(@2);
        $$.loc3 = loc(@4);
    }
    | '(' type_list ')'
    {
        $$.loc = (Loc){0, 0};
        $$.ty = desc_from_unk();
    }
    | '[' error
    {
        $$.loc = (Loc){0, 0};
        $$.ty = desc_from_unk();
    }
    | '[' type error
    {
        $$.loc = (Loc){0, 0};
        $$.ty = desc_from_unk();
    }
    | '[' type ':' error
    {
        $$.loc = (Loc){0, 0};
        $$.ty = desc_from_unk();
    }
    | '[' type ':' type error
    {
        $$.loc = (Loc){0, 0};
        $$.ty = desc_from_unk();
    }
    | '(' error
    {
        $$.loc = (Loc){0, 0};
        $$.ty = desc_from_unk();
    }
    | '(' type_list error
    {
        $$.loc = (Loc){0, 0};
        $$.ty = desc_from_unk();
    }
    ;

atom_type
    : INT8
    {
        $$.loc = loc(@1);
        $$.ty = desc_from_int8();
    }
    | INT16
    {
        $$.loc = loc(@1);
        $$.ty = desc_from_int16();
    }
    | INT32
    {
        $$.loc = loc(@1);
        $$.ty = desc_from_int32();
    }
    | INT64
    {
        $$.loc = loc(@1);
        $$.ty = desc_from_int64();
    }
    | UINT8
    {
        $$.loc = loc(@1);
        $$.ty = desc_from_uint8();
    }
    | UINT16
    {
        $$.loc = loc(@1);
        $$.ty = desc_from_uint16();
    }
    | UINT32
    {
        $$.loc = loc(@1);
        $$.ty = desc_from_uint32();
    }
    | UINT64
    {
        $$.loc = loc(@1);
        $$.ty = desc_from_uint64();
    }
    | FLOAT32
    {
        $$.loc = loc(@1);
        $$.ty = desc_from_float32();
    }
    | FLOAT64
    {
        $$.loc = loc(@1);
        $$.ty = desc_from_float64();
    }
    | BOOL
    {
        $$.loc = loc(@1);
        $$.ty = desc_from_bool();
    }
    | CHAR
    {
        $$.loc = loc(@1);
        $$.ty = desc_from_char();
    }
    | STRING
    {
        $$.loc = loc(@1);
        $$.ty = desc_from_str();
    }
    | ANY
    {
        $$.loc = loc(@1);
        $$.ty = desc_from_any();
    }
    | klass_type
    {
        $$ = $1;
    }
    | func_type
    {
        $$ = $1;
    }
    | POINTER '<' type '>'
    {
        printf("pointer\n");
        $$.loc = loc(@1);
        $$.ty = desc_from_ptr($3.ty);
        $$.loc2 = loc(@3);
    }
    ;

klass_type
    : ID
    {
        $$.loc = loc(@1);
        $$.ty = desc_from_klass(NULL, $1, NULL);
    }
    | ID '.' ID
    {
        $$.loc = loc(@1);
        $$.ty = desc_from_klass($1, $3, NULL);
        $$.loc2 = loc(@3);
    }
    | ID l_angle type_list r_angle
    {
        $$.loc = loc(@1);
        $$.ty = desc_from_klass(NULL, $1, $3);
        $$.loc2 = loc(@3);
    }
    | ID '.' ID l_angle type_list r_angle
    {
        $$.loc = loc(@1);
        $$.ty = desc_from_klass($1, $3, $5);
        $$.loc2 = loc(@3);
        $$.loc3 = loc(@5);
    }
    | ID '.' error
    {
        $$.loc = loc(@3);
        $$.ty = desc_from_unk();
        parser_error(loc(@3), "expected identifier");
        yyerrok;
    }
    | ID l_angle error
    {
        $$.loc = loc(@3);
        $$.ty = desc_from_unk();
        parser_error(loc(@3), "expected type list");
        yyerrok;
    }
    | ID l_angle type_list error
    {
        $$.loc = loc(@4);
        $$.ty = desc_from_unk();
        parser_error(loc(@4), "expected >");
        yyerrok;
    }
    | ID '.' ID l_angle error
    {
        $$.loc = loc(@5);
        $$.ty = desc_from_unk();
        parser_error(loc(@5), "expected type list");
        yyerrok;
    }
    | ID '.' ID l_angle type_list error
    {
        $$.loc = loc(@6);
        $$.ty = desc_from_unk();
        parser_error(loc(@6), "expected >");
        yyerrok;
    }
    ;

r_angle
    : '>'
    | R_ANGLE_SHIFT
    ;

l_angle
    : '<'
    | L_ANGLE_ARGS
    ;

func_type
    : FUNC '(' type_list ')' type
    {
        $$.loc = (Loc){0, 0};
        $$.ty = desc_from_unk();
    }
    | FUNC '(' type_list ')'
    {
        $$.loc = (Loc){0, 0};
        $$.ty = desc_from_unk();
    }
    | FUNC '(' ')' type
    {
        $$.loc = (Loc){0, 0};
        $$.ty = desc_from_unk();
    }
    | FUNC '(' ')'
    {
        $$.loc = (Loc){0, 0};
        $$.ty = desc_from_unk();
    }
    | FUNC '(' param_list ')' type
    {
        $$.loc = (Loc){0, 0};
        $$.ty = desc_from_unk();
    }
    | FUNC '(' param_list ')'
    {
        $$.loc = (Loc){0, 0};
        $$.ty = desc_from_unk();
    }
    | FUNC error
    {
        $$.loc = (Loc){0, 0};
        $$.ty = desc_from_unk();
    }
    | FUNC '(' error
    {
        $$.loc = (Loc){0, 0};
        $$.ty = desc_from_unk();
    }
    | FUNC '(' type_list error
    {
        $$.loc = (Loc){0, 0};
        $$.ty = desc_from_unk();
    }
    | FUNC '(' param_list error
    {
        $$.loc = (Loc){0, 0};
        $$.ty = desc_from_unk();
    }
    ;

type_list
    : type
    {
        $$ = vector_create(sizeof(ExprType));
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
