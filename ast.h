
#ifndef _KOALA_AST_H_
#define _KOALA_AST_H_

#include "types.h"
#include "list.h"

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
      char *mod_name;
      char *type_name;
    } userdef;
    struct {
      struct clist *tlist;
      struct clist *rlist;
    } functype;
  } v;
  int dims;
  struct list_head link;
};

struct type *type_from_primitive(int primitive);
struct type *type_from_userdef(char *mod_name, char *type_name);
struct type *type_from_functype(struct clist *tlist, struct clist *rlist);
#define type_set_dims(type, dims_val) ((type)->dims = (dims_val))
#define type_inc_dims(type, dims_val) ((type)->dims += (dims_val))

struct array_tail {
  struct list_head link;
  /* one of pointers is null, and another is not null */
  struct clist *list;
  struct expr *expr;
};

struct array_tail *array_tail_from_expr(struct expr *expr);
struct array_tail *array_tail_from_list(struct clist *list);

enum atom_kind {
  NAME_KIND = 1, INT_KIND = 2, FLOAT_KIND = 3, STRING_KIND = 4, BOOL_KIND = 5,
  SELF_KIND = 6, NULL_KIND = 7, EXP_KIND = 8, NEW_PRIMITIVE_KIND = 9,
  ARRAY_KIND, ANONYOUS_FUNC_KIND,
  ATTRIBUTE_KIND, SUBSCRIPT_KIND, CALL_KIND, INTF_IMPL_KIND
};

struct atom {
  enum atom_kind kind;
  union {
    struct {
      char *id;
      int type;
    } name;
    int64_t ival;
    float64_t fval;
    char *str;
    int bval;
    struct expr *exp;
    struct {
      struct type *type;
      /* one of pointers is null, and another is not null */
      struct clist *dim_list;
      struct clist *tail_list;
    } array;
    struct {
      struct clist *plist;
      struct clist *rlist;
      struct clist *body;
    } anonyous_func;
    /* Trailer */
    struct {
      struct atom *atom;
      char *id;
      int type;
    } attribute;
    struct {
      struct atom *atom;
      struct expr *index;
    } subscript;
    struct {
      struct atom *atom;
      struct clist *list;   /* expression list */
    } call;
    struct {
      struct atom *atom;
      struct clist *list;   /* method implementation list */
    } intf_impl;
  } v;
  struct list_head link;
};

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
  ATOM_KIND = 1, UNARY_KIND = 2, BINARY_KIND = 3,
};

struct expr {
  enum expr_kind kind;
  union {
    struct atom *atom;
    struct {
      enum unary_op_kind op;
      struct expr *operand;
    } unary_op;
    struct {
      struct expr *left;
      enum operator_kind op;
      struct expr *right;
    } bin_op;
  } v;
  struct list_head link;
};

struct atom *trailer_from_attribute(char *id);
struct atom *trailer_from_subscript(struct expr *idx);
struct atom *trailer_from_call(struct clist *para);
struct atom *atom_from_name(char *id);
struct atom *atom_from_int(int64_t ival);
struct atom *atom_from_float(float64_t fval);
struct atom *atom_from_string(char *str);
struct atom *atom_from_bool(int bval);
struct atom *atom_from_self(void);
struct atom *atom_from_expr(struct expr *exp);
struct atom *atom_from_null(void);
struct atom *atom_from_array(struct type *type, int tail, struct clist *list);
struct atom *atom_from_anonymous_func(struct clist *plist,
                                      struct clist *rlist,
                                      struct clist *body);

struct expr *expr_from_atom_trailers(struct clist *list, struct atom *atom);
struct expr *expr_from_atom(struct atom *atom);
struct expr *expr_for_binary(enum operator_kind kind,
                             struct expr *left, struct expr *right);
struct expr *expr_for_unary(enum unary_op_kind kind, struct expr *expr);

void expr_traverse(struct expr *exp);

struct var {
  struct list_head link;
  char *id;
  struct type *type;
};

struct var *new_var(char *id);
struct var *new_var_with_type(char *id, struct type *type);
#define var_set_type(v, t) ((v)->type = (t))

struct if_expr {
  struct expr *test;
  struct clist *body;
  struct list_head link;
};

struct if_expr *new_if_expr(struct expr *test, struct clist *body);

