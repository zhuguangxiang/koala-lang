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

#define YY_EXTRA_TYPE ParserState *

#define yyps ((ParserState *)yyextra)

#define YY_USER_ACTION \
do { \
  yylloc->first_line = yyps->line.row; \
  yylloc->last_line = yyps->line.row; \
  yylloc->first_column = yyps->line.col + 1; \
  yylloc->last_column = yylloc->first_column + strlen(yytext); \
  Lexer_DoUserAction(yyps, yytext); \
} while (0);

#define YY_INPUT(buf, result, max_size) \
do { \
  result = Lexer_DoYYInput(yyps, buf, max_size, yyin); \
  if (result <= 0) \
    result = YY_NULL; \
} while (0);

#define YY_RETURN(token) \
  yyps->lastToken = token; \
  return token;

static int Lexer_NeedInsertSemiColon(ParserState *ps)
{
  static int tokens[] = {
    ID, CHAR_LITERAL, INT_LITERAL, FLOAT_LITERAL, STRING_LITERAL, TOKEN_NIL,
    TOKEN_TRUE, TOKEN_FALSE,
    BREAK, CONTINUE, FALLTHROUGH, TOKEN_RETURN,
    ')', ']', '}',
    CHAR, BYTE, INTEGER, FLOAT, STRING, BOOL, ANY, ELLIPSIS
  };
  int lastToken = ps->lastToken;
  for ( int i = 0; i < nr_elts(tokens); i++) {
    if (tokens[i] == lastToken) return 1;
  }
  return 0;
}

#define YY_NEWLINE() \
do { \
  if (Lexer_NeedInsertSemiColon(yyps)) { \
    YY_RETURN(';'); \
  } \
} while (0);

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

%%

[\+\-\*\/\%&\|=><,.;\{\}\[\]\(\)^~:] {
  YY_RETURN(yytext[0]);
}

"=="             { YY_RETURN(EQ);              }
"!="             { YY_RETURN(NE);              }
">="             { YY_RETURN(GE);              }
"<="             { YY_RETURN(LE);              }
"and"            { YY_RETURN(AND);             }
"or"             { YY_RETURN(OR);              }
"not"            { YY_RETURN(NOT);             }
"&&"             { YY_RETURN(AND);             }
"||"             { YY_RETURN(OR);              }
"!"              { YY_RETURN(NOT);             }
">>"             { YY_RETURN(RSHIFT);          }
"<<"             { YY_RETURN(LSHIFT);          }
":="             { YY_RETURN(TYPELESS_ASSIGN); }
"+="             { YY_RETURN(PLUS_ASSGIN);     }
"-="             { YY_RETURN(MINUS_ASSIGN);    }
"*="             { YY_RETURN(MULT_ASSIGN);     }
"/="             { YY_RETURN(DIV_ASSIGN);      }
"%="             { YY_RETURN(MOD_ASSIGN);      }
"&="             { YY_RETURN(AND_ASSIGN);      }
"|="             { YY_RETURN(OR_ASSIGN);       }
"^="             { YY_RETURN(XOR_ASSIGN);      }
">>="            { YY_RETURN(RSHIFT_ASSIGN);   }
"<<="            { YY_RETURN(LSHIFT_ASSIGN);   }
"..."            { YY_RETURN(ELLIPSIS);        }
"**"             { YY_RETURN(POWER);           }
"..<"            { YY_RETURN(DOTDOTLESS);      }
">.."            { YY_RETURN(GREATERDOTDOT);   }

"if"             { YY_RETURN(IF);              }
"else"           { YY_RETURN(ELSE);            }
"while"          { YY_RETURN(WHILE);           }
"for"            { YY_RETURN(FOR);             }
"switch"         { YY_RETURN(SWITCH);          }
"case"           { YY_RETURN(CASE);            }
"fallthrough"    { YY_RETURN(FALLTHROUGH);     }
"break"          { YY_RETURN(BREAK);           }
"continue"       { YY_RETURN(CONTINUE);        }
"default"        { YY_RETURN(DEFAULT);         }
"var"            { YY_RETURN(VAR);             }
"func"           { YY_RETURN(FUNC);            }
"return"         { YY_RETURN(TOKEN_RETURN);    }
"class"          { YY_RETURN(CLASS);           }
"trait"          { YY_RETURN(TRAIT);           }
"extends"        { YY_RETURN(EXTENDS);         }
"with"           { YY_RETURN(WITH);            }
"const"          { YY_RETURN(CONST);           }
"package"        { YY_RETURN(PACKAGE);         }
"import"         { YY_RETURN(IMPORT);          }
"go"             { YY_RETURN(GO);              }
"defer"          { YY_RETURN(DEFER);           }
"typealias"      { YY_RETURN(TYPEALIAS);       }
"native"         { YY_RETURN(NATIVE);          }

"char"           { YY_RETURN(CHAR);            }
"byte"           { YY_RETURN(BYTE);            }
"int"            { YY_RETURN(INTEGER);         }
"float"          { YY_RETURN(FLOAT);           }
"bool"           { YY_RETURN(BOOL);            }
"string"         { YY_RETURN(STRING);          }
"error"          { YY_RETURN(ERROR);           }
"Any"            { YY_RETURN(ANY);             }

"self"           { YY_RETURN(SELF);            }
"super"          { YY_RETURN(SUPER);           }
"true"           { YY_RETURN(TOKEN_TRUE);      }
"false"          { YY_RETURN(TOKEN_FALSE);     }
"nil"            { YY_RETURN(TOKEN_NIL);       }

"[]"+ {
  yylval->Dims = yyleng/2;
  YY_RETURN(DIMS);
}

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
  assert(yyleng < 64);
  yylval->SVal = AtomString_New_NStr(yytext, yyleng + 1);
  YY_RETURN(ID);
}

[\n] {
  YY_NEWLINE();
}

(\'.+\') {
  printf("%s\n", yytext);
  YY_RETURN(CHAR_LITERAL);
}

[\t\r ]+ {}

"/*"(([^\*]*(\*[^\/])?)*)"*/" {}

"#"[^\n]* {}

"//"[^\n]* {}

. {}

%%
