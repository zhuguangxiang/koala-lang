
#include "typedesc.h"
#include "log.h"

TypeDesc Byte_Type = {
  .kind = TYPE_PRIMITIVE,
  .primitive = PRIMITIVE_BYTE
};
TypeDesc Char_Type = {
  .kind = TYPE_PRIMITIVE,
  .primitive = PRIMITIVE_CHAR
};
TypeDesc Int_Type = {
  .kind = TYPE_PRIMITIVE,
  .primitive = PRIMITIVE_INT
};
TypeDesc Float_Type = {
  .kind = TYPE_PRIMITIVE,
  .primitive = PRIMITIVE_FLOAT
};
TypeDesc Bool_Type = {
  .kind = TYPE_PRIMITIVE,
  .primitive = PRIMITIVE_BOOL
};
TypeDesc String_Type = {
  .kind = TYPE_PRIMITIVE,
  .primitive = PRIMITIVE_STRING
};
TypeDesc Any_Type = {
  .kind = TYPE_PRIMITIVE,
  .primitive = PRIMITIVE_ANY
};
TypeDesc Varg_Type = {
  .kind = TYPE_PRIMITIVE,
  .primitive = PRIMITIVE_VARG
};

struct primitive_type_s {
  int primitive;
  char *str;
  TypeDesc *type;
} primitive_types[] = {
  {PRIMITIVE_BYTE,   "byte",   &Byte_Type   },
  {PRIMITIVE_CHAR,   "char",   &Char_Type   },
  {PRIMITIVE_INT,    "int",    &Int_Type    },
  {PRIMITIVE_FLOAT,  "float",  &Float_Type  },
  {PRIMITIVE_BOOL,   "bool",   &Bool_Type   },
  {PRIMITIVE_STRING, "string", &String_Type },
  {PRIMITIVE_ANY,    "any",    &Any_Type    },
  {PRIMITIVE_VARG,   "...",    &Varg_Type   }
};

static struct primitive_type_s *get_primitive(int kind)
{
  struct primitive_type_s *pt;
  for (int i = 0; i < nr_elts(primitive_types); i++) {
    pt = primitive_types + i;
    if (kind == pt->primitive)
      return pt;
  }
  return NULL;
}

static TypeDesc *type_new(int kind)
{
  TypeDesc *desc = calloc(1, sizeof(TypeDesc));
  desc->kind = kind;
  return desc;
}

static TypeDesc *cnstring_to_usrdef(char *str, int len)
{
  char *dot = strchr(str, '.');
  assert(dot);
  TypeDesc *desc = type_new(TYPE_USRDEF);
  desc->usrdef.path = strndup(str, dot - str);
  desc->usrdef.type = strndup(dot + 1, str + len - dot - 1);
  return desc;
}

/*---------------------------------------------------------------------------*/

static int primitive_equal(TypeDesc *t1, TypeDesc *t2)
{
  return t1->primitive == t2->primitive;
}

static void primitive_tostring(TypeDesc *t, char *buf)
{
  strcpy(buf, get_primitive(t->primitive)->str);
}

static TypeDesc *primitive_dup(TypeDesc *desc)
{
  /* the same */
  return desc;
}

static void usrdef_free(TypeDesc *t)
{
  if (t->usrdef.path) free(t->usrdef.path);
  assert(t->usrdef.type); free(t->usrdef.type);
  free(t);
}

static int usrdef_equal(TypeDesc *t1, TypeDesc *t2)
{
  int eq = !strcmp(t1->usrdef.type, t2->usrdef.type);
  if (eq) {
    if (t1->usrdef.path && t2->usrdef.path) {
      eq = !strcmp(t1->usrdef.path, t2->usrdef.path);
    } if (!t1->usrdef.path && !t2->usrdef.path) {
      eq = 1;
    } else {
      eq = 0;
    }
  }
  return eq;
}

