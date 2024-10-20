/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2024 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/* prologue */

%{

#include "parser.h"
#include "koala_yacc.h"
#include "atom.h"
#include "buffer.h"

#define YY_EXTRA_TYPE ParserState *

#define yyps ((YY_EXTRA_TYPE)yyextra)

/* auto insert semi-colon check */
static int semicolon(ParserState *ps)
{
    static int tokens[] = {
        INT_LITERAL, FLOAT_LITERAL, STRING_LITERAL, NONE, ID, SELF, SUPER,
        TRUE, FALSE, INT, FLOAT, STRING, BOOL, OBJECT,
        RETURN, BREAK, CONTINUE, ')', ']', '}', '?',
        '_', NOT,
    };

    int token = ps->token;
    for (int i = 0; i < COUNT_OF(tokens); ++i) {
        if (tokens[i] == token) return 1;
    }
    return 0;
}

static int file_input(ParserState *ps, char *buf, int size, FILE *in)
{
    char *s = fgets(buf, size, in);
    if (!s) return 0;

    int result = strlen(s);
    // RESET_BUF(ps->linebuf[ps->index]);
    // buf_write_nstr(&ps->linebuf[ps->index % 64], buf, result - 1);
    // ps->index = (ps->index + 1) % 64;
    return result;
}

#define YY_INPUT(buf, ret, size) ret = file_input(yyps, buf, size, yyin)

#define loc_first_line yylloc->first_line
#define loc_last_line  yylloc->last_line
#define loc_first_col  yylloc->first_column
#define loc_last_col   yylloc->last_column

/* clang-format off */

#define YY_NEWLINE do {    \
    yyps->newline = 1;     \
    if (semicolon(yyps)) { \
        YY_RETURN(';');    \
    }                      \
} while (0)

#define YY_USER_ACTION {                  \
    if (yyps->newline) {                  \
        ++loc_last_line;                  \
        loc_first_line = loc_last_line;   \
        loc_first_col = 1;                \
        loc_last_col = 1;                 \
        yyps->newline = 0;                \
    }                                     \
    char ch = yytext[0];                  \
    if (ch != '\r' && ch != '\n') {       \
        if (yyps->multi) {                \
            loc_last_col += yyleng;       \
        } else {                          \
            loc_first_col = loc_last_col; \
            loc_last_col += yyleng;       \
        }                                 \
    } else {                              \
        if (yyps->multi) {                \
            ++loc_last_line;              \
            loc_last_col = 1;             \
        }                                 \
    }                                     \
}

#define YY_RETURN(x) yyps->token = x; return x
#define YY_IDENT_VALUE(s, len) yylval->sval = atom_nstr(s, len)
#define YY_INT_VALUE(s, len, base) \
    errno = 0; yyps->sval = atom_nstr(s, len); \
    yylval->ival = strtoll(s, NULL, base)
#define YY_FLOAT_VALUE(s, len) \
    errno = 0; yyps->sval = atom_nstr(s, len); \
    yylval->fval = strtold(s, NULL)

#define YY_S_BEGIN yyps->multi = 1; BEGIN(S);
#define YY_S_WRITE(ch) buf_write_char(&yyps->sbuf, ch)
#define YY_S_END yyps->multi = 0; BEGIN(INITIAL); YY_RETURN(STRING_LITERAL)

#define YY_STR_BEGIN yyps->multi = 1; BEGIN(STR);
#define YY_STR_WRITE(ch) buf_write_char(&yyps->sbuf, ch)
#define YY_STR_END yyps->multi = 0; BEGIN(INITIAL); YY_RETURN(STRING_LITERAL)

#define YY_TXT_BEGIN yyps->multi = 1; BEGIN(TXT);
#define YY_TXT_WRITE(ch) buf_write_char(&yyps->sbuf, ch)
#define YY_TXT_END yyps->multi = 0; BEGIN(INITIAL); YY_RETURN(STRING_LITERAL)

%}

%option bison-bridge
%option reentrant
%option bison-locations
%option nounput
%option noinput
%option never-interactive

%x S STR TXT

/* rules */

%%

