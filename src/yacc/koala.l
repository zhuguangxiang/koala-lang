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
#include "log.h"

LOGGER(0)

#define YY_EXTRA_TYPE ParserState *

#define yyps ((ParserState *)yyextra)

#define YY_USER_ACTION                       \
({                                           \
  Lexer_DoUserAction(yyps, yytext);          \
  yylloc->first_line = yyps->line.pos.row;   \
  yylloc->last_line = yyps->line.pos.row;    \
  yylloc->first_column = yyps->line.pos.col; \
  yylloc->last_column = yylloc->first_column \
    + yyps->line.len;                        \
});

#define YY_INPUT(buf, result, max_size)                \
({                                                     \
  result = Lexer_DoYYInput(yyps, buf, max_size, yyin); \
  if (result <= 0)                                     \
    result = YY_NULL;                                  \
})

#define YY_RETURN(token) \
  yyps->lastToken = token; \
  return token;

static int Lexer_NeedInsertSemiColon(ParserState *ps)
{
  static int tokens[] = {
    ID, CHAR_LITERAL, INT_LITERAL, FLOAT_LITERAL, STRING_LITERAL,
    SELF, SUPER, TOKEN_NIL, TOKEN_TRUE, TOKEN_FALSE,
    BREAK, CONTINUE, FALLTHROUGH, TOKEN_RETURN,
    ')', ']', '}',
    CHAR, BYTE, INTEGER, FLOAT, BOOL, STRING, ANY, DOTDOTDOT,
  };
  int lastToken = ps->lastToken;
  for ( int i = 0; i < nr_elts(tokens); i++) {
    if (tokens[i] == lastToken) return 1;
  }
  return 0;
}

#define YY_NEWLINE()                     \
({                                       \
  if (Lexer_NeedInsertSemiColon(yyps)) { \
    YY_RETURN(';');                      \
  }                                      \
})

%}

%option bison-bridge
%option reentrant
/* %option header-file="koala_lex.h" */
%option bison-locations
%option yylineno
%option nounput
%option noinput
%option noyywrap
%option nointeractive

D   [0-9]
H   [a-fA-F0-9]
L   [a-zA-Z_]

/*
  https://stackoverflow.com/questions/9611682/flexlexer-support-for-unicode
  The UANY means match any character, single-byte ASCII or multi-byte UTF-8.
  UANYN means like UANY but no not match the newline. This is useful for tokens
  that do not break across lines, like say a comment from # to the end of the
  line, containing international text. UONLY means match only a UTF-8 extended
  character, not an ASCII one. This is useful for writing a lex rule which
  needs to exclude certain specific ASCII characters (not just newline)
  but all extended characters are okay.
*/

ASC     [\x00-\x7f]
ASCN    [\x00-\t\v-\x7f]
U       [\x80-\xbf]
U2      [\xc2-\xdf]
U3      [\xe0-\xef]
U4      [\xf0-\xf4]

UANY    {ASC}|{U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}
UANYN   {ASCN}|{U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}
UONLY   {U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}

%%

[\+\-\*\/\%&\|=><,.;\{\}\[\]\(\)^~:!] {
  YY_RETURN(yytext[0]);
}

"=="             { YY_RETURN(OP_EQ);           }
"!="             { YY_RETURN(OP_NE);           }
">="             { YY_RETURN(OP_GE);           }
"<="             { YY_RETURN(OP_LE);           }
"and"            { YY_RETURN(OP_AND);          }
"or"             { YY_RETURN(OP_OR);           }
"not"            { YY_RETURN(OP_NOT);          }
":="             { YY_RETURN(TYPELESS_ASSIGN); }
"+="             { YY_RETURN(PLUS_ASSGIN);     }
"-="             { YY_RETURN(MINUS_ASSIGN);    }
"*="             { YY_RETURN(MULT_ASSIGN);     }
"/="             { YY_RETURN(DIV_ASSIGN);      }
"%="             { YY_RETURN(MOD_ASSIGN);      }
"&="             { YY_RETURN(AND_ASSIGN);      }
"|="             { YY_RETURN(OR_ASSIGN);       }
"^="             { YY_RETURN(XOR_ASSIGN);      }
"..."            { YY_RETURN(DOTDOTDOT);       }
"..<"            { YY_RETURN(DOTDOTLESS);      }
"=>"             { YY_RETURN(FAT_ARROW);      }

