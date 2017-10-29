
%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "compile.h"

int yyerror(const char *str);
int yylex(void);

%}

%union {
  char *id;
  int64_t ival;
  float64_t fval;
  char *string_const;
  int dims;
  int primitive;
  struct expr *expr;
  struct sequence *sequence;
  struct stmt *stmt;
  struct type *type;
  int operator;
  struct field *field;
  struct intf_func *intf_func;
  struct test_block *testblock;
}

%token ELLIPSIS

/* 算术运算和位运算 */
%token TYPELESS_ASSIGN
%token PLUS_ASSGIN
%token MINUS_ASSIGN
%token MULT_ASSIGN
%token DIV_ASSIGN
%token MOD_ASSIGN
%token AND_ASSIGN
%token OR_ASSIGN
%token XOR_ASSIGN
%token RSHIFT_ASSIGN
%token LSHIFT_ASSIGN

%token EQ
%token NE
%token GE
%token LE
%token AND
%token OR
%token NOT
%token LSHIFT
%token RSHIFT

%token IF
%token ELSE
%token WHILE
%token DO
%token FOR
%token IN
%token SWITCH
%token CASE
%token BREAK
%token FALLTHROUGH
%token CONTINUE
%token DEFAULT
%token VAR
%token FUNC
%token RETURN
%token STRUCT
%token INTERFACE
%token CONST
%token IMPORT
%token AS
%token GO
%token DEFER
%token NEWLINE

%token CHAR
%token BYTE
%token SHORT
%token INTEGER
%token FLOAT
%token BOOL
%token STRING
%token ANY
%token <dims> DIMS

%token SELF
%token TOKEN_NULL
%token TOKEN_TRUE
%token TOKEN_FALSE

%token <ival> BYTE_CONST
%token <ival> CHAR_CONST
%token <ival> INT_CONST
%token <ival> HEX_CONST
%token <ival> OCT_CONST
%token <fval> FLOAT_CONST
%token <string_const> STRING_CONST

%token <id> ID

/*--------------------------------------------------------------------------*/

%precedence ID
%precedence '.'
%precedence ')'
%precedence '('

/*--------------------------------------------------------------------------*/
%type <primitive> PrimitiveType
%type <type> UserDefinedType
%type <type> BaseType
%type <type> Type
%type <type> FunctionType
%type <type> TypeName
%type <sequence> TypeNameListOrEmpty
%type <sequence> TypeNameList
%type <sequence> ReturnTypeList
%type <sequence> TypeList
%type <sequence> ParameterList
%type <sequence> ParameterListOrEmpty
%type <operator> UnaryOperator
%type <operator> CompoundAssignOperator
%type <sequence> VariableList
%type <sequence> ExpressionList
%type <sequence> PrimaryExpressionList

%type <expr> Expression
%type <expr> LogicalOrExpression
%type <expr> LogicalAndExpression
%type <expr> InclusiveOrExpression
%type <expr> ExclusiveOrExpression
%type <expr> AndExpression
%type <expr> EqualityExpression
%type <expr> RelationalExpression
%type <expr> ShiftExpression
%type <expr> AdditiveExpression
%type <expr> MultiplicativeExpression
%type <expr> UnaryExpression
%type <expr> PrimaryExpression
%type <expr> Atom
%type <expr> ArrayDeclaration
%type <expr> AnonymousFunctionDeclaration
%type <expr> CONSTANT
%type <sequence> DimExprList
%type <sequence> ArrayInitializerList
%type <expr> ArrayInitializer
%type <sequence> Imports
%type <sequence> ModuleStatements
%type <stmt> Import
%type <stmt> ModuleStatement
%type <stmt> Statement
%type <stmt> ConstDeclaration
%type <stmt> VariableDeclaration
%type <stmt> Assignment
%type <stmt> IfStatement
%type <sequence> ElseIfStatements
%type <testblock> ElseIfStatement
%type <testblock> OptionELSE
%type <stmt> WhileStatement
%type <stmt> SwitchStatement
%type <sequence> CaseStatements
%type <testblock> CaseStatement
%type <stmt> ForStatement
%type <stmt> ForInit
%type <stmt> ForTest
%type <stmt> ForIncr
%type <sequence> Block
%type <stmt> GoStatement
%type <sequence> LocalStatements
%type <stmt> ReturnStatement
%type <stmt> JumpStatement
%type <stmt> FunctionDeclaration
%type <stmt> TypeDeclaration
%type <sequence> FieldDeclarations
%type <field> FieldDeclaration
%type <sequence> IntfFuncDecls
%type <intf_func> IntfFuncDecl

