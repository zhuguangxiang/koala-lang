/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
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
        INT_LITERAL, FLOAT_LITERAL, STRING_LITERAL, NONE, ID, SELF,
        TRUE, FALSE, UINT8, UINT16, UINT32, UINT64, INT8, INT16, INT32, INT64,
        FLOAT16, FLOAT32, FLOAT64, BFLOAT16, STRING, BOOL, ANY,
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

static void replace_comma_under(int base, char *s, int len, Buffer *buf)
{
    if (base == 2 || base == 8) s += 2;
    char *end = s + len;
    while (s < end) {
        char ch = *s;
        if (ch != '_' && ch != ',') {
            buf_write_char(buf, ch);
        }
        ++s;
    }
    return;
}

static void parse_decimal_int_literal(ParserState *ps, YYSTYPE *yylval, char *s)
{
    ps->bit_mode = 0;

    // default is signed
    ps->sign = 1;

    int negative = 0;

    if (s[0] == '-') {
        negative = 1;
        s += 1;
    }

    /* 0 ... INT64_MAX ... UINT64_MAX */
    errno = 0;
    yylval->ival = (__int128)strtoll(s, NULL, 10);
    if (errno) {
        // [127 + 1, ...]
        errno = 0;
        yylval->ival = (__int128)strtoull(s, NULL, 10);
        if (errno == 0) {
            if (negative) {
                if (yylval->ival == 9223372036854775808ULL) {
                    yylval->ival = (__int128)-9223372036854775807LL - 1;
                } else {
                    errno = 1;
                }
            } else {
                // mark it as unsigned
                ps->sign = 0;
            }
        }
    } else {
        // [0, 127]
        if (negative) { yylval->ival = -yylval->ival; }
    }
}

