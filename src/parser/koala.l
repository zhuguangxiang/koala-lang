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
#include "parser.h"
#include "readline.h"
#include "koala_yacc.h"

#define YY_EXTRA_TYPE ParserState *
#define yyps ((YY_EXTRA_TYPE)yyextra)

#define YY_USER_ACTION {                    \
    char ch = *yytext;                      \
    if (ch != '\0' &&                       \
        ch != '\r' && ch != '\n') {         \
        yyps->col += yys->len;              \
        yyps->len = strlen(yytext);         \
        yylloc->first_column = yyps->col;   \
        yylloc->first_line = yyps->line;    \
    }                                       \
}

#define YY_RETURN(x) yyps->token = x; return x

#define YY_INPUT(buf, result, size) result = readline("> ", buf, size)

#define YY_NEWLINE              \
    ++yyps->line;               \
    yyps->col = 1;              \
    yyps->len = 0;              \
    if (need_semicolon(yyps)) { \
        YY_RETURN(';');         \
    }

#define YY_STRING_BEGIN yyps->next = 0

#define YY_STRING_END               \
    int ret = parse_string(yyps);   \
    yyps->next = 0;                 \
    if (ret) YY_RETURN(';');        \
    else YY_RETURN(STRING_LITERAL)

#define YY_STRING_APPEND(ch)        \
    yyps->buf[yyps->next++] = ch

%}

%option bison-locations
%option bison-bridge
%option reentrant
%option nounput
%option noinput
%option never-interactive

%x STRING TEXT

/* rules */

%%

[ \t\r\0]+      ;

[\n]            {
                    BEGIN(INITIAL);
                    YY_NEWLINE;
                }

\"              {
                    BEGIN(STRING);
                    YY_STRING_BEGIN;
                }
<STRING>\"      {
                    BEGIN(INITIAL);
                    YY_STRING_END;
                }
<STRING>\\\"    { YY_STRING_APPEND('\"');      }
<STRING>\\\\    { YY_STRING_APPEND('\\');      }
<STRING>\n      { YY_STRING_APPEND('\n');      }
<STRING>\r      { YY_STRING_APPEND('\r');      }
<STRING>\0      ;
<STRING>.       { YY_STRING_APPEND(yytext[0]); }

\"\"\"          { 
                    BEGIN(TEXT);
                    YY_STRING_BEGIN;
                }
<TEXT>\"\"\"    {
                    BEGIN(INITIAL);
                    YY_STRING_END;
                }
<TEXT>\n        { YY_STRING_APPEND('\n');      }
<TEXT>\r        { YY_STRING_APPEND('\r');      }
<TEXT>\0        ;
<TEXT>.         { YY_STRING_APPEND(yytext[0]); }

"import"    { YY_RETURN(IMPORT);   }
"const"     { YY_RETURN(CONST);    }
"var"       { YY_RETURN(VAR);      }
"func"      { YY_RETURN(FUNC);     }
"class"     { YY_RETURN(CLASS);    }
"trait"     { YY_RETURN(TRAIT);    }
"enum"      { YY_RETURN(ENUM);     }
"if"        { YY_RETURN(IF);       }
"else"      { YY_RETURN(ELSE);     }
"while"     { YY_RETURN(WHILE);    }
"for"       { YY_RETURN(FOR);      }
"match"     { YY_RETURN(MATCH);    }
"break"     { YY_RETURN(BREAK);    }
"continue"  { YY_RETURN(CONTINUE); }
"return"    { YY_RETURN(RETURN);   }
"in"        { YY_RETURN(IN);       }
"by"        { YY_RETURN(BY);       }
"as"        { YY_RETURN(AS);       }
"is"        { YY_RETURN(IS);       }

"int8"      { YY_RETURN(BYTE);     }
"int16"     { YY_RETURN(BYTE);     }
"int32"     { YY_RETURN(BYTE);     }
"int64"     { YY_RETURN(BYTE);     }
"int"       { YY_RETURN(INTEGER);  }
"float32"   { YY_RETURN(FLOAT);    }
"float64"   { YY_RETURN(FLOAT);    }
"float"     { YY_RETURN(FLOAT);    }
"bool"      { YY_RETURN(BOOL);     }
"char"      { YY_RETURN(CHAR);     }
"string"    { YY_RETURN(STRING);   }
"any"       { YY_RETURN(ANY);      }

"self"      { YY_RETURN(SELF);     }
"super"     { YY_RETURN(SUPER);    }
"true"      { YY_RETURN(TRUE);     }
"false"     { YY_RETURN(FALSE);    }
"nil"       { YY_RETURN(NIL);      }

"_"     {}
"and"   {  }
"or"    { }
"not"   {  }

[A-Za-z_][A-Za-z0-9_]*  {}

0[xX][0-9a-fA-F_]+  {  }
0[dD][0-9_]+        {  }
0[oO][0-7_]+        {  }
0[bB][01_]+         {  }
[0-9][0-9_]*        {  }

"=="    {  }
"!="    { }
">="    {}
"<="    {  }
"&&"    {  }
"||"    {  }
"**"    {  }
":="    {  }
"+="    { }
"-="    { }
"*="    { }
"/="    {  }
"**="   { }
"%="    {  }
"&="    {  }
"|="    {  }
"^="    {     }
"<<="   { }
">>="   {  }
"..."   {  }
"..<"   { }
"=>"    {    }
"~="    {     }

"<"     {
        YY_RETURN('<');
    }

">" {
        YY_RETURN('>');
    }

"+" {
        YY_RETURN('+');
    }

"-" {
        YY_RETURN('-');
    }

"*" {
        YY_RETURN('*');
    }

"/" {
        YY_RETURN('/');
    }

"%" {
        YY_RETURN('%');
    }

"|" {
        YY_RETURN('|');
    }

"&" {
        YY_RETURN('&');
    }

"^" {
        YY_RETURN('^');
    }

"~" {
        YY_RETURN('~');
    }

"!" {
        YY_RETURN('!');
    }

":" {
        YY_RETURN(':');
    }

"," {
        YY_RETURN(',');
    }

"?" {
        YY_RETURN('?');
    }

"." {
        YY_RETURN('.');
    }

";" {
        YY_RETURN(';');
    }

"=" {
        YY_RETURN('=');
    }

"@" {
        YY_RETURN('@');
    }

"(" {
        YY_RETURN('(');
    }

")" {
        YY_RETURN(')');
    }

"[" {
        YY_RETURN('[');
    }

"]" {
        YY_RETURN(']');
    }

"{" {
        YY_RETURN('{');
    }

"}" {
        YY_RETURN('}');
    }

"<<"    {
        YY_RETURN(L_SHIFT);
    }

">"/">" {
        YY_RETURN(R_ANGLE_SHIFT);
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
