/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_AST_H_
#define _KOALA_AST_H_

#include "symbol.h"
#include "util/typedesc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ExprType ExprType;
typedef enum _ExprKind ExprKind;
typedef enum _ExprOpKind ExprOpKind;
typedef struct _Loc Loc;
typedef struct _Expr Expr;
typedef struct _ConstExpr ConstExpr;
typedef struct _IdExpr IdExpr;
typedef enum _UnOpKind UnOpKind;
typedef struct _UnaryExpr UnaryExpr;
typedef enum _BinOpkind BinOpKind;
typedef struct _BinaryExpr BinaryExpr;
typedef enum _StmtKind StmtKind;
typedef struct _Stmt Stmt;
typedef struct _VarDeclStmt VarDeclStmt;
typedef enum _AssignOpKind AssignOpKind;
typedef struct _AssignStmt AssignStmt;
typedef struct _ReturnStmt ReturnStmt;
typedef struct _BlockStmt BlockStmt;
typedef struct _ExprStmt ExprStmt;
typedef struct _FuncDeclStmt FuncDeclStmt;
typedef struct _Ident Ident;
typedef struct _ExprKlassType ExprKlassType;
typedef struct _ExprArrayType ExprArrayType;
typedef struct _ExprMapType ExprMapType;
typedef struct _ExprTupleType ExprTupleType;
typedef struct _ExprVaListType ExprVaListType;
typedef struct _ExprProtoType ExprProtoType;

struct _Loc {
    uint16 row;
    uint16 col;
};

/* clang-format off */
#define EXPR_TYPE_HEAD TypeDesc *ty; Loc loc; int ref;
/* clang-format on */

/* type */
struct _ExprType {
    EXPR_TYPE_HEAD
};

/* identifier */
struct _Ident {
    char *str;
    Loc loc;
    Symbol *sym;
};

struct _ExprKlassType {
    EXPR_TYPE_HEAD
    Ident pkg;
    Ident id;
    Vector *args;
};

struct _ExprArrayType {
    EXPR_TYPE_HEAD
    ExprType *sub;
};

struct _ExprMapType {
    EXPR_TYPE_HEAD
    ExprType *kty;
    ExprType *vty;
};

struct _ExprTupleType {
    EXPR_TYPE_HEAD
    Vector *subs;
};

struct _ExprProtoType {
    EXPR_TYPE_HEAD
    ExprType *ret;
    Vector *params;
};

#define expr_type_set_loc(et, l) (et)->loc = (l)

ExprType *expr_type_ref(TypeDesc *ty);
ExprType *expr_type_any(void);
ExprType *expr_type_int8(void);
ExprType *expr_type_int16(void);
ExprType *expr_type_int32(void);
ExprType *expr_type_int64(void);
ExprType *expr_type_float32(void);
ExprType *expr_type_float64(void);
ExprType *expr_type_bool(void);
ExprType *expr_type_char(void);
ExprType *expr_type_str(void);
ExprType *expr_type_klass(Ident *pkg, Ident *name, Vector *args);
ExprType *expr_type_array(ExprType *sub);
ExprType *expr_type_map(ExprType *kty, ExprType *vty);
ExprType *expr_type_tuple(Vector *subs);
ExprType *expr_type_proto(ExprType *ret, Vector *params);

void free_expr_type(ExprType *ty);

enum _ExprKind {
    /* nil, self and super */
    EXPR_NIL_KIND = 1,
    EXPR_SELF_KIND,
    EXPR_SUPER_KIND,
    /* int, float, bool, char, string */
    EXPR_INT_KIND,
    EXPR_FLOAT_KIND,
    EXPR_BOOL_KIND,
    EXPR_CHAR_KIND,
    EXPR_STR_KIND,
    /* ident and underscore(_) */
    EXPR_ID_KIND,
    EXPR_UNDER_KIND,
    /* unary, binary */
    EXPR_UNARY_KIND,
    EXPR_BINARY_KIND,
    /* dot, [], (), [:] access */
    EXPR_ATTRIBUTE_KIND,
    EXPR_SUBSCRIPT_KIND,
    EXPR_CALL_KIND,
    EXPR_SLICE_KIND,
    /* dot tuple */
    EXPR_DOT_TUPLE_KIND,
    /* tuple, array, map, anonymous */
    EXPR_TUPLE_KIND,
    EXPR_ARRAY_KIND,
    EXPR_MAP_KIND,
    EXPR_ANONY_KIND,
    /* is, as */
    EXPR_IS_KIND,
    EXPR_AS_KIND,
    /* range object */
    EXPR_RANGE_KIND,
    /* expr type */
    EXPR_TYPE_KIND,
    EXPR_MAX_KIND
};

enum _ExprOpKind {
    EXPR_OP_INVALID,
    EXPR_OP_RO,
    EXPR_OP_RW,
    EXPR_OP_CALL,
};

#define EXPR_HEAD   \
    ExprKind kind;  \
    ExprType *type; \
    ExprOpKind eop; \
    Expr *right;    \
    Loc loc;

struct _Expr {
    EXPR_HEAD
};

struct _ConstExpr {
    EXPR_HEAD
    union {
        int64 ival;
        double fval;
        int cval;
        int bval;
        char *sval;
    };
};

