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

LOGGER(0)

void Const_Show(ConstValue *val, char *buf)
{
  switch (val->kind) {
  case 0:
    sprintf(buf, "(nil)");
    break;
  case BASE_INT:
    sprintf(buf, "%lld", val->ival);
    break;
  case BASE_FLOAT:
    sprintf(buf, "%.16lf", val->fval);
    break;
  case BASE_BOOL:
    sprintf(buf, "%s", val->bval ? "true" : "false");
    break;
  case BASE_STRING:
    sprintf(buf, "%s", val->str);
    break;
  case BASE_CHAR:
    sprintf(buf, "%s", val->ch.data);
    break;
  default:
    assert(0);
    break;
  }
}

static BaseDesc Byte_Type;
static BaseDesc Char_Type;
static BaseDesc Int_Type;
static BaseDesc Float_Type;
static BaseDesc Bool_Type;
static BaseDesc String_Type;
static BaseDesc Any_Type;

struct base_type_s {
  int kind;
  char *str;
  BaseDesc *type;
  char *pkgpath;
  char *typestr;
} base_types[] = {
  {BASE_BYTE,   "byte",   &Byte_Type,   "lang", "Byte"   },
  {BASE_CHAR,   "char",   &Char_Type,   "lang", "Char"   },
  {BASE_INT,    "int",    &Int_Type,    "lang", "Integer"},
  {BASE_FLOAT,  "float",  &Float_Type,  "lang", "Float"  },
  {BASE_BOOL,   "bool",   &Bool_Type,   "lang", "Bool"   },
  {BASE_STRING, "string", &String_Type, "lang", "String" },
  {BASE_ANY,    "any",    &Any_Type,    "lang", "Any"    },
};

static struct base_type_s *get_base(int kind)
{
  struct base_type_s *p;
  for (int i = 0; i < nr_elts(base_types); i++) {
    p = base_types + i;
    if (kind == p->kind)
      return p;
  }
  return NULL;
}

static void init_basetypes(void)
{
  BaseDesc *base;
  int result;
  struct base_type_s *p;
  for (int i = 0; i < nr_elts(base_types); i++) {
    p = base_types + i;
    base = p->type;
    base->kind = TYPE_BASE;
    base->type = p->kind;
    base->pkgpath = p->pkgpath;
    base->typestr = p->typestr;
    base->refcnt = 1;
  }
}

void Init_TypeDesc(void)
{
  init_basetypes();
}

static void check_base_refcnt(void)
{
  Log_Puts("Base Type Refcnt:");
  BaseDesc *base;
  struct base_type_s *p;
  for (int i = 0; i < nr_elts(base_types); i++) {
    p = base_types + i;
    base = p->type;
    Log_Printf("  %s: %d\n", p->str, base->refcnt);
    assert(base->refcnt == 1);
  }
}

void Fini_TypeDesc(void)
{
  check_base_refcnt();
}

static void __base_tostring(TypeDesc *desc, StringBuf *buf)
{
  BaseDesc *base = (BaseDesc *)desc;
  __StringBuf_Append_CStr(buf, get_base(base->type)->str);
}

static void __base_fini(TypeDesc *desc)
{
  assert(desc->refcnt > 0);
}

static int __base_equal(TypeDesc *desc1, TypeDesc *desc2)
{
  if (desc1 == desc2)
    return 1;
  else
    return 0;
}

static void __klass_tostring(TypeDesc *desc, StringBuf *buf)
{
  KlassDesc *klass = (KlassDesc *)desc;
  if (AtomString_Length(klass->path) > 0)
    __StringBuf_Format(buf, 1, "#.#", klass->path.str, klass->type.str);
  else
    __StringBuf_Format(buf, 1, "#", klass->type.str);
}

TypeParaDef *TypeParaDef_New(char *name, Vector *bases)
{
  TypeParaDef *para = Malloc(sizeof(TypeParaDef));
  para->name = AtomString_New(name);
  para->bases = bases;
  return para;
}

void TypeParaDef_Free(TypeParaDef *paradef)
{
  Mfree(paradef);
}

int TypePara_Compatible(TypeParaDef *def, TypeDesc *desc)
{
  TypeDesc *type;
  Vector_ForEach(type, def->bases) {
    if (!TypeDesc_Equal(desc, type))
      return 0;
  }
  return 1;
}

