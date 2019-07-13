/*
MIT License

Copyright (c) 2018 James, https://github.com/zhuguangxiang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/* prologue */
%{

#include <stdlib.h>
#include <string.h>
#include "parser.h"

#define yyerror(loc, ps, scanner, errmsg) ((void)0)

#define interactive ps->interactive

int prompt;

%}

%union {
  int64_t ival;
  double fval;
  uint32_t cval;
  char *sval;
  char *text;
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

%token <ival> BYTE_LITERAL
%token <ival> INT_LITERAL
%token <fval> FLOAT_LITERAL
%token <cval> CHAR_LITERAL
%token <sval> STRING_LITERAL
%token <sval> ID

%precedence ID
%precedence ','
%precedence ')'

%locations
%parse-param {struct parser_state *ps}
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
{
  if (interactive) {
    ps->more = 0;
  }
}
| units unit
{
  if (interactive) {
    ps->more = 0;
  }
}
;

unit:
  import
| const_decl
| var_decl
| free_var_decl
| assignment
| expr ';'
| func_decl
  /*
| type_decl
  */
| ';'
| COMMENT
| DOC
| MODDOC
| error
;

local_expr:
  var_decl
| free_var_decl
| assignment
| return_expr
| jump_expr
| block
| expr ';'
| ';'
| COMMENT
| DOC
| error
;

import:
  IMPORT STRING_LITERAL ';'
{
  printf("import STRING_LITERAL;\n");
}
| IMPORT ID STRING_LITERAL ';'
| IMPORT '.' STRING_LITERAL ';'
| IMPORT '{' id_as_list '}' STRING_LITERAL ';'
| IMPORT error
{
  printf("import error\n");
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
| VAR '(' id_list ID ')' '=' expr ';'
| VAR '(' id_list ID ')' '(' type_list ')' '=' expr ';'
| VAR ID error
{
  printf("var ID error\n");
}
| VAR error
{
  printf("var error\n");
}
;

free_var_decl:
  ID FREE_ASSIGN expr ';'
| '(' id_list ID ')' FREE_ASSIGN expr ';'
;

id_list:
  ID ','
| id_list ID ','
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
  '{' local_expr_list '}'
| '{' basic_expr '}'
;

local_expr_list:
  local_expr
| local_expr_list local_expr
;

expr:
  basic_expr
  /*
| if_expr
| while_expr
| match_expr
| for_each_expr
  */
;

basic_expr:
  range_object
| lambda_object
| logic_or_expr
;

range_object:
  logic_or_expr DOTDOTDOT logic_or_expr
| logic_or_expr DOTDOTLESS logic_or_expr
;

lambda_object:
  '(' ID ')' FAT_ARROW block
| '(' ID ')' FAT_ARROW basic_expr
| '(' id_list ID ')' FAT_ARROW block
| '(' id_list ID ')' FAT_ARROW basic_expr
| '(' ')' FAT_ARROW block
| '(' ')' FAT_ARROW basic_expr
;

logic_or_expr:
  logic_and_expr
| logic_or_expr OP_OR logic_and_expr
;

logic_and_expr:
  inclusive_or_expr
| logic_and_expr OP_AND inclusive_or_expr
;

inclusive_or_expr:
  exclusive_or_expr
| inclusive_or_expr '|' exclusive_or_expr
;

exclusive_or_expr:
  and_expr
| exclusive_or_expr '^' and_expr
;

and_expr:
  equality_expr
| and_expr '&' equality_expr
;

equality_expr:
  relation_expr
| equality_expr OP_EQ relation_expr
| equality_expr OP_NE relation_expr
;

relation_expr:
  add_expr
| relation_expr '<' add_expr
| relation_expr '>' add_expr
| relation_expr OP_LE add_expr
| relation_expr OP_GE add_expr
;

add_expr:
  multi_expr
| add_expr '+' multi_expr
| add_expr '-' multi_expr
;

multi_expr:
  unary_expr
| multi_expr '*' unary_expr
| multi_expr '/' unary_expr
| multi_expr '%' unary_expr
| multi_expr OP_POWER unary_expr
;

unary_expr:
  primary_expr
| unary_operator unary_expr
;

unary_operator:
  '+'
| '-'
| '~'
| OP_NOT
;

primary_expr:
  call_expr
| dot_expr
| index_expr
| atom
;

call_expr:
  primary_expr '(' ')'
| primary_expr '(' basic_expr_list ')'
;

dot_expr:
  primary_expr '.' ID
;

index_expr:
  primary_expr '[' basic_expr ']'
| primary_expr '[' basic_expr ':' basic_expr ']'
| primary_expr '[' ':' basic_expr ']'
| primary_expr '[' basic_expr ':' ']'
;

atom:
  ID
| BYTE_LITERAL
| INT_LITERAL
| FLOAT_LITERAL
| CHAR_LITERAL
| STRING_LITERAL
| TRUE
| FALSE
| NIL
| SELF
| SUPER
| '(' basic_expr ')'
| tuple_object
| array_object
| map_object
| anony_object
;

tuple_object:
  '(' basic_expr_list ',' ')'
| '(' basic_expr_list ',' basic_expr ')'
| '(' ')'
;

array_object:
  '[' basic_expr_list ']'
{
  printf("array object\n");
}
| '[' basic_expr_list ',' ']'
{
  printf("array object\n");
}
| '[' ']'
{
  printf("array object\n");
}
| '[' error ']'
{
  printf("array object error\n");
}
;

basic_expr_list:
  basic_expr
| basic_expr_list ',' basic_expr
;

map_object:
  '{' kv_list '}'
{
  printf("map object\n");
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
