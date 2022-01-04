/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

/* prologue */

%{

#include "parser.h"
#include "koala_yacc.h"

#define YY_EXTRA_TYPE ParserState *
#define yyps ((YY_EXTRA_TYPE)yyextra)

/* auto insert semi-colon check */
static int semicolon(YY_EXTRA_TYPE ps)
{
    static int tokens[] = {
        INT_LITERAL, FLOAT_LITERAL, CHAR_LITERAL, STRING_LITERAL,
        ID, SELF, SUPER, TRUE, FALSE, NIL,
        INT8, INT16, INT32, INT64, CHAR, STRING, BOOL, ANY,
        RETURN, BREAK, CONTINUE, ')', ']', '}', '>', '_',
        DOTDOTDOT, DOTDOTLESS,
    };

    int token = ps->token;
    for (int i = 0; i < COUNT_OF(tokens); ++i) {
        if (tokens[i] == token) return 1;
    }
    return 0;
}

static char esc_convert(char ch)
{
    char val;
    switch (ch) {
    case 'a':
        val = 7;
        break;
    case 'b':
        val = 8;
        break;
    case 'f':
        val = 12;
        break;
    case 'n':
        val = 10;
        break;
    case 'r':
        val = 13;
        break;
    case 't':
        val = 9;
        break;
    case 'v':
        val = 11;
        break;
    default:
        val = ch;
        break;
    }
    return val;
}

/* escape convert */
static void do_escape(YY_EXTRA_TYPE ps, YYSTYPE *yylval)
{
    char *s = BUF_STR(ps->buf);
    int len = BUF_LEN(ps->buf);

    if (!s) {
        yylval->sval = atom_str(null, 0);
        return;
    }

    BUF(buf);
    char ch;

    for (int i = 0; i < len; ++i) {
        ch = s[i];
        if (ch == '\\') {
            if (i + 1 < len) {
                ch = esc_convert(s[++i]);
            }
        }
        buf_write_char(&buf, ch);
    }

    if (len < 256) {
        /* short length string in Atom */
        yylval->sval = atom_str(BUF_STR(buf), len);
        FINI_BUF(buf);
    } else {
        /* long string in string buffer */
        yylval->sval = BUF_STR(buf);
        vector_push_back(&ps->svec, &buf);
    }
}

static void do_text(YY_EXTRA_TYPE ps, YYSTYPE *yylval)
{
    int len = BUF_LEN(ps->buf);
    char *s = BUF_STR(ps->buf);

    if (!s) {
        yylval->sval = atom_str(null, 0);
        return;
    }

    int i = 0;
    while (isspace(s[i])) { i++; len--; }
    while (isspace(s[len - 1])) len--;

    if (len < 256) {
        /* short length text in Atom */
        yylval->sval = atom_str(s + i, len);
    } else {
        /* text in string buffer */
        BUF(buf);
        buf_write_nstr(&buf, s + i, len);
        yylval->sval = BUF_STR(buf);
        vector_push_back(&ps->svec, &buf);
    }
}

static void do_doc(YY_EXTRA_TYPE ps, char *txt, int len)
{
    // char *s = txt;
    // char *e = txt + len - 1;
    // while (isspace(*s)) s++;
    // while (isspace(*e)) e--;
    // buf_write_nstr(&ps->doc_buf, s, e - s + 1);

    buf_write_nstr(&ps->doc_buf, txt, len);
    buf_write_char(&ps->doc_buf, '\n');
}

/* Macros in flex */

#define YY_INPUT(buf, ret, size) do { \
    ret = yyps->lexer_input(yyps, buf, size, yyin); \
} while (0)

#define YY_USER_ACTION {                        \
    char ch = yytext[0];                        \
    if (ch != '\n' && ch != '\r') {             \
        yyps->col += yyps->len;                 \
        yyps->len = strlen(yytext);             \
        if (!yyps->in_str) {                    \
            yylloc->first_line = yyps->row;     \
            yylloc->first_column = yyps->col;   \
            yylloc->last_column = yylloc->first_column + yyps->len; \
        }                                       \
    } else if (ch == '\n' && yyps->in_str) {    \
        ++yyps->row;                            \
        yyps->col = 1;                          \
        yyps->len = 0;                          \
    }                                           \
}

