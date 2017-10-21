
#include "gstate.h"
#include "namei.h"
#include "module_object.h"

struct global_state gstate;

int gstate_get_module(char *mod_name, struct object **ob, int *index)
{
  struct hash_table *table = gstate.mod_table;

  struct namei ni = {
    .name = mod_name,
    .type = NT_MODULE,
  };

  struct hash_node *hnode = hash_table_find(table, &ni);
  if (hnode == NULL) {
    struct object *mo = load_module(mod_name);
    if (mo == NULL) {
      fprintf(stderr, "[ERROR]cannot find module %s\n", mod_name);
      return -1;
    }
    if (ob != NULL) *ob = mo;
    int res = gstate_add_module(mod_name, mo, index);
    assert(!res);
    return 0;
  }

  if (ob != NULL)
    *ob = vector_get(gstate.mod_vec, hnode_to_namei(hnode)->offset);
  return 0;
}

int gstate_add_module(char *mod_name, struct object *ob, int *index)
{
  assert(OB_KLASS(ob) == &module_klass);
  struct namei *ni = new_module_namei(mod_name);
  assert(ni);
  int res = hash_table_insert(gstate.mod_table, &ni->hnode);
  if (res) {
    fprintf(stderr, "add module '%s' failed\n", mod_name);
    free_namei(ni);
    return -1;
  }
  ni->offset = ++gstate.nr_mods;
  if (index != NULL) *index = ni->offset;
  res = vector_set(gstate.mod_vec, ni->offset, ob);
  assert(!res);
  return 0;
}

void init_gstate(void)
{
  gstate.nr_mods   = 0;
  gstate.mod_table = hash_table_create(namei_hash, namei_equal);
  gstate.mod_vec   = vector_create();
}
