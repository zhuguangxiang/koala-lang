/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_PARSER_H_
#define _KOALA_PARSER_H_

#include "ast.h"
#include "common.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _ScopeKind {
    SCOPE_TOP,
    SCOPE_TYPE,
    SCOPE_FUNC,
    SCOPE_BLOCK,
    SCOPE_ANONY,
} ScopeKind;

typedef enum _BlockType {
    UNK_BLOCK,
    ONLY_BLOCK,
    IF_BLOCK,
    WHILE_BLOCK,
    FOR_BLOCK,
    SWITCH_BLOCK,
    CASE_BLOCK
} BlockType;

typedef struct _ParserScope {
    /* link to scope stack */
    struct _ParserScope *next;
    /* one of ScopeKind */
    ScopeKind kind;
    /* which block scope */
    BlockType block_type;
    /* this scope's symbol(func, class and etc.) */
    Symbol *sym;
    /* symbol table of this scope */
    HashMap *stbl;
} ParserScope;

/* per compiled file */
typedef struct _ParserState {
    /* src file name */
    char *filename;
    /* statements */
    Vector stmts;

    /* current scope */
    ParserScope *scope;
    /* depth of scope */
    int depth;

    /* symbol table */
    HashMap *stbl;

    /* token */
    int token;
    /* multi-lines mode */
    int multi;
    /* newline */
    int newline;
    /* errors */
    int errors;

    /* string for print error */
    char *sval;
} ParserState;

/* more than MAX_ERRORS, discard remaining errors shown */
#define MAX_ERRORS 8

#define BOLD_SEQ     "\033[1m"
#define COLOR_RED    "\x1b[31m"
#define COLOR_GREEN  "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_RESET  "\x1b[0m"

#define BOLD(x)      BOLD_SEQ x COLOR_RESET
#define ERROR_PREFIX BOLD_SEQ COLOR_RED "error: " COLOR_RESET

/* clang-format off */

#define kl_printf_error(fmt, ...) do { \
    if (isatty(STDOUT_FILENO)) \
        fprintf(stdout, ERROR_PREFIX fmt, ##__VA_ARGS__); \
    else \
        fprintf(stdout, "error: " fmt, ##__VA_ARGS__); \
} while (0)

void kl_error_detail(ParserState *, Loc *);

#define kl_error(_loc, fmt, ...) do { \
    if (ps->errors++ >= MAX_ERRORS) { \
        printf(BOLD("%s: ") ERROR_PREFIX "Too many errors.\n", ps->filename); \
    } else { \
        Loc loc = _loc; \
        printf(BOLD("%s:%d:%d: ") ERROR_PREFIX fmt "\n", \
            ps->filename, loc.line, loc.col, ##__VA_ARGS__); \
        kl_error_detail(ps, &loc); \
    } \
} while (0)

/* clang-format on */

void parse_expr(ParserState *ps, Expr *exp);
void yyparse_top_decl(ParserState *ps, Stmt *stmt);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_PARSER_H_ */
