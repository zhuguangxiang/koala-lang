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

typedef struct expr Expr;
typedef struct stmt Stmt;

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

typedef struct idparadef {
  Ident id;
  Vector *vec;
} IdParaDef;

typedef struct typepara {
  Ident type;
  Vector *bounds;
} TypeParaDef;

static inline TypeParaDef *new_type_para(Ident type, Vector *bounds)
{
  TypeParaDef *tp = kmalloc(sizeof(TypeParaDef));
  tp->type = type;
  tp->bounds = bounds;
  return tp;
}

typedef struct extendsdef {
  Type type;
  Vector *withes;
} ExtendsDef;

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

static inline ExtendsDef *new_extends(Type type, Vector *withes)
{
  ExtendsDef *extends = kmalloc(sizeof(ExtendsDef));
  extends->type = type;
  extends->withes = withes;
  return extends;
}

static inline void free_extends(ExtendsDef *extends)
{
  if (extends == NULL)
    return;

  TYPE_DECREF(extends->type.desc);
  Type *type;
  vector_for_each(type, extends->withes) {
    free_type(type);
  }
  vector_free(extends->withes);
  kfree(extends);
}

typedef struct alias {
  Ident id;
  Ident alias;
} Alias;

static inline Alias *new_alias(Ident id, Ident *_alias)
{
  Alias *alias = kmalloc(sizeof(Alias));
  alias->id = id;
  if (_alias != NULL)
    alias->alias = *_alias;
  return alias;
}

static inline void free_alias(Alias *alias)
{
  kfree(alias);
}

static inline void free_aliases(Vector *vec)
{
  if (vec == NULL)
    return;

  Alias *alias;
  vector_for_each(alias, vec) {
    free_alias(alias);
  }
  vector_free(vec);
}

typedef struct enummembrs {
  Vector *labels;
  Vector *methods;
} EnumMembers;

typedef struct enumlabel {
  Ident id;
  int value;
  Vector *types;
} EnumLabel;

static inline EnumLabel *new_label(Ident id, int value, Vector *types)
{
  EnumLabel *label = kmalloc(sizeof(EnumLabel));
  label->id = id;
  label->value = value;
  label->types = types;
  return label;
}

static inline void free_label(EnumLabel *label)
{
  TypeDesc *item;
  vector_for_each(item, label->types) {
    TYPE_DECREF(item);
  }
  vector_free(label->types);
  kfree(label);
}

static inline void free_labels(Vector *vec)
{
  if (vec == NULL)
    return;

  EnumLabel *label;
  vector_for_each(label, vec) {
    free_label(label);
  }
  vector_free(vec);
}

typedef struct matchclause {
  Expr *pattern;
  Stmt *block;
  STable *stbl;
} MatchClause;

static inline MatchClause *new_match_clause(Expr *pattern, Stmt *block)
{
  MatchClause *match = kmalloc(sizeof(MatchClause));
  match->pattern = pattern;
  match->block = block;
  return match;
}

void expr_free(Expr *exp);
void exprlist_free(Vector *vec);
void stmt_free(Stmt *stmt);

static inline void free_match_clause(MatchClause *match)
{
  expr_free(match->pattern);
  stmt_free(match->block);
  kfree(match);
}

static inline void free_match_clauses(Vector *vec)
{
  if (vec == NULL)
    return;

  MatchClause *match;
  vector_for_each(match, vec) {
    free_match_clause(match);
  }
  vector_free(vec);
}

Expr *expr_enum_pattern(Ident id, Ident *ename, Ident *mname, Vector *exps);

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
  /* &, ^, | */
  BINARY_BIT_AND, BINARY_BIT_XOR, BINARY_BIT_OR,
  /* >, >=, <, <=, ==, != */
  BINARY_GT, BINARY_GE, BINARY_LT, BINARY_LE, BINARY_EQ, BINARY_NEQ,
  /* &&, || */
  BINARY_LAND, BINARY_LOR,
} BinaryOpKind;

