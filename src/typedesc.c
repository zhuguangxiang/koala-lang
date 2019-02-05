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
#include "stringbuf.h"
#include "mem.h"
#include "log.h"

static BasicDesc Byte_Type;
static BasicDesc Char_Type;
static BasicDesc Int_Type;
static BasicDesc Float_Type;
static BasicDesc Bool_Type;
static BasicDesc String_Type;
static BasicDesc Error_Type;
static BasicDesc Any_Type;

struct basic_type_s {
  int kind;
  char *str;
  BasicDesc *type;
} basic_types[] = {
  {BASIC_BYTE,   "byte",   &Byte_Type   },
  {BASIC_CHAR,   "char",   &Char_Type   },
  {BASIC_INT,    "int",    &Int_Type    },
  {BASIC_FLOAT,  "float",  &Float_Type  },
  {BASIC_BOOL,   "bool",   &Bool_Type   },
  {BASIC_STRING, "string", &String_Type },
  {BASIC_ERROR,  "error",  &Error_Type  },
  {BASIC_ANY,    "any",    &Any_Type    },
};

static struct basic_type_s *get_basic(int kind)
{
  struct basic_type_s *p;
  for (int i = 0; i < nr_elts(basic_types); i++) {
    p = basic_types + i;
    if (kind == p->kind)
      return p;
  }
  return NULL;
}

static HashTable descTable;

static uint32 desc_hash(void *k)
{
  TypeDesc *desc = k;
  return AtomString_Hash(desc->desc);
}

static int desc_equal(void *k1, void *k2)
{
  TypeDesc *desc1 = k1;
  TypeDesc *desc2 = k2;
  return AtomString_Equal(desc1->desc, desc2->desc);
}

static void init_basictypes(void)
{
  BasicDesc *basic;
  int result;
  struct basic_type_s *p;
  for (int i = 0; i < nr_elts(basic_types); i++) {
    p = basic_types + i;
    basic = p->type;
    basic->kind = TYPE_BASIC;
    basic->type = p->kind;
    Init_HashNode(&basic->hnode, basic);
    basic->desc = AtomString_New((char *)&p->kind);
    /* no need free basic descriptors */
    basic->refcnt = 2;
    result = HashTable_Insert(&descTable, &basic->hnode);
    assert(!result);
  }
}

void Init_TypeDesc(void)
{
  HashTable_Init(&descTable, desc_hash, desc_equal);
  init_basictypes();
}

static void desc_free_func(HashNode *hnode, void *arg)
{
  TypeDesc *desc = container_of(hnode, TypeDesc, hnode);
  TYPE_DECREF(desc);
}

static void check_basic_refcnt(void)
{
  Log_Puts("Basic Type Refcnt:");
  BasicDesc *basic;
  struct basic_type_s *p;
  for (int i = 0; i < nr_elts(basic_types); i++) {
    p = basic_types + i;
    basic = p->type;
    Log_Printf("  %s: %d\n", p->str, basic->refcnt);
    assert(basic->refcnt == 1);
  }
}

void Fini_TypeDesc(void)
{
  HashTable_Fini(&descTable, desc_free_func, NULL);
  check_basic_refcnt();
}

static void __basic_tostring(TypeDesc *desc, char *buf)
{
  BasicDesc *basic = (BasicDesc *)desc;
  strcpy(buf, get_basic(basic->type)->str);
}

static void __basic_fini(TypeDesc *desc)
{
  assert(desc->refcnt > 0);
}

static void __klass_tostring(TypeDesc *desc, char *buf)
{
  KlassDesc *klass = (KlassDesc *)desc;
  if (AtomString_Length(klass->path) > 0)
    sprintf(buf, "%s.%s", klass->path.str, klass->type.str);
  else
    sprintf(buf, "%s", klass->type.str);
}

static void __klass_fini(TypeDesc *desc)
{
  /* empty */
  UNUSED_PARAMETER(desc);
}

static void __proto_tostring(TypeDesc *desc, char *buf)
{
  ProtoDesc *proto = (ProtoDesc *)desc;
  strcpy(buf, "func(");
  TypeDesc *item;
  Vector_ForEach(item, proto->arg) {
    if (i != 0)
      strcat(buf, ", ");
    TypeDesc_ToString(item, buf + strlen(buf));
  }
  strcat(buf, ") ");

  int nret = Vector_Size(proto->ret);
  if (nret > 1)
    strcat(buf, "(");
  Vector_ForEach(item, proto->ret) {
    if (i != 0)
      strcat(buf, ", ");
    TypeDesc_ToString(item, buf + strlen(buf));
  }
  if (nret > 1)
    strcat(buf, ")");
}

