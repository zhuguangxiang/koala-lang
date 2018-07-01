
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
	yyerrok; Parser_Handle_Error(parser, ERRMSG, expected, Token)
#define syntax_error_clear(msg) yyclearin; yyerrok; \
	Parser_Handle_Error(parser, "%s", msg)

/*---------------------------------------------------------------------------*/

static Vector *new_vardecl_stmts(ParserState *ps, Vector *vars,
	TypeDesc *desc, Vector *exprs, int bconst)
{
	int vsz = Vector_Size(vars);
	int esz = Vector_Size(exprs);
	if (esz != 0 && esz != vsz) {
		Parser_Handle_Error(ps, "cannot assign %d values to %d variables",
			esz, vsz);
		free_vars(vars);
		free_exprs(exprs);
		Type_DecRef(desc);
		return NULL;
	}

	Vector *ret = Vector_New();
	struct var *var;
	struct stmt *stmt;
	struct expr *exp;
	for (int i = 0; i < vsz; i++) {
		var = Vector_Get(vars, i);
		var->bconst = bconst;
		var->desc = Type_IncRef(desc);

		stmt = stmt_new(VARDECL_KIND);
		stmt->vardecl.var = var;

		if (esz > 0) {
			exp = Vector_Get(exprs, i);
			stmt->vardecl.exp = exp;
			if (!var->desc && exp->desc) var->desc = Type_IncRef(exp->desc);
		}

		Vector_Append(ret, stmt);
	}

	Vector_Free(vars, NULL, NULL);
	Vector_Free(exprs, NULL, NULL);
	return ret;
}

static void free_vardecl_stmts(Vector *stmts)
{
	struct stmt *s;
	Vector_ForEach(s, stmts) {
		free_var(s->vardecl.var);
		free_expr(s->vardecl.exp);
		free_stmt(s);
	}
	Vector_Free(stmts, NULL, NULL);
}

/*---------------------------------------------------------------------------*/

%}

%union {
	char *id;
	int64 ival;
	float64 fval;
	char *string_const;
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
%type <operator> UnaryOperator
%type <operator> CompoundAssignOperator
%type <list> VariableList
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
%type <expr> ArrayObject
%type <expr> AnonyFuncObject
%type <expr> CONSTANT
%type <list> VariableDeclaration
%type <list> ConstDeclaration
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

%start CompileUnit

%parse-param {ParserState *parser}
%parse-param {void *scanner}
%define api.pure full
%lex-param {void *scanner}

%code provides {
int yylex(YYSTYPE *yylval_param, void *yyscanner);
}

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
	INTEGER { $$ = Type_New_Prime(PRIME_INT);    }
| FLOAT   { $$ = Type_New_Prime(PRIME_FLOAT);  }
| BOOL    { $$ = Type_New_Prime(PRIME_BOOL);   }
| STRING  { $$ = Type_New_Prime(PRIME_STRING); }
| ANY     { $$ = Type_New_Prime(PRIME_ANY);    }
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
			Parser_Error();
			$$ = NULL;
		} else {
			$$ = Type_New_UsrDef(fullpath, $3);
		}
	}
;

FunctionType:
	FUNC '(' TypeList ')' ReturnTypeList
	{
		$$ = Type_New_Proto($3, $5);
	}
| FUNC '(' TypeList ')'
	{
		$$ = Type_New_Proto($3, NULL);
	}
| FUNC '(' ')' ReturnTypeList
	{
		$$ = Type_New_Proto(NULL, $4);
	}
| FUNC '(' ')'
	{
		$$ = Type_New_Proto(NULL, NULL);
	}
| FUNC '(' TypeList ',' ELLIPSIS ')' ReturnTypeList
	{
		Vector_Append($3, Type_New_Prime(PRIME_VARG));
		$$ = Type_New_Proto($3, $7);
	}
| FUNC '(' TypeList ',' ELLIPSIS ')'
	{
		Vector_Append($3, Type_New_Prime(PRIME_VARG));
		$$ = Type_New_Proto($3, NULL);
	}
