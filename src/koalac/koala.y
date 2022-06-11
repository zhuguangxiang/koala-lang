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
%type <klasslist> type_param_bounds
%type <klasslist> klass_list
%type <klasslist> extends
%type <id_typeparams> type_param_decl
%type <list> param_list

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
%type <expr> slice_expr
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

    }
    | PACKAGE error
    {

    }
    | PACKAGE ID error
    {

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
        //parser_new_var(ps, $1);
        //set_var_decl_where($1, VAR_DECL_GLOBAL);
    }
    | PUBLIC let_decl
    {

    }
    | var_decl
    {
        //parser_new_var(ps, $1);
        //set_var_decl_where($1, VAR_DECL_GLOBAL);
    }
    | PUBLIC var_decl
    {

    }
    | func_decl
    {
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

    }
    | extern_scope
    {
    }
    | ';'
    {
    }
    | error
    {
        // // yy_errmsg(loc(@1), "syntax error");
        printf("syntax error\n");
        yyclearin; yyerrok;
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
        Ident id = {$2, loc(@2), NULL};
        ExprType ty;
        ty.loc = (Loc){0, 0};
        ty.ty = desc_from_unk();
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
        printf("error let-1\n");
        // yy_errmsg(loc(@2), "expected identifier");
        yy_clearin_errok;
        $$ = NULL;
    }
    | LET ID error
    {
        printf("error let\n");
        // yy_errmsg(loc(@3), "expected 'TYPE' or '='");
        yy_clearin_errok;
        $$ = NULL;
    }
    | LET ID '=' error
    {
        // yy_errmsg(loc(@4), "expected expression");
        yy_clearin_errok; $$ = NULL;
    }
    | LET ID type error
    {
        printf("error let2:%d\n", $3.ty->kind);
        // yy_errmsg(loc(@4), "expected '='");
        yy_clearin_errok; $$ = NULL;
    }
    | LET ID type '=' error
    {
        // yy_errmsg(loc(@5), "expected expression");
        yy_clearin_errok; $$ = NULL;
    }
    | LET ID '=' expr error
    {
        // yy_errmsg(loc(@5), "expected ';' or '\\n'");
        yy_clearin_errok; $$ = NULL;
    }
    | LET ID type '=' expr error
    {
        // yy_errmsg(loc(@6), "expected ';' or '\\n'");
        yy_clearin_errok; $$ = NULL;
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
        ExprType ty;
        ty.loc = (Loc){0, 0};
        ty.ty = desc_from_unk();
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
        // yy_errmsg(loc(@2), "expected identifier");
        yy_clearin_errok; $$ = NULL;
    }
    | VAR ID error
    {
        // yy_errmsg(loc(@3), "expected 'TYPE' or '='");
        yy_clearin_errok; $$ = NULL;
    }
    | VAR ID '=' error
    {
        // yy_errmsg(loc(@4), "expected expression");
        yy_clearin_errok; $$ = NULL;
    }
    | VAR ID type error
    {
        printf("var decl error3, %c\n", yychar);
        yy_clearin_errok; $$ = NULL;
    }
    | VAR ID '=' expr error
    {
        // yy_errmsg(loc(@5), "expected ';' or '\\n'");
        yy_clearin_errok; $$ = NULL;
    }
    | VAR ID type '=' error
    {
        // yy_errmsg(loc(@5), "expected expression");
        yy_clearin_errok; $$ = NULL;
    }
    | VAR ID type '=' expr error
    {
        // yy_errmsg(loc(@6), "expected ';' or '\\n'");
        yy_clearin_errok; $$ = NULL;
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
        vector_push_back($$, &$1);
    }
    | local_list local
    {
        $$ = $1;
        vector_push_back($$, &$2);
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
        // yy_errmsg(loc(@2), "expected ';' or '\\n'");
        yy_clearin_errok;
        $$ = NULL;
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
    | interface_decl
    | enum_decl
    | struct_decl
    ;

class_decl
    : CLASS name extends '{' class_members '}'
    | CLASS name extends '{' '}'
    | CLASS name extends ';'
    | CLASS error
    | CLASS name error
    | CLASS name extends '{' error
    | CLASS name extends '{' class_members error
    ;

struct_decl
    : STRUCT name extends '{' class_members '}'
    | STRUCT name extends '{' '}'
    | STRUCT name extends ';'
    | STRUCT error
    | STRUCT name error
    | STRUCT name extends '{' error
    | STRUCT name extends '{' class_members error
    ;

interface_decl
    : INTERFACE name extends '{' interface_members '}'
    | INTERFACE name extends '{' '}'
    | INTERFACE name extends ';'
    | INTERFACE error
    | INTERFACE name error
    | INTERFACE name extends '{' error
    | INTERFACE name extends '{' interface_members error
    ;

enum_decl
    : ENUM name extends '{' enum_members '}'
    | ENUM name extends ';'
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
    | ID ':' type_param_bounds
    {
        $$.id = (Ident){$1, loc(@1), NULL};
        $$.type_params = $3;
        $$.loc = loc(@3);
        $$.error = 0;
    }
    | ID error
    {
        // yy_errmsg(loc(@2), "expected ':'");
        yy_clearin_errok;
        $$.id = (Ident){$1, loc(@1), NULL};
        $$.type_params = NULL;
        $$.loc = loc(@2);
        $$.error = 1;
    }
    ;

type_param_bounds
    : klass_type
    {
        $$ = vector_create(sizeof(ExprType));
        vector_push_back($$, &$1);
    }
    | type_param_bounds '&' klass_type
    {
        $$ = $1;
        vector_push_back($$, &$3);
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
    | klass_list ',' klass_type
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
    : '@' ID
    | at_expr_list '@' ID
    ;

class_member
    : var_decl
    | PUBLIC var_decl
    | func_decl
    | PUBLIC func_decl
    | proto_decl
    | PUBLIC proto_decl
    | ';'
    ;

interface_members
    : interface_member
    | interface_members interface_member
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
    : func_decl ';'
    {
    }
    | enum_methods func_decl ';'
    {
    }
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
        $$ = NULL;
        // $$ = expr_from_range();
        // expr_set_loc($$, loc(@1));
    }
    | or_expr DOTDOTLESS or_expr
    {
        $$ = NULL;
        // $$ = expr_from_range();
        // expr_set_loc($$, loc(@1));
    }
    | or_expr DOTDOTDOT error
    {
        // yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok;
        $$ = NULL;
    }
    | or_expr DOTDOTLESS error
    {
        // yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok;
        $$ = NULL;
    }
    ;

is_expr
    : primary_expr IS type
    {
        $$ = NULL;
        //$$ = expr_from_is($1, $3);
        //expr_set_loc($$, loc(@1));
    }
    | primary_expr IS error {
        // yy_errmsg(loc(@3), "expected 'TYPE'");
        yy_clearin_errok;
        $$ = NULL;
    }
    ;

as_expr
    : primary_expr AS type
    {
        $$ = NULL;
        //$$ = expr_from_as($1, $3);
        //expr_set_loc($$, loc(@1));
    }
    | primary_expr AS error {
        // yy_errmsg(loc(@3), "expected 'TYPE'");
        yy_clearin_errok;
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
        // yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok;
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
        // yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok;
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
        // yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok;
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
        // yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok;
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
        // yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok;
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
        // yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok;
        $$ = NULL;
    }
    | equality_expr NE error
    {
        // yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok;
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
        // yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = NULL;
    }
    | relation_expr '>' error
    {
        printf("HERE?\n");
        // yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok;
        $$ = NULL;
    }
    | relation_expr LE error
    {
        // yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok;
        $$ = NULL;
    }
    | relation_expr GE error
    {
        // yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok;
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
        // yy_errmsg(loc(@4), "expected expression");
        yy_clearin_errok;
        $$ = NULL;
    }
    | shift_expr L_SHIFT error
    {
        // yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok;
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
        // yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok;
        $$ = NULL;
    }
    | add_expr '-' error
    {
        // yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok;
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
        // yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok;
        $$ = NULL;
    }
     | multi_expr '/' error
    {
        // yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok;
        $$ = NULL;
    }
     | multi_expr '%' error
    {
        // yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok;
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
        //$$ = expr_from_call();
    }
    | primary_expr '(' expr_list ')'
    | primary_expr '(' expr_list ';' ')'
    | primary_expr '(' error
    | primary_expr '(' expr_list error
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
    | primary_expr '.' error
    {
        // yy_errmsg(loc(@3),
            // "expected 'ID' or 'index' before '%c' token", yychar);
        yyclearin;
        yyerrok;
        $$ = NULL;
    }
    ;

index_expr
    : primary_expr '[' expr ']'
    | primary_expr '[' slice_expr ']'
    ;

slice_expr
    : expr ':' expr
    {

    }
    | ':' expr
    {

    }
    | expr ':'
    {

    }
    | ':'
    {

    }
    | ':' error
    {

    }
    | expr ':' error
    {

    }
    | error
    {
        // yy_errmsg(loc(@1), "expected slice expression");
        yy_clearin_errok; $$ = NULL;
    }
    ;

angle_expr
    : primary_expr L_ANGLE_ARGS type_list r_angle
    {

    }
    ;

atom_expr
    : ID
    {
        //printf("ID expr\n");
        //Ident id = {$1, loc(@1), NULL};
        //$$ = expr_from_ident(&id, NULL);
       // expr_set_loc($$, loc(@1));
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
    | INT8 '(' INT_LITERAL ')'
    {
        $$ = expr_from_int8($3);
        $$->loc = loc(@1);
    }
    | INT16 '(' INT_LITERAL ')'
    {
        $$ = expr_from_int16($3);
        $$->loc = loc(@1);
    }
    | INT32 '(' INT_LITERAL ')'
    {
        $$ = expr_from_int32($3);
        $$->loc = loc(@1);
    }
    | INT64 '(' INT_LITERAL ')'
    {
        $$ = expr_from_int64($3);
        $$->loc = loc(@1);
    }
    | UINT8 '(' INT_LITERAL ')'
    {
        $$ = expr_from_uint8($3);
        $$->loc = loc(@1);
    }
    | UINT16 '(' INT_LITERAL ')'
    {
        $$ = expr_from_uint16($3);
        $$->loc = loc(@1);
    }
    | UINT32 '(' INT_LITERAL ')'
    {
        $$ = expr_from_uint32($3);
        $$->loc = loc(@1);
    }
    | UINT64 '(' INT_LITERAL ')'
    {
        $$ = expr_from_uint64($3);
        $$->loc = loc(@1);
    }
    | FLOAT_LITERAL
    {
        $$ = expr_from_float($1);
        $$->loc = loc(@1);
    }
    | FLOAT32 '(' FLOAT_LITERAL ')'
    {
        $$ = expr_from_float32($3);
        $$->loc = loc(@1);
    }
    | FLOAT64 '(' FLOAT_LITERAL ')'
    {
        $$ = expr_from_float64($3);
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
    | STRING '(' STRING_LITERAL ')'
    {
        $$ = expr_from_str($3);
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
    | SIZEOF '(' type ')'
    {

    }
    | '(' expr ')'
    {
        $$ = $2;
    }
    | '(' error
    {
        assert(0);
    }
    | '(' expr error
    {
        assert(0);
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
    | POINTER '<' type '>' '(' ID ')'
    {
        assert(0);
    }
    /*
    | POINTER '<' type '>' '(' ID '.' ID ')'
    {
        assert(0);
    }
    */
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
    | ID L_ANGLE_ARGS type_list r_angle
    {
        $$.loc = loc(@1);
        $$.ty = desc_from_klass(NULL, $1, $3);
        $$.loc2 = loc(@3);
    }
    | ID '.' ID L_ANGLE_ARGS type_list r_angle
    {
        $$.loc = loc(@1);
        $$.ty = desc_from_klass($1, $3, $5);
        $$.loc2 = loc(@3);
        $$.loc3 = loc(@5);
    }
    | ID '.' error
    {
        printf("error types\n");
        $$.loc = loc(@3);
        $$.ty = desc_from_unk();
    }
    | ID L_ANGLE_ARGS error
    {
        printf("error types2\n");
        $$.loc = loc(@3);
        $$.ty = desc_from_unk();
    }
    | ID L_ANGLE_ARGS type_list error
    {
        printf("error types3\n");
        $$.loc = loc(@4);
        $$.ty = desc_from_unk();
    }
    | ID '.' ID L_ANGLE_ARGS error
    {
        printf("error types4\n");
        $$.loc = loc(@5);
        $$.ty = desc_from_unk();
    }
    | ID '.' ID L_ANGLE_ARGS type_list error
    {
        printf("error types5\n");
        $$.loc = loc(@6);
        $$.ty = desc_from_unk();
    }
    ;

r_angle
    : '>'
    | R_ANGLE_SHIFT
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
