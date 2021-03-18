/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* prologue */

%{

#include "parser.h"
#include "readline.h"
#include "koala_yacc.h"

/* auto insert semi-colon check */
static int semicolon(ParserStateRef ps)
{
    static int tokens[] = {
        INT_LITERAL, FLOAT_LITERAL, STRING_LITERAL,
        ID, SELF, SUPER, TRUE, FALSE,
        ')', ']', '}', '>', '_',
    };

    int token = ps->token;
    for (int i = 0; i < COUNT_OF(tokens); ++i) {
        if (tokens[i] == token) return 1;
    }
    return 0;
}

static char esc(char ch)
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
static void do_escape(ParserStateRef ps, YYSTYPE *yylval)
{
    char *s = SBUF_STR(ps->sbuf);
    int len = SBUF_LEN(ps->sbuf);

    if (!s) {
        yylval->sval = atom_nstr(NULL, 0);
        return;
    }

    SBUF(sbuf);
    char ch;

    for (int i = 0; i < len; ++i) {
        ch = s[i];
        if (ch == '\\') {
            if (i + 1 < len) {
                ch = esc(s[++i]);
            }
        }
        sbuf_print_char(&sbuf, ch);
    }

    if (len < 256) {
        /* short length string in atom */
        yylval->sval = atom_nstr(SBUF_STR(sbuf), len);
        FINI_SBUF(sbuf);
    } else {
        /* long string in string buffer */
        yylval->sval = SBUF_STR(sbuf);
        vector_push_back(&ps->svec, &sbuf);
    }
}

static void do_text(ParserStateRef ps, YYSTYPE *yylval)
{
    int len = SBUF_LEN(ps->sbuf);
    char *s = SBUF_STR(ps->sbuf);

    if (!s) {
        yylval->sval = atom_nstr(NULL, 0);
        return;
    }

    int i = 0;
    while (isspace(s[i])) { i++; len--; }
    while (isspace(s[len - 1])) len--;

    if (len < 256) {
        /* short length text in atom */
        yylval->sval = atom_nstr(s + i, len);
    } else {
        /* text in string buffer */
        SBUF(sbuf);
        sbuf_nprint(&sbuf, s + i, len);
        yylval->sval = SBUF_STR(sbuf);
        vector_push_back(&ps->svec, &sbuf);
    }
}

static void do_doc(ParserStateRef ps, char *txt, int len)
{
    // char *s = txt;
    // char *e = txt + len - 1;
    // while (isspace(*s)) s++;
    // while (isspace(*e)) e--;
    // sbuf_nprint(&ps->doc_buf, s, e - s + 1);

    sbuf_nprint(&ps->doc_buf, txt, len);
    sbuf_print_char(&ps->doc_buf, '\n');
}

int stdin_input(ParserStateRef ps, char *buf, int size);

int file_input(ParserStateRef ps, char *buf, int size, FILE *in)
{
    return 0;
}

/* Macros in flex */

#define YY_INPUT(buf, result, size) \
    if (yyps->interactive) \
        result = stdin_input(yyps, buf, size); \
    else \
        result = file_input(yyps, buf, size, yyin);

#define YY_EXTRA_TYPE ParserStateRef
#define yyps ((YY_EXTRA_TYPE)yyextra)

#define YY_USER_ACTION {                        \
    char ch = yytext[0];                        \
    if (ch != '\n' && ch != '\r') {             \
        yyps->col += yyps->len;                 \
        yyps->len = strlen(yytext);             \
        if (!yyps->in_str) {                    \
            yylloc->first_column = yyps->col;   \
            yylloc->first_line = yyps->line;    \
        }                                       \
    } else if (ch == '\n' && yyps->in_str) {    \
        ++yyps->line;                           \
        yyps->col = 1;                          \
        yyps->len = 0;                          \
    }                                           \
}

#define YY_RETURN(x) yyps->token = x; return x

#define YY_NEWLINE         \
    ++yyps->line;          \
    yyps->col = 1;         \
    yyps->len = 0;         \
    if (semicolon(yyps)) { \
        YY_RETURN(';');    \
    }

#define YY_STR_BEGIN yyps->in_str = 1; RESET_SBUF(yyps->sbuf)

#define YY_STR_END do {      \
    yyps->in_str = 0;           \
    do_escape(yyps, yylval);    \
    RESET_SBUF(yyps->sbuf);     \
    YY_RETURN(STRING_LITERAL);  \
} while (0)

#define YY_STRING_WRITE(ch) \
    sbuf_print_char(&yyps->sbuf, ch)

