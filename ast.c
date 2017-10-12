
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "ast.h"
#include "namei.h"

struct list_head *new_list(void)
{
  struct list_head *list = malloc(sizeof(*list));
  init_list_head(list);
  return list;
}

void free_list(struct list_head *list)
{
  assert(list_empty(list));
  free(list);
}

struct atom *trailer_from_attribute(char *id)
{
  struct atom *atom = malloc(sizeof(*atom));
  atom->kind = ATTRIBUTE_KIND;
  atom->v.attribute.atom = NULL;
  atom->v.attribute.id   = id;
  atom->v.attribute.type = 0;
  init_list_head(&atom->link);
  return atom;
}

struct atom *trailer_from_subscript(struct expr *idx)
{
  struct atom *atom = malloc(sizeof(*atom));
  atom->kind = SUBSCRIPT_KIND;
  atom->v.subscript.atom = NULL;
  atom->v.subscript.index = idx;
  init_list_head(&atom->link);
  return atom;
}

struct atom *trailer_from_call(struct list_head *para)
{
  struct atom *atom = malloc(sizeof(*atom));
  atom->kind = CALL_KIND;
  atom->v.call.atom = NULL;
  atom->v.call.list = para;
  init_list_head(&atom->link);
  return atom;
}

static void trailer_set_left_atom(struct atom *atom, struct atom *left_atom)
{
  switch (atom->kind) {
    case ATTRIBUTE_KIND:
      atom->v.attribute.atom = left_atom;
      break;
    case SUBSCRIPT_KIND:
      atom->v.subscript.atom = left_atom;
      break;
    case CALL_KIND:
      atom->v.call.atom = left_atom;
      if (left_atom->kind == NAME_KIND) {
        left_atom->v.name.type = NT_FUNC;
      } else if (left_atom->kind == ATTRIBUTE_KIND) {
        left_atom->v.attribute.type = NT_FUNC;
      } else {
        printf("[DEBUG] call left_atom kind:%d\n", left_atom->kind);
      }
      break;
    case INTF_IMPL_KIND:
    default:
      assert(0);
      break;
  }
}

struct atom *atom_from_name(char *id)
{
  struct atom *atom = malloc(sizeof(*atom));
  atom->kind        = NAME_KIND;
  atom->v.name.id   = id;
  atom->v.name.type = 0;
  init_list_head(&atom->link);
  return atom;
}

struct atom *atom_from_int(int64_t ival)
{
  struct atom *atom = malloc(sizeof(*atom));
  atom->kind   = INT_KIND;
  atom->v.ival = ival;
  init_list_head(&atom->link);
  return atom;
}

struct atom *atom_from_float(float64_t fval)
{
  struct atom *atom = malloc(sizeof(*atom));
  atom->kind   = FLOAT_KIND;
  atom->v.fval = fval;
  init_list_head(&atom->link);
  return atom;
}

struct atom *atom_from_string(char *str)
{
  struct atom *atom = malloc(sizeof(*atom));
  atom->kind  = STRING_KIND;
  atom->v.str = str;
  init_list_head(&atom->link);
  return atom;
}

struct atom *atom_from_bool(int bval)
{
  struct atom *atom = malloc(sizeof(*atom));
  atom->kind   = BOOL_KIND;
  atom->v.bval = bval;
  init_list_head(&atom->link);
  return atom;
}

struct atom *atom_from_self(void)
{
  struct atom *atom = malloc(sizeof(*atom));
  atom->kind = SELF_KIND;
  init_list_head(&atom->link);
  return atom;
}

struct atom *atom_from_expr(struct expr *exp)
{
  struct atom *atom = malloc(sizeof(*atom));
  atom->kind  = EXP_KIND;
  atom->v.exp = exp;
  init_list_head(&atom->link);
  return atom;
}

struct atom *atom_from_null(void)
{
  struct atom *atom = malloc(sizeof(*atom));
  atom->kind = NULL_KIND;
  init_list_head(&atom->link);
  return atom;
}

