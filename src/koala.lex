/*
MIT License

Copyright (c) 2018 James, https://github.com/zhuguangxiang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/* prologue */
%{

#include <stdio.h>
#include "parser.h"
#include "koala_yacc.h"

int interactive(struct parserstate *ps, char *buf, int size);
int file_input(struct parserstate *ps, char *buf, int size, FILE *in);

#define YY_EXTRA_TYPE struct parserstate *
#define yyps ((struct parserstate *)yyextra)

static int need_semicolon(YY_EXTRA_TYPE ps)
{
  static int tokens[] = {
    BYTE_LITERAL, INT_LITERAL, FLOAT_LITERAL, CHAR_LITERAL, STRING_LITERAL,
    ID, SELF, SUPER, TRUE, FALSE, NIL,
    RETURN, BREAK, CONTINUE, ')', ']', '}',
    BYTE, INTEGER, FLOAT, CHAR, STRING, BOOL, ANY,
    DOTDOTDOT, DOTDOTLESS, FAT_ARROW,
  };

  int token = ps->token;
  for (int i = 0; i < sizeof(tokens)/sizeof(tokens[0]); i++) {
    if (tokens[i] == token)
      return 1;
  }
  return 0;
}

#define YY_USER_ACTION                    \
{                                         \
  if (!yyps->interactive) {               \
    yyps->pos.col += yyps->len;           \
    yyps->len = strlen(yytext);           \
    yylloc->first_column = yyps->pos.col; \
    yylloc->first_line = yylineno;        \
  }                                       \
}

#define YY_RETURN(_token) \
  yyps->token = _token;   \
  return _token;

#define YY_INPUT(buf, result, size)        \
  if (yyps->interactive)                   \
    result = interactive(yyps, buf, size); \
  else                                     \
    result = file_input(yyps, buf, size, yyin);

#define YY_NEWLINE            \
  if (need_semicolon(yyps)) { \
    YY_RETURN(';');           \
  }

%}

%option bison-bridge
%option reentrant
%option bison-locations
%option nounput
%option noinput
%option noyywrap
%option never-interactive

digit [0-9]
hex   [a-fA-F0-9]
oct   [0-7]
bin   0|1
exp   [eE][+-]?{digit}+
i     {digit}+

float_const [+-]?({i}\.{i}?|{i}?\.{i}|{i})+{exp}?
int_const   [+-]?{i}
hex_const   0[xX]{hex}+
oct_const   0{oct}*
bin_const   0b{bin}+

asc [\x00-\t\v-\x21\x23-\x26\x28-\x5b\x5d-\x7f]
u   [\x80-\xbf]
u2  [\xc0-\xdf]
u3  [\xe0-\xef]
u4  [\xf0-\xf7]
asc_char  {asc}
utf8_char {u2}{u}|{u3}{u}{u}|{u4}{u}{u}{u}
esc_char  \\[\xa]|\\\\|\\\'|\\\"|\\a|\\b|\\f|\\n|\\r|\\t|\\v
hex_char  \\x({hex}|{hex}{hex})
u16_char  \\u{hex}{hex}{hex}{hex}
u32_char  \\U{hex}{hex}{hex}{hex}{hex}{hex}{hex}{hex}
character {asc_char}|{utf8_char}|{esc_char}|{hex_char}|{u16_char}|{u32_char}
string    {character}+

alpha      [A-Za-z_]
identifier {alpha}({alpha}|{digit})*

