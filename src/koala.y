/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

/* prologue */
%{

#include <stdlib.h>
#include <string.h>
#include "parser.h"

#define yyerror(loc, ps, scanner, msg) ((void)0)

#define yyloc_row(loc) ((loc).first_line)
#define yyloc_col(loc) ((loc).first_column)

#define set_expr_pos(exp, loc) ({ \
  (exp)->row = yyloc_row(loc);   \
  (exp)->col = yyloc_col(loc);   \
})

#define IDENT(name, s, loc) \
  Ident name = {s, yyloc_row(loc), yyloc_col(loc)}

/* interactive mode */
void Cmd_EvalStmt(ParserState *ps, Stmt *stmt);

%}

%union {
  int64_t ival;
  double fval;
  wchar cval;
  char *sval;
  char *text;
  Vector *list;
  Expr *expr;
  Stmt *stmt;
}

%token IMPORT
%token CONST
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

%token BYTE
%token INTEGER
%token FLOAT
%token CHAR
%token STRING
%token BOOL
%token ANY

%token SELF
%token SUPER
%token TRUE
%token FALSE
%token NIL

%token FREE_ASSIGN
%token PLUS_ASSIGN
%token MINUS_ASSIGN
%token MULT_ASSIGN
%token DIV_ASSIGN
%token MOD_ASSIGN
%token AND_ASSIGN
%token OR_ASSIGN
%token XOR_ASSIGN

%token OP_AND
%token OP_OR
%token OP_NOT
%token OP_EQ
%token OP_NE
%token OP_LE
%token OP_GE
%token OP_POWER

%token DOTDOTDOT
%token DOTDOTLESS
%token FAT_ARROW

%token <text> COMMENT
%token <text> DOC
%token <text> MODDOC

%token <ival> INT_LITERAL
%token <fval> FLOAT_LITERAL
%token <cval> CHAR_LITERAL
%token <sval> STRING_LITERAL
%token <sval> ID

%type <list> basic_expr_list
%type <expr> atom
%type <expr> dot_expr
%type <expr> call_expr
%type <expr> index_expr
%type <expr> primary_expr
%type <expr> unary_expr
%type <expr> multi_expr
%type <expr> add_expr
%type <expr> relation_expr
%type <expr> equality_expr
%type <expr> and_expr
%type <expr> exclusive_or_expr
%type <expr> inclusive_or_expr
%type <expr> logic_and_expr
%type <expr> logic_or_expr
%type <expr> basic_expr
%type <expr> expr

%precedence ID
%precedence ','
%precedence ')'

%locations
%parse-param {ParserState *ps}
%parse-param {void *scanner}
%define api.pure full
%lex-param {void *scanner}
%code provides {
  int yylex(YYSTYPE *yylval_param, YYLTYPE *yylloc, void *yyscanner);
}

%start units

/* grammar rules */

%%

units:
  unit
| units unit
;

unit:
  import
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
| var_decl
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
| expr ';'
{
  if (ps->interactive) {
    ps->more = 0;
    Stmt *stmt = stmt_from_expr($1);
    Cmd_EvalStmt(ps, stmt);
    stmt_free(stmt);
    ps->errnum = 0;
  }
}
| func_decl
{
  if (ps->interactive) {
    ps->more = 0;
  }
}
  /*
| type_decl
  */
| ';'
{
  if (ps->interactive) {
    ps->more = 0;
  }
}
| COMMENT
{
  if (ps->interactive) {
    ps->more = 0;
  }
}
| DOC
{
  if (ps->interactive) {
    ps->more = 0;
  }
}
| MODDOC
{
  if (ps->interactive) {
    ps->more = 0;
  }
}
| error
{
  syntax_error(ps, yyloc_row(@1), yyloc_col(@1), "invalid statements");
  if (ps->interactive) {
    ps->more = 0;
  }
}
;

local:
  var_decl
| free_var_decl
| assignment
| return_expr
| jump_expr
| block
| expr ';'
| ';'
| COMMENT
| error
;

import:
  IMPORT STRING_LITERAL ';'
{
  print("import STRING_LITERAL;\n");
}
| IMPORT ID STRING_LITERAL ';'
| IMPORT '.' STRING_LITERAL ';'
| IMPORT '{' id_as_list '}' STRING_LITERAL ';'
| IMPORT error
{
  print("import error\n");
}
;

