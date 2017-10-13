
%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
  /*int dims;*/
  int type;
  struct expr *expr;
  struct atom *atom;
  struct list_head *list;
  struct mod *module;
  struct stmt *stmt;
  /*base_type_t base_type;*/
  /*struct {
    string mod_name;
    string type_name;
  } module_type;*/
  /*
  struct {
    linked_list_t *parameter_type_list;
    linked_list_t *return_type_list;
  } func_type;
  name_type_t *name_type;
  type_t type;
  linked_list_t *linked_list;
  expr_t *expr;
  term_t term;
  trailer_t *trailer;
  anonymous_function_t *anonymous;
  array_object_t *array_object;
  compound_op_t compound_op;
  linked_list_t *member_declarations[2];
  struct {
    int type;
    union {
      variable_t *var;
      function_t *func;
    };
  } member_declaration;
  variable_t *variable;
  function_t *function;
  struct {
    string name;
    linked_list_t *parameter_list;
    linked_list_t *parameter_type_list;
    linked_list_t *return_type_list;
  } method_header;
  intf_func_proto_t *intf_func_proto;
  */
}

%token ELLIPSIS

/* 算术运算和位运算 */
%token TYPELESS_ASSIGN
%token PLUS_ASSGIN
%token MINUS_ASSIGN
%token TIMES_ASSIGN
%token DIVIDE_ASSIGN
%token MOD_ASSIGN
%token AND_ASSIGN
%token OR_ASSIGN
%token XOR_ASSIGN
%token RIGHT_SHIFT_ASSIGN
%token LEFT_SHIFT_ASSIGN

%token EQ
%token NE
%token GE
%token LE
%token AND
%token OR
%token NOT
%token SHIFT_LEFT
%token SHIFT_RIGHT
%token INC
%token DEC

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
%token TYPE
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
%token ROOT_OBJECT
/*%token <dims> DIMS*/

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

/*%nonassoc ELSE

%precedence ID
%precedence '.'

%precedence ')'
%precedence '('

%precedence FUNC*/

/*%expect 2*/

/*--------------------------------------------------------------------------*/

/*%type <linked_list> TypeNameList*/

%type <type> PrimitiveType
/*%type <module_type> ModuleType*/
/*%type <func_type> FunctionType
%type <base_type> BaseType
%type <name_type> TypeName*/
/*
%type <member_declarations> MemberDeclarations
%type <member_declaration> MemberDeclaration
%type <variable> FieldDeclaration
%type <function> MethodDeclaration
%type <method_header> MethodDeclarationHeader1
%type <method_header> MethodDeclarationHeader2
%type <intf_func_proto> InterfaceFunctionDeclaration

%type <expr> VariableInitializer
%type <linked_list> VariableInitializerList
%type <linked_list> ArrayInitializerList


%type <linked_list> VariableList
%type <linked_list> ParameterList
%type <linked_list> ReturnTypeList
%type <linked_list> LocalVariableDeclsOrStatements
%type <linked_list> FunctionDeclarationList
%type <linked_list> InterfaceFunctionDeclarations

%type <expr> CodeBlock
%type <expr> LocalVariableDeclOrStatement
%type <expr> Statement
%type <expr> ExpressionStatement
%type <expr> ReturnStatement*/

/*%type <compound_op> CompoundOperator*/
%type <list> ExpressionList
/*%type <linked_list> PostfixExpressionList*/
/*%type <expr> AssignmentExpression*/
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
%type <expr> PostfixExpression
%type <expr> PrimaryExpression
%type <atom> Atom
%type <atom> Constant
%type <atom> Trailer
%type <list> TrailerList

/*%type <expr> DimExpr
%type <linked_list> DimExprs*/

/*%type <anonymous> AnonymousFunctionDeclaration*/

%type <module> CompileModule
%type <list> Imports
%type <list> Statements
%type <stmt> Import
%type <stmt> Statement

%start CompileModule

%%

/*
TypeNameList
  : TypeName {
    $$ = new_linked_list_data($1);
  }
  | TypeNameList ',' TypeName {
    linked_list_add_tail($1, $3);
    $$ = $1;
  }
  ;

TypeName
  : BaseType {
    $$ = new_name_type(0, $1);
  }
  | DIMS BaseType {
    $$ = new_name_type($1, $2);
  }
  ;


BaseType
  : PrimitiveType {
    $$ = primitive_type($1);
  }
  | ModuleType {
    $$ = module_type($1.mod_name, $1.type_name);
  }
  | FunctionType {
    $$ = func_type($1.parameter_type_list, $1.return_type_list);
  }
  ;
*/

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
  ;

