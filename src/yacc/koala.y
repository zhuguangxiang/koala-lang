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

#include "parser.h"

#if 1
#define YYERROR_VERBOSE 1
static int yyerror(void *loc, ParserState *ps, void *scanner, const char *msg)
{
  UNUSED_PARAMETER(ps);
  UNUSED_PARAMETER(scanner);
  fprintf(stderr, ">>>>> %s\n", msg);
  return 0;
}
#else
#define yyerror(yylloc, ps, scanner, errmsg) ((void)0)
#endif

#define ERRORMSG "expected '%s' before '%s'\n"

#define Syntax_Error(loc, expected) \
  yyerrok; \
  Parser_Synatx_Error(ps, loc, ERRORMSG, expected, ps->line.token)

#define Syntax_Error_Clear(loc, msg) \
  yyclearin; \
  yyerrok; \
  Parser_Synatx_Error(ps, loc, "%s\n", msg)

%}

%union {
  int Dims;
  int64 IVal;
  float64 FVal;
  String SVal;
  Vector *List;
  Expr *Expr;
  Stmt *Stmt;
  TypeDesc *TypeDesc;
  int Operator;
}

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

%token <Dims> DIMS
%token <IVal> BYTE_LITERAL
%token <IVal> CHAR_LITERAL
%token <IVal> INT_LITERAL
%token <FVal> FLOAT_LITERAL
%token <SVal> STRING_LITERAL
%token <SVal> ID

%type <TypeDesc> Type
%type <TypeDesc> ArrayType
%type <TypeDesc> BaseType
%type <TypeDesc> VArgType
%type <TypeDesc> PrimitiveType
%type <TypeDesc> KlassType
%type <TypeDesc> FunctionType
%type <TypeDesc> MapType
%type <TypeDesc> SetType
%type <TypeDesc> KeyType

%type <List> IDTypeList
%type <List> ParameterList
%type <List> ReturnList
%type <List> IDList

%type <SVal> PackagePath

%type <Stmt> ConstDeclaration
%type <Stmt> VariableDeclaration
%type <Stmt> FunctionDeclaration
%type <Stmt> TypeAliasDeclaration
%type <Stmt> TypeDeclaration
%type <List> ClassMemberDeclarationsOrEmpty
%type <List> ClassMemberDeclarations
%type <Stmt> ClassMemberDeclaration
%type <List> Extends
%type <List> ExtendsMore
%type <List> Traits
%type <List> TraitMemberDeclarationsOrEmpty
%type <List> TraitMemberDeclarations
%type <Stmt> TraitMemberDeclaration
%type <Stmt> FieldDeclaration
%type <Stmt> ProtoDeclaration
%type <List> Block
%type <List> LocalStatements
%type <Stmt> LocalStatement
%type <Stmt> ExprStatement
%type <Stmt> VariableDeclarationTypeless
%type <Stmt> Assignment
%type <Stmt> IfStatement
%type <Stmt> OrElseStatement
%type <Stmt> WhileStatement
%type <Stmt> SwitchStatement
%type <Stmt> CaseStatements
//%type <testblock> CaseStatement
%type <Stmt> ForStatement
%type <Stmt> JumpStatement
%type <Stmt> ReturnStatement

%type <Expr> PrimaryExpression
%type <Expr> Atom
%type <SVal> Selector
%type <Expr> Index
%type <List> Arguments
%type <Expr> CONSTANT
%type <Expr> ArrayCreationExpression
%type <Expr> SetCreationExpression
%type <Expr> ArrayOrSetInitializer
%type <Expr> MapCreationExpression
%type <Expr> MapInitializer
%type <Expr> AnonyFuncExpression
%type <Operator> UnaryOp
%type <Expr> UnaryExpression
%type <Expr> MultipleExpression
%type <Expr> AddExpression
%type <Expr> ShiftExpression
%type <Expr> RelationExpression
%type <Expr> EqualityExpression
%type <Expr> AndExpression
%type <Expr> ExclusiveOrExpression
%type <Expr> InclusiveOrExpression
%type <Expr> LogicAndExpression
%type <Expr> LogicOrExpression
%type <Expr> Expression
%type <List> ExpressionList
%type <Operator> CompAssignOp

%token PREC_0
%token PREC_1

