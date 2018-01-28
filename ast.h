
#ifndef _KOALA_AST_H_
#define _KOALA_AST_H_

#include "common.h"
#include "list.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------------------*/

enum type_kind {
  PRIMITIVE_KIND = 1, USERDEF_TYPE = 2, FUNCTION_TYPE = 3,
};

struct type {
  enum type_kind kind;
  union {
    int primitive;
    struct {
      char *mod;
      char *type;
    } userdef;
    struct {
      Vector *tseq;
      Vector *rseq;
    } functype;
  };
  int dims;
};

struct type *type_from_primitive(int primitive);
struct type *type_from_userdef(char *mod_name, char *type_name);
struct type *type_from_functype(Vector *tseq, Vector *rseq);

/*-------------------------------------------------------------------------*/

enum unary_op_kind {
  OP_PLUS = 1, OP_MINUS = 2, OP_BIT_NOT = 3, OP_LNOT = 4
};

enum operator_kind {
  OP_MULT = 1, OP_DIV = 2, OP_MOD = 3,
  OP_ADD = 4, OP_SUB = 5,
  OP_LSHIFT, OP_RSHIFT,
  OP_GT, OP_GE, OP_LT, OP_LE,
  OP_EQ, OP_NEQ,
  OP_BIT_AND,
  OP_BIT_XOR,
  OP_BIT_OR,
  OP_LAND,
  OP_LOR,
};

enum expr_kind {
  NAME_KIND = 1, INT_KIND = 2, FLOAT_KIND = 3, STRING_KIND = 4, BOOL_KIND = 5,
  SELF_KIND = 6, NULL_KIND = 7, EXP_KIND = 8,
  ARRAY_KIND, ANONYOUS_FUNC_KIND,
  ATTRIBUTE_KIND, SUBSCRIPT_KIND, CALL_KIND,
  UNARY_KIND, BINARY_KIND, SEQ_KIND
};

enum expr_ctx {
  LOAD = 1, STORE = 2,
};

struct expr {
  enum expr_kind kind;
  struct type *type;
  union {
    struct {
      char *id;
      enum expr_ctx ctx;
    } name;
    int64 ival;
    float64 fval;
    char *str;
    int bval;
    struct expr *exp;
    struct {
      /* one of pointers is null, and another is not null */
      Vector *dseq;
      Vector *tseq;
    } array;
    struct {
      Vector *pseq;
      Vector *rseq;
      Vector *body;
    } anonyous_func;
    /* Trailer */
    struct {
      struct expr *left;
      char *id;
    } attribute;
    struct {
      struct expr *left;
      struct expr *index;
    } subscript;
    struct {
      struct expr *left;
      Vector *pseq;   /* expression list */
    } call;
    /* arithmetic operation */
    struct {
      enum unary_op_kind op;
      struct expr *operand;
    } unary_op;
    struct {
      struct expr *left;
      enum operator_kind op;
      struct expr *right;
    } bin_op;
    /* expression is also an expr sequence, for array initializer */
    Vector *seq;
  };
};

struct expr *expr_from_trailer(enum expr_kind kind, void *trailer,
                               struct expr *left);
struct expr *expr_from_name(char *id);
struct expr *expr_from_name_type(char *id, struct type *type);
struct expr *expr_from_int(int64 ival);
struct expr *expr_from_float(float64 fval);
struct expr *expr_from_string(char *str);
struct expr *expr_from_bool(int bval);
struct expr *expr_from_self(void);
struct expr *expr_from_expr(struct expr *exp);
struct expr *expr_from_null(void);
struct expr *expr_from_array(struct type *type, Vector *dseq, Vector *tseq);
struct expr *expr_from_array_with_tseq(Vector *tseq);
struct expr *expr_from_anonymous_func(Vector *pseq, Vector *rseq, Vector *body);
struct expr *expr_from_binary(enum operator_kind kind,
                              struct expr *left, struct expr *right);
struct expr *expr_from_unary(enum unary_op_kind kind, struct expr *expr);

void expr_traverse(struct expr *exp);

/*-------------------------------------------------------------------------*/

struct test_block {
  struct expr *test;
  Vector *body;
};

struct test_block *new_test_block(struct expr *test, Vector *body);

enum assign_operator {
  OP_PLUS_ASSIGN = 1, OP_MINUS_ASSIGN = 2,
  OP_MULT_ASSIGN, OP_DIV_ASSIGN,
  OP_MOD_ASSIGN, OP_AND_ASSIGN, OP_OR_ASSIGN, OP_XOR_ASSIGN,
  OP_RSHIFT_ASSIGN, OP_LSHIFT_ASSIGN,
};

