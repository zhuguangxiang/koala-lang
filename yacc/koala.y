
%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "types.h"
#include "compile.h"

int yyerror(const char *str);
int yylex(void);

%}

%union {
  char *ident;
  int64_t ival;
  float64_t fval;
  char *string_const;
  int dims;
  int primitive;
  struct expr *expr;
  struct atom *atom;
  struct clist *list;
  struct stmt *stmt;
  struct type *type;
  struct array_tail *array_tail;
  int assign_op;
  struct member *member;
  struct if_expr *ifexpr;
  struct case_stmt *casestmt;
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
%token TYPEDEF
%token CONST
%token IMPORT
%token AS
%token GO
%token DEFER

%token BYTE
%token CHAR
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

%token <ival> INT_CONST
%token <ival> HEX_CONST
%token <ival> OCT_CONST
%token <fval> FLOAT_CONST
%token <string_const> STRING_CONST

%token <ident> ID

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
%type <list> TypeNameListOrEmpty
%type <list> TypeNameList
%type <list> ReturnTypeList
%type <list> TypeList
%type <list> ParameterList
%type <list> ParameterListOrEmpty

%type <assign_op> CompoundAssignOperator
%type <list> VariableList
%type <list> VariableInitializerList
%type <list> ExpressionList
%type <list> PrimaryExpressionList

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
%type <atom> Atom
%type <atom> ArrayAtom
%type <atom> AnonymousFunctionDeclaration
%type <atom> Constant
%type <atom> Trailer
%type <list> TrailerList
%type <list> DimExprList
%type <list> ArrayTailList
%type <array_tail> ArrayTail

%type <list> Imports
%type <list> ModuleStatements
%type <stmt> Import
%type <stmt> ModuleStatement
%type <stmt> Statement
%type <stmt> VariableConstantDeclaration
%type <stmt> ConstDeclaration
%type <stmt> VariableDeclaration
%type <stmt> Assignment
%type <stmt> IfStatement
%type <list> ElseIfStatements
%type <ifexpr> ElseIfStatement
%type <ifexpr> OptionELSE
%type <stmt> WhileStatement
%type <stmt> SwitchStatement
%type <list> CaseStatements
%type <casestmt> CaseStatement
%type <list> Block
%type <list> LocalStatements
%type <stmt> ReturnStatement
%type <stmt> JumpStatement
%type <stmt> FunctionDeclaration
%type <stmt> TypeDeclaration
%type <list> MemberDeclarations
%type <list> InterfaceFunctionDeclarations
%type <member> MemberDeclaration
%type <member> FieldDeclaration
%type <member> MethodDeclaration
%type <member> InterfaceFunctionDeclaration

%start CompileModule

%%

Type
  : BaseType {
    $$ = $1;
  }
  | DIMS BaseType {
    type_set_dims($2, $1);
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
    $$ = new_clist();
    clist_add_tail(&($1)->link, $$);
  }
  | TypeNameList ',' TypeName {
    clist_add_tail(&($3)->link, $1);
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
    $$ = new_clist();
    clist_add_tail(&($1)->link, $$);
  }
  | '(' TypeList ')' {
    $$ = $2;
  }
  ;

TypeList
  : Type {
    $$ = new_clist();
    clist_add_tail(&($1)->link, $$);
  }
  | TypeList ',' Type {
    clist_add_tail(&($3)->link, $1);
    $$ = $1;
  }
  ;

/*--------------------------------------------------------------------------*/
CompileModule
  : Imports ModuleStatements {;
    struct mod *mod = new_mod($1, $2);
    mod_traverse(mod);
    //compiler_module(mod);
  }
  | ModuleStatements {
    //struct mod *mod = new_mod(NULL, $1);
    //mod_traverse(mod);
    //compiler_module(mod);
  }
  ;

Imports
  : Import {
    $$ = new_clist();
    clist_add_tail(&($1)->link, $$);
  }
  | Imports Import {
    clist_add_tail(&($2)->link, $1);
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
    if ($1 != NULL) {
      $$ = new_clist();
      clist_add_tail(&($1)->link, $$);
    } else {
      printf("[ERROR] Statement is NULL\n");
    }
  }
  | ModuleStatements ModuleStatement {
    assert($2 != NULL);
    clist_add_tail(&($2)->link, $1);
    $$ = $1;
  }
  ;

/*--------------------------------------------------------------------------*/
ModuleStatement
  : Statement {
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
  | VariableConstantDeclaration {
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
  /*
  | ForStatement {

  }
  */
  | JumpStatement {
    $$ = $1;
  }
  | ReturnStatement {
    $$ = $1;
  }
  | Block {
    $$ = stmt_from_seq($1);
  }
  ;

VariableConstantDeclaration
  : ConstDeclaration ';' {
    $$ = $1;
  }
  | VariableDeclaration ';' {
    $$ = $1;
  }
  ;

/*--------------------------------------------------------------------------*/

ConstDeclaration
  : CONST VariableList '=' VariableInitializerList {
    $$ = stmt_from_vardecl($2, $4, 1, NULL);
  }
  | CONST VariableList Type '=' VariableInitializerList {
    $$ = stmt_from_vardecl($2, $5, 1, $3);
  }
  ;

VariableDeclaration
  : VAR VariableList Type {
    $$ = stmt_from_vardecl($2, NULL, 0, $3);
  }
  | VAR VariableList '=' VariableInitializerList {
    $$ = stmt_from_vardecl($2, $4, 0, NULL);
  }
  | VAR VariableList Type '=' VariableInitializerList {
    $$ = stmt_from_vardecl($2, $5, 0, $3);
  }
  ;

VariableList
  : ID {
    $$ = new_clist();
    clist_add_tail(&new_var($1)->link, $$);
  }
  | VariableList ',' ID {
    clist_add_tail(&new_var($3)->link, $1);
    $$ = $1;
  }
  ;

VariableInitializerList
  : ExpressionList {
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
    $$ = new_clist();
    clist_add_tail(&new_var_with_type($1, $2)->link, $$);
  }
  | ParameterList ',' ID Type {
    clist_add_tail(&new_var_with_type($3, $4)->link, $1);
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
  : STRUCT ID '{' MemberDeclarations '}' {
    $$ = stmt_from_structure($2, $4);
  }
  | INTERFACE ID '{' InterfaceFunctionDeclarations '}' {
    $$ = stmt_from_interface($2, $4);
  }
  | TYPEDEF ID Type ';' {
    $$ = stmt_from_typedef($2, $3);
  }
  ;

MemberDeclarations
  : MemberDeclaration {
    $$ = new_clist();
    clist_add_tail(&($1)->link, $$);
  }
  | MemberDeclarations MemberDeclaration {
    clist_add_tail(&($2)->link, $1);
    $$ = $1;
  }
  ;

MemberDeclaration
  : FieldDeclaration {
    $$ = $1;
  }
  | MethodDeclaration {
    $$ = $1;
  }
  ;

FieldDeclaration
  : VAR ID Type ';' {
    $$ = new_structure_vardecl($2, $3, NULL);
  }
  | ID Type ';' {
    $$ = new_structure_vardecl($1, $2, NULL);
  }
  | VAR ID Type '=' Expression ';' {
    $$ = new_structure_vardecl($2, $3, $5);
  }
  | ID Type '=' Expression ';' {
    $$ = new_structure_vardecl($1, $2, $4);
  }
  ;

MethodDeclaration
  : FUNC ID '(' ParameterListOrEmpty ')' Block {
    $$ = new_structure_funcdecl($2, $4, NULL, $6);
  }
  | FUNC ID '(' ParameterListOrEmpty ')' ReturnTypeList Block {
    $$ = new_structure_funcdecl($2, $4, $6, $7);
  }
  | ID '(' ParameterListOrEmpty ')' Block {
    $$ = new_structure_funcdecl($1, $3, NULL, $5);
  }
  | ID '(' ParameterListOrEmpty ')' ReturnTypeList Block {
    $$ = new_structure_funcdecl($1, $3, $5, $6);
  }
  ;

InterfaceFunctionDeclarations
  : InterfaceFunctionDeclaration ';' {
    $$ = new_clist();
    clist_add_tail(&($1)->link, $$);
  }
  | InterfaceFunctionDeclarations InterfaceFunctionDeclaration ';' {
    clist_add_tail(&($2)->link, $1);
    $$ = $1;
  }
  ;

InterfaceFunctionDeclaration
  : FUNC ID '(' TypeNameListOrEmpty ')' ReturnTypeList {
    $$ = new_interface_funcdecl($2, $4, $6);
  }
  | FUNC ID '(' TypeNameListOrEmpty ')' {
    $$ = new_interface_funcdecl($2, $4, NULL);
  }
  | ID '(' TypeNameListOrEmpty ')' ReturnTypeList {
    $$ = new_interface_funcdecl($1, $3, $5);
  }
  | ID '(' TypeNameListOrEmpty ')' {
    $$ = new_interface_funcdecl($1, $3, NULL);
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
    $$ = new_clist();
    clist_add_tail(&($1)->link, $$);
  }
  | LocalStatements Statement {
    clist_add_tail(&($2)->link, $1);
    $$ = $1;
  }
  ;

/*--------------------------------------------------------------------------*/

IfStatement
  : IF '(' Expression ')' Block OptionELSE {
    $$ = stmt_from_if(new_if_expr($3, $5), NULL, $6);
  }
  | IF '(' Expression ')' Block ElseIfStatements OptionELSE {
    $$ = stmt_from_if(new_if_expr($3, $5), $6, $7);
  }
  ;

ElseIfStatements
  : ElseIfStatement {
    $$ = new_clist();
    clist_add_tail(&($1)->link, $$);
  }
  | ElseIfStatements ElseIfStatement {
    clist_add_tail(&($2)->link, $1);
    $$ = $1;
  }
  ;

ElseIfStatement
  : ELSE IF '(' Expression ')' Block {
    $$ = new_if_expr($4, $6);
  }
  ;

OptionELSE
  : ELSE Block {
    $$ = new_if_expr(NULL, $2);
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
    $$ = new_clist();
    clist_add_tail(&($1)->link, $$);
  }
  | CaseStatements CaseStatement {
    if (($2)->expr == NULL) {
      /* default case */
      clist_add(&($2)->link, $1);
    } else {
      clist_add_tail(&($2)->link, $1);
    }
    $$ = $1;
  }
  ;

CaseStatement
  : CASE Expression ':' Block {
    $$ = new_case_stmt($2, $4);
  }
  | DEFAULT ':' Block {
    $$ = new_case_stmt(NULL, $3);
  }
  ;

/*--------------------------------------------------------------------------*/

/*
ForStatement
  : FOR '(' ForInit ForExpr ForIncr ')' Block
  //| FOR '(' ID IN ')'
  ;

ForInit
  : ExpressionStatement ';'
  | VariableDeclaration
  | ';'
  ;

ForExpr
  : Expression ';'
  | ';'
  ;

ForIncr
  : ExpressionStatement
  ;
*/

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
    $$ = expr_from_atom($1);
  }
  | Atom TrailerList {
    $$ = expr_from_atom_trailers($2, $1);
    free_clist($2);
  }
  ;

Atom
  : ID {
    $$ = atom_from_name($1);
  }
  | Constant {
    $$ = $1;
  }
  | SELF {
    $$ = atom_from_self();
  }
  | PrimitiveType '(' Constant ')' {
    $$ = $3;
  }
  | '(' Expression ')' {
    $$ = atom_from_expr($2);
  }
  | ArrayAtom {
    $$ = $1;
  }
  | AnonymousFunctionDeclaration {
    $$ = $1;
  }
  ;

AnonymousFunctionDeclaration
  : FUNC '(' ParameterListOrEmpty ')' Block {
    $$ = atom_from_anonymous_func($3, NULL, $5);
  }
  | FUNC '(' ParameterListOrEmpty ')' ReturnTypeList Block {
    $$ = atom_from_anonymous_func($3, $5, $6);
  }
  ;

/* 常量(当做对象)允许访问成员变量和成员方法 */
Constant
  : INT_CONST {
    $$ = atom_from_int($1);
  }
  | FLOAT_CONST {
    $$ = atom_from_float($1);
  }
  | STRING_CONST {
    $$ = atom_from_string($1);
  }
  | TOKEN_NULL {
    $$ = atom_from_null();
  }
  | TOKEN_TRUE {
    $$ = atom_from_bool(1);
  }
  | TOKEN_FALSE {
    $$ = atom_from_bool(0);
  }
  ;

ArrayAtom
  /*
    [20][30]int             √
    [20][]int               √
    {{1,2}, {3}, {4,5,6}}   ×
    {}                      ×
    [][]int{}               ×
    [][]int{{1,2},{3}}      √
    [3][][]int              √
    [3][][2]int             ×
    [20][30]int{}           ×
    [20][]int{}             ×
    --------------------------
    [20]int                 √
    {12,3}                  ×
    []int{1,2}              √
    [3]int{1,2,3}           ×
   */
  : DimExprList Type {
    type_inc_dims($2, ($1)->count);
    $$ = atom_from_array($2, 0, $1);
  }
  | DIMS BaseType '{' ArrayTailList '}' {
    type_set_dims($2, $1);
    $$ = atom_from_array($2, 1, $4);
  }
  ;

DimExprList
  : '[' Expression ']' {
    $$ = new_clist();
    clist_add_tail(&($2)->link, $$);
  }
  | DimExprList '[' Expression ']' {
    clist_add_tail(&($3)->link, $1);
    $$ = $1;
  }
  ;

ArrayTailList
  : ArrayTail {
    $$ = new_clist();
    clist_add_tail(&($1)->link, $$);
  }
  | ArrayTailList ',' ArrayTail {
    clist_add_tail(&($3)->link, $1);
    $$ = $1;
  }
  ;

ArrayTail
  : Expression {
    $$ = array_tail_from_expr($1);
  }
  | '{' ArrayTailList '}' {
    $$ = array_tail_from_list($2);
  }
  ;

/*-------------------------------------------------------------------------*/

TrailerList
  : Trailer {
    $$ = new_clist();
    clist_add_tail(&($1)->link, $$);
  }
  | TrailerList Trailer {
    clist_add_tail(&($2)->link, $1);
    $$ = $1;
  }
  ;

Trailer
  : '.' ID {
    $$ = trailer_from_attribute($2);
  }
  | '[' Expression ']' {
    $$ = trailer_from_subscript($2);
  }
  | '(' ExpressionList ')' {
    $$ = trailer_from_call($2);
  }
  | '(' ')' {
    $$ = trailer_from_call(NULL);
  }
  | '(' ')' '{' FunctionDeclarationList '}' {
    // 匿名接口实现
    $$ = NULL;
  }
  ;

FunctionDeclarationList
  : FunctionDeclaration {
    /*$$ = new_linked_list();
    linked_list_add_tail($$, $1);*/
  }
  | FunctionDeclarationList FunctionDeclaration {
    /*linked_list_add_tail($1, $2);
    $$ = $1;*/
  }
  ;

/*-------------------------------------------------------------------------*/

UnaryExpression
  : PrimaryExpression {
    $$ = $1;
  }
  | '+' UnaryExpression {
    $$ = expr_for_unary(OP_PLUS, $2);;
  }
  | '-' UnaryExpression {
    $$ = expr_for_unary(OP_MINUS, $2);
  }
  | '~' UnaryExpression {
    /* bit operation : bit reversal */
    $$ = expr_for_unary(OP_BIT_NOT, $2);
  }
  | '!' UnaryExpression {
    /* logic operation : not */
    $$ = expr_for_unary(OP_LNOT, $2);
  }
  ;

MultiplicativeExpression
  : UnaryExpression {
    $$ = $1;
  }
  | MultiplicativeExpression '*' UnaryExpression {
    $$ = expr_for_binary(OP_MULT, $1, $3);
  }
  | MultiplicativeExpression '/' UnaryExpression {
    $$ = expr_for_binary(OP_DIV, $1, $3);
  }
  | MultiplicativeExpression '%' UnaryExpression {
    $$ = expr_for_binary(OP_MOD, $1, $3);
  }
  ;

AdditiveExpression
  : MultiplicativeExpression {
    $$ = $1;
  }
  | AdditiveExpression '+' MultiplicativeExpression {
    $$ = expr_for_binary(OP_ADD, $1, $3);
  }
  | AdditiveExpression '-' MultiplicativeExpression {
    $$ = expr_for_binary(OP_SUB, $1, $3);
  }
  ;

ShiftExpression
  : AdditiveExpression {
    $$ = $1;
  }
  | ShiftExpression LSHIFT AdditiveExpression {
    $$ = expr_for_binary(OP_LSHIFT, $1, $3);
  }
  | ShiftExpression RSHIFT AdditiveExpression {
    $$ = expr_for_binary(OP_RSHIFT, $1, $3);
  }
  ;

RelationalExpression
  : ShiftExpression {
    $$ = $1;
  }
  | RelationalExpression '<' ShiftExpression {
    $$ = expr_for_binary(OP_LT, $1, $3);
  }
  | RelationalExpression '>' ShiftExpression {
    $$ = expr_for_binary(OP_GT, $1, $3);
  }
  | RelationalExpression LE  ShiftExpression {
    $$ = expr_for_binary(OP_LE, $1, $3);
  }
  | RelationalExpression GE  ShiftExpression {
    $$ = expr_for_binary(OP_GE, $1, $3);
  }
  ;

EqualityExpression
  : RelationalExpression {
    $$ = $1;
  }
  | EqualityExpression EQ RelationalExpression {
    $$ = expr_for_binary(OP_EQ, $1, $3);
  }
  | EqualityExpression NE RelationalExpression {
    $$ = expr_for_binary(OP_NEQ, $1, $3);
  }
  ;

AndExpression
  : EqualityExpression {
    $$ = $1;
  }
  | AndExpression '&' EqualityExpression {
    $$ = expr_for_binary(OP_BIT_AND, $1, $3);
  }
  ;

ExclusiveOrExpression
  : AndExpression {
    $$ = $1;
  }
  | ExclusiveOrExpression '^' AndExpression {
    $$ = expr_for_binary(OP_BIT_XOR, $1, $3);
  }
  ;

InclusiveOrExpression
  : ExclusiveOrExpression {
    $$ = $1;
  }
  | InclusiveOrExpression '|' ExclusiveOrExpression {
    $$ = expr_for_binary(OP_BIT_OR, $1, $3);
  }
  ;

LogicalAndExpression
  : InclusiveOrExpression {
    $$ = $1;
  }
  | LogicalAndExpression AND InclusiveOrExpression {
    $$ = expr_for_binary(OP_LAND, $1, $3);
  }
  ;

LogicalOrExpression
  : LogicalAndExpression {
    $$ = $1;
  }
  | LogicalOrExpression OR LogicalAndExpression {
    $$ = expr_for_binary(OP_LOR, $1, $3);
  }
  ;

Expression
  : LogicalOrExpression {
    $$ = $1;
  }
  ;

ExpressionList
  : Expression {
    $$ = new_clist();
    clist_add_tail(&($1)->link, $$);
  }
  | ExpressionList ',' Expression {
    clist_add_tail(&($3)->link, $1);
    $$ = $1;
  }
  ;

Assignment
  : PrimaryExpressionList '=' ExpressionList {
    $$ = stmt_from_assign($1, $3);
  }
  /*
  | PrimaryExpressionList TYPELESS_ASSIGN ExpressionList {
  }
  */
  | PrimaryExpression CompoundAssignOperator Expression {
    $$ = stmt_from_compound_assign($1, $2, $3);
  }
  ;

PrimaryExpressionList
  : PrimaryExpression {
    $$ = new_clist();
    clist_add_tail(&($1)->link, $$);
  }
  | PrimaryExpressionList ',' PrimaryExpression {
    clist_add_tail(&($3)->link, $1);
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
