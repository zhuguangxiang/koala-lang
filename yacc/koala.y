
%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "koala_yacc.h"

int yylex(void);

int yyerror(ParserState *parser, const char *str)
{
  fprintf(stderr, "syntax error: %s\n", str);
  return 0;
}

static decl_mod(mod);

//#define YYERROR_VERBOSE 1
#define YY_USER_ACTION yylloc.first_line = yylloc.last_line = yylineno;

%}

%union {
  char *id;
  int64 ival;
  float64 fval;
  char *string_const;
  int dims;
  int primitive;
  struct expr *expr;
  Vector *vector;
  struct stmt *stmt;
  TypeDesc *type;
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

%token PACKAGE
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
%token CLASS
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
%token TOKEN_NIL
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
%type <type> UserDefType
%type <type> BaseType
%type <type> Type
%type <type> FunctionType
%type <type> TypeName
%type <vector> TypeNameListOrEmpty
%type <vector> TypeNameList
%type <vector> ReturnTypeList
%type <vector> TypeList
%type <vector> ParameterList
%type <vector> ParameterListOrEmpty
%type <operator> UnaryOperator
%type <operator> CompoundAssignOperator
%type <vector> VariableList
%type <vector> ExpressionList
%type <vector> PrimaryExpressionList

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
%type <vector> DimExprList
%type <vector> ArrayInitializerList
%type <expr> ArrayInitializer
%type <stmt> VariableDeclaration
%type <stmt> ConstDeclaration
%type <stmt> ModuleStatement
%type <stmt> LocalStatement
%type <stmt> Assignment
%type <stmt> IfStatement
%type <vector> ElseIfStatements
%type <testblock> ElseIfStatement
%type <testblock> OptionELSE
%type <stmt> WhileStatement
%type <stmt> SwitchStatement
%type <vector> CaseStatements
%type <testblock> CaseStatement
%type <stmt> ForStatement
%type <stmt> ForInit
%type <stmt> ForTest
%type <stmt> ForIncr
%type <vector> Block
%type <stmt> GoStatement
%type <vector> LocalStatements
%type <stmt> ReturnStatement
%type <stmt> JumpStatement
%type <stmt> FunctionDeclaration
%type <stmt> TypeDeclaration
%type <vector> MemberDeclarations
%type <field> FieldDeclaration
%type <vector> IntfFuncDecls
%type <intf_func> IntfFuncDecl

%start CompileUnit

%parse-param {ParserState *parser}

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
    $$ = TypeDesc_From_Primitive($1);
  }
  | UserDefType {
    $$ = $1;
  }

  | FunctionType {
    $$ = $1;
  }
  ;

PrimitiveType
  : INTEGER {
    $$ = PRIMITIVE_INT;
  }
  | FLOAT {
    $$ = PRIMITIVE_FLOAT;
  }
  | BOOL {
    $$ = PRIMITIVE_BOOL;
  }
  | STRING {
    $$ = PRIMITIVE_STRING;
  }
  | ANY {
    $$ = PRIMITIVE_ANY;
  }
  ;

UserDefType
  : ID {
    // type in local module
    $$ = TypeDesc_From_UserDef(NULL, $1);
  }
  | ID '.' ID {
    // type in external module
    if (!strcmp($1, "lang") && !strcmp($3, "String")) {
      $$ = TypeDesc_From_Primitive(PRIMITIVE_STRING);
    } else {
      char *path = userdef_get_path(parser, $1);
      $$ = TypeDesc_From_UserDef(path, $3);
    }
  }
  ;

FunctionType
  : FUNC '(' TypeNameListOrEmpty ')' ReturnTypeList {
    $$ = TypeDesc_From_Proto($3, $5);
  }
  | FUNC '(' TypeNameListOrEmpty ')' {
    $$ = TypeDesc_From_Proto($3, NULL);
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
    $$ = Vector_New();
    Vector_Append($$, $1);
  }
  | TypeNameList ',' TypeName {
    Vector_Append($1, $3);
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
    $$ = Vector_New();
    Vector_Append($$, $1);
  }
  | '(' TypeList ')' {
    $$ = $2;
  }
  | error {
    yyerror(parser, "return declaration is not correct");
    $$ = NULL;
  }
  ;