;

ReturnTypeList:
	Type
	{
		$$ = Vector_New();
		Vector_Append($$, $1);
	}
| '(' TypeList ')'
	{
		$$ = $2;
	}
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
| VariableDeclaration ';'   { Parser_New_Vars(parser, $1);  }
| ConstDeclaration ';'      { Parser_New_Vars(parser, $1);  }
| FunctionDeclaration       { parse_func(parser, $1);  }
| TypeDeclaration           { parse_type(parser, $1);  }
| TypeAliasDeclaration      { parse_alias(parser, $1); }
| VariableDeclaration error
	{
		free_vardecl_stmts(parser, $1);
		syntax_error(";");
	}
| ConstDeclaration error
	{
		free_vardecl_stmts(parser, $1);
		syntax_error(";");
	}
| error { syntax_error_clear("invalid statement"); }
;

/*---------------------------------------------------------------------------*/

ConstDeclaration:
	CONST VariableList '=' ExpressionList
	{
		$$ = new_vardecl_stmts(parser, $2, NULL, $4, 1);
	}
| CONST VariableList Type '=' ExpressionList
	{
		$$ = new_vardecl_stmts(parser, $2, $3, $5, 1);
	}
| CONST VariableList '=' error
	{
		syntax_error("right's expression-list");
		$$ = NULL;
	}
| CONST VariableList Type '=' error
	{
		syntax_error("right's expression-list");
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
		$$ = new_vardecl_stmts(parser, $2, $3, NULL, 0);
	}
| VAR VariableList '=' ExpressionList
	{
		$$ = new_vardecl_stmts(parser, $2, NULL, $4, 0);
	}