%precedence ID
%precedence '.'
%precedence ')'
%precedence '('
%precedence PREC_0
%precedence '{'
//%precedence PREC_1

%locations

%parse-param {ParserState *ps}
%parse-param {void *scanner}
%define api.pure full
%lex-param {void *scanner}
%code provides {
  int yylex(YYSTYPE *yylval_param, YYLTYPE *yylloc, void *yyscanner);
}

%start CompileUnit

%%

Type
  : BaseType
  {
    $$ = $1;
  }
  | ArrayType
  {
    $$ = $1;
  }
  ;

ArrayType
  : DIMS BaseType
  {
    $$ = TypeDesc_Get_Array($1, $2);
  }
  ;

BaseType
  : PrimitiveType
  {
    $$ = $1;
  }
  | KlassType
  {
    $$ = $1;
  }
  | FunctionType
  {
    $$ = $1;
  }
  | MapType
  {
    $$ = $1;
   }
  | SetType
  {
    $$ = $1;
  }
  ;

MapType
  : MAP '[' KeyType ']' Type
  {
    $$ = TypeDesc_Get_Map($3, $5);
  }
  ;

SetType
  : SET '[' KeyType ']'
  {
    $$ = TypeDesc_Get_Set($3);
  }
  ;

KeyType
  : INTEGER
  {
    $$ = TypeDesc_Get_Basic(BASIC_INT);
  }
  | STRING
  {
    $$ = TypeDesc_Get_Basic(BASIC_STRING);
  }
  | KlassType
  {
    $$ = $1;
  }
  ;

PrimitiveType
  : BYTE
  {
    $$ = TypeDesc_Get_Basic(BASIC_BYTE);
  }
  | CHAR
  {
    $$ = TypeDesc_Get_Basic(BASIC_CHAR);
  }
  | INTEGER
  {
    $$ = TypeDesc_Get_Basic(BASIC_INT);
  }
  | FLOAT
  {
    $$ = TypeDesc_Get_Basic(BASIC_FLOAT);
  }
  | BOOL
  {
    $$ = TypeDesc_Get_Basic(BASIC_BOOL);
  }
  | STRING
  {
    $$ = TypeDesc_Get_Basic(BASIC_STRING);
  }
  | ERROR
  {
    $$ = TypeDesc_Get_Basic(BASIC_ERROR);
  }
  | ANY
  {
    $$ = TypeDesc_Get_Basic(BASIC_ANY);
  }
  ;

KlassType
  : ID
  {
    $$ = Parser_New_KlassType(ps, NULL, $1.str);
  }
  | ID '.' ID
  {
    $$ = Parser_New_KlassType(ps, $1.str, $3.str);
  }
  ;

FunctionType
  : FUNC '(' ParameterList ')' ReturnList
  {
    $$ = TypeDesc_Get_Proto($3, $5);
  }
  | FUNC '(' ParameterList ')'
  {
    $$ = TypeDesc_Get_Proto($3, NULL);
  }
  | FUNC '(' ')' ReturnList
  {
    $$ = TypeDesc_Get_Proto(NULL, $4);
  }
  | FUNC '(' ')'
  {
    $$ = TypeDesc_Get_Proto(NULL, NULL);
  }
  ;

IDTypeList
  : Type
  {
    $$ = Vector_New();
    Vector_Append($$, New_IdType(NULL, $1));
  }
  | ID Type
  {
    $$ = Vector_New();
    Vector_Append($$, New_IdType($1.str, $2));
  }
  | IDTypeList ',' Type
  {
    $$ = $1;
    Vector_Append($$, New_IdType(NULL, $3));
  }
  | IDTypeList ',' ID Type
  {
    $$ = $1;
    Vector_Append($$, New_IdType($3.str, $4));
  }
  ;

ParameterList
  : IDTypeList
  {
    $$ = $1;
  }
  | IDTypeList VArgType
  {
    $$ = $1;
    Vector_Append($$, New_IdType(NULL, $2));
  }
  | VArgType
  {
    $$ = Vector_New();
    Vector_Append($$, New_IdType(NULL, $1));
  }
  ;

VArgType
  : ELLIPSIS
  {
    TypeDesc *any = TypeDesc_Get_Basic(BASIC_ANY);
    $$ = TypeDesc_Get_Varg(any);
  }
  | ELLIPSIS BaseType
  {
    $$ = TypeDesc_Get_Varg($2);
  }
  ;