enum stmt_kind {
  IMPORT_KIND = 1, EXPR_KIND, VARDECL_KIND, INIT_ASSIGN_KIND,
  FUNCDECL_KIND = 5, ASSIGN_KIND, COMPOUND_ASSIGN_KIND,
  CLASS_KIND = 8, INTF_KIND, RETURN_KIND,
  IF_KIND = 11, WHILE_KIND, SWITCH_KIND,
  FOR_TRIPLE_KIND = 14, FOR_EACH_KIND, BREAK_KIND, CONTINUE_KIND,
  GO_KIND = 18, BLOCK_KIND, STMT_KIND_MAX
};

struct stmt {
  enum stmt_kind kind;
  union {
    struct {
      char *id;
      char *path;
    } import;
    struct expr *expr;
    struct {
      Vector *var_seq;
      Vector *expr_seq;
    } vardecl;
    struct {
      char *id;
      Vector *pseq;
      Vector *rseq;
      Vector *body;
    } funcdecl;
    struct {
      Vector *left_seq;
      Vector *right_seq;
    } assign;
    struct {
      struct expr *left;
      enum assign_operator op;
      struct expr *right;
    } compound_assign;
    struct {
      char *id;
      Vector *seq;
    } structure;
    struct {
      char *id;
      struct type *type;
    } user_typedef;
    struct {
      struct test_block *if_part;
      Vector *elseif_seq;
      struct test_block *else_part;
    } if_stmt;
    struct {
      int btest;
      struct expr *test;
      Vector *body;
    } while_stmt;
    struct {
      struct expr *expr;
      Vector *case_seq;
    } switch_stmt;
    struct {
      struct stmt *init;
      struct stmt *test;
      struct stmt *incr;
      Vector *body;
    } for_triple_stmt;
    struct {
      int bdecl;
      struct var *var;
      struct expr *expr;
      Vector *body;
    } for_each_stmt;
    struct expr *go_stmt;
    Vector *seq;
  };
};

struct stmt *stmt_from_expr(struct expr *expr);
struct stmt *stmt_from_import(char *id, char *path);
struct stmt *stmt_from_vardecl(Vector *varseq, Vector *initseq,
                               int bconst, struct type *type);
struct stmt *stmt_from_initassign(Vector *var_seq, Vector *expr_seq);
struct stmt *stmt_from_funcdecl(char *id, Vector *pseq, Vector *rseq,
                                Vector *body);
struct stmt *stmt_from_assign(Vector *left_seq, Vector *right_seq);
struct stmt *stmt_from_compound_assign(struct expr *left,
                                       enum assign_operator op,
                                       struct expr *right);
struct stmt *stmt_from_block(Vector *seq);
struct stmt *stmt_from_return(Vector *seq);
struct stmt *stmt_from_empty(void);
struct stmt *stmt_from_structure(char *id, Vector *seq);
struct stmt *stmt_from_interface(char *id, Vector *seq);
struct stmt *stmt_from_jump(int kind);
struct stmt *stmt_from_if(struct test_block *if_part,
                          Vector *elseif_seq, struct test_block *else_part);
struct stmt *stmt_from_while(struct expr *test, Vector *body, int b);
struct stmt *stmt_from_switch(struct expr *expr, Vector *case_seq);
struct stmt *stmt_from_for(struct stmt *init, struct stmt *test,
                           struct stmt *incr, Vector *body);
struct stmt *stmt_from_foreach(struct var *var, struct expr *expr,
                               Vector *body, int bdecl);
struct stmt *stmt_from_go(struct expr *expr);

/*-------------------------------------------------------------------------*/

struct var {
  char *id;
  int bconst;
  struct type *type;
};

struct field {
  char *id;
  struct type *type;
  struct expr *expr;
};

struct func_proto {
  Vector *pseq;
  Vector *rseq;
};

struct intf_func {
  char *id;
  struct func_proto proto;
};

struct var *new_var(char *id, struct type *type);
struct field *new_struct_field(char *id, struct type *t, struct expr *e);
struct intf_func *new_intf_func(char *id, Vector *pseq,
                                Vector *rseq);

/*-------------------------------------------------------------------------*/

void ast_traverse(Vector *seq);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_AST_H_ */