struct expr *expr_for_atom(struct list_head *list, struct atom *atom)
{
  struct atom *trailer, *temp;
  atom_foreach_safe(trailer, list, temp) {
    list_del(&trailer->link);
    trailer_set_left_atom(trailer, atom);
    atom = trailer;
  }

  return expr_from_atom(atom);
}

struct expr *expr_from_atom(struct atom *atom)
{
  struct expr *exp = malloc(sizeof(*exp));
  exp->kind   = ATOM_KIND;
  exp->v.atom = atom;
  init_list_head(&exp->link);
  return exp;
}

struct stmt *stmt_from_expr(struct expr *expr)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind   = EXPR_KIND;
  stmt->v.expr = expr;
  init_list_head(&stmt->link);
  return stmt;
}

struct stmt *stmt_from_import(char *alias, char *path)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind   = IMPORT_KIND;
  stmt->v.import.alias = alias;
  stmt->v.import.path  = path;
  init_list_head(&stmt->link);
  return stmt;
}

struct mod *new_mod(struct list_head *imports, struct list_head *stmts)
{
  struct mod *mod = malloc(sizeof(*mod));
  mod->imports = imports;
  mod->stmts   = stmts;
  return mod;
}

void atom_traverse(struct atom *atom)
{
  switch (atom->kind) {
    case NAME_KIND: {
      /*
      id scope
      method:
      local variable -> field variable -> module variable
      -> external module name
      function:
      module variable -> external module name
      */
      printf("[id]\n");
      printf("%s, %s\n", atom->v.name.id, ni_type_string(atom->v.name.type));
      break;
    }
    case INT_KIND: {
      printf("[integer]\n");
      printf("%lld\n", atom->v.ival);
      break;
    }
    case FLOAT_KIND: {
      printf("[float]\n");
      printf("%f\n", atom->v.fval);
      break;
    }
    case STRING_KIND: {
      printf("[string]\n");
      printf("%s\n", atom->v.str);
      break;
    }
    case BOOL_KIND: {
      printf("[boolean]\n");
      printf("%s\n", atom->v.bval ? "true" : "false");
      break;
    }
    case SELF_KIND: {
      printf("[self]\n");
      break;
    }
    case NULL_KIND: {
      printf("[null]\n");
      break;
    }
    case EXP_KIND: {
      printf("[sub-expr]\n");
      expr_traverse(atom->v.exp);
      break;
    }
    case NEW_PRIMITIVE_KIND: {
      printf("[new primitive object]\n");
      break;
    }
    case ATTRIBUTE_KIND: {
      atom_traverse(atom->v.attribute.atom);
      printf("[attribute]\n");
      printf("%s, %s\n", atom->v.attribute.id,
             ni_type_string(atom->v.attribute.type));
      break;
    }
    case CALL_KIND: {
      atom_traverse(atom->v.call.atom);
      printf("[func call]\n");
      printf("paras:\n");
      struct expr *expr;
      expr_foreach(expr, atom->v.call.list) {
        expr_traverse(expr);
      }
      break;
    }
    default: {
      assert(0);
      break;
    }
  }
}

void expr_traverse(struct expr *exp)
{
  switch (exp->kind) {
    case ATOM_KIND: {
      atom_traverse(exp->v.atom);
      break;
    }
    default: {
      assert(0);
      break;
    }
  }
}

void stmt_traverse(struct stmt *stmt)
{
  switch (stmt->kind) {
    case IMPORT_KIND: {
      printf("[import]\n");
      printf("%s:%s\n", stmt->v.import.alias, stmt->v.import.path);
      break;
    }
    case EXPR_KIND: {
      printf("[expr]\n");
      expr_traverse(stmt->v.expr);
      break;
    }
    default:{
      assert(0);
    }
  }
}

void mod_traverse(struct mod *mod)
{
  struct stmt *stmt;
  stmt_foreach(stmt, mod->imports) {
    stmt_traverse(stmt);
  }
  stmt_foreach(stmt, mod->stmts) {
    stmt_traverse(stmt);
  }
}
