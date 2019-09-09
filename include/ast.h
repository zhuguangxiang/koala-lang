/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
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

static inline Type *new_type(TypeDesc *desc, short row, short col)
{
  Type *type = kmalloc(sizeof(*type));
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

/* idtype for parameter-list in AST */
typedef struct idtype {
  Ident id;
  Type type;
} IdType;

/* parameter type */
typedef struct typepara {
  Ident id;       /* T: Foo & Bar */
  Vector *types;  /* Foo, Bar and etc, Type */
} TypePara;

void TypePara_IsDuplicated(Vector *typeparas, Ident *id);

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
  /* tuple, array, map entry, map, anonymous */
  TUPLE_KIND, ARRAY_KIND, MAP_ENTRY_KIND, MAP_KIND, ANONY_KIND,
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
      struct expr *key;
      struct expr *val;
    } mapentry;
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
Expr *expr_from_mapentry(Expr *key, Expr *val);
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
  ENUM_VALUE_KIND,
  /* break */
  BREAK_KIND,
  /* continue */
  CONTINUE_KIND,
  STMT_KIND_MAX
} StmtKind;

/* assignment operators */
typedef enum assignopkind {
  OP_ASSIGN = 1,
  OP_PLUS_ASSIGN, OP_MINUS_ASSIGN,
  OP_MULT_ASSIGN, OP_DIV_ASSIGN, OP_POW_ASSIGN, OP_MOD_ASSIGN,
  OP_AND_ASSIGN, OP_OR_ASSIGN, OP_XOR_ASSIGN,
} AssignOpKind;

typedef struct stmt {
  StmtKind kind;
  short last;
  short hasvalue;
  union {
    struct {
      int freevar;
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
      /* native flag */
      int native;
      /* type parameters */
      Vector *typeparas;
      /* idtype */
      Vector *args;
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
  };
} Stmt;

void stmt_free(Stmt *stmt);
Stmt *stmt_from_constdecl(Ident id, Type *type, Expr *exp);
Stmt *stmt_from_vardecl(Ident id, Type *type, Expr *exp);
Stmt *stmt_from_assign(AssignOpKind op, Expr *left, Expr *right);
Stmt *stmt_from_funcdecl(Ident id, Vector *typeparam, Vector *args,
                         Type *ret, Vector *body);
Stmt *stmt_from_return(Expr *exp);
Stmt *stmt_from_expr(Expr *exp);
Stmt *stmt_from_block(Vector *list);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_AST_H_ */
