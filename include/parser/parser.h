/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
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

typedef struct _ParserModule {
    /* module file path(xxx.klc) */
    char *path;

    /* module pkg-path(xxx) */
    char *pkg_path;

    /* per-file ParserState */
    Vector pss;

    /* module-level symbol table */
    HashMap *stbl;

    /* all imported packages */
    HashMap *imported;

    /* builtin package */
    HashMap *builtin;

    /* module-level IR*/
    KlrModule *m;

    /* links */
    Vector links;
} ParserModule;

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
    /* continue block for loops */
    KlrBasicBlock *continue_bb;
    /* break block for loops */
    KlrBasicBlock *break_bb;
} ParserScope;

/* per source file */
typedef struct _ParserState {
    /* module pointer */
    ParserModule *pm;

    /* src file name */
    char *filename;

    /* statements */
    Vector stmts;

    /* klass stmts */
    Vector kls_stmts;

    /* func stmts */
    Vector fn_stmts;

    /* static func stmts */
    Vector static_methods;

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

ParserState *new_parser_state(ParserModule *pm, char *path);
void free_parser_state(ParserState *ps);
void kl_parse_ast(ParserState *ps);
Vector *infer_func_tp(FuncSymbol *fn, Vector *args, ParserState *ps);
FuncSymbol *get_current_function(ParserState *ps);

void init_parser(ParserModule *module);
void fini_parser(ParserModule *module);

void parse_stmt(ParserState *ps, Stmt *stmt);

ParserScope *enter_scope(ParserState *ps, ScopeKind kind, BlockType block, char *name);
void exit_scope(ParserState *ps);
ParserScope *find_loop_scope(ParserState *ps);

TypeSpec *resolve_type(ParserState *ps, TypeSpec *_ts);
int check_type(ParserState *ps, TypeSpec *ts);
int type_spec_compatible(TypeSpec *dst, TypeSpec *src);

void write_to_klc(ParserModule *pm);
PkgSymbol *import_package(ParserModule *pm, char *path);
PkgSymbol *load_module(char *path, ParserModule *pm);
void kl_gen_ir(ParserModule *pm);

void init_const_placeholder(void);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_PARSER_H_ */