/*ModuleType
  : ID {
    $$.mod_name  = null_string;
    $$.type_name = $1;
  }
  | ID '.' ID {
    $$.mod_name  = $1;
    $$.type_name = $3;
  }
  ;*/

/*
FunctionType
  : FUNC '(' ')' ReturnTypeList {
    $$.parameter_type_list = null;
    $$.return_type_list = $4;
  }
  | FUNC '(' ')' {
    $$.parameter_type_list = null;
    $$.return_type_list = null;
    $$ = func_type(null, null);
  }
  | FUNC '(' TypeNameList ')' ReturnTypeList {
    $$.parameter_type_list = $3;
    $$.return_type_list = $5;
  }
  | FUNC '(' TypeNameList ')' {
    $$.parameter_type_list = $3;
    $$.return_type_list = null;
  }
  ;

ReturnTypeList
  : TypeName {
    $$ = new_linked_list_data($1);
  }
  | '(' TypeNameList ')' {
    $$ = $2;
  }
  ;
*/
/*--------------------------------------------------------------------------*/
CompileModule
  : Imports Statements {;
    $$ = new_mod($1, $2);
    mod_traverse($$);
    compiler_module($$);
  }
  | Statements {
    $$ = new_mod(NULL, $1);
    mod_traverse($$);
    compiler_module($$);
  }
  ;

Imports
  : Import {
    $$ = new_list();
    list_add_tail(&($1)->link, $$);
  }
  | Imports Import {
    list_add_tail(&($2)->link, $1);
    $$ = $1;
  }
  ;

Import
  : IMPORT STRING_CONST {
    $$ = NULL;
  }
  | IMPORT ID STRING_CONST {
    $$ = stmt_from_import($2, $3);
  }
  ;

Statements
  : Statement {
    $$ = new_list();
    list_add_tail(&($1)->link, $$);
  }
  | Statements Statement {
    list_add_tail(&($2)->link, $1);
    $$ = $1;
  }
  ;

/*Declaration
  : Statement
  | ConstDeclaration
  | VariableDeclaration
  | TypeDeclaration
  | FunctionDeclaration
  ;*/