%start CompileUnit

%%

Type
  : BaseType {
    $$ = $1;
  }
  | DIMS BaseType {
    $2->dims = $1;
    $$ = $2;
  }
  ;

BaseType
  : PrimitiveType {
    $$ = type_from_primitive($1);
  }
  | UserDefinedType {
    $$ = $1;
  }

  | FunctionType {
    $$ = $1;
  }
  ;

PrimitiveType
  : CHAR {
    $$ = TYPE_CHAR;
  }
  | BYTE {
    $$ = TYPE_BYTE;
  }
  | SHORT {
    $$ = TYPE_SHORT;
  }
  | INTEGER {
    $$ = TYPE_INT;
  }
  | FLOAT {
    $$ = TYPE_FLOAT;
  }
  | BOOL {
    $$ = TYPE_BOOL;
  }
  | STRING {
    $$ = TYPE_STRING;
  }
  | ANY {
    $$ = TYPE_ANY;
  }
  ;

UserDefinedType
  : ID {
    // type in local module
    $$ = type_from_userdef(NULL, $1);
  }
  | ID '.' ID {
    // type in external module
    $$ = type_from_userdef($1, $3);
  }
  ;

FunctionType
  : FUNC '(' TypeNameListOrEmpty ')' ReturnTypeList {
    $$ = type_from_functype($3, $5);
  }
  | FUNC '(' TypeNameListOrEmpty ')' {
    $$ = type_from_functype($3, NULL);
  }
  ;

TypeNameListOrEmpty
  : TypeNameList {
    $$ = $1;
  }
  | %empty {
    $$ = NULL;
  }
  ;

TypeNameList
  : TypeName {
    $$ = seq_new();
    seq_append($$, $1);
  }
  | TypeNameList ',' TypeName {
    seq_append($1, $3);
    $$ = $1;
  }
  ;

TypeName
  : ID Type {
    $$ = $2;
  }
  | Type {
    $$ = $1;
  }
  ;

ReturnTypeList
  : Type {
    $$ = seq_new();
    seq_append($$, $1);
  }
  | '(' TypeList ')' {
    $$ = $2;
  }
  ;

TypeList
  : Type {
    $$ = seq_new();
    seq_append($$, $1);
  }
  | TypeList ',' Type {
    seq_append($1, $3);
    $$ = $1;
  }
  ;

/*--------------------------------------------------------------------------*/
CompileUnit
  : Imports ModuleStatements {
    ast_traverse($1);
    ast_traverse($2);
    //compiler_module($1);
    //compiler_module($2);
  }
  | ModuleStatements {
    ast_traverse($1);
    //compiler_module($1);
  }
  ;

Imports
  : Import {
    $$ = seq_new();
    seq_append($$, $1);
  }
  | Imports Import {
    seq_append($1, $2);
    $$ = $1;
  }
  ;

Import
  : IMPORT STRING_CONST ';' {
    $$ = stmt_from_import(NULL, $2);
  }
  | IMPORT ID STRING_CONST ';' {
    $$ = stmt_from_import($2, $3);
  }
  ;

ModuleStatements
  : ModuleStatement {
    $$ = seq_new();
    seq_append($$, $1);
  }
  | ModuleStatements ModuleStatement {
    seq_append($1, $2);
    $$ = $1;
  }
  ;

/*--------------------------------------------------------------------------*/
ModuleStatement
  : Statement {
    $$ = $1;
  }
  | ConstDeclaration ';' {
    $$ = $1;
  }
  | FunctionDeclaration {
    $$ = $1;
  }
  | TypeDeclaration {
    $$ = $1;
  }
  ;

Statement
  : ';' {
    $$ = stmt_from_empty();
  }
  | Expression ';' {
    $$ = stmt_from_expr($1);
  }
  | VariableDeclaration ';' {
    $$ = $1;
  }
  | Assignment ';' {
    $$ = $1;
  }
  | IfStatement {
    $$ = $1;
  }
  | WhileStatement {
    $$ = $1;
  }
  | SwitchStatement {
    $$ = $1;
  }
  | ForStatement {
    $$ = $1;
  }
  | JumpStatement {
    $$ = $1;
  }
  | ReturnStatement {
    $$ = $1;
  }
  | Block {
    $$ = stmt_from_block($1);
  }
  | GoStatement {
    $$ = $1;
  }
  ;

