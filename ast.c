
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "ast.h"
#include "namei.h"

struct clist *new_clist(void)
{
  struct clist *list = malloc(sizeof(*list));
  init_list_head(&list->head);
  list->count = 0;
  return list;
}

void free_clist(struct clist *list)
{
  assert(clist_empty(list));
  free(list);
}

struct type *type_from_primitive(int primitive)
{
  struct type *type = malloc(sizeof(*type));
  type->kind = PRIMITIVE_KIND;
  type->v.primitive = primitive;
  return type;
}

struct type *type_from_userdef(char *mod_name, char *type_name)
{
  struct type *type = malloc(sizeof(*type));
  type->kind = USERDEF_TYPE;
  type->v.userdef.mod_name  = mod_name;
  type->v.userdef.type_name = type_name;
  return type;
}

struct array_tail *array_tail_from_expr(struct expr *expr)
{
  struct array_tail *tail = malloc(sizeof(*tail));
  init_list_head(&tail->link);
  tail->list   = NULL;
  tail->expr   = expr;
  return tail;
}

struct array_tail *array_tail_from_list(struct clist *list)
{
  struct array_tail *tail = malloc(sizeof(*tail));
  init_list_head(&tail->link);
  tail->list   = list;
  tail->expr   = NULL;
  return tail;
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

struct atom *trailer_from_call(struct clist *para)
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

struct atom *atom_from_array(struct type *type, int tail, struct clist *list)
{
  struct atom *atom = malloc(sizeof(*atom));
  atom->kind = ARRAY_KIND;
  atom->v.array.type = type;
  if (tail) {
    atom->v.array.tail_list = list;
    atom->v.array.dim_list  = NULL;
  } else {
    atom->v.array.tail_list = NULL;
    atom->v.array.dim_list  = list;
  }
  init_list_head(&atom->link);
  return atom;
}

struct expr *expr_for_atom(struct clist *list, struct atom *atom)
{
  struct atom *trailer, *temp;
  atom_foreach_safe(trailer, list, temp) {
    clist_del(&trailer->link, list);
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
  stmt->kind = IMPORT_KIND;
  if (alias == NULL) {
    char *s = strrchr(path, '/');
    if (s == NULL)
      s = path;
    else
      s += 1;
    char *tmp = malloc(strlen(s) + 1);
    strcpy(tmp, s);
    stmt->v.import.alias = tmp;
  } else {
    stmt->v.import.alias = alias;
  }
  stmt->v.import.path  = path;
  init_list_head(&stmt->link);
  return stmt;
}

struct var *new_var(char *id)
{
  struct var *v = malloc(sizeof(*v));
  v->id = id;
  init_list_head(&v->link);
  return v;
}

void free_var(struct var *v)
{
  free(v);
}

int vars_add_symtable(struct clist *list, int bconst, struct type *type)
{
  return 0;
}

struct stmt *stmt_from_assign(struct clist *varlist,
                              struct clist *initlist,
                              int bdecl, int bconst, struct type *type)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = ASSIGN_KIND;
  stmt->v.assign.bdecl    = bdecl;
  stmt->v.assign.bconst   = bconst;
  stmt->v.assign.type     = type;
  stmt->v.assign.varlist  = varlist;
  stmt->v.assign.exprlist = initlist;
  init_list_head(&stmt->link);
  return stmt;
}

struct mod *new_mod(struct clist *imports, struct clist *stmts)
{
  struct mod *mod = malloc(sizeof(*mod));
  mod->imports = imports;
  mod->stmts   = stmts;
  return mod;
}

void type_traverse(struct type *type)
{
  printf("type_kind:%d\n", type->kind);
  printf("type_dims:%d\n", type->dims);
}

void array_tail_traverse(struct array_tail *tail)
{
  if (tail->list) {
    printf("subarray,length:%d\n", tail->list->count);
    assert(!tail->expr);
    int count = 0;
    struct array_tail *t;
    array_tail_foreach(t, tail->list) {
      count++;
      array_tail_traverse(t);
    }
    printf("real subcount222:%d\n", count);
  } else {
    printf("tail expr\n");
    assert(tail->expr);
    expr_traverse(tail->expr);
    printf("tail expr end\n");
  }
}

void array_traverse(struct atom *atom)
{
  type_traverse(atom->v.array.type);
  if (atom->v.array.tail_list != NULL) {
    printf("array tail list, length:%d\n", atom->v.array.tail_list->count);
    int count = 0;
    struct array_tail *tail;
    array_tail_foreach(tail, atom->v.array.tail_list) {
      count++;
      printf("start subarray\n");
      array_tail_traverse(tail);
      printf("end subarray\n");
    }
    printf("real count:%d\n", count);
  } else {
    printf("array dim list, length:%d\n", atom->v.array.dim_list->count);
  }
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
    case ARRAY_KIND: {
      printf("[new array]\n");
      array_traverse(atom);
      break;
    }
    case ATTRIBUTE_KIND: {
      atom_traverse(atom->v.attribute.atom);
      printf("[attribute]\n");
      printf("%s, %s\n", atom->v.attribute.id,
             ni_type_string(atom->v.attribute.type));
      break;
    }
    case SUBSCRIPT_KIND: {
      atom_traverse(atom->v.subscript.atom);
      printf("[subscript]\n");
      expr_traverse(atom->v.subscript.index);
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
    case INTF_IMPL_KIND: {
      printf("[anonymous interface impl]\n");
      break;
    }
    default: {
      printf("[ERROR] unknown atom kind :%d\n", atom->kind);
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

void assign_traverse(struct stmt *stmt)
{
  printf("variable declaration ? %s\n",
         stmt->v.assign.bdecl ? "true" : "false");
  printf("const variable ? %s\n",
         stmt->v.assign.bconst ? "true" : "false");
  if (stmt->v.assign.type != NULL)
    type_traverse(stmt->v.assign.type);

  printf("variables name:\n");
  struct var *var;
  var_foreach(var, stmt->v.assign.varlist) {
    printf("%s ", var->id);
  }
  putchar('\n');

  struct expr *expr;
  expr_foreach(expr, stmt->v.assign.exprlist) {
    expr_traverse(expr);
  }

  printf("end variable declaration\n");
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
    case ASSIGN_KIND: {
      printf("[ASSIGN]\n");
      assign_traverse(stmt);
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
