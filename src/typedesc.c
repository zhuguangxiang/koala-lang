/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#include "typedesc.h"
#include "atom.h"
#include "memory.h"
#include "log.h"

void literal_show(Literal *val, StrBuf *sbuf)
{
  switch (val->kind) {
  case 0:
    strbuf_append(sbuf, "(nil)");
    break;
  case BASE_INT:
    strbuf_append_int(sbuf, val->ival);
    break;
  case BASE_FLOAT:
    strbuf_append_float(sbuf, val->fval);
    break;
  case BASE_BOOL:
    strbuf_append(sbuf, val->bval ? "true" : "false");
    break;
  case BASE_STR:
    strbuf_append_char(sbuf, '\'');
    strbuf_append(sbuf, val->str);
    strbuf_append_char(sbuf, '\'');
    break;
  case BASE_CHAR:
    strbuf_append_int(sbuf, val->cval.val);
    break;
  default:
    panic("invalid literal %d", val->kind);
    break;
  }
}

#define DECLARE_BASE(name, kind) \
  TypeDesc type_base_##name = {TYPE_BASE, 1, .base = kind}

DECLARE_BASE(int,   BASE_INT);
DECLARE_BASE(str,   BASE_STR);
DECLARE_BASE(any,   BASE_ANY);
DECLARE_BASE(bool,  BASE_BOOL);
DECLARE_BASE(byte,  BASE_BYTE);
DECLARE_BASE(char,  BASE_CHAR);
DECLARE_BASE(float, BASE_FLOAT);
DECLARE_BASE(desc,  -1);

static struct baseinfo {
  TypeDesc *desc;
  char *str;
} bases[] = {
  {&type_base_int,   "int"   },
  {&type_base_str,   "string"},
  {&type_base_any,   "any"   },
  {&type_base_bool,  "bool"  },
  {&type_base_byte,  "byte"  },
  {&type_base_char,  "char"  },
  {&type_base_float, "float" },
};

char *base_str(int kind)
{
  struct baseinfo *base;
  for (int i = 0; i < COUNT_OF(bases); ++i) {
    base = bases + i;
    if (base->desc->base == kind)
      return base->str;
  }
  return NULL;
}

TypeDesc *desc_from_base(int kind)
{
  struct baseinfo *base;
  for (int i = 0; i < COUNT_OF(bases); ++i) {
    base = bases + i;
    if (base->desc->base == kind)
      return TYPE_INCREF(base->desc);
  }
  return NULL;
}

static inline void check_base_refcnt(void)
{
  struct baseinfo *base;
  int refcnt;
  for (int i = 0; i < COUNT_OF(bases); ++i) {
    base = bases + i;
    refcnt = base->desc->refcnt;
    bug(refcnt != 1, "type of '%s' refcnt is %d.", base->str, refcnt);
  }
  refcnt = type_base_desc.refcnt;
  bug(refcnt != 1, "type of 'desc' refcnt is %d.", refcnt);
}

void init_typedesc(void)
{

}

void fini_typedesc(void)
{
  check_base_refcnt();
}

TypeDesc *desc_from_klass(char *path, char *type)
{
  TypeDesc *desc = kmalloc(sizeof(TypeDesc));
  desc->kind = TYPE_KLASS;
  desc->refcnt = 1;
  desc->klass.path = path;
  desc->klass.type = type;
  return desc;
}

TypeDesc *desc_from_proto(Vector *args, TypeDesc *ret)
{
  TypeDesc *desc = kmalloc(sizeof(TypeDesc));
  desc->kind = TYPE_PROTO;
  desc->refcnt = 1;
  desc->proto.args = args;
  desc->proto.ret = TYPE_INCREF(ret);
  return desc;
}

TypeDesc *desc_from_varg(TypeDesc *base)
{
  TypeDesc *desc = kmalloc(sizeof(TypeDesc));
  desc->kind = TYPE_VARG;
  desc->refcnt = 1;
  desc->varg = TYPE_INCREF(base);
  return desc;
}

