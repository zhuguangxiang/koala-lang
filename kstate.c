
#include "kstate.h"
#include "debug.h"
#include "tableobject.h"
#include "moduleobject.h"

/*-------------------------------------------------------------------------*/

struct mod_entry {
  HashNode hnode;
  char *path;
  Object *mo;
};

static struct mod_entry *new_mod_entry(char *path, Object *mo)
{
  struct mod_entry *e = malloc(sizeof(*e));
  init_hash_node(&e->hnode, e);
  e->path = path;
  e->mo = mo;
  return e;
}

static void free_mod_entry(struct mod_entry *e)
{
  ASSERT(hash_node_unhashed(&e->hnode));
  free(e);
}

static uint32 mod_entry_hash(void *k)
{
  struct mod_entry *e = k;
  return hash_string(e->path);
}

static int mod_entry_equal(void *k1, void *k2)
{
  struct mod_entry *e1 = k1;
  struct mod_entry *e2 = k2;
  return !strcmp(e1->path, e2->path);
}

static void mod_entry_fini(HashNode *hnode, void *arg)
{
  struct mod_entry *e = container_of(hnode, struct mod_entry, hnode);
  free_mod_entry(e);
}

#define ATOM_ITEM_MAX   (ITEM_CONST + 1)

static uint32 htable_hash(void *key)
{
  ItemEntry *e = key;
  ASSERT(e->type > 0 && e->type < ATOM_ITEM_MAX);
  item_hash_t hash_fn = item_func[e->type].ihash;
  ASSERT(hash_fn != NULL);
  return hash_fn(e->data);
}

static int htable_equal(void *k1, void *k2)
{
  ItemEntry *e1 = k1;
  ItemEntry *e2 = k2;
  ASSERT(e1->type > 0 && e1->type < ATOM_ITEM_MAX);
  ASSERT(e2->type > 0 && e2->type < ATOM_ITEM_MAX);
  if (e1->type != e2->type) return 0;
  item_equal_t equal_fn = item_func[e1->type].iequal;
  ASSERT(equal_fn != NULL);
  return equal_fn(e1->data, e2->data);
}

/*-------------------------------------------------------------------------*/

void KState_Init(KoalaState *ks)
{
  HashTable_Init(&ks->modules, mod_entry_hash, mod_entry_equal);

  ks->itable = ItemTable_Create(htable_hash, htable_equal, ATOM_ITEM_MAX);

  ks->nr_rts = 0;
  init_list_head(&ks->rt_list);
}

void KState_Fini(KoalaState *ks)
{
  HashTable_Fini(&ks->modules, mod_entry_fini, ks);
}

KoalaState *KState_Create(void)
{
  KoalaState *ks = malloc(sizeof(*ks));
  ASSERT_PTR(ks);
  KState_Init(ks);
  return ks;
}

/*-------------------------------------------------------------------------*/

int KState_Add_Module(KoalaState *ks, char *path, Object *mo)
{
  ASSERT(OB_KLASS(mo) == &Module_Klass);
  struct mod_entry *e = new_mod_entry(path, mo);
  if (HashTable_Insert(&ks->modules, &e->hnode) < 0) {
    debug_error("add module '%s' failed\n", path);
    free_mod_entry(e);
    return -1;
  }
  ((ModuleObject *)mo)->itable = ks->itable;
  return 0;
}

Object *KState_Find_Module(KoalaState *ks, char *path)
{
  struct mod_entry e = {.path = path};
  HashNode *hnode = HashTable_Find(&ks->modules, &e);
  if (hnode == NULL) return NULL;
  return (Object *)(container_of(hnode, struct mod_entry, hnode)->mo);
}