struct _IdExpr {
    EXPR_HEAD
    Ident id;
    // where is the identifier in?
    int where;
#define CURRENT_SCOPE 1
#define UP_SCOPE      2
#define EXT_SCOPE     3
#define EXTDOT_SCOPE  4
#define AUTO_IMPORTED 5
#define ID_IN_ENUM    6
#define ID_LABEL      7
    /* scope pointer */
    void *scope;
};

/* unary operator kind */
enum _UnOpKind {
    /* + */
    UNARY_PLUS = 1,
    /* - */
    UNARY_NEG,
    /* ~ */
    UNARY_BIT_NOT,
    /* ! */
    UNARY_NOT
};

struct _UnaryExpr {
    EXPR_HEAD
    UnOpKind uop;
    Loc oploc;
    Expr *exp;
};

/* binary operator kind */
enum _BinOpkind {
    /* +, -, *, /, %, <<, >> */
    BINARY_ADD = 1,
    BINARY_SUB,
    BINARY_MULT,
    BINARY_DIV,
    BINARY_MOD,
    BINARY_LSHIFT,
    BINARY_RSHIFT,

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
};

struct _BinaryExpr {
    EXPR_HEAD
    BinOpKind bop;
    Loc oploc;
    Expr *lexp;
    Expr *rexp;
};

#define expr_set_loc(e, l) (e)->loc = (l)

Expr *expr_from_nil(void);
Expr *expr_from_self(void);
Expr *expr_from_super(void);
Expr *expr_from_under(void);

Expr *expr_from_int64(int64 val);
Expr *expr_from_float64(double val);
Expr *expr_from_bool(int val);
Expr *expr_from_str(char *val);
Expr *expr_from_char(int val);

Expr *expr_from_ident(Ident *val, ExprType *ty);
Expr *expr_from_unary(UnOpKind ukind, Loc oploc, Expr *e);
Expr *expr_from_binary(BinOpKind op, Loc oploc, Expr *le, Expr *re);

Expr *expr_from_type(ExprType *ty);

void free_expr(Expr *e);

enum _StmtKind {
    /* import */
    STMT_IMPORT_KIND = 1,
    /* let */
    STMT_LET_KIND,
    /* variable */
    STMT_VAR_KIND,
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
    /* ifunc */
    STMT_IFUNC_KIND,
    /* class */
    STMT_CLASS_KIND,
    /* trait */
    STMT_TRAIT_KIND,
    /* enum */
    STMT_ENUM_KIND,
    /* break, continue */
    STMT_BREAK_KIND,
    STMT_CONTINUE_KIND,
    /* if, while, for, match */
    STMT_IF_KIND,
    STMT_WHILE_KIND,
    STMT_FOR_KIND,
    STMT_MATCH_KIND,
    STMT_MAX_KIND
};

/* clang-format off */

#define STMT_HEAD StmtKind kind; Loc loc;

/* clang-format on */

struct _Stmt {
    STMT_HEAD
};

#define stmt_set_loc(s, l) (s)->loc = (l)

struct _VarDeclStmt {
    STMT_HEAD
    int where;
#define VAR_DECL_GLOBAL 1
#define VAR_DECL_LOCAL  2
#define VAR_DECL_FIELD  3
    Ident id;
    ExprType *type;
    Expr *exp;
};

struct _FuncDeclStmt {
    STMT_HEAD
    Ident id;
    Vector *param_types;
    Vector *args;
    ExprType *ret;
    Vector *body;
};

enum _AssignOpKind {
    OP_ASSIGN = 1,
    OP_PLUS_ASSIGN,
    OP_MINUS_ASSIGN,
    OP_MULT_ASSIGN,
    OP_DIV_ASSIGN,
    OP_MOD_ASSIGN,
    OP_LSHIFT_ASSIGN,
    OP_RSHIFT_ASSIGN,
    OP_AND_ASSIGN,
    OP_OR_ASSIGN,
    OP_XOR_ASSIGN,
};

struct _AssignStmt {
    STMT_HEAD
    Loc oploc;
    Expr *lhs;
    Expr *rhs;
};

struct _ReturnStmt {
    STMT_HEAD
    Expr *exp;
};

struct _BlockStmt {
    STMT_HEAD
    Vector *stmts;
};

struct _ExprStmt {
    STMT_HEAD
    Expr *exp;
};

Stmt *stmt_from_letdecl(Ident *name, ExprType *ty, Expr *e);
Stmt *stmt_from_vardecl(Ident *name, ExprType *ty, Expr *e);
Stmt *stmt_from_assign(AssignOpKind op, Loc oploc, Expr *lhs, Expr *rhs);
Stmt *stmt_from_ret(Expr *ret);
Stmt *stmt_from_break(void);
Stmt *stmt_from_continue(void);
Stmt *stmt_from_block(Vector *stmts);
Stmt *stmt_from_expr(Expr *e);
Stmt *stmt_from_funcdecl(Ident *name, Vector *param_types, Vector *args,
                         ExprType *ret, Vector *body);

void free_stmt(Stmt *s);

typedef struct {
    Ident id;
    Vector *args;
} IdentArgs;

typedef struct {
    Ident id;
    ExprType *type;
} IdentType;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_AST_H_ */
