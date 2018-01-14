
%{
#include "common.h"
#include "koala_yacc.h"

static void echo();

%}

%option nounput
%option noinput

D   [0-9]
H   [a-fA-F0-9]
L   [a-zA-Z_]

%%

[\+\-\*\/\%&\|=><,.;{}\[\]\(\)^~:] {echo();return yytext[0];}

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
"struct"                  {return STRUCT;}
"interface"               {return INTERFACE;}
"const"                   {return CONST;}
"import"                  {return IMPORT;}
"as"                      {return AS;}
"go"                      {return GO;}
"defer"                   {return DEFER;}

"char"                    {return CHAR;}
"byte"                    {return BYTE;}
"short"                   {return SHORT;}
"int"                     {return INTEGER;}
"float"                   {return FLOAT;}
"bool"                    {return BOOL;}
"string"                  {return STRING;}
"Any"                     {return ANY;}
"[]"+                     {
                            yylval.dims = yyleng/2;
                            return DIMS;
                          }

"self"                    {return SELF;}
"true"                    {return TOKEN_TRUE;}
"false"                   {return TOKEN_FALSE;}
"null"                    {return TOKEN_NULL;}

([1-9][0-9]*|[0])         {
                            echo();
                            yylval.ival = atoi(yytext);
                            return INT_CONST;
                          }
0[xX]{H}+                 {return HEX_CONST;}
0[0-7]*                   {return OCT_CONST;}
[0-9]+[.][0-9]+           {
                            echo();
                            yylval.fval = atof(yytext);
                            return FLOAT_CONST;
                          }

(\"(\\.|[^"\\])*\")       {
                            char *tmp = malloc(yyleng-1);
                            strncpy(tmp, yytext+1, yyleng-2);
                            tmp[yyleng - 2] = 0;
                            echo();
                            yylval.string_const = tmp;
                            return STRING_CONST;
                          }

{L}({L}|{D})*             {
                            char *tmp = malloc(yyleng+1);
                            strncpy(tmp, yytext, yyleng+1);
                            echo();
                            yylval.id = tmp;
                            return ID;
                          }
[\n]                      {}
[\t ]+                    {}
"/*"(([^\*]*(\*[^\/])?)*)"*/"       {echo();}
"#"[^\n]*                           {echo();}
"//"[^\n]*                          {echo();}


.                         {}

%%

static void echo() {
  //fprintf(stdout, ">>>%s\n", yytext);
}

int yywrap() {
  return 1;
}
