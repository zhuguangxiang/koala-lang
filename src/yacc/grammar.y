/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

%{

%}

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
%token POWER
%token ELLIPSIS
%token DOTDOTLESS
%token GREATERDOTDOT

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
%token FOR
%token SWITCH
%token CASE
%token BREAK
%token FALLTHROUGH
%token CONTINUE
%token DEFAULT
%token VAR
%token FUNC
%token TOKEN_RETURN
%token CLASS
%token TRAIT
%token EXTENDS
%token WITH
%token CONST
%token PACKAGE
%token IMPORT
%token GO
%token DEFER
%token TYPEALIAS
%token NATIVE

%token CHAR
%token BYTE
%token INTEGER
%token FLOAT
%token BOOL
%token STRING
%token ERROR
%token ANY
%token MAP
%token SET

%token SELF
%token SUPER
%token TOKEN_NIL
%token TOKEN_TRUE
%token TOKEN_FALSE

%token DIMS
%token BYTE_LITERAL
%token CHAR_LITERAL
%token INT_LITERAL
%token FLOAT_LITERAL
%token STRING_LITERAL
%token ID

%token PREC_0
%token PREC_1

%precedence ID
%precedence '.'
%precedence ')'
%precedence '('
%precedence PREC_0
%precedence '{'
//%precedence PREC_1

%start CompileUnit

%%

Type
  : BaseType
  | ArrayType
  ;

ArrayType
  : DIMS BaseType
  ;

BaseType
  : PrimitiveType
  | KlassType
  | FunctionType
  | MapType
  | SetType
  ;

MapType
  : MAP '[' Type ']' Type
  ;

SetType
  : SET '[' Type ']'
  ;

PrimitiveType
  : BYTE
  | CHAR
  | INTEGER
  | FLOAT
  | BOOL
  | STRING
  | ERROR
  | ANY
  ;

KlassType
  : ID
  | ID '.' ID
  ;

FunctionType
  : FUNC '(' ParameterList ')' ReturnList
  | FUNC '(' ParameterList ')'
  | FUNC '(' ')' ReturnList
  | FUNC '(' ')'
  ;

IDTypeList
  : Type
  | ID Type
  | IDTypeList ',' Type
  | IDTypeList ',' ID Type
  ;

ParameterList
  : IDTypeList
  | IDTypeList VArgType
  | VArgType
  ;

VArgType
  : ELLIPSIS
  | ELLIPSIS BaseType
  ;

ReturnList
  : Type
  | '(' IDTypeList ')'
  ;

IDList
  : ID
  | IDList ',' ID
  ;

CompileUnit
  : Package Imports ModuleStatements
  | Package ModuleStatements
  | Imports ModuleStatements
  | ModuleStatements
  ;

Package
  : PACKAGE ID ';'
  ;

Imports
  : Import
  | Imports Import
  ;

Import
  : IMPORT PackagePath ';'
  | IMPORT ID PackagePath ';'
  | IMPORT '*' PackagePath ';'
  ;

PackagePath
  : STRING_LITERAL
  ;

ModuleStatements
  : ModuleStatement
  | ModuleStatements ModuleStatement
  ;

ModuleStatement
  : ';'
  | VariableDeclaration
  | ConstDeclaration
  | FunctionDeclaration
  | NATIVE ProtoDeclaration
  | TypeAliasDeclaration
  | TypeDeclaration
  | error
  ;

ConstDeclaration
  : CONST IDList '=' ExpressionList ';'
  | CONST IDList Type '=' ExpressionList ';'
  ;

VariableDeclaration
  : VAR IDList Type ';'
  | VAR IDList '=' ExpressionList ';'
  | VAR IDList Type '=' ExpressionList ';'
  ;

FunctionDeclaration
  : FUNC ID '(' ParameterList ')' ReturnList Block
  | FUNC ID '(' ParameterList ')' Block
  | FUNC ID '(' ')' ReturnList Block
  | FUNC ID '(' ')' Block
  ;

TypeAliasDeclaration
  : TYPEALIAS ID Type ';'
  ;

TypeDeclaration
  : CLASS ID Extends '{' ClassMemberDeclarationsOrEmpty '}'
  | CLASS ID Extends ';'
  | TRAIT ID Extends '{' TraitMemberDeclarationsOrEmpty '}'
  | TRAIT ID Extends ';'
  ;

