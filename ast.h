
#ifndef _KOALA_AST_H_
#define _KOALA_AST_H_

#include "types.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

enum atom_kind {
  NAME_KIND = 1, INT_KIND = 2, FLOAT_KIND = 3, STRING_KIND = 4, BOOL_KIND = 5,
  SELF_KIND = 6, NULL_KIND = 7, EXP_KIND = 8, NEW_PRIMITIVE_KIND = 9,
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
      struct list_head *list;   /* expression list */
    } call;
    struct {
      struct atom *atom;
      struct list_head *list;   /* method implementation list */
    } intf_impl;
  } v;
  struct list_head link;
};

#define atom_foreach_safe(pos, head, temp) \
  list_for_each_entry_safe(pos, temp, head, link)

enum operator_kind {
  ADD = 1, SUB = 2, MULT = 3, DIV = 4, MOD = 5
};

enum expr_kind {
  ATOM_KIND = 1, BINARY_KIND = 2, UNARY_KIND = 3,
};

struct expr {
  enum expr_kind kind;
  union {
    struct atom *atom;
    struct {
      struct expr *left;
      enum operator_kind op;
      struct expr *right;
    } bin_op;
  } v;
  struct list_head link;
};

#define expr_foreach(pos, list) \
  if ((list) != NULL) list_for_each_entry(pos, list, link)

struct list_head *new_list(void);
void free_list(struct list_head *list);

struct atom *trailer_from_attribute(char *id);
struct atom *trailer_from_subscript(struct expr *idx);
struct atom *trailer_from_call(struct list_head *para);
struct atom *atom_from_name(char *id);
struct atom *atom_from_int(int64_t ival);
struct atom *atom_from_float(float64_t fval);
struct atom *atom_from_string(char *str);
struct atom *atom_from_bool(int bval);
struct atom *atom_from_self(void);
struct atom *atom_from_expr(struct expr *exp);
struct atom *atom_from_null(void);
struct expr *expr_for_atom(struct list_head *list, struct atom *atom);
struct expr *expr_from_atom(struct atom *atom);
void expr_traverse(struct expr *exp);

enum stmt_kind {
  IMPORT_KIND = 1, EXPR_KIND = 2,
};

struct stmt {
  enum stmt_kind kind;
  union {
    struct {
      char *alias;
      char *path;
    } import;
    struct expr *expr;
  } v;
  struct list_head link;
};

#define stmt_foreach(stmt, list) \
  if ((list) != NULL) list_for_each_entry(stmt, list, link)

struct stmt *stmt_from_expr(struct expr *expr);
struct stmt *stmt_from_import(char *alias, char *path);

struct mod {
  struct list_head *imports;
  struct list_head *stmts;
};

struct mod *new_mod(struct list_head *imports, struct list_head *stmts);
void mod_traverse(struct mod *mod);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_AST_H_ */
