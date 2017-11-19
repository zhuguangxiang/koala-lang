
#include "tableobject.h"
#include "tupleobject.h"
#include "kstate.h"

struct entry {
  struct hash_node hnode;
  TValue key;
  TValue val;
};

static int entry_equal(void *k1, void *k2)
{
  TValue *tv1 = k1;
  TValue *tv2 = k2;

  if (tval_isint(tv1) && tval_isint(tv2)) {
    return !Ineger_Compare(tv1, tv2);
  }

  if (tval_isname(tv1) && tval_isname(tv2)) {
    return !Name_Compare(tv1, tv2);
  }

  if (tval_isobject(tv1) && tval_isobject(tv2)) {
    if (ob_klass_equal(TVAL_OVAL(tv1), TVAL_OVAL(tv2))) {
      return OB_KLASS(TVAL_OVAL(tv1))->ob_cmp(tv1, tv2);
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
  TValue *tv = k;

  if (tval_isint(tv)) {
    return hash_uint32((uint32)TVAL_IVAL(tv), 0);
  }

  if (tval_isname(tv)) {
    return Name_Hash(tv);
  }

  if (tval_isobject(tv)) {
    return OB_KLASS(TVAL_OVAL(tv))->ob_hash(tv);
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

TValue *Table_Get(Object *ob, TValue *key)
{
  assert(OB_KLASS(ob) == &Table_Klass);

  TableObject *table = (TableObject *)ob;
  struct hash_node *hnode = hash_table_find(&table->table, key);
  if (hnode != NULL) {
    struct entry *e = container_of(hnode, struct entry, hnode);
    return &e->val;
  } else {
    return NULL;
  }
}

int Table_Put(Object *ob, TValue *key, TValue *value)
{
  assert(OB_KLASS(ob) == &Table_Klass);

  TableObject *table = (TableObject *)ob;
  struct entry *e = new_entry(key, value);
  int res = hash_table_insert(&table->table, &e->hnode);
  if (res) {
    free_entry(e);
    return -1;
  } else {
    return 0;
  }
}

/*-------------------------------------------------------------------------*/

static Object *table_init(Object *ob, Object *args)
{
  assert(OB_KLASS(ob) == &Table_Klass);
  return NULL;
}

static Object *table_get(Object *ob, Object *args)
{
  TValue *key;
  if (!(key = Tuple_Get(args, 0))) return NULL;
  return Tuple_Pack(Table_Get(ob, key));
}

static Object *table_put(Object *ob, Object *args)
{
  Object *tuple = Tuple_Get_Slice(args, 0, 1);
  if (!tuple) return NULL;

  TValue *key = Tuple_Get(tuple, 0);
  TValue *value = Tuple_Get(tuple, 1);
  TValue v = {.type = TYPE_INT, .ival = Table_Put(ob, key, value)};
  return Tuple_Pack(&v);
}

static MethodStruct table_methods[] = {
  {
    "__init__",
    "(V)(V)",
    ACCESS_PRIVATE,
    table_init
  },
  {
    "Put",
    "(Okoala/lang.Any;Okoala/lang.Any;)(I)",
    ACCESS_PUBLIC,
    table_put
  },
  {
    "Get",
    "(Okoala/lang.Any;)(Okoala/lang.Any;)",
    ACCESS_PUBLIC,
    table_get
  },
  {NULL, NULL, 0, NULL}
};

void Init_Table_Klass(void)
{
  Klass_Add_Methods(&Table_Klass, table_methods);
}

/*-------------------------------------------------------------------------*/

static void table_visit(struct hlist_head *hlist, int size, void *arg)
{
  struct entry *e;
  struct hash_node *pos;
  Object *ob;
  for (int i = 0; i < size; i++) {
    hlist_for_each_entry(pos, hlist + i, link) {
      e = container_of(pos, struct entry, hnode);
      if (tval_isobject(&e->key)) {
        ob = TVAL_OVAL(&e->key);
        OB_KLASS(ob)->ob_mark(ob);
      }

      if (tval_isobject(&e->val)) {
        ob = TVAL_OVAL(&e->val);
        OB_KLASS(ob)->ob_mark(ob);
      }
    }
  }
}

static void table_mark(Object *ob)
{
  assert(OB_KLASS(ob) == &Table_Klass);
  TableObject *table = (TableObject *)ob;
  hash_table_traverse(&table->table, table_visit, NULL);
}

static Object *table_alloc(Klass *klazz, int num)
{
  assert(klazz == &Table_Klass);
  return Table_New();
}

static void table_fini(struct hash_node *hnode, void *arg)
{
  struct entry *e = container_of(hnode, struct entry, hnode);
  if (tval_isname(&e->key)) {
    Name_Free(e->key.name);
  }

  if (tval_isname(&e->val)) {
    Name_Free(e->val.name);
  }

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