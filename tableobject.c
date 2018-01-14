
#include "tableobject.h"
#include "tupleobject.h"
#include "moduleobject.h"
#include "symbol.h"
#include "debug.h"

struct entry {
  HashNode hnode;
  TValue key;
  TValue val;
};

static uint32 Integer_Hash(TValue *v)
{
  return hash_uint32((uint32)VALUE_INT(v), 0);
}

static int Ineger_Compare(TValue *v1, TValue *v2)
{
  return VALUE_INT(v1) - VALUE_INT(v2);
}

static int entry_equal(void *k1, void *k2)
{
  TValue *v1 = k1;
  TValue *v2 = k2;

  if (VALUE_ISINT(v1) && VALUE_ISINT(v2)) {
    return !Ineger_Compare(v1, v2);
  }

  if (VALUE_ISOBJECT(v1) && VALUE_ISOBJECT(v2)) {
    Object *o1 = VALUE_OBJECT(v1);
    Object *o2 = VALUE_OBJECT(v2);
    if (OB_KLASS_EQUAL(o1, o2)) {
      return OB_KLASS(o1)->ob_cmp(v1, v2);
    } else {
      debug_warn("the two key types are not the same.\n");
      return 0;
    }
  }

  debug_warn("unsupported type as table's key\n");
  return 0;
}

static uint32 entry_hash(void *k)
{
  TValue *v = k;

  if (VALUE_ISINT(v)) {
    return Integer_Hash(v);
  }

  if (VALUE_ISOBJECT(v)) {
    return OB_KLASS(VALUE_OBJECT(v))->ob_hash(v);
  }

  debug_warn("unsupported type for hashing\n");
  return 0;
}

static struct entry *new_entry(TValue *key, TValue *value)
{
  struct entry *entry = malloc(sizeof(*entry));
  entry->key = *key;
  entry->val = *value;
  init_hash_node(&entry->hnode, &entry->key);
  return entry;
}

static void free_entry(struct entry *e) {
  free(e);
}

/*-------------------------------------------------------------------------*/

Object *Table_New(void)
{
  TableObject *table = malloc(sizeof(*table));
  init_object_head(table, &Table_Klass);
  int res = HashTable_Init(&table->table, entry_hash, entry_equal);
  ASSERT(!res);
  //Object_Add_GCList(table);
  return (Object *)table;
}

int Table_Get(Object *ob, TValue *key, TValue *rk, TValue *rv)
{
  OB_ASSERT_KLASS(ob, Table_Klass);
  TableObject *table = (TableObject *)ob;
  HashNode *hnode = HashTable_Find(&table->table, key);
  if (hnode != NULL) {
    struct entry *e = container_of(hnode, struct entry, hnode);
    if (rk != NULL) *rk = e->key;
    if (rv != NULL) *rv = e->val;
    return 0;
  } else {
    return -1;
  }
}

int Table_Put(Object *ob, TValue *key, TValue *value)
{
  OB_ASSERT_KLASS(ob, Table_Klass);
  TableObject *table = (TableObject *)ob;
  struct entry *e = new_entry(key, value);
  int res = HashTable_Insert(&table->table, &e->hnode);
  if (res) {
    free_entry(e);
    return -1;
  } else {
    return 0;
  }
}

int Table_Count(Object *ob)
{
  OB_ASSERT_KLASS(ob, Table_Klass);
  TableObject *table = (TableObject *)ob;
  return HashTable_Count(&table->table);
}

struct visit_struct {
  table_visit_func visit;
  void *arg;
};

static void table_visit(struct hlist_head *head, int size, void *arg)
{
  struct visit_struct *vs = arg;
  HashNode *hnode;
  struct entry *entry;
  for (int i = 0; i < size; i++) {
    hash_list_for_each(hnode, head) {
      entry = container_of(hnode, struct entry, hnode);
      vs->visit(&entry->key, &entry->val, vs->arg);
    }
    ++head;
  }
}

void Table_Traverse(Object *ob, table_visit_func visit, void *arg)
{
  OB_ASSERT_KLASS(ob, Table_Klass);
  TableObject *table = (TableObject *)ob;
  struct visit_struct vs = {visit, arg};
  HashTable_Traverse(&table->table, table_visit, &vs);
}

/*-------------------------------------------------------------------------*/

static Object *__table_get(Object *ob, Object *args)
{
  TValue key, val;
  if (Tuple_Get(args, 0, &key) < 0) return NULL;
  if (VALUE_ISNIL(&key)) return NULL;
  if (Table_Get(ob, &key, NULL, &val)) return NULL;
  return Tuple_From_Va_TValues(1, &val);
}

static Object *__table_put(Object *ob, Object *args)
{
  Object *tuple = Tuple_Get_Slice(args, 0, 1);
  if (!tuple) return NULL;
  TValue key, val;
  if (Tuple_Get(tuple, 0, &key) < 0) return NULL;
  if (Tuple_Get(tuple, 1, &val) < 0) return NULL;
  return Tuple_Build("i", Table_Put(ob, &key, &val));
}

static FunctionStruct table_functions[] = {
  {"Put", "i", "AA", ACCESS_PUBLIC, __table_put},
  {"Get", "A", "A", ACCESS_PUBLIC, __table_get},
  {NULL}
};

void Init_Table_Klass(Object *ob)
{
  ModuleObject *mo = (ModuleObject *)ob;
  Table_Klass.itable = mo->itable;
  Klass_Add_CFunctions(&Table_Klass, table_functions);
  Module_Add_Class(ob, &Table_Klass,  ACCESS_PUBLIC);
}

/*-------------------------------------------------------------------------*/

static void entry_visit(TValue *key, TValue *val, void *arg)
{
  UNUSED_PARAMETER(arg);
  Object *ob;

  if (VALUE_ISOBJECT(key)) {
    ob = VALUE_OBJECT(key);
    OB_KLASS(ob)->ob_mark(ob);
  }

  if (VALUE_ISOBJECT(val)) {
    ob = VALUE_OBJECT(val);
    OB_KLASS(ob)->ob_mark(ob);
  }
}

static void table_mark(Object *ob)
{
  Table_Traverse(ob, entry_visit, NULL);
}

static void table_finalize(struct hash_node *hnode, void *arg)
{
  UNUSED_PARAMETER(arg);
  struct entry *e = container_of(hnode, struct entry, hnode);
  free_entry(e);
}

static void table_free(Object *ob)
{
  OB_ASSERT_KLASS(ob, Table_Klass);
  TableObject *table = (TableObject *)ob;
  HashTable_Fini(&table->table, table_finalize, NULL);
  free(ob);
}

Klass Table_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass),
  .name  = "Table",
  .bsize = sizeof(TableObject),

  .ob_mark  = table_mark,
  .ob_free  = table_free,
};