TypeDesc *desc_from_array(TypeDesc *base)
{
  TypeDesc *desc = kmalloc(sizeof(TypeDesc));
  desc->kind = TYPE_ARRAY;
  desc->refcnt = 1;
  if (base != NULL) {
    Vector *types = vector_new();
    vector_push_back(types, TYPE_INCREF(base));
    desc->types = types;
  }
  return desc;
}

TypeDesc *desc_from_map(TypeDesc *key, TypeDesc *val)
{
  TypeDesc *desc = kmalloc(sizeof(TypeDesc));
  desc->kind = TYPE_MAP;
  desc->refcnt = 1;
  if (key != NULL && val != NULL) {
    Vector *types = vector_new();
    vector_push_back(types, TYPE_INCREF(key));
    vector_push_back(types, TYPE_INCREF(val));
    desc->types = types;
  }
  return desc;
}

TypeDesc *desc_from_tuple(Vector *types)
{
  TypeDesc *desc = kmalloc(sizeof(TypeDesc));
  desc->kind = TYPE_TUPLE;
  desc->refcnt = 1;
  desc->types = types;
  return desc;
}

static TypeDesc *new_pararef(char *name)
{
  TypeDesc *desc = kmalloc(sizeof(TypeDesc));
  desc->kind = TYPE_PARAREF;
  desc->refcnt = 1;
  desc->pararef.name = name;
  desc->pararef.index = -1;
  return desc;
}

static TypeDesc *new_paradef(char *name, Vector *types)
{
  TypeDesc *desc = kmalloc(sizeof(TypeDesc));
  desc->kind = TYPE_PARADEF;
  desc->refcnt = 1;
  desc->paradef.name = name;
  desc->paradef.types = types;
  return desc;
}

void desc_add_paradef(TypeDesc *desc, char *name, Vector *types)
{
  Vector *vec = desc->paras;
  if (vec == NULL) {
    vec = vector_new();
    desc->paras = vec;
  }
  vector_push_back(vec, new_paradef(name, types));
}

void free_descs(Vector *vec)
{
  if (vec == NULL)
    return;
  TypeDesc *type;
  vector_for_each(type, vec) {
    TYPE_DECREF(type);
  }
  vector_free(vec);
}

void desc_free(TypeDesc *desc)
{
  if (desc == NULL)
    return;

  free_descs(desc->paras);
  free_descs(desc->types);

  int kind = desc->kind;
  switch (kind) {
  case TYPE_KLASS: {
    kfree(desc);
    break;
  }
  case TYPE_PROTO: {
    free_descs(desc->proto.args);
    TYPE_DECREF(desc->proto.ret);
    kfree(desc);
    break;
  }
  case TYPE_VARG: {
    TYPE_DECREF(desc->varg);
    kfree(desc);
    break;
  }
  case TYPE_PARAREF: {
    kfree(desc);
    break;
  }
  case TYPE_PARADEF: {
    free_descs(desc->paradef.types);
    kfree(desc);
    break;
  }
  case TYPE_ARRAY: {
    kfree(desc);
    break;
  }
  case TYPE_MAP: {
    kfree(desc);
    break;
  }
  case TYPE_TUPLE: {
    kfree(desc);
    break;
  }
  default:
    panic("invalid typedesc %d", kind);
    break;
  }
}

void desc_tostr(TypeDesc *desc, StrBuf *buf)
{

}

void desc_show(TypeDesc *desc)
{
  if (desc != NULL) {
    STRBUF(sbuf);
    desc_tostr(desc, &sbuf);
    puts(strbuf_tostr(&sbuf)); /* with newline */
    strbuf_fini(&sbuf);
  } else {
    puts("<undefined-type>");  /* with newline */
  }
}

