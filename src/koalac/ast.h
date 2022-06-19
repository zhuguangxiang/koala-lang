/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

#ifndef _KOALA_AST_H_
#define _KOALA_AST_H_

#include "common/common.h"
#include "symbol.h"
#include "typedesc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _ExprKind ExprKind;
typedef enum _ExprOpKind ExprOpKind;
typedef struct _Loc Loc;
typedef struct _ExprType ExprType;
typedef struct _Ident Ident;
typedef struct _Expr Expr;
typedef struct _ErrorExpr ErrorExpr;
typedef struct _LiteralExpr LiteralExpr;
typedef struct _SizeOfExpr SizeOfExpr;
typedef struct _IdExpr IdExpr;
typedef enum _UnOpKind UnOpKind;
typedef struct _UnaryExpr UnaryExpr;
typedef enum _BinOpkind BinOpKind;
typedef struct _BinaryExpr BinaryExpr;
typedef enum _RangeOpKind RangeOpKind;
typedef struct _RangeExpr RangeExpr;
typedef struct _IsAsExpr IsAsExpr;
typedef struct _PointerExpr PointerExpr;
typedef struct _CallExpr CallExpr;
typedef struct _AttrExpr AttrExpr;
typedef struct _IndexExpr IndexExpr;
typedef struct _TypeParamsExpr TypeParamsExpr;
typedef struct _TupleAccessExpr TupleAccessExpr;
typedef enum _StmtKind StmtKind;
typedef struct _Stmt Stmt;
typedef struct _VarDeclStmt VarDeclStmt;
typedef enum _AssignOpKind AssignOpKind;
typedef struct _AssignStmt AssignStmt;
typedef struct _ReturnStmt ReturnStmt;
typedef struct _BlockStmt BlockStmt;
typedef struct _ExprStmt ExprStmt;
typedef struct _FuncDeclStmt FuncDeclStmt;
typedef struct _StructDeclStmt StructDeclStmt;
typedef struct _Ident Ident;
typedef struct _ExprKlassType ExprKlassType;
typedef struct _ExprArrayType ExprArrayType;
typedef struct _ExprMapType ExprMapType;
typedef struct _ExprTupleType ExprTupleType;
typedef struct _ExprVaListType ExprVaListType;
typedef struct _ExprProtoType ExprProtoType;

/* location */
struct _Loc {
    short row;
    short col;
};

/* type */
struct _ExprType {
    Loc loc;
    Loc loc2;
    Loc loc3;
    TypeDesc *ty;
};

/* identifier */
struct _Ident {
    char *str;
    Loc loc;
    Symbol *sym;
};

enum _ExprKind {
    EXPR_ERROR_KIND,
    /* null, self and super */
    EXPR_NULL_KIND,
    EXPR_SELF_KIND,
    EXPR_SUPER_KIND,
    /* literal: int, float, bool, char, string */
    EXPR_LITERAL_KIND,
    /* sizeof */
    EXPR_SIZEOF_KIND,
    /* ident and underscore(_) */
    EXPR_ID_KIND,
    EXPR_UNDER_KIND,
    /* unary, binary */
    EXPR_UNARY_KIND,
    EXPR_BINARY_KIND,
    /* dot, [], () */
    EXPR_ATTR_KIND,
    EXPR_INDEX_KIND,
    EXPR_CALL_KIND,
    /* dot tuple */
    EXPR_DOT_TUPLE_KIND,
    /* tuple, array, map, anonymous */
    EXPR_TUPLE_KIND,
    EXPR_ARRAY_KIND,
    EXPR_MAP_KIND,
    EXPR_ANONY_KIND,
    /* pointer */
    EXPR_POINTER_KIND,
    /* type parameters */
    EXPR_TYPE_PARAMS_KIND,
    /* is, as */
    EXPR_IS_KIND,
    EXPR_AS_KIND,
    /* range object */
    EXPR_RANGE_KIND,
    EXPR_MAX_KIND
};

enum _ExprOpKind {
    EXPR_OP_INVALID,
    EXPR_OP_RO,
    EXPR_OP_RW,
    EXPR_OP_CALL,
};

