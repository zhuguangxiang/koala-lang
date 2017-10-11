
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "ast.h"

struct list_head *new_list(void)
{
  struct list_head *list = malloc(sizeof(*list));
  init_list_head(list);
  return list;
}

struct atom *trailer_from_attribute(char *id)
{
  struct atom *atom = malloc(sizeof(*atom));
  atom->kind = ATTRIBUTE_KIND;
  atom->v.attribute.atom = NULL;
  atom->v.attribute.id   = id;
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
      break;
    case INTF_IMPL_KIND:
    default:
      assert(0);
      break;
  }
}

struct atom *atom_from_id(char *id)
{
  struct atom *atom = malloc(sizeof(*atom));
  atom->kind = ID_KIND;
  atom->v.id = id;
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


void atom_traverse(struct atom *atom)
{
  switch (atom->kind) {
    case ID_KIND: {
      /*
      id scope
      method:
      local variable -> field variable -> module variable
      -> external module name
      function:
      module variable -> external module name
      */
      printf("[id]\n");
      printf("%s\n", atom->v.id);
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
      printf("%s\n", atom->v.attribute.id);
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
