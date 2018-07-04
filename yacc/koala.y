
%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

/*---------------------------------------------------------------------------*/

#if LOG_DEBUG
#define YYERROR_VERBOSE 1
static int yyerror(ParserState *parser, void *scanner, const char *errmsg)
{
	UNUSED_PARAMETER(parser);
	UNUSED_PARAMETER(scanner);
	fprintf(stderr, "%s\n", errmsg);
	return 0;
}
#else
#define yyerror(parser, scanner, errmsg) ((void)0)
#endif

#define ERRMSG "expected '%s' before '%s'"
#define syntax_error(expected) \
	yyerrok; Parser_Do_Error(parser, ERRMSG, expected, Token)
#define syntax_error_clear(msg) yyclearin; yyerrok; \
	Parser_Do_Error(parser, "%s", msg)

struct stmt *do_vardecl_list(ParserState *ps, Vector *vars, TypeDesc *desc,
	Vector *exprs, int bconst);

%}

%union {
	char *id;
	int64 ival;
	float64 fval;
	char *string_const;
	int dims;
	struct expr *expr;
	Vector *list;
	struct stmt *stmt;
	TypeDesc *type;
	int operator;
	struct field *field;
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
%token TRAIT
%token EXTENDS
%token WITH
%token CONST
%token IMPORT
%token AS
%token GO
%token DEFER
%token NEWLINE
%token TYPEALIAS

%token CHAR
%token BYTE
%token INTEGER
%token FLOAT
%token BOOL
%token STRING
%token ANY
%token <dims> DIMS
%token MAP

%token SELF
%token SUPER
%token TOKEN_NIL
%token TOKEN_TRUE
%token TOKEN_FALSE
%token TYPEOF

%token <ival> BYTE_CONST
%token <ival> CHAR_CONST
%token <ival> INT_CONST
%token <ival> HEX_CONST
%token <ival> OCT_CONST
%token <fval> FLOAT_CONST
%token <string_const> STRING_CONST
%token <id> ID

%type <type> PrimitiveType
%type <type> UsrDefType
%type <type> BaseType
%type <type> ArrayType
%type <type> Type
%type <type> FunctionType
%type <type> MapType
%type <list> ReturnTypeList
%type <list> TypeList
%type <list> ParameterList
%type <operator> UnaryOp
%type <operator> CompAssignOp
%type <list> VariableList
%type <list> ExpressionList
%type <list> PrimaryExpressionList
%type <expr> Expression
%type <expr> LogOrExpr
%type <expr> LogAndExpr
%type <expr> InclOrExpr
%type <expr> ExclOrExpr
%type <expr> AndExpr
%type <expr> EqualityExpr
%type <expr> RelationExpr
%type <expr> ShiftExpr
%type <expr> AddExpr
%type <expr> MultiplExpr
%type <expr> UnaryExpr
%type <expr> PrimaryExpr
%type <expr> ArrayObject
%type <expr> AnonyFuncObject
%type <expr> Atom
%type <expr> CONSTANT
%type <stmt> VariableDeclaration
%type <stmt> ConstDeclaration
%type <stmt> LocalStatement
%type <stmt> Assignment
%type <stmt> IfStatement
%type <stmt> OrElseStatement
%type <stmt> WhileStatement
%type <stmt> SwitchStatement
%type <list> CaseStatements
%type <testblock> CaseStatement
%type <stmt> ForStatement
%type <stmt> ForInit
%type <stmt> ForTest
%type <stmt> ForIncr
%type <list> Block
%type <stmt> GoStatement
%type <list> LocalStatements
%type <stmt> ReturnStatement
%type <stmt> JumpStatement
%type <stmt> FunctionDeclaration
%type <stmt> TypeDeclaration
%type <stmt> TypeAliasDeclaration
%type <list> MemberDeclarations
%type <stmt> MemberDeclaration
%type <stmt> FieldDeclaration
%type <stmt> ProtoDeclaration
%type <stmt> ExtendsOrEmpty
%type <list> WithesOrEmpty
%type <list> Traits
%type <list> TraitMemberDeclarations
%type <stmt> TraitMemberDeclaration

%parse-param {ParserState *parser}
%parse-param {void *scanner}
%define api.pure full
%lex-param {void *scanner}
%code provides { int yylex(YYSTYPE *yylval_param, void *yyscanner); }

%start CompileUnit

%%

/*---------------------------------------------------------------------------*/

Type:
	BaseType  { $$ = $1; }
| ArrayType { $$ = $1; }
;

ArrayType:
	DIMS BaseType { $$ = Type_New_Array($1, $2); }
;

BaseType:
	PrimitiveType { $$ = $1; }
| UsrDefType    { $$ = $1; }
| FunctionType  { $$ = $1; }
| MapType       { $$ = $1; }
;

MapType:
	'[' PrimitiveType ']' Type { Type_New_Map($2, $4); }
| '[' UsrDefType ']' Type    { Type_New_Map($2, $4); }
;

PrimitiveType:
	INTEGER { $$ = Type_IncRef(&Int_Type);    }
| FLOAT   { $$ = Type_IncRef(&Float_Type);  }
| BOOL    { $$ = Type_IncRef(&Bool_Type);   }
| STRING  { $$ = Type_IncRef(&String_Type); }
| ANY     { $$ = Type_IncRef(&Any_Type);    }
;

UsrDefType:
	ID
	{
		/* type in local module */
		$$ = Type_New_UsrDef(NULL, $1);
	}
| ID '.' ID
	{
		/* type in external module */
		char *fullpath = Parser_Get_FullPath(parser, $1);
		if (!fullpath) {
			Parser_Do_Error(parser, "cannot find '%s'", $1);
			$$ = NULL;
		} else {
			$$ = Type_New_UsrDef(fullpath, $3);
		}
	}
;

FunctionType:
	FUNC '(' TypeList ')' ReturnTypeList { $$ = Type_New_Proto($3, $5);     }
| FUNC '(' TypeList ')'                { $$ = Type_New_Proto($3, NULL);   }
| FUNC '(' ')' ReturnTypeList          { $$ = Type_New_Proto(NULL, $4);   }
| FUNC '(' ')'                         { $$ = Type_New_Proto(NULL, NULL); }
;

ReturnTypeList:
	Type
	{
		$$ = Vector_New();
		Vector_Append($$, $1);
	}
| '(' TypeList ')' { $$ = $2; }
;

TypeList:
	Type
	{
		$$ = Vector_New();
		Vector_Append($$, $1);
	}
| TypeList ',' Type
	{
		Vector_Append($1, $3);
		$$ = $1;
	}
| TypeList ',' ELLIPSIS
	{
		Vector_Append($$, Type_IncRef(&Varg_Type));
		$$ = $1;
	}
;

/*---------------------------------------------------------------------------*/

CompileUnit:
	Package Imports ModuleStatements
| Package ModuleStatements
| Imports ModuleStatements
| ModuleStatements
;

Package:
	PACKAGE ID ';'   { Parser_Set_Package(parser, $2);         }
| PACKAGE ID error { syntax_error(";"); exit(-1);            }
| PACKAGE error    { syntax_error("package-name"); exit(-1); }
;

Imports:
	Import
| Imports Import
;

Import:
	IMPORT STRING_CONST ';'      { Parser_New_Import(parser, NULL, $2); }
| IMPORT ID STRING_CONST ';'   { Parser_New_Import(parser, $2, $3);   }
| IMPORT STRING_CONST error    { syntax_error(";");                   }
| IMPORT ID STRING_CONST error { syntax_error(";");                   }
| IMPORT ID error              { syntax_error("path");                }
| IMPORT error                 { syntax_error("id or path");          }
;

ModuleStatements:
	ModuleStatement
| ModuleStatements ModuleStatement
;

ModuleStatement:
	';' {}
| VariableDeclaration ';'   { Parser_New_Vars(parser, $1);         }
| ConstDeclaration ';'      { Parser_New_Vars(parser, $1);         }
| FunctionDeclaration       { Parser_New_Func(parser, $1);         }
| TypeDeclaration           { Parser_New_ClassOrTrait(parser, $1); }
| TypeAliasDeclaration      { Parser_New_TypeAlias(parser, $1);    }
| VariableDeclaration error
	{
		stmt_free_vardecl_list($1);
		syntax_error(";");
	}
| ConstDeclaration error
	{
		stmt_free_vardecl_list($1);
		syntax_error(";");
	}
| error
	{
		syntax_error_clear("invalid statement");
	}
;

/*---------------------------------------------------------------------------*/

ConstDeclaration:
	CONST VariableList '=' ExpressionList
	{
		$$ = do_vardecl_list(parser, $2, NULL, $4, 1);
	}
| CONST VariableList Type '=' ExpressionList
	{
		$$ = do_vardecl_list(parser, $2, $3, $5, 1);
	}
| CONST VariableList '=' error
	{
		syntax_error("rexpr-list");
		$$ = NULL;
	}
| CONST VariableList Type '=' error
	{
		syntax_error("rexpr-list");
		$$ = NULL;
	}
| CONST error
	{
		syntax_error("id-list");
		$$ = NULL;
	}
;

VariableDeclaration:
	VAR VariableList Type
	{
		$$ = do_vardecl_list(parser, $2, $3, NULL, 0);
	}
| VAR VariableList '=' ExpressionList
	{
		$$ = do_vardecl_list(parser, $2, NULL, $4, 0);
	}
| VAR VariableList Type '=' ExpressionList
	{
		$$ = do_vardecl_list(parser, $2, $3, $5, 0);
	}
| VAR VariableList error
	{
		syntax_error("'TYPE' or '='");
		$$ = NULL;
	}
| VAR VariableList '=' error
	{
		syntax_error("right's expression-list");
		$$ = NULL;
	}
| VAR VariableList Type '=' error
	{
		syntax_error("right's expression-list");
		$$ = NULL;
	}
| VAR error
	{
		syntax_error("id-list");
		$$ = NULL;
	}
;

VariableList:
	ID
	{
		$$ = Vector_New();
		Vector_Append($$, new_var($1, NULL));
	}
| VariableList ',' ID
	{
		Vector_Append($1, new_var($3, NULL));
		$$ = $1;
	}
;

/*---------------------------------------------------------------------------*/

FunctionDeclaration:
	FUNC ID '(' ParameterList ')' ReturnTypeList Block
	{
		$$ = stmt_from_funcdecl($2, $4, $6, $7);
	}
| FUNC ID '(' ParameterList ')' Block
	{
		$$ = stmt_from_funcdecl($2, $4, NULL, $6);
	}
| FUNC ID '(' ')' ReturnTypeList Block
	{
		$$ = stmt_from_funcdecl($2, NULL, $5, $6);
	}
| FUNC ID '(' ')' Block
	{
		$$ = stmt_from_funcdecl($2, NULL, NULL, $5);
	}
| FUNC error
	{
		syntax_error("ID");
		$$ = NULL;
	}
;

ParameterList:
	ID Type
	{
		$$ = Vector_New();
		Vector_Append($$, new_var($1, $2));
	}
| ParameterList ',' ID Type
	{
		Vector_Append($$, new_var($3, $4));
		$$ = $1;
	}
| ParameterList ',' ID ELLIPSIS
	{
		Vector_Append($$, new_var($3, Type_IncRef(&Varg_Type)));
		$$ = $1;
	}
;

/*---------------------------------------------------------------------------*/

TypeAliasDeclaration
	: TYPEALIAS ID Type ';' {
		// $$ = stmt_from_typealias($2, $3);
	}
	| TYPEALIAS ID Type error {
		syntax_error(";");
	}
	| TYPEALIAS ID error {
		syntax_error("TYPE");
	}
	| TYPEALIAS error {
		syntax_error("ID");
	}
	;

/*---------------------------------------------------------------------------*/

TypeDeclaration
	: CLASS ID ExtendsOrEmpty '{' MemberDeclarations '}' {
		//$$ = $3;
		//$$->class_info.id = $2;
		//$$->class_info.body = $5;
	}
	| TRAIT ID WithesOrEmpty '{' TraitMemberDeclarations '}' {
		//$$ = stmt_from_trait($2, $3, $5);
	}
	| CLASS ID ExtendsOrEmpty ';' {
		//$$ = $3;
		//$$->class_info.id = $2;
	}
	| TRAIT ID WithesOrEmpty ';' {
		//$$ = stmt_from_trait($2, $3, NULL);
	}
	;

ExtendsOrEmpty
	: %empty {
		$$ = stmt_new(CLASS_KIND);
	}
	| EXTENDS UsrDefType WithesOrEmpty {
		$$ = stmt_new(CLASS_KIND);
		$$->class_info.super = $2;
		$$->class_info.traits = $3;
	}
	| Traits {
		$$ = stmt_new(CLASS_KIND);
		$$->class_info.traits = $1;
	}
	;

WithesOrEmpty
	: %empty {
		$$ = NULL;
	}
	| Traits {
		$$ = $1;
	}
	;

Traits
	: WITH UsrDefType {
		$$ = Vector_New();
		Vector_Append($$, $2);
	}
	| Traits WITH UsrDefType {
		Vector_Append($1, $3);
		$$ = $1;
	}
	;

MemberDeclarations
	: MemberDeclaration {
		$$ = Vector_New();
		Vector_Append($$, $1);
	}
	| MemberDeclarations MemberDeclaration {
		Vector_Append($1, $2);
		$$ = $1;
	}
	;

MemberDeclaration
	: FieldDeclaration {
		$$ = $1;
	}
	| FunctionDeclaration {
		$$ = $1;
	}
	;

TraitMemberDeclarations
	: TraitMemberDeclaration {
		$$ = Vector_New();
		Vector_Append($$, $1);
	}
	| TraitMemberDeclarations TraitMemberDeclaration {
		Vector_Append($1, $2);
		$$ = $1;
	}
	;

TraitMemberDeclaration
	: FieldDeclaration {
		$$ = $1;
	}
	| ProtoDeclaration {
		$$ = $1;
	}
	| FunctionDeclaration {
		$$ = $1;
	}
	;

FieldDeclaration
	: ID Type ';' {
		//$$ = stmt_from_vardecl(new_var($2, $3), NULL, 0);
	}
	| ID '=' Expression ';' {
		//$$ = stmt_from_vardecl(new_var($2, NULL), $4, 0);
	}
	| ID Type '=' Expression ';' {
		//$$ = stmt_from_vardecl(new_var($2, $3), $5, 0);
	}
	;

ProtoDeclaration
	: FUNC ID '(' TypeList ')' ReturnTypeList ';' {
		// $$ = stmt_from_funcproto($2, $4, $6);
	}
	| FUNC ID '(' TypeList ')' ';' {

	}
	| FUNC ID '(' ')' ReturnTypeList ';' {
		//$$ = stmt_from_funcproto($2, NULL, $5);
	}
	| FUNC ID '(' ')' ';' {

	}
	;

/*---------------------------------------------------------------------------*/

Block:
	'{' LocalStatements '}' { $$ = $2;   }
| '{' '}'                 { $$ = NULL; }
;

LocalStatements:
	LocalStatement
	{
		$$ = Vector_New();
		if ($1 != NULL) Vector_Append($$, $1);
	}
| LocalStatements LocalStatement
	{
		if ($2 != NULL) Vector_Append($1, $2);
		$$ = $1;
	}
;

LocalStatement:
 ';'                      { $$ = NULL;                }
| Expression ';'          { $$ = stmt_from_expr($1);  }
| VariableDeclaration ';' { $$ = $1;                  }
| Assignment ';'          { $$ = $1;                  }
| IfStatement             { $$ = $1;                  }
| WhileStatement          { $$ = $1;                  }
| SwitchStatement         {                           }
| ForStatement            {                           }
| JumpStatement           { $$ = $1;                  }
| ReturnStatement         { $$ = $1;                  }
| Block                   { $$ = stmt_from_block($1); }
| GoStatement             {                           }
| Expression error
	{
		//free
		syntax_error(";"); $$ = NULL;
	}
| VariableDeclaration error
	{
		//free
		syntax_error(";"); $$ = NULL;
	}
| Assignment error
	{
		//free
		syntax_error(";"); $$ = NULL;
	}
;

/*---------------------------------------------------------------------------*/

GoStatement
	: GO PrimaryExpr '(' ExpressionList ')' ';' {
		$$ = stmt_from_go(expr_from_trailer(CALL_KIND, $4, $2));
	}
	| GO PrimaryExpr '(' ')' ';' {
		$$ = stmt_from_go(expr_from_trailer(CALL_KIND, NULL, $2));
	}
	;

/*---------------------------------------------------------------------------*/

IfStatement
	: IF '(' Expression ')' Block OrElseStatement {
		$$ = stmt_from_if($3, $5, $6);
		$$->if_stmt.belse = 0;
	}
	;

OrElseStatement
	: %empty {
		$$ = NULL;
	}
	| ELSE Block {
		$$ = stmt_from_if(NULL, $2, NULL);
		$$->if_stmt.belse = 1;
	}
	| ELSE IfStatement {
		$$ = $2;
		$$->if_stmt.belse = 1;
	}
	;

/*---------------------------------------------------------------------------*/

WhileStatement
	: WHILE '(' Expression ')' Block {
		$$ = stmt_from_while($3, $5, 1);
	}
	| DO Block WHILE '(' Expression ')' {
		$$ = stmt_from_while($5, $2, 0);
	}
	;

/*---------------------------------------------------------------------------*/

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

/*---------------------------------------------------------------------------*/

ForStatement
	: FOR '(' ForInit ';' ForTest ';' ForIncr ')' Block {
		$$ = stmt_from_for($3, $5, $7, $9);
	}
	| FOR '(' ID IN Expression ')' Block {
		//$$ = stmt_from_foreach(new_var($3, NULL), $5, $7, 0);
	}
	| FOR '(' VAR ID IN Expression ')' Block {
		//$$ = stmt_from_foreach(new_var($4, NULL), $6, $8, 1);
	}
	| FOR '(' VAR VariableList Type IN Expression ')' Block {
		// if (Vector_Size($4) != 1) {
		// 	fprintf(stderr, "[ERROR]syntax error, foreach usage\n");
		// 	exit(0);
		// } else {
		// 	struct var *v = Vector_Get($4, 0);
		// 	assert(v);
		// 	v->desc = $5;
		// 	Vector_Free($4, NULL, NULL);
		// 	$$ = stmt_from_foreach(v, $7, $9, 1);
		// }
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

/*---------------------------------------------------------------------------*/

JumpStatement
	: BREAK ';' {
		$$ = stmt_from_jump(BREAK_KIND, 1);
	}
	| BREAK INT_CONST ';' {
		$$ = stmt_from_jump(BREAK_KIND, $2);
	}
	| CONTINUE ';' {
		$$ = stmt_from_jump(CONTINUE_KIND, 1);
	}
	| CONTINUE INT_CONST ';' {
		$$ = stmt_from_jump(CONTINUE_KIND, $2);
	}
	;

ReturnStatement
	: RETURN ';' {
		$$ = stmt_from_return(NULL);
	}
	| RETURN ExpressionList ';' {
		//$$ = stmt_from_return($2);
	}
	;

/*---------------------------------------------------------------------------*/

PrimaryExpr:
	Atom
	{
		$$ = $1;
	}
| PrimaryExpr '.' ID
	{
		/* $$ = expr_from_trailer(ATTRIBUTE_KIND, $3, $1);
		Parser_SetLine(parser, $$); */
	}
| PrimaryExpr '[' Expression ']'
	{
		/* $$ = expr_from_trailer(SUBSCRIPT_KIND, $3, $1);
		Parser_SetLine(parser, $$); */
	}
| PrimaryExpr '(' ExpressionList ')'
	{
		/* $$ = expr_from_trailer(CALL_KIND, $3, $1);
		Parser_SetLine(parser, $$); */
	}
| PrimaryExpr '(' ')'
	{
		/* $$ = expr_from_trailer(CALL_KIND, NULL, $1);
		Parser_SetLine(parser, $$); */
	}
| PrimaryExpr '['  Expression ':' Expression ']'
	{

	}
;

Atom:
	CONSTANT
	{
		$$ = $1;
		Parser_SetLine(parser, $$);
	}
| SELF
	{
		$$ = expr_from_self();
	}
| SUPER
	{
		/* $$ = expr_from_super(); */
	}
| TYPEOF
	{
		/* $$ = expr_from_typeof(); */
	}
| ID
	{
	}
| '(' Expression ')'
	{
		$$ = $2;
	}
| ArrayObject
	{
	}
| DictObject
	{
	}
| AnonyFuncObject
	{
	}
;

CONSTANT:
	INT_CONST    { $$ = expr_from_int($1);    }
| FLOAT_CONST  { $$ = expr_from_float($1);  }
| STRING_CONST { $$ = expr_from_string($1); }
| TOKEN_NIL    { $$ = expr_from_nil();      }
| TOKEN_TRUE   { $$ = expr_from_bool(1);    }
| TOKEN_FALSE  { $$ = expr_from_bool(0);    }
;

ArrayObject:
	'[' ExpressionList ']' {}
| '[' ']' {}
;

DictObject:
	'{'  Expression ':' Expression '}' {}
| '{' ':' '}' {}
;

AnonyFuncObject:
	FUNC '(' ParameterList ')' ReturnTypeList Block
	{
		// $$ = expr_from_anonymous_func($2, $3, $4);
	}
| FUNC '(' ParameterList ')' Block
	{

	}
| FUNC '(' ')' ReturnTypeList Block
	{

	}
| FUNC '(' ')' Block
	{

	}
| FUNC error
	{
		syntax_error_clear("invalid anonymous function");
		$$ = NULL;
	}
;

/*---------------------------------------------------------------------------*/

UnaryExpr:
	PrimaryExpr { $$ = $1; }
| UnaryOp UnaryExpr { $$ = expr_from_unary($1, $2); }
;

UnaryOp:
	'+' { $$ = UNARY_PLUS;    }
| '-' { $$ = UNARY_MINUS;   }
| '~' { $$ = UNARY_BIT_NOT; }
| NOT { $$ = UNARY_LNOT;    }
;

MultiplExpr:
	UnaryExpr { $$ = $1; }
| MultiplExpr '*' UnaryExpr { $$ = expr_from_binary(BINARY_MULT, $1, $3); }
| MultiplExpr '/' UnaryExpr { $$ = expr_from_binary(BINARY_DIV, $1, $3);  }
| MultiplExpr '%' UnaryExpr { $$ = expr_from_binary(BINARY_MOD, $1, $3);  }
;

AddExpr:
	MultiplExpr { $$ = $1; }
| AddExpr '+' MultiplExpr { $$ = expr_from_binary(BINARY_ADD, $1, $3); }
| AddExpr '-' MultiplExpr { $$ = expr_from_binary(BINARY_SUB, $1, $3); }
;

ShiftExpr:
	AddExpr { $$ = $1; }
| ShiftExpr LSHIFT AddExpr { $$ = expr_from_binary(BINARY_LSHIFT, $1, $3); }
| ShiftExpr RSHIFT AddExpr { $$ = expr_from_binary(BINARY_RSHIFT, $1, $3); }
;

RelationExpr:
	ShiftExpr { $$ = $1; }
| RelationExpr '<' ShiftExpr { $$ = expr_from_binary(BINARY_LT, $1, $3); }
| RelationExpr '>' ShiftExpr { $$ = expr_from_binary(BINARY_GT, $1, $3); }
| RelationExpr LE  ShiftExpr { $$ = expr_from_binary(BINARY_LE, $1, $3); }
| RelationExpr GE  ShiftExpr { $$ = expr_from_binary(BINARY_GE, $1, $3); }
;

EqualityExpr:
	RelationExpr                 { $$ = $1; }
| EqualityExpr EQ RelationExpr { $$ = expr_from_binary(BINARY_EQ, $1, $3);  }
| EqualityExpr NE RelationExpr { $$ = expr_from_binary(BINARY_NEQ, $1, $3); }
;

AndExpr:
	EqualityExpr { $$ = $1; }
| AndExpr '&' EqualityExpr { $$ = expr_from_binary(BINARY_BIT_AND, $1, $3); }
;

ExclOrExpr:
	AndExpr { $$ = $1; }
| ExclOrExpr '^' AndExpr { $$ = expr_from_binary(BINARY_BIT_XOR, $1, $3);}
;

InclOrExpr:
	ExclOrExpr { $$ = $1; }
| InclOrExpr '|' ExclOrExpr { $$ = expr_from_binary(BINARY_BIT_OR, $1, $3); }
;

LogAndExpr:
	InclOrExpr { $$ = $1; }
| LogAndExpr AND InclOrExpr { $$ = expr_from_binary(BINARY_LAND, $1, $3); }
;

LogOrExpr:
	LogAndExpr { $$ = $1; }
| LogOrExpr OR LogAndExpr { $$ = expr_from_binary(BINARY_LOR, $1, $3); }
;

/*---------------------------------------------------------------------------*/

Expression:
	LogOrExpr { $$ = $1; }
;

ExpressionList:
	Expression
	{
		$$ = Vector_New();
		Vector_Append($$, $1);
	}
| ExpressionList ',' Expression
	{
		Vector_Append($1, $3);
		$$ = $1;
	}
;

Assignment:
	PrimaryExpressionList '=' ExpressionList
	{
		//$$ = stmt_from_assignlist($1, OP_ASSIGN, $3);
	}
| PrimaryExpr CompAssignOp Expression
	{
		//$$ = stmt_from_compound_assign($1, $2, $3);
	}
;

PrimaryExpressionList:
	PrimaryExpr
	{
		$$ = Vector_New();
		Vector_Append($$, $1);
	}
| PrimaryExpressionList ',' PrimaryExpr
	{
		Vector_Append($1, $3);
		$$ = $1;
	}
;

/* 组合赋值运算符：算术运算和位运算 */
CompAssignOp:
	PLUS_ASSGIN   { $$ = OP_PLUS_ASSIGN;   }
| MINUS_ASSIGN  { $$ = OP_MINUS_ASSIGN;  }
| MULT_ASSIGN   { $$ = OP_MULT_ASSIGN;   }
| DIV_ASSIGN    { $$ = OP_DIV_ASSIGN;    }
| MOD_ASSIGN    { $$ = OP_MOD_ASSIGN;    }
| AND_ASSIGN    { $$ = OP_AND_ASSIGN;    }
| OR_ASSIGN     { $$ = OP_OR_ASSIGN;     }
| XOR_ASSIGN    { $$ = OP_XOR_ASSIGN;    }
| RSHIFT_ASSIGN { $$ = OP_RSHIFT_ASSIGN; }
| LSHIFT_ASSIGN { $$ = OP_LSHIFT_ASSIGN; }
;

%%

struct stmt *do_vardecl_list(ParserState *ps, Vector *vars, TypeDesc *desc,
	Vector *exprs, int bconst)
{
	int vsz = Vector_Size(vars);
	int esz = Vector_Size(exprs);
	if (esz != 0 && esz != vsz) {
		Parser_Do_Error(ps, "cannot assign %d values to %d variables", esz, vsz);
		//FIXME
		//free_vars(vars);
		//free_exprs(exprs);
		Type_DecRef(desc);
		return NULL;
	}

	struct stmt *stmt = stmt_from_vardecl_list(vars, desc, exprs, bconst);
	Vector_Free(vars, NULL, NULL);
	Vector_Free(exprs, NULL, NULL);
	return stmt;
}