/*--------------------------------------------------------------------------.
|  |
|                                                     |
`--------------------------------------------------------------------------*/
/*
ConstDeclaration
  : CONST VariableList '=' VariableInitializerList ';' {
    do_const_decl($2, null, $4);
  }
  | CONST VariableList TypeName '=' VariableInitializerList ';' {
    do_const_decl($2, $3, $5);
  }
  ;

VariableDeclaration
  : VAR VariableList TypeName ';' {
    do_var_decl($2, $3, null);
  }
  | VAR VariableList '=' VariableInitializerList ';' {
    do_var_decl($2, null, $4);
  }
  | VAR VariableList TypeName '=' VariableInitializerList ';' {
    do_var_decl($2, $3, $5);
  }
  ;

VariableList
  : ID {
    $$ = new_linked_list_data(to_str_ptr($1));
  }
  | VariableList ',' ID {
    linked_list_add_tail($1, to_str_ptr($3));
    $$ = $1;
  }
  ;

VariableInitializerList
  : ArrayInitializerList {
    $$ = $1;
  }
  ;

ArrayInitializerList
  : VariableInitializer {
    $$ = new_linked_list_data($1);
  }
  | ArrayInitializerList ',' VariableInitializer {
    linked_list_add_tail($1, $3);
    $$ = $1;
  }
  ;

VariableInitializer
  : Expression {
    $$ = $1;
  }
  | '{' ArrayInitializerList '}' {
    $$ = new_exp_seq($2);
  }
  ;
*/
/*--------------------------------------------------------------------------*/
/*
SemiOrEmpty
  : %empty
  | ';'
  ;

TypeDeclaration
  : TYPE ID STRUCT '{' MemberDeclarations '}' SemiOrEmpty {
    //$$ = new_exp_type_struct($2, $5[0], $5[1]);
    //free_linked_list($5[0]);
    //free_linked_list($5[1]);
  }
  | TYPE ID INTERFACE '{' InterfaceFunctionDeclarations '}' SemiOrEmpty {
    //$$ = new_exp_type_interface($2, $5);
    //free_linked_list($5);
  }
  | TYPE ID TypeName SemiOrEmpty {
    //$$ = new_exp_type_redef($2, $3);
  }
  ;

MemberDeclarations
  : MemberDeclaration {
    $$[0] = new_linked_list();
    $$[1] = new_linked_list();
    if ($1.type == 1) {
      linked_list_add_tail($$[0], $1.var);
    } else {
      linked_list_add_tail($$[1], $1.func);
    }
  }
  | MemberDeclarations MemberDeclaration {
    if ($2.type == 1) {
      linked_list_add_tail($1[0], $2.var);
    } else {
      linked_list_add_tail($1[1], $2.func);
    }
    $$[0] = $1[0];
    $$[1] = $1[1];
  }
  ;

MemberDeclaration
  : FieldDeclaration {
    $$.type = 1;
    $$.var = $1;
  }
  | MethodDeclaration {
    $$.type = 2;
    $$.func = $1;
  }
  ;

FieldDeclaration
  : VAR ID TypeName ';' {
    $$ = new_variable($2, $3);
  }
  | ID TypeName ';' {
    $$ = new_variable($1, $2);
  }
  ;

MethodDeclaration
  : MethodDeclarationHeader1 CodeBlock {
    $$ = new_method($1.name, $1.parameter_list, $1.return_type_list, $2);
    if ($1.parameter_list != null)
      free_linked_list($1.parameter_list);
    if ($1.return_type_list != null)
      free_linked_list($1.return_type_list);
  }
  ;

MethodDeclarationHeader1
  : FUNC ID '(' ')' ReturnTypeList {
    $$.name = $2;
    $$.parameter_list = null;
    $$.return_type_list = $5;
  }
  | FUNC ID '(' ')' {
    $$.name = $2;
    $$.parameter_list = null;
    $$.return_type_list = null;
  }
  | FUNC ID '(' ParameterList ')' ReturnTypeList {
    $$.name = $2;
    $$.parameter_list = $4;
    $$.return_type_list = $6;
  }
  | FUNC ID '(' ParameterList ')' {
    $$.name = $2;
    $$.parameter_list = $4;
    $$.return_type_list = null;
  }
  | ID '(' ')' ReturnTypeList {
    $$.name = $1;
    $$.parameter_list = null;
    $$.return_type_list = $4;
  }
  | ID '(' ')' {
    $$.name = $1;
    $$.parameter_list = null;
    $$.return_type_list = null;
  }
  | ID '(' ParameterList ')' ReturnTypeList {
    $$.name = $1;
    $$.parameter_list = $3;
    $$.return_type_list = $5;
  }
  | ID '(' ParameterList ')' {
    $$.name = $1;
    $$.parameter_list = $3;
    $$.return_type_list = null;
  }
  ;

MethodDeclarationHeader2
  : FUNC ID '(' TypeNameList ')' ReturnTypeList {
    $$.name = $2;
    $$.parameter_type_list = $4;
    $$.return_type_list = $6;
  }
  | FUNC ID '(' TypeNameList ')' {
    $$.name = $2;
    $$.parameter_type_list = $4;
    $$.return_type_list = null;
  }
  | ID '(' TypeNameList ')' ReturnTypeList {
    $$.name = $1;
    $$.parameter_type_list = $3;
    $$.return_type_list = $5;
  }
  | ID '(' TypeNameList ')' {
    $$.name = $1;
    $$.parameter_type_list = $3;
    $$.return_type_list = null;
  }
  ;

ParameterList
  : ID TypeName {
    $$ = new_linked_list();
    linked_list_add_tail($$, new_variable($1, $2));
  }
  | ParameterList ',' ID TypeName {
    linked_list_add_tail($1, new_variable($3, $4));
    $$ = $1;
  }
  ;

InterfaceFunctionDeclarations
  : InterfaceFunctionDeclaration {
    $$ = new_linked_list();
    linked_list_add_tail($$, $1);
  }
  | InterfaceFunctionDeclarations InterfaceFunctionDeclaration {
    linked_list_add_tail($1, $2);
    $$ = $1;
  }
  ;

InterfaceFunctionDeclaration
  : MethodDeclarationHeader1 ';' {
    $$ = new_intf_func_proto($1.name,
                             $1.parameter_list,
                             null,
                             $1.return_type_list);
    free_linked_list($1.parameter_list);
    free_linked_list($1.return_type_list);
  }
  | MethodDeclarationHeader2 ';' {
    $$ = new_intf_func_proto($1.name,
                             null,
                             $1.parameter_type_list,
                             $1.return_type_list);
    free_linked_list($1.parameter_type_list);
    free_linked_list($1.return_type_list);
  }
  ;
*/
/*--------------------------------------------------------------------------*/
/*
FunctionDeclaration
  : FUNC ID '(' ')' ReturnTypeList CodeBlock {
    $$ = new_exp_function($2, null, $5, $6);
    free_linked_list($5);
  }
  | FUNC ID '(' ')' CodeBlock {
    $$ = new_exp_function($2, null, null, $5);
  }
  | FUNC ID '(' ParameterList ')' ReturnTypeList CodeBlock {
    $$ = new_exp_function($2, $4, $6, $7);
    free_linked_list($4);
    free_linked_list($6);
  }
  | FUNC ID '(' ParameterList ')' CodeBlock {
    $$ = new_exp_function($2, $4, null, $6);
    free_linked_list($4);
  }
  ;

AnonymousFunctionDeclaration
  : FUNC '(' ')' ReturnTypeList CodeBlock {
    $$ = new_anonymous_func(null, $4, $5);
    free_linked_list($4);
  }
  | FUNC '(' ')' CodeBlock {
    $$ = new_anonymous_func(null, null, $4);
  }
  | FUNC '(' ParameterList ')' ReturnTypeList CodeBlock {
    $$ = new_anonymous_func($3, $5, $6);
    free_linked_list($3);
    free_linked_list($5);
  }
  | FUNC '(' ParameterList ')' CodeBlock {
    $$ = new_anonymous_func($3, null, $5);
    free_linked_list($3);
  }
  ;
*/
/*--------------------------------------------------------------------------*/
/*
CodeBlock
  : '{' LocalVariableDeclsOrStatements '}' {
    $$ = new_exp_seq($2);
    free_linked_list($2);
  }
  | '{' '}' {
    $$ = new_exp_seq(null);
  }
  ;

LocalVariableDeclsOrStatements
  : LocalVariableDeclOrStatement {
    $$ = new_linked_list();
    linked_list_add_tail($$, $1);
  }
  | LocalVariableDeclsOrStatements LocalVariableDeclOrStatement {
    linked_list_add_tail($1, $2);
    $$ = $1;
  }
  ;

LocalVariableDeclOrStatement
  : VariableDeclaration {
    $$ = $1;
  }
  | Statement {
    $$ = $1;
  }
  ;
*/
/*--------------------------------------------------------------------------*/