TypeList
  : Type {
    $$ = Vector_New();
    Vector_Append($$, $1);
  }
  | TypeList ',' Type {
    Vector_Append($1, $3);
    $$ = $1;
  }
  ;

/*--------------------------------------------------------------------------*/

CompileUnit
  : Imports ModuleStatements {
    mod.package = NULL;
    ast_traverse(&mod.stmts);
  }
  | ModuleStatements {
    mod.package = NULL;;
    ast_traverse(&mod.stmts);
  }
  | Package Imports ModuleStatements {
    ast_traverse(&mod.stmts);
    parse_module(parser, &mod);
  }
  | Package ModuleStatements {
    ast_traverse(&mod.stmts);
    parse_module(parser, &mod);
  }
  ;

Package
  : PACKAGE ID ';' {
    mod.package = $2;
  }
  ;

Imports
  : Import
  | Imports Import
  ;

Import
  : IMPORT STRING_CONST ';' {
    parse_import(parser, NULL, $2);
  }
  | IMPORT ID STRING_CONST ';' {
    parse_import(parser, $2, $3);
  }
  ;

ModuleStatements
  : ModuleStatement {
    if ($1 != NULL) Vector_Append(&mod.stmts, $1);
  }
  | ModuleStatements ModuleStatement {
    if ($2 != NULL) Vector_Append(&mod.stmts, $2);
  }
  ;

ModuleStatement
  : ';' {
    printf("empty statement\n");
    $$ = NULL;
  }
  | VariableDeclaration ';' {
    parse_vardecl(parser, $1);
    $$ = $1;
  }
  | ConstDeclaration {
    parse_vardecl(parser, $1);
    $$ = $1;
  }
  | FunctionDeclaration {
    parse_funcdecl(parser, $1);
    $$ = $1;
  }
  | TypeDeclaration {
    parse_typedecl(parser, $1);
    $$ = $1;
  }
  | error {
    yyerror(parser, "non-declaration statement outside function body");
    yyerrok;
    yyclearin;
  }
  ;

/*--------------------------------------------------------------------------*/

ConstDeclaration
  : CONST VariableList '=' ExpressionList ';' {
    $$ = stmt_from_vardecl($2, $4, 1, NULL);
  }
  | CONST VariableList Type '=' ExpressionList ';' {
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
    $$ = Vector_New();
    Vector_Append($$, new_var($1, NULL));
  }
  | VariableList ',' ID {
    Vector_Append($1, new_var($3, NULL));
    $$ = $1;
  }
  ;

/*--------------------------------------------------------------------------*/

FunctionDeclaration
  : FUNC ID '(' ParameterListOrEmpty ')' Block {
    $$ = stmt_from_funcdecl($2, $4, NULL, $6);
  }
  | FUNC ID '(' ParameterListOrEmpty ')' ReturnTypeList Block {
    $$ = stmt_from_funcdecl($2, $4, $6, $7);
  }
  ;