#define YY_RETURN(x) yyps->token = x; return x

/* 32 or 64 check */
#if INTPTR_MAX == INT64_MAX
    #define YY_RETURN_INT() YY_RETURN(INT64)
#elif INTPTR_MAX == INT32_MAX
    #define YY_RETURN_INT() YY_RETURN(INT32)
#else
    #error "Must define either 32 or 64 bits".
#endif

#define YY_NEWLINE do { \
    int len = yyps->len; \
    ++yyps->row; \
    yyps->col = 1; \
    yyps->len = 0; \
    if (semicolon(yyps)) { \
        yylloc->first_column += len; \
        yylloc->last_column = yylloc->first_column + len; \
        buf_write_char(&yyps->linebuf, ';'); \
        YY_RETURN(';'); \
    } \
} while (0)

#define YY_STR_BEGIN yyps->in_str = 1; RESET_BUF(yyps->buf)

#define YY_STR_END do {         \
    yyps->in_str = 0;           \
    do_escape(yyps, yylval);    \
    RESET_BUF(yyps->buf);       \
    YY_RETURN(STRING_LITERAL);  \
} while (0)

#define YY_STRING_WRITE(ch) \
    buf_write_char(&yyps->buf, ch)

#define YY_TEXT_BEGIN yyps->in_str = 1; RESET_BUF(yyps->buf)

#define YY_TEXT_END do {        \
    yyps->in_str = 0;           \
    do_text(yyps, yylval);      \
    RESET_BUF(yyps->buf);       \
    YY_RETURN(STRING_LITERAL);  \
} while (0)

#define YY_TEXT_WRITE(ch) \
    buf_write_char(&yyps->buf, ch)

#define YY_IDENT_VALUE(s, len) \
    yylval->sval = atom_str(s, len)

#define YY_INT_VALUE(s, base) \
    yylval->ival = strtoll(s, null, base)

#define YY_FLOAT_VALUE(s) \
    yylval->fval = strtod(s, null); \
    printf("%lf\n", yylval->fval);

%}

%option bison-locations
%option bison-bridge
%option reentrant
%option nounput
%option noinput
%option never-interactive

%x STR TXT COM

/* rules */

%%

"import"    { YY_RETURN(IMPORT);   }
"type"      { YY_RETURN(TYPE);     }
"let"       { YY_RETURN(LET);      }
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
"case"      { YY_RETURN(CASE);     }
"break"     { YY_RETURN(BREAK);    }
"continue"  { YY_RETURN(CONTINUE); }
"return"    { YY_RETURN(RETURN);   }
"in"        { YY_RETURN(IN);       }
"as"        { YY_RETURN(AS);       }
"is"        { YY_RETURN(IS);       }
"final"     { YY_RETURN(FINAL);    }

"self"      { YY_RETURN(SELF);  }
"super"     { YY_RETURN(SUPER); }
"true"      { YY_RETURN(TRUE);  }
"false"     { YY_RETURN(FALSE); }
"nil"       { YY_RETURN(NIL);   }

"int8"      { YY_RETURN(INT8);    }
"int16"     { YY_RETURN(INT16);   }
"int32"     { YY_RETURN(INT32);   }
"int"       { YY_RETURN_INT();    }
"int64"     { YY_RETURN(INT64);   }
"float32"   { YY_RETURN(FLOAT32); }
"float64"   { YY_RETURN(FLOAT64); }
"bool"      { YY_RETURN(BOOL);    }
"char"      { YY_RETURN(CHAR);    }
"string"    { YY_RETURN(STRING);  }
"any"       { YY_RETURN(ANY);     }

