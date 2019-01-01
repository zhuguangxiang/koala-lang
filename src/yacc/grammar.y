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
%token CHAR_LITERAL
%token INT_LITERAL
%token FLOAT_LITERAL
%token STRING_LITERAL
%token ID

%precedence ')'
%precedence '('

%start CompileUnit

%%

Type
  : BaseType
  | ArrayType
  | VArgType
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

VArgType
  : ELLIPSIS
  | ELLIPSIS BaseType
  ;

MapType
  : MAP '[' KeyType ']' MapValueType
  ;

SetType
  : SET '[' KeyType ']'
  ;

KeyType
  : BYTE
  | CHAR
  | INTEGER
  | STRING
  | ANY
  | KlassType
  ;

MapValueType
  : BaseType
  | ArrayType
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

ParameterList
  : Type
  | ID Type
  | ParameterList ',' Type
  | ParameterList ',' ID Type
  ;

ReturnList
  : Type
  | '(' ParameterList ')'
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
  | TypeAliasDeclaration
  | TypeDeclaration
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
  : CLASS ID ExtendsOrEmpty '{' ClassMemberDeclarationsOrEmpty '}'
  | TRAIT ID WithesOrEmpty '{' TraitMemberDeclarationsOrEmpty '}'
  | CLASS ID ExtendsOrEmpty ';'
  | TRAIT ID WithesOrEmpty ';'
  ;

ExtendsOrEmpty
  : %empty
  | EXTENDS KlassType WithesOrEmpty
  | Traits
  ;

WithesOrEmpty
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
  | ProtoDeclaration
  | FunctionDeclaration
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
 * PrimaryExpressionList is really IDList
 */
VariableDeclarationTypeless
  : PrimaryExpressionList TYPELESS_ASSIGN ExpressionList ';'
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
 * PrimaryExpressionList is really IDList
 */
RangeCause
  : PrimaryExpressionList '=' RangeExpression
  | PrimaryExpressionList TYPELESS_ASSIGN RangeExpression
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
  | ArrayExpression
  | SetExpression
  | MapExpression
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

ArrayExpression
  : DimExprList Type LiteralValue
  | DIMS BaseType LiteralValue
  ;

DimExprList
  : '[' Expression ']'
  | DimExprList '[' Expression ']'
  ;

LiteralValue
  : '{' ElementList '}'
  | '{' '}'
  | '(' ')'
  ;

ElementList
  : Element
  | ElementList ',' Element
  ;

Element
  : Expression
  | LiteralValue
  ;

SetExpression
  : SetType LiteralValue
  ;

MapExpression
  : MapType MapLiteralValue
  ;

MapLiteralValue
  : '{' MapKeyValueList '}'
  | '{' '}'
  | '(' ')'
  ;

MapKeyValueList
  : MapKeyValue
  | MapKeyValueList ',' MapKeyValue
  ;

MapKeyValue
  : MapKey ':' MapValue
  ;

MapKey
  : PrimaryExpression
  ;

MapValue
  : Expression
  | MapLiteralValue
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
  : PrimaryExpressionList '=' ExpressionList ';'
  | PrimaryExpression CompAssignOp Expression ';'
  ;

PrimaryExpressionList
  : PrimaryExpression
  | PrimaryExpressionList ',' PrimaryExpression
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