Statement
  : Expression ';' {
    $$ = stmt_from_expr($1);
  }
  /*
  | IfStatement {

  }
  | SwitchStatement {

  }
  | WhileStatement {

  }
  | DoWhileStatement {

  }
  | ForStatement {

  }
  | JumpStatement {
  }
  | ReturnStatement {
    $$ = $1;
  }
  | CodeBlock {
    $$ = $1;
  }
  */
  ;

/*ExpressionStatement
  : Expression {
    $$ = new_stmt($1);
  }
  | AssignmentExpression {
    $$ = $1;
  }
  ;*/
/*
IfStatement
  : IF '(' Expression ')' CodeBlock
  | IF '(' Expression ')' CodeBlock ELSE CodeBlock
  | IF '(' Expression ')' CodeBlock ELSE IfStatement
  ;

SwitchStatement
  : SWITCH '(' Expression ')' '{' CaseStatement '}'
  ;

CaseStatement
  : CASE Expression ':' CodeBlock
  | DEFAULT ':' CodeBlock
  ;

WhileStatement
  : WHILE '(' Expression ')' CodeBlock
  ;

DoWhileStatement
  : DO CodeBlock WHILE '(' Expression ')' ';'
  ;

ForStatement
  : FOR '(' ForInit ForExpr ForIncr ')' CodeBlock
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

JumpStatement
  : BREAK ';'
  | FALLTHROUGH ';'
  | CONTINUE ';'
  ;

ReturnStatement
  : RETURN ';' {
    $$ = new_exp_return(null);
  }
  | RETURN ExpressionList ';' {
    $$ = new_exp_return($2);
    free_linked_list($2);
  }
  ;
*/
ExpressionList
  : Expression {
    $$ = new_list();
    list_add_tail(&($1)->link, $$);
  }
  | ExpressionList ',' Expression {
    list_add_tail(&($3)->link, $1);
    $$ = $1;
  }
  ;

