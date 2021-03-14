/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

/* prologue */

%{

#include <stdio.h>
#include "readline.h"

#define YY_INPUT(buf, result, size) result = readline("> ", buf, size)
#define YY_USER_ACTION
#define YY_RETURN(x)

char buf[4096];
int next;

%}

%option reentrant
%option nounput
%option noinput
%option never-interactive

%x STRING TEXT

/* rules */

%%

[ \t\r]+        ;
[\n]            { BEGIN(INITIAL); }

\"              {   
                    BEGIN(STRING); 
                    next = 0; 
                }
<STRING>\"      { 
                    BEGIN(INITIAL); 
                    buf[next] = 0; 
                    next = 0; 
                    printf("%s\n", buf); 
                }
<STRING>\\\"    {
                    buf[next++] = yytext[0];
                    buf[next++] = yytext[1];
                }
<STRING>\\\\    {
                    buf[next++] = yytext[0];
                    buf[next++] = yytext[1];
                }
<STRING>\n      {
                    buf[next++] = yytext[0];
                }
<STRING>.       { 
                    buf[next++] = yytext[0]; 
                }

\"\"\"          {   
                    BEGIN(TEXT); 
                    next = 0; 
                }
<TEXT>\"\"\"    { 
                    BEGIN(INITIAL); 
                    buf[next] = 0; 
                    next = 0; 
                    printf("%s\n", buf); 
                }
<TEXT>\n        {
                    buf[next++] = yytext[0];
                }
<TEXT>.         { 
                    buf[next++] = yytext[0]; 
                }

"import"    { YY_RETURN(IMPORT);        }
"const"     { YY_RETURN(CONST);         }
"var"       { YY_RETURN(VAR);           }
"func"      { YY_RETURN(FUNC);          }
"class"     { YY_RETURN(CLASS);         }
"trait"     { YY_RETURN(TRAIT);         }
"enum"      { YY_RETURN(ENUM);          }
"type"      { YY_RETURN(TYPEALIAS);     }
"if"        { YY_RETURN(IF);            }
"else"      { YY_RETURN(ELSE);          }
"while"     { YY_RETURN(WHILE);         }
"for"       { YY_RETURN(FOR);           }
"match"     { YY_RETURN(MATCH);         }
"break"     { YY_RETURN(BREAK);         }
"continue"  { YY_RETURN(CONTINUE);      }
"return"    { YY_RETURN(RETURN);        }
"in"        { YY_RETURN(IN);            }
"by"        { YY_RETURN(BY);            }
"as"        { YY_RETURN(AS);            }
"is"        { YY_RETURN(IS);            }
"where"     { YY_RETURN(WHERE);         }

"int8"      { YY_RETURN(BYTE);          }
"int16"      { YY_RETURN(BYTE);          }
"int32"      { YY_RETURN(BYTE);          }
"int64"      { YY_RETURN(BYTE);          }
"int"       { YY_RETURN(INTEGER);       }
"float32"     { YY_RETURN(FLOAT);         }
"float64"     { YY_RETURN(FLOAT);         }
"float"     { YY_RETURN(FLOAT);         }
"bool"      { YY_RETURN(BOOL);          }
"char"      { YY_RETURN(CHAR);          }
"string"    { YY_RETURN(STRING);        }
"any"       { YY_RETURN(ANY);           }

"self"      { YY_RETURN(SELF);          }
"super"     { YY_RETURN(SUPER);         }
"true"      { YY_RETURN(TRUE);          }
"false"     { YY_RETURN(FALSE);         }
"nil"       { YY_RETURN(CONST_NULL);    }

"_" {
        puts("_");
    }

"and"       {  }
"or"        { }
"not"       {  }

[A-Za-z_][A-Za-z0-9_]*  {
                            printf("`%s` is ident? or type?\n", yytext);
                        }

0[xX][0-9a-fA-F_]+ {  }
0[dD][0-9_]+       {  }
0[oO][0-7_]+       {  }
0[bB][01_]+        {  }
[0-9][0-9_]*       { }

"=="        {  }
"!="        { }
">="        {}
"<="        {  }
"&&"        {  }
"||"        {  }
"**"        {  }
":="        {  }
"+="        { }
"-="        { }
"*="        { }
"/="        {  }
"**="       { }
"%="        {  }
"&="        {  }
"|="        {  }
"^="        {     }
"<<="       { }
">>="       {  }
"..."       {  }
"..<"       { }
"=>"        {    }
"~="        {     }

"<<" {
        puts("<<");
    }

"+" {
        puts("+");
    }

"-" {
        puts("-");
    }

"*" {
        puts("*");
    }

"/" {
        puts("/");
    }

"%" {
        puts("%");
    }

"|" {
        puts("|");
    }

"&" {
        puts("&");
    }

"^" {
        puts("^");
    }

"~" {
        puts("~");
    }

"!" {
        puts("!");
    }

":" {
        puts(":");
    }

"," {
        puts(",");
    }

"?" {
        puts("?");
    }

"." {
        puts(".");
    }

"=" {
        puts("=");
    }

";" {
        puts(";");
    }

"@" {
        puts("@");
    }

"(" {
        puts("(");
    }

")" {
        puts(")");
    }

"[" {
        puts("[");
    }

"]" {
        puts("]");
    }

"{" {
        puts("{");
    }

"}" {
        puts("}");
    }

"<" {
        puts("lt<");
    }

">"/">" {
        puts(">>");
    }

">" {
        puts("gt>");
    }

.           {
                printf("invalid\n");
                // return 0;
            }

%%

/* epilogue */

int yywrap(yyscan_t yyscanner)
{
    return 1;
}

int main(int argc, char *argv[])
{
    init_line();
    yyscan_t scan;
    yylex_init(&scan);
    yylex(scan);
    fini_line();
    return 0;
}