"and"       { YY_RETURN(AND); }
"or"        { YY_RETURN(OR);  }
"not"       { YY_RETURN(NOT); }

"=="        { YY_RETURN(EQ);  }
"!="        { YY_RETURN(NE);  }
">="        { YY_RETURN(GE);  }
"<="        { YY_RETURN(LE);  }

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

"..."       { YY_RETURN(DOTDOTDOT);  }
"..<"       { YY_RETURN(DOTDOTLESS); }

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
"<" {
    if (yyps->in_angle) {
        yyps->in_angle = 0;
        YY_RETURN(L_ANGLE_ARGS);
    } else {
        YY_RETURN('<');
    }
}

"<<"        { YY_RETURN(L_SHIFT); }
">"/">"     { YY_RETURN(R_ANGLE_SHIFT); }
">>>"       { YY_RETURN(R_ANGLE_USHIFT); }

\"          { BEGIN(STR); YY_STR_BEGIN;     }
<STR>\"     { BEGIN(INITIAL); YY_STR_END;   }
<STR>\\\"   { YY_STRING_WRITE('\"');        }
<STR>\\\\   { YY_STRING_WRITE('\\');        }
<STR>\n     {
                /* omit newline in string */
                yyps->more = 1;
            }
<STR>\r     { /* string omit return */      }
<STR>.      { YY_STRING_WRITE(yytext[0]);   }

\"\"\"      { BEGIN(TXT); YY_TEXT_BEGIN;    }
<TXT>\"\"\" { BEGIN(INITIAL); YY_TEXT_END;  }
<TXT>\n     {
                /* text with newline */
                yyps->more = 1;
                YY_TEXT_WRITE('\n');
            }
<TXT>\r     { /* text omit return */        }
<TXT>.      { YY_TEXT_WRITE(yytext[0]);     }

[A-Za-z_][A-Za-z0-9_]*  {
    YY_IDENT_VALUE(yytext, yyleng);
    ident_has_param_type(yyps, yytext);
    YY_RETURN(ID);
}

0[xX][0-9a-fA-F_]+  { YY_INT_VALUE(yytext + 2, 16); YY_RETURN(INT_LITERAL); }

0[dD][0-9_]+    { YY_INT_VALUE(yytext + 2, 10); YY_RETURN(INT_LITERAL); }
0[oO][0-7_]+    { YY_INT_VALUE(yytext + 2, 8); YY_RETURN(INT_LITERAL);  }
0[bB][01_]+     { YY_INT_VALUE(yytext + 2, 2); YY_RETURN(INT_LITERAL);  }
[0-9][0-9_]*    { YY_INT_VALUE(yytext, 10); YY_RETURN(INT_LITERAL);     }

([0-9]+\.[0-9]+|[0-9]+)([eE][-+]?[0-9]+)? {
    YY_FLOAT_VALUE(yytext); YY_RETURN(FLOAT_LITERAL);
}

[ ]+  { }
[\t]+ { }
[\r]+ { }

[\n]+ {
    if (yyps->doc_line > 0 && yyps->doc_line != yyps->row) {
        /* not consecutive document */
        printf("doc\n");
        printf("%s", BUF_STR(yyps->doc_buf));
        printf("doc is taken\n");
        yyps->doc_line = 0;
        RESET_BUF(yyps->doc_buf);
    }
    YY_NEWLINE;
}

"///"[^\n]* {
    yyps->more = 0;
    yyps->doc_line = yyps->row;
    do_doc(yyps, yytext + 3, yyleng - 3);
}

("//"|#)[^\n]*  {
    /* single line comment */
    yyps->more = 0;
}

"/*"        { BEGIN(COM); yyps->in_str = 1; }
<COM>"*/"   { BEGIN(INITIAL); yyps->in_str = 0; yyps->more = 0; }
<COM>\n     { yyps->more = 1; }
<COM>.      { /* omit other characters */ }

. { yylval->sval = yytext; YY_RETURN(INVALID); }

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
