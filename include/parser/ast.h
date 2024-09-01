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
Type *optional_type(Type *ty);
Type *array_type(Type *sub);

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
    EXPR_ANONY_KIND,
    EXPR_TYPE_KIND,
    EXPR_CALL_KIND,
    EXPR_ATTR_KIND,
    EXPR_TUPLE_GET_KIND,
    EXPR_INDEX_KIND,
    EXPR_UNARY_KIND,
    EXPR_BINARY_KIND,
    EXPR_RANGE_KIND,
    EXPR_IS_KIND,
    EXPR_AS_KIND,
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