/*-------------------------------------------------------------------------*/

PrimaryExpression
  : Atom {
    $$ = expr_from_atom($1);
  }
  | Atom TrailerList {
    $$ = expr_for_atom($2, $1);
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
    //check_primitive_type($1, $3->term);
    //$$ = $3;
    $$ = NULL;
  }
  | '(' Expression ')' {
    $$ = atom_from_expr($2);
  }
  /*| AnonymousFunctionDeclaration {
    $$ = new_exp_term_anonymous($1);
  }
  | DimExprs BaseType {
    $$ = new_exp_term_array_object($2, $1, 0, null);
    free_linked_list($1);
  }
  | DimExprs DIMS BaseType {
    $$ = new_exp_term_array_object($3, $1, $2, null);
    free_linked_list($1);
  }
  | DIMS BaseType '{' ArrayInitializerList '}' {
    $$ = new_exp_term_array_object($2, null, $1, $4);
    free_linked_list($4);
  }*/
  ;

/* 常量允许访问成员变量和成员方法，不允许数组操作 */
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

/*DimExprs
  : DimExpr {
    $$ = new_linked_list();
    linked_list_add_tail($$, $1);
  }
  | DimExprs DimExpr {
    linked_list_add_tail($1, $2);
    $$ = $1;
  }
  ;

DimExpr
  : '[' Expression ']' {
    $$ = $2;
  }
  ;*/

TrailerList
  : Trailer {
    $$ = new_list();
    list_add_tail(&($1)->link, $$);
  }
  | TrailerList Trailer {
    list_add_tail(&($2)->link, $1);
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
    //free_linked_list($2);
  }
  | '(' ')' {
    $$ = trailer_from_call(NULL);
  }
  /*| '(' ')' '{' FunctionDeclarationList '}' {
    //$$ = new_trailer_interface_implementation();
    //free_linked_list($4);
  }*/
  ;
/*
FunctionDeclarationList
  : FunctionDeclaration {
    $$ = new_linked_list();
    linked_list_add_tail($$, $1);
  }
  | FunctionDeclarationList FunctionDeclaration {
    linked_list_add_tail($1, $2);
    $$ = $1;
  }
  ;*/

/*-------------------------------------------------------------------------*/

PostfixExpression
  : PrimaryExpression {
    $$ = $1;
  }
  /*| PostfixExpression INC {
    $$ = new_exp_unary(OP_INC_AFTER, $1);
  }
  | PostfixExpression DEC {
    $$ = new_exp_unary(OP_DEC_AFTER, $1);
  }*/
  ;

UnaryExpression
  : PostfixExpression {
    $$ = $1;
  }
  /*| INC UnaryExpression {
    if ($2->kind != EXP_TERM) {
      yyerror("rvalue required as increment operand\n");
      exit(-1);
    } else {
      $$ = new_exp_unary(OP_INC_BEFORE, $2);
    }
  }
  | DEC UnaryExpression {
    if ($2->kind != EXP_TERM) {
      yyerror("rvalue required as decrement operand\n");
      exit(-1);
    } else {
      $$ = new_exp_unary(OP_DEC_BEFORE, $2);
    }
  }
  | '+' UnaryExpression {
    $$ = $2;
  }
  | '-' UnaryExpression {
    $$ = new_exp_unary(OP_MINUS, $2);
  }
  | '~' UnaryExpression {
    $$ = new_exp_unary(OP_BNOT, $2);
  }
  | '!' UnaryExpression {
    $$ = new_exp_unary(OP_LNOT, $2);
  }*/
  ;

MultiplicativeExpression
  : UnaryExpression {
    $$ = $1;
  }
  /*| MultiplicativeExpression '*' UnaryExpression {
    $$ = new_exp_binary(OP_TIMES, $1, $3);
  }
  | MultiplicativeExpression '/' UnaryExpression {
    $$ = new_exp_binary(OP_DIVIDE, $1, $3);
  }
  | MultiplicativeExpression '%' UnaryExpression {
    $$ = new_exp_binary(OP_MOD, $1, $3);
  }*/
  ;

AdditiveExpression
  : MultiplicativeExpression {
    $$ = $1;
  }
  /*| AdditiveExpression '+' MultiplicativeExpression {
    $$ = new_exp_binary(OP_PLUS, $1, $3);
  }
  | AdditiveExpression '-' MultiplicativeExpression {
    $$ = new_exp_binary(OP_MINUS, $1, $3);
  }*/
  ;