ParameterList
  : ID Type {
    $$ = Vector_New();
    Vector_Append($$, new_var($1, $2));
  }
  | ParameterList ',' ID Type {
    Vector_Append($$, new_var($3, $4));
    $$ = $1;
  }
  | error {
    yyerror(parser, "parameter declaration is not correct");
    $$ = NULL;
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
  : CLASS ID '{' MemberDeclarations '}' {
    //$$ = stmt_from_structure($2, $4);
  }
  | CLASS ID ':' UserDefType '{' '}' {

  }
  | INTERFACE ID '{' IntfFuncDecls '}' {
    $$ = stmt_from_interface($2, $4);
  }
  ;

MemberDeclarations
  : MemberDeclaration {
    //$$ = Vector_New();
    //Vector_Append($$, $1);
  }
  | MemberDeclarations MemberDeclaration {
    //Vector_Append($1, $2);
    //$$ = $1;
  }
  ;

MemberDeclaration
  : FieldDeclaration
  | FunctionDeclaration
  ;

FieldDeclaration
  : ID Type ';' {
    $$ = new_struct_field($1, $2, NULL);
  }
  | ID Type '=' Expression ';' {

  }
  ;

IntfFuncDecls
  : IntfFuncDecl {
    $$ = Vector_New();
    Vector_Append($$, $1);
  }
  | IntfFuncDecls IntfFuncDecl {
    Vector_Append($1, $2);
    $$ = $1;
  }
  ;

IntfFuncDecl
  : FUNC ID '(' TypeNameListOrEmpty ')' ReturnTypeList ';'{
    $$ = new_intf_func($2, $4, $6);
  }
  | FUNC ID '(' TypeNameListOrEmpty ')' ';' {
    $$ = new_intf_func($2, $4, NULL);
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
  : LocalStatement {
    $$ = Vector_New();
    if ($1 != NULL) Vector_Append($$, $1);
  }
  | LocalStatements LocalStatement {
    if ($2 != NULL) Vector_Append($1, $2);
    $$ = $1;
  }
  ;

LocalStatement
  : ';' {
    $$ = NULL;
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

  }
  | WhileStatement {

  }
  | SwitchStatement {

  }
  | ForStatement {

  }
  | JumpStatement {

  }
  | ReturnStatement {
    $$ = $1;
  }
  | Block {
    stmt_from_block($1);
  }
  | GoStatement {
  }
  ;

/*--------------------------------------------------------------------------*/

GoStatement
  : GO PrimaryExpression '(' ExpressionList ')' ';' {
    $$ = stmt_from_go(expr_from_trailer(CALL_KIND, $4, $2));
  }
  | GO PrimaryExpression '(' ')' ';' {
    $$ = stmt_from_go(expr_from_trailer(CALL_KIND, NULL, $2));
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
    $$ = Vector_New();
    Vector_Append($$, $1);
  }
  | ElseIfStatements ElseIfStatement {
    Vector_Append($1, $2);
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
    $$ = Vector_New();
    Vector_Append($$, $1);
  }
  | CaseStatements CaseStatement {
    if ($2->test == NULL) {
      /* default case */
      struct test_block *tb = Vector_Get($1, 0);
      if (tb != NULL && tb->test == NULL) {
        fprintf(stderr, "[ERROR] default case needs only one\n");
        exit(0);
      } else {
        Vector_Set($1, 0, $2);
      }
    } else {
      Vector_Append($1, $2);
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
    $$ = stmt_from_foreach(new_var($3, NULL), $5, $7, 0);
  }
  | FOR '(' VAR ID ':' Expression ')' Block {
    $$ = stmt_from_foreach(new_var($4, NULL), $6, $8, 1);
  }
  | FOR '(' VAR VariableList Type ':' Expression ')' Block {
    if (Vector_Size($4) != 1) {
      fprintf(stderr, "[ERROR]syntax error, foreach usage\n");
      exit(0);
    } else {
      struct var *v = Vector_Get($4, 0);
      ASSERT_PTR(v);
      v->type = $5;
      Vector_Free($4, NULL, NULL);
      $$ = stmt_from_foreach(v, $7, $9, 1);
    }
  }
  ;

ForInit
  : Expression {
    $$ = stmt_from_expr($1);
  }
  | VariableDeclaration {
    //$$ = $1;
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
    $$ = stmt_from_return($2);
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
  | TOKEN_NIL {
    $$ = expr_from_nil();
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
    $2->dims += Vector_Size($1);
    $$ = expr_from_array($2, $1, NULL);
  }
  | DIMS BaseType '{' ArrayInitializerList '}' {
    $2->dims = $1;
    $$ = expr_from_array($2, NULL, $4);
  }
  ;

DimExprList
  : '[' Expression ']' {
    $$ = Vector_New();
    Vector_Append($$, $2);
  }
  | DimExprList '[' Expression ']' {
    Vector_Append($1, $3);
    $$ = $1;
  }
  ;

ArrayInitializerList
  : ArrayInitializer {
    $$ = Vector_New();
    Vector_Append($$, $1);
  }
  | ArrayInitializerList ',' ArrayInitializer {
    Vector_Append($1, $3);
    $$ = $1;
  }
  ;

ArrayInitializer
  : Expression {
    $$ = $1;
  }
  | '{' ArrayInitializerList '}' {
    $$ = expr_from_array_with_tseq($2);
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
    $$ = Vector_New();
    Vector_Append($$, $1);
  }
  | ExpressionList ',' Expression {
    Vector_Append($1, $3);
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
    $$ = Vector_New();
    Vector_Append($$, $1);
  }
  | PrimaryExpressionList ',' PrimaryExpression {
    Vector_Append($1, $3);
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