ReturnList
  : Type
  {
    $$ = Vector_New();
    Vector_Append($$, New_IdType(NULL, $1));
  }
  | '(' IDTypeList ')'
  {
    $$ = $2;
  }
  ;

IDList
  : ID
  {
    $$ = Vector_New();
    Vector_Append($$, $1.str);
  }
  | IDList ',' ID
  {
    $$ = $1;
    Vector_Append($$, $3.str);
  }
  ;

CompileUnit
  : Imports ModuleStatements
  | ModuleStatements
  ;

Imports
  : Import
  | Imports Import
  ;

Import
  : IMPORT PackagePath ';'
  {
    Parser_New_Import(ps, NULL, $2.str, NULL, &@2);
  }
  | IMPORT ID PackagePath ';'
  {
    Parser_New_Import(ps, $2.str, $3.str, NULL, NULL);
  }
  | IMPORT ID error
  {
    Syntax_Error_Clear(&@3, "invalid import");
  }
  | IMPORT error
  {
    Syntax_Error_Clear(&@2, "invalid import");
  }
  ;

PackagePath
  : STRING_LITERAL
  {
    $$ = $1;
  }
  ;

ModuleStatements
  : ModuleStatement
  | ModuleStatements ModuleStatement
  ;

ModuleStatement
  : ';'
  {
  }
  | VariableDeclaration
  {
    Parser_New_Variables(ps, $1);
  }
  | ConstDeclaration
  {
    Parser_New_Variables(ps, $1);
  }
  | FunctionDeclaration
  {
    Parser_New_Function(ps, $1);
  }
  | NATIVE ProtoDeclaration
  {
    $2->native = 1;
    Parser_New_Function(ps, $2);
  }
  | TypeAliasDeclaration
  {
    Parser_New_TypeAlias(ps, $1);
  }
  | TypeDeclaration
  {
    Parser_New_ClassOrTrait(ps, $1);
  }
  | error
  {
    Syntax_Error_Clear(&@1, "invalid statement");
  }
  ;

ConstDeclaration
  : CONST IDList '=' ExpressionList ';'
  {
    $$ = Parser_Do_Constants(ps, $2, NULL, $4);
  }
  | CONST IDList Type '=' ExpressionList ';'
  {
    $$ = Parser_Do_Constants(ps, $2, $3, $5);
  }
  | CONST IDList '=' error
  {
    Syntax_Error(&@4, "expr-list");
    $$ = NULL;
  }
  | CONST IDList Type '=' error
  {
    Syntax_Error(&@5, "expr-list");
    $$ = NULL;
  }
  | CONST error
  {
    Syntax_Error(&@2, "id-list");
    $$ = NULL;
  }
  ;

VariableDeclaration
  : VAR IDList Type ';'
  {
    $$ = Parser_Do_Variables(ps, $2, $3, NULL);
  }
  | VAR IDList '=' ExpressionList ';'
  {
    $$ = Parser_Do_Variables(ps, $2, NULL, $4);
  }
  | VAR IDList Type '=' ExpressionList ';'
  {
    $$ = Parser_Do_Variables(ps, $2, $3, $5);
  }
  | VAR IDList error
  {
    Syntax_Error(&@3, "'TYPE' or '='");
    $$ = NULL;
  }
  | VAR IDList '=' error
  {
    Syntax_Error(&@4, "right's expression-list");
    $$ = NULL;
  }
  | VAR IDList Type '=' error
  {
    Syntax_Error(&@5, "right's expression-list");
    $$ = NULL;
  }
  | VAR error
  {
    Syntax_Error(&@2, "id-list");
    $$ = NULL;
  }
  ;

FunctionDeclaration
  : FUNC ID '(' ParameterList ')' ReturnList Block
  {
    $$ = Stmt_From_FuncDecl($2.str, $4, $6, $7);
  }
  | FUNC ID '(' ParameterList ')' Block
  {
    $$ = Stmt_From_FuncDecl($2.str, $4, NULL, $6);
  }
  | FUNC ID '(' ')' ReturnList Block
  {
    $$ = Stmt_From_FuncDecl($2.str, NULL, $5, $6);
  }
  | FUNC ID '(' ')' Block
  {
    $$ = Stmt_From_FuncDecl($2.str, NULL, NULL, $5);
  }
  | FUNC error
  {
    Syntax_Error(&@2, "ID");
    $$ = NULL;
  }
  ;

