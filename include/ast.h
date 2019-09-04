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
} ident;

/* typedesc with position */
typedef struct type {
  typedesc *desc;
  short row;
  short col;
} type;

/* idtype for parameter-list in AST */
typedef struct idtype {
  ident id;
  type type;
} idtype;

/* parameter type */
typedef struct typepara {
  ident id;       /* T: Foo & Bar */
  vector *types;  /* Foo, Bar and etc, type */
} TypePara;

void TypePara_IsDuplicated(vector *typeparas, ident *id);

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
} unaryopkind;

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
} binaryopkind;

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
  EXPR_KIND_MAX
} exprkind;

/* expression context */
typedef enum exprctx {
  EXPR_INVALID,
  /* left or right value indicator */
  EXPR_LOAD, EXPR_STORE,
  /* call or load function indicator */
  EXPR_CALL_FUNC, EXPR_LOAD_FUNC
} exprctx;

typedef struct expr {
  exprkind kind;
  short row;
  short col;
  exprctx ctx;
  symbol *sym;
  typedesc *desc;
  struct expr *right;
  int argc;
  int leftside;
  int hasvalue;
  union {
    struct {
      int omit;
      literal value;
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
      unaryopkind op;
      struct expr *exp;
      literal val;
    } unary;
    struct {
      binaryopkind op;
      struct expr *lexp;
      struct expr *rexp;
      literal val;
    } binary;
    struct {
      struct expr *cond;
      struct expr *lexp;
      struct expr *rexp;
    } ternary;
    struct {
      ident id;
      struct expr *lexp;
    } attr;
    struct {
      struct expr *lexp;
      struct expr *index;
    } subscr;
    struct {
      /* arguments list */
      vector *args;
      struct expr *lexp;
    } call;
    struct {
      struct expr *lexp;
      struct expr *start;
      struct expr *end;
    } slice;
    vector *tuple;
    struct {
      int dims;
      vector *elems;
    } array;
    vector *map;
    struct {
      struct expr *key;
      struct expr *val;
    } mapentry;
    struct {
      struct expr *exp;
      type type;
    } isas;
  };
} expr;

void expr_free(expr *exp);
void exprlist_free(vector *vec);
expr *expr_from_nil(void);
expr *expr_from_self(void);
expr *expr_from_super(void);
expr *expr_from_integer(int64_t val);
expr *expr_from_float(double val);
expr *expr_from_bool(int val);
expr *expr_from_string(char *val);
expr *expr_from_char(wchar val);
expr *expr_from_ident(char *val);
expr *expr_from_unary(unaryopkind op, expr *exp);
expr *expr_from_binary(binaryopkind op, expr *left, expr *right);
expr *expr_from_ternary(expr *cond, expr *left, expr *right);
expr *expr_from_attribute(ident id, expr *left);
expr *expr_from_subScript(expr *left, expr *index);
expr *expr_from_call(vector *args, expr *left);
expr *expr_from_slice(expr *left, expr *start, expr *end);
expr *expr_from_tuple(vector *exps);
expr *expr_from_array(vector *exps);
expr *expr_from_mapentry(expr *key, expr *val);
expr *expr_from_map(vector *exps);
expr *expr_from_istype(expr *exp, type type);
expr *expr_from_astype(expr *exp, type type);

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
} stmtkind;

/* assignment operators */
typedef enum assignopkind {
  OP_ASSIGN = 1,
  OP_PLUS_ASSIGN, OP_MINUS_ASSIGN,
  OP_MULT_ASSIGN, OP_DIV_ASSIGN, OP_POW_ASSIGN, OP_MOD_ASSIGN,
  OP_AND_ASSIGN, OP_OR_ASSIGN, OP_XOR_ASSIGN,
} assignopkind;

typedef struct stmt {
  stmtkind kind;
  short last;
  short hasvalue;
  union {
    struct {
      int freevar;
      ident id;
      type type;
      expr *exp;
    } vardecl;
    struct {
      assignopkind op;
      expr *lexp;
      expr *rexp;
    } assign;
    struct {
      ident id;
      /* native flag */
      int native;
      /* type parameters */
      vector *typeparas;
      /* idtype */
      vector *args;
      /* return type */
      type ret;
      /* body */
      vector *body;
    } funcdecl;
    struct {
      expr *exp;
    } ret;
    struct {
      expr *exp;
    } expr;
    struct {
      vector *vec;
    } block;
  };
} stmt;

void stmt_free(stmt *s);
stmt *stmt_from_constdecl(ident id, type *type, expr *exp);
stmt *stmt_from_vardecl(ident id, type *type, expr *exp);
stmt *stmt_from_assign(assignopkind op, expr *left, expr *right);
stmt *stmt_from_funcdecl(ident id, vector *typeparam, vector *args,
                         type *ret, vector *body);
stmt *stmt_from_return(expr *exp);
stmt *stmt_from_expr(expr *exp);
stmt *stmt_from_block(vector *list);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_AST_H_ */
