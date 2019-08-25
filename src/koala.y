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

#define TYPE(name, type, loc) \
  Type name = {type, yyloc_row(loc), yyloc_col(loc)}

/* interactive mode */
void Cmd_EvalStmt(ParserState *ps, Stmt *stmt);
void Cmd_Add_Var(Ident id, Type type);
void Cmd_Add_Func(Ident id, Type type);

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
  TypeDesc *desc;
  AssignOpKind assginop;
  UnaryOpKind unaryop;
  Ident name;
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

%type <stmt> var_decl
%type <stmt> free_var_decl
%type <stmt> assignment
%type <assginop> assign_operator
%type <stmt> func_decl
%type <stmt> block
%type <list> local_list
%type <stmt> local
%type <name> name

%type <expr> kv
%type <list> kv_list
%type <expr> map_object
%type <expr> array_object
%type <expr> tuple_object
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
%type <unaryop> unary_operator

%type <list> type_list
%type <desc> type
%type <desc> no_array_type

%destructor { printf("free expr\n"); expr_free($$); } <expr>
%destructor { printf("free stmt\n"); stmt_free($$); } <stmt>
%destructor { printf("decref desc\n"); TYPE_DECREF($$); } <desc>

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
    ps->errnum = 0;
  }
}
| const_decl
{
  if (ps->interactive) {
    ps->more = 0;
    ps->errnum = 0;
  }
}
| var_decl
{
  if (ps->interactive) {
    ps->more = 0;
    if ($1 != NULL) {
      Cmd_Add_Var($1->vardecl.id, $1->vardecl.type);
      Cmd_EvalStmt(ps, $1);
      stmt_free($1);
    }
    ps->errnum = 0;
  } else {
  }
}
| free_var_decl
{
  if (ps->interactive) {
    ps->more = 0;
    if ($1 != NULL) {
      Cmd_Add_Var($1->vardecl.id, $1->vardecl.type);
      Cmd_EvalStmt(ps, $1);
      stmt_free($1);
    }
    ps->errnum = 0;
  }
}
| assignment
{
  if (ps->interactive) {
    ps->more = 0;
    Cmd_EvalStmt(ps, $1);
    stmt_free($1);
    ps->errnum = 0;
  }
}
| expr ';'
{
  Stmt *stmt = stmt_from_expr($1);
  if (ps->interactive) {
    ps->more = 0;
    Cmd_EvalStmt(ps, stmt);
    stmt_free(stmt);
    ps->errnum = 0;
  } else {
  }
}
| func_decl
{
  if (ps->interactive) {
    ps->more = 0;
    if ($1 != NULL) {
      Type type = {NULL};
      Cmd_Add_Func($1->funcdecl.id, type);
      Cmd_EvalStmt(ps, $1);
      stmt_free($1);
    }
    ps->errnum = 0;
  }
}
  /*
| type_decl
  */