id_as_list:
  ID
| ID AS ID
| id_as_list ',' ID
| id_as_list ',' ID AS ID
;

const_decl:
  CONST ID '=' basic_expr ';'
| CONST ID type '=' basic_expr ';'
| CONST error
;

var_decl:
  VAR ID type ';'
| VAR ID type '=' expr ';'
| VAR ID '=' expr ';'
| VAR ID error
{
  print("var ID error\n");
}
| VAR error
{
  print("var error\n");
}
;

free_var_decl:
  ID FREE_ASSIGN expr ';'
;

assignment:
  primary_expr assign_operator expr ';'
;

assign_operator:
  '='
| PLUS_ASSIGN
| MINUS_ASSIGN
| MULT_ASSIGN
| DIV_ASSIGN
| MOD_ASSIGN
| AND_ASSIGN
| OR_ASSIGN
| XOR_ASSIGN
;

return_expr:
  RETURN ';'
| RETURN expr ';'
| RETURN error
;

jump_expr:
  BREAK ';'
| CONTINUE ';'
;

block:
  '{' local_list '}'
| '{' basic_expr '}'
;

local_list:
  local
| local_list local
;

expr:
  basic_expr
{
  $$ = $1;
}
  /*
| if_expr
| while_expr
| match_expr
| for_each_expr
  */
;

basic_expr:
  range_object
{

}
| lambda_object
{

}
| logic_or_expr
{
  $$ = $1;
}
;

range_object:
  logic_or_expr DOTDOTDOT logic_or_expr
| logic_or_expr DOTDOTLESS logic_or_expr
;

lambda_object:
  '(' ID ')' FAT_ARROW block
| '(' ID ')' FAT_ARROW basic_expr
| '(' idlist ID ')' FAT_ARROW block
| '(' idlist ID ')' FAT_ARROW basic_expr
| '(' ')' FAT_ARROW block
| '(' ')' FAT_ARROW basic_expr
;

idlist:
  ID ','
| idlist ID ','
;

logic_or_expr:
  logic_and_expr
{
  $$ = $1;
}
| logic_or_expr OP_OR logic_and_expr
;

logic_and_expr:
  inclusive_or_expr
{
  $$ = $1;
}
| logic_and_expr OP_AND inclusive_or_expr
;

inclusive_or_expr:
  exclusive_or_expr
{
  $$ = $1;
}
| inclusive_or_expr '|' exclusive_or_expr
;

exclusive_or_expr:
  and_expr
{
  $$ = $1;
}
| exclusive_or_expr '^' and_expr
;

and_expr:
  equality_expr
{
  $$ = $1;
}
| and_expr '&' equality_expr
;

equality_expr:
  relation_expr
{
  $$ = $1;
}
| equality_expr OP_EQ relation_expr
| equality_expr OP_NE relation_expr
;

relation_expr:
  add_expr
{
  $$ = $1;
}
| relation_expr '<' add_expr
| relation_expr '>' add_expr
| relation_expr OP_LE add_expr
| relation_expr OP_GE add_expr
;

add_expr:
  multi_expr
{
  $$ = $1;
}
| add_expr '+' multi_expr
| add_expr '-' multi_expr
;

multi_expr:
  unary_expr
{
  $$ = $1;
}
| multi_expr '*' unary_expr
| multi_expr '/' unary_expr
| multi_expr '%' unary_expr
| multi_expr OP_POWER unary_expr
;

unary_expr:
  primary_expr
{
  $$ = $1;
}
| unary_operator unary_expr
{

}
;

unary_operator:
  '+'
| '-'
| '~'
| OP_NOT
;

primary_expr:
  call_expr
{
  $$= $1;
}
| dot_expr
{
  $$= $1;
}
| index_expr
{
  $$= $1;
}
| atom
{
  $$ = $1;
}
;

call_expr:
  primary_expr '(' ')'
{
  $$ = expr_from_call(NULL, $1);
}
| primary_expr '(' basic_expr_list ')'
{
  $$ = expr_from_call($3, $1);
}
;

dot_expr:
  primary_expr '.' ID
{
  IDENT(id, $3, @3);
  $$ = expr_from_attribute(id, $1);
}
;