static void parse_int_literal(ParserState *ps, YYSTYPE *yyval, char *s, int len, int base)
{
    ps->sval = atom_nstr(s, len);
    BUF(buf);
    replace_comma_under(base, s, len, &buf);
    char *_s = BUF_STR(buf);
    if (base != 10) {
        errno = 0;
        if (_s[0] == '-') {
            ps->sign = 1;
            _s += 1;
        } else {
            ps->sign = 0;
        }
        yyval->ival = (__int128)strtoull(_s, NULL, base);
        ps->bit_mode = 1;
        if (ps->sign) { yyval->ival = -yyval->ival; }
    } else {
        parse_decimal_int_literal(ps, yyval, _s);
    }
    FINI_BUF(buf);
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
    parse_int_literal(yyps, yylval, s, len, base)

#define YY_FLOAT_VALUE(s, len) \
    errno = 0; yyps->sval = atom_nstr(s, len); \
    yylval->fval = strtold(s, NULL)

#define YY_S_BEGIN yyps->multi = 1; RESET_BUF(yyps->sbuf); BEGIN(S);
#define YY_S_WRITE(ch) buf_write_char(&yyps->sbuf, ch)
#define YY_S_END yyps->multi = 0; BEGIN(INITIAL); YY_RETURN(STRING_LITERAL);

#define YY_STR_BEGIN yyps->multi = 1; RESET_BUF(yyps->sbuf); BEGIN(STR);
#define YY_STR_WRITE(ch) buf_write_char(&yyps->sbuf, ch)
#define YY_STR_END yyps->multi = 0; BEGIN(INITIAL); YY_RETURN(STRING_LITERAL);

#define YY_TXT_BEGIN yyps->multi = 1; RESET_BUF(yyps->sbuf); BEGIN(TXT);
#define YY_TXT_WRITE(ch) buf_write_char(&yyps->sbuf, ch)
#define YY_TXT_END yyps->multi = 0; BEGIN(INITIAL); YY_RETURN(STRING_LITERAL);

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
"const"     { YY_RETURN(CONST);     }
"let"       { YY_RETURN(LET);       }
"var"       { YY_RETURN(VAR);       }
"func"      { YY_RETURN(FUNC);      }
"class"     { YY_RETURN(CLASS);     }
"trait"     { YY_RETURN(TRAIT);     }
"if"        { YY_RETURN(IF);        }
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
"pub"       { YY_RETURN(PUBLIC);    }
"infer"     { YY_RETURN(INFER);     }

"self"      { YY_RETURN(SELF);      }
"true"      { YY_RETURN(TRUE);      }
"false"     { YY_RETURN(FALSE);     }
"null"      { YY_RETURN(NONE);      }

"uint8"     { YY_RETURN(UINT8);     }
"uint16"    { YY_RETURN(UINT16);    }
"uint32"    { YY_RETURN(UINT32);    }
"uint64"    { YY_RETURN(UINT64);    }
"uint"      { YY_RETURN(UINT64);    }
"int8"      { YY_RETURN(INT8);      }
"int16"     { YY_RETURN(INT16);     }
"int32"     { YY_RETURN(INT32);     }
"int64"     { YY_RETURN(INT64);     }
"int"       { YY_RETURN(INT64);     }
"float16"   { YY_RETURN(FLOAT16);   }
"float32"   { YY_RETURN(FLOAT32);   }
"float64"   { YY_RETURN(FLOAT64);   }
"float"     { YY_RETURN(FLOAT64);   }
"bfloat16"  { YY_RETURN(BFLOAT16);  }
"bool"      { YY_RETURN(BOOL);      }
"str"       { YY_RETURN(STRING);    }
"any"       { YY_RETURN(ANY);       }
"list"      { YY_RETURN(LIST);      }
"dict"      { YY_RETURN(MAP);       }
"set"       { YY_RETURN(SET);       }
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
"..."       { YY_RETURN(DOTDOTDOT);    }
"..<"       { YY_RETURN(DOTDOTDOT);    }
"<<"        { YY_RETURN(L_SHIFT);      }
">>"        { YY_RETURN(R_SHIFT);      }

"?:"        { YY_RETURN(OPT_DEF);   }
"?."        { YY_RETURN(OPT_DOT);   }
"!."        { YY_RETURN(BANG_DOT);  }

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

[+-]?0[bB][01]([01_]*[01])? {
    YY_INT_VALUE(yytext, yyleng, 2); YY_RETURN(INT_LITERAL);
}

[+-]?0[oO][0-7]([0-7_]*[0-7])? {
    YY_INT_VALUE(yytext, yyleng, 8); YY_RETURN(INT_LITERAL);
}

[+-]?0[xX][0-9a-fA-F]([0-9a-fA-F_]*[0-9a-fA-F])? {
    YY_INT_VALUE(yytext, yyleng, 16); YY_RETURN(INT_LITERAL);
}

[A-Za-z_$][A-Za-z0-9_$]* {
    YY_IDENT_VALUE(yytext, yyleng); YY_RETURN(ID);
}

[+-]?([0-9]+\.[0-9]+([eE][+-]?[0-9]+)?|[0-9]+[eE][+-]?[0-9]+) {
    YY_FLOAT_VALUE(yytext, yyleng); YY_RETURN(FLOAT_LITERAL);
}

[+-]?[0-9]([0-9_]*[0-9])? {
    YY_INT_VALUE(yytext, yyleng, 10); YY_RETURN(INT_LITERAL);
}

\'              { YY_S_BEGIN;              }
<S>\'           { YY_S_END;                }
<S>\\\'         { YY_S_WRITE('\'');        }
<S>.            { YY_S_WRITE(yytext[0]);   }

\"              { YY_STR_BEGIN;            }
<STR>\"         { YY_STR_END;              }
<STR>\\\"       { YY_STR_WRITE('\"');      }
<STR>.          { YY_STR_WRITE(yytext[0]); }

\"\"\"          { YY_TXT_BEGIN;            }
<TXT>\"\"\"     { YY_TXT_END;              }
<TXT>.          { YY_TXT_WRITE(yytext[0]); }

"///"[^\n]* {
    RESET_BUF(yyps->sbuf);
    buf_write_nstr(&yyps->sbuf, yytext + 3, yyleng - 3);
    YY_RETURN(DOC);
}

"\n"            { YY_NEWLINE; }
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