TypeAliasDeclaration
  : TYPEALIAS ID Type ';'
  {
    $$ = Stmt_From_TypeAlias($2.str, $3);
  }
  | TYPEALIAS ID error
  {
    Syntax_Error(&@3, "TYPE");
    $$ = NULL;
  }
  | TYPEALIAS error
  {
    Syntax_Error(&@2, "ID");
    $$ = NULL;
  }
  ;

TypeDeclaration
  : CLASS ID Extends '{' ClassMemberDeclarationsOrEmpty '}'
  {
    $$ = Stmt_From_Klass($2.str, CLASS_KIND, $3, $5);
  }
  | CLASS ID Extends ';'
  {
    $$ = Stmt_From_Klass($2.str, CLASS_KIND, $3, NULL);
  }
  | TRAIT ID Extends '{' TraitMemberDeclarationsOrEmpty '}'
  {
    $$ = Stmt_From_Klass($2.str, TRAIT_KIND, $3, $5);
  }
  | TRAIT ID Extends ';'
  {
    $$ = Stmt_From_Klass($2.str, TRAIT_KIND, $3, NULL);
  }
  ;

Extends
  : %empty
  {
    $$ = NULL;
  }
  | ':' KlassType ExtendsMore
  {
    $$ = Vector_New();
    Vector_Append($$, $2);
    Vector_Concat($$, $3);
    Vector_Free_Self($3);
  }
  ;

ExtendsMore
  : %empty
  {
    $$ = NULL;
  }
  | Traits
  {
    $$ = $1;
  }
  ;

Traits
  : ',' KlassType
  {
    $$ = Vector_New();
    Vector_Append($$, $2);
  }
  | Traits ',' KlassType
  {
    $$ = $1;
    Vector_Append($$, $3);
  }
  ;

ClassMemberDeclarationsOrEmpty
  : %empty
  {
    $$ = NULL;
  }
  | ClassMemberDeclarations
  {
    $$ = $1;
  }
  ;

ClassMemberDeclarations
  : ClassMemberDeclaration
  {
    $$ = Vector_New();
    if ($1 != NULL)
      Vector_Append($$, $1);
  }
  | ClassMemberDeclarations ClassMemberDeclaration
  {
    $$ = $1;
    if ($2 != NULL)
      Vector_Append($$, $2);
  }
  ;

ClassMemberDeclaration
  : FieldDeclaration
  {
    $$ = $1;
  }
  | FunctionDeclaration
  {
    $$ = $1;
  }
  | NATIVE ProtoDeclaration
  {
    $2->native = 1;
    $$ = $2;
  }
  | ';'
  {
    $$ = NULL;
  }
  ;

TraitMemberDeclarationsOrEmpty
  : %empty
  {
    $$ = NULL;
  }
  | TraitMemberDeclarations
  {
    $$ = $1;
  }
  ;

TraitMemberDeclarations
  : TraitMemberDeclaration
  {
    $$ = Vector_New();
    if ($1 != NULL)
      Vector_Append($$, $1);
  }
  | TraitMemberDeclarations TraitMemberDeclaration
  {
    $$ = $1;
    if ($2 != NULL)
      Vector_Append($$, $2);
  }
  ;

TraitMemberDeclaration
  : ClassMemberDeclaration
  {
    $$ = $1;
  }
  | ProtoDeclaration
  {
    $$ = $1;
  }
  ;

FieldDeclaration
  : ID Type ';'
  {
    $$ = Stmt_From_VarDecl($1.str, $2, NULL);
  }
  | ID '=' Expression ';'
  {
    $$ = Stmt_From_VarDecl($1.str, NULL, $3);
  }
  | ID Type '=' Expression ';'
  {
    $$ = Stmt_From_VarDecl($1.str, $2, $4);
  }
  ;

ProtoDeclaration
  : FUNC ID '(' ParameterList ')' ReturnList ';'
  {
    $$ = Stmt_From_ProtoDecl($2.str, $4, $6);
  }
  | FUNC ID '(' ParameterList ')' ';'
  {
    $$ = Stmt_From_ProtoDecl($2.str, $4, NULL);
  }
  | FUNC ID '(' ')' ReturnList ';'
  {
    $$ = Stmt_From_ProtoDecl($2.str, NULL, $5);
  }
  | FUNC ID '(' ')' ';'
  {
    $$ = Stmt_From_ProtoDecl($2.str, NULL, NULL);
  }
  ;