"from"      { YY_RETURN(FROM);      }
"import"    { YY_RETURN(IMPORT);    }
"let"       { YY_RETURN(LET);       }
"var"       { YY_RETURN(VAR);       }
"func"      { YY_RETURN(FUNC);      }
"class"     { YY_RETURN(CLASS);     }
"trait"     { YY_RETURN(TRAIT);     }
"if"        { YY_RETURN(IF);        }
"guard"     { YY_RETURN(GUARD);     }
"else"      { YY_RETURN(ELSE);      }
"while"     { YY_RETURN(WHILE);     }
"for"       { YY_RETURN(FOR);       }
"switch"    { YY_RETURN(MATCH);     }
"case"      { YY_RETURN(CASE);      }
"break"     { YY_RETURN(BREAK);     }
"continue"  { YY_RETURN(CONTINUE);  }
"return"    { YY_RETURN(RETURN);    }
"in"        { YY_RETURN(IN);        }
"as"        { YY_RETURN(AS);        }
"is"        { YY_RETURN(IS);        }
"public"    { YY_RETURN(PUBLIC);    }
"final"     { YY_RETURN(FINAL);     }
"static"    { YY_RETURN(STATIC);    }

"self"      { YY_RETURN(SELF);      }
"super"     { YY_RETURN(SUPER);     }
"true"      { YY_RETURN(TRUE);      }
"false"     { YY_RETURN(FALSE);     }
"none"      { YY_RETURN(NONE);      }

"int"       { YY_RETURN(INT);       }
"float"     { YY_RETURN(FLOAT);     }
"bool"      { YY_RETURN(BOOL);      }
"str"       { YY_RETURN(STRING);    }
"object"    { YY_RETURN(OBJECT);    }
"list"      { YY_RETURN(ARRAY);     }
"dict"      { YY_RETURN(MAP);       }
"set"       { YY_RETURN(SET);       }
"bytes"     { YY_RETURN(BYTES);     }
"tuple"     { YY_RETURN(TUPLE);     }
"type"      { YY_RETURN(TYPE);      }
"range"     { YY_RETURN(RANGE);     }

"&&"        { YY_RETURN(AND);       }
"||"        { YY_RETURN(OR);        }
"!"         { YY_RETURN(NOT);       }

"=="        { YY_RETURN(EQ);        }
"!="        { YY_RETURN(NE);        }
">="        { YY_RETURN(GE);        }
"<="        { YY_RETURN(LE);        }

":="        { YY_RETURN(FREE_ASSIGN);  }
"+="        { YY_RETURN(PLUS_ASSIGN);  }
"-="        { YY_RETURN(MINUS_ASSIGN); }
"*="        { YY_RETURN(MULT_ASSIGN);  }
"/="        { YY_RETURN(DIV_ASSIGN);   }
"%="        { YY_RETURN(MOD_ASSIGN);   }
"&="        { YY_RETURN(AND_ASSIGN);   }
"|="        { YY_RETURN(OR_ASSIGN);    }
"^="        { YY_RETURN(XOR_ASSIGN);   }
"<<="       { YY_RETURN(SHL_ASSIGN);   }
">>="       { YY_RETURN(SHR_ASSIGN);   }
">>>="      { YY_RETURN(USHR_ASSIGN);  }
"..."       { YY_RETURN(DOTDOTDOT);    }
"<<"        { YY_RETURN(L_SHIFT);      }
">>"        { YY_RETURN(R_SHIFT);      }
">>>"       { YY_RETURN(R_USHIFT);     }

"?"         { YY_RETURN('?'); }
"@"         { YY_RETURN('@'); }
"+"         { YY_RETURN('+'); }
"-"         { YY_RETURN('-'); }
"*"         { YY_RETURN('*'); }
"/"         { YY_RETURN('/'); }
"%"         { YY_RETURN('%'); }
"&"         { YY_RETURN('&'); }
"|"         { YY_RETURN('|'); }
"~"         { YY_RETURN('~'); }
"^"         { YY_RETURN('^'); }

":"         { YY_RETURN(':'); }
","         { YY_RETURN(','); }
";"         { YY_RETURN(';'); }
"."         { YY_RETURN('.'); }
"="         { YY_RETURN('='); }
"_"         { YY_RETURN('_'); }

"("         { YY_RETURN('('); }
")"         { YY_RETURN(')'); }
"["         { YY_RETURN('['); }
"]"         { YY_RETURN(']'); }
"{"         { YY_RETURN('{'); }
"}"         { YY_RETURN('}'); }