TypeParaDef *Get_TypeParaDef(ParaRefDesc *ref, TypeDesc *desc)
{
  assert(desc->kind == TYPE_KLASS);
  KlassDesc *klass = (KlassDesc *)desc;
  TypeParaDef *def;
  Vector_ForEach(def, klass->paras) {
    if (!strcmp(def->name.str, ref->name.str))
      return def;
  }
  return NULL;
}

static void __klass_fini(TypeDesc *desc)
{
  KlassDesc *klass = (KlassDesc *)desc;
  if (klass->typepara == TYPEPARA_DEF) {
    TypeParaDef *paradef;
    Vector_ForEach(paradef, klass->paras) {
      TypeParaDef_Free(paradef);
    }
    Vector_Free_Self(klass->paras);
  } else if (klass->typepara == TYPEPARA_INST) {
    TypeDesc *type;
    Vector_ForEach(type, klass->paras) {
      TYPE_DECREF(type);
    }
    Vector_Free_Self(klass->paras);
  } else {
    assert(!klass->typepara);
  }
}

/* desc1 is subclass of desc2 */
static int __klass_equal(TypeDesc *desc1, TypeDesc *desc2)
{
  KlassDesc *k1 = (KlassDesc *)desc1;
  KlassDesc *k2 = (KlassDesc *)desc2;

  if (k1->path.str != NULL && k2->path.str != NULL) {
    if (strcmp(k1->path.str, k2->path.str))
      return 0;
  } else if (k1->path.str != NULL & k2->path.str == NULL) {
    return 0;
  } else if (k1->path.str == NULL & k2->path.str != NULL) {
    return 0;
  } else {
    //fallthrough
  }

  assert(k1->type.str != NULL && k2->type.str != NULL);
  if (strcmp(k1->type.str, k2->type.str))
    return 0;

  if (k1->typepara == TYPEPARA_INST && k2->typepara == TYPEPARA_INST) {
    if (k1->paras == NULL || k2->paras == NULL)
      return 1;
    int sz1 = Vector_Size(k1->paras);
    int sz2 = Vector_Size(k2->paras);
    if (sz1 != sz2)
      return 0;
    TypeDesc *type;
    TypeDesc *type2;
    Vector_ForEach(type, k1->paras) {
      type2 = Vector_Get(k2->paras, i);
      if (!TypeDesc_Equal(type, type2))
        return 0;
    }
  } else {
    assert(k1->typepara == 0);
    assert(k2->typepara == 0);
  }
  return 1;
}

void Proto_Print(ProtoDesc *proto, StringBuf *buf)
{
  __StringBuf_Append_CStr(buf, "(");
  TypeDesc *item;
  Vector_ForEach(item, proto->arg) {
    if (i != 0)
      __StringBuf_Append_CStr(buf, ", ");
    __StringBuf_Append(buf, TypeDesc_ToString(item));
  }
  char *s = (proto->ret != NULL) ? ") " : ")";
  __StringBuf_Append_CStr(buf, s);

  __StringBuf_Append(buf, TypeDesc_ToString(proto->ret));
}

static void __proto_tostring(TypeDesc *desc, StringBuf *buf)
{
  ProtoDesc *proto = (ProtoDesc *)desc;
  __StringBuf_Append_CStr(buf, "func");
  Proto_Print(proto, buf);
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
  TYPE_DECREF(proto->ret);
}

static void __array_tostring(TypeDesc *desc, StringBuf *buf)
{
  ArrayDesc *array = (ArrayDesc *)desc;
  __StringBuf_Append_CStr(buf, "[]");
  int i = 1;
  while (i++ < array->dims)
    __StringBuf_Append_CStr(buf, "[]");
  __StringBuf_Append(buf, TypeDesc_ToString(array->base));
}

static void __array_fini(TypeDesc *desc)
{
  ArrayDesc *array = (ArrayDesc *)desc;
  TYPE_DECREF(array->base);
}

static void __map_tostring(TypeDesc *desc, StringBuf *buf)
{
  MapDesc *map = (MapDesc *)desc;
  __StringBuf_Append_CStr(buf, "map[");
  __StringBuf_Append(buf, TypeDesc_ToString(map->key));
  __StringBuf_Append_CStr(buf, "]");
  __StringBuf_Append(buf, TypeDesc_ToString(map->val));
}

static void __map_fini(TypeDesc *desc)
{
  MapDesc *map = (MapDesc *)desc;
  TYPE_DECREF(map->key);
  TYPE_DECREF(map->val);
}

