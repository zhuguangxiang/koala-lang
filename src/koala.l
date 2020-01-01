/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

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
#include "utf8.h"
#include "textblock.h"
#include "koala_yacc.h"

static int esc_char(char esc)
{
  static struct {
    char esc;
    char ch;
  } mappings[] = {
    {'n',  10},
    {'"',  34},
    {'\'', 39},
    {'\\', 92},
    {10,   10},
    {'r',  13},
    {'a',  7 },
    {'b',  8 },
    {'f',  12},
    {'t',  9 },
    {'v',  11},
  };

  for (int i = 0; i < COUNT_OF(mappings); ++i) {
    if (mappings[i].esc == esc)
      return mappings[i].ch;
  }
  panic("invalid escape character");
}

static int is_newline_return(char ch)
{
  return ch == '\n' || ch == '\r';
}

int interactive(ParserState *ps, char *buf, int size);
int file_input(ParserState *ps, char *buf, int size, FILE *in);

#define YY_EXTRA_TYPE ParserState *
#define yyps ((YY_EXTRA_TYPE)yyextra)

static int need_semicolon(YY_EXTRA_TYPE ps)
{
  static int tokens[] = {
    INT_LITERAL, FLOAT_LITERAL, CHAR_LITERAL, STRING_LITERAL,
    ID, SELF, SUPER, TRUE, FALSE, CONST_NULL,
    RETURN, BREAK, CONTINUE, ')', ']', '}', '>', '_',
    BYTE, INTEGER, FLOAT, CHAR, STRING, BOOL, ANY,
    DOTDOTDOT, DOTDOTLESS, FAT_ARROW,
  };

  int token = ps->token;
  for (int i = 0; i < COUNT_OF(tokens); ++i) {
    if (tokens[i] == token)
      return 1;
  }
  return 0;
}

#define YY_USER_ACTION                \
{                                     \
  char ch = *yytext;                  \
  if (ch != '\r' && ch != '\n') {     \
    yyps->col += yyps->len;           \
    yyps->len = strlen(yytext);       \
    yylloc->first_column = yyps->col; \
    yylloc->first_line = yyps->row;   \
  }                                   \
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
  ++yyps->row;                \
  yyps->len = 0;              \
  yyps->col = 1;              \
  if (need_semicolon(yyps)) { \
    YY_RETURN(';');           \
  }

%}

%option bison-bridge
%option reentrant
%option bison-locations
%option nounput
%option noinput
%option never-interactive

digit [0-9]
hex   [a-fA-F0-9]
oct   [0-7]
bin   0|1
exp   [eE][+-]?{digit}+
i     {digit}+

float_const ({i}\.{i}|{i}){exp}?
int_const   {i}
hex_const   0[xX]{hex}+
oct_const   0{oct}*
bin_const   0b{bin}+

asc [\x00-\x26\x28-\x5b\x5d-\x7f]
u   [\x80-\xbf]
u2  [\xc0-\xdf]
u3  [\xe0-\xef]
u4  [\xf0-\xf7]
asc_char  {asc}

utf8_char {u2}{u}|{u3}{u}{u}|{u4}{u}{u}{u}
esc_char  \\\\|\\\"|\\a|\\b|\\f|\\n|\\r|\\t|\\v
hex_char  \\x({hex}{hex})
u16_char  \\u{hex}{hex}{hex}{hex}
u32_char  \\U{hex}{hex}{hex}{hex}{hex}{hex}{hex}{hex}
unasc_char {utf8_char}|{esc_char}|{hex_char}|{u16_char}|{u32_char}
character2 {asc_char}|{unasc_char}

str_char  [\x00-\x21\x23-\x5b\x5d-\x7f]
character {str_char}|{unasc_char}
string    {character}+

alpha      [A-Za-z_]
identifier {alpha}({alpha}|{digit})*

