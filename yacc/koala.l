
%{
#include "parser.h"
#include "koala_yacc.h"

#define YY_EXTRA_TYPE ParserState *

#define YY_USER_ACTION { \
	Lexer_DoUserAction(yyextra, yytext); \
}

#define YY_INPUT(buf, result, max_size) { \
	result = Lexer_DoYYInput(yyextra, buf, max_size, yyin); \
	if (result <= 0) result = YY_NULL; \
}

%}

%option bison-bridge
%option reentrant
%option header-file="koala_lex.h"
%option nounput
%option noinput
%option noyywrap
%option nointeractive


D   [0-9]
H   [a-fA-F0-9]
L   [a-zA-Z_]

%%

[\+\-\*\/\%&\|=><,.;{}\[\]\(\)^~:] {return yytext[0];}

"=="                      {return EQ;}
"!="                      {return NE;}
">="                      {return GE;}
"<="                      {return LE;}
"and"                     {return AND;}
"or"                      {return OR;}
"not"                     {return NOT;}
">>"                      {return RSHIFT;}
"<<"                      {return LSHIFT;}
":="                      {return TYPELESS_ASSIGN;}
"+="                      {return PLUS_ASSGIN;}
"-="                      {return MINUS_ASSIGN;}
"*="                      {return MULT_ASSIGN;}
"/="                      {return DIV_ASSIGN;}
"%="                      {return MOD_ASSIGN;}
"&="                      {return AND_ASSIGN;}
"|="                      {return OR_ASSIGN;}
"^="                      {return XOR_ASSIGN;}
">>="                     {return RSHIFT_ASSIGN;}
"<<="                     {return LSHIFT_ASSIGN;}
"..."                     {return ELLIPSIS;}
"->"                      {return RIGHT_ARROW;}

"package"                 {return PACKAGE;}
"if"                      {return IF;}
"else"                    {return ELSE;}
"while"                   {return WHILE;}
"do"                      {return DO;}
"for"                     {return FOR;}
"in"                      {return IN;}
"switch"                  {return SWITCH;}
"case"                    {return CASE;}
"fallthrough"             {return FALLTHROUGH;}
"break"                   {return BREAK;}
"continue"                {return CONTINUE;}
"default"                 {return DEFAULT;}
"var"                     {return VAR;}
"func"                    {return FUNC;}
"return"                  {return RETURN;}
"class"                   {return CLASS;}
"trait"                   {return TRAIT;}
"extends"                 {return EXTENDS;}
"with"                    {return WITH;}
"const"                   {return CONST;}
"import"                  {return IMPORT;}
"as"                      {return AS;}
"go"                      {return GO;}
"defer"                   {return DEFER;}
"type"                    {return TYPEALIAS;}

"char"                    {return CHAR;}
"byte"                    {return BYTE;}
"int"                     {return INTEGER;}
"float"                   {return FLOAT;}
"bool"                    {return BOOL;}
"string"                  {return STRING;}
"any"                     {return ANY;}
"map"                     {return MAP;}
"[]"+                     {
                            yylval->dims = yyleng/2;
                            return DIMS;
                          }

"self"                    {return SELF;}
"super"                   {return SUPER;}
"true"                    {return TOKEN_TRUE;}
"false"                   {return TOKEN_FALSE;}
"nil"                     {return TOKEN_NIL;}

([1-9][0-9]*|[0])         {
														yylval->ival = atoi(yytext);
														return INT_CONST;
													}
0[xX]{H}+                 {return HEX_CONST;}
0[0-7]*                   {return OCT_CONST;}
[0-9]+[.][0-9]+           {
														yylval->fval = atof(yytext);
														return FLOAT_CONST;
													}

(\"(\\.|[^"\\])*\")       {
														char *tmp = malloc(yyleng-1);
														strncpy(tmp, yytext+1, yyleng-2);
														tmp[yyleng - 2] = 0;
														yylval->string_const = tmp;
														return STRING_CONST;
													}

{L}({L}|{D})*             {
                            assert(yyleng < 64);
														char *tmp = malloc(yyleng+1);
														strncpy(tmp, yytext, yyleng+1);
														yylval->id = tmp;
														return ID;
													}
[\n]                      {}
[\t ]+                    {}
"/*"(([^\*]*(\*[^\/])?)*)"*/"       {}
"#"[^\n]*                           {}
"//"[^\n]*                          {}


.                         {}

%%
