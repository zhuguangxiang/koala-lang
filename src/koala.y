/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

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

#define yyerror(loc, ps, scanner, msg) ((void)0)

#define row(loc) ((loc).first_line)
#define col(loc) ((loc).first_column)

#define set_expr_pos(exp, loc) ({ \
  (exp)->row = row(loc);   \
  (exp)->col = col(loc);   \
})

#define IDENT(name, s, loc) \
  Ident name = {s, row(loc), col(loc)}

#define TYPE(name, type, loc) \
  Type name = {type, row(loc), col(loc)}

/* interactive mode */
void cmd_eval_stmt(ParserState *ps, Stmt *stmt);
int cmd_add_const(ParserState *ps, Ident id, Type type);
int cmd_add_var(ParserState *ps, Ident id, Type type);
int cmd_add_func(ParserState *ps, char *name, Vector *idtypes, Type ret);

%}

%union {
  int64_t ival;
  double fval;
  wchar cval;
  char *sval;
  char *text;
  Vector *list;
  Vector *exprlist;
  Vector *stmtlist;
  Vector *idtypelist;
  Vector *desclist;
  void *ptr;
  Expr *expr;
  Stmt *stmt;
  TypeDesc *desc;
  AssignOpKind assginop;
  UnaryOpKind unaryop;
  IdParaDef name;
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
%token BY
%token AS
%token IS

%token BYTE
%token INTEGER
%token FLOAT
%token CHAR
%token STRING
%token BOOL
%token ANY

%token NEW
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
%token POW_ASSIGN
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

%type <stmt> const_decl
%type <stmt> var_decl
%type <stmt> free_var_decl
%type <stmt> assignment
%type <assginop> assign_operator
%type <stmt> return_stmt
%type <stmt> jump_stmt
%type <stmt> if_stmt
%type <stmt> empty_else
%type <stmt> while_stmt
%type <stmt> for_each_stmt
%type <stmt> func_decl
%type <stmtlist> block
%type <stmtlist> local_list
%type <stmtlist> expr2_list
%type <stmt> local
%type <name> name
%type <list> type_para_list
%type <desc> type_para
%type <list> type_upbound_list
%type <idtypelist> para_list
%type <idtypelist> id_type_list
%type <ptr> id_varg

%type <expr> new_object
%type <expr> anony_func
%type <ptr> mapentry
%type <list> mapentry_list
%type <expr> map_object
%type <expr> array_object
%type <expr> tuple_object
%type <exprlist> expr_list
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
%type <expr> condition_expr
%type <expr> expr
%type <unaryop> unary_operator
%type <expr> expr_as_type
%type <expr> expr_is_type
%type <expr> range_object

%type <desclist> type_list
%type <desclist> type_varg_list
%type <desc> type
%type <desc> no_array_type
%type <desc> klass_type
%type <desc> func_type

%destructor {
  printf("free expr\n");
  expr_free($$);
} <expr>
%destructor {
  printf("free stmt\n");
  stmt_free($$);
} <stmt>
%destructor {
  printf("decref desc\n");
  TYPE_DECREF($$);
} <desc>
%destructor {
  printf("free exprlist\n");
  exprlist_free($$);
} <exprlist>
%destructor {
  printf("free stmtblock\n");
  stmt_block_free($$);
} <stmtlist>
%destructor {
  printf("free idtypelist\n");
  free_idtypes($$);
} <idtypelist>
%destructor {
  printf("free name\n");
  free_descs($$.vec);
} <name>

%precedence INT_LITERAL CHAR_LITERAL
%precedence '|'
%precedence ID
%precedence '(' '.'

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
    if ($1 != NULL) {
      if (!cmd_add_const(ps, $1->vardecl.id, $1->vardecl.type))
        cmd_eval_stmt(ps, $1);
      stmt_free($1);
    }
  }
}
| var_decl
{
  if (ps->interactive) {
    ps->more = 0;
    if ($1 != NULL) {
      if (!cmd_add_var(ps, $1->vardecl.id, $1->vardecl.type))
        cmd_eval_stmt(ps, $1);
      stmt_free($1);
    }
  } else {
  }
}
| free_var_decl
{
  if (ps->interactive) {
    ps->more = 0;
    if ($1 != NULL) {
      if (!cmd_add_var(ps, $1->vardecl.id, $1->vardecl.type))
        cmd_eval_stmt(ps, $1);
      stmt_free($1);
    }
  }
}
| assignment
{
  if (ps->interactive) {
    ps->more = 0;
    cmd_eval_stmt(ps, $1);
    stmt_free($1);
  }
}
| expr ';'
{
  Stmt *stmt = stmt_from_expr($1);
  if (ps->interactive) {
    ps->more = 0;
    cmd_eval_stmt(ps, stmt);
    stmt_free(stmt);
  } else {
  }
}
| if_stmt
{
  if (ps->interactive) {
    ps->more = 0;
    cmd_eval_stmt(ps, $1);
    stmt_free($1);
  } else {

  }
}
| while_stmt
{
  if (ps->interactive) {
    ps->more = 0;
    cmd_eval_stmt(ps, $1);
    stmt_free($1);
  } else {

  }
}
| match_stmt
{
  if (ps->interactive) {
    ps->more = 0;
  }
}
| for_each_stmt
{
  if (ps->interactive) {
    ps->more = 0;
    cmd_eval_stmt(ps, $1);
    stmt_free($1);
  } else {

  }
}
| func_decl
{
  if (ps->interactive) {
    ps->more = 0;
    if ($1 != NULL) {
      if (!cmd_add_func(ps, $1->funcdecl.id.name, $1->funcdecl.idtypes,
          $1->funcdecl.ret))
        cmd_eval_stmt(ps, $1);
      stmt_free($1);
    }
  }
}
| type_decl
{
  if (ps->interactive) {
    ps->more = 0;
  }
}
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
  if (ps->interactive) {
    if (!ps->quit) {
      syntax_error(ps, row(@1), col(@1), "invalid statement");
      yyclearin;
    }
    ps->more = 0;
    yyerrok;
  } else {
    yyclearin;
    yyerrok;
  }
}
;