enum assign_operator {
  OP_PLUS_ASSIGN = 1, OP_MINUS_ASSIGN = 2,
  OP_MULT_ASSIGN, OP_DIV_ASSIGN,
  OP_MOD_ASSIGN, OP_AND_ASSIGN, OP_OR_ASSIGN, OP_XOR_ASSIGN,
  OP_RSHIFT_ASSIGN, OP_LSHIFT_ASSIGN,
};

enum stmt_kind {
  EMPTY_KIND = 1, IMPORT_KIND = 2, EXPR_KIND = 3, VARDECL_KIND = 4,
  FUNCDECL_KIND = 5, ASSIGN_KIND = 6, COMPOUND_ASSIGN_KIND = 7,
  STRUCT_KIND = 8, INTF_KIND = 9, TYPEDEF_KIND = 10, SEQ_KIND = 11,
  RETURN_KIND, IF_KIND, WHILE_KIND, BREAK_KIND, CONTINUE_KIND,
};

struct stmt {
  enum stmt_kind kind;
  union {
    struct {
      char *alias;
      char *path;
    } import;
    struct expr *expr;
    struct {
      int bconst;
      struct clist *var_list;
      struct clist *expr_list;
    } vardecl;
    struct {
      char *id;
      struct clist *plist;
      struct clist *rlist;
      struct clist *body;
    } funcdecl;
    struct {
      struct clist *left_list;
      struct clist *right_list;
    } assign;
    struct {
      struct expr *left;
      enum assign_operator op;
      struct expr *right;
    } compound_assign;
    struct {
      char *id;
      struct clist *list;
    } structure;
    struct {
      char *id;
      struct type *type;
    } user_typedef;
    struct clist *seq;
    struct {
      struct if_expr *if_part;
      struct clist *elseif_list;
      struct if_expr *else_part;
    } if_stmt;
    struct {
      int btest;
      struct expr *test;
      struct clist *body;
    } while_stmt;
  } v;
  struct list_head link;
};

struct stmt *stmt_from_expr(struct expr *expr);
struct stmt *stmt_from_import(char *alias, char *path);
struct stmt *stmt_from_vardecl(struct clist *varlist,
                               struct clist *initlist,
                               int bconst, struct type *type);
struct stmt *stmt_from_funcdecl(char *id, struct clist *plist,
                               struct clist *rlist, struct clist *body);
struct stmt *stmt_from_assign(struct clist *left_list,
                              struct clist *right_list);
struct stmt *stmt_from_compound_assign(struct expr *left,
                                       enum assign_operator op,
                                       struct expr *right);
struct stmt *stmt_from_seq(struct clist *list);
struct stmt *stmt_from_return(struct clist *list);
struct stmt *stmt_from_empty(void);
struct stmt *stmt_from_structure(char *id, struct clist *list);
struct stmt *stmt_from_interface(char *id, struct clist *list);
struct stmt *stmt_from_typedef(char *id, struct type *type);
struct stmt *stmt_from_jump(int kind);
struct stmt *stmt_from_if(struct if_expr *if_part, struct clist *elseif_list,
                          struct if_expr *else_part);
struct stmt *stmt_from_while(struct expr *test, struct clist *body, int b);

enum member_kind {
  FIELD_KIND = 1, METHOD_KIND = 2, INTF_FUNCDECL_KIND = 3,
};

struct member {
  enum member_kind kind;
  char *id;
  union {
    struct {
      struct type *type;
      struct expr *expr;
    } field;
    struct {
      struct clist *plist;
      struct clist *rlist;
      struct clist *body;
    } method;
    struct {
      struct clist *tlist;
      struct clist *rlist;
    } intf_funcdecl;
  } v;
  struct list_head link;
};

struct member *new_structure_vardecl(char *id, struct type *t, struct expr *e);
struct member *new_structure_funcdecl(char *id, struct clist *plist,
                                      struct clist *rlist,
                                      struct clist *body);
struct member *new_interface_funcdecl(char *id, struct clist *tlist,
                                      struct clist *rlist);
struct mod {
  struct clist *imports;
  struct clist *stmts;
};

struct mod *new_mod(struct clist *imports, struct clist *stmts);
void mod_traverse(struct mod *mod);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_AST_H_ */
