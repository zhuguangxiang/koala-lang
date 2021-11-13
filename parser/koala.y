/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

/* prologue */

%{

#include "parser.h"
#include "koala_yacc.h"

#define yyerror(loc, ps, scanner, msg) ((void)0)

#define row(loc) ((loc).first_line)
#define col(loc) ((loc).first_column)
#define col_last(loc) ((loc).last_column)
#define loc(l) (Loc){row(l), col(l)}

static inline void set_var_decl_where(Stmt *stmt, int where)
{
    if (!stmt) return;
    VarDeclStmt *var = (VarDeclStmt *)stmt;
    var->where = where;
}

int keyword(int token)
{
    int keys[] = {
        IMPORT, TYPE, LET, VAR, FUNC,
        IF, WHILE, FOR, MATCH, CLASS,
        TRAIT, ENUM, RETURN, FINAL
    };

    for (int i = 0; i < COUNT_OF(keys); i++) {
        if (keys[i] == token) return 1;
    }
    return 0;
}

#define yy_clearin_errok if (!keyword(ps->token)) yyclearin; yyerrok

%}

%union {
    int64 ival;
    double fval;
    int cval;
    char *sval;
    AssignOpKind assignop;
    Expr *expr;
    Stmt *stmt;
    ExprType *type;
    Vector *list;
    Vector *typelist;
    Vector *stmtlist;
    IdentArgs idargs;
    IdentType idtype;
}

%token IMPORT
%token TYPE
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
%token FINAL

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
%token <sval> INVALID

%token <ival> INT_LITERAL
%token <fval> FLOAT_LITERAL
%token <cval> CHAR_LITERAL
%token <sval> STRING_LITERAL
%token <sval> ID

%type <stmt> stmt
%type <stmt> type_alias_decl
%type <stmt> let_decl
%type <stmt> var_decl
%type <stmt> free_var_decl
%type <assignop> assignop
%type <stmt> assignment
%type <stmt> return_stmt
%type <stmt> jump_stmt
%type <list> block
%type <stmt> local
%type <stmtlist> local_list
%type <stmt> func_decl
%type <idargs> name
%type <list> param_type_decl_list
%type <idargs> param_type_decl
%type <list> param_type_bound_list
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
%type <expr> atom_expr
%type <expr> dot_expr
%type <expr> index_expr
%type <expr> slice_expr
%type <type> base_type_expr

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
%lex-param {void *scanner}
%code provides {
  int yylex(YYSTYPE *yylval_param, YYLTYPE *yylloc, void *yyscanner);
}

%start program

/* grammar rules */

%%

program
    : stmt
    {
        if (ps->cmdline) ps->more = 0;
        ps->handle_stmt(ps, $1);
    }
    | program stmt
    {
        if (ps->cmdline) ps->more = 0;
        ps->handle_stmt(ps, $2);
    }
    ;