static void __item_free(void *item, void *arg)
{
  UNUSED_PARAMETER(arg);
  TYPE_DECREF(item);
}

static inline void free_typelist(Vector *vec)
{
  Vector_Free(vec, __item_free, NULL);
}

static void __proto_fini(TypeDesc *desc)
{
  ProtoDesc *proto = (ProtoDesc *)desc;
  free_typelist(proto->arg);
  free_typelist(proto->ret);
}

static void __array_tostring(TypeDesc *desc, char *buf)
{
  ArrayDesc *array = (ArrayDesc *)desc;
  strcpy(buf, "[]");
  int i = 1;
  while (i++ < array->dims)
    strcat(buf, "[]");
  TypeDesc_ToString(array->base, buf + strlen(buf));
}

static void __array_fini(TypeDesc *desc)
{
  ArrayDesc *array = (ArrayDesc *)desc;
  TYPE_DECREF(array->base);
}

static void __map_tostring(TypeDesc *desc, char *buf)
{
  MapDesc *map = (MapDesc *)desc;
  strcpy(buf, "map[");
  TypeDesc_ToString(map->key, buf + strlen(buf));
  strcpy(buf + strlen(buf), "]");
  TypeDesc_ToString(map->val, buf + strlen(buf));
}

static void __map_fini(TypeDesc *desc)
{
  MapDesc *map = (MapDesc *)desc;
  TYPE_DECREF(map->key);
  TYPE_DECREF(map->val);
}

static void __set_tostring(TypeDesc *desc, char *buf)
{
  SetDesc *set = (SetDesc *)desc;
  strcpy(buf, "set[");
  TypeDesc_ToString(set->base, buf + strlen(buf));
  strcpy(buf + strlen(buf), "]");
}

static void __set_fini(TypeDesc *desc)
{
  SetDesc *set = (SetDesc *)desc;
  TYPE_DECREF(set->base);
}

static void __varg_tostring(TypeDesc *desc, char *buf)
{
  VargDesc *varg = (VargDesc *)desc;
  strcpy(buf, "...");
  TypeDesc_ToString(varg->base, buf + strlen(buf));
}

static void __varg_fini(TypeDesc *desc)
{
  VargDesc *varg = (VargDesc *)desc;
  TYPE_DECREF(varg->base);
}

struct typedesc_ops_s {
  void (*tostring)(TypeDesc *, char *);
  void (*fini)(TypeDesc *);
} typedesc_ops[] = {
  /* 0 is not used */
  {NULL, NULL},
  {__basic_tostring, __basic_fini},
  {__klass_tostring, __klass_fini},
  {__proto_tostring, __proto_fini},
  {__array_tostring, __array_fini},
  {__map_tostring,   __map_fini},
  {__set_tostring,   __set_fini},
  {__varg_tostring,  __varg_fini},
};

int TypeDesc_Equal(TypeDesc *desc1, TypeDesc *desc2)
{
  if (desc1 == NULL || desc2 == NULL)
    return 0;
  if (desc1 == desc2)
    return 1;
  if (desc1->kind != desc2->kind)
    return 0;
  return AtomString_Equal(desc1->desc, desc2->desc);
}

void TypeDesc_ToString(TypeDesc *desc, char *buf)
{
  if (desc == NULL)
    return;
  int kind = desc->kind;
  assert(kind > 0 && kind < nr_elts(typedesc_ops));
  typedesc_ops[kind].tostring(desc, buf);
}

void ProtoVec_ToString(Vector *vec, char *buf)
{
  /* clear old string */
  buf[0] = '\0';

  TypeDesc *item;
  Vector_ForEach(item, vec) {
    if (i != 0)
      strcat(buf, ", ");
    TypeDesc_ToString(item, buf + strlen(buf));
  }
}

void TypeDesc_Free(TypeDesc *desc)
{
  int kind = desc->kind;
  assert(kind > 0 && kind < nr_elts(typedesc_ops));
  assert(desc->kind != TYPE_BASIC);
  HashTable_Remove(&descTable, &desc->hnode);
  typedesc_ops[kind].fini(desc);
  mm_free(desc);
}

TypeDesc *TypeDesc_Get_Basic(int basic)
{
  return (TypeDesc *)get_basic(basic)->type;
}

