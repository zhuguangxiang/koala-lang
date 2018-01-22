
#include "compile.h"

int init_compiler(struct compiler *cp)
{
  //cp->stable =
  cp->scope = 0;
  init_list_head(&cp->scopes);
  return 0;
}

int fini_compiler(struct compiler *cp)
{
  return 0;
}

static void scope_enter(struct compiler *cp)
{

}

static void scope_exit(struct compiler *cp)
{

}

int compile(struct compiler *cp, Vector *stmts)
{
  return 0;
}