comment ("//"[^\n]*)|(#[^\n]*)
doc     "///"[^\n]*
moddoc  "//!"[^\n]*

/* lex scan rules */
%%

"=="       { YY_RETURN(OP_EQ);        }
"!="       { YY_RETURN(OP_NE);        }
">="       { YY_RETURN(OP_GE);        }
"<="       { YY_RETURN(OP_LE);        }
"&&"       { YY_RETURN(OP_AND);       }
"||"       { YY_RETURN(OP_OR);        }
"!"        { YY_RETURN(OP_NOT);       }
"and"      { YY_RETURN(OP_AND);       }
"or"       { YY_RETURN(OP_OR);        }
"not"      { YY_RETURN(OP_NOT);       }
":="       { YY_RETURN(FREE_ASSIGN);  }
"+="       { YY_RETURN(PLUS_ASSIGN);  }
"-="       { YY_RETURN(MINUS_ASSIGN); }
"*="       { YY_RETURN(MULT_ASSIGN);  }
"/="       { YY_RETURN(DIV_ASSIGN);   }
"%="       { YY_RETURN(MOD_ASSIGN);   }
"&="       { YY_RETURN(AND_ASSIGN);   }
"|="       { YY_RETURN(OR_ASSIGN);    }
"^="       { YY_RETURN(XOR_ASSIGN);   }
"..."      { YY_RETURN(DOTDOTDOT);    }
"..<"      { YY_RETURN(DOTDOTLESS);   }
"=>"       { YY_RETURN(FAT_ARROW);    }

"import"   { YY_RETURN(IMPORT);       }
"const"    { YY_RETURN(CONST);        }
"var"      { YY_RETURN(VAR);          }
"func"     { YY_RETURN(FUNC);         }
"class"    { YY_RETURN(CLASS);        }
"trait"    { YY_RETURN(TRAIT);        }
"enum"     { YY_RETURN(ENUM);         }
"if"       { YY_RETURN(IF);           }
"else"     { YY_RETURN(ELSE);         }
"while"    { YY_RETURN(WHILE);        }
"for"      { YY_RETURN(FOR);          }
"match"    { YY_RETURN(MATCH);        }
"break"    { YY_RETURN(BREAK);        }
"continue" { YY_RETURN(CONTINUE);     }
"return"   { YY_RETURN(RETURN);       }
"in"       { YY_RETURN(IN);           }
"as"       { YY_RETURN(AS);           }

"byte"     { YY_RETURN(BYTE);         }
"int"      { YY_RETURN(INTEGER);      }
"float"    { YY_RETURN(FLOAT);        }
"char"     { YY_RETURN(CHAR);         }
"string"   { YY_RETURN(STRING);       }
"bool"     { YY_RETURN(BOOL);         }
"any"      { YY_RETURN(ANY);          }

"self"     { YY_RETURN(SELF);         }
"super"    { YY_RETURN(SUPER);        }
"true"     { YY_RETURN(TRUE);         }
"false"    { YY_RETURN(FALSE);        }
"nil"      { YY_RETURN(NIL);          }

{bin_const} {
  yylval->ival = strtoll(yytext+2, NULL, 2);
  YY_RETURN(INT_LITERAL);
}

{oct_const} {
  yylval->ival = strtoll(yytext+1, NULL, 8);
  YY_RETURN(INT_LITERAL);
}

{hex_const} {
  yylval->ival = strtoll(yytext+2, NULL, 16);
  YY_RETURN(INT_LITERAL);
}

{int_const} {
  yylval->ival = strtoll(yytext, NULL, 10);
  YY_RETURN(INT_LITERAL);
}

{float_const} {
  yylval->fval = strtod(yytext, NULL);
  YY_RETURN(FLOAT_LITERAL);
}

(\'{asc_char}\') {
  printf("ascii:%s\n", yytext);
  return 0;
}

(\'{utf8_char}\') {
  printf("utf8:%s\n", yytext);
  return 0;
}

(\'{esc_char}\') {
  printf("escape:%s\n", yytext);
  return 0;
}

(\'{hex_char}\') {
  printf("hex:%s\n", yytext);
  return 0;
}

(\'{u16_char}\') {
  printf("unicode-16:%s\n", yytext);
  return 0;
}

(\'{u32_char}\') {
  printf("unicode-32:%s\n", yytext);
  return 0;
}

(\"{string}\") {
  YY_RETURN(STRING_LITERAL);
}

{identifier} {
  //strncpy(yylval->sval, yytext, yyleng);
  YY_RETURN(ID);
}

{doc} {
  return DOC;
}

{moddoc} {
  return MODDOC;
}

{comment} {
  return COMMENT;
}

[\n]+ {
  /* new line */
  YY_NEWLINE
}

[\+\-\*\/\%\|\=\&\>\<\,\;\^\~\:\.\{\}\[\]\(\)] {
  /* single character */
  YY_RETURN(yytext[0]);
}

[ \t\r]+ {
  /* blank character */
}

. {
  printf("error input\n");
}

%%
