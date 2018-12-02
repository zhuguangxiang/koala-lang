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
#include "koala_tokens.h"

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
    ')', ']', '}',
    CHAR, BYTE, INTEGER, FLOAT, STRING, BOOL, ANY
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
/* %option header-file="koala_lex.h" */
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
"&&"                      {RETURN(AND);}
"||"                      {RETURN(OR);}
"!"                       {RETURN(NOT);}
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
"**"                      {RETURN(POWER);}
"..<"                     {RETURN(DOTDOTLESS);}

"if"                      {RETURN(IF);}
"else"                    {RETURN(ELSE);}
"while"                   {RETURN(WHILE);}
"do"                      {RETURN(DO);}
"for"                     {RETURN(FOR);}
"switch"                  {RETURN(SWITCH);}
"case"                    {RETURN(CASE);}
"fallthrough"             {RETURN(FALLTHROUGH);}
"break"                   {RETURN(BREAK);}
"continue"                {RETURN(CONTINUE);}
"default"                 {RETURN(DEFAULT);}
"var"                     {RETURN(VAR);}
"func"                    {RETURN(FUNC);}
"return"                  {RETURN(TOKEN_RETURN);}
"class"                   {RETURN(CLASS);}
"trait"                   {RETURN(TRAIT);}
"extends"                 {RETURN(EXTENDS);}
"with"                    {RETURN(WITH);}
"const"                   {RETURN(CONST);}
"package"                 {RETURN(PACKAGE);}
"import"                  {RETURN(IMPORT);}
"as"                      {RETURN(AS);}
"go"                      {RETURN(GO);}
"defer"                   {RETURN(DEFER);}
"typealias"               {RETURN(TYPEALIAS);}
"syscall"                 {RETURN(SYSCALL);}

"char"                    {RETURN(CHAR);}
"byte"                    {RETURN(BYTE);}
"int"                     {RETURN(INTEGER);}
"float"                   {RETURN(FLOAT);}
"bool"                    {RETURN(BOOL);}
"string"                  {RETURN(STRING);}
"any"                     {RETURN(ANY);}

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

(\'.\') {
  printf("character>>>%s\n", yytext);
}

[\t\r ]+                  {}
"/*"(([^\*]*(\*[^\/])?)*)"*/"       {}
"#"[^\n]*                           {}
"//"[^\n]*                          {}


.                         {}

%%