#define EXPR_HEAD  \
    ExprKind kind; \
    ExprOpKind op; \
    TypeDesc *ty;  \
    Expr *right;   \
    Expr **parent; \
    Loc loc;

struct _Expr {
    EXPR_HEAD
};

struct _ErrorExpr {
    EXPR_HEAD
    Loc inv_loc;
};

struct _LiteralExpr {
    EXPR_HEAD
    union {
        int8_t i8val;
        int16_t i16val;
        int32_t i32val;
        int64_t i64val;
        uint8_t u8val;
        uint16_t u16val;
        uint32_t u32val;
        uint64_t u64val;
        float f32val;
        double f64val;
        int cval;
        int bval;
        char *sval;
    };
};

struct _SizeOfExpr {
    EXPR_HEAD
    ExprType sub_ty;
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
    Loc op_loc;
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
    BINARY_NE,

    /* &&, || */
    BINARY_AND,
    BINARY_OR,
};

struct _BinaryExpr {
    EXPR_HEAD
    BinOpKind bop;
    Loc op_loc;
    Expr *lexp;
    Expr *rexp;
};

enum _RangeOpKind {
    RANGE_DOTDOTDOT,
    RANGE_DOTDOTLESS,
};

struct _RangeExpr {
    EXPR_HEAD
    Expr *lhs;
    Expr *rhs;
    RangeOpKind range_op;
    Loc op_loc;
};

struct _IsAsExpr {
    EXPR_HEAD
    Expr *exp;
    ExprType sub_ty;
};

struct _PointerExpr {
    EXPR_HEAD
    ExprType sub_ty;
};

struct _CallExpr {
    EXPR_HEAD
    Expr *lhs;
    Vector *arguments;
};

struct _AttrExpr {
    EXPR_HEAD
    Expr *lhs;
    Expr *attr;
};

struct _IndexExpr {
    EXPR_HEAD
    Expr *lhs;
    Expr *arg;
};

struct _TupleAccessExpr {
    EXPR_HEAD
    Expr *lhs;
    int index;
};

struct _TypeParamsExpr {
    EXPR_HEAD
    Expr *lhs;
    Vector *type_params;
};

Expr *expr_from_error(void);
Expr *expr_from_null(void);
Expr *expr_from_self(void);
Expr *expr_from_super(void);
Expr *expr_from_under(void);

Expr *expr_from_int8(int8_t val);
Expr *expr_from_int16(int16_t val);
Expr *expr_from_int32(int32_t val);
Expr *expr_from_int64(int64_t val);
Expr *expr_from_uint8(uint8_t val);
Expr *expr_from_uint16(uint16_t val);
Expr *expr_from_uint32(uint32_t val);
Expr *expr_from_uint64(uint64_t val);
Expr *expr_from_float32(float val);
Expr *expr_from_float64(double val);
Expr *expr_from_bool(int val);
Expr *expr_from_str(char *val);
Expr *expr_from_char(int val);
Expr *expr_from_sizeof(ExprType ty);
Expr *expr_from_pointer(void);

#if INTPTR_MAX == INT64_MAX
#define expr_from_int(v)   expr_from_int64(v)
#define expr_from_float(v) expr_from_float64(v)
#elif INTPTR_MAX == INT32_MAX
#define expr_from_int(v)   expr_from_int32(v)
#define expr_from_float(v) expr_from_float32(v)
#else
#error "Must be either 32 or 64 bits".
#endif

Expr *expr_from_range(Expr *lhs, RangeOpKind op, Loc op_loc, Expr *rhs);
Expr *expr_from_is(Expr *exp, ExprType ty);
Expr *expr_from_as(Expr *exp, ExprType ty);
Expr *expr_from_ident(Ident name);
Expr *expr_from_unary(UnOpKind op, Loc op_loc, Expr *e);
Expr *expr_from_binary(BinOpKind op, Loc op_loc, Expr *lhs, Expr *rhs);
Expr *expr_from_call(Expr *lhs, Vector *arguments);
Expr *expr_from_attr(Expr *lhs, Expr *attr);
Expr *expr_from_index(Expr *lhs, Expr *arg);
Expr *expr_from_type_params(Expr *lhs, Vector *type_params);
Expr *expr_from_tuple_access(Expr *lhs, int index);