local:
  var_decl
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
}
| expr ';'
{
  $$ = stmt_from_expr($1);
}
| if_stmt
{
  $$ = $1;
}
| while_stmt
{
  $$ = $1;
}
| match_stmt
{
  $$ = NULL;
}
| for_each_stmt
{
  $$ = $1;
}
| ';'
{
  $$ = NULL;
}
| COMMENT
{
  $$ = NULL;
}
;

import:
  IMPORT STRING_LITERAL ';'
{
  print("import STRING_LITERAL;\n");
}
| IMPORT ID STRING_LITERAL ';'
| IMPORT '.' STRING_LITERAL ';'
| IMPORT '{' id_as_list '}' STRING_LITERAL ';'
;

id_as_list:
  ID
| ID AS ID
| id_as_list ',' ID
| id_as_list ',' ID AS ID
;

const_decl:
  CONST ID '=' expr ';'
{
  $$ = NULL;
}
| CONST ID type '=' expr ';'
{
  $$ = NULL;
}
;

var_decl:
  VAR ID type ';'
{
  IDENT(id, $2, @2);
  TYPE(type, $3, @3);
  $$ = stmt_from_vardecl(id, &type, NULL);
}
| VAR ID type '=' expr ';'
{
  IDENT(id, $2, @2);
  TYPE(type, $3, @3);
  $$ = stmt_from_vardecl(id, &type, $5);
}
| VAR ID '=' expr ';'
{
  IDENT(id, $2, @2);
  $$ = stmt_from_vardecl(id, NULL, $4);
}
;

free_var_decl:
  ID FREE_ASSIGN expr ';'
{
  IDENT(id, $1, @1);
  $$ = stmt_from_vardecl(id, NULL, $3);
}
;

assignment:
  primary_expr assign_operator expr ';'
{
  $$ = stmt_from_assign($2, $1, $3);
}
;