Extends
  : %empty
  | EXTENDS KlassType Withes
  ;

Withes
  : %empty
  | Traits
  ;

Traits
  : WITH KlassType
  | Traits WITH KlassType
  ;

ClassMemberDeclarationsOrEmpty
  : %empty
  | ClassMemberDeclarations
  ;

ClassMemberDeclarations
  : ClassMemberDeclaration
  | ClassMemberDeclarations ClassMemberDeclaration
  ;

ClassMemberDeclaration
  : FieldDeclaration
  | FunctionDeclaration
  | NATIVE ProtoDeclaration
  | ';'
  ;

TraitMemberDeclarationsOrEmpty
  : %empty
  | TraitMemberDeclarations
  ;

TraitMemberDeclarations
  : TraitMemberDeclaration
  | TraitMemberDeclarations TraitMemberDeclaration
  ;

TraitMemberDeclaration
  : FieldDeclaration
  | FunctionDeclaration
  | NATIVE ProtoDeclaration
  | ProtoDeclaration
  | ';'
  ;

FieldDeclaration
  : ID Type ';'
  | ID '=' Expression ';'
  | ID Type '=' Expression ';'
  ;

ProtoDeclaration
  : FUNC ID '(' ParameterList ')' ReturnList ';'
  | FUNC ID '(' ParameterList ')' ';'
  | FUNC ID '(' ')' ReturnList ';'
  | FUNC ID '(' ')' ';'
  ;

Block
  : '{' LocalStatements '}'
  | '{' '}'
  ;

LocalStatements
  : LocalStatement
  | LocalStatements LocalStatement
  ;

LocalStatement
  : ';'
  | ExprStatement
  | VariableDeclaration
  | VariableDeclarationTypeless
  | Assignment
  | IfStatement
  | WhileStatement
  | SwitchStatement
  | ForStatement
  | JumpStatement
  | ReturnStatement
  | Block
  ;

ExprStatement
  : Expression ';'
  ;

/*
 * TYPELESS_ASSIGN is used only in local blocks
 * ExpressionList is really IDList
 */
VariableDeclarationTypeless
  : ExpressionList TYPELESS_ASSIGN ExpressionList ';'
  ;

IfStatement
  : IF Expression Block OrElseStatement
  ;

OrElseStatement
  : %empty
  | ELSE Block
  | ELSE IfStatement
  ;

WhileStatement
  : WHILE Expression Block
  ;

SwitchStatement
  : SWITCH Expression '{' CaseStatements '}'
  ;

CaseStatements
  : CaseStatement
  | CaseStatements CaseStatement
  ;

CaseStatement
  : CASE Expression ':' Block
  | DEFAULT ':' Block
  ;

ForStatement
  : FOR RangeCause Block
  ;

/*
 * TYPELESS_ASSIGN is used only in local blocks
 */
RangeCause
  : IDList '=' RangeExpression
  | IDList TYPELESS_ASSIGN RangeExpression
  ;

/*
 * 'Expression' is Range
 * 'Expression ELLIPSIS Expression' is Range
 * 'Expression DOTDOTLESS Expression' is Range
 * 'Expression GREATERDOTDOT Expression' is Range
 */
RangeExpression
  : Expression
  | Expression ELLIPSIS Expression
  | Expression DOTDOTLESS Expression
  | Expression GREATERDOTDOT Expression
  ;

JumpStatement
  : BREAK ';'
  | CONTINUE ';'
  ;

ReturnStatement
  : TOKEN_RETURN ';'
  | TOKEN_RETURN ExpressionList ';'
  | TOKEN_RETURN error
  | TOKEN_RETURN ExpressionList error
  ;

PrimaryExpression
  : Atom
  | PrimaryExpression Selector
  | PrimaryExpression Arguments
  | PrimaryExpression Index
  | PrimaryExpression Slice
  ;

Selector
  : '.' ID
  ;

Arguments
  : '(' ')'
  | '(' ExpressionList ')'
  ;

Index
  : '[' Expression ']'
  ;