/*--------------------------------------------------------------------------*/

ConstDeclaration
  : CONST VariableList '=' ExpressionList {
    $$ = stmt_from_vardecl($2, $4, 1, NULL);
  }
  | CONST VariableList Type '=' ExpressionList {
    $$ = stmt_from_vardecl($2, $5, 1, $3);
  }
  ;

VariableDeclaration
  : VAR VariableList Type {
    $$ = stmt_from_vardecl($2, NULL, 0, $3);
  }
  | VAR VariableList '=' ExpressionList {
    $$ = stmt_from_vardecl($2, $4, 0, NULL);
  }
  | VAR VariableList Type '=' ExpressionList {
    $$ = stmt_from_vardecl($2, $5, 0, $3);
  }
  ;

VariableList
  : ID {
    $$ = seq_new();
    seq_append($$, expr_from_name($1));
  }
  | VariableList ',' ID {
    seq_append($1, expr_from_name($3));
    $$ = $1;
  }
  ;

/*--------------------------------------------------------------------------*/

FunctionDeclaration
  : FUNC ID '(' ParameterListOrEmpty ')' Block {
    $$ = stmt_from_funcdecl(NULL, $2, $4, NULL, $6);
  }
  | FUNC ID '(' ParameterListOrEmpty ')' ReturnTypeList Block {
    $$ = stmt_from_funcdecl(NULL, $2, $4, $6, $7);
  }
  | FUNC ID '.' ID '(' ParameterListOrEmpty ')' Block {
    $$ = stmt_from_funcdecl($2, $4, $6, NULL, $8);
  }
  | FUNC ID '.' ID '(' ParameterListOrEmpty ')' ReturnTypeList Block {
    $$ = stmt_from_funcdecl($2, $4, $6, $8, $9);
  }
  ;

ParameterList
  : ID Type {
    $$ = seq_new();
    seq_append($$, expr_from_name_type($1, $2));
  }
  | ParameterList ',' ID Type {
    seq_append($$, expr_from_name_type($3, $4));
    $$ = $1;
  }
  ;

ParameterListOrEmpty
  : ParameterList {
    $$ = $1;
  }
  | %empty {
    $$ = NULL;
  }
  ;

/*--------------------------------------------------------------------------*/

TypeDeclaration
  : STRUCT ID '{' FieldDeclarations '}' {
    $$ = stmt_from_structure($2, $4);
  }
  | INTERFACE ID '{' IntfFuncDecls '}' {
    $$ = stmt_from_interface($2, $4);
  }
  ;

FieldDeclarations
  : FieldDeclaration {
    $$ = seq_new();
    seq_append($$, $1);
  }
  | FieldDeclarations FieldDeclaration {
    seq_append($1, $2);
    $$ = $1;
  }
  ;

FieldDeclaration
  : ID Type ';' {
    $$ = new_struct_field($1, $2, NULL);
  }
  | ID Type '=' Expression ';' {
    $$ = new_struct_field($1, $2, $4);
  }
  ;

IntfFuncDecls
  : IntfFuncDecl {
    $$ = seq_new();
    seq_append($$, $1);
  }
  | IntfFuncDecls IntfFuncDecl {
    seq_append($1, $2);
    $$ = $1;
  }
  ;

IntfFuncDecl
  : ID '(' TypeNameListOrEmpty ')' ReturnTypeList ';'{
    $$ = new_intf_func($1, $3, $5);
  }
  | ID '(' TypeNameListOrEmpty ')' ';' {
    $$ = new_intf_func($1, $3, NULL);
  }
  ;

/*--------------------------------------------------------------------------*/

Block
  : '{' LocalStatements '}' {
    $$ = $2;
  }
  | '{' '}' {
    $$ = NULL;
  }
  ;

LocalStatements
  : Statement {
    $$ = seq_new();
    seq_append($$, $1);
  }
  | LocalStatements Statement {
    seq_append($1, $2);
    $$ = $1;
  }
  ;

/*--------------------------------------------------------------------------*/

GoStatement
  : GO PrimaryExpression '(' ExpressionList ')' ';' {
    /*$$ = stmt_from_go(expr_from_atom_trailers($3, $2));
    free_clist($3);*/
  }
  | GO PrimaryExpression '(' ')' ';' {

  }
  ;

/*--------------------------------------------------------------------------*/