| VAR VariableList Type '=' ExpressionList
	{
		$$ = new_vardecl_stmts(parser, $2, $3, $5, 0);
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

FunctionDeclaration
	: FUNC ID '(' ParameterList ')' ReturnTypeList Block {
		// $$ = stmt_from_funcdecl($2, $3, $4, $5);
	}
	| FUNC ID '(' ParameterList ')' Block {

	}
	| FUNC ID '(' ')' ReturnTypeList Block {
		// $$ = stmt_from_funcdecl($2, $3, $4, $5);
	}
	| FUNC ID '(' ')' Block {

	}
	| FUNC ID '(' ParameterList ',' ID ELLIPSIS ')' ReturnTypeList Block {

	}
	| FUNC ID '(' ParameterList ',' ID ELLIPSIS ')' Block {

	}
	| FUNC error {
		syntax_error(parser, EXPECTED, "ID", Lexer_Token);
		$$ = NULL;
	}
	;

ParameterList
	: ID Type {
		$$ = Vector_New();
		//Vector_Append($$, new_var($1, $2));
	}
	| ParameterList ',' ID Type {
		//Vector_Append($$, new_var($3, $4));
		$$ = $1;
	}
	;

/*---------------------------------------------------------------------------*/

TypeAliasDeclaration
	: TYPEALIAS ID Type ';' {
		// $$ = stmt_from_typealias($2, $3);
	}
	| TYPEALIAS ID Type error {
		syntax_error(parser, EXPECTED, ";", Lexer_Token);
	}
	| TYPEALIAS ID error {
		syntax_error(parser, EXPECTED, "TYPE", Lexer_Token);
	}
	| TYPEALIAS error {
		syntax_error(parser, EXPECTED, "ID", Lexer_Token);
	}
	;

/*---------------------------------------------------------------------------*/

TypeDeclaration
	: CLASS ID ExtendsOrEmpty '{' MemberDeclarations '}' {
		$$ = $3;
		$$->class_info.id = $2;
		$$->class_info.body = $5;
	}
	| TRAIT ID WithesOrEmpty '{' TraitMemberDeclarations '}' {
		$$ = stmt_from_trait($2, $3, $5);
	}
	| CLASS ID ExtendsOrEmpty ';' {
		$$ = $3;
		$$->class_info.id = $2;
	}
	| TRAIT ID WithesOrEmpty ';' {
		$$ = stmt_from_trait($2, $3, NULL);
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
	| FUNC ID '(' TypeList ',' ELLIPSIS ')' ReturnTypeList ';' {

	}
	| FUNC ID '(' TypeList ',' ELLIPSIS ')' ';' {

	}
	;

/*---------------------------------------------------------------------------*/

Block
	: '{' LocalStatements '}' {
		$$ = $2;
	}
	| '{' '}' {}
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

LocalStatement:
 ';' {
		$$ = NULL;
	}
	| Expression ';' {
		$$ = stmt_from_expr($1);
	}
	| Expression error {
		syntax_error(parser, EXPECTED, ";", Lexer_Token);
		$$ = NULL;
	}
	| VariableDeclaration ';' {
		$$ = $1;
	}
	| Assignment ';' {
		$$ = $1;
	}
	| Assignment error {
		syntax_error(parser, EXPECTED, ";", Lexer_Token);
		$$ = NULL;
	}
	| IfStatement {
		$$ = $1;
	}
	| WhileStatement {
		$$ = $1;
	}
	| SwitchStatement {

	}
	| ForStatement {

	}
	| JumpStatement {
		$$ = $1;
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

/*---------------------------------------------------------------------------*/

GoStatement
	: GO PrimaryExpression '(' ExpressionList ')' ';' {
		$$ = stmt_from_go(expr_from_trailer(CALL_KIND, $4, $2));
	}
	| GO PrimaryExpression '(' ')' ';' {
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

PrimaryExpression
	: Atom {

	}
	| PrimaryExpression '.' ID {
		/* $$ = expr_from_trailer(ATTRIBUTE_KIND, $3, $1);
		Parser_SetLine(parser, $$); */
	}
	| PrimaryExpression '[' Expression ']' {
		/* $$ = expr_from_trailer(SUBSCRIPT_KIND, $3, $1);
		Parser_SetLine(parser, $$); */
	}
	| PrimaryExpression '(' ExpressionList ')' {
		/* $$ = expr_from_trailer(CALL_KIND, $3, $1);
		Parser_SetLine(parser, $$); */
	}
	| PrimaryExpression '(' ')' {
		/* $$ = expr_from_trailer(CALL_KIND, NULL, $1);
		Parser_SetLine(parser, $$); */
	}
	| PrimaryExpression '['  Expression ':' Expression ']' {

	}
	;

Atom
	: ID {
		/* $$ = $1;
		Parser_SetLine(parser, $$); */
	}
	| CONSTANT {

	}
	| SELF {
		/* $$ = expr_from_self(); */
	}
	| SUPER {
		/* $$ = expr_from_super(); */
	}
	| TYPEOF {
		/* $$ = expr_from_typeof(); */
	}
	| '(' Expression ')' {
		/* $$ = $2; */
	}
	| ArrayObject {
		//$$ = $1;
	}
	| DictObject {

	}
	| AnonyFuncObject {

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

/*---------------------------------------------------------------------------*/

ArrayObject:
	'[' ExpressionList ']' {
		/* $2->dims = $1; */
		/* $$ = expr_from_array($2, NULL, $4); */
	}
| '[' ']' {

	}
;

DictObject:
	'{'  Expression ':' Expression '}' {}
| '{' ':' '}' {}
;

AnonyFuncObject
	: FUNC '(' ParameterList ')' ReturnTypeList Block {
		// $$ = expr_from_anonymous_func($2, $3, $4);
	}
	| FUNC '(' ParameterList ')' Block {

	}
	| FUNC '(' ')' ReturnTypeList Block {

	}
	| FUNC '(' ')' Block {

	}
	| FUNC '(' ParameterList ',' ID ELLIPSIS ')' ReturnTypeList Block {

	}
	| FUNC '(' ParameterList ',' ID ELLIPSIS ')' Block {

	}
	| FUNC error {
		syntax_error_clearin(parser, "anonymous function needs no func-name");
		$$ = NULL;
	}
	;

/*---------------------------------------------------------------------------*/

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
		$$ = UNARY_PLUS;
	}
	| '-' {
		$$ = UNARY_MINUS;
	}
	| '~' {
		$$ = UNARY_BIT_NOT;
	}
	| NOT {
		$$ = UNARY_LNOT;
	}
	;

MultiplicativeExpression
	: UnaryExpression {
		$$ = $1;
	}
	| MultiplicativeExpression '*' UnaryExpression {
		$$ = expr_from_binary(BINARY_MULT, $1, $3);
	}
	| MultiplicativeExpression '/' UnaryExpression {
		$$ = expr_from_binary(BINARY_DIV, $1, $3);
	}
	| MultiplicativeExpression '%' UnaryExpression {
		$$ = expr_from_binary(BINARY_MOD, $1, $3);
	}
	;

AdditiveExpression
	: MultiplicativeExpression {
		$$ = $1;
	}
	| AdditiveExpression '+' MultiplicativeExpression {
		$$ = expr_from_binary(BINARY_ADD, $1, $3);
	}
	| AdditiveExpression '-' MultiplicativeExpression {
		$$ = expr_from_binary(BINARY_SUB, $1, $3);
	}
	;

ShiftExpression
	: AdditiveExpression {
		$$ = $1;
	}
	| ShiftExpression LSHIFT AdditiveExpression {
		$$ = expr_from_binary(BINARY_LSHIFT, $1, $3);
	}
	| ShiftExpression RSHIFT AdditiveExpression {
		$$ = expr_from_binary(BINARY_RSHIFT, $1, $3);
	}
	;

RelationalExpression
	: ShiftExpression {
		$$ = $1;
	}
	| RelationalExpression '<' ShiftExpression {
		$$ = expr_from_binary(BINARY_LT, $1, $3);
	}
	| RelationalExpression '>' ShiftExpression {
		$$ = expr_from_binary(BINARY_GT, $1, $3);
	}
	| RelationalExpression LE  ShiftExpression {
		$$ = expr_from_binary(BINARY_LE, $1, $3);
	}
	| RelationalExpression GE  ShiftExpression {
		$$ = expr_from_binary(BINARY_GE, $1, $3);
	}
	;

EqualityExpression
	: RelationalExpression {
		$$ = $1;
	}
	| EqualityExpression EQ RelationalExpression {
		$$ = expr_from_binary(BINARY_EQ, $1, $3);
	}
	| EqualityExpression NE RelationalExpression {
		$$ = expr_from_binary(BINARY_NEQ, $1, $3);
	}
	;

AndExpression
	: EqualityExpression {
		$$ = $1;
	}
	| AndExpression '&' EqualityExpression {
		$$ = expr_from_binary(BINARY_BIT_AND, $1, $3);
	}
	;

ExclusiveOrExpression
	: AndExpression {
		$$ = $1;
	}
	| ExclusiveOrExpression '^' AndExpression {
		$$ = expr_from_binary(BINARY_BIT_XOR, $1, $3);
	}
	;

InclusiveOrExpression
	: ExclusiveOrExpression {
		$$ = $1;
	}
	| InclusiveOrExpression '|' ExclusiveOrExpression {
		$$ = expr_from_binary(BINARY_BIT_OR, $1, $3);
	}
	;

LogicalAndExpression
	: InclusiveOrExpression {
		$$ = $1;
	}
	| LogicalAndExpression AND InclusiveOrExpression {
		$$ = expr_from_binary(BINARY_LAND, $1, $3);
	}
	;

LogicalOrExpression
	: LogicalAndExpression {
		$$ = $1;
	}
	| LogicalOrExpression OR LogicalAndExpression {
		$$ = expr_from_binary(BINARY_LOR, $1, $3);
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
		//$$ = stmt_from_assignlist($1, OP_ASSIGN, $3);
	}
	| PrimaryExpression CompoundAssignOperator Expression {
		//$$ = stmt_from_compound_assign($1, $2, $3);
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

%%
