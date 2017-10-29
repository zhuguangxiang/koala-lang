
#ifndef _KOALA_AST_H_
#define _KOALA_AST_H_

#include "types.h"
#include "list.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------------------*/

struct sequence {
  int count;
  struct vector vec;
};

struct sequence *seq_new(void);
void seq_free(struct sequence *seq);
#define seq_size(seq) ((seq)->count)
int seq_insert(struct sequence *seq, int index, void *obj);
int seq_append(struct sequence *seq, void *obj);
void *seq_get(struct sequence *seq, int index);

/*-------------------------------------------------------------------------*/

enum type_kind {
  PRIMITIVE_KIND = 1, USERDEF_TYPE = 2, FUNCTION_TYPE = 3,
};

struct type {
  enum type_kind kind;
  union {
    int primitive;
    struct {
      char *mod_name;
      char *type_name;
    } userdef;
    struct {
      struct sequence *tseq;
      struct sequence *rseq;
    } functype;
  };
  int dims;
};

struct type *type_from_primitive(int primitive);
struct type *type_from_userdef(char *mod_name, char *type_name);
struct type *type_from_functype(struct sequence *tseq, struct sequence *rseq);

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
  NEW_PRIMITIVE_KIND, ARRAY_KIND, ANONYOUS_FUNC_KIND,
  ATTRIBUTE_KIND, SUBSCRIPT_KIND, CALL_KIND,
  UNARY_KIND, BINARY_KIND, SEQ_KIND
};

struct expr {
  enum expr_kind kind;
  struct type *type;
  union {
    struct {
      char *id;
    } name;
    int64_t ival;
    float64_t fval;
    char *str;
    int bval;
    struct expr *exp;
    struct {
      struct type *type;
      /* one of pointers is null, and another is not null */
      struct sequence *dseq;
      struct sequence *tseq;
    } array;
    struct {
      struct sequence *pseq;
      struct sequence *rseq;
      struct sequence *body;
    } anonyous_func;
    /* Trailer */
    struct {
      struct expr *left;
      struct expr *id;
    } attribute;
    struct {
      struct expr *left;
      struct expr *index;
    } subscript;
    struct {
      struct expr *left;
      struct sequence *pseq;   /* expression list */
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
    /* expression is also an expr sequence */
    struct sequence *seq;
  };
};

struct expr *expr_from_trailer(enum expr_kind kind, void *trailer,
                               struct expr *left);
struct expr *expr_from_name(char *id);
struct expr *expr_from_name_type(char *id, struct type *type);
struct expr *expr_from_int(int64_t ival);
struct expr *expr_from_float(float64_t fval);
struct expr *expr_from_string(char *str);
struct expr *expr_from_bool(int bval);
struct expr *expr_from_self(void);
struct expr *expr_from_expr(struct expr *exp);
struct expr *expr_from_null(void);
struct expr *expr_from_array0(struct type *type, struct sequence *seq);
struct expr *expr_from_anonymous_func(struct sequence *pseq,
                                      struct sequence *rseq,
                                      struct sequence *body);
struct expr *expr_from_binary(enum operator_kind kind,
                             struct expr *left, struct expr *right);
struct expr *expr_from_unary(enum unary_op_kind kind, struct expr *expr);

void expr_traverse(struct expr *exp);

/*-------------------------------------------------------------------------*/

struct test_block {
  struct expr *test;
  struct sequence *body;
};

struct test_block *new_test_block(struct expr *test, struct sequence *body);

enum assign_operator {
  OP_PLUS_ASSIGN = 1, OP_MINUS_ASSIGN = 2,
  OP_MULT_ASSIGN, OP_DIV_ASSIGN,
  OP_MOD_ASSIGN, OP_AND_ASSIGN, OP_OR_ASSIGN, OP_XOR_ASSIGN,
  OP_RSHIFT_ASSIGN, OP_LSHIFT_ASSIGN,
};

enum stmt_kind {
  EMPTY_KIND = 1, IMPORT_KIND, EXPR_KIND, VARDECL_KIND,
  FUNCDECL_KIND = 5, ASSIGN_KIND, COMPOUND_ASSIGN_KIND,
  STRUCT_KIND = 8, INTF_KIND, RETURN_KIND,
  IF_KIND = 11, WHILE_KIND, SWITCH_KIND,
  FOR_TRIPLE_KIND = 14, FOR_EACH_KIND, BREAK_KIND, CONTINUE_KIND,
  GO_KIND = 18, BLOCK_KIND
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
      int bconst;
      struct sequence *var_seq;
      struct sequence *expr_seq;
    } vardecl;
    struct {
      char *sid;
      char *fid;
      struct sequence *pseq;
      struct sequence *rseq;
      struct sequence *body;
    } funcdecl;
    struct {
      struct sequence *left_seq;
      struct sequence *right_seq;
    } assign;
    struct {
      struct expr *left;
      enum assign_operator op;
      struct expr *right;
    } compound_assign;
    struct {
      char *id;
      struct sequence *seq;
    } structure;
    struct {
      char *id;
      struct type *type;
    } user_typedef;
    struct {
      struct test_block *if_part;
      struct sequence *elseif_seq;
      struct test_block *else_part;
    } if_stmt;
    struct {
      int btest;
      struct expr *test;
      struct sequence *body;
    } while_stmt;
    struct {
      struct expr *expr;
      struct sequence *case_seq;
    } switch_stmt;
    struct {
      struct stmt *init;
      struct stmt *test;
      struct stmt *incr;
      struct sequence *body;
    } for_triple_stmt;
    struct {
      int bdecl;
      struct expr *var;
      struct expr *expr;
      struct sequence *body;
    } for_each_stmt;
    struct expr *go_stmt;
    struct sequence *seq;
  };
};