comment   ("//"|#)[^\n]*
blockcomment  "/*"(([^\*]*(\*[^\/])?)*)"*/"

doc     "///"[^\n]*
moddoc  "//!"[^\n]*

/* lex scan rules */
%%

[\+\-\*\/\%\|\=\&\>\<\,\;\^\~\:\?\.\{\}\[\]\(\)\_] {
  /* single character */
  YY_RETURN(yytext[0]);
}

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
"**"       { YY_RETURN(OP_POWER);     }
":="       { YY_RETURN(FREE_ASSIGN);  }
"+="       { YY_RETURN(PLUS_ASSIGN);  }
"-="       { YY_RETURN(MINUS_ASSIGN); }
"*="       { YY_RETURN(MULT_ASSIGN);  }
"/="       { YY_RETURN(DIV_ASSIGN);   }
"**="      { YY_RETURN(POW_ASSIGN);   }
"%="       { YY_RETURN(MOD_ASSIGN);   }
"&="       { YY_RETURN(AND_ASSIGN);   }
"|="       { YY_RETURN(OR_ASSIGN);    }
"^="       { YY_RETURN(XOR_ASSIGN);   }
"..."      { YY_RETURN(DOTDOTDOT);    }
"..<"      { YY_RETURN(DOTDOTLESS);   }
"=>"       { YY_RETURN(FAT_ARROW);    }
"~="       { YY_RETURN(OP_MATCH);     }

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
"by"       { YY_RETURN(BY);           }
"as"       { YY_RETURN(AS);           }
"is"       { YY_RETURN(IS);           }
"extends"  { YY_RETURN(EXTENDS);      }
"with"     { YY_RETURN(WITH);         }

"byte"     { YY_RETURN(BYTE);         }
"int"      { YY_RETURN(INTEGER);      }
"float"    { YY_RETURN(FLOAT);        }
"char"     { YY_RETURN(CHAR);         }
"string"   { YY_RETURN(STRING);       }
"bool"     { YY_RETURN(BOOL);         }
"any"      { YY_RETURN(ANY);          }

"new"      { YY_RETURN(NEW);          }
"self"     { YY_RETURN(SELF);         }
"super"    { YY_RETURN(SUPER);        }
"true"     { YY_RETURN(TRUE);         }
"false"    { YY_RETURN(FALSE);        }
"null"     { YY_RETURN(CONST_NULL);   }

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
  char *s = yytext + 1;
  yylval->cval.val = *s;
  return YY_RETURN(CHAR_LITERAL);
}

(\'{utf8_char}\') {
  printf("utf8:%s\n", yytext);
  char *s = yytext + 1;
  yylval->cval.val = decode_one_utf8_char(&s);
  return YY_RETURN(CHAR_LITERAL);
}

(\'{esc_char}\') {
  char *s = yytext + 2;
  printf("escape:%c\n", *s);
  yylval->cval.val = esc_char(*s);
  return YY_RETURN(CHAR_LITERAL);
}

(\'\\\'\') {
  char *s = yytext + 2;
  printf("escape:%c\n", *s);
  yylval->cval.val = esc_char(*s);
  return YY_RETURN(CHAR_LITERAL);
}

(\'{hex_char}\') {
  char s[4];
  printf("hex:%s\n", yytext);
  char *nptr = yytext + 3;
  strncpy(s, nptr, strlen(nptr) - 1);
  yylval->cval.val = strtol(s, NULL, 16);
  return YY_RETURN(CHAR_LITERAL);
}

(\'{u16_char}\') {
  printf("unicode-16:%s\n", yytext);
  return 0;
}

(\'{u32_char}\') {
  print("unicode-32:%s\n", yytext);
  return 0;
}

(\'\') {
  synerr(yyps, yyps->row, yyps->col, "illegal character");
  if (yyps->interactive) {
    yyps->more = 0;
  }
}

(\'{character2}{character2}+\') {
  char *s = yytext;
  synerr(yyps, yyps->row, yyps->col, "illegal character:%s", yytext);
  if (yyps->interactive) {
    yyps->more = 0;
  }
}

(\"{string}?\") {
  yylval->sval = atom_nstring(yytext + 1, yyleng - 2);
  YY_RETURN(STRING_LITERAL);
}

(\"\"\") {
  int state = textblock_state();
  if (state == TB_END) {
    char *data = textblock_data();
    if (data != NULL) {
      char *s = str_trim(data);
      yylval->sval = atom(s);
      free(s);
      reset_textblock();
      YY_RETURN(STRING_LITERAL);
    }
  }
}

{identifier} {
  yylval->sval = atom_nstring(yytext, yyleng);
  YY_RETURN(ID);
}

{doc} {
  char *s = str_ntrim(yytext + 3, yyleng - 3);
  if (s != NULL) {
    yylval->text = s;
    return DOC;
  }
}

{moddoc} {
  char *s = str_ntrim(yytext + 3, yyleng - 3);
  if (s != NULL) {
    yylval->text = s;
    return DOC;
  }
}

{comment} {
  /* omit single line comment */
  return COMMENT;
}

{blockcomment} {
  /* omit multi lines comment */
  return COMMENT;
}

[\n] {
  /* new line */
  YY_NEWLINE
}

[ \t]+ {
  /* blank character */
}

[\r]+ {
  /* blank character */
}

. {
  yylval->sval = yytext;
  return INVALID;
}

%%
