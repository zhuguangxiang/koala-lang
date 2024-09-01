/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_AST_H_
#define _KOALA_AST_H_

#include "symbol.h"
#include "typedesc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* location */
typedef struct _Loc {
    int line;
    int col;
    int last_line;
    int last_col;
} Loc;

/* identifier */
typedef struct _Ident {
    /* name */
    char *name;
    /* location */
    Loc loc;
    /* symbol */
    Symbol *sym;
    /* where is this ident ? */
    int where;
    /* scope pointer */
    void *scope;
} Ident;

/* type */
typedef struct _Type {
    TypeDesc *desc;
    Loc loc;
    Vector *subs;
} Type;

Type *int8_type(void);
Type *int16_type(void);
Type *int32_type(void);
Type *int64_type(void);
Type *float32_type(void);
Type *float64_type(void);
Type *bool_type(void);
Type *str_type(void);
Type *object_type(void);
Type *char_type(void);
Type *bytes_type(void);
Type *type_type(void);
Type *range_type(void);
Type *optional_type(Type *ty);
Type *array_type(Type *sub);
Type *map_type(Type *key, Type *val);
Type *tuple_type(Vector *vec);
Type *set_type(Type *sub);
Type *klass_type(Ident *mod, Ident *id, Vector *vec);
void free_type(Type *ty);
#define type_set_loc(ty, _loc) (ty)->loc = (_loc)

typedef enum _ExprKind {
    EXPR_UNK_KIND,
    EXPR_ID_KIND,
    EXPR_UNDER_KIND,
    EXPR_LITERAL_KIND,
    EXPR_SELF_KIND,
    EXPR_SUPER_KIND,
    EXPR_ARRAY_KIND,
    EXPR_MAP_KIND,
    EXPR_MAP_ENTRY_KIND,
    EXPR_TUPLE_KIND,
    EXPR_SET_KIND,
    EXPR_ANONY_KIND,
    EXPR_TYPE_KIND,
    EXPR_CALL_KIND,
    EXPR_DOT_KIND,
    EXPR_INDEX_KIND,
    EXPR_INDEX_SLICE_KIND,
    EXPR_SLICE_KIND,
    EXPR_UNARY_KIND,
    EXPR_BINARY_KIND,
    EXPR_RANGE_KIND,
    EXPR_IS_KIND,
    EXPR_AS_KIND,
    EXPR_IN_KIND,
    EXPR_MAX_KIND,
} ExprKind;

typedef enum _ExprCtx {
    EXPR_CTX_INVALID,
    /* rhs expr */
    EXPR_CTX_LOAD,
    /* lhs expr */
    EXPR_CTX_STORE,
    /* expr is called */
    EXPR_CTX_CALL,
} ExprCtx;

/* clang-format off */
#define EXPR_HEAD ExprKind kind; Loc loc; ExprCtx ctx; TypeDesc *desc; \
    TypeDesc *expected; HashMap *stbl;
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
        int64_t ival;
        double fval;
        int bval;
        char *sval;
    };
} LitExpr;

Expr *expr_from_lit_int(int64_t val);
Expr *expr_from_lit_float(double val);
Expr *expr_from_lit_bool(int val);
Expr *expr_from_lit_str(Buffer *buf);
Expr *expr_from_lit_none(void);

typedef struct _IdentExpr {
    EXPR_HEAD
    Ident id;
} IdentExpr;

#define IDENT(name, s, l) Ident name = { s, l, NULL, 0, NULL }
Expr *expr_from_ident(Ident *id);
Expr *expr_from_under(void);
Expr *expr_from_self(void);
Expr *expr_from_super(void);

typedef struct _IsExpr {
    EXPR_HEAD
    Expr *exp;
    Loc op_loc;
    Type *type;
} IsExpr;

typedef struct _AsExpr {
    EXPR_HEAD
    Expr *exp;
    Loc op_loc;
    Type *type;
} AsExpr;

typedef struct _InExpr {
    EXPR_HEAD
    Expr *lhs;
    Loc op_loc;
    Expr *rhs;
} InExpr;

