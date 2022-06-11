/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#ifndef _KOALA_PARSER_H_
#define _KOALA_PARSER_H_

#include "ast.h"
#include "common/atom.h"
#include "common/buffer.h"
#include "common/hashmap.h"
#include "common/vector.h"
#include "symbol.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ERRORS 8

typedef enum _ScopeKind ScopeKind;
typedef struct _ParserScope ParserScope;
typedef struct _ParserState ParserState;
typedef struct _ParserPackage ParserPackage;

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
    /* function, class, interface and method */
    Symbol *sym;
    /* symbol table for current scope */
    HashMap *stbl;
    /* which block scope */
    int block_type;
#define ONLY_BLOCK   1
#define IF_BLOCK     2
#define WHILE_BLOCK  3
#define FOR_BLOCK    4
#define MATCH_BLOCK  5
#define MATCH_CASE   6
#define MATCH_CLAUSE 7
};

/* per file one package */
struct _ParserState {
    /* file name */
    char *filename;
    /* file */
    FILE *in;
    /* -> ParserPackage */
    ParserPackage *pkg;
    /* statements */
    Vector stmts;

    /* current scope */
    ParserScope *scope;
    /* depth of scope */
    int depth;
    /* scope stack */
    Vector scope_stack;

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

    /* bytes read */
    int read_bytes;

    /* errors */
    int errors;

    int last_line;
    int line;
};

/* one package is compiled unit, and has one meta file */
struct _ParserPackage {
    /* imported packages */
    HashMap *pkgs;
    /* package name */
    char *name;
    /* symbol table */
    HashMap *stbl;
    /* compiled files (ParserState) */
    Vector files;
};

/* one module is linked into one shared object */
struct _ParserModule {
    char *so_name;
    Vector packages;
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

static inline void parser_error_detail(ParserState *ps, int row, int col)
{
    FILE *in = ps->in;
    if (!in) {
        in = fopen(ps->filename, "r");
        ps->in = in;
    }

    // printf(">>%d,%d\n", ps->last_line, ps->line);

    if (col == 1) {
        fseek(in, ps->line, SEEK_SET);
    } else {
        fseek(in, ps->last_line, SEEK_SET);
    }

    char buf[256];
    int nbytes = fread(buf, 1, 256, in);
    for (int i = 0; i < nbytes; i++) {
        if (buf[i] == '\n') {
            buf[i] = 0;
        }
    }

    char *line = buf;
    while (*line == 0) line++;

    printf("%5d | %s\n", row, line);
    if (col - 1 == 0) {
        printf("%5c | ^\n", ' ');
    } else {
        printf("%5c | %*c^\n", ' ', col - 1, ' ');
    }
}

/* clang-format off */

#define parser_error(loc, fmt, ...) do { \
    if (ps->errors++ >= MAX_ERRORS) { \
        printf(RED_COLOR("%s: error: ") "Too many errors.\n", ps->filename); \
    } else { \
        printf(RED_COLOR("%s:%d:%d: error: ") fmt "\n", \
            ps->filename, (loc).row, (loc).col, ##__VA_ARGS__); \
        parser_error_detail(ps, (loc).row, (loc).col); \
    } \
} while (0)
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_PARSER_H_ */