/* desc1 <- desc2 */
int desc_check(TypeDesc *desc1, TypeDesc *desc2)
{
  bug(desc1->paras != NULL, "typeparas is not null");
  bug(desc1->kind == TYPE_PARAREF, "TYPE_PARAREF here?");
  bug(desc1->kind == TYPE_PARADEF, "TYPE_PARADEF here?");
  bug(desc2->paras != NULL, "typeparas is not null");
  bug(desc2->kind == TYPE_PARAREF, "TYPE_PARAREF here?");
  bug(desc2->kind == TYPE_PARADEF, "TYPE_PARADEF here?");

  if (desc1 == desc2)
    return 1;

  if (desc_isany(desc1))
    return 1;

  if (desc1->kind != desc2->kind)
    return 0;

  switch (desc1->kind) {
  case TYPE_BASE:
    if (desc1->base != desc2->base)
      return 0;
    else
      return 1;
  case TYPE_KLASS: {
    char *path1 = desc1->klass.path;
    char *path2 = desc2->klass.path;
    if (strcmp(path1, path2))
      return 0;

    char *name1 = desc1->klass.type;
    char *name2 = desc2->klass.type;
    if (strcmp(name1, name2))
      return 0;

    Vector *vec1 = desc1->types;
    Vector *vec2 = desc2->types;
    int size1 = vector_size(vec1);
    int size2 = vector_size(vec2);
    if (size1 != size2)
      return 0;

    TypeDesc *type1;
    TypeDesc *type2;
    for (int i = 0; i < size1; ++i) {
      type1 = vector_get(vec1, i);
      type2 = vector_get(vec2, i);
      if (!desc_check(type1, type2))
        return 0;
    }
    return 1;
  }
  default:
    panic("who goes here? descriptor %d", desc1->kind);
  }
  return 0;
}

static TypeDesc *__to_klass(char *s, int len)
{
  char *dot = strrchr(s, '.');
  bug(dot == NULL, "null pointer");
  char *path = atom_nstring(s, dot - s);
  char *type = atom_nstring(dot + 1, len - (dot - s) - 1);
  return desc_from_klass(path, type);
}

static TypeDesc *__to_desc(char **str)
{
  char *s = *str;
  char ch = *s;
  char *k;
  TypeDesc *desc;
  TypeDesc *base;

  switch (ch) {
  case 'L': {
    s++;
    k = s;
    while (*s != ';')
      s++;
    desc = __to_klass(k, s - k);
    s++;
    break;
  }
  case '<': {
    s++;
    k = s;
    while (*s != '>')
      s++;
    k = atom_nstring(k, s - k);
    desc = new_pararef(k);
    s++;
    break;
  }
  case '.': {
    if (s[1] != '.' && s[2] != '.')
      panic("invalid variable argument");
    s += 3;
    if (s[0] == 'A') {
      base = desc_from_any;
      s++;
    } else if (*s) {
      base = __to_desc(&s);
    } else {
      base = desc_from_any;
    }
    desc = desc_from_varg(base);
    TYPE_DECREF(base);
    break;
  }
  case '[': {
    base = __to_desc(&s);
    desc = desc_from_array(base);
    TYPE_DECREF(base);
    break;
  }
  case 'M': {
    s += 1;
    TypeDesc *key = __to_desc(&s);
    TypeDesc *val = __to_desc(&s);
    desc = desc_from_map(key, val);
    TYPE_DECREF(key);
    TYPE_DECREF(val);
    break;
  }
  case 'P': {
    s++;
    Vector *args = NULL;
    if (*s != ':') {
      args = vector_new();
      TypeDesc *arg;
      while (*s != ':') {
        arg = __to_desc(&s);
        vector_push_back(args, arg);
      }
    }
    s++;
    base = __to_desc(&s);
    desc = desc_from_proto(args, base);
    TYPE_DECREF(base);
    break;
  }
  default: {
    desc = desc_from_base(ch);
    bug(desc == NULL, "invalid typestr:%c", ch);
    s++;
    break;
  }
  }
  *str = s;
  return desc;
}

TypeDesc *str_to_desc(char *s)
{
  if (s == NULL)
    return NULL;
  return __to_desc(&s);
}

TypeDesc *str_to_proto(char *ptype, char *rtype)
{
  Vector *args = NULL;
  if (ptype != NULL) {
    args = vector_new();
    char *s = ptype;
    TypeDesc *desc;
    while (*s) {
      desc = __to_desc(&s);
      vector_push_back(args, desc);
    }
  }

  TypeDesc *ret = NULL;
  if (rtype != NULL) {
    char *s = rtype;
    ret = __to_desc(&s);
  }

  TypeDesc *desc;
  desc = desc_from_proto(args, ret);
  TYPE_DECREF(ret);
  return desc;
}