static void __varg_tostring(TypeDesc *desc, StringBuf *buf)
{
  VargDesc *varg = (VargDesc *)desc;
  __StringBuf_Append_CStr(buf, "...");
  __StringBuf_Append(buf, TypeDesc_ToString(varg->base));
}

static void __varg_fini(TypeDesc *desc)
{
  VargDesc *varg = (VargDesc *)desc;
  TYPE_DECREF(varg->base);
}

static void __tuple_tostring(TypeDesc *desc, StringBuf *buf)
{
  TupleDesc *tuple = (TupleDesc *)desc;
}

static void __tuple_fini(TypeDesc *desc)
{
  TupleDesc *tuple = (TupleDesc *)desc;
  free_typelist(tuple->bases);
}

static void __pararef_tostring(TypeDesc *desc, StringBuf *buf)
{
  ParaRefDesc *para = (ParaRefDesc *)desc;
  __StringBuf_Append_CStr(buf, para->name.str);
}

static void __pararef_fini(TypeDesc *desc)
{
  ParaRefDesc *para = (ParaRefDesc *)desc;
}

struct typedesc_ops_s {
  void (*tostring)(TypeDesc *, StringBuf *);
  void (*fini)(TypeDesc *);
  int (*equal)(TypeDesc *, TypeDesc *);
} typedesc_ops[] = {
  /* 0 is not used */
  {NULL, NULL},
  {__base_tostring,     __base_fini,    __base_equal},
  {__klass_tostring,    __klass_fini,   __klass_equal},
  {__proto_tostring,    __proto_fini,   NULL},
  {__array_tostring,    __array_fini,   NULL},
  {__map_tostring,      __map_fini,     NULL},
  {__varg_tostring,     __varg_fini,    NULL},
  {__tuple_tostring,    __tuple_fini,   NULL},
  {__pararef_tostring,  __pararef_fini, NULL},
};

int TypeDesc_Equal(TypeDesc *desc1, TypeDesc *desc2)
{
  if (desc1 == NULL || desc2 == NULL)
    return 0;
  if (desc1 == desc2)
    return 1;
  if (desc1->kind != desc2->kind)
    return 0;
  TypeDesc *any = (TypeDesc *)&Any_Type;
  if (desc1 == any || desc2 == any)
    return 1;
  int kind = desc1->kind;
  assert(kind > 0 && kind < nr_elts(typedesc_ops));
  return typedesc_ops[kind].equal(desc1, desc2);
}

String TypeDesc_ToString(TypeDesc *desc)
{
  String s = {NULL};
  if (desc == NULL)
    return s;
  int kind = desc->kind;
  assert(kind > 0 && kind < nr_elts(typedesc_ops));
  DeclareStringBuf(buf);
  typedesc_ops[kind].tostring(desc, &buf);
  s = AtomString_New(buf.data);
  FiniStringBuf(buf);
  return s;
}

void TypeDesc_Free(TypeDesc *desc)
{
  int kind = desc->kind;
  assert(kind > 0 && kind < nr_elts(typedesc_ops));
  assert(desc->kind != TYPE_BASE);
  typedesc_ops[kind].fini(desc);
  Mfree(desc);
}

TypeDesc *TypeDesc_Get_Base(int base)
{
  return (TypeDesc *)get_base(base)->type;
}

static void *new_typedesc(DescKind kind, int size)
{
  TypeDesc *desc = Malloc(size);
  desc->kind = kind;
  desc->refcnt = 1;
  return desc;
}

#define DeclareTypeDesc(name, kind, type) \
  type *name = new_typedesc(kind, sizeof(type))

TypeDesc *TypeDesc_New_Klass(char *path, char *type)
{
  /* Lpath.type; */
  DeclareTypeDesc(desc, TYPE_KLASS, KlassDesc);
  if (path != NULL)
    desc->path = AtomString_New(path);
  desc->type = AtomString_New(type);
  return (TypeDesc *)desc;
}

TypeDesc *TypeDesc_New_Klass_Def(char *path, char *type, Vector *paras)
{
  TypeDesc *desc = TypeDesc_New_Klass(path, type);
  KlassDesc *klazz = (KlassDesc *)desc;
  klazz->typepara = TYPEPARA_DEF;
  klazz->paras = paras;
  return desc;
}