assign_operator:
  '='
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
| POW_ASSIGN
{
  $$ = OP_POW_ASSIGN;
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
;

return_stmt:
  RETURN ';'
{
  $$ = stmt_from_return(NULL);
}
| RETURN expr ';'
{
  $$ = stmt_from_return($2);
}
;

jump_stmt:
  BREAK ';'
{
  $$ = stmt_from_break(row(@1), col(@1));
}
| CONTINUE ';'
{
  $$ = stmt_from_continue(row(@1), col(@1));
}
;

block:
  '{' local_list '}'
{
  $$ = $2;
}
| '{' expr '}'
{
  $$ = vector_new();
  Stmt *s = stmt_from_expr($2);
  vector_push_back($$, s);
}
| '{' expr2_list '}'
{
  $$ = $2;
}
| '{' expr2_list ';' '}'
{
  $$ = $2;
}
| '{' '}'
{
  $$ = NULL;
}
;

expr2_list:
  expr ',' expr
{
  $$ = vector_new();
  Stmt *s = stmt_from_expr($1);
  vector_push_back($$, s);
  s = stmt_from_expr($3);
  vector_push_back($$, s);
}
| expr2_list ',' expr
{
  $$ = $1;
  Stmt *s = stmt_from_expr($3);
  vector_push_back($$, s);
}
;

local_list:
  local
{
  $$ = vector_new();
  if ($1 != NULL)
    vector_push_back($$, $1);
}
| local_list local
{
  $$ = $1;
  if ($2 != NULL)
    vector_push_back($$, $2);
}
;

expr:
  range_object
{
  $$ = $1;
}
| condition_expr
{
  $$ = $1;
}
| expr_as_type
{
  $$ = $1;
}
| expr_is_type
{
  $$ = $1;
}
;

range_object:
  condition_expr DOTDOTDOT condition_expr
{
  $$ = expr_from_range(0, $1, $3);
}
| condition_expr DOTDOTLESS condition_expr
{
  $$ = expr_from_range(1, $1, $3);
}
;

expr_as_type:
  condition_expr AS type
{
  TYPE(type, $3, @3);
  $$ = expr_from_astype($1, type);
}
;

expr_is_type:
  condition_expr IS type
{
  TYPE(type, $3, @3);
  $$ = expr_from_istype($1, type);
}
;

condition_expr:
  logic_or_expr
{
  $$ = $1;
}
| logic_or_expr '?' logic_or_expr ':' logic_or_expr
{
  $$ = expr_from_ternary($1, $3, $5);
}
;

logic_or_expr:
  logic_and_expr
{
  $$ = $1;
}
| logic_or_expr OP_OR logic_and_expr
{
  $$ = expr_from_binary(BINARY_LOR, $1, $3);
}
;

logic_and_expr:
  inclusive_or_expr
{
  $$ = $1;
}
| logic_and_expr OP_AND inclusive_or_expr
{
  $$ = expr_from_binary(BINARY_LAND, $1, $3);
}
;

inclusive_or_expr:
  exclusive_or_expr
{
  $$ = $1;
}
| inclusive_or_expr '|' exclusive_or_expr
{
  $$ = expr_from_binary(BINARY_BIT_OR, $1, $3);
}
;

exclusive_or_expr:
  and_expr
{
  $$ = $1;
}
| exclusive_or_expr '^' and_expr
{
  $$ = expr_from_binary(BINARY_BIT_XOR, $1, $3);
}
;

and_expr:
  equality_expr
{
  $$ = $1;
}
| and_expr '&' equality_expr
{
  $$ = expr_from_binary(BINARY_BIT_AND, $1, $3);
}
;

equality_expr:
  relation_expr
{
  $$ = $1;
}
| equality_expr OP_EQ relation_expr
{
  $$ = expr_from_binary(BINARY_EQ, $1, $3);
}
| equality_expr OP_NE relation_expr
{
  $$ = expr_from_binary(BINARY_NEQ, $1, $3);
}
;

relation_expr:
  add_expr
{
  $$ = $1;
}
| relation_expr '<' add_expr
{
  $$ = expr_from_binary(BINARY_LT, $1, $3);
}
| relation_expr '>' add_expr
{
  $$ = expr_from_binary(BINARY_GT, $1, $3);
}
| relation_expr OP_LE add_expr
{
  $$ = expr_from_binary(BINARY_LE, $1, $3);
}
| relation_expr OP_GE add_expr
{
  $$ = expr_from_binary(BINARY_GE, $1, $3);
}
;

add_expr:
  multi_expr
{
  $$ = $1;
}
| add_expr '+' multi_expr
{
  $$ = expr_from_binary(BINARY_ADD, $1, $3);
}
| add_expr '-' multi_expr
{
  $$ = expr_from_binary(BINARY_SUB, $1, $3);
}
;

multi_expr:
  unary_expr
{
  $$ = $1;
}
| multi_expr '*' unary_expr
{
  $$ = expr_from_binary(BINARY_MULT, $1, $3);
}
| multi_expr '/' unary_expr
{
  $$ = expr_from_binary(BINARY_DIV, $1, $3);
}
| multi_expr '%' unary_expr
{
  $$ = expr_from_binary(BINARY_MOD, $1, $3);
}
| multi_expr OP_POWER unary_expr
{
  $$ = expr_from_binary(BINARY_POW, $1, $3);
}
;

unary_expr:
  primary_expr
{
  $$ = $1;
}
| unary_operator unary_expr
{
  $$ = expr_from_unary($1, $2);
}
;

unary_operator:
  '+'
{
  $$ = UNARY_PLUS;
}
| '-'
{
  $$ = UNARY_NEG;
}
| '~'
{
  $$ = UNARY_BIT_NOT;
}
| OP_NOT
{
  $$ = UNARY_LNOT;
}
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
| primary_expr '(' expr_list ')'
{
  $$ = expr_from_call($3, $1);
}
| primary_expr '(' expr_list ';' ')'
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
  primary_expr '[' expr ']'
{
  $$ = expr_from_subscr($1, $3);
}
| primary_expr '[' expr ':' expr ']'
{
  $$ = expr_from_slice($1, $3, $5);
}
| primary_expr '[' ':' expr ']'
{
  $$ = expr_from_slice($1, $4, NULL);
}
| primary_expr '[' expr ':' ']'
{
  $$ = expr_from_slice($1, $3, NULL);
}
| primary_expr '[' ':' ']'
{
  $$ = expr_from_slice($1, NULL, NULL);
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
  $$ = expr_from_super();
  set_expr_pos($$, @1);
}
| '(' expr ')'
{
  $$ = $2;
}
| tuple_object
{
  $$ = $1;
}
| array_object
{
  $$ = $1;
}
| map_object
{
  $$ = $1;
}
| anony_func
{
  $$ = $1;
}
| new_object
{
  $$ = $1;
}
;

tuple_object:
  '(' expr_list ',' ')'
{
  $$ = expr_from_tuple($2);
}
| '(' expr_list ',' expr ')'
{
  vector_push_back($2, $4);
  $$ = expr_from_tuple($2);
}
| '(' expr_list ',' expr ';' ')'
{
  vector_push_back($2, $4);
  $$ = expr_from_tuple($2);
}
| '(' ')'
{
  $$ = expr_from_tuple(NULL);
}
;

array_object:
  '[' expr_list ']'
{
  $$ = expr_from_array($2);
}
| '[' expr_list ',' ']'
{
  $$ = expr_from_array($2);
}
| '[' expr_list ';' ']'
{
  /* auto insert semicolon */
  $$ = expr_from_array($2);
}
| '[' ']'
{
  $$ = expr_from_array(NULL);
}
;

expr_list:
  expr
{
  $$ = vector_new();
  vector_push_back($$, $1);
}
| expr_list ',' expr
{
  $$ = $1;
  vector_push_back($$, $3);
}
;

map_object:
  '{' mapentry_list '}'
{
  $$ = expr_from_map($2);
}
| '{' mapentry_list ',' '}'
{
  $$ = expr_from_map($2);
}
| '{' mapentry_list ';' '}'
{
  /* auto insert semicolon */
  $$ = expr_from_map($2);
}
| '{' ':' '}'
{
  $$ = expr_from_map(NULL);
}
;

mapentry_list:
  mapentry
{
  $$ = vector_new();
  vector_push_back($$, $1);
}
| mapentry_list ',' mapentry
{
  $$ = $1;
  vector_push_back($$, $3);
}
;

mapentry:
  expr ':' expr
{
  $$ = new_mapentry($1, $3);
}
;

anony_func:
  FUNC '(' para_list ')' type block
{
  TYPE(type, $5, @5);
  $$ = expr_from_anony($3, &type, $6);
}
| FUNC '(' para_list ')' block
{
  $$ = expr_from_anony($3, NULL, $5);
}
| FUNC '(' ')' type block
{
  TYPE(type, $4, @4);
  $$ = expr_from_anony(NULL, &type, $5);
}
| FUNC '(' ')' block
{
  $$ = expr_from_anony(NULL, NULL, $4);
}
;

new_object:
  NEW ID '(' ')'
{
  IDENT(id, $2, @2);
  $$ = expr_from_object(NULL, id, NULL, NULL);
}
| NEW ID '.' ID '(' ')'
{
  $$ = NULL;
}
| NEW ID '<' type_list '>' '(' ')'
{
  IDENT(id, $2, @2);
  $$ = expr_from_object(NULL, id, $4, NULL);
}
| NEW ID '.' ID '<' type_list '>' '(' ')'
{
  $$ = NULL;
}
| NEW ID '(' expr_list ')'
{
  IDENT(id, $2, @2);
  $$ = expr_from_object(NULL, id, NULL, $4);
}
| NEW ID '.' ID '(' expr_list ')'
{
  $$ = NULL;
}
| NEW ID '<' type_list '>' '(' expr_list ')'
{
  IDENT(id, $2, @2);
  $$ = expr_from_object(NULL, id, $4, $7);
}
| NEW ID '.' ID '<' type_list '>' '(' expr_list ')'
{
  $$ = NULL;
}
;

if_stmt:
  IF expr block empty_else
{
  $$ = stmt_from_if($2, $3, $4);
}
;

empty_else:
  %empty
{
  $$ = NULL;
}
| ELSE block
{
  $$ = stmt_from_block($2);
}
| ELSE if_stmt
{
  $$ = $2;
}
;

while_stmt:
  WHILE expr block
{
  $$ = stmt_from_while($2, $3);
}
| WHILE block
{
  $$ = stmt_from_while(NULL, $2);
}
;

for_each_stmt:
  FOR expr IN expr block
{
  $$ = stmt_from_for($2, $4, NULL, $5);
}
| FOR expr IN expr BY expr block
{
  $$ = stmt_from_for($2, $4, $6, $7);
}
;

match_stmt:
  MATCH expr '{' match_clauses '}'
;

match_clauses:
  match_clause
| match_clauses match_clause
;

match_clause:
  match_pattern FAT_ARROW match_block_expr ';'
;

match_pattern:
  '_'
| INT_LITERAL
| int_seq INT_LITERAL
| CHAR_LITERAL
| char_seq CHAR_LITERAL
| STRING_LITERAL
| TRUE
| FALSE
| range_object
| tuple_object
| IS type
| ID
| ID '(' id_list ')'
| ID '.' ID
| ID '.' ID '(' id_list ')'
;

int_seq:
  INT_LITERAL '|'
| int_seq INT_LITERAL '|'
;

char_seq:
  CHAR_LITERAL '|'
| char_seq CHAR_LITERAL '|'
;

id_list:
  ID
| id_list ',' ID
;

match_block_expr:
  block
| expr
;

func_decl:
  FUNC name '(' para_list ')' type block
{
  TYPE(type, $6, @6);
  $$ = stmt_from_funcdecl($2.id, $2.vec, $4, &type, $7);
}
| FUNC name '(' para_list ')' block
{
  $$ = stmt_from_funcdecl($2.id, $2.vec, $4, NULL, $6);
}
| FUNC name '(' ')' type block
{
  TYPE(type, $5, @5);
  $$ = stmt_from_funcdecl($2.id, $2.vec, NULL, &type, $6);
}
| FUNC name '(' ')' block
{
  $$ = stmt_from_funcdecl($2.id, $2.vec, NULL, NULL, $5);
}
;

name:
  ID
{
  IDENT(id, $1, @1);
  IdParaDef idpara = {id, NULL};
  $$ = idpara;
}
| ID '<' type_para_list '>'
{
  IDENT(id, $1, @1);
  IdParaDef idpara = {id, $3};
  $$ = idpara;
}
;

type_para_list:
  type_para
{
  $$ = vector_new();
  vector_push_back($$, $1);
}
| type_para_list ',' type_para
{
  $$ = $1;
  vector_push_back($$, $3);
}
;

type_para:
  ID
{
  $$ = desc_from_paradef($1, NULL);
}
| ID ':' type_upbound_list
{
  $$ = desc_from_paradef($1, $3);
}
;

type_upbound_list:
  klass_type
{
  $$ = vector_new();
  vector_push_back($$, $1);
}
| type_upbound_list '&' klass_type
{
  $$ = $1;
  vector_push_back($$, $3);
}
;

para_list:
  id_type_list
{
  $$ = $1;
}
| id_varg
{
  $$ = vector_new();
  vector_push_back($$, $1);
}
| id_type_list ',' id_varg
{
  $$ = $1;
  vector_push_back($$, $3);
}
;

id_type_list:
  ID type
{
  IDENT(id, $1, @1);
  TYPE(type, $2, @2);
  $$ = vector_new();
  vector_push_back($$, new_idtype(id, type));
}
| id_type_list ',' ID type
{
  $$ = $1;
  IDENT(id, $3, @3);
  TYPE(type, $4, @4);
  vector_push_back($$, new_idtype(id, type));
}
;

id_varg:
  ID DOTDOTDOT
{
  IDENT(id, $1, @1);
  Type type = {desc_from_any, 0, 0};
  $$ = new_idtype(id, type);
}
| ID DOTDOTDOT no_array_type
{
  IDENT(id, $1, @1);
  TYPE(type, $3, @3);
  $$ = new_idtype(id, type);
}
;

type_decl:
  CLASS name extends '{' members '}'
| CLASS name extends ';'
| TRAIT name extends '{' members '}'
| TRAIT name extends ';'
| ENUM  name '{' enum_members '}'
;

extends:
  %empty
| ':' bases
;

bases:
  klass_type
| bases ',' klass_type
;

members:
  %empty
| member_decls
;

member_decls:
  member_decl
| member_decls member_decl
;

member_decl:
  field_decl
| method_decl
| proto_decl
| ';'
;

field_decl:
  ID type ';'
| ID '=' expr ';'
| ID type '=' expr ';'
;

method_decl:
  FUNC ID '(' para_list')' type block
| FUNC ID '(' para_list ')' block
| FUNC ID '(' ')' type block
| FUNC ID '(' ')' block
;

proto_decl:
  FUNC ID '(' type_varg_list ')' type ';'
| FUNC ID '(' para_list')' type ';'
| FUNC ID '(' type_varg_list ')' ';'
| FUNC ID '(' para_list ')' ';'
| FUNC ID '(' ')' type ';'
| FUNC ID '(' ')' ';'
;

enum_members:
  enum_labels
| enum_labels ','
| enum_labels ';'
| enum_labels enum_methods
| enum_labels ',' enum_methods
| enum_labels ';' enum_methods
;

enum_labels:
  enum_label
| enum_labels ',' enum_label
;

enum_label:
  ID
| ID '(' type_list ')'
| ID '=' INT_LITERAL
| ID '=' '-' INT_LITERAL
;

enum_methods:
  method_decl
| method_decl ';'
| enum_methods method_decl
| enum_methods method_decl ';'
;

type_list:
  type
{
  $$ = vector_new();
  vector_push_back($$, $1);
}
| type_list ',' type
{
  $$ = $1;
  vector_push_back($$, $3);
}
;

type:
  no_array_type
{
  $$ = $1;
}
| '[' type ']'
{
  $$ = desc_from_array;
  desc_add_paratype($$, $2);
  TYPE_DECREF($2);
}
;

no_array_type:
  BYTE
{
  $$ = desc_from_byte;
}
| INTEGER
{
  $$ = desc_from_int;
}
| FLOAT
{
  $$ = desc_from_float;
}
| CHAR
{
  $$ = desc_from_char;
}
| STRING
{
  $$ = desc_from_str;
}
| BOOL
{
  $$ = desc_from_bool;
}
| ANY
{
  $$ = desc_from_any;
}
| '[' type ':' type ']'
{
  $$ = desc_from_map;
  desc_add_paratype($$, $2);
  desc_add_paratype($$, $4);
  TYPE_DECREF($2);
  TYPE_DECREF($4);
}
| '(' type_list ')'
{
  $$ = desc_from_tuple;
  TypeDesc *item;
  vector_for_each(item, $2) {
    desc_add_paratype($$, item);
  }
  free_descs($2);
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

klass_type:
  ID
{
  $$ = desc_from_klass(NULL, $1);
}
| ID '.' ID
{
  $$ = NULL;
}
| ID '<' type_list '>'
{
  $$ = desc_from_klass(NULL, $1);
  $$->types = $3;
}
| ID '.' ID '<' type_list '>'
{
  $$ = NULL;
}
;

func_type:
  FUNC '(' type_varg_list ')' type
{
  $$ = desc_from_proto($3, $5);
  TYPE_DECREF($5);
}
| FUNC '(' type_varg_list ')'
{
  $$ = desc_from_proto($3, NULL);
}
| FUNC '(' ')' type
{
  $$ = desc_from_proto(NULL, $4);
  TYPE_DECREF($4);
}
| FUNC '(' ')'
{
  $$ = desc_from_proto(NULL, NULL);
}
;

type_varg_list:
  type_list
{
  $$ = $1;
}
| type_list ',' DOTDOTDOT
{
  $$ = $1;
  TypeDesc *varg = desc_from_varg;
  TypeDesc *any = desc_from_any;
  desc_add_paratype(varg, any);
  TYPE_DECREF(any);
  vector_push_back($$, varg);
}
| type_list ',' DOTDOTDOT no_array_type
{
  $$ = $1;
  TypeDesc *varg = desc_from_varg;
  desc_add_paratype(varg, $4);
  TYPE_DECREF($4);
  vector_push_back($$, varg);
}
;

%%

/* epilogue */
