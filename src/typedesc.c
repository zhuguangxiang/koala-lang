/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "typedesc.h"
#include "mem.h"
#include "log.h"

TypeDesc Byte_Type   = {.kind = TYPE_BASIC, .basic = BASIC_BYTE};
TypeDesc Char_Type   = {.kind = TYPE_BASIC, .basic = BASIC_CHAR};
TypeDesc Int_Type    = {.kind = TYPE_BASIC, .basic = BASIC_INT};
TypeDesc Float_Type  = {.kind = TYPE_BASIC, .basic = BASIC_FLOAT};
TypeDesc Bool_Type   = {.kind = TYPE_BASIC, .basic = BASIC_BOOL};
TypeDesc String_Type = {.kind = TYPE_BASIC, .basic = BASIC_STRING};
TypeDesc Any_Type    = {.kind = TYPE_BASIC, .basic = BASIC_ANY};
TypeDesc Varg_Type   = {.kind = TYPE_BASIC, .basic = BASIC_VARG};

struct basic_type_s {
  int kind;
  char *str;
  TypeDesc *type;
} basic_types[] = {
  {BASIC_BYTE,   "byte",   &Byte_Type   },
  {BASIC_CHAR,   "char",   &Char_Type   },
  {BASIC_INT,    "int",    &Int_Type    },
  {BASIC_FLOAT,  "float",  &Float_Type  },
  {BASIC_BOOL,   "bool",   &Bool_Type   },
  {BASIC_STRING, "string", &String_Type },
  {BASIC_ANY,    "any",    &Any_Type    },
  {BASIC_VARG,   "...",    &Varg_Type   }
};

static struct basic_type_s *get_basic(int kind)
{
  struct basic_type_s *basic;
  for (int i = 0; i < nr_elts(basic_types); i++) {
    basic = basic_types + i;
    if (kind == basic->kind)
      return basic;
  }
  return NULL;
}

static TypeDesc *typedesc_new(TypeDescKind kind)
{
  TypeDesc *desc = mm_alloc(sizeof(TypeDesc));
  if (!desc)
    return NULL;

  desc->kind = kind;
  return desc;
}

static int basic_equal(TypeDesc *t1, TypeDesc *t2)
{
  return t1->basic == t2->basic;
}

static void basic_tostring(TypeDesc *t, char *buf)
{
  strcpy(buf, get_basic(t->basic)->str);
}

static TypeDesc *basic_dup(TypeDesc *desc)
{
  /* the same for basic types */
  return desc;
}

static void basic_free(TypeDesc *desc)
{
  /* no need free for basic types */
  UNUSED_PARAMETER(desc);
}

static int klass_equal(TypeDesc *t1, TypeDesc *t2)
{
  int eq = AtomString_Equal(t1->klass.type, t2->klass.type);
  if (!eq)
    return 0;
  return AtomString_Equal(t1->klass.path, t2->klass.path);
}

static void klass_tostring(TypeDesc *t, char *buf)
{
  if (t->klass.path.str)
    sprintf(buf, "%s.%s", t->klass.path.str, t->klass.type.str);
  else
    sprintf(buf, "%s", t->klass.type.str);
}

static TypeDesc *klass_dup(TypeDesc *desc)
{
  /* klass is duplicated path and type internally */
  return TypeDesc_New_Klass(desc->klass.path.str, desc->klass.type.str);
}

static void klass_free(TypeDesc *t)
{
  mm_free(t);
}

static int typelist_equal(Vector *v1, Vector *v2)
{
  if (v1 && v2) {
    if (Vector_Size(v1) != Vector_Size(v2)) return 0;
    TypeDesc *t1, *t2;
    Vector_ForEach(t1, v1) {
      t2 = Vector_Get(v2, i);
      if (!TypeDesc_Equal(t1, t2))
        return 0;
    }
    return 1;
  } else if (!v1 && !v2) {
    return 1;
  } else {
    return 0;
  }
}

static int proto_equal(TypeDesc *t1, TypeDesc *t2)
{
  int eq = typelist_equal(t1->proto.arg, t2->proto.arg);
  if (!eq)
    return 0;
  return typelist_equal(t1->proto.ret, t2->proto.ret);
}

static void proto_tostring(TypeDesc *t, char *buf)
{
  strcpy(buf, "func(");
  TypeDesc *item;
  Vector_ForEach(item, t->proto.arg) {
    if (i != 0)
      strcat(buf, ", ");
    TypeDesc_ToString(item, buf + strlen(buf));
  }
  strcat(buf, ") ");

  int nret = Vector_Size(t->proto.ret);
  if (nret > 1)
    strcat(buf, "(");
  Vector_ForEach(item, t->proto.ret) {
    if (i != 0)
      strcat(buf, ", ");
    TypeDesc_ToString(item, buf + strlen(buf));
  }
  if (nret > 1)
    strcat(buf, ")");
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
      Vector_Append(arg, TypeDesc_Dup(item));
  }

  if (desc->proto.ret) {
    ret = Vector_New();
    TypeDesc *item;
    Vector_ForEach(item, desc->proto.ret)
      Vector_Append(ret, TypeDesc_Dup(item));
  }

  return TypeDesc_New_Proto(arg, ret);
}

static void __item_free(void *item, void *arg)
{
  UNUSED_PARAMETER(arg);
  TypeDesc_Free(item);
}

static void proto_free(TypeDesc *t)
{
  if (t->proto.arg)
    Vector_Free(t->proto.arg, __item_free, NULL);
  if (t->proto.ret)
    Vector_Free(t->proto.ret, __item_free, NULL);
  mm_free(t);
}