struct stmt *stmt_from_expr(struct expr *expr);
struct stmt *stmt_from_import(char *id, char *path);
struct stmt *stmt_from_vardecl(struct sequence *varseq,
                               struct sequence *initseq,
                               int bconst, struct type *type);
struct stmt *stmt_from_funcdecl(char *sid, char *fid,
                                struct sequence *pseq,
                                struct sequence *rseq,
                                struct sequence *body);
struct stmt *stmt_from_assign(struct sequence *left_seq,
                              struct sequence *right_seq);
struct stmt *stmt_from_compound_assign(struct expr *left,
                                       enum assign_operator op,
                                       struct expr *right);
struct stmt *stmt_from_block(struct sequence *seq);
struct stmt *stmt_from_return(struct sequence *seq);
struct stmt *stmt_from_empty(void);
struct stmt *stmt_from_structure(char *id, struct sequence *seq);
struct stmt *stmt_from_interface(char *id, struct sequence *seq);
struct stmt *stmt_from_jump(int kind);
struct stmt *stmt_from_if(struct test_block *if_part,
                          struct sequence *elseif_seq,
                          struct test_block *else_part);
struct stmt *stmt_from_while(struct expr *test, struct sequence *body, int b);
struct stmt *stmt_from_switch(struct expr *expr, struct sequence *case_seq);
struct stmt *stmt_from_for(struct stmt *init, struct stmt *test,
                           struct stmt *incr, struct sequence *body);
struct stmt *stmt_from_foreach(struct expr *var, struct expr *expr,
                              struct sequence *body, int bdecl);
struct stmt *stmt_from_go(struct expr *expr);

/*-------------------------------------------------------------------------*/

enum member_kind {
  FIELD_KIND = 1, METHOD_KIND = 2, INTF_FUNCDECL_KIND = 3,
};

struct field {
  char *id;
  struct type *type;
  struct expr *expr;
};

struct intf_func {
  char *id;
  struct sequence *tseq;
  struct sequence *rseq;
};

struct field *new_struct_field(char *id, struct type *t, struct expr *e);
struct intf_func *new_intf_func(char *id, struct sequence *pseq,
                                struct sequence *rseq);

/*-------------------------------------------------------------------------*/

void ast_traverse(struct sequence *seq);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_AST_H_ */