Block
  : '{' LocalStatements '}'
  {
    $$ = $2;
  }
  | '{' '}'
  {
    $$ = NULL;
  }
  ;

LocalStatements
  : LocalStatement
  {
    $$ = Vector_New();
    if ($1 != NULL)
      Vector_Append($$, $1);
  }
  | LocalStatements LocalStatement
  {
    $$ = $1;
    if ($2 != NULL)
      Vector_Append($$, $2);
  }
  ;

LocalStatement
  : ';'
  {
    $$ = NULL;
  }
  | ExprStatement
  {
    $$ = $1;
  }
  | VariableDeclaration
  {
    $$ = $1;
  }
  | VariableDeclarationTypeless
  {
    $$ = $1;
  }
  | Assignment
  {
    $$ = $1;
  }
  | IfStatement
  {
    $$ = $1;
  }
  | WhileStatement
  {
    $$ = $1;
  }
  | SwitchStatement
  {

  }
  | ForStatement
  {
    $$ = $1;
  }
  | JumpStatement
  {
    $$ = $1;
  }
  | ReturnStatement
  {
    $$ = $1;
  }
  | Block
  {
    $$ = Stmt_From_List($1);
  }
  | error
  {
    Syntax_Error_Clear(&@1, "invalid local statement");
    $$ = NULL;
  }
  ;

ExprStatement
  : Expression ';'
  {
    $$ = Stmt_From_Expr($1);
  }
  ;

/*
 * TYPELESS_ASSIGN is used only in local blocks
 * ExpressionList is really IDList
 */
VariableDeclarationTypeless
  : ExpressionList TYPELESS_ASSIGN ExpressionList ';'
  {

  }
  ;

IfStatement
  : IF Expression Block OrElseStatement
  {
    //$$ = stmt_from_if($2, $3, $4);
    //$$->if_stmt.belse = 0;
  }
  ;

OrElseStatement
  : %empty
  {
    $$ = NULL;
  }
  | ELSE Block
  {
    //$$ = stmt_from_if(NULL, $2, NULL);
    //$$->if_stmt.belse = 1;
  }
  | ELSE IfStatement
  {
    //$$ = $2;
    //$$->if_stmt.belse = 1;
  }
  ;

WhileStatement
  : WHILE Expression Block
  {
    //stmt_from_while($2, $3, 1);
  }
  ;

SwitchStatement
  : SWITCH Expression '{' CaseStatements '}'
  {

  }
  ;

CaseStatements
  : CaseStatement
  {
    /*
    $$ = Vector_New();
    Vector_Append($$, $1);
    */
  }
  | CaseStatements CaseStatement
  {
    /*
    $$ = $1;
    if ($2->test == NULL) {
      // default case
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
    */
  }
  ;

CaseStatement
  : CASE Expression ':' Block
  {
    //$$ = new_test_block($2, $4);
  }
  | DEFAULT ':' Block
  {
    //$$ = new_test_block(NULL, $3);
  }
  ;

ForStatement
  : FOR RangeCause Block
  {
    $$ = NULL;
  }
  ;

/*
 * TYPELESS_ASSIGN is used only in local blocks
 */
RangeCause
  : IDList '=' RangeExpression
  {

  }
  | IDList TYPELESS_ASSIGN RangeExpression
  {
  }
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
  {
    //$$ = stmt_from_jump(BREAK_KIND, 1);
  }
  | CONTINUE ';'
  {
    //$$ = stmt_from_jump(CONTINUE_KIND, 1);
  }
  ;

ReturnStatement
  : TOKEN_RETURN ';'
  {
    $$ = Stmt_From_Return(NULL);
  }
  | TOKEN_RETURN ExpressionList ';'
  {
    $$ = Stmt_From_Return($2);
  }
  | TOKEN_RETURN error
  {

  }
  | TOKEN_RETURN ExpressionList error
  {

  }
  ;