IfStatement
  : IF '(' Expression ')' Block OptionELSE {
    $$ = stmt_from_if(new_test_block($3, $5), NULL, $6);
  }
  | IF '(' Expression ')' Block ElseIfStatements OptionELSE {
    $$ = stmt_from_if(new_test_block($3, $5), $6, $7);
  }
  ;

ElseIfStatements
  : ElseIfStatement {
    $$ = seq_new();
    seq_append($$, $1);
  }
  | ElseIfStatements ElseIfStatement {
    seq_append($1, $2);
    $$ = $1;
  }
  ;

ElseIfStatement
  : ELSE IF '(' Expression ')' Block {
    $$ = new_test_block($4, $6);
  }
  ;

OptionELSE
  : ELSE Block {
    $$ = new_test_block(NULL, $2);
  }
  | %empty {
    $$ = NULL;
  }
  ;

/*--------------------------------------------------------------------------*/

WhileStatement
  : WHILE '(' Expression ')' Block {
    $$ = stmt_from_while($3, $5, 0);
  }
  | DO Block WHILE '(' Expression ')' {
    $$ = stmt_from_while($5, $2, 1);
  }
  ;

/*--------------------------------------------------------------------------*/

SwitchStatement
  : SWITCH '(' Expression ')' '{' CaseStatements '}' {
    $$ = stmt_from_switch($3, $6);
  }
  ;

CaseStatements
  : CaseStatement {
    $$ = seq_new();
    seq_append($$, $1);
  }
  | CaseStatements CaseStatement {
    if ($2->test == NULL) {
      /* default case */
      //clist_add(&($2)->link, $1);
      //seq_append($1, $2);
      struct test_block *tb = seq_get($1, 0);
      if (tb != NULL && tb->test == NULL) {
        fprintf(stderr, "[ERROR] default case needs only one\n");
        exit(0);
      } else {
        seq_insert($1, 0, $2);
      }
    } else {
      seq_append($1, $2);
    }
    $$ = $1;
  }
  ;

CaseStatement
  : CASE Expression ':' Block {
    $$ = new_test_block($2, $4);
  }
  | DEFAULT ':' Block {
    $$ = new_test_block(NULL, $3);
  }
  ;

/*--------------------------------------------------------------------------*/

ForStatement
  : FOR '(' ForInit ';' ForTest ';' ForIncr ')' Block {
    $$ = stmt_from_for($3, $5, $7, $9);
  }
  | FOR '(' ID ':' Expression ')' Block {
    $$ = stmt_from_foreach(expr_from_name($3), $5, $7, 0);
  }
  | FOR '(' VAR ID ':' Expression ')' Block {
    $$ = stmt_from_foreach(expr_from_name($4), $6, $8, 1);
  }
  | FOR '(' VAR VariableList Type ':' Expression ')' Block {
    /*
    if (clist_length($4) != 1) {
      fprintf(stderr, "syntax error, foreach usage\n");
      exit(0);
    } else {
      struct list_head *node = clist_first($4);
      assert(node != NULL);
      clist_del(node, $4);
      free_clist($4);
      struct var *v = clist_entry(node, struct var);
      var_set_type(v, $5);
      $$ = stmt_from_foreach(v, $7, $9, 1);
    }
    */
    $$ = NULL;
  }
  ;

ForInit
  : Expression {
    $$ = stmt_from_expr($1);
  }
  | VariableDeclaration {
    $$ = $1;
  }
  | Assignment {
    $$ = $1;
  }
  | %empty {
    $$ = NULL;
  }
  ;

ForTest
  : Expression {
    $$ = stmt_from_expr($1);
  }
  | %empty {
    $$ = NULL;
  }
  ;

ForIncr
  : Expression {
    $$ = stmt_from_expr($1);
  }
  | Assignment {
    $$ = $1;
  }
  | %empty {
    $$ = NULL;
  }
  ;

/*--------------------------------------------------------------------------*/

JumpStatement
  : BREAK ';' {
    $$ = stmt_from_jump(BREAK_KIND);
  }
  | CONTINUE ';' {
    $$ = stmt_from_jump(CONTINUE_KIND);
  }
  ;

ReturnStatement
  : RETURN ';' {
    $$ = stmt_from_return(NULL);
  }
  | RETURN ExpressionList ';' {
    $$ = NULL; //stmt_from_return($2);
  }
  ;

/*-------------------------------------------------------------------------*/

