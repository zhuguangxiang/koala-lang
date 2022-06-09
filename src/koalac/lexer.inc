/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#include <errno.h>
/* clang-format off */
#include "parser.h"
#include "koala_yacc.h"
/* clang-format on */

#define YY_EXTRA_TYPE ParserState *

#define yyps ((YY_EXTRA_TYPE)yyextra)

/* auto insert semi-colon check */
static int semicolon(YY_EXTRA_TYPE ps)
{
    static int tokens[] = {
        INT_LITERAL, FLOAT_LITERAL, CHAR_LITERAL, STRING_LITERAL,
        ID,          SELF,          SUPER,        TRUE,
        FALSE,       NULL_TK,       INT8,         INT16,
        INT32,       INT64,         CHAR,         STRING,
        BOOL,        ANY,           RETURN,       BREAK,
        CONTINUE,    ')',           ']',          '}',
        '>',         '_',           DOTDOTDOT,    DOTDOTLESS,
        EXTERNC,
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
        yylval->sval = atom_nstr(NULL, 0);
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
        yylval->sval = atom_nstr(BUF_STR(buf), len);
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
        yylval->sval = atom_nstr(NULL, 0);
        return;
    }

    int i = 0;
    while (isspace(s[i])) {
        i++;
        len--;
    }
    while (isspace(s[len - 1])) len--;

    if (len < 256) {
        /* short length text in Atom */
        yylval->sval = atom_nstr(s + i, len);
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

int file_input(YY_EXTRA_TYPE ps, char *buf, int size, FILE *in)
{
    UNUSED(ps);
    errno = 0;
    int result = 0;
    while ((result = (int)fread(buf, 1, (size_t)size, in)) == 0 && ferror(in)) {
        if (errno != EINTR) {
            printf("Input in scanner failed.\n");
            break;
        }
        errno = 0;
        clearerr(in);
    }
    return result;
}

/* Macros in flex */

#define YY_INPUT(buf, ret, size) ret = file_input(yyps, buf, size, yyin)

#define YY_USER_ACTION                                                  \
    {                                                                   \
        char ch = yytext[0];                                            \
        if (ch != '\n' && ch != '\r') {                                 \
            yyps->col += yyps->len;                                     \
            yyps->len = strlen(yytext);                                 \
            if (!yyps->in_str) {                                        \
                yylloc->first_line = yyps->row;                         \
                yylloc->first_column = yyps->col;                       \
                yylloc->last_column = yylloc->first_column + yyps->len; \
            }                                                           \
        } else if (ch == '\n' && yyps->in_str) {                        \
            ++yyps->row;                                                \
            yyps->col = 1;                                              \
            yyps->len = 0;                                              \
        }                                                               \
    }

#define YY_RETURN(x) \
    yyps->token = x; \
    return x

/* 32 or 64 check */
#if INTPTR_MAX == INT64_MAX
#define YY_RETURN_INT()  YY_RETURN(INT64)
#define YY_RETURN_UINT() YY_RETURN(UINT64)
#elif INTPTR_MAX == INT32_MAX
#define YY_RETURN_INT()  YY_RETURN(INT32)
#define YY_RETURN_UINT() YY_RETURN(UINT32)
#else
#error "Must define either 32 or 64 bits".
#endif

#define YY_NEWLINE                                            \
    do {                                                      \
        int len = yyps->len;                                  \
        ++yyps->row;                                          \
        yyps->col = 1;                                        \
        yyps->len = 0;                                        \
        if (semicolon(yyps)) {                                \
            yylloc->first_column += len;                      \
            yylloc->last_column = yylloc->first_column + len; \
            buf_write_char(&yyps->linebuf, ';');              \
            YY_RETURN(';');                                   \
        }                                                     \
    } while (0)

#define YY_STR_BEGIN  \
    yyps->in_str = 1; \
    RESET_BUF(yyps->buf)

#define YY_STR_END                 \
    do {                           \
        yyps->in_str = 0;          \
        do_escape(yyps, yylval);   \
        RESET_BUF(yyps->buf);      \
        YY_RETURN(STRING_LITERAL); \
    } while (0)

#define YY_STRING_WRITE(ch) buf_write_char(&yyps->buf, ch)

#define YY_TEXT_BEGIN \
    yyps->in_str = 1; \
    RESET_BUF(yyps->buf)

#define YY_TEXT_END                \
    do {                           \
        yyps->in_str = 0;          \
        do_text(yyps, yylval);     \
        RESET_BUF(yyps->buf);      \
        YY_RETURN(STRING_LITERAL); \
    } while (0)

#define YY_TEXT_WRITE(ch) buf_write_char(&yyps->buf, ch)

#define YY_IDENT_VALUE(s, len) yylval->sval = atom_nstr(s, len)

#define YY_INT_VALUE(s, base) yylval->ival = strtoll(s, NULL, base)

#define YY_FLOAT_VALUE(s)           \
    yylval->fval = strtod(s, NULL); \
    printf("%lf\n", yylval->fval);