void free_expr(Expr *e);

enum _StmtKind {
    STMT_UNK_KIND,
    /* import */
    STMT_IMPORT_KIND,
    /* type alias */
    STMT_TYPE_ALIAS_KIND,
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
    /* struct */
    STMT_STRUCT_KIND,
    /* class */
    STMT_CLASS_KIND,
    /* interface */
    STMT_INTERFACE_KIND,
    /* enum */
    STMT_ENUM_KIND,
    /* break, continue */
    STMT_BREAK_KIND,
    STMT_CONTINUE_KIND,
    /* if, while, for, switch */
    STMT_IF_KIND,
    STMT_WHILE_KIND,
    STMT_FOR_KIND,
    STMT_SWITCH_KIND,
    STMT_MAX_KIND
};

/* clang-format off */

#define STMT_HEAD StmtKind kind; Loc loc;

/* clang-format on */

struct _Stmt {
    STMT_HEAD
};

struct _VarDeclStmt {
    STMT_HEAD
    Ident id;
    ExprType ty;
    Expr *exp;
    int where;
#define VAR_DECL_GLOBAL 1
#define VAR_DECL_LOCAL  2
#define VAR_DECL_FIELD  3
    int pub;
};

struct _FuncDeclStmt {
    STMT_HEAD
    Ident id;
    Vector *type_params;
    Vector *args;
    ExprType ret;
    Vector *body;
    int pub;
};

enum _AssignOpKind {
    OP_ASSIGN = 1,
    OP_PLUS_ASSIGN,
    OP_MINUS_ASSIGN,
    OP_MULT_ASSIGN,
    OP_DIV_ASSIGN,
    OP_MOD_ASSIGN,
    OP_SHL_ASSIGN,
    OP_SHR_ASSIGN,
    OP_AND_ASSIGN,
    OP_OR_ASSIGN,
    OP_XOR_ASSIGN,
};

struct _AssignStmt {
    STMT_HEAD
    AssignOpKind op;
    Loc op_loc;
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

struct _StructDeclStmt {
    STMT_HEAD
    Ident name;
    int c_struct;
    Vector *type_params;
    Vector *bases;
    Vector *fields;
    Vector *functions;
};

struct _ClassDeclStmt {
    STMT_HEAD
    Ident name;
    int has_abstract;
    Vector *type_params;
    Vector *bases;
    Vector *members;
};

struct _IntfDeclStmt {
    STMT_HEAD
    Ident name;
    Vector *type_params;
    Vector *bases;
    Vector *protos;
};

Stmt *stmt_from_type_decl();
Stmt *stmt_from_let_decl(Ident name, ExprType ty, Expr *e);
Stmt *stmt_from_var_decl(Ident name, ExprType ty, Expr *e);
Stmt *stmt_from_assign(AssignOpKind op, Loc op_loc, Expr *lhs, Expr *rhs);
Stmt *stmt_from_ret(Expr *ret);
Stmt *stmt_from_break(void);
Stmt *stmt_from_continue(void);
Stmt *stmt_from_block(Vector *stmts);
Stmt *stmt_from_expr(Expr *e);
Stmt *stmt_from_func_decl(Ident name, Vector *type_params, Vector *args,
                          ExprType *ret, Vector *body);
Stmt *stmt_from_struct_decl(Ident name, Vector *type_params, Vector *bases,
                            Vector *members, int c_struct);
Stmt *stmt_from_class_decl(Ident name, Vector *type_params, Vector *bases,
                           Vector *members);
Stmt *stmt_from_intf_decl(Ident name, Vector *type_params, Vector *bases,
                          Vector *protos);
Stmt *stmt_from_enum_decl();

void free_stmt(Stmt *s);

typedef struct {
    Ident id;
    Vector *type_params;
    Loc loc;
    int error;
} Ident_TypeParams;

typedef struct {
    Ident id;
    ExprType type;
} Ident_Type;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_AST_H_ */
