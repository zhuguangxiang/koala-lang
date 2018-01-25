
#include "compile.h"

int init_compiler(struct compiler *cp)
{
  Koala_Init();
  cp->stmts = Vector_Create();
  cp->module = (ModuleObject *)Module_New(cp->package, "/", 0);
  cp->scope = 0;
  init_list_head(&cp->scopes);
  return 0;
}

int fini_compiler(struct compiler *cp)
{
  Koala_Fini();
  return 0;
}

static void scope_enter(struct compiler *cp)
{

}

static void scope_exit(struct compiler *cp)
{

}

static struct scope *get_scope(struct compiler *cp)
{
  if (cp->scope == 0) return NULL;
  struct list_head *first = list_first(&cp->scopes);
  if (first != NULL) return container_of(first, struct scope, link);
  ASSERT(0);
  return NULL;
}

static char *typestring(struct type *t)
{
  switch (t->kind) {
    case PRIMITIVE_KIND: {
      if (t->primitive == PRIMITIVE_INT) return "i";
      else if (t->primitive == PRIMITIVE_FLOAT) return "f";
      else if (t->primitive == PRIMITIVE_BOOL) return "z";
      else if (t->primitive == PRIMITIVE_STRING) return "s";
      else if (t->primitive == PRIMITIVE_ANY) return "A";
      else return NULL;
      break;
    }
    default: {
      assert(0);
    }
  }
}

void add_variable(struct compiler *cp, struct var *var, int bconst)
{
  char *desc;
  if (var->type != NULL) {
    desc = typestring(var->type);
  }
  STable_Add_Var(&cp->module->stable, var->id, desc, bconst);
}

void add_variables(struct stmt *stmt, struct compiler *cp)
{
  ASSERT(stmt->kind == VARDECL_KIND);
  int bconst = stmt->vardecl.bconst;
  Vector *vec = stmt->vardecl.var_seq;
  struct var *var;
  for (int i = 0; i < Vector_Size(vec); i++) {
    var = Vector_Get(vec, i);
    ASSERT_PTR(var);
    add_variable(cp, var, bconst);
  }
}

int import_stmt_handler(struct stmt *stmt, struct compiler *cp)
{
  return 0;
}

int vardecl_stmt_handler(struct stmt *stmt, struct compiler *cp)
{
  if (cp->count == 1) {
    add_variables(stmt, cp);
    return 0;
  } else if (cp->count == 2) {
    return 0;
  } else {
    ASSERT_MSG("cannot 3rd time for compiler\n");
    return 0;
  }
}

int funcdecl_stmt_handler(struct stmt *stmt, struct compiler *cp)
{
  if (cp->count == 1) {
    return 0;
  } else if (cp->count == 2) {
    return 0;
  } else {
    ASSERT_MSG("cannot 3rd time for compiler\n");
    return 0;
  }
}

typedef int (*stmt_handler_t)(struct stmt *, struct compiler *);

static stmt_handler_t stmt_handlers[] = {
  NULL, /* INVALID */
  NULL, /* EMPTY_KIND */
  import_stmt_handler,
  NULL,
  vardecl_stmt_handler,
  funcdecl_stmt_handler,
};

int stmt_handler(struct stmt *stmt, struct compiler *cp)
{
  ASSERT(stmt->kind > 0 && stmt->kind < STMT_KIND_MAX);
  stmt_handler_t handler = stmt_handlers[stmt->kind];
  ASSERT_PTR(handler);
  return handler(stmt, cp);
}

static void symbol_display(struct compiler *cp)
{
  printf("package:%s\n", cp->package);
  STable_Display(&cp->module->stable);
}

int compile(struct compiler *cp)
{
  struct stmt *stmt;
  Vector *vec = cp->stmts;
  printf("-----------------------\n");
  cp->count = 1;
  for (int i = 0; i < Vector_Size(vec); i++) {
    stmt = Vector_Get(vec, i);
    stmt_handler(stmt, cp);
  }
  cp->count = 2;
  for (int i = 0; i < Vector_Size(vec); i++) {
    stmt = Vector_Get(vec, i);
    stmt_handler(stmt, cp);
  }

  symbol_display(cp);

  return 0;
}
