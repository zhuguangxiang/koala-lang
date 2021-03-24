/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _KOALA_AST_H_
#define _KOALA_AST_H_

#include "klvm_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/* expression kind */
typedef enum _ExprKind {
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
    /* unary, binary, ternary op */
    EXPR_UNARY_KIND,
    EXPR_BINARY_KIND,
    EXPR_TERNARY_KIND,
    /* dot, [], (), [:] access */
    EXPR_ATTRIBUTE_KIND,
    EXPR_SUBSCRIPT_KIND,
    EXPR_CALL_KIND,
    EXPR_SLICE_KIND,
    /* type arguments */
    EXPR_DOT_TYPEARGS_KIND,
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
    /* binary match */
    EXPR_BINARY_MATCH_KIND,
    EXPR_KIND_MAX
} ExprKind;

/* expression context */
typedef enum _ExprCtx {
    EXPR_CTX_INVALID,
    /* left or right value indicator */
    EXPR_CTX_LOAD,
    EXPR_CTX_STORE,
    /* inplace assign */
    EXPR_CTX_INPLACE,
    /* call or load function indicator */
    EXPR_CTX_CALL_FUNC,
    EXPR_CTX_LOAD_FUNC,
} ExprCtx;

#define EXPR_HEAD        \
    ExprKind kind;       \
    KLVMTypeRef type;    \
    ExprCtx ctx;         \
    struct _Expr *right; \
    int row;             \
    int col;

typedef struct _Expr {
    EXPR_HEAD
} Expr, *ExprRef;

typedef struct _ExprK {
    EXPR_HEAD
    union {
        int64_t ival;
        double fval;
        int cval;
        int bval;
        char *sval;
    };
} ExprK, *ExprKRef;

typedef struct _ExprID {
    EXPR_HEAD
    char *name;
} ExprID, *ExprIDRef;

/* unary operator kind */
typedef enum _UnKind {
    /* - */
    UNARY_NEG = 1,
    /* ~ */
    UNARY_BIT_NOT,
    /* ! */
    UNARY_NOT
} UnKind;

typedef struct _ExprUn {
    EXPR_HEAD
    UnKind ukind;
    ExprRef exp;
} ExprUn, *ExprUnRef;

/* binary operator kind */
typedef enum _Bikind {
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
} BiKind;

typedef struct _ExprBi {
    EXPR_HEAD
    BiKind bkind;
    ExprRef lexp;
    ExprRef rexp;
} ExprBi, *ExprBiRef;

static inline void expr_set_loc(ExprRef e, int row, int col)
{
    e->row = row;
    e->col = col;
}

ExprRef expr_from_nil(void);
ExprRef expr_from_self(void);
ExprRef expr_from_super(void);
ExprRef expr_from_under(void);

ExprRef expr_from_int64(int64_t val);
ExprRef expr_from_float64(double val);
ExprRef expr_from_bool(int val);
ExprRef expr_from_str(char *val);
ExprRef expr_from_char(int val);

ExprRef expr_from_ident(char *val);
ExprRef expr_from_unary(UnKind ukind, ExprRef e);
ExprRef expr_from_binary(BiKind bkind, ExprRef le, ExprRef re);

typedef enum _StmtKind {
    /* import */
    STMT_IMPORT_KIND = 1,
    /* const */
    STMT_CONST_KIND,
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
    STMT_KIND_MAX
} StmtKind;

#define STMT_HEAD  \
    StmtKind kind; \
    int row;       \
    int col;

typedef struct _Stmt {
    STMT_HEAD
} Stmt, *StmtRef;

static inline void stmt_set_loc(StmtRef s, int row, int col)
{
    s->row = row;
    s->col = col;
}

typedef struct _StmtExpr {
    STMT_HEAD
    ExprRef exp;
} StmtExpr, *StmtExprRef;

StmtRef stmt_from_expr(ExprRef e);

void free_stmt(StmtRef s);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_AST_H_ */