static void usrdef_tostring(TypeDesc *t, char *buf)
{
  sprintf(buf, "%s.%s", t->usrdef.path, t->usrdef.type);
}

static TypeDesc *usrdef_dup(TypeDesc *desc)
{
  /* usrdef is duplicated path and type internally */
  return Type_New_UsrDef(desc->usrdef.path, desc->usrdef.type);
}

static void __type_free_fn(void *item, void *arg)
{
  UNUSED_PARAMETER(arg);
  Type_Free(item);
}

static void proto_free(TypeDesc *t)
{
  Vector_Free(t->proto.arg, __type_free_fn, NULL);
  Vector_Free(t->proto.ret, __type_free_fn, NULL);
  free(t);
}

static int proto_equal(TypeDesc *t1, TypeDesc *t2)
{
  int eq = TypeList_Equal(t1->proto.arg, t2->proto.arg);
  if (eq) eq = TypeList_Equal(t1->proto.ret, t2->proto.ret);
  return eq;
}

static void proto_tostring(TypeDesc *t, char *buf)
{
  sprintf(buf, "%s.%s", t->usrdef.path, t->usrdef.type);
}

static TypeDesc *proto_dup(TypeDesc *desc)
{
  /* need deep copy */
  Vector *arg = NULL;
  Vector *ret = NULL;

  if (desc->proto.arg) {
    arg = Vector_New();
    TypeDesc *item;
    Vector_ForEach(item, desc->proto.arg)
      Vector_Append(arg, Type_Dup(item));
  }

  if (desc->proto.ret) {
    ret = Vector_New();
    TypeDesc *item;
    Vector_ForEach(item, desc->proto.ret)
      Vector_Append(ret, item);
  }

  return Type_New_Proto(arg, ret);
}

static void array_free(TypeDesc *t)
{
  Type_Free(t->array.base);
  free(t);
}

static int array_equal(TypeDesc *t1, TypeDesc *t2)
{
  UNUSED_PARAMETER(t1);
  UNUSED_PARAMETER(t2);
  return 0;
}

static void array_tostring(TypeDesc *t, char *buf)
{
  UNUSED_PARAMETER(t);
  UNUSED_PARAMETER(buf);
}

static TypeDesc *array_dup(TypeDesc *desc)
{
  /* need duplicate deeply */
  TypeDesc *base = Type_Dup(desc->array.base);
  return Type_New_Array(desc->array.dims, base);
}

static void map_free(TypeDesc *t)
{
  Type_Free(t->map.key);
  Type_Free(t->map.val);
  free(t);
}

static int map_equal(TypeDesc *t1, TypeDesc *t2)
{
  UNUSED_PARAMETER(t1);
  UNUSED_PARAMETER(t2);
  return 0;
}

static void map_tostring(TypeDesc *t, char *buf)
{
  UNUSED_PARAMETER(t);
  UNUSED_PARAMETER(buf);
}

static TypeDesc *map_dup(TypeDesc *desc)
{
  /* need duplicate deeply */
  TypeDesc *key = Type_Dup(desc->map.key);
  TypeDesc *val = Type_Dup(desc->map.val);
  return Type_New_Map(key, val);
}

struct type_ops_s {
  void (*__free)(TypeDesc *t);
  int (*__equal)(TypeDesc *t1, TypeDesc *t2);
  void (*__tostring)(TypeDesc *t, char *buf); /* NOTES: not safe */
  TypeDesc *(*__dup)(TypeDesc *t); /* deep copy */
} type_ops[] = {
  /* array[0] is not used */
  {NULL,        NULL,            NULL,               NULL         },
  /* Primitive needn't free */
  {NULL,        primitive_equal, primitive_tostring, primitive_dup},
  {usrdef_free, usrdef_equal,    usrdef_tostring,    usrdef_dup   },
  {proto_free,  proto_equal,     proto_tostring,     proto_dup    },
  {array_free,  array_equal,     array_tostring,     array_dup    },
  {map_free,    map_equal,       map_tostring,       map_dup      },
};

