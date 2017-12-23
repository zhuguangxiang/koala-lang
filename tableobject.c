
#include "tableobject.h"
#include "tupleobject.h"
#include "nameobject.h"

struct entry {
  struct hash_node hnode;
  TValue key;
  TValue val;
};

static int entry_equal(void *k1, void *k2)
{
  TValue v1 = *(TValue *)k1;
  TValue v2 = *(TValue *)k2;

  if (TVAL_ISINT(v1) && TVAL_ISINT(v2)) {
    return !Ineger_Compare(v1, v2);
  }

  if (TVAL_ISOBJ(v1) && TVAL_ISOBJ(v2)) {
    if (OB_KLASS_EQUAL(OBJECT_TVAL(v1), OBJECT_TVAL(v2))) {
      return OB_KLASS(OBJECT_TVAL(v1))->ob_cmp(v1, v2);
    } else {
      fprintf(stderr, "[WARN] the two key types are not the same.\n");
      return 0;
    }
  }

  fprintf(stderr, "[WARN] unsupported type as table's key\n");
  return 0;
}

static uint32 entry_hash(void *k)
{
  TValue v = *(TValue *)k;

  if (TVAL_ISINT(v)) {
    return Integer_Hash(v);
  }

  if (TVAL_ISOBJ(v)) {
    return OB_KLASS(OBJECT_TVAL(v))->ob_hash(v);
  }

  fprintf(stderr, "[WARN] unsupported type for hashing\n");
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
  int res = hash_table_init(&table->table, entry_hash, entry_equal);
  assert(!res);
  Object_Add_GCList((Object *)table);
  return (Object *)table;
}

int Table_Get(Object *ob, TValue key, TValue *rk, TValue *rv)
{
  assert(OB_KLASS(ob) == &Table_Klass);

  TableObject *table = (TableObject *)ob;
  struct hash_node *hnode = hash_table_find(&table->table, &key);
  if (hnode != NULL) {
    struct entry *e = container_of(hnode, struct entry, hnode);
    if (rk != NULL) *rk = e->key;
    if (rv != NULL) *rv = e->val;
    return 0;
  } else {
    return -1;
  }
}

int Table_Put(Object *ob, TValue key, TValue value)
{
  assert(OB_KLASS(ob) == &Table_Klass);

  TableObject *table = (TableObject *)ob;
  struct entry *e = new_entry(&key, &value);
  int res = hash_table_insert(&table->table, &e->hnode);
  if (res) {
    free_entry(e);
    return -1;
  } else {
    return 0;
  }
}

struct visit_struct {
  Table_Visit visit;
  void *arg;
};

static void table_visit(struct hlist_head *head, int size, void *arg)
{
  struct visit_struct *vs = arg;
  struct hash_node *hnode;
  struct entry *entry;
  for (int i = 0; i < size; i++) {
    hash_list_for_each(hnode, head) {
      entry = container_of(hnode, struct entry, hnode);
      vs->visit(entry->key, entry->val, vs->arg);
    }
    ++head;
  }
}

void Table_Traverse(Object *ob, Table_Visit visit, void *arg)
{
  assert(OB_KLASS(ob) == &Table_Klass);
  TableObject *table = (TableObject *)ob;
  struct visit_struct vs = {visit, arg};
  hash_table_traverse(&table->table, table_visit, &vs);
}

/*-------------------------------------------------------------------------*/

static Object *__table_init(Object *ob, Object *args)
{
  UNUSED_PARAMETER(args);
  assert(OB_KLASS(ob) == &Table_Klass);
  return Tuple_Build("z", TRUE);
}

static Object *__table_get(Object *ob, Object *args)
{
  TValue key, v;
  key = Tuple_Get(args, 0);
  if (TVAL_ISANY(key)) return NULL;
  if (Table_Get(ob, key, NULL, &v)) return NULL;
  return Tuple_From_Va_TValues(1, v);
}

static Object *__table_put(Object *ob, Object *args)
{
  Object *tuple = Tuple_Get_Slice(args, 0, 1);
  if (!tuple) return NULL;

  TValue key = Tuple_Get(tuple, 0);
  TValue value = Tuple_Get(tuple, 1);
  return Tuple_Build("i", Table_Put(ob, key, value));
}

static MethodStruct table_methods[] = {
  {
    "__init__",
    "Z",
    NULL,
    ACCESS_PRIVATE,
    __table_init
  },
  {
    "Put",
    "I",
    "TT",
    ACCESS_PUBLIC,
    __table_put
  },
  {
    "Get",
    "T",
    "T",
    ACCESS_PUBLIC,
    __table_get
  },
  {NULL, NULL, NULL, 0, NULL}
};

void Init_Table_Klass(void)
{
  Klass_Add_Methods(&Table_Klass, table_methods);
}

/*-------------------------------------------------------------------------*/

static void entry_visit(TValue key, TValue val, void *arg)
{
  UNUSED_PARAMETER(arg);
  Object *ob;

  if (TVAL_ISOBJ(key)) {
    ob = OBJECT_TVAL(key);
    OB_KLASS(ob)->ob_mark(ob);
  }

  if (TVAL_ISOBJ(val)) {
    ob = OBJECT_TVAL(val);
    OB_KLASS(ob)->ob_mark(ob);
  }
}

static void table_mark(Object *ob)
{
  Table_Traverse(ob, entry_visit, NULL);
}

static Object *table_alloc(Klass *klazz, int num)
{
  UNUSED_PARAMETER(num);
  assert(klazz == &Table_Klass);
  return Table_New();
}

static void table_fini(struct hash_node *hnode, void *arg)
{
  UNUSED_PARAMETER(arg);
  struct entry *e = container_of(hnode, struct entry, hnode);
  free_entry(e);
}

static void table_free(Object *ob)
{
  assert(OB_KLASS(ob) == &Table_Klass);
  TableObject *table = (TableObject *)ob;
  hash_table_fini(&table->table, table_fini, NULL);
  free(ob);
}

Klass Table_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass),
  .name  = "Table",
  .bsize = sizeof(TableObject),

  .ob_mark  = table_mark,

  .ob_alloc = table_alloc,
  .ob_free  = table_free,
};