">"         { YY_RETURN('>'); }
"<"         { YY_RETURN('<'); }

0[xX][0-9a-fA-F_]+  { YY_INT_VALUE(yytext, yyleng, 16); YY_RETURN(INT_LITERAL); }
0[dD][0-9_]+        { YY_INT_VALUE(yytext, yyleng, 10); YY_RETURN(INT_LITERAL); }
0[oO][0-7_]+        { YY_INT_VALUE(yytext, yyleng, 8);  YY_RETURN(INT_LITERAL); }
0[bB][01_]+         { YY_INT_VALUE(yytext, yyleng, 2);  YY_RETURN(INT_LITERAL); }
[0-9][0-9_]*        { YY_INT_VALUE(yytext, yyleng, 10); YY_RETURN(INT_LITERAL); }

[A-Za-z_$][A-Za-z0-9_$]* {
    YY_IDENT_VALUE(yytext, yyleng); YY_RETURN(ID);
}

([0-9]+\.[0-9]+|[0-9]+)([eE][-+]?[0-9]+)? {
    YY_FLOAT_VALUE(yytext, yyleng); YY_RETURN(FLOAT_LITERAL);
}

\'            { printf("s begin\n");YY_S_BEGIN;             }
<S>\'         { printf("s end\n");YY_S_END;               }
<S>\\\a       { YY_S_WRITE('\a');       }
<S>\\\b       { YY_S_WRITE('\b');       }
<S>\\\f       { YY_S_WRITE('\f');       }
<S>\t         { printf("s: \\t\n"); YY_S_WRITE('\t');       }
<S>\\\v       { YY_S_WRITE('\v');       }
<S>\\\n       { printf("s-newlin\n"); YY_S_WRITE('\n');       }
<S>\\\r       { /* omit */              }
<S>\n         { printf("skip \\n\n"); /* omit */              }
<S>\r         { /* omit */              }
<S>\\\'       { printf("s: \\'\n"); YY_S_WRITE('\'');       }
<S>.          { YY_S_WRITE(yytext[0]);  }

\"              { printf("str begin\n");YY_STR_BEGIN;             }
<STR>\"         { printf("str end\n");YY_STR_END;               }
<STR>\\\a       { YY_STR_WRITE('\a');       }
<STR>\\\b       { YY_STR_WRITE('\b');       }
<STR>\\\f       { YY_STR_WRITE('\f');       }
<STR>\\\t       { YY_STR_WRITE('\t');       }
<STR>\\\v       { YY_STR_WRITE('\v');       }
<STR>\\\n       { printf("str-newline\n");  YY_STR_WRITE('\n');       }
<STR>\\\r       { /* omit */                }
<STR>\n         { /* omit */                }
<STR>\r         { /* omit */                }
<STR>\\\"       { YY_STR_WRITE('\"');       }
<STR>\\\\       { printf("str\\?\n"); YY_STR_WRITE('\\');       }
<STR>.          { YY_STR_WRITE(yytext[0]);  }

\"\"\"          { YY_TXT_BEGIN;             }
<TXT>\"\"\"     { YY_TXT_END;               }
<TXT>\n         { YY_TXT_WRITE('\n');       }
<TXT>\r         { /* omit */                }
<TXT>.          { YY_TXT_WRITE(yytext[0]);  }

"//!"[^\n]*     { YY_RETURN(MOD_DOC);       }
"/**"(([^\*]*(\*[^\/])?)*)"*/" {
    RESET_BUF(yyps->sbuf);
    buf_write_nstr(&yyps->sbuf, yytext + 3, yyleng - 5);
    YY_RETURN(DOC);
}

"\n"            { YY_NEWLINE;               }
[ \t\r]         { }
"#"[^\n]*       { }
"//"[^\n]*      { }
"/*"(([^\*]*(\*[^\/])?)*)"*/" { }
.               { }

%%

/* epilogue */

/*
When the scanner receives an end-of-file indication from YY_INPUT, it then
checks the yywrap() function.

If yywrap() returns false (zero), then it is assumed that the function has gone
ahead and set up yyin to point to another input file, and scanning continues.

If it returns true (non-zero), then the scanner terminates.
*/
int yywrap(yyscan_t yyscanner)
{
    return -1;
}
