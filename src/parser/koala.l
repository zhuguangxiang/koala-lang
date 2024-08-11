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
        INT_LITERAL, FLOAT_LITERAL, STRING_LITERAL, ID, SELF, SUPER,
        TRUE, FALSE, INT8, INT16, INT32, INT64, FLOAT32, FLOAT64,
        STRING, BOOL, OBJECT, RETURN, BREAK, CONTINUE, ')', ']', '}',
        '_', NOT, DOTDOTDOT, DOTDOTEQ,
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

%x STR TXT

/* rules */

%%

"import"    { YY_RETURN(IMPORT);    }
"let"       { YY_RETURN(LET);       }
"var"       { YY_RETURN(VAR);       }
"func"      { YY_RETURN(FUNC);      }
"class"     { YY_RETURN(CLASS);     }
"trait"     { YY_RETURN(TRAIT);     }
"enum"      { YY_RETURN(ENUM);      }
"if"        { YY_RETURN(IF);        }
"else"      { YY_RETURN(ELSE);      }
"while"     { YY_RETURN(WHILE);     }
"for"       { YY_RETURN(FOR);       }
"match"     { YY_RETURN(MATCH);     }
"case"      { YY_RETURN(CASE);      }
"break"     { YY_RETURN(BREAK);     }
"continue"  { YY_RETURN(CONTINUE);  }
"return"    { YY_RETURN(RETURN);    }
"in"        { YY_RETURN(IN);        }
"as"        { YY_RETURN(AS);        }
"is"        { YY_RETURN(IS);        }
"public"    { YY_RETURN(PUBLIC);    }
"final"     { YY_RETURN(FINAL);     }

"self"      { YY_RETURN(SELF);      }
"super"     { YY_RETURN(SUPER);     }
"True"      { YY_RETURN(TRUE);      }
"False"     { YY_RETURN(FALSE);     }
"None"      { YY_RETURN(NONE);      }

"Int8"      { YY_RETURN(INT8);      }
"Int16"     { YY_RETURN(INT16);     }
"Int32"     { YY_RETURN(INT32);     }
"Int64"     { YY_RETURN(INT64);     }
"Int"       { YY_RETURN(INT64);     }
"Float32"   { YY_RETURN(FLOAT32);   }
"Float64"   { YY_RETURN(FLOAT64);   }
"Float"     { YY_RETURN(FLOAT64);   }
"Bool"      { YY_RETURN(BOOL);      }
"String"    { YY_RETURN(STRING);    }
"Object"    { YY_RETURN(OBJECT);    }

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

"..."       { YY_RETURN(DOTDOTDOT); }
"..="       { YY_RETURN(DOTDOTEQ);}

"<<"        { YY_RETURN(L_SHIFT);   }
">>"        { YY_RETURN(R_SHIFT);   }
">>>"       { YY_RETURN(R_USHIFT);  }

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

\"              { YY_STR_BEGIN;   }
<STR>\"         { YY_STR_END; }
<STR>\\\"       { YY_STR_WRITE('\"');         }
<STR>\\\\       { YY_STR_WRITE('\\');         }
<STR>\\\a       { YY_STR_WRITE('\a');         }
<STR>\\\b       { YY_STR_WRITE('\b');         }
<STR>\\\f       { YY_STR_WRITE('\f');         }
<STR>\\\t       { YY_STR_WRITE('\t');         }
<STR>\\\v       { YY_STR_WRITE('\v');         }
<STR>\\\n       { YY_STR_WRITE('\n');         }
<STR>\\\r       { /* omit */                  }
<STR>\n         { /* omit */                  }
<STR>\r         { /* omit */                  }
<STR>.          { YY_STR_WRITE(yytext[0]);    }

\"\"\"          { YY_TXT_BEGIN;   }
<TXT>\"\"\"     { YY_TXT_END; }
<TXT>\n         { YY_TXT_WRITE('\n');         }
<TXT>\r         { /* omit */                  }
<TXT>.          { YY_TXT_WRITE(yytext[0]);    }

"///"[^\n]*     { printf("doc:\n%s\n", yytext); /* return DOC; */ }

"\n"        { YY_NEWLINE; }
[ \t\r]     { }
"#"[^\n]*   { printf("# comment\n"); }
"//"[^\n]*  { }
"/*"(([^\*]*(\*[^\/])?)*)"*/" { }
.           { }

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
