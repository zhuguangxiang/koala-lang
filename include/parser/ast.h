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

/* clang-format off */
#define TYPE_HEAD TypeDesc *desc; Loc loc; HashMap *stbl;
/* clang-format on */

/* type */
typedef struct _type {
    TYPE_HEAD
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

typedef struct _LitExpr {
    EXPR_HEAD
    int which;
#define LIT_EXPR_INT  1
#define LIT_EXPR_FLT  2
#define LIT_EXPR_BOOL 3
#define LIT_EXPR_STR  4
    int len;
    union {
        int64_t ival;
        double fval;
        int bval;
        char *sval;
    };
} LitExpr;

#define expr_set_loc(e, l) (e)->loc = (l)
Expr *expr_from_lit_int(int64_t val);
Expr *expr_from_lit_float(double val);
Expr *expr_from_lit_bool(int val);
Expr *expr_from_lit_str(Buffer *buf);

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
    STMT_MAX_KIND
} StmtKind;

/* clang-format off */
#define STMT_HEAD StmtKind kind; Loc loc;

/* clang-format on */

typedef struct _Stmt {
    STMT_HEAD
} Stmt;

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

#define stmt_set_loc(s, l) (s)->loc = (l)

void stmt_free(Stmt *stmt);
Stmt *stmt_from_var_decl(Ident id, Type *ty, int ro, int pub, Expr *e);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_AST_H_ */