stmt
    : import_stmt
    {

    }
    | type_alias_decl
    {

    }
    | let_decl
    {
        parser_new_var(ps, $1);
        set_var_decl_where($1, VAR_DECL_GLOBAL);
        $$ = $1;
    }
    | var_decl
    {
        parser_new_var(ps, $1);
        set_var_decl_where($1, VAR_DECL_GLOBAL);
        $$ = $1;
    }
    | free_var_decl
    {
        parser_new_var(ps, $1);
        set_var_decl_where($1, VAR_DECL_GLOBAL);
        $$ = $1;
    }
    | assignment
    {
        $$ = $1;
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
    | func_decl
    {
        parser_new_func(ps, $1);
        $$ = $1;
    }
    | type_decl
    {

    }
    | expr ';'
    {
        $$ = stmt_from_expr($1);
        stmt_set_loc($$, loc(@1));
    }
    | expr error
    {
        yy_errmsg(loc(@2), "expected ';' or '\\n'");
        yy_clearin_errok; $$ = null;
    }
    | ';'
    {
        $$ = null;
    }
    | INVALID
    {
        yy_errmsg(loc(@1), "input error '%s'", $1);
        yyclearin; yyerrok; $$ = null;
    }
    | error
    {
        if (ps->quit) YYACCEPT;
        yy_errmsg(loc(@1), "syntax error");
        yyclearin; yyerrok; $$ = null;
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
        Ident id = {$2, loc(@2)};
        $$ = stmt_from_letdecl(&id, null, $4);
        stmt_set_loc($$, loc(@1));
    }
    | LET ID type '=' expr ';'
    {
        Ident id = {$2, loc(@2)};
        $$ = stmt_from_letdecl(&id, $3, $5);
        stmt_set_loc($$, loc(@1));
    }
    | LET error
    {
        yy_errmsg(loc(@2), "expected identifier");
        yy_clearin_errok; $$ = null;
    }
    | LET ID error
    {
        yy_errmsg(loc(@3), "expected 'TYPE' or '='");
        yy_clearin_errok; $$ = null;
    }
    | LET ID '=' error
    {
        yy_errmsg(loc(@4), "expected expression");
        yy_clearin_errok; $$ = null;
    }
    | LET ID type error
    {
        yy_errmsg(loc(@4), "expected '='");
        yy_clearin_errok; $$ = null;
    }
    | LET ID type '=' error
    {
        yy_errmsg(loc(@5), "expected expression");
        yy_clearin_errok; $$ = null;
    }
    | LET ID '=' expr error
    {
        yy_errmsg(loc(@5), "expected ';' or '\\n'");
        yy_clearin_errok; $$ = null;
    }
    | LET ID type '=' expr error
    {
        yy_errmsg(loc(@6), "expected ';' or '\\n'");
        yy_clearin_errok; $$ = null;
    }
    ;

var_decl
    : VAR ID type ';'
    {
        Ident id = {$2, loc(@2)};
        $$ = stmt_from_vardecl(&id, $3, null);
        stmt_set_loc($$, loc(@1));
    }
    | VAR ID '=' expr ';'
    {
        Ident id = {$2, loc(@2)};
        $$ = stmt_from_vardecl(&id, null, $4);
        stmt_set_loc($$, loc(@1));
    }
    | VAR ID type '=' expr ';'
    {
        Ident id = {$2, loc(@2)};
        $$ = stmt_from_vardecl(&id, $3, $5);
        stmt_set_loc($$, loc(@1));
    }
    | VAR error
    {
        yy_errmsg(loc(@2), "expected identifier");
        yy_clearin_errok; $$ = null;
    }
    | VAR ID error
    {
        yy_errmsg(loc(@3), "expected 'TYPE' or '='");
        yy_clearin_errok; $$ = null;
    }
    | VAR ID '=' error
    {
        yy_errmsg(loc(@4), "expected expression");
        yy_clearin_errok; $$ = null;
    }
    | VAR ID type error
    {
        printf("var decl error3, %c\n", yychar);
        yy_clearin_errok; $$ = null;
    }
    | VAR ID '=' expr error
    {
        yy_errmsg(loc(@5), "expected ';' or '\\n'");
        yy_clearin_errok; $$ = null;
    }
    | VAR ID type '=' error
    {
        yy_errmsg(loc(@5), "expected expression");
        yy_clearin_errok; $$ = null;
    }
    | VAR ID type '=' expr error
    {
        yy_errmsg(loc(@6), "expected ';' or '\\n'");
        yy_clearin_errok; $$ = null;
    }
    ;

free_var_decl
    : ID FREE_ASSIGN expr ';'
    {
        Ident id = {$1, loc(@1)};
        $$ = stmt_from_vardecl(&id, null, $3);
        stmt_set_loc($$, loc(@1));
    }
    | ID FREE_ASSIGN error
    {
        yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = null;
    }
    | ID FREE_ASSIGN expr error
    {
        yy_errmsg(loc(@4), "expected ';' or '\\n'");
        yy_clearin_errok; $$ = null;
    }
    ;

assignment
    : primary_expr assignop expr ';'
    {
        $$ = stmt_from_assign($2, loc(@2), $1, $3);
        stmt_set_loc($$, loc(@1));
    }
    | primary_expr assignop error
    {
        yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = null;
    }
    | primary_expr assignop expr error
    {
        yy_errmsg(loc(@4), "expected ';' or '\\n'");
        yy_clearin_errok; $$ = null;
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
        $$ = stmt_from_ret(null);
        stmt_set_loc($$, loc(@1));
    }
    | RETURN expr ';'
    {
        $$ = stmt_from_ret($2);
        stmt_set_loc($$, loc(@1));
    }
    | RETURN error
    {
        yy_errmsg(loc(@2), "expected expression, ';' or '\\n'");
        yy_clearin_errok; $$ = null;
    }
    | RETURN expr error
    {
        yy_errmsg(loc(@3), "expected ';' or '\\n'");
        yy_clearin_errok; $$ = null;
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
    | BREAK error
    {
        yy_errmsg(loc(@2), "expected ';' or '\\n'");
        yy_clearin_errok; $$ = null;
    }
    | CONTINUE error
    {
        yy_errmsg(loc(@2), "expected ';' or '\\n'");
        yy_clearin_errok; $$ = null;
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
        stmt_set_loc(s, loc(@2));

        $$ = vector_create_ptr();
        vector_push_back($$, &s);
    }
    | '{' '}'
    {
        $$ = null;
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
        stmt_set_loc($$, loc(@1));
    }
    | expr error
    {
        yy_errmsg(loc(@2), "expected ';' or '\\n'");
        yy_clearin_errok; $$ = null;
    }
    | let_decl
    {
        set_var_decl_where($1, VAR_DECL_LOCAL);
        $$ = $1;
    }
    | var_decl
    {
        set_var_decl_where($1, VAR_DECL_LOCAL);
        $$ = $1;
    }
    | free_var_decl
    {
        set_var_decl_where($1, VAR_DECL_LOCAL);
        $$ = $1;
    }
    | assignment
    {
        $$ = $1;
    }
    | if_stmt
    {
        $$ = null;
    }
    | while_stmt
    {
        $$ = null;
    }
    | for_stmt
    {
        $$ = null;
    }
    | match_stmt
    {
        $$ = null;
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
        stmt_set_loc($$, loc(@1));
    }
    | ';'
    {
        $$ = null;
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

match_stmt
    : MATCH expr '{' match_clauses '}'
    | MATCH error
    {
        yy_errmsg(loc(@2), "match expr error\n");
        yyerrok;
    }
    | MATCH expr error
    {

    }
    | MATCH expr '{' error
    {

    }
    | MATCH expr '{' match_clauses error
    {

    }
    ;

match_clauses
    : match_clause
    | match_clauses match_clause
    ;

match_clause
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

case_tail
    : ';'
    | ','
    ;

case_block
    : %empty
    | block
    | expr
    ;

func_decl
    : FUNC name '(' param_list ')' type block
    {
        $$ = stmt_from_funcdecl(&$2.id, $2.args, $4, $6, $7);
        stmt_set_loc($$, loc(@1));
    }
    | FUNC name '(' param_list ')' block
    {
        $$ = stmt_from_funcdecl(&$2.id, $2.args, $4, null, $6);
        stmt_set_loc($$, loc(@1));
    }
    | FUNC name '(' ')' type block
    {
        $$ = stmt_from_funcdecl(&$2.id, $2.args, null, $5, $6);
        stmt_set_loc($$, loc(@1));
    }
    | FUNC name '(' ')' block
    {
        $$ = stmt_from_funcdecl(&$2.id, $2.args, null, null, $5);
        stmt_set_loc($$, loc(@1));
    }
    | FUNC error
    {
        yy_errmsg(loc(@2), "expected identifier");
        yy_clearin_errok; $$ = null;
    }
    | FUNC name error
    {
        yy_errmsg(loc(@3), "expected '('");
        yy_clearin_errok; $$ = null;
    }
    | FUNC name '(' error
    {
        yy_errmsg(loc(@4), "expected declaration specifiers or ')'");
        yy_clearin_errok; $$ = null;
    }
    | FUNC name '(' param_list error
    {
        yy_errmsg(loc(@5), "expected ')'");
        yy_clearin_errok; $$ = null;
    }
    | FUNC name '(' param_list ')' error
    {
        yy_errmsg(loc(@6), "expected ';', 'TYPE' or 'BLOCK'");
        yy_clearin_errok; $$ = null;
    }
    | FUNC name '(' param_list ')' type error
    {
        yy_errmsg(loc(@7), "expected ';' or 'BLOCK'");
        yy_clearin_errok; $$ = null;
    }
    | FUNC name '(' ')' error
    {
        yy_errmsg(loc(@5), "expected ';', 'TYPE' or 'BLOCK'");
        yy_clearin_errok; $$ = null;
    }
    | FUNC name '(' ')' type error
    {
        yy_errmsg(loc(@6), "expected ';' or 'BLOCK'");
        yy_clearin_errok; $$ = null;
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
        IdentType idtype = {{$1, loc(@1)}, $2};
        $$ = vector_create(sizeof(IdentType));
        vector_push_back($$, &idtype);
    }
    | param_list ',' ID type
    {
        $$ = $1;
        IdentType idtype = {{$3, loc(@3)}, $4};
        vector_push_back($$, &idtype);
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
    | FINAL class_decl
    | trait_decl
    | enum_decl
    | FINAL enum_decl
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

trait_decl
    : TRAIT name extends '{' trait_members '}'
    | TRAIT name extends '{' '}'
    | TRAIT name extends ';'
    | TRAIT error
    | TRAIT name error
    | TRAIT name extends '{' error
    | TRAIT name extends '{' trait_members error
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
        Ident id = {$1, loc(@1)};
        $$ = (IdentArgs){id, null};
    }
    | ID '<' param_type_decl_list '>'
    {
        Ident id = {$1, loc(@1)};
        $$ = (IdentArgs){id, $3};
    }
    | ID '<' error
    {

    }
    ;

param_type_decl_list
    : param_type_decl
    {
        $$ = vector_create(sizeof(IdentArgs));
        vector_push_back($$, &$1);
    }
    | param_type_decl_list ',' param_type_decl
    {
        $$ = $1;
        vector_push_back($$, &$3);
    }
    | param_type_decl_list ',' error
    {

    }
    ;

param_type_decl
    : ID
    {
        Ident id = {$1, loc(@1)};
        $$ = (IdentArgs){id, null};
    }
    | ID ':' param_type_bound_list
    {
        Ident id = {$1, loc(@1)};
        $$ = (IdentArgs){id, $3};
    }
    | ID error
    {
        yy_errmsg(loc(@2), "expected ':'");
        yy_clearin_errok;
        Ident id = {$1, loc(@1)};
        $$ = (IdentArgs){id, null};
    }
    ;

param_type_bound_list
    : klass_type
    {
        $$ = vector_create_ptr();
        vector_push_back($$, &$1);
    }
    | param_type_bound_list '&' klass_type
    {
        $$ = $1;
        vector_push_back($$, &$3);
    }
    | error
    {
        yy_errmsg(loc(@1), "expected 'TYPE'");
        yy_clearin_errok; $$ = null;
    }
    | param_type_bound_list error
    {
        yy_errmsg(loc(@2), "expected '&'");
        yy_clearin_errok; $$ = null;
    }
    | param_type_bound_list '&' error
    {
        yy_errmsg(loc(@1), "expected 'TYPE'");
        yy_clearin_errok; $$ = null;
    }
    ;

extends
    : %empty
    | ':' klass_list
    | ':' error
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
    : var_decl
    | let_decl
    | func_decl
    | LET ID type ';'
    | ';'
    ;

trait_members
    : trait_member
    | trait_members trait_member
    ;

trait_member
    : func_decl
    | proto_decl
    | var_decl
    {
        // partial ok
    }
    | let_decl
    {

    }
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
        $$ = null;
        // $$ = expr_from_range();
        // expr_set_loc($$, loc(@1));
    }
    | or_expr DOTDOTLESS or_expr
    {
        $$ = null;
        // $$ = expr_from_range();
        // expr_set_loc($$, loc(@1));
    }
    | or_expr DOTDOTDOT error
    {
        yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = null;
    }
    | or_expr DOTDOTLESS error
    {
        yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = null;
    }
    ;

is_expr
    : primary_expr IS type
    {
        $$ = null;
        //$$ = expr_from_is($1, $3);
        //expr_set_loc($$, loc(@1));
    }
    | primary_expr IS error {
        yy_errmsg(loc(@3), "expected 'TYPE'");
        yy_clearin_errok; $$ = null;
    }
    ;

as_expr
    : primary_expr AS type
    {
        $$ = null;
        //$$ = expr_from_as($1, $3);
        //expr_set_loc($$, loc(@1));
    }
    | primary_expr AS error {
        yy_errmsg(loc(@3), "expected 'TYPE'");
        yy_clearin_errok; $$ = null;
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
        expr_set_loc($$, loc(@1));
    }
    | or_expr OR error
    {
        yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = null;
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
        expr_set_loc($$, loc(@1));
    }
    | and_expr AND error
    {
        yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = null;
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
        expr_set_loc($$, loc(@1));
    }
    | bit_or_expr '|' error
    {
        yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = null;
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
        expr_set_loc($$, loc(@1));
    }
    | bit_xor_expr '^' error
    {
        yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = null;
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
        expr_set_loc($$, loc(@1));
    }
    | bit_and_expr '&' error
    {
        yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = null;
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
        expr_set_loc($$, loc(@1));
    }
    | equality_expr NE relation_expr
    {
        $$ = expr_from_binary(BINARY_NE, loc(@2), $1, $3);
        expr_set_loc($$, loc(@1));
    }
    | equality_expr EQ error
    {
        yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = null;
    }
    | equality_expr NE error
    {
        yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = null;
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
        expr_set_loc($$, loc(@1));
    }
    | relation_expr '>' shift_expr
    {
        $$ = expr_from_binary(BINARY_GT, loc(@2), $1, $3);
        expr_set_loc($$, loc(@1));
    }
    | relation_expr LE shift_expr
    {
        $$ = expr_from_binary(BINARY_LE, loc(@2), $1, $3);
        expr_set_loc($$, loc(@1));
    }
    | relation_expr GE shift_expr
    {
        $$ = expr_from_binary(BINARY_GE, loc(@2), $1, $3);
        expr_set_loc($$, loc(@1));
    }
    | relation_expr '<' error
    {
        yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = null;
    }
    | relation_expr '>' error
    {
        printf("HERE?\n");
        yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = null;
    }
    | relation_expr LE error
    {
        yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = null;
    }
    | relation_expr GE error
    {
        yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = null;
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
        expr_set_loc($$, loc(@1));
    }
    | shift_expr L_SHIFT add_expr
    {
        $$ = expr_from_binary(BINARY_SHL, loc(@2), $1, $3);
        expr_set_loc($$, loc(@1));
    }
    | shift_expr R_ANGLE_SHIFT '>' error
    {
        yy_errmsg(loc(@4), "expected expression");
        yy_clearin_errok; $$ = null;
    }
    | shift_expr L_SHIFT error
    {
        yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = null;
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
        expr_set_loc($$, loc(@1));
    }
    | add_expr '-' multi_expr
    {
        $$ = expr_from_binary(BINARY_SUB, loc(@2), $1, $3);
        expr_set_loc($$, loc(@1));
    }
    | add_expr '+' error
    {
        yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = null;
    }
    | add_expr '-' error
    {
        yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = null;
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
        expr_set_loc($$, loc(@1));
    }
    | multi_expr '/' unary_expr
    {
        $$ = expr_from_binary(BINARY_DIV, loc(@2), $1, $3);
        expr_set_loc($$, loc(@1));
    }
    | multi_expr '%' unary_expr
    {
        $$ = expr_from_binary(BINARY_MOD, loc(@2), $1, $3);
        expr_set_loc($$, loc(@1));
    }
    | multi_expr '*' error
    {
        yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = null;
    }
     | multi_expr '/' error
    {
        yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = null;
    }
     | multi_expr '%' error
    {
        yy_errmsg(loc(@3), "expected expression");
        yy_clearin_errok; $$ = null;
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
        expr_set_loc($$, loc(@1));
    }
    | '-' unary_expr
    {
        $$ = expr_from_unary(UNARY_NEG, loc(@1), $2);
        expr_set_loc($$, loc(@1));
    }
    | '~' unary_expr
    {
        $$ = expr_from_unary(UNARY_BIT_NOT, loc(@1), $2);
        expr_set_loc($$, loc(@1));
    }
    | NOT unary_expr
    {
        $$ = expr_from_unary(UNARY_NOT, loc(@1), $2);
        expr_set_loc($$, loc(@1));
    }
    ;

primary_expr
    : call_expr
    {
        //$$ = $1;
    }
    | dot_expr
    {
        $$ = $1;
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
        yy_errmsg(loc(@3),
            "expected 'ID' or 'index' before '%c' token", yychar);
        yyclearin;
        yyerrok;
        $$ = null;
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
        yy_errmsg(loc(@1), "expected slice expression");
        yy_clearin_errok; $$ = null;
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
        Ident id = {$1, loc(@1)};
        $$ = expr_from_ident(&id, null);
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
    | new_map_expr
    {
        assert(0);
    }
    | base_type_expr
    {
        $$ = expr_from_type($1);
        expr_set_loc($$, loc(@1));
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

new_map_expr
    : '[' expr ':' expr ']'
    {
        printf("new map\n");
    }
    | '[' expr ':' error
    | '[' expr ':' expr error
    ;

base_type_expr
    : INT8
    {
        $$ = expr_type_int8();
        expr_type_set_loc($$, loc(@1));
    }
    | INT16
    {
        $$ = expr_type_int16();
        expr_type_set_loc($$, loc(@1));
    }
    | INT32
    {
        $$ = expr_type_int32();
        expr_type_set_loc($$, loc(@1));
    }
    | INT64
    {
        $$ = expr_type_int64();
        expr_type_set_loc($$, loc(@1));
    }
    | FLOAT32
    {
        $$ = expr_type_float32();
        expr_type_set_loc($$, loc(@1));
    }
    | FLOAT64
    {
        $$ = expr_type_float64();
        expr_type_set_loc($$, loc(@1));
    }
    | BOOL
    {
        $$ = expr_type_bool();
        expr_type_set_loc($$, loc(@1));
    }
    | CHAR
    {
        $$ = expr_type_char();
        expr_type_set_loc($$, loc(@1));
    }
    | STRING
    {
        $$ = expr_type_str();
        expr_type_set_loc($$, loc(@1));
    }
    | ANY
    {
        $$ = expr_type_any();
        expr_type_set_loc($$, loc(@1));
    }
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
        $$ = expr_type_array($2);
        expr_type_set_loc($$, loc(@1));
    }
    | '[' type ':' type ']'
    {
        $$ = expr_type_map($2, $4);
        expr_type_set_loc($$, loc(@1));
    }
    | '(' type_list ')'
    {
        $$ = expr_type_tuple($2);
        expr_type_set_loc($$, loc(@1));
    }
    | '[' error
    {

    }
    | '[' type error
    {

    }
    | '[' type ':' error
    {

    }
    | '[' type ':' type error
    {

    }
    | '(' error
    {

    }
    | '(' type_list error
    {

    }
    ;

atom_type
    : INT8
    {
        $$ = expr_type_int8();
        expr_type_set_loc($$, loc(@1));
    }
    | INT16
    {
        $$ = expr_type_int16();
        expr_type_set_loc($$, loc(@1));
    }
    | INT32
    {
        $$ = expr_type_int32();
        expr_type_set_loc($$, loc(@1));
    }
    | INT64
    {
        $$ = expr_type_int64();
        expr_type_set_loc($$, loc(@1));
    }
    | FLOAT32
    {
        $$ = expr_type_float32();
        expr_type_set_loc($$, loc(@1));
    }
    | FLOAT64
    {
        $$ = expr_type_float64();
        expr_type_set_loc($$, loc(@1));
    }
    | BOOL
    {
        $$ = expr_type_bool();
        expr_type_set_loc($$, loc(@1));
    }
    | CHAR
    {
        $$ = expr_type_char();
        expr_type_set_loc($$, loc(@1));
    }
    | STRING
    {
        $$ = expr_type_str();
        expr_type_set_loc($$, loc(@1));
    }
    | ANY
    {
        $$ = expr_type_any();
        expr_type_set_loc($$, loc(@1));
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
        Ident pkg = {null};
        Ident name = {$1, loc(@1)};
        $$ = expr_type_klass(&pkg, &name, null);
        expr_type_set_loc($$, loc(@1));
    }
    | ID '.' ID
    {
        Ident pkg = {$1, loc(@1)};
        Ident name = {$3, loc(@3)};
        $$ = expr_type_klass(&pkg, &name, null);
        expr_type_set_loc($$, loc(@1));
    }
    | ID L_ANGLE_ARGS type_list r_angle
    {
        Ident pkg = {null};
        Ident name = {$1, loc(@1)};
        $$ = expr_type_klass(&pkg, &name, $3);
        expr_type_set_loc($$, loc(@1));
    }
    | ID '.' ID L_ANGLE_ARGS type_list r_angle
    {
        Ident pkg = {$1, loc(@1)};
        Ident name = {$3, loc(@3)};
        $$ = expr_type_klass(&pkg, &name, $5);
        expr_type_set_loc($$, loc(@1));
    }
    ;

r_angle
    : '>'
    | R_ANGLE_SHIFT
    ;

func_type
    : FUNC '(' type_list ')' type
    {
        $$ = expr_type_proto($5, $3);
        expr_type_set_loc($$, loc(@1));
    }
    | FUNC '(' type_list ')'
    {
        $$ = expr_type_proto(null, $3);
        expr_type_set_loc($$, loc(@1));
    }
    | FUNC '(' ')' type
    {
        $$ = expr_type_proto($4, null);
        expr_type_set_loc($$, loc(@1));
    }
    | FUNC '(' ')'
    {
        $$ = expr_type_proto(null, null);
        expr_type_set_loc($$, loc(@1));
    }
    | FUNC '(' param_list ')' type
    {

    }
    | FUNC '(' param_list ')'
    {

    }
    | FUNC error
    {

    }
    | FUNC '(' error
    {

    }
    | FUNC '(' type_list error
    {

    }
    | FUNC '(' param_list error
    {

    }
    ;

type_list
    : type
    {
        $$ = vector_create_ptr();
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
