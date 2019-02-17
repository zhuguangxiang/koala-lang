
#include "atomstring.h"
#include "pkgnode.h"

static void fini_string_leafnode_func(void *data, void *arg)
{
  int flag = 0;
  flag = flag || !strcmp((char *)data, "HELLO");
  flag = flag || !strcmp((char *)data, "IO");
  flag = flag || !strcmp((char *)data, "LANG");
  assert(flag != 0);
}

void test_pkgnode(void)
{
  PkgState ps;
  Init_PkgState(&ps);
  Add_PkgNode(&ps, "github.com/koala/hello", "HELLO");
  Add_PkgNode(&ps, "github.com/koala/io//", "IO");
  Add_PkgNode(&ps, "github.com/koala///lang", "LANG");
  PkgNode *node = Find_PkgNode(&ps, "github.com/koala/");
  assert(node != NULL);
  node = Find_PkgNode(&ps, "github.com/koala/net///");
  assert(node == NULL);
  Show_PkgState(&ps);
  Fini_PkgState(&ps, fini_string_leafnode_func, NULL);
}

int main(int argc, char *argv[])
{
  AtomString_Init();
  test_pkgnode();
  AtomString_Fini();
  return 0;
}
