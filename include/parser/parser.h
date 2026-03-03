/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_PARSER_H_
#define _KOALA_PARSER_H_

#include "ast.h"
#include "common.h"
#include "ir.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _ScopeKind {
    SCOPE_TOP,
    SCOPE_CLASS,
    SCOPE_TRAIT,
    SCOPE_FUNC,
    SCOPE_BLOCK,
    SCOPE_ANONY,
} ScopeKind;

typedef enum _BlockType {
    UNK_BLOCK,
    ONLY_BLOCK,
    IF_BLOCK,
    ELSE_BLOCK,
    IF_LET_BLOCK,
    WHILE_BLOCK,
    WHILE_LET_BLOCK,
    FOR_BLOCK,
    SWITCH_BLOCK,
    CASE_BLOCK
} BlockType;

typedef struct _ParserScope {
    /* link to scope stack */
    struct _ParserScope *next;
    /* one of ScopeKind */
    ScopeKind kind;
    /* scope name */
    char *name;
    /* scope depth */
    int depth;
    /* which block scope */
    BlockType block_type;
    /* this scope's symbol(func, class and etc.) */
    Symbol *sym;
    /* symbol table of this scope */
    HashMap *stbl;
    /* basic block */
    KlrBasicBlock *bb;
} ParserScope;

/* per source file */
typedef struct _ParserState {
    /* src file name */
    char *filename;

    /* statements */
    Vector stmts;

    /* klass stmts */
    Vector kls_stmts;

    /* func stmts */
    Vector fn_stmts;

    /* current scope */
    ParserScope *scope;
    /* depth of scope */
    int depth;

    /* status */
    int status;
#define PS_STATUS_UNRESOLVED 0
#define PS_STATUS_RESOLVED   1
#define PS_STATUS_RESOLVING  2

    /* temperary shadows */
    Vector shadows;

    /* current file imported */
    HashMap *imported;

    /* builtin table */
    HashMap *builtin;

    /* symbol table */
    HashMap *stbl;

    /* IR module */
    KlrModule *module;

    /* token */
    int token;
    /* multi-lines */
    int multi;
    /* newline */
    int newline;
    /* errors */
    int errors;

    /* string */
    Buffer sbuf;

    /* string for print error */
    char *sval;
    /* for literal integer */
    int sign;
    /* for non-decimal literals */
    int bit_mode;
} ParserState;

Symbol *find_symbol(ParserState *ps, Ident *id);

/* more than MAX_ERRORS, discard remaining errors shown */
#define MAX_ERRORS 32

#define BOLD_SEQ     "\033[1m"
#define COLOR_RED    "\x1b[31m"
#define COLOR_GREEN  "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_RESET  "\x1b[0m"

#define BOLD(x)      BOLD_SEQ x COLOR_RESET
#define ERROR_PREFIX BOLD_SEQ COLOR_RED "error: " COLOR_RESET
#define WARN_PREFIX  BOLD_SEQ COLOR_YELLOW "warning: " COLOR_RESET

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

#define kl_warn(_loc, fmt, ...) do { \
    Loc loc = _loc; \
    printf(BOLD("%s:%d:%d: ") WARN_PREFIX fmt "\n", \
        ps->filename, loc.line, loc.col, ##__VA_ARGS__); \
    kl_error_detail(ps, &loc); \
} while (0)

#define kl_error_incompatible_type(_loc, expected, actual) do { \
    if (ps->errors++ >= MAX_ERRORS) { \
        printf(BOLD("%s: ") ERROR_PREFIX "Too many errors.\n", ps->filename); \
    } else { \
        BUF(buf); \
        buf_write_str(&buf, ERROR_PREFIX "Incompatible type: expected '"); \
        type_spec_print(expected, &buf); \
        buf_write_str(&buf, "', but got '"); \
        type_spec_print(actual, &buf); \
        buf_write_str(&buf, "'.\n"); \
        Loc loc = _loc; \
        printf(BOLD("%s:%d:%d: ") "%s", ps->filename, loc.line, loc.col, BUF_STR(buf)); \
        kl_error_detail(ps, &loc); \
        FINI_BUF(buf); \
    } \
} while (0)

/* clang-format on */

void parser_visit_expr(ParserState *ps, Expr *exp);
void parse_top_stmt(ParserState *ps, Stmt *stmt);
void parser_ast_genir(ParserState *ps);
ParserState *new_parser_state(char *path);
void free_parser_state(ParserState *ps);
int do_compile(Vector *pss, char *output);
Vector *infer_func_tp(FuncSymbol *fn, Vector *args, ParserState *ps);

void init_parser(void);
void fini_parser(void);

void parse_stmt(ParserState *ps, Stmt *stmt);

ParserScope *enter_scope(ParserState *ps, ScopeKind kind, BlockType block, char *name);
void exit_scope(ParserState *ps);

TypeSpec *resolve_type(ParserState *ps, TypeSpec *_ts);
int check_type(ParserState *ps, TypeSpec *ts);
int type_spec_compatible(TypeSpec *dst, TypeSpec *src);

void write_to_klc(HashMap *stbl, char *path);
HashMap *load_module(char *path);
void kl_emit(ParserState *ps, KlrModule *m);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_PARSER_H_ */