static void *new_typedesc(DescKind kind, int size, char *descstr)
{
  TypeDesc *desc = mm_alloc(size);
  desc->kind = kind;
  Init_HashNode(&desc->hnode, desc);
  desc->refcnt = 1;
  desc->desc = AtomString_New(descstr);
  int result = HashTable_Insert(&descTable, &desc->hnode);
  assert(!result);
  return desc;
}

#define DeclareTypeDesc(name, kind, type, desc) \
  type *name = new_typedesc(kind, sizeof(type), desc)

static void *FindTypeDesc(char *desc)
{
  TypeDesc key = {.desc.str = desc};
  HashNode *hnode = HashTable_Find(&descTable, &key);
  if (hnode != NULL) {
    return container_of(hnode, TypeDesc, hnode);
  }
  return NULL;
}

TypeDesc *TypeDesc_Get_Klass(char *path, char *type)
{
  /* Lpath.type; */
  DeclareStringBuf(buf);
  StringBuf_Format_CStr(buf, "L#.#;", path, type);

  TypeDesc *exist = FindTypeDesc(buf.data);
  if (exist != NULL) {
    FiniStringBuf(buf);
    return exist;
  }

  DeclareTypeDesc(desc, TYPE_KLASS, KlassDesc, buf.data);
  if (path != NULL)
    desc->path = AtomString_New(path);
  desc->type = AtomString_New(type);
  FiniStringBuf(buf);
  return (TypeDesc *)desc;
}

TypeDesc *TypeDesc_Get_Proto(Vector *arg, Vector *ret)
{
  String sArg = TypeList_ToString(arg);
  String sRet = TypeList_ToString(ret);

  /* Pargs:rets;*/
  DeclareStringBuf(buf);
  StringBuf_Format(buf, "P#:#;", sArg, sRet);

  TypeDesc *exist = FindTypeDesc(buf.data);
  if (exist != NULL) {
    FiniStringBuf(buf);
    /* NOTES: not use parameters, free them here */
    free_typelist(arg);
    free_typelist(ret);
    return exist;
  }

  DeclareTypeDesc(desc, TYPE_PROTO, ProtoDesc, buf.data);
  desc->arg = arg;
  desc->ret = ret;
  FiniStringBuf(buf);
  return (TypeDesc *)desc;
}

TypeDesc *TypeDesc_Get_Array(int dims, TypeDesc *base)
{
  /* [type */
  DeclareStringBuf(buf);
  int i = 0;
  while (i++ < dims)
    StringBuf_Append_Char(buf, '[');
  StringBuf_Append(buf, base->desc);

  TypeDesc *exist = FindTypeDesc(buf.data);
  if (exist != NULL) {
    FiniStringBuf(buf);
    return exist;
  }

  DeclareTypeDesc(desc, TYPE_ARRAY, ArrayDesc, buf.data);
  desc->dims = dims;
  TYPE_INCREF(base);
  desc->base = base;
  FiniStringBuf(buf);
  return (TypeDesc *)desc;
}

TypeDesc *TypeDesc_Get_Map(TypeDesc *key, TypeDesc *val)
{
  /* Mtype:type */
  DeclareStringBuf(buf);
  StringBuf_Format(buf, "M#:#", key->desc, val->desc);

  TypeDesc *exist = FindTypeDesc(buf.data);
  if (exist != NULL) {
    FiniStringBuf(buf);
    return exist;
  }

  DeclareTypeDesc(desc, TYPE_MAP, MapDesc, buf.data);
  TYPE_INCREF(key);
  desc->key = key;
  TYPE_INCREF(val);
  desc->val = val;
  FiniStringBuf(buf);
  return (TypeDesc *)desc;
}

TypeDesc *TypeDesc_Get_Set(TypeDesc *base)
{
  /* Stype*/
  DeclareStringBuf(buf);
  StringBuf_Format(buf, "S#", base->desc);

  TypeDesc *exist = FindTypeDesc(buf.data);
  if (exist != NULL) {
    FiniStringBuf(buf);
    return exist;
  }

  DeclareTypeDesc(desc, TYPE_SET, SetDesc, buf.data);
  TYPE_INCREF(base);
  desc->base = base;
  FiniStringBuf(buf);
  return (TypeDesc *)desc;
}

