/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#ifndef _KOALA_AST_H_
#define _KOALA_AST_H_

#include "symbol.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* identifier */
typedef struct ident {
  char *name;
  short row;
  short col;
} Ident;

/* typedesc with position */
typedef struct type {
  TypeDesc *desc;
  short row;
  short col;
} Type;

/* idtype for parameter-list in AST */
typedef struct idtype {
  Ident id;
  Type type;
} IdType;

static inline Type *new_type(TypeDesc *desc, short row, short col)
{
  Type *type = kmalloc(sizeof(Type));
  type->desc = TYPE_INCREF(desc);
  type->row = row;
  type->col = col;
  return type;
}

static inline void free_type(Type *type)
{
  TYPE_DECREF(type->desc);
  kfree(type);
}

static inline IdType *new_idtype(Ident id, Type type)
{
  IdType *idtype = kmalloc(sizeof(IdType));
  idtype->id = id;
  idtype->type = type;
  return idtype;
}

static inline void free_idtype(IdType *idtype)
{
  TYPE_DECREF(idtype->type.desc);
  kfree(idtype);
}

void free_idtypes(Vector *vec);

typedef struct {
  Ident id;
  Vector *vec;
} IdParaDef;

/* unary operator kind */
typedef enum unaryopkind {
  /* + */
  UNARY_PLUS = 1,
  /* - */
  UNARY_NEG,
  /* ~ */
  UNARY_BIT_NOT,
  /* ! */
  UNARY_LNOT
} UnaryOpKind;

/* binary operator kind */
typedef enum binaryopkind {
  /* +, -, *, /, %, ** */
  BINARY_ADD = 1, BINARY_SUB, BINARY_MULT, BINARY_DIV, BINARY_MOD, BINARY_POW,
  /* >, >=, <, <=, ==, != */
  BINARY_GT, BINARY_GE, BINARY_LT, BINARY_LE, BINARY_EQ, BINARY_NEQ,
  /* &, ^, | */
  BINARY_BIT_AND, BINARY_BIT_XOR, BINARY_BIT_OR,
  /* &&, || */
  BINARY_LAND, BINARY_LOR,
} BinaryOpKind;

/* expression kind */
typedef enum exprkind {
  /* nil, self and super */
  NIL_KIND = 1, SELF_KIND, SUPER_KIND,
  /* literal and ident */
  LITERAL_KIND, ID_KIND,
  /* unary, binary, ternary op */
  UNARY_KIND, BINARY_KIND, TERNARY_KIND,
  /* dot, [], (), [:] access */
  ATTRIBUTE_KIND, SUBSCRIPT_KIND, CALL_KIND, SLICE_KIND,
  /* tuple, array, map, anonymous */
  TUPLE_KIND, ARRAY_KIND, MAP_KIND, ANONY_KIND,
  /* is, as */
  IS_KIND, AS_KIND,
  /* new object */
  NEW_KIND,
  /* range object */
  RANGE_KIND,
  EXPR_KIND_MAX
} ExprKind;

/* expression context */
typedef enum exprctx {
  EXPR_INVALID,
  /* left or right value indicator */
  EXPR_LOAD, EXPR_STORE,
  /* call or load function indicator */
  EXPR_CALL_FUNC, EXPR_LOAD_FUNC
} ExprCtx;

typedef struct expr {
  ExprKind kind;
  short row;
  short col;
  ExprCtx ctx;
  Symbol *sym;
  TypeDesc *desc;
  struct expr *right;
  int argc;
  int leftside;
  int hasvalue;
  union {
    struct {
      int omit;
      Literal value;
    } k;
    struct {
      char *name;
      /*
      * where is the identifier?
      */
      int where;
    #define CURRENT_SCOPE 1
    #define UP_SCOPE      2
    #define EXT_SCOPE     3
    #define EXTDOT_SCOPE  4
      /* scope pointer */
      void *scope;
    } id;
    struct {
      UnaryOpKind op;
      struct expr *exp;
      Literal val;
    } unary;
    struct {
      BinaryOpKind op;
      struct expr *lexp;
      struct expr *rexp;
      Literal val;
    } binary;
    struct {
      struct expr *cond;
      struct expr *lexp;
      struct expr *rexp;
    } ternary;
    struct {
      Ident id;
      struct expr *lexp;
    } attr;
    struct {
      struct expr *lexp;
      struct expr *index;
    } subscr;
    struct {
      /* arguments list */
      Vector *args;
      struct expr *lexp;
    } call;
    struct {
      struct expr *lexp;
      struct expr *start;
      struct expr *end;
    } slice;
    Vector *tuple;
    Vector *array;
    Vector *map;
    struct {
      struct expr *exp;
      Type type;
    } isas;
    struct {
      char *path;
      Ident id;
      Vector *types;
      Vector *args;
    } newobj;
    struct {
      int type;
      struct expr *left;
      struct expr *right;
    } range;
  };
} Expr;

