/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_PARSER_H_
#define _KOALA_PARSER_H_

#include "ast.h"
#include "symbol.h"
#include "util/atom.h"
#include "util/buffer.h"
#include "util/color.h"
#include "util/hashmap.h"
#include "util/vector.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ERRORS 8

typedef enum _ScopeKind ScopeKind;
typedef struct _ParserScope ParserScope;
typedef struct _ParserState ParserState;
typedef struct _ParserGroup ParserGroup;

enum _ScopeKind {
    SCOPE_INVALID,
    SCOPE_TOP,
    SCOPE_TYPE,
    SCOPE_FUNC,
    SCOPE_BLOCK,
    SCOPE_ANONY,
};

struct _ParserScope {
    /* one of ScopeKind */
    ScopeKind kind;
    /* function, class, trait and method */
    Symbol *sym;
    /* symbol table for current scope */
    HashMap *stbl;
    /* which block scope */
    int blocktype;
#define ONLY_BLOCK   1
#define IF_BLOCK     2
#define WHILE_BLOCK  3
#define FOR_BLOCK    4
#define MATCH_BLOCK  5
#define MATCH_CASE   6
#define MATCH_CLAUSE 7
};

/* per file in one package */
struct _ParserState {
    /* file name */
    char *filename;
    /* -> ParserGroup */
    ParserGroup *grp;
    /* statements */
    Vector stmts;
    /* errors */
    int errors;

    /* current scope */
    ParserScope *scope;
    /* depth of scope */
    int depth;
    /* scope stack */
    Vector scope_stack;

    /* is interactive ? */
    int cmdline;
    /* is complete ? */
    int more;

    /* current line buffer */
    Buffer linebuf;

    /* token ident */
    int token;
    /* token length */
    int len;
    /* token line */
    int row;
    /* token column */
    int col;
    /* in multi-line string */
    int in_str;

    /* string buffer */
    Buffer buf;
    /* long string/text */
    Vector svec;

    /* document */
    Buffer doc_buf;
    int doc_line;

    /* type parameter */
    int in_angle;

    /* CTRL_D */
    int quit;

    /* lexer input */
    int (*lexer_input)(ParserState *, char *, int, FILE *);

    /* parser statement handler */
    void (*handle_stmt)(ParserState *, Stmt *);
};

/* per one package */
struct _ParserGroup {
    /* external packages */
    HashMap *pkgs;
    /* package symbol */
    Symbol *sym;
    /* symbol table */
    HashMap *stbl;
    /* compiled files */
    Vector files;
    /* errors */
    int errors;
};

void init_parser(void);
void fini_parser(void);
void parser_enter_scope(ParserState *ps, ScopeKind kind, int block);
void parser_exit_scope(ParserState *ps);
void parse_stmt(ParserState *ps, Stmt *stmt);
void parser_append_stmt(ParserState *ps, Stmt *stmt);
void parser_new_var(ParserState *ps, Stmt *Stmt);
void parser_new_func(ParserState *ps, Stmt *stmt);
void ident_has_param_type(ParserState *ps, char *name);

void yy_error_detail(ParserState *ps, int row, int col);

/* clang-format off */
#define ERROR_PREFIX(x) BOLD_COLOR(x) ERR_COLOR

#define yy_errmsg(loc, fmt, ...) do { \
    if (ps->errors++ >= MAX_ERRORS) { \
        printf(ERROR_PREFIX("%s: ") "Too many errors.\n", ps->filename); \
    } else { \
        printf(ERROR_PREFIX("%s:%d:%d: ") fmt "\n", \
            ps->filename, (loc).row, (loc).col, ##__VA_ARGS__); \
        yy_error_detail(ps, (loc).row, (loc).col); \
    } \
} while (0)
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_PARSER_H_ */
