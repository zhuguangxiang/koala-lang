/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_AST_H_
#define _KOALA_AST_H_

#include "symbol.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "typespec.h"

typedef struct _SimpleFlag {
    int flag;
    Loc loc;
} SimpleFlag;

typedef struct _Annotation {
    Loc id_loc;
    char *ident;
    char *value;
    Vector *types;
} Annotation;

typedef struct _PrefixFlags {
    SimpleFlag pub;
    SimpleFlag st;
    Annotation ann;
} PrefixFlags;

/* identifier */
typedef struct _Ident {
    /* name */
    char *name;
    /* location */
    Loc loc;
    /* where is this ident ? */
    int where;
#define CURRENT_SCOPE  1
#define UP_SCOPE       2
#define EXT_SCOPE      3
#define BLTIN_SCOPE    4
#define IMPORTED_SCOPE 5
    /* scope pointer */
    void *scope;
} Ident;

typedef enum _ExprKind {
    EXPR_UNK_KIND,
    EXPR_ID_KIND,
    EXPR_UNDER_KIND,
    EXPR_LITERAL_KIND,
    EXPR_CONST_TUPLE_KIND,
    EXPR_SELF_KIND,
    EXPR_LIST_KIND,
    EXPR_MAP_KIND,
    EXPR_MAP_ENTRY_KIND,
    EXPR_TUPLE_KIND,
    EXPR_SET_KIND,
    EXPR_ANONY_KIND,
    EXPR_TYPE_KIND,
    EXPR_CALL_KIND,
    EXPR_DOT_KIND,
    EXPR_INDEX_KIND,
    EXPR_SLICE_KIND,
    EXPR_UNARY_KIND,
    EXPR_BINARY_KIND,
    EXPR_KW_KIND,
    EXPR_IS_KIND,
    EXPR_AS_KIND,
    EXPR_IN_KIND,
    EXPR_BANG_KIND,
    EXPR_CONST_PLACEHOLDER,
    EXPR_MAX_KIND,
} ExprKind;

typedef enum _ExprCtx {
    EXPR_CTX_INVALID,
    /* rhs expr */
    EXPR_CTX_LOAD,
    /* lhs expr */
    EXPR_CTX_STORE,
    /* load & store */
    EXPR_CTX_LOAD_STORE,
    /* expr is called */
    EXPR_CTX_CALL,
} ExprCtx;

/* clang-format off */
#define EXPR_HEAD ExprKind kind; Loc loc; ExprCtx ctx; TypeSpec *ts; \
    TypeSpec *expected; Symbol *sym; void *arg; void *tp_args; KlrValue *ir_val;
/* clang-format on */

typedef struct _Expr {
    EXPR_HEAD
} Expr;

void expr_free(Expr *exp);
#define expr_set_loc(e, l) (e)->loc = (l)

typedef struct _LitExpr {
    EXPR_HEAD
    int which;
#define LIT_EXPR_INT  1
#define LIT_EXPR_FLT  2
#define LIT_EXPR_BOOL 3
#define LIT_EXPR_STR  4
#define LIT_EXPR_NONE 5
    int len;
    union {
        struct {
            int sign;
            // non-decimal literals
            int bit_mode;
            char *orginal;
            uint64_t ival;
            __int128 ival_128;
        };
        double fval;
        int bval;
        char *sval;
    };
} LitExpr;

Expr *expr_from_lit_int(char *orginal, int sign, int bit_mode, __int128 val);
Expr *expr_from_lit_float(double val);
Expr *expr_from_lit_bool(int val);
Expr *expr_from_lit_str(Buffer *buf);
Expr *expr_from_lit_none(void);
Expr *expr_from_literal(Literal *lit);
Literal *expr_to_literal(Expr *exp);

typedef struct _ConstTupleExpr {
    EXPR_HEAD
    Vector *vec;
} ConstTupleExpr;

Expr *expr_from_const_tuple(Vector *vec);

typedef struct _ConstPlaceholderExpr {
    EXPR_HEAD
    char *name;
    Literal *lit;
} ConstPlaceholderExpr;

Expr *expr_from_const_placeholder(Buffer *buf);

typedef struct _IdentExpr {
    EXPR_HEAD
    Ident id;
} IdentExpr;

#define IDENT(name, s, l) Ident name = { s, l, 0, NULL }
Expr *expr_from_ident(Ident *id);
Expr *expr_from_under(void);
Expr *expr_from_self(void);