static int array_equal(TypeDesc *t1, TypeDesc *t2)
{
  if (t1->array.dims != t2->array.dims)
    return 0;
  return TypeDesc_Equal(t1->array.base, t2->array.base);
}

static void array_tostring(TypeDesc *t, char *buf)
{
  strcpy(buf, "[");
  TypeDesc_ToString(t->array.base, buf + strlen(buf));
  strcat(buf, "]");
}

static TypeDesc *array_dup(TypeDesc *desc)
{
  /* need duplicate deeply */
  TypeDesc *base = TypeDesc_Dup(desc->array.base);
  return TypeDesc_New_Array(desc->array.dims, base);
}

static void array_free(TypeDesc *t)
{
  TypeDesc_Free(t->array.base);
  mm_free(t);
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
  TypeDesc *key = TypeDesc_Dup(desc->map.key);
  TypeDesc *val = TypeDesc_Dup(desc->map.val);
  return TypeDesc_New_Map(key, val);
}

static void map_free(TypeDesc *t)
{
  TypeDesc_Free(t->map.key);
  TypeDesc_Free(t->map.val);
  mm_free(t);
}

struct typedesc_ops_s {
  int (*__equal)(TypeDesc *t1, TypeDesc *t2);
  void (*__tostring)(TypeDesc *t, char *buf);
  TypeDesc *(*__dup)(TypeDesc *t);
  void (*__free)(TypeDesc *t);
} typedesc_ops[] = {
  /* 0 is not used */
  {NULL, NULL, NULL, NULL},
  /* basic type needn't free */
  {basic_equal, basic_tostring, basic_dup, basic_free},
  {klass_equal, klass_tostring, klass_dup, klass_free},
  {proto_equal, proto_tostring, proto_dup, proto_free},
  {array_equal, array_tostring, array_dup, array_free},
  {map_equal,   map_tostring,   map_dup,   map_free},
};

int TypeDesc_Equal(TypeDesc *t1, TypeDesc *t2)
{
  if (t1 == t2)
    return 1;
  if (t1->kind != t2->kind)
    return 0;
  int kind = t1->kind;
  assert(kind > 0 && kind < nr_elts(typedesc_ops));
  return typedesc_ops[kind].__equal(t1, t2);
}

void TypeDesc_ToString(TypeDesc *desc, char *buf)
{
  if (!desc)
    return;
  int kind = desc->kind;
  assert(kind > 0 && kind < nr_elts(typedesc_ops));
  typedesc_ops[kind].__tostring(desc, buf);
}

/* deep copy */
TypeDesc *TypeDesc_Dup(TypeDesc *desc)
{
  int kind = desc->kind;
  assert(kind > 0 && kind < nr_elts(typedesc_ops));
  return typedesc_ops[kind].__dup(desc);
}

void TypeDesc_Free(TypeDesc *desc)
{
  int kind = desc->kind;
  assert(kind > 0 && kind < nr_elts(typedesc_ops));
  typedesc_ops[kind].__free(desc);
}

TypeDesc *TypeDesc_New_Klass(char *path, char *type)
{
  TypeDesc *desc = typedesc_new(TYPE_KLASS);
  if (path)
    desc->klass.path = AtomString_New(path);
  desc->klass.type = AtomString_New(type);
  return desc;
}

TypeDesc *TypeDesc_New_Proto(Vector *arg, Vector *ret)
{
  TypeDesc *desc = typedesc_new(TYPE_PROTO);
  desc->proto.arg = arg;
  desc->proto.ret = ret;
  return desc;
}

TypeDesc *TypeDesc_New_Array(int dims, TypeDesc *base)
{
  if (dims <= 0 || !base) return NULL;

  TypeDesc *desc = typedesc_new(TYPE_ARRAY);
  desc->array.dims = dims;
  desc->array.base = base;
  return desc;
}

TypeDesc *TypeDesc_New_Map(TypeDesc *key, TypeDesc *val)
{
  TypeDesc *desc = typedesc_new(TYPE_MAP);
  desc->map.key = key;
  desc->map.val = val;
  return desc;
}

static TypeDesc *cnstring_to_klass(char *str, int len)
{
  TypeDesc *desc;
  char *dot = strchr(str, '.');
  if (dot) {
    char path[dot - str + 1];
    char type[len - (dot-str) - 1 + 1];
    strncpy(path, str, dot - str);
    path[dot - str] = 0;
    strncpy(type, dot + 1, len - (dot-str) - 1);
    type[len - (dot-str) - 1] = 0;
    desc = TypeDesc_New_Klass(path, type);
  } else {
    char type[len + 1];
    strncpy(type, str, len);
    type[len] = 0;
    desc = TypeDesc_New_Klass(NULL, type);
  }
  return desc;
}

/*
 * "i[sOkoala/lang.Tuple;" -->>> int, [string], koala/lang.Tuple
 */
Vector *String_To_TypeDescList(char *str)
{
  if (!str)
    return NULL;
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
      while ((ch = *str) != ';')
        str++;
      desc = cnstring_to_klass(start, str - start);
      if (dims > 0)
        desc = TypeDesc_New_Array(dims, desc);
      Vector_Append(v, desc);
      dims = 0;
      str++;
    } else if (ch == '[') {
      while ((ch = *str) == '[') {
        dims++;
        str++;
      }
    } else if ((desc = get_basic(ch)->type)) {
      if (dims > 0)
        desc = TypeDesc_New_Array(dims, desc);
      Vector_Append(v, desc);
      dims = 0;
      str++;
    } else {
      assert(0);
    }
  }

  return v;
}