ShiftExpression
  : AdditiveExpression {
    $$ = $1;
  }
  /*| ShiftExpression SHIFT_LEFT AdditiveExpression {
    $$ = new_exp_binary(OP_LSHIFT, $1, $3);
  }
  | ShiftExpression SHIFT_RIGHT AdditiveExpression {
    $$ = new_exp_binary(OP_RSHIFT, $1, $3);
  }*/
  ;

RelationalExpression
  : ShiftExpression {
    $$ = $1;
  }
  /*| RelationalExpression '<' ShiftExpression {
    $$ = new_exp_binary(OP_LT, $1, $3);
  }
  | RelationalExpression '>' ShiftExpression {
    $$ = new_exp_binary(OP_GT, $1, $3);
  }
  | RelationalExpression LE  ShiftExpression {
    $$ = new_exp_binary(OP_LE, $1, $3);
  }
  | RelationalExpression GE  ShiftExpression {
    $$ = new_exp_binary(OP_GE, $1, $3);
  }*/
  ;

EqualityExpression
  : RelationalExpression {
    $$ = $1;
  }
  /*| EqualityExpression EQ RelationalExpression {
    $$ = new_exp_binary(OP_EQ, $1, $3);
  }
  | EqualityExpression NE RelationalExpression {
    $$ = new_exp_binary(OP_NEQ, $1, $3);
  }*/
  ;

AndExpression
  : EqualityExpression {
    $$ = $1;
  }
  /*| AndExpression '&' EqualityExpression {
    $$ = new_exp_binary(OP_BAND, $1, $3);
  }*/
  ;

ExclusiveOrExpression
  : AndExpression {
    $$ = $1;
  }
  /*| ExclusiveOrExpression '^' AndExpression {
    $$ = new_exp_binary(OP_BXOR, $1, $3);
  }*/
  ;

InclusiveOrExpression
  : ExclusiveOrExpression {
    $$ = $1;
  }
  /*| InclusiveOrExpression '|' ExclusiveOrExpression {
    $$ = new_exp_binary(OP_BOR, $1, $3);
  }*/
  ;

LogicalAndExpression
  : InclusiveOrExpression {
    $$ = $1;
  }
  /*| LogicalAndExpression AND InclusiveOrExpression {
    $$ = new_exp_binary(OP_LAND, $1, $3);
  }*/
  ;

LogicalOrExpression
  : LogicalAndExpression {
    $$ = $1;
  }
  /*| LogicalOrExpression OR LogicalAndExpression {
    $$ = new_exp_binary(OP_LOR, $1, $3);
  }*/
  ;

Expression
  : LogicalOrExpression {
    $$ = $1;
  }
  ;
/*
AssignmentExpression
  : PostfixExpressionList '=' VariableInitializerList {
    $$ = new_exp_assign_list($1, $3, true);
    free_linked_list($1);
    free_linked_list($3);
  }
  | PostfixExpressionList TYPELESS_ASSIGN VariableInitializerList {
    $$ = new_exp_assign_list($1, $3, false);
    free_linked_list($1);
    free_linked_list($3);
  }
  | PostfixExpression CompoundOperator VariableInitializer {
    $$ = new_exp_compound_assign($2, $1, $3);
  }
  ;

PostfixExpressionList
  : PostfixExpression {
    $$ = new_linked_list();
    linked_list_add_tail($$, $1);
  }
  | PostfixExpressionList ',' PostfixExpression {
    linked_list_add_tail($1, $3);
    $$ = $1;
  }
  ;*/

/* 算术运算和位运算 */
/*
CompoundOperator
  : PLUS_ASSGIN {
    $$ = OP_PLUS_ASSIGN;
  }
  | MINUS_ASSIGN {
    $$ = OP_MINUS_ASSIGN;
  }
  | TIMES_ASSIGN {
    $$ = OP_TIMES_ASSIGN;
  }
  | DIVIDE_ASSIGN {
    $$ = OP_DIVIDE_ASSIGN;
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
  | RIGHT_SHIFT_ASSIGN {
    $$ = OP_RIGHT_SHIFT_ASSIGN;
  }
  | LEFT_SHIFT_ASSIGN {
    $$ = OP_LEFT_SHIFT_ASSIGN;
  }
  ;*/

/*--------------------------------------------------------------------------*/

%%