TypeDesc *TypeDesc_New_Klass_Inst(char *path, char *type, Vector *paras)
{
  TypeDesc *desc = TypeDesc_New_Klass(path, type);
  KlassDesc *klazz = (KlassDesc *)desc;
  klazz->typepara = TYPEPARA_INST;
  klazz->paras = paras;
  return desc;
}

TypeDesc *TypeDesc_New_Proto(Vector *arg, TypeDesc *ret)
{
  DeclareTypeDesc(desc, TYPE_PROTO, ProtoDesc);
  desc->arg = arg;
  TYPE_INCREF(ret);
  desc->ret = ret;
  return (TypeDesc *)desc;
}

TypeDesc *TypeDesc_New_Array(TypeDesc *base)
{
  /* [type */
  int dims = 1;
  if (base->kind == TYPE_ARRAY) {
    ArrayDesc *arrayDesc = (ArrayDesc *)base;
    dims = arrayDesc->dims + 1;
  }
  DeclareTypeDesc(desc, TYPE_ARRAY, ArrayDesc);
  desc->dims = dims;
  TYPE_INCREF(base);
  desc->base = base;
  return (TypeDesc *)desc;
}

TypeDesc *TypeDesc_New_Map(TypeDesc *key, TypeDesc *val)
{
  /* Mtype:type */
  DeclareTypeDesc(desc, TYPE_MAP, MapDesc);
  TYPE_INCREF(key);
  desc->key = key;
  TYPE_INCREF(val);
  desc->val = val;
  return (TypeDesc *)desc;
}

TypeDesc *TypeDesc_New_Varg(TypeDesc *base)
{
  if (base == NULL)
    base = TypeDesc_Get_Base(BASE_ANY);

  /* ...type*/
  DeclareTypeDesc(desc, TYPE_VARG, VargDesc);
  TYPE_INCREF(base);
  desc->base = base;
  return (TypeDesc *)desc;
}

TypeDesc *TypeDesc_New_Tuple(Vector *bases)
{
  /* (type,type,type,) */
  DeclareTypeDesc(desc, TYPE_TUPLE, TupleDesc);
  desc->bases = bases;
  return (TypeDesc *)desc;
}

TypeDesc *TypeDesc_New_ParaRef(char *name)
{
  DeclareTypeDesc(desc, TYPE_REF, ParaRefDesc);
  desc->name = AtomString_New(name);
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
  return TypeDesc_New_Klass(path.str, type.str);
}

TypeDesc *__String_To_TypeDesc(char **string, int _dims, int _varg)
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
    base = __String_To_TypeDesc(&s, dims, 0);
    //FIXME
    desc = TypeDesc_New_Array(base);
    TYPE_DECREF(base);
    break;
  }
  case 'M': {
    s += 1;
    TypeDesc *key = __String_To_TypeDesc(&s, 0, 0);
    TypeDesc *val = __String_To_TypeDesc(&s, 0, 0);
    desc = TypeDesc_New_Map(key, val);
    TYPE_DECREF(key);
    TYPE_DECREF(val);
    break;
  }
  case 'P': {
    Vector *args = NULL;
    s += 1;
    while ((ch = *s) != ':') {
      if (args == NULL)
        args = Vector_New();
      base = __String_To_TypeDesc(&s, 0, 0);
      Vector_Append(args, base);
    }
    s += 1;
    base = __String_To_TypeDesc(&s, 0, 0);
    assert(*s == ';');
    s += 1;
    desc = TypeDesc_New_Proto(args, base);
    TYPE_DECREF(base);
    break;
  }
  case '.': {
    assert(_dims == 0 && _varg == 0);
    assert(s[1] == '.' && s[2] == '.');
    s += 3;
    if (s[0] == 'A') {
      base = TypeDesc_Get_Base('A');
      TYPE_INCREF(desc);
      s += 1;
    } else {
      base = __String_To_TypeDesc(&s, 0, 1);
    }
    desc = TypeDesc_New_Varg(base);
    TYPE_DECREF(base);
    break;
  }
  default: {
    assert(!(_dims > 0 && _varg > 0));
    desc = TypeDesc_Get_Base(ch);
    TYPE_INCREF(desc);
    s += 1;
    break;
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
    desc = __String_To_TypeDesc(&s, 0, 0);
    assert(desc != NULL);
    Vector_Append(vec, desc);
  }

  return vec;
}
