
#include "koala.h"
#include "log.h"
#include "mod_io.h"
#include "thread.h"

static HashTable modules;

struct mod_entry {
  HashNode hnode;
  char *path;
  Object *mo;
};

static struct mod_entry *new_mod_entry(char *path, Object *mo)
{
  struct mod_entry *e = malloc(sizeof(struct mod_entry));
  Init_HashNode(&e->hnode, e);
  e->path = path;
  e->mo = mo;
  return e;
}

static void free_mod_entry(struct mod_entry *e)
{
  ASSERT(HashNode_Unhashed(&e->hnode));
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
  UNUSED_PARAMETER(arg);
  struct mod_entry *e = container_of(hnode, struct mod_entry, hnode);
  free_mod_entry(e);
}

/*-------------------------------------------------------------------------*/

static int koala_add_module(char *path, Object *mo)
{
  struct mod_entry *e = new_mod_entry(path, mo);
  if (HashTable_Insert(&modules, &e->hnode) < 0) {
    error("add module '%s' failed", path);
    free_mod_entry(e);
    return -1;
  }
  return 0;
}

Object *Koala_New_Module(char *name, char *path)
{
  Object *ob = Module_New(name);
  if (koala_add_module(path, ob) < 0) {
    Module_Free(ob);
    return NULL;
  }
  return ob;
}

Object *Koala_Get_Module(char *path)
{
  struct mod_entry e = {.path = path};
  HashNode *hnode = HashTable_Find(&modules, &e);
  if (hnode == NULL) return NULL;
  return (Object *)(container_of(hnode, struct mod_entry, hnode)->mo);
}

static Object *__load_module_file(char *path)
{
  UNUSED_PARAMETER(path);
  return NULL;
}

Object *Koala_Load_Module(char *path)
{
  Object *ob = Koala_Get_Module(path);
  if (ob == NULL) return __load_module_file(path);
  return ob;
}

void Koala_Run_Module(Object *ob)
{
  UNUSED_PARAMETER(ob);
}

void Koala_Run_File(char *path)
{
  UNUSED_PARAMETER(path);
}

/*-------------------------------------------------------------------------*/

static void Init_Lang_Module(void)
{
  Object *ob = Koala_New_Module("lang", "koala/lang");
  ASSERT_PTR(ob);
  Module_Add_Class(ob, &Klass_Klass);
  Module_Add_Class(ob, &String_Klass);
  Module_Add_Class(ob, &Tuple_Klass);
  Module_Add_Class(ob, &Table_Klass);
  Module_Add_Class(ob, &Module_Klass);
  Module_Add_Class(ob, &Method_Klass);
  Init_String_Klass();
  Init_Tuple_Klass();
  Init_Table_Klass();
}

static void Init_Modules(void)
{
  HashInfo hashinfo;
  Init_HashInfo(&hashinfo, mod_entry_hash, mod_entry_equal);
  HashTable_Init(&modules, &hashinfo);

  /* koala/lang.klc */
  Init_Lang_Module();

  /* koala/io.klc */
  Init_IO_Module();
}

/*-------------------------------------------------------------------------*/

void Koala_Init(void)
{
  Init_Modules();
  sched_init();
  schedule();
}

void Koala_Fini(void)
{
  HashTable_Fini(&modules, mod_entry_fini, NULL);
}