PrimaryExpression
  : Atom {
    $$ = $1;
  }
  | PrimaryExpression '.' ID {
    $$ = expr_from_trailer(ATTRIBUTE_KIND, $3, $1);
  }
  | PrimaryExpression '[' Expression ']' {
    $$ = expr_from_trailer(SUBSCRIPT_KIND, $3, $1);
  }
  | PrimaryExpression '(' ExpressionList ')' {
    $$ = expr_from_trailer(CALL_KIND, $3, $1);
  }
  | PrimaryExpression '(' ')' {
    $$ = expr_from_trailer(CALL_KIND, NULL, $1);
  }
  ;

Atom
  : ID {
    $$ = expr_from_name($1);
  }
  | CONSTANT {
    $$ = $1;
  }
  | SELF {
    $$ = expr_from_self();
  }
  | PrimitiveType '(' CONSTANT ')' {
    $$ = $3;
  }
  | '(' Expression ')' {
    $$ = $2;
  }
  | ArrayDeclaration {
    $$ = $1;
  }
  | AnonymousFunctionDeclaration {
    $$ = $1;
  }
  ;

AnonymousFunctionDeclaration
  : FUNC '(' ParameterListOrEmpty ')' Block {
    $$ = expr_from_anonymous_func($3, NULL, $5);
  }
  | FUNC '(' ParameterListOrEmpty ')' ReturnTypeList Block {
    $$ = expr_from_anonymous_func($3, $5, $6);
  }
  ;

/* 常量(当做对象)允许访问成员变量和成员方法 */
CONSTANT
  : INT_CONST {
    $$ = expr_from_int($1);
  }
  | FLOAT_CONST {
    $$ = expr_from_float($1);
  }
  | STRING_CONST {
    $$ = expr_from_string($1);
  }
  | TOKEN_NULL {
    $$ = expr_from_null();
  }
  | TOKEN_TRUE {
    $$ = expr_from_bool(1);
  }
  | TOKEN_FALSE {
    $$ = expr_from_bool(0);
  }
  ;

ArrayDeclaration
  : DimExprList Type {
    //type_inc_dims($2, ($1)->count);
    /*$$ = expr_from_array0($3, $1, $2);*/
    printf("array declaration\n");
  }
  | DIMS BaseType '{' ArrayInitializerList '}' {
    //type_set_dims($2, $1);
    /*$$ = expr_from_array1($2, $1, $4);*/
    printf("array declaration 2\n");
  }
  ;

DimExprList
  : '[' Expression ']' {
    $$ = seq_new();
    seq_append($$, $2);
    printf("DimExprList\n");
  }
  | DimExprList '[' Expression ']' {
    seq_append($1, $3);
    $$ = $1;
    printf("DimExprList 2\n");
  }
  ;

ArrayInitializerList
  : ArrayInitializer {
    $$ = seq_new();
    seq_append($$, $1);
  }
  | ArrayInitializerList ',' ArrayInitializer {
    seq_append($1, $3);
    $$ = $1;
  }
  ;

ArrayInitializer
  : Expression {
    /*$$ = expr_from_array_tail_with_expr($1);*/
  }
  | '{' ArrayInitializerList '}' {
    /*$$ = expr_from_array_tail_with_list($2);*/
  }
  ;

/*-------------------------------------------------------------------------*/

UnaryExpression
  : PrimaryExpression {
    $$ = $1;
  }
  | UnaryOperator UnaryExpression {
    $$ = expr_from_unary($1, $2);
  }
  ;

UnaryOperator
  : '+' {
    $$ = OP_PLUS;
  }
  | '-' {
    $$ = OP_MINUS;
  }
  | '~' {
    $$ = OP_BIT_NOT;
  }
  | NOT {
    $$ = OP_LNOT;
  }
  ;

MultiplicativeExpression
  : UnaryExpression {
    $$ = $1;
  }
  | MultiplicativeExpression '*' UnaryExpression {
    $$ = expr_from_binary(OP_MULT, $1, $3);
  }
  | MultiplicativeExpression '/' UnaryExpression {
    $$ = expr_from_binary(OP_DIV, $1, $3);
  }
  | MultiplicativeExpression '%' UnaryExpression {
    $$ = expr_from_binary(OP_MOD, $1, $3);
  }
  ;

AdditiveExpression
  : MultiplicativeExpression {
    $$ = $1;
  }
  | AdditiveExpression '+' MultiplicativeExpression {
    $$ = expr_from_binary(OP_ADD, $1, $3);
  }
  | AdditiveExpression '-' MultiplicativeExpression {
    $$ = expr_from_binary(OP_SUB, $1, $3);
  }
  ;