"package"        { YY_RETURN(PACKAGE);         }
"if"             { YY_RETURN(IF);              }
"else"           { YY_RETURN(ELSE);            }
"while"          { YY_RETURN(WHILE);           }
"for"            { YY_RETURN(FOR);             }
"match"          { YY_RETURN(MATCH);           }
"fallthrough"    { YY_RETURN(FALLTHROUGH);     }
"break"          { YY_RETURN(BREAK);           }
"continue"       { YY_RETURN(CONTINUE);        }
"default"        { YY_RETURN(DEFAULT);         }
"var"            { YY_RETURN(VAR);             }
"const"          { YY_RETURN(CONST);           }
"func"           { YY_RETURN(FUNC);            }
"return"         { YY_RETURN(TOKEN_RETURN);    }
"class"          { YY_RETURN(CLASS);           }
"trait"          { YY_RETURN(TRAIT);           }
"enum"           { YY_RETURN(ENUM);            }
"in"             { YY_RETURN(IN);              }
"import"         { YY_RETURN(IMPORT);          }
"as"             { YY_RETURN(AS);              }
"go"             { YY_RETURN(GO);              }
"defer"          { YY_RETURN(DEFER);           }
"native"         { YY_RETURN(NATIVE);          }

"char"           { YY_RETURN(CHAR);            }
"byte"           { YY_RETURN(BYTE);            }
"int"            { YY_RETURN(INTEGER);         }
"float"          { YY_RETURN(FLOAT);           }
"bool"           { YY_RETURN(BOOL);            }
"string"         { YY_RETURN(STRING);          }
"any"            { YY_RETURN(ANY);             }

"self"           { YY_RETURN(SELF);            }
"super"          { YY_RETURN(SUPER);           }
"true"           { YY_RETURN(TOKEN_TRUE);      }
"false"          { YY_RETURN(TOKEN_FALSE);     }
"nil"            { YY_RETURN(TOKEN_NIL);       }

([1-9][0-9]*|[0]) {
  yylval->IVal = atoi(yytext);
  YY_RETURN(INT_LITERAL);
}

0[xX]{H}+ {
  yylval->IVal = (int64)strtoul(yytext, NULL, 16);
  YY_RETURN(INT_LITERAL);
}

0[0-7]* {
  yylval->IVal = (int64)strtoul(yytext, NULL, 8);
  YY_RETURN(INT_LITERAL);
}

[0-9]+[.][0-9]+ {
  yylval->FVal = atof(yytext);
  YY_RETURN(FLOAT_LITERAL);
}

(\"(([^\"]|\\\")*[^\\])?\") {
  yylval->SVal = AtomString_New_NStr(yytext + 1, yyleng - 2);
  YY_RETURN(STRING_LITERAL);
}

{L}({L}|{D})* {
  yylval->SVal = AtomString_New_NStr(yytext, yyleng + 1);
  YY_RETURN(ID);
}

[\n] {
  YY_NEWLINE();
}

(\'{UANYN}\') {
  Log_Debug("\x1b[32mUnicode Character: \x1b[34m%s\x1b[0m", yytext);
  assert(yyleng >= 3 && yyleng <= 6);
  yylval->ucVal.val = 0;
  yylval->ucVal.data[0] = *(uint8 *)(yytext + 1);
  if (yyleng >= 4)
    yylval->ucVal.data[1] = *(uint8 *)(yytext + 2);
  if (yyleng >= 5)
    yylval->ucVal.data[2] = *(uint8 *)(yytext + 3);
  if (yyleng >= 6)
    yylval->ucVal.data[3] = *(uint8 *)(yytext + 4);
  YY_RETURN(CHAR_LITERAL);
}

[\t\r ]+ {}

"/*"(([^\*]*(\*[^\/])?)*)"*/" { }

"///"[^\n]* { printf("doc comment\n"); }

"//"[^\n]* { }

. {
  Position pos = {yyps->line.pos.row, yyps->line.pos.col};
  __Syntax_Error(yyps, &pos, "invalid characters");
}

%%
