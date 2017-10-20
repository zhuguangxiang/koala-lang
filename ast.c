
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
  init_list_head(&type->link);
  return type;
}

struct type *type_from_userdef(char *mod_name, char *type_name)
{
  struct type *type = malloc(sizeof(*type));
  type->kind = USERDEF_TYPE;
  type->v.userdef.mod_name  = mod_name;
  type->v.userdef.type_name = type_name;
  init_list_head(&type->link);
  return type;
}

struct type *type_from_functype(struct clist *tlist, struct clist *rlist)
{
  struct type *type = malloc(sizeof(*type));
  type->kind = FUNCTION_TYPE;
  type->v.functype.tlist = tlist;
  type->v.functype.rlist  = rlist;
  init_list_head(&type->link);
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

struct atom *atom_from_anonymous_func(struct clist *plist,
                                      struct clist *rlist,
                                      struct clist *body)
{
  struct atom *atom = malloc(sizeof(*atom));
  atom->kind = ANONYOUS_FUNC_KIND;
  atom->v.anonyous_func.plist = plist;
  atom->v.anonyous_func.rlist = rlist;
  atom->v.anonyous_func.body  = body;
  init_list_head(&atom->link);
  return atom;
}

struct expr *expr_from_atom_trailers(struct clist *list, struct atom *atom)
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

struct expr *expr_for_binary(enum operator_kind kind,
                             struct expr *left, struct expr *right)
{
  struct expr *exp = malloc(sizeof(*exp));
  exp->kind = BINARY_KIND;
  exp->v.bin_op.left  = left;
  exp->v.bin_op.op    = kind;
  exp->v.bin_op.right = right;
  init_list_head(&exp->link);
  return exp;
}

struct expr *expr_for_unary(enum unary_op_kind kind, struct expr *expr)
{
  struct expr *exp = malloc(sizeof(*exp));
  exp->kind = UNARY_KIND;
  exp->v.unary_op.op = kind;
  exp->v.unary_op.operand = expr;
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
  v->id   = id;
  v->type = NULL;
  init_list_head(&v->link);
  return v;
}

void free_var(struct var *v)
{
  free(v);
}

struct var *new_var_with_type(char *id, struct type *type)
{
  struct var *v = malloc(sizeof(*v));
  v->id   = id;
  v->type = type;
  init_list_head(&v->link);
  return v;
}

int vars_add_symtable(struct clist *list, int bconst, struct type *type)
{
  return 0;
}

struct stmt *stmt_from_vardecl(struct clist *varlist,
                               struct clist *initlist,
                               int bconst, struct type *type)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = VARDECL_KIND;
  stmt->v.vardecl.bconst    = bconst;
  stmt->v.vardecl.var_list  = varlist;
  stmt->v.vardecl.expr_list = initlist;
  init_list_head(&stmt->link);
  struct var *var;
  var_foreach(var, varlist) {
    var_set_type(var, type);
  }
  return stmt;
}

struct stmt *stmt_from_funcdecl(char *id, struct clist *plist,
                                struct clist *rlist, struct clist *body)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = FUNCDECL_KIND;
  stmt->v.funcdecl.id    = id;
  stmt->v.funcdecl.plist = plist;
  stmt->v.funcdecl.rlist = rlist;
  stmt->v.funcdecl.body  = body;
  init_list_head(&stmt->link);
  return stmt;
}

struct stmt *stmt_from_assign(struct clist *left_list,
                              struct clist *right_list)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = ASSIGN_KIND;
  stmt->v.assign.left_list  = left_list;
  stmt->v.assign.right_list = right_list;
  init_list_head(&stmt->link);
  return stmt;
}

struct stmt *stmt_from_compound_assign(struct expr *left,
                                       enum assign_operator op,
                                       struct expr *right)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = COMPOUND_ASSIGN_KIND;
  stmt->v.compound_assign.left  = left;
  stmt->v.compound_assign.op    = op;
  stmt->v.compound_assign.right = right;
  init_list_head(&stmt->link);
  return stmt;
}

struct stmt *stmt_from_seq(struct clist *list)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind  = SEQ_KIND;
  stmt->v.seq = list;
  init_list_head(&stmt->link);
  return stmt;
}

struct stmt *stmt_from_return(struct clist *list)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind  = RETURN_KIND;
  stmt->v.seq = list;
  init_list_head(&stmt->link);
  return stmt;
}

struct stmt *stmt_from_empty(void)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = EMPTY_KIND;
  init_list_head(&stmt->link);
  return stmt;
}