void Type_Free(TypeDesc *desc)
{
  int kind = desc->kind;
  assert(kind > 0 && kind < nr_elts(type_ops));
  struct type_ops_s *ops = type_ops + kind;
  if (ops->__free) ops->__free(desc);
}

/** deep copy */
TypeDesc *Type_Dup(TypeDesc *desc)
{
  int kind = desc->kind;
  assert(kind > 0 && kind < nr_elts(type_ops));
  struct type_ops_s *ops = type_ops + kind;
  return ops->__dup(desc);
}

TypeDesc *Type_New_UsrDef(char *path, char *type)
{
  TypeDesc *desc = type_new(TYPE_USRDEF);
  desc->usrdef.path = strdup(path);
  desc->usrdef.type = strdup(type);
  return desc;
}

TypeDesc *Type_New_Proto(Vector *arg, Vector *ret)
{
  TypeDesc *desc = type_new(TYPE_PROTO);
  desc->proto.arg = arg;
  desc->proto.ret = ret;
  return desc;
}

TypeDesc *Type_New_Array(int dims, TypeDesc *base)
{
  if (dims <= 0 || !base) return NULL;

  TypeDesc *desc = type_new(TYPE_ARRAY);
  desc->array.dims = dims;
  desc->array.base = base;
  return desc;
}

TypeDesc *Type_New_Map(TypeDesc *key, TypeDesc *val)
{
  TypeDesc *desc = type_new(TYPE_MAP);
  desc->map.key = key;
  desc->map.val = val;
  return desc;
}

int TypeList_Equal(Vector *v1, Vector *v2)
{
  if (v1 && v2) {
    if (Vector_Size(v1) != Vector_Size(v2)) return 0;
    TypeDesc *t1, *t2;
    Vector_ForEach(t1, v1) {
      t2 = Vector_Get(v2, i);
      if (!Type_Equal(t1, t2)) return 0;
    }
    return 1;
  } else if (!v1 && !v2) {
    return 1;
  } else {
    return 0;
  }
}

int Type_Equal(TypeDesc *t1, TypeDesc *t2)
{
  if (t1 == t2) return 1;
  if (t1->kind != t2->kind) return 0;
  int kind = t1->kind;
  assert(kind > 0 && kind < nr_elts(type_ops));
  struct type_ops_s *ops = type_ops + kind;
  return ops->__equal(t1, t2);
}

void Type_ToString(TypeDesc *desc, char *buf)
{
  if (!desc) return;
  int kind = desc->kind;
  assert(kind > 0 && kind < nr_elts(type_ops));
  struct type_ops_s *ops = type_ops + kind;
  ops->__tostring(desc, buf);
}

/**
 * "i[sOkoala/lang.Tuple;" -->>> int, []string, koala/lang.Tuple
 */
Vector *CString_To_TypeList(char *str)
{
  if (!str) return NULL;
  Vector *v = Vector_New();
  TypeDesc *desc;
  char ch;
  int dims = 0;

  while ((ch = *str) != '\0') {
    if (ch == '.') {
      assert(str[1] == '.' && str[2] == '.');
      desc = &Varg_Type;
      Vector_Append(v, desc);
      str += 3;
      assert(!*str);
    } else if (ch == 'O') {
      char *start = str + 1;
      while ((ch = *str) != ';') str++;
      desc = cnstring_to_usrdef(start, str - start);
      if (dims > 0) desc = Type_New_Array(dims, desc);
      Vector_Append(v, desc);
      dims = 0;
      str++;
    } else if (ch == '[') {
      while ((ch = *str) == '[') { dims++; str++; }
    } else if ((desc = get_primitive(ch)->type)) {
      if (dims > 0) desc = Type_New_Array(dims, desc);
      Vector_Append(v, desc);
      dims = 0;
      str++;
    } else {
      kassert(0, "unknown type:%c\n", ch);
    }
  }

  return v;
}