typedef struct _IsExpr {
    EXPR_HEAD
    Expr *exp;
    Loc op_loc;
    TypeSpec *type;
    int result;
} IsExpr;

typedef struct _AsExpr {
    EXPR_HEAD
    Expr *exp;
    Loc op_loc;
    TypeSpec *type;
    int safe_cast;
} AsExpr;

typedef struct _InExpr {
    EXPR_HEAD
    Expr *lhs;
    Loc op_loc;
    Expr *rhs;
} InExpr;

Expr *expr_from_is_expr(Expr *exp, Loc op_loc, TypeSpec *type);
Expr *expr_from_as_expr(Expr *exp, Loc op_loc, TypeSpec *type);
Expr *expr_from_in_expr(Expr *lhs, Loc op_loc, Expr *rhs);

/* unary operator kind */
typedef enum _UnOpKind {
    /* + */
    UNARY_PLUS = 1,
    /* - */
    UNARY_NEG,
    /* ~ */
    UNARY_BIT_NOT,
    /* ! */
    UNARY_NOT
} UnOpKind;

typedef struct _UnaryExpr {
    EXPR_HEAD
    UnOpKind op;
    Loc op_loc;
    Expr *exp;
    // when double negation is optimized,
    // this flag indicates whether to skip this unary expression
    int skip;
} UnaryExpr;

/* binary operator kind */
typedef enum _BiOpKind {
    /* +, -, *, /, %, <<, >> */
    BINARY_ADD = 1,
    BINARY_SUB,
    BINARY_MUL,
    BINARY_DIV,
    BINARY_MOD,
    BINARY_SHL,
    BINARY_SHR,

    /* &, ^, | */
    BINARY_BIT_AND,
    BINARY_BIT_XOR,
    BINARY_BIT_OR,

    /* >, >=, <, <=, ==, != */
    BINARY_GT,
    BINARY_GE,
    BINARY_LT,
    BINARY_LE,
    BINARY_EQ,
    BINARY_NEQ,

    /* &&, || */
    BINARY_AND,
    BINARY_OR,
} BiOpKind;

typedef struct _BinaryExpr {
    EXPR_HEAD
    BiOpKind op;
    Loc op_loc;
    Expr *lhs;
    Expr *rhs;
} BinaryExpr;

Expr *expr_from_unary(UnOpKind kind, Loc op_loc, Expr *e);
Expr *expr_from_binary(BiOpKind op, Loc op_loc, Expr *lhs, Expr *rhs);

typedef struct _KeyWordExpr {
    EXPR_HEAD
    Ident key;
    Expr *value;
} KeyWordExpr;

Expr *expr_from_keyword(Loc loc, Ident key, Expr *value);

typedef struct _TypeExpr {
    EXPR_HEAD
} TypeExpr;

Expr *expr_from_type(TypeSpec *type);

typedef struct _ListExpr {
    EXPR_HEAD
    Vector *vec;
} ListExpr;

Expr *expr_from_list(Vector *vec);

typedef struct _MapExpr {
    EXPR_HEAD
    Vector *vec;
} MapExpr;

Expr *expr_from_map(Vector *vec);

typedef struct _MapEntryExpr {
    EXPR_HEAD
    Expr *key;
    Expr *val;
} MapEntryExpr;

Expr *expr_from_map_entry(Expr *key, Expr *val);

typedef struct _TupleExpr {
    EXPR_HEAD
    Vector *vec;
} TupleExpr;

Expr *expr_from_tuple(Vector *vec);

typedef struct _SetExpr {
    EXPR_HEAD
    Vector *vec;
} SetExpr;

Expr *expr_from_set(Vector *vec);

typedef struct _CallExpr {
    EXPR_HEAD
    Expr *lhs;
    Vector *args;
    int done;
} CallExpr;

Expr *expr_from_call(Expr *lhs, Vector *args);

typedef struct _DotExpr {
    EXPR_HEAD
    Expr *lhs;
    Ident id;
    int opt_or_bang;
#define DOT_NORMAL   0
#define DOT_OPTIONAL 1
#define DOT_BANG     2
} DotExpr;

Expr *expr_from_dot(Expr *lhs, Ident *id, int opt_or_bang);

typedef struct _IndexExpr {
    EXPR_HEAD
    Expr *lhs;
    Vector *vec;
} IndexExpr;