PrimaryExpression
  : Atom
  {
    $$ = $1;
  }
  | PrimaryExpression Selector
  {
    //$$ = expr_from_trailer(ATTRIBUTE_KIND, $2, $1);
  }
  | PrimaryExpression Arguments
  {
    //$$ = expr_from_trailer(CALL_KIND, $2, $1);
  }
  | PrimaryExpression Index
  {
    //$$ = expr_from_trailer(SUBSCRIPT_KIND, $2, $1);
  }
  | PrimaryExpression Slice
  {
    $$ = NULL;
  }
  ;

Selector
  : '.' ID
  {

  }
  ;

Arguments
  : '(' ')'
  {

  }
  | '(' ExpressionList ')'
  {

  }
  ;

Index
  : '[' Expression ']'
  {

  }
  ;

Slice
  : '[' Expression ':' Expression ']'
  | '[' ':' Expression ']'
  | '['Expression ':' ']'
  | '[' ':' ']'
  ;

Atom
  : ID
  {
    $$ = Expr_From_Id($1.str);
  }
  | CONSTANT
  {
    $$ = $1;
  }
  | SELF
  {
    $$ = Expr_From_Self();
  }
  | SUPER
  {
    $$ = Expr_From_Super();
  }
  | '(' Expression ')'
  {
    $$ = $2;
  }
  | ArrayCreationExpression
  {

  }
  | SetCreationExpression
  {

  }
  | ArrayOrSetInitializer
  {
    printf("ArrayOrSetInitializer\n");
  }
  | MapCreationExpression
  {

  }
  | MapInitializer
  | AnonyFuncExpression
  {

  }
  ;

CONSTANT
  : CHAR_LITERAL
  {
    $$ = NULL;
  }
  | INT_LITERAL
  {
    $$ = Expr_From_Integer($1);
  }
  | FLOAT_LITERAL
  {
    $$ = Expr_From_Float($1);
  }
  | STRING_LITERAL
  {
    $$ = Expr_From_String($1.str);
  }
  | TOKEN_NIL
  {
    $$ = Expr_From_Nil();
  }
  | TOKEN_TRUE
  {
    $$ = Expr_From_Bool(1);
  }
  | TOKEN_FALSE
  {
    $$ = Expr_From_Bool(0);
  }
  ;

ArrayCreationExpression
  : DimExprList Type ArrayOrSetInitializer
  {
    printf("DimExprList Type ArrayOrSetInitializer\n");
  }
  | DimExprList Type
  {
    printf("DimExprList Type\n");
  } %prec PREC_0
  | DIMS BaseType ArrayOrSetInitializer
  {

  }
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
  {

  }
  | SetType
  {

  } %prec PREC_0
  ;

ArrayOrSetInitializer
  : '{' ExpressionList '}'
  {

  }
  ;

MapCreationExpression
  : MapType MapInitializer
  {

  }
  | MapType
  {

  } %prec PREC_0
  ;

MapInitializer
  : '{' MapKeyValueList '}'
  {

  }
  ;

MapKeyValueList
  : MapKeyValue
  {

  }
  | MapKeyValueList ',' MapKeyValue
  {

  }
  ;

MapKeyValue
  : Expression ':' Expression
  {

  }
  ;

AnonyFuncExpression
  : FUNC '(' ParameterList ')' ReturnList Block
  {
    // $$ = expr_from_anonymous_func($2, $3, $4);
  }
  | FUNC '(' ParameterList ')' Block
  {

  }
  | FUNC '(' ')' ReturnList Block
  {

  }
  | FUNC '(' ')' Block
  {

  }
  | FUNC error
  {
    Syntax_Error_Clear(&@2, "invalid anonymous function");
    $$ = NULL;
  }
  ;

UnaryExpression
  : PrimaryExpression
  {
    $$ = $1;
  }
  | UnaryOp UnaryExpression
  {
    $$ = Expr_From_Unary($1, $2);
  }
  ;

UnaryOp
  : '+'
  {
    $$ = UNARY_PLUS;
  }
  | '-'
  {
    $$ = UNARY_MINUS;
  }
  | '~'
  {
    $$ = UNARY_BIT_NOT;
  }
  | NOT
  {
    $$ = UNARY_LNOT;
  }
  ;