struct stmt *stmt_from_structure(char *id, struct clist *list)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = STRUCT_KIND;
  stmt->v.structure.id   = id;
  stmt->v.structure.list = list;
  init_list_head(&stmt->link);
  return stmt;
}

struct stmt *stmt_from_interface(char *id, struct clist *list)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = INTF_KIND;
  stmt->v.structure.id   = id;
  stmt->v.structure.list = list;
  init_list_head(&stmt->link);
  return stmt;
}

struct stmt *stmt_from_typedef(char *id, struct type *type)
{
  struct stmt *stmt = malloc(sizeof(*stmt));
  stmt->kind = TYPEDEF_KIND;
  stmt->v.user_typedef.id   = id;
  stmt->v.user_typedef.type = type;
  init_list_head(&stmt->link);
  return stmt;
}

struct member *new_structure_vardecl(char *id, struct type *t, struct expr *e)
{
  struct member *member = malloc(sizeof(*member));
  member->id   = id;
  member->kind = FIELD_KIND;
  member->v.field.type = t;
  member->v.field.expr = e;
  init_list_head(&member->link);
  return member;
}

struct member *new_structure_funcdecl(char *id, struct clist *plist,
                                      struct clist *rlist,
                                      struct clist *body)
{
  struct member *member = malloc(sizeof(*member));
  member->id   = id;
  member->kind = METHOD_KIND;
  member->v.method.plist = plist;
  member->v.method.rlist = rlist;
  member->v.method.body  = body;
  init_list_head(&member->link);
  return member;
}

struct member *new_interface_funcdecl(char *id, struct clist *tlist,
                                      struct clist *rlist)
{
  struct member *member = malloc(sizeof(*member));
  member->id   = id;
  member->kind = INTF_FUNCDECL_KIND;
  member->v.intf_funcdecl.tlist = tlist;
  member->v.intf_funcdecl.rlist = rlist;
  init_list_head(&member->link);
  return member;
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
  if (type != NULL) {
    printf("type_kind:%d\n", type->kind);
    printf("type_dims:%d\n", type->dims);
  } else {
    printf("no type declared\n");
  }
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
    case ANONYOUS_FUNC_KIND: {
      printf("[anonymous function]\n");
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
    case INTF_IMPL_KIND:
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
    case UNARY_KIND: {
      printf("unary expr,op:%d\n", exp->v.unary_op.op);
      expr_traverse(exp->v.unary_op.operand);
      break;
    }
    case BINARY_KIND: {
      printf("binary expr,op:%d\n", exp->v.bin_op.op);
      expr_traverse(exp->v.bin_op.left);
      expr_traverse(exp->v.bin_op.right);
      break;
    }
    default: {
      assert(0);
      break;
    }
  }
}

void vardecl_traverse(struct stmt *stmt)
{
  printf("variable declaration\n");
  printf("const variable ? %s\n",
         stmt->v.vardecl.bconst ? "true" : "false");
  printf("variables name:\n");
  struct var *var;
  var_foreach(var, stmt->v.vardecl.var_list) {
    printf("%s ", var->id);
    type_traverse(var->type);
  }
  putchar('\n');

  struct expr *expr;
  expr_foreach(expr, stmt->v.vardecl.expr_list) {
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
    case VARDECL_KIND: {
      printf("[var decl]\n");
      vardecl_traverse(stmt);
      break;
    }
    case FUNCDECL_KIND: {
      printf("[functin decl]:%s\n", stmt->v.funcdecl.id);
      break;
    }
    case ASSIGN_KIND: {
      printf("[assignment list]\n");
      struct expr *expr;
      expr_foreach(expr, stmt->v.assign.left_list) {
        expr_traverse(expr);
      }

      expr_foreach(expr, stmt->v.assign.right_list) {
        expr_traverse(expr);
      }
      break;
    }
    case COMPOUND_ASSIGN_KIND: {
      printf("[compound assignment]:%d\n", stmt->v.compound_assign.op);
      expr_traverse(stmt->v.compound_assign.left);
      expr_traverse(stmt->v.compound_assign.right);
      break;
    }
    case STRUCT_KIND: {
      printf("[structure]:%s\n", stmt->v.structure.id);
      break;
    }
    case INTF_KIND: {
      printf("[interface]:%s\n", stmt->v.structure.id);
      break;
    }
    case TYPEDEF_KIND: {
      printf("[typedef]:%s\n", stmt->v.user_typedef.id);
      break;
    }
    default:{
      printf("[ERROR] unknown stmt kind :%d\n", stmt->kind);
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
