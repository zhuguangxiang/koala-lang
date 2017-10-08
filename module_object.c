
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "module_object.h"
#include "namei.h"
#include "gstate.h"
#include "method_object.h"

static struct hash_table *__get_table(struct object *ob)
{
  struct module_object *mo = (struct module_object *)ob;
  struct hash_table *table = mo->table;
  if (!table)
    mo->table = table = hash_table_create(namei_hash, namei_equal);
  return table;
}

static struct vector *__get_vector(struct object *ob)
{
  struct module_object *mo = (struct module_object *)ob;
  struct vector *vec = mo->vec;
  if (!vec)
    mo->vec = vec = vector_create();
  return vec;
}

int module_add(struct object *mo, struct namei *ni, struct object *ob)
{
  assert(OB_KLASS(mo) == &module_klass);
  struct module_object *mod = (struct module_object *)mo;
  int res = hash_table_insert(__get_table(mo), &ni->hnode);
  assert(!res);
  ni->offset = ++mod->nr_objs;
  res = vector_set(__get_vector(mo), ni->offset, ob);
  assert(!res);
  return 0;
}

struct klass_object module_klass = {
  OBJECT_HEAD_INIT(&klass_klass),
  .name  = "Module",
  .bsize = sizeof(struct module_object),
};

int module_get(struct object *mo, struct namei *ni,
               struct object **ob, int *index)
{
  assert(OB_KLASS(mo) == &module_klass);

  struct hash_node *hnode = hash_table_find(__get_table(mo), ni);
  if (hnode == NULL) return -1;

  struct namei *temp = hnode_to_namei(hnode);
  if (index != NULL) *index = temp->offset;
  if (ob != NULL) *ob = vector_get(__get_vector(mo), temp->offset);
  return 0;
}

#if 0
/**
 * import_module - import one module
 * @ob: the module which needs import other modules.
 * @full_name: relative module file name with KOALA_PATH
 *
 * example:
 *  import "github.com/koala/io"(.kl/.klc)
 *  import "github.com/koala/regex"(.kl/.klc)
 */
int import_module(struct object *ob, char *full_name)
{
  char *mod_name = strrchr(full_name, '/');
  if (mod_name == NULL)
    mod_name = full_name;
  struct object *sym = new_symid(mod_name, SYMID_MODULE);
  SYMID_SIGNATURE(sym) = full_name;
  return talbe_put(__get_table(ob), sym, NULL);
}

/**
 * import_module - import all symbols from a module
 * @ob: the module which needs import other module's symbols.
 * @full_name: relative module file name with KOALA_PATH
 *
 * example:
 *  import myio "github.com/koala/io"
 *  import * "github.com/koala/regex"
 */
int import_module_symbols(struct object *ob, char *full_name)
{
  struct object *tbl = vm_get_modules();
  struct object *sym = new_symid(full_name, SYMID_MODULE);
  struct object *mod = table_get(tbl, sym);
  if (mod == NULL) {
    //mod = load_module_from_file();
    return -1;
  }

  //iterate all symbols and add to current module
  return 0;
}
#endif

int module_add_cfunctions(struct object *mo, struct cfunc_struct *funcs)
{
  struct namei *ni;
  struct object *fo;
  struct cfunc_struct *cf = funcs;
  while (cf->name != NULL) {
    ni = new_func_namei(cf->name, cf->signature, cf->access);
    fo = new_cfunc(cf->func);
    module_add(mo, ni, fo);
    ++cf;
  }

  return 0;
}

struct object *new_module(char *name)
{
  struct module_object *mod = malloc(sizeof(*mod));
  assert(mod);

  init_object_head(mod, &module_klass);
  mod->nr_objs = 0;
  mod->table   = NULL;
  mod->vec     = NULL;
  gstate_add_module(name, (struct object *)mod, NULL);

  return (struct object *)mod;
}

struct object *find_module(char *name)
{
  struct object *ob = NULL;
  int res = gstate_get_module(name, &ob, NULL);
  return (res == 0) ? ob : NULL;
}

struct object *load_module(char *mod_name)
{
  return NULL;
}

static void module_visit(struct hlist_head *head, int size, void *arg)
{
  struct hash_node *hnode;
  struct namei *ni;
  for (int i = 0; i < size; i++) {
    hash_list_for_each(hnode, head) {
      ni = hnode_to_namei(hnode);
      fprintf(stdout, "%-16s%-16s%-24s%-16s\n",
              ni->name, namei_type_string(ni),
              ni->signature, namei_access_string(ni));
    }
    ++head;
  }
}

void module_display(struct object *ob)
{
  assert(OB_KLASS(ob) == &module_klass);
  fprintf(stdout, "%-16s%-16s%-24s%-16s\n",
          "name", "type", "signature", "access");
  hash_table_traverse(__get_table(ob), module_visit, NULL);
}