#define YY_TEXT_BEGIN yyps->in_str = 1; RESET_SBUF(yyps->sbuf)

#define YY_TEXT_END do {        \
    yyps->in_str = 0;           \
    do_text(yyps, yylval);      \
    RESET_SBUF(yyps->sbuf);     \
    YY_RETURN(STRING_LITERAL);  \
} while (0)

#define YY_TEXT_WRITE(ch) \
    sbuf_print_char(&yyps->sbuf, ch)

#define YY_IDENT_VALUE(s, len) \
    yylval->sval = atom_nstr(s, len)

#define YY_INT_VALUE(s, base) \
    yylval->ival = strtoll(s, NULL, base)

#define YY_FLOAT_VALUE(s) \
    yylval->fval = strtod(s, NULL)

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
"final"     { YY_RETURN(FINAL);    }
"pub"       { YY_RETURN(PUB);      }
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

"self"      { YY_RETURN(SELF);  }
"super"     { YY_RETURN(SUPER); }
"true"      { YY_RETURN(TRUE);  }
"false"     { YY_RETURN(FALSE); }
"nil"       { YY_RETURN(NIL);   }

"int8"      { YY_RETURN(INT8);    }
"int16"     { YY_RETURN(INT16);   }
"int32"     { YY_RETURN(INT32);   }
"int64"     { YY_RETURN(INT64);   }
"int"       { YY_RETURN(INT64);   }
"float32"   { YY_RETURN(FLOAT32); }
"float64"   { YY_RETURN(FLOAT64); }
"float"     { YY_RETURN(FLOAT64); }
"bool"      { YY_RETURN(BOOL);    }
"char"      { YY_RETURN(CHAR);    }
"str"       { YY_RETURN(STRING);  }
"any"       { YY_RETURN(ANY);     }

"and"       { YY_RETURN(AND); }
"or"        { YY_RETURN(OR);  }
"not"       { YY_RETURN(NOT); }

"=="        { YY_RETURN(EQ);  }
"!="        { YY_RETURN(NE);  }
">="        { YY_RETURN(GE);  }
"<="        { YY_RETURN(LE);  }
"&&"        { YY_RETURN(AND); }
"||"        { YY_RETURN(OR);  }

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

"..."       {  }
"..<"       {  }
"=>"        {  }
"~="        {  }

"+"         { YY_RETURN('+'); }
"-"         { YY_RETURN('-'); }
"*"         { YY_RETURN('*'); }
"/"         { YY_RETURN('/'); }
"%"         { YY_RETURN('%'); }
"&"         { YY_RETURN('&'); }
"|"         { YY_RETURN('|'); }
"~"         { YY_RETURN('~'); }
"^"         { YY_RETURN('^'); }
"!"         { YY_RETURN(NOT); }

":"         { YY_RETURN(':'); }
","         { YY_RETURN(','); }
";"         { YY_RETURN(';'); }
"."         { YY_RETURN('.'); }
"="         { YY_RETURN('='); }
"_"         { YY_RETURN('_'); }
"?"         { YY_RETURN('?'); }

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
    do_klass_typeparams(yyps, yytext);
    YY_RETURN(ID);
}

0[xX][0-9a-fA-F_]+  { YY_INT_VALUE(yytext + 2, 16); YY_RETURN(INT_LITERAL); }

0[dD][0-9_]+    { YY_INT_VALUE(yytext + 2, 10); YY_RETURN(INT_LITERAL); }
0[oO][0-7_]+    { YY_INT_VALUE(yytext + 2, 8); YY_RETURN(INT_LITERAL);  }
0[bB][01_]+     { YY_INT_VALUE(yytext + 2, 2); YY_RETURN(INT_LITERAL);  }
[0-9][0-9_]*    { YY_INT_VALUE(yytext, 10); YY_RETURN(INT_LITERAL);     }

([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][-+]?[0-9]+)? {
    YY_FLOAT_VALUE(yytext); YY_RETURN(FLOAT_LITERAL);
}

[ \t\r]+ ;

[\n]+ {
    if (yyps->doc_line > 0 && yyps->doc_line != yyps->line) {
        /* not consecutive document */
        printf("doc:\n");
        printf("%s", SBUF_STR(yyps->doc_buf));
        printf("doc is taken\n");
        yyps->doc_line = 0;
        RESET_SBUF(yyps->doc_buf);
    }
    YY_NEWLINE;
}

"///"[^\n]* {
    yyps->more = 0;
    yyps->doc_line = yyps->line;
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

. { printf("error: invalid input\n"); abort(); }

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