Slice
  : '[' Expression ':' Expression ']'
  | '[' ':' Expression ']'
  | '['Expression ':' ']'
  | '[' ':' ']'
  ;

Atom
  : ID
  | CONSTANT
  | SELF
  | SUPER
  | '(' Expression ')'
  | ArrayCreationExpression
  | SetCreationExpression
  | ArrayOrSetInitializer
  | MapCreationExpression
  | MapInitializer
  | AnonyFuncExpression
  ;

CONSTANT
  : CHAR_LITERAL
  | INT_LITERAL
  | FLOAT_LITERAL
  | STRING_LITERAL
  | TOKEN_NIL
  | TOKEN_TRUE
  | TOKEN_FALSE
  ;

ArrayCreationExpression
  : DimExprList Type ArrayOrSetInitializer
  | DimExprList Type
  {

  } %prec PREC_0
  | DIMS BaseType ArrayOrSetInitializer
  | DIMS BaseType
  {

  } %prec PREC_0
  ;

DimExprList
  : '[' Expression ']'
  | DimExprList '[' Expression ']'
  ;

SetCreationExpression
  : SetType ArrayOrSetInitializer
  | SetType
  {

  } %prec PREC_0
  ;

ArrayOrSetInitializer
  : '{' ExpressionList '}'
  ;

MapCreationExpression
  : MapType MapInitializer
  | MapType
  {

  } %prec PREC_0
  ;

MapInitializer
  : '{' MapKeyValueList '}'
  ;

MapKeyValueList
  : MapKeyValue
  | MapKeyValueList ',' MapKeyValue
  ;

MapKeyValue
  : Expression ':' Expression
  ;

AnonyFuncExpression
  : FUNC '(' ParameterList ')' ReturnList Block
  | FUNC '(' ParameterList ')' Block
  | FUNC '(' ')' ReturnList Block
  | FUNC '(' ')' Block
  | FUNC error
  ;

UnaryExpression
  : PrimaryExpression
  | UnaryOp UnaryExpression
  ;

UnaryOp
  : '+'
  | '-'
  | '~'
  | NOT
  ;

MultipleExpression
  : UnaryExpression
  | MultipleExpression '*' UnaryExpression
  | MultipleExpression '/' UnaryExpression
  | MultipleExpression '%' UnaryExpression
  | MultipleExpression POWER UnaryExpression
  ;

AddExpression
  : MultipleExpression
  | AddExpression '+' MultipleExpression
  | AddExpression '-' MultipleExpression
  ;

ShiftExpression
  : AddExpression
  | ShiftExpression LSHIFT AddExpression
  | ShiftExpression RSHIFT AddExpression
  ;

RelationExpression
  : ShiftExpression
  | RelationExpression '<' ShiftExpression
  | RelationExpression '>' ShiftExpression
  | RelationExpression LE  ShiftExpression
  | RelationExpression GE  ShiftExpression
  ;

EqualityExpression
  : RelationExpression
  | EqualityExpression EQ RelationExpression
  | EqualityExpression NE RelationExpression
  ;

AndExpression
  : EqualityExpression
  | AndExpression '&' EqualityExpression
  ;

ExclusiveOrExpression
  : AndExpression
  | ExclusiveOrExpression '^' AndExpression
  ;

InclusiveOrExpression
  : ExclusiveOrExpression
  | InclusiveOrExpression '|' ExclusiveOrExpression
  ;

LogicAndExpression
  : InclusiveOrExpression
  | LogicAndExpression AND InclusiveOrExpression
  ;

LogicOrExpression
  : LogicAndExpression
  | LogicOrExpression OR LogicAndExpression
  ;

Expression
  : LogicOrExpression
  ;

ExpressionList
  : Expression
  | ExpressionList ',' Expression
  ;

Assignment
  : ExpressionList '=' ExpressionList ';'
  | PrimaryExpression CompAssignOp Expression ';'
  ;

CompAssignOp
  : PLUS_ASSGIN
  | MINUS_ASSIGN
  | MULT_ASSIGN
  | DIV_ASSIGN
  | MOD_ASSIGN
  | AND_ASSIGN
  | OR_ASSIGN
  | XOR_ASSIGN
  | RSHIFT_ASSIGN
  | LSHIFT_ASSIGN
  ;

%%