TypeDesc *TypeDesc_Get_Varg(TypeDesc *base)
{
  if (base == NULL)
    base = TypeDesc_Get_Basic(BASIC_ANY);

  /* ...type*/
  DeclareStringBuf(buf);
  StringBuf_Format(buf, "...#", base->desc);

  TypeDesc *exist = FindTypeDesc(buf.data);
  if (exist != NULL) {
    FiniStringBuf(buf);
    return exist;
  }

  DeclareTypeDesc(desc, TYPE_VARG, VargDesc, buf.data);
  if (base != NULL) {
    TYPE_INCREF(base);
    desc->base = base;
  }
  FiniStringBuf(buf);
  return (TypeDesc *)desc;
}

static TypeDesc *string_to_klass(char *s, int len)
{
  TypeDesc *desc;
  char *dot = strchr(s, '.');
  String path;
  String type;
  if (dot) {
    path = AtomString_New_NStr(s, dot - s);
    type = AtomString_New_NStr(dot + 1, len - (dot - s) - 1);
  } else {
    path.str = NULL;
    type = AtomString_New_NStr(s, dot - s);
  }
  return TypeDesc_Get_Klass(path.str, type.str);
}

TypeDesc *String_To_TypeDesc(char **string, int _dims, int _varg)
{
  char *s = *string;
  char ch = *s;
  int dims = 0;
  TypeDesc *desc;
  TypeDesc *base;

  switch (ch) {
    case 'L': {
      s += 1;
      char *k = s;
      while (*s != ';')
        s += 1;
      desc = string_to_klass(k, s - k);
      s += 1;
      break;
    }
    case '[': {
      assert(_dims == 0 && _varg == 0);
      while (*s == '[') {
        dims += 1;
        s += 1;
      }
      base = String_To_TypeDesc(&s, dims, 0);
      desc = TypeDesc_Get_Array(dims, base);
      break;
    }
    case 'M': {
      s += 1;
      TypeDesc *key = String_To_TypeDesc(&s, 0, 0);
      TypeDesc *val = String_To_TypeDesc(&s, 0, 0);
      desc = TypeDesc_Get_Map(key, val);
      break;
    }
    case 'S': {
      s += 1;
      base = String_To_TypeDesc(&s, 0, 0);
      desc = TypeDesc_Get_Set(base);
      break;
    }
    case 'P': {
      Vector *args = NULL;
      Vector *rets = NULL;
      s += 1;
      while ((ch = *s) != ':') {
        if (args == NULL)
          args = Vector_New();
        base = String_To_TypeDesc(&s, 0, 0);
        TYPE_INCREF(base);
        Vector_Append(args, base);
      }
      s += 1;
      while ((ch = *s) != ';') {
        if (rets == NULL)
          rets = Vector_New();
        base = String_To_TypeDesc(&s, 0, 0);
        TYPE_INCREF(base);
        Vector_Append(rets, base);
      }
      s += 1;
      desc = TypeDesc_Get_Proto(args, rets);
      break;
    }
    case '.': {
      assert(_dims == 0 && _varg == 0);
      assert(s[1] == '.' && s[2] == '.');
      s += 3;
      if (s[0] == '\0') {
        base = (TypeDesc *)&Any_Type;
      } else {
        base = String_To_TypeDesc(&s, 0, 1);
      }
      desc = TypeDesc_Get_Varg(base);
      break;
    }
    default: {
      struct basic_type_s *p = get_basic(ch);
      assert(p != NULL);
      assert(!(_dims > 0 && _varg > 0));
      desc = (TypeDesc *)p->type;
      s += 1;
    }
  }

  *string = s;
  return desc;
}

/*
 * "i[sLkoala/lang.Tuple;" -->>> int, [string], koala/lang.Tuple
 */
Vector *String_ToTypeList(char *s)
{
  if (s == NULL)
    return NULL;

  Vector *vec = Vector_New();
  TypeDesc *desc;

  while (*s != '\0') {
    desc = String_To_TypeDesc(&s, 0, 0);
    assert(desc != NULL);
    TYPE_INCREF(desc);
    Vector_Append(vec, desc);
  }

  return vec;
}

String TypeList_ToString(Vector *vec)
{
  String s;

  if (Vector_Size(vec) <= 0) {
    s.str = NULL;
    return s;
  }

  DeclareStringBuf(buf);

  TypeDesc *desc;
  Vector_ForEach(desc, vec) {
    StringBuf_Append(buf, desc->desc);
  }
  s = AtomString_New(buf.data);
  FiniStringBuf(buf);

  return s;
}
