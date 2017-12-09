
#include "hash.h"
#include "nameobject.h"

static void parse_desc(TypeList *tlist, char *desc)
{
  char *t = desc;
  char ch;
  int idx = 0;
  int is_arr = 0;

  while ((ch = *t)) {
    if (ch == 'I' || ch == 'F' || ch == 'Z' || ch == 'S') {
      assert(idx < tlist->size);

      if (is_arr) {
        tlist->index[idx].size = 2;
        is_arr = 0;
      } else {
        tlist->index[idx].offset = t - desc;
        tlist->index[idx].size = 1;
      }

      ++idx;
      ++t;
    } else if (ch == 'O') {
      assert(idx < tlist->size);

      if (!is_arr) {
        tlist->index[idx].offset = t - desc;
      }

      int cnt = 0;
      while (ch != ';') {
        ++cnt;
        ch = *++t;
      }

      if (is_arr) {
        tlist->index[idx].size = cnt + 1;
        is_arr = 0;
      } else {
        tlist->index[idx].size = cnt;
      }

      ++idx;
      ++t;
    } else if (ch == '[') {
      assert(idx < tlist->size);
      is_arr = 1;
      tlist->index[idx].offset = t - desc;
      ++t;
    } else {
      fprintf(stderr, "unknown type:%c\n", ch);
      assert(0);
    }
  }
}

static int count_desc(char *desc)
{
  char *t = desc;
  char ch;
  int cnt = 0;

  while ((ch = *t)) {
    if (ch == 'I' || ch == 'F' || ch == 'Z' || ch == 'S') {
      ++cnt;
      ++t;
    } else if (ch == 'O') {
      while (ch != ';') {
        ch = *++t;
      }
      ++t;
      ++cnt;
    } else if (ch == '[') {
      ++t;
    } else {
      fprintf(stderr, "unknown type:%c\n", ch);
      assert(0);
    }
  }

  return cnt;
}

static TypeList *typelist_new(char *desc)
{
  int size = 0;
  size = count_desc(desc);
  if (size > 0) --size;
  TypeList *tlist = malloc(sizeof(*tlist) + sizeof(TypeIndex) *size);
  tlist->desc = desc;
  tlist->size = size + 1;
  parse_desc(tlist, desc);
  return tlist;
}

static void typelist_free(TypeList *tlist)
{
  if (tlist != NULL) free(tlist);
}

Object *Name_New(char *name, uint8 type, uint8 access,
                 char *desc, char *pdesc)
{
  int size = sizeof(NameObject);
  if (pdesc) size += sizeof(TypeList **);

  NameObject *n = malloc(size);
  init_object_head(n, &Name_Klass);
  n->name = name;
  n->type = type;
  n->access = access;
  n->size = 0;
  n->tlist[0] = NULL;

  if (desc) {
    n->size = 1;
    n->tlist[0] = typelist_new(desc);
  }

  if (pdesc) {
    n->size = 2;
    n->tlist[1] = typelist_new(pdesc);
  }

  Object_Add_GCList((Object *)n);
  return (Object *)n;
}

void Name_Free(Object *ob)
{
  OB_CHECK_KLASS(ob, Name_Klass);
  NameObject *no = (NameObject *)ob;

  for (int i = 0; i < no->size; i++)
    typelist_free(no->tlist[i]);

  free(ob);
}

/*-------------------------------------------------------------------------*/

static MethodStruct name_methods[] = {
  {NULL, NULL, NULL, 0, NULL}
};

void Init_Name_Klass(void)
{
  Klass_Add_Methods(&Name_Klass, name_methods);
}

/*-------------------------------------------------------------------------*/

static int name_equal(TValue v1, TValue v2)
{
  Object *ob1 = TVAL_OBJECT(v1);
  Object *ob2 = TVAL_OBJECT(v2);
  assert((OB_KLASS(ob1) == &Name_Klass) && (OB_KLASS(ob2) == &Name_Klass));
  NameObject *n1 = (NameObject *)ob1;
  NameObject *n2 = (NameObject *)ob2;
  return !strcmp(n1->name, n2->name);
}

static uint32 name_hash(TValue v)
{
  Object *ob = TVAL_OBJECT(v);
  assert(OB_KLASS(ob) == &Name_Klass);
  NameObject *n = (NameObject *)ob;
  return hash_string(n->name);
}

static void name_free(Object *ob)
{
  Name_Free(ob);
}

Klass Name_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass),
  .name  = "Name",
  .bsize = sizeof(NameObject),

  .ob_free = name_free,

  .ob_hash = name_hash,
  .ob_cmp  = name_equal
};

/*-------------------------------------------------------------------------*/

static inline char *to_str(char *str[], int size, int idx)
{
  return (idx >= 0 && idx < size) ? str[idx] : "";
}

static char *type_tostr(int type)
{
  static char *str[] = {
    "", "const", "var", "func", "class", "interface", "var", "func"
  };

  return to_str(str, nr_elts(str), type);
}

static char *access_tostr(int access)
{
  static char *str[] = {
    "public", "private"
  };

  return to_str(str, nr_elts(str), access);
}

void Name_Display(Object *ob)
{
  OB_CHECK_KLASS(ob, Name_Klass);
  NameObject *no = (NameObject *)ob;
  fprintf(stdout, "%-16s%-16s%-24s%-16s\n",
          no->name, type_tostr(no->type), access_tostr(no->access), "");
}