/* expression kind */
typedef enum exprkind {
  /* null, self and super */
  NULL_KIND = 1, SELF_KIND, SUPER_KIND,
  /* literal, ident and underscore(_) */
  LITERAL_KIND, ID_KIND, UNDER_KIND,
  /* unary, binary, ternary op */
  UNARY_KIND, BINARY_KIND, TERNARY_KIND,
  /* dot, [], (), [:] access */
  ATTRIBUTE_KIND, SUBSCRIPT_KIND, CALL_KIND, SLICE_KIND,
  /* dot tuple */
  DOT_TUPLE_KIND,
  /* tuple, array, map, anonymous */
  TUPLE_KIND, ARRAY_KIND, MAP_KIND, ANONY_KIND,
  /* is, as */
  IS_KIND, AS_KIND,
  /* new object */
  NEW_KIND,
  /* range object */
  RANGE_KIND,
  /* enum pattern */
  ENUM_PATTERN_KIND,
  /* binary match */
  BINARY_MATCH_KIND,
  EXPR_KIND_MAX
} ExprKind;

/* expression context */
typedef enum exprctx {
  EXPR_INVALID,
  /* left or right value indicator */
  EXPR_LOAD, EXPR_STORE,
  /* inplace assign */
  EXPR_INPLACE,
  /* call or load function indicator */
  EXPR_CALL_FUNC, EXPR_LOAD_FUNC,
} ExprCtx;

struct expr {
  ExprKind kind;
  short row;
  short col;
  ExprCtx ctx;
  Symbol *sym;
  TypeDesc *desc;
  Expr *right;
  Stmt *inplace;
  char *funcname;
  int argc;
  int8_t leftside;
  int8_t hasvalue;
  int8_t first;
  int8_t super;

  /* for match */
  Expr *pattern;
  Vector *types;
  int16_t index;
  int16_t newvar;

  union {
    struct {
      int omit;
      Literal value;
    } k;
    struct {
      char *name;
      /* saved as enum type, if ident is enum type */
      TypeDesc *etype;
      /*
      * where is the identifier?
      */
      int where;
    #define CURRENT_SCOPE 1
    #define UP_SCOPE      2
    #define EXT_SCOPE     3
    #define EXTDOT_SCOPE  4
    #define AUTO_IMPORTED 5
    #define ID_IN_ENUM    6
      /* scope pointer */
      void *scope;
    } id;
    struct {
      UnaryOpKind op;
      Expr *exp;
      Literal val;
    } unary;
    struct {
      BinaryOpKind op;
      int oprow;
      int opcol;
      Expr *lexp;
      Expr *rexp;
      Literal val;
    } binary;
    struct {
      Expr *test;
      Expr *lexp;
      Expr *rexp;
    } ternary;
    struct {
      Ident id;
      Expr *lexp;
    } attr;
    struct {
      int64_t index;
      Expr *lexp;
    } dottuple;
    struct {
      Expr *lexp;
      Expr *index;
    } subscr;
    struct {
      /* arguments list */
      Vector *args;
      Expr *lexp;
    } call;
    struct {
      Expr *lexp;
      Expr *start;
      Expr *end;
    } slice;
    Vector *tuple;
    Vector *array;
    Vector *map;
    struct {
      Vector *idtypes;
      Type ret;
      Vector *body;
    } anony;
    struct {
      Expr *exp;
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
      Expr *start;
      Expr *end;
    } range;
    struct {
      Ident id;
      Ident ename;
      Ident mname;
      Vector *exps;
      Symbol *sym; // match expr's symbol
      int argc; // count of literals of exps
    } enum_pattern;
    struct {
      Expr *some;
      Expr *pattern;
    } binary_match;
  };
};

typedef struct mapentry {
  Expr *key;
  Expr *val;
} MapEntry;

Expr *expr_from_null(void);
Expr *expr_from_self(void);
Expr *expr_from_super(void);
Expr *expr_from_int(int64_t val);
Expr *expr_from_float(double val);
Expr *expr_from_bool(int val);
Expr *expr_from_str(char *val);
Expr *expr_from_char(wchar val);
Expr *expr_from_ident(char *val);
Expr *expr_from_underscore(void);
Expr *expr_from_unary(UnaryOpKind op, Expr *exp);
Expr *expr_from_binary(BinaryOpKind op, Expr *left, Expr *right);
Expr *expr_from_ternary(Expr *test, Expr *left, Expr *right);
Expr *expr_from_attribute(Ident id, Expr *left);
Expr *expr_from_dottuple(int64_t index, Expr *left);
Expr *expr_from_subscr(Expr *left, Expr *index);
Expr *expr_from_call(Vector *args, Expr *left);
Expr *expr_from_slice(Expr *left, Expr *start, Expr *end);
Expr *expr_from_tuple(Vector *exps);
Expr *expr_from_array(Vector *exps);
MapEntry *new_mapentry(Expr *key, Expr *val);
Expr *expr_from_map(Vector *exps);
Expr *expr_from_anony(Vector *idtypes, Type *ret, Vector *body);
Expr *expr_from_istype(Expr *exp, Type type);
Expr *expr_from_astype(Expr *exp, Type type);
Expr *expr_from_object(char *path, Ident id, Vector *types, Vector *args);
Expr *expr_from_range(int type, Expr *start, Expr *end);
Expr *expr_from_binary_match(Expr *pattern, Expr *some);

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
  /* ifunc */
  IFUNC_KIND,
  /* class */
  CLASS_KIND,
  /* trait */
  TRAIT_KIND,
  /* enum */
  ENUM_KIND,
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
  OP_MULT_ASSIGN, OP_DIV_ASSIGN, OP_MOD_ASSIGN, OP_POW_ASSIGN,
  OP_AND_ASSIGN, OP_OR_ASSIGN, OP_XOR_ASSIGN,
} AssignOpKind;

struct stmt {
  StmtKind kind;
  short last;
  short hasvalue;
  TypeDesc *desc;
  short row;
  short col;
  union {
    struct {
      int type;
#define IMPORT_ALL      1
#define IMPORT_PARTIAL  2
      Ident id;
      char *path;
      int pathrow;
      int pathcol;
      Vector *aliases;
    } import;
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
      Vector *block;
      Stmt *orelse;
    } if_stmt;
    struct {
      Expr *test;
      Vector *block;
    } while_stmt;
    struct {
      Expr *vexp;
      Expr *iter;
      Expr *step;
      Vector *block;
    } for_stmt;
    struct {
      Ident id;
      /* type parameters */
      Vector *typeparas;
      /* extends */
      ExtendsDef *extends;
      /* body */
      Vector *body;
    } class_stmt;
    struct {
      Ident id;
      /* type parameters */
      Vector *typeparas;
      EnumMembers mbrs;
    } enum_stmt;
    struct {
      Expr *exp;
      Vector *clauses;
    } match_stmt;
  };
};

void stmt_block_free(Vector *vec);
Stmt *stmt_from_import(Ident *id, char *path);
Stmt *stmt_from_import_all(char *path);
Stmt *stmt_from_import_partial(Vector *vec, char *path);
Stmt *stmt_from_constdecl(Ident id, Type *type, Expr *exp);
Stmt *stmt_from_vardecl(Ident id, Type *type, Expr *exp);
Stmt *stmt_from_assign(AssignOpKind op, Expr *left, Expr *right);
Stmt *stmt_from_funcdecl(Ident id, Vector *typeparas, Vector *args,
                         Type *ret, Vector *body);
Stmt *stmt_from_return(Expr *exp);
Stmt *stmt_from_break(short row, short col);
Stmt *stmt_from_continue(short row, short col);
Stmt *stmt_from_expr(Expr *exp);
Stmt *stmt_from_block(Vector *list);
Stmt *stmt_from_if(Expr *test, Vector *block, Stmt *orelse);
Stmt *stmt_from_while(Expr *test, Vector *block);
Stmt *stmt_from_for(Expr *vexp, Expr *iter, Expr *step, Vector *block);
Stmt *stmt_from_class(Ident id, Vector *typeparas, ExtendsDef *extends,
                      Vector *body);
Stmt *stmt_from_trait(Ident id, Vector *typeparas, ExtendsDef *extends,
                      Vector *body);
Stmt *stmt_from_ifunc(Ident id, Vector *args, Type *ret);
Stmt *stmt_from_enum(Ident id, Vector *typeparas, EnumMembers mbrs);
Stmt *stmt_from_match(Expr *exp, Vector *clauses);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_AST_H_ */