typedef struct mapentry {
  Expr *key;
  Expr *val;
} MapEntry;

void expr_free(Expr *exp);
void exprlist_free(Vector *vec);
Expr *expr_from_nil(void);
Expr *expr_from_self(void);
Expr *expr_from_super(void);
Expr *expr_from_integer(int64_t val);
Expr *expr_from_float(double val);
Expr *expr_from_bool(int val);
Expr *expr_from_string(char *val);
Expr *expr_from_char(wchar val);
Expr *expr_from_ident(char *val);
Expr *expr_from_unary(UnaryOpKind op, Expr *exp);
Expr *expr_from_binary(BinaryOpKind op, Expr *left, Expr *right);
Expr *expr_from_ternary(Expr *cond, Expr *left, Expr *right);
Expr *expr_from_attribute(Ident id, Expr *left);
Expr *expr_from_subscr(Expr *left, Expr *index);
Expr *expr_from_call(Vector *args, Expr *left);
Expr *expr_from_slice(Expr *left, Expr *start, Expr *end);
Expr *expr_from_tuple(Vector *exps);
Expr *expr_from_array(Vector *exps);
MapEntry *new_mapentry(Expr *key, Expr *val);
Expr *expr_from_map(Vector *exps);
Expr *expr_from_istype(Expr *exp, Type type);
Expr *expr_from_astype(Expr *exp, Type type);
Expr *expr_from_object(char *path, Ident id, Vector *types, Vector *args);
Expr *expr_from_range(int type, Expr *left, Expr *right);

typedef enum stmtkind {
  /* import */
  IMPORT_KIND = 1,
  /* const */
  CONST_KIND,
  /* variable */
  VAR_KIND,
  /* assignment */
  ASSIGN_KIND,
  /* function */
  FUNC_KIND,
  /* return */
  RETURN_KIND,
  /* expression */
  EXPR_KIND,
  /* statements */
  BLOCK_KIND,
  /* proto/native */
  PROTO_KIND,
  /* class */
  CLASS_KIND,
  /* trait */
  TRAIT_KIND,
  /* enum */
  ENUM_KIND,
  /* enum value */
  EVAL_KIND,
  /* break, continue */
  BREAK_KIND, CONTINUE_KIND,
  /* if, while, for, match */
  IF_KIND, WHILE_KIND, FOR_KIND, MATCH_KIND,
  STMT_KIND_MAX
} StmtKind;

/* assignment operators */
typedef enum assignopkind {
  OP_ASSIGN = 1,
  OP_PLUS_ASSIGN, OP_MINUS_ASSIGN,
  OP_MULT_ASSIGN, OP_DIV_ASSIGN, OP_POW_ASSIGN, OP_MOD_ASSIGN,
  OP_AND_ASSIGN, OP_OR_ASSIGN, OP_XOR_ASSIGN,
} AssignOpKind;

typedef struct stmt Stmt;

struct stmt {
  StmtKind kind;
  short last;
  short hasvalue;
  TypeDesc *desc;
  union {
    struct {
      Ident id;
      Type type;
      Expr *exp;
    } vardecl;
    struct {
      AssignOpKind op;
      Expr *lexp;
      Expr *rexp;
    } assign;
    struct {
      Ident id;
      /* type parameters */
      Vector *typeparas;
      /* idtype */
      Vector *idtypes;
      /* return type */
      Type ret;
      /* body */
      Vector *body;
    } funcdecl;
    struct {
      Expr *exp;
    } ret;
    struct {
      Expr *exp;
    } expr;
    struct {
      Vector *vec;
    } block;
    struct {
      Expr *test;
      Stmt *block;
      Stmt *orelse;
    } if_stmt;
    struct {
      Expr *test;
      Stmt *block;
    } while_stmt;
    struct {
      Expr *vexp;
      Expr *iter;
      Stmt *block;
    } for_stmt;
  };
};

void stmt_free(Stmt *stmt);
void stmt_block_free(Vector *vec);
Stmt *stmt_from_constdecl(Ident id, Type *type, Expr *exp);
Stmt *stmt_from_vardecl(Ident id, Type *type, Expr *exp);
Stmt *stmt_from_assign(AssignOpKind op, Expr *left, Expr *right);
Stmt *stmt_from_funcdecl(Ident id, Vector *typeparam, Vector *args,
  Type *ret, Vector *body);
Stmt *stmt_from_return(Expr *exp);
Stmt *stmt_from_expr(Expr *exp);
Stmt *stmt_from_block(Vector *list);
Stmt *stmt_from_if(Expr *test, Stmt *block, Stmt *orelse);
Stmt *stmt_from_while(Expr *test, Stmt *block);
Stmt *stmt_from_for(Expr *vexp, Expr *iter, Stmt *block);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_AST_H_ */
