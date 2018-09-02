
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

#define RETURN(token) \
  ((ParserState *)yyextra)->lastToken = token; return token;

int Lexer_NeedInsertSemiColon(ParserState *ps)
{
  static int tokens[] = {
    ID, INT_CONST, FLOAT_CONST, STRING_CONST,
    BREAK, CONTINUE, FALLTHROUGH, TOKEN_RETURN,
    ')', ']', '}'
  };
  int lastToken = ps->lastToken;
  for ( int i = 0; i < nr_elts(tokens); i++) {
    if (tokens[i] == lastToken) return 1;
  }
  return 0;
}

#define NEWLINE() if (Lexer_NeedInsertSemiColon(yyextra)) { RETURN(';'); }

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

[\+\-\*\/\%&\|=><,.;\{\}\[\]\(\)^~:] {RETURN(yytext[0]);}

"=="                      {RETURN(EQ);}
"!="                      {RETURN(NE);}
">="                      {RETURN(GE);}
"<="                      {RETURN(LE);}
"and"                     {RETURN(AND);}
"or"                      {RETURN(OR);}
"not"                     {RETURN(NOT);}
">>"                      {RETURN(RSHIFT);}
"<<"                      {RETURN(LSHIFT);}
":="                      {RETURN(TYPELESS_ASSIGN);}
"+="                      {RETURN(PLUS_ASSGIN);}
"-="                      {RETURN(MINUS_ASSIGN);}
"*="                      {RETURN(MULT_ASSIGN);}
"/="                      {RETURN(DIV_ASSIGN);}
"%="                      {RETURN(MOD_ASSIGN);}
"&="                      {RETURN(AND_ASSIGN);}
"|="                      {RETURN(OR_ASSIGN);}
"^="                      {RETURN(XOR_ASSIGN);}
">>="                     {RETURN(RSHIFT_ASSIGN);}
"<<="                     {RETURN(LSHIFT_ASSIGN);}
"..."                     {RETURN(ELLIPSIS);}

"if"                      {RETURN(IF);}
"else"                    {RETURN(ELSE);}
"while"                   {RETURN(WHILE);}
"do"                      {RETURN(DO);}
"for"                     {RETURN(FOR);}
"in"                      {RETURN(IN);}
"switch"                  {RETURN(SWITCH);}
"case"                    {RETURN(CASE);}
"fallthrough"             {RETURN(FALLTHROUGH);}
"break"                   {RETURN(BREAK);}
"continue"                {RETURN(CONTINUE);}
"default"                 {RETURN(DEFAULT);}
"var"                     {RETURN(VAR);}
"func"                    {RETURN(FUNC);}
"->"                      {RETURN(RIGHT_ARROW);}
"return"                  {RETURN(TOKEN_RETURN);}
"class"                   {RETURN(CLASS);}
"trait"                   {RETURN(TRAIT);}
"extends"                 {RETURN(EXTENDS);}
"with"                    {RETURN(WITH);}
"const"                   {RETURN(CONST);}
"import"                  {RETURN(IMPORT);}
"as"                      {RETURN(AS);}
"go"                      {RETURN(GO);}
"defer"                   {RETURN(DEFER);}
"typealias"               {RETURN(TYPEALIAS);}
"char"                    {RETURN(CHAR);}
"byte"                    {RETURN(BYTE);}
"int"                     {RETURN(INTEGER);}
"float"                   {RETURN(FLOAT);}
"bool"                    {RETURN(BOOL);}
"string"                  {RETURN(STRING);}
"any"                     {RETURN(ANY);}
"map"                     {RETURN(MAP);}

"self"                    {RETURN(SELF);}
"super"                   {RETURN(SUPER);}
"true"                    {RETURN(TOKEN_TRUE);}
"false"                   {RETURN(TOKEN_FALSE);}
"nil"                     {RETURN(TOKEN_NIL);}

([1-9][0-9]*|[0])         {
                            yylval->ival = atoi(yytext);
                            RETURN(INT_CONST);
                          }
0[xX]{H}+                 {RETURN(HEX_CONST);}
0[0-7]*                   {RETURN(OCT_CONST);}
[0-9]+[.][0-9]+           {
                            yylval->fval = atof(yytext);
                            RETURN(FLOAT_CONST);
                          }

(\"(([^\"]|\\\")*[^\\])?\") {
  char *tmp = malloc(yyleng-1);
  strncpy(tmp, yytext+1, yyleng-2);
  tmp[yyleng - 2] = 0;
  yylval->string_const = tmp;
  RETURN(STRING_CONST);
}

{L}({L}|{D})*             {
                            assert(yyleng < 64);
                            char *tmp = malloc(yyleng+1);
                            strncpy(tmp, yytext, yyleng+1);
                            yylval->id = tmp;
                            RETURN(ID);
                          }

[\n]                      {
                            NEWLINE();
                          }

[\t\r ]+                  {}
"/*"(([^\*]*(\*[^\/])?)*)"*/"       {}
"#"[^\n]*                           {}
"//"[^\n]*                          {}


.                         {}

%%