index_expr:
  primary_expr '[' basic_expr ']'
{

}
| primary_expr '[' basic_expr ':' basic_expr ']'
{

}
| primary_expr '[' ':' basic_expr ']'
{

}
| primary_expr '[' basic_expr ':' ']'
{

}
;

atom:
  ID
{
  $$ = expr_from_ident($1);
  set_expr_pos($$, @1);
}
| INT_LITERAL
{
  $$ = expr_from_integer($1);
  set_expr_pos($$, @1);
}
| FLOAT_LITERAL
{
  $$ = expr_from_float($1);
  set_expr_pos($$, @1);
}
| CHAR_LITERAL
{
  $$ = expr_from_char($1);
  set_expr_pos($$, @1);
}
| STRING_LITERAL
{
  $$ = expr_from_string($1);
  set_expr_pos($$, @1);
}
| TRUE
{
  $$ = expr_from_bool(1);
  set_expr_pos($$, @1);
}
| FALSE
{
  $$ = expr_from_bool(0);
  set_expr_pos($$, @1);
}
| NIL
{
  $$ = expr_from_nil();
  set_expr_pos($$, @1);
}
| SELF
{
  $$ = expr_from_self();
  set_expr_pos($$, @1);
}
| SUPER
{

}
| '(' basic_expr ')'
{

}
| tuple_object
{

}
| array_object
{

}
| map_object
{

}
| anony_object
{

}
;

tuple_object:
  '(' basic_expr_list ',' ')'
| '(' basic_expr_list ',' basic_expr ')'
| '(' ')'
;

array_object:
  '[' basic_expr_list ']'
{
  print("array object\n");
}
| '[' basic_expr_list ',' ']'
{
  print("array object\n");
}
| '[' ']'
{
  print("array object\n");
}
| '[' error ']'
{
  print("array object error\n");
}
;

basic_expr_list:
  basic_expr
{
  $$ = vector_new();
  vector_push_back($$, $1);
}
| basic_expr_list ',' basic_expr
{
  $$ = $1;
  vector_push_back($$, $3);
}
;

map_object:
  '{' kv_list '}'
{
  print("map object\n");
}
| '{' kv_list ',' '}'
| '{' ':' '}'
;

kv_list:
  kv
| kv_list ',' kv
;

kv:
  basic_expr ':' basic_expr
;

anony_object:
  FUNC '(' para_list ')' type block
| FUNC '(' para_list ')' block
| FUNC '(' ')' type block
| FUNC '(' ')' block
;

/*
if_expr:
;

while_expr:
;

for_each_expr:
;

match_expr:
;
*/

func_decl:
  FUNC name '(' para_list ')' type block
| FUNC name '(' para_list ')' block
| FUNC name '(' ')' type block
| FUNC name '(' ')' block
| FUNC error
;

name:
  ID
| ID '<' type_para_list '>'
;

type_para_list:
  type_para
| type_para_list ',' type_para
;

type_para:
  ID
| ID ':' type_upbound_list
;

type_upbound_list:
  klass_type
| type_upbound_list '&' klass_type
;

para_list:
  id_type_list
| id_varg
| id_type_list ',' id_varg
;

id_type_list:
  ID type
| id_type_list ',' ID type
;

id_varg:
  ID DOTDOTDOT
| ID DOTDOTDOT no_array_type
;

/*
type_decl:
;
*/

type_list:
  type
| type_list ',' type
;

type:
  no_array_type
| '[' type ']'
;

no_array_type:
  BYTE
| INTEGER
| FLOAT
| CHAR
| STRING
| BOOL
| ANY
| '[' type ':' type ']'
| '(' type_list ')'
| klass_type
| func_type
;

klass_type:
  ID
| ID '.' ID
| ID '<' type_list '>'
| ID '.' ID '<' type_list '>'
;

func_type:
  FUNC '(' type_varg_list ')' type
| FUNC '(' para_list')' type
| FUNC '(' type_varg_list ')'
| FUNC '(' para_list ')'
| FUNC '(' ')' type
| FUNC '(' ')'
| FUNC error
;

type_varg_list:
  type_list
| type_list ',' DOTDOTDOT
| type_list ',' DOTDOTDOT no_array_type
;

%%

/* epilogue */