Expr *expr_from_is_expr(Expr *exp, Loc op_loc, Type *type);
Expr *expr_from_as_expr(Expr *exp, Loc op_loc, Type *type);
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
    BINARY_USHR,

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
    BINARY_NE,

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

typedef struct _TypeExpr {
    EXPR_HEAD
    Type *type;
} TypeExpr;

Expr *expr_from_type(Type *type);

typedef struct _ArrayExpr {
    EXPR_HEAD
    Vector *vec;
} ArrayExpr;

Expr *expr_from_array(Vector *vec);

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
} CallExpr;

Expr *expr_from_call(Expr *lhs, Vector *args);

typedef struct _DotExpr {
    EXPR_HEAD
    Expr *lhs;
    Ident id;
} DotExpr;

Expr *expr_from_dot(Expr *lhs, Ident *id);

typedef struct _IndexExpr {
    EXPR_HEAD
    Expr *lhs;
    Vector *vec;
} IndexExpr;

Expr *expr_from_index(Expr *lhs, Vector *vec);

typedef struct _IndexSliceExpr {
    EXPR_HEAD
    Expr *lhs;
    Expr *slice;
} IndexSliceExpr;

Expr *expr_from_index_slice(Expr *lhs, Expr *slice);

typedef struct _SliceExpr {
    EXPR_HEAD
    Expr *start;
    Expr *stop;
} SliceExpr;

Expr *expr_from_slice(Expr *start, Expr *stop);

typedef enum _StmtKind {
    STMT_UNK_KIND,
    /* import */
    STMT_IMPORT_KIND,
    /* let/var */
    STMT_VAR_KIND,
    /* tuple var */
    STMT_TUPLE_VAR_KIND,
    /* assignment */
    STMT_ASSIGN_KIND,
    /* function */
    STMT_FUNC_KIND,
    /* return */
    STMT_RETURN_KIND,
    /* expression */
    STMT_EXPR_KIND,
    /* statements */
    STMT_BLOCK_KIND,
    /* class */
    STMT_CLASS_KIND,
    /* trait */
    STMT_TRAIT_KIND,
    /* break, continue */
    STMT_BREAK_KIND,
    STMT_CONTINUE_KIND,
    /* if, while, for, match */
    STMT_IF_KIND,
    STMT_WHILE_KIND,
    STMT_FOR_KIND,
    STMT_MATCH_KIND,
    /* if-let and guard-let */
    STMT_IF_LET_KIND,
    STMT_GUARD_LET_KIND,
    STMT_MAX_KIND
} StmtKind;

/* clang-format off */
#define STMT_HEAD StmtKind kind; Loc loc;

/* clang-format on */

typedef struct _Stmt {
    STMT_HEAD
} Stmt;

#define stmt_set_loc(s, l) (s)->loc = (l)
void stmt_free(Stmt *stmt);

typedef struct _VarDeclStmt {
    STMT_HEAD
    int where;
#define VAR_GLOBAL 1
#define VAR_LOCAL  2
#define VAR_FIELD  3
    int ro;
    int pub;
    Ident id;
    Type *type;
    Expr *exp;
} VarDeclStmt;

Stmt *stmt_from_var_decl(Ident id, Type *ty, int ro, Expr *e);

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
    OP_USHR_ASSIGN,
} AssignOpKind;

typedef struct _AssignStmt {
    STMT_HEAD
    AssignOpKind op;
    Loc op_loc;
    Expr *lhs;
    Expr *rhs;
} AssignStmt;

Stmt *stmt_from_assignment(AssignOpKind op, Expr *lhs, Expr *rhs);

typedef struct _BlockStmt {
    STMT_HEAD
    Vector *stmts;
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
    Ident id;
    Expr *exp;
    Vector *block;
} IfLetStmt, GuardLetStmt;

Stmt *stmt_from_if_let(Ident *id, Expr *exp, Vector *block);
Stmt *stmt_from_guard_let(Ident *id, Expr *exp, Vector *block);

typedef struct _ExprStmt {
    STMT_HEAD
    Expr *exp;
} ExprStmt;

Stmt *stmt_from_expr(Expr *exp);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_AST_H_ */
