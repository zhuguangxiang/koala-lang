
#include "compile.h"

int init_compiler(struct compiler *cp)
{
  cp->stmts = Vector_Create();
  cp->scope = 0;
  init_list_head(&cp->scopes);
  return 0;
}

int fini_compiler(struct compiler *cp)
{
  Vector_Destroy(cp->stmts, NULL, NULL);
  return 0;
}

static void scope_enter(struct compiler *cp)
{

}

static void scope_exit(struct compiler *cp)
{

}

int compile(struct compiler *cp)
{
  scope_enter(cp);

  scope_exit(cp);

  return 0;
}