Expr *expr_from_index(Expr *lhs, Vector *vec);

typedef struct _SliceExpr {
    EXPR_HEAD
    Expr *start;
    Expr *end;
    Expr *step;
} SliceExpr;

Expr *expr_from_slice(Expr *start, Expr *stop, Expr *step);

typedef struct _RangeExpr {
    EXPR_HEAD
    Expr *start;
    Expr *stop;
    int inclusive;
} RangeExpr;

Expr *expr_from_range(Expr *start, Expr *stop, int inclusive);

typedef struct _BangExpr {
    EXPR_HEAD
    Expr *exp;
} BangExpr;

Expr *expr_from_bang(Expr *exp);

static inline int expr_is_literal_null(Expr *e)
{
    if (e->kind != EXPR_LITERAL_KIND) return 0;
    return ((LitExpr *)e)->which == LIT_EXPR_NONE;
}

static inline int expr_is_binary(Expr *e) { return e->kind == EXPR_BINARY_KIND; }

typedef enum _StmtKind {
    STMT_UNK_KIND,
    /* import */
    STMT_IMPORT_KIND,
    /* link */
    STMT_LINK_KIND,
    /* let/var */
    STMT_VAR_KIND,
    /* function */
    STMT_FUNC_KIND,
    /* class */
    STMT_CLASS_KIND,
    /* trait */
    STMT_TRAIT_KIND,
    /* return */
    STMT_RETURN_KIND,
    /* assignment */
    STMT_ASSIGN_KIND,
    /* break, continue */
    STMT_BREAK_KIND,
    STMT_CONTINUE_KIND,
    /* expression */
    STMT_EXPR_KIND,
    /* statements */
    STMT_BLOCK_KIND,
    /* if, while, for, match */
    STMT_IF_KIND,
    STMT_WHILE_KIND,
    STMT_FOR_KIND,
    STMT_MATCH_KIND,
    /* if-let */
    STMT_IF_LET_KIND,
    STMT_WHILE_LET_KIND,
    STMT_MAX_KIND
} StmtKind;

/* clang-format off */
#define STMT_HEAD StmtKind kind; Loc loc; PrefixFlags flags;

/* clang-format on */

typedef struct _Stmt {
    STMT_HEAD
} Stmt;

#define stmt_set_loc(s, l) (s)->loc = (l)

#define stmt_set_prefix(s, prefix) ((Stmt *)s)->flags = (prefix)

void stmt_free(Stmt *stmt);

typedef struct _IdentAsIdent {
    Ident id;
    Ident alias_id;
} IdentAsIdent;

typedef struct _ImportStmt {
    STMT_HEAD
    char *path;
    char *alias;
    Vector *names; /* only used for 'from ... import ...' */
} ImportStmt;

Stmt *stmt_from_import(Buffer *buf, char *alias, Vector *names);

typedef struct _LinkStmt {
    STMT_HEAD
    char *path;
} LinkStmt;

Stmt *stmt_from_link(Buffer *buf);

typedef struct _VarDeclStmt {
    STMT_HEAD
    int where;
#define VAR_GLOBAL 1
#define VAR_LOCAL  2
#define VAR_FIELD  3
    int which;
#define VAR_DECL_VAR   0
#define VAR_DECL_LET   1
#define VAR_DECL_CONST 2
    int pub;
    Ident id;
    Symbol *sym;
    TypeSpec *type;
    Expr *exp;
} VarDeclStmt;

Stmt *stmt_from_var_decl(Ident id, TypeSpec *ty, int which, Expr *e);
#define var_set_where(stmt, _where) ((VarDeclStmt *)stmt)->where = _where;

typedef struct _TypeParamDecl {
    Loc loc;
    Ident id;
    int which;
#define TP_DECL_NORMAL 0
#define TP_DECL_CONST  1
#define TP_DECL_INFER  2
    union {
        Vector *bound;
        TypeSpec *const_type;
    };
} TypeParamDecl;

TypeParamDecl *type_param_new(Loc loc, Ident id, Vector *bound);
TypeParamDecl *const_type_param_new(Loc loc, Ident id, TypeSpec *type);
TypeParamDecl *infer_type_param_new(Loc loc, Ident id);

typedef struct _ParamDecl {
    Loc loc;
    Ident id;
    int va_arg;
    TypeSpec *type;
    Expr *value;
} ParamDecl;

ParamDecl *param_new(Loc loc, Ident id, TypeSpec *type, Expr *value);