ShiftExpression
  : AdditiveExpression {
    $$ = $1;
  }
  | ShiftExpression LSHIFT AdditiveExpression {
    $$ = expr_from_binary(OP_LSHIFT, $1, $3);
  }
  | ShiftExpression RSHIFT AdditiveExpression {
    $$ = expr_from_binary(OP_RSHIFT, $1, $3);
  }
  ;

RelationalExpression
  : ShiftExpression {
    $$ = $1;
  }
  | RelationalExpression '<' ShiftExpression {
    $$ = expr_from_binary(OP_LT, $1, $3);
  }
  | RelationalExpression '>' ShiftExpression {
    $$ = expr_from_binary(OP_GT, $1, $3);
  }
  | RelationalExpression LE  ShiftExpression {
    $$ = expr_from_binary(OP_LE, $1, $3);
  }
  | RelationalExpression GE  ShiftExpression {
    $$ = expr_from_binary(OP_GE, $1, $3);
  }
  ;

EqualityExpression
  : RelationalExpression {
    $$ = $1;
  }
  | EqualityExpression EQ RelationalExpression {
    $$ = expr_from_binary(OP_EQ, $1, $3);
  }
  | EqualityExpression NE RelationalExpression {
    $$ = expr_from_binary(OP_NEQ, $1, $3);
  }
  ;

AndExpression
  : EqualityExpression {
    $$ = $1;
  }
  | AndExpression '&' EqualityExpression {
    $$ = expr_from_binary(OP_BIT_AND, $1, $3);
  }
  ;

ExclusiveOrExpression
  : AndExpression {
    $$ = $1;
  }
  | ExclusiveOrExpression '^' AndExpression {
    $$ = expr_from_binary(OP_BIT_XOR, $1, $3);
  }
  ;

InclusiveOrExpression
  : ExclusiveOrExpression {
    $$ = $1;
  }
  | InclusiveOrExpression '|' ExclusiveOrExpression {
    $$ = expr_from_binary(OP_BIT_OR, $1, $3);
  }
  ;

LogicalAndExpression
  : InclusiveOrExpression {
    $$ = $1;
  }
  | LogicalAndExpression AND InclusiveOrExpression {
    $$ = expr_from_binary(OP_LAND, $1, $3);
  }
  ;

LogicalOrExpression
  : LogicalAndExpression {
    $$ = $1;
  }
  | LogicalOrExpression OR LogicalAndExpression {
    $$ = expr_from_binary(OP_LOR, $1, $3);
  }
  ;

Expression
  : LogicalOrExpression {
    $$ = $1;
  }
  ;

ExpressionList
  : Expression {
    $$ = seq_new();
    seq_append($$, $1);
  }
  | ExpressionList ',' Expression {
    seq_append($1, $3);
    $$ = $1;
  }
  ;

Assignment
  : PrimaryExpressionList '=' ExpressionList {
    $$ = stmt_from_assign($1, $3);
  }
  | PrimaryExpression CompoundAssignOperator Expression {
    $$ = stmt_from_compound_assign($1, $2, $3);
  }
  | PrimaryExpressionList TYPELESS_ASSIGN ExpressionList {

  }
  ;

PrimaryExpressionList
  : PrimaryExpression {
    $$ = seq_new();
    seq_append($$, $1);
  }
  | PrimaryExpressionList ',' PrimaryExpression {
    seq_append($1, $3);
    $$ = $1;
  }
  ;

/* 组合赋值运算符：算术运算和位运算 */
CompoundAssignOperator
  : PLUS_ASSGIN {
    $$ = OP_PLUS_ASSIGN;
  }
  | MINUS_ASSIGN {
    $$ = OP_MINUS_ASSIGN;
  }
  | MULT_ASSIGN {
    $$ = OP_MULT_ASSIGN;
  }
  | DIV_ASSIGN {
    $$ = OP_DIV_ASSIGN;
  }
  | MOD_ASSIGN {
    $$ = OP_MOD_ASSIGN;
  }
  | AND_ASSIGN {
    $$ = OP_AND_ASSIGN;
  }
  | OR_ASSIGN {
    $$ = OP_OR_ASSIGN;
  }
  | XOR_ASSIGN {
    $$ = OP_XOR_ASSIGN;
  }
  | RSHIFT_ASSIGN {
    $$ = OP_RSHIFT_ASSIGN;
  }
  | LSHIFT_ASSIGN {
    $$ = OP_LSHIFT_ASSIGN;
  }
  ;

/*--------------------------------------------------------------------------*/

%%