| ';'
{
  if (ps->interactive) {
    ps->more = 0;
    ps->errnum = 0;
  }
}
| COMMENT
{
  if (ps->interactive) {
    ps->more = 0;
    ps->errnum = 0;
  }
}
| DOC
{
  if (ps->interactive) {
    ps->more = 0;
    ps->errnum = 0;
  }
}
| MODDOC
{
  if (ps->interactive) {
    ps->more = 0;
    ps->errnum = 0;
  }
}
| error
{
  if (ps->interactive) {
    if (!ps->quit) {
      syntax_error(ps, yyloc_row(@1), yyloc_col(@1), "invalid statement");
      yyclearin;
    }
    ps->more = 0;
    ps->errnum = 0;
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
| return_expr
{
  $$ = NULL;
}
| jump_expr
{
  $$ = NULL;
}
| block
{
  $$ = $1;
}
| expr ';'
{
  $$ = stmt_from_expr($1);
}
| ';'
{
  $$ = NULL;
}
| COMMENT
{
  $$ = NULL;
}
| error
{
  if (ps->interactive) {
    syntax_error(ps, yyloc_row(@1), yyloc_col(@1), "invalid local statement");
    ps->more = 0;
    ps->errnum = 0;
  }
  yyclearin;
  yyerrok;
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
  CONST ID '=' basic_expr ';'
| CONST ID type '=' basic_expr ';'
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

return_expr:
  RETURN ';'
| RETURN expr ';'
;

jump_expr:
  BREAK ';'
| CONTINUE ';'
;

block:
  '{' local_list '}'
{
  $$ = stmt_from_block($2);
}
| '{' basic_expr '}'
{
  Stmt *stmt = stmt_from_expr($2);
  Vector *vec = vector_new();
  vector_push_back(vec, stmt);
  $$ = stmt_from_block(vec);
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
  $$ = expr_from_subScript($1, $3);
}
| primary_expr '[' basic_expr ':' basic_expr ']'
{
  $$ = expr_from_slice($1, $3, $5);
}
| primary_expr '[' ':' basic_expr ']'
{
  $$ = expr_from_slice($1, $4, NULL);
}
| primary_expr '[' basic_expr ':' ']'
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
| '(' basic_expr ')'
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
| anony_object
{
  $$ = NULL;
}
;

tuple_object:
  '(' basic_expr_list ',' ')'
{
  $$ = expr_from_tuple($2);
}
| '(' basic_expr_list ',' basic_expr ')'
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
  '[' basic_expr_list ']'
{
  $$ = expr_from_array($2);
}
| '[' basic_expr_list ',' ']'
{
  $$ = expr_from_array($2);
}
| '[' ']'
{
  $$ = expr_from_array(NULL);
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
  $$ = expr_from_map($2);
}
| '{' kv_list ',' '}'
{
  $$ = expr_from_map($2);
}
| '{' ':' '}'
{
  $$ = expr_from_map(NULL);
}
;

kv_list:
  kv
{
  $$ = vector_new();
  vector_push_back($$, $1);
}
| kv_list ',' kv
{
  $$ = $1;
  vector_push_back($$, $3);
}
;

kv:
  basic_expr ':' basic_expr
{
  $$ = expr_from_mapentry($1, $3);
}
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
{
  TYPE(type, $6, @6);
  $$ = stmt_from_funcdecl($2, NULL, NULL, &type, $7);
}
| FUNC name '(' para_list ')' block
{
  $$ = stmt_from_funcdecl($2, NULL, NULL, NULL, $6);
}
| FUNC name '(' ')' type block
{
  TYPE(type, $5, @5);
  $$ = stmt_from_funcdecl($2, NULL, NULL, &type, $6);
}
| FUNC name '(' ')' block
{
  $$ = stmt_from_funcdecl($2, NULL, NULL, NULL, $5);
}
;

name:
  ID
{
  IDENT(id, $1, @1);
  $$ = id;
}
| ID '<' type_para_list '>'
{
  IDENT(id, $1, @1);
  $$ = id;
}
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
  $$ = NULL;
  // desc_from_array();
}
;

no_array_type:
  BYTE
{
  $$ = desc_from_byte();
}
| INTEGER
{
  $$ = desc_from_integer();
}
| FLOAT
{
  $$ = desc_from_float();
}
| CHAR
{
  $$ = desc_from_char();
}
| STRING
{
  $$ = desc_from_string();
}
| BOOL
{
  $$ = desc_from_bool();
}
| ANY
{
  $$ = desc_from_any();
}
| '[' type ':' type ']'
{
  $$ = NULL;
}
| '(' type_list ')'
{
  $$ = NULL;
}
| klass_type
{
  $$ = NULL;
}
| func_type
{
  $$ = NULL;
}
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
;

type_varg_list:
  type_list
| type_list ',' DOTDOTDOT
| type_list ',' DOTDOTDOT no_array_type
;

%%

/* epilogue */