typedef struct _Argument {
    Loc loc;
    Ident id;
    Expr *value;
} Argument;

typedef struct _FuncDeclStmt {
    STMT_HEAD
    Ident id;
    Symbol *sym;
    Vector *tps;
    Vector *args;
    TypeSpec *ret;
    Vector *body;
    void *data;
} FuncDeclStmt;

Stmt *stmt_from_func_decl(Ident id, Vector *args, TypeSpec *ret, Vector *tps);

typedef enum _AssignOpKind {
    OP_ASSIGN = 1,
    OP_PLUS_ASSIGN,
    OP_MINUS_ASSIGN,
    OP_MULT_ASSIGN,
    OP_DIV_ASSIGN,
    OP_MOD_ASSIGN,
    OP_AND_ASSIGN,
    OP_OR_ASSIGN,
    OP_XOR_ASSIGN,
    OP_SHL_ASSIGN,
    OP_SHR_ASSIGN,
} AssignOpKind;

typedef struct _AssignStmt {
    STMT_HEAD
    AssignOpKind op;
    Loc op_loc;
    Expr *lhs;
    Expr *rhs;
    /* only used for inplace assignment, e.g. a += b -> a = a + b, bin_exp is a + b */
    Expr *bin_exp;
    // function of __iadd__, __isub__ etc.
    FuncSymbol *fn_sym;
} AssignStmt;

Stmt *stmt_from_assignment(AssignOpKind op, Expr *lhs, Expr *rhs);

typedef struct _BlockStmt {
    STMT_HEAD
    Vector *stmts;
    int has_terminal;
} BlockStmt;

Stmt *stmt_from_block(Vector *stmts);

typedef struct _IfStmt {
    STMT_HEAD
    Expr *cond;
    Vector *block;
    Stmt *_else;
} IfStmt;

Stmt *stmt_from_if(Expr *cond, Vector *block, Stmt *_else);

typedef struct {
    STMT_HEAD
    Symbol *sym;
    Ident id;
    Expr *cond;
    Vector *block;
    Stmt *_else;
} IfLetStmt;

Stmt *stmt_from_if_let(Ident *id, Expr *exp, Vector *block, Stmt *_else);

typedef struct _ExprStmt {
    STMT_HEAD
    Expr *exp;
} ExprStmt, RetStmt;

Stmt *stmt_from_expr(Expr *exp);

typedef struct _WhileStmt {
    STMT_HEAD
    Expr *cond;
    Vector *block;
} WhileStmt;

Stmt *stmt_from_while(Expr *cond, Vector *block);

typedef struct _WhileLetStmt {
    STMT_HEAD
    Symbol *sym;
    Ident id;
    Expr *cond;
    Vector *block;
} WhileLetStmt;

Stmt *stmt_from_while_let(Ident *id, Expr *cond, Vector *block);

typedef struct _ForStmt {
    STMT_HEAD
    Vector *ids;
    Expr *iterable;
    Vector *block;
    Vector sym_ids;
} ForStmt;

Stmt *stmt_from_for(Vector *ids, Expr *iterable, Vector *block);

typedef struct _KlassStmt {
    STMT_HEAD
    Ident id;
    TypeSpec *ts;
    Symbol *sym;
    Vector *tps;
    Vector *bases;
    Vector *stmts;
} KlassDeclStmt;

typedef struct _IdentType {
    Ident id;
    TypeSpec *ts;
} IdentType;

Stmt *stmt_from_type(StmtKind kind, IdentType name, Vector *tps, Vector *bases, Vector *stmts);

#define stmt_from_klass(name, tps, bases, stmts) \
    stmt_from_type(STMT_CLASS_KIND, name, tps, bases, stmts)
#define stmt_from_trait(name, tps, bases, stmts) \
    stmt_from_type(STMT_TRAIT_KIND, name, tps, bases, stmts)

Stmt *stmt_from_return(Expr *exp);
Stmt *stmt_from_continue(void);
Stmt *stmt_from_break(void);

static inline bool has_specialized_meta(Stmt *stmt)
{
    Annotation *ann = &stmt->flags.ann;
    return ann->ident && !strcmp(ann->ident, "specialized");
}

static inline Vector *get_specialized_types_list(Stmt *stmt)
{
    Annotation *ann = &stmt->flags.ann;
    return ann->types;
}

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_AST_H_ */