MultipleExpression
  : UnaryExpression
  {
    $$ = $1;
  }
  | MultipleExpression '*' UnaryExpression
  {
    $$ = Expr_From_Binary(BINARY_MULT, $1, $3);
  }
  | MultipleExpression '/' UnaryExpression
  {
    $$ = Expr_From_Binary(BINARY_DIV, $1, $3);
  }
  | MultipleExpression '%' UnaryExpression
  {
    $$ = Expr_From_Binary(BINARY_MOD, $1, $3);
  }
  | MultipleExpression POWER UnaryExpression
  {

  }
  ;

AddExpression
  : MultipleExpression
  {
    $$ = $1;
  }
  | AddExpression '+' MultipleExpression
  {
    $$ = Expr_From_Binary(BINARY_ADD, $1, $3);
  }
  | AddExpression '-' MultipleExpression
  {
    $$ = Expr_From_Binary(BINARY_SUB, $1, $3);
  }
  ;

ShiftExpression
  : AddExpression
  {
    $$ = $1;
  }
  | ShiftExpression LSHIFT AddExpression
  {
    $$ = Expr_From_Binary(BINARY_LSHIFT, $1, $3);
  }
  | ShiftExpression RSHIFT AddExpression
  {
    $$ = Expr_From_Binary(BINARY_RSHIFT, $1, $3);
  }
  ;

RelationExpression
  : ShiftExpression
  {
    $$ = $1;
  }
  | RelationExpression '<' ShiftExpression
  {
    $$ = Expr_From_Binary(BINARY_LT, $1, $3);
  }
  | RelationExpression '>' ShiftExpression
  {
    $$ = Expr_From_Binary(BINARY_GT, $1, $3);
  }
  | RelationExpression LE  ShiftExpression
  {
    $$ = Expr_From_Binary(BINARY_LE, $1, $3);
  }
  | RelationExpression GE  ShiftExpression
  {
    $$ = Expr_From_Binary(BINARY_GE, $1, $3);
  }
  ;

EqualityExpression
  : RelationExpression
  {
    $$ = $1;
  }
  | EqualityExpression EQ RelationExpression
  {
    $$ = Expr_From_Binary(BINARY_EQ, $1, $3);
  }
  | EqualityExpression NE RelationExpression
  {
    $$ = Expr_From_Binary(BINARY_NEQ, $1, $3);
  }
  ;

AndExpression
  : EqualityExpression
  {
    $$ = $1;
  }
  | AndExpression '&' EqualityExpression
  {
    $$ = Expr_From_Binary(BINARY_BIT_AND, $1, $3);
  }
  ;

ExclusiveOrExpression
  : AndExpression
  {
    $$ = $1;
  }
  | ExclusiveOrExpression '^' AndExpression
  {
    $$ = Expr_From_Binary(BINARY_BIT_XOR, $1, $3);
  }
  ;

InclusiveOrExpression
  : ExclusiveOrExpression
  {
    $$ = $1;
  }
  | InclusiveOrExpression '|' ExclusiveOrExpression
  {
    $$ = Expr_From_Binary(BINARY_BIT_OR, $1, $3);
  }
  ;

LogicAndExpression
  : InclusiveOrExpression
  {
    $$ = $1;
  }
  | LogicAndExpression AND InclusiveOrExpression
  {
    $$ = Expr_From_Binary(BINARY_LAND, $1, $3);
  }
  ;

LogicOrExpression
  : LogicAndExpression
  {
    $$ = $1;
  }
  | LogicOrExpression OR LogicAndExpression
  {
    $$ = Expr_From_Binary(BINARY_LOR, $1, $3);
  }
  ;

Expression
  : LogicOrExpression
  {
    $$ = $1;
  }
  ;

ExpressionList
  : Expression
  {
    $$ = Vector_New();
    Vector_Append($$, $1);
  }
  | ExpressionList ',' Expression
  {
    $$ = $1;
    Vector_Append($$, $3);
  }
  ;

Assignment
  : ExpressionList '=' ExpressionList ';'
  {
    $$ = Parser_Do_Assignments(ps, $1, $3);
  }
  | PrimaryExpression CompAssignOp Expression ';'
  {
    $$ = Stmt_From_Assign($2, $1, $3);
  }
  ;

CompAssignOp
  : PLUS_ASSGIN
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
  | RSHIFT_ASSIGN
  {
    $$ = OP_RSHIFT_ASSIGN;
  }
  | LSHIFT_ASSIGN
  {
    $$ = OP_LSHIFT_ASSIGN;
  }
  ;

%%
