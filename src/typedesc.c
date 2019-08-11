/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "typedesc.h"
#include "atom.h"
#include "memory.h"
#include "log.h"

static BaseDesc bases[] = {
  {TYPE_BASE, 1, BASE_INT,   "int"   },
  {TYPE_BASE, 1, BASE_STR,   "string"},
  {TYPE_BASE, 1, BASE_ANY,   "any"   },
  {TYPE_BASE, 1, BASE_BYTE,  "byte"  },
  {TYPE_BASE, 1, BASE_CHAR,  "char"  },
  {TYPE_BASE, 1, BASE_FLOAT, "float" },
  {TYPE_BASE, 1, BASE_BOOL,  "bool"  },
};

TypeDesc *typedesc_getbase(int kind)
{
  BaseDesc *p;
  for (int i = 0; i < COUNT_OF(bases); ++i) {
    p = bases + i;
    if (p->type == kind)
      return (TypeDesc *)TYPE_INCREF(p);
  }
  return NULL;
}

char *basedesc_str(int kind)
{
  BaseDesc *p;
  for (int i = 0; i < COUNT_OF(bases); ++i) {
    p = bases + i;
    if (p->type == kind)
      return p->str;
  }
  return NULL;
}

static inline void check_base_refcnt(void)
{
  BaseDesc *p;
  for (int i = 0; i < COUNT_OF(bases); ++i) {
    p = bases + i;
    panic(p->refcnt != 1,
          "type of '%s' refcnt checked failed.",
          bases[i].str);
  }
}

void init_typedesc(void)
{

}

void fini_typedesc(void)
{
  check_base_refcnt();
}

static inline void *new_typedesc(int kind, int size)
{
  TypeDesc *desc = kmalloc(size);
  desc->kind = kind;
  desc->refcnt = 1;
  return desc;
}

#define DECLARE_TYPEDESC(name, kind, type) \
  type *name = new_typedesc(kind, sizeof(type))

TypeDesc *typedesc_getklass(char *path, char *type)
{
  DECLARE_TYPEDESC(desc, TYPE_KLASS, KlassDesc);
  desc->path = path;
  desc->type = type;
  return (TypeDesc *)desc;
}

TypeDesc *typedesc_getproto(Vector *args, TypeDesc *ret)
{
  DECLARE_TYPEDESC(desc, TYPE_PROTO, ProtoDesc);
  desc->args = args;
  desc->ret  = TYPE_INCREF(ret);
  return (TypeDesc *)desc;
}

void typedesc_free(TypeDesc *desc)
{
  int kind = desc->kind;
  switch (kind) {
  case TYPE_KLASS: {
    kfree(desc);
    break;
  }
  case TYPE_PROTO: {
    ProtoDesc *proto = (ProtoDesc *)desc;
    VECTOR_ITERATOR(iter, proto->args);
    TypeDesc *item;
    iter_for_each(&iter, item) {
      TYPE_DECREF(item);
    }
    kfree(proto->args);
    TYPE_DECREF(proto->ret);
    kfree(proto);
    break;
  }
  default:
    panic(1, "invalid type '%d' branch.", kind);
    break;
  }
}

void typedesc_tostring(TypeDesc *desc, StrBuf *buf)
{

}

void typedesc_show(TypeDesc *desc)
{
  if (desc != NULL) {
    STRBUF(sbuf);
    typedesc_tostring(desc, &sbuf);
    puts(strbuf_tostr(&sbuf)); /* with newline */
    strbuf_fini(&sbuf);
  } else {
    puts("<undefined-type>");  /* with newline */
  }
}

int typedesc_equal(TypeDesc *desc1, TypeDesc *desc2)
{

}

static TypeDesc *__to_klass(char *s, int len)
{
  char *dot = strrchr(s, '.');
  panic(!dot, "null pointer");
  char *path = atom_nstring(s, dot - s);
  char *type = atom_nstring(dot + 1, len - (dot - s) - 1);
  return typedesc_getklass(path, type);
}

static TypeDesc *__to_desc(char **str, int _dims, int _varg)
{
  char *s = *str;
  char ch = *s;
  TypeDesc *desc;

  switch (ch) {
  case 'L': {
    s++;
    char *k = s;
    while (*s != ';') s++;
    desc = __to_klass(k, s - k);
    s++;
    break;
  }
  default: {
    panic((_dims > 0 && _varg > 0),
          "invalid dims '%d' and varg '%d'.", _dims, _varg);
    desc = typedesc_getbase(ch);
    s++;
    break;
  }
  }
  *str = s;
  return desc;
}

TypeDesc *string_totypedesc(char *s)
{
  if (!s)
    return NULL;
  return __to_desc(&s, 0, 0);
}

TypeDesc *string_toproto(char *ptype, char *rtype)
{
  Vector *args = string_totypedescvec(ptype);
  TypeDesc *ret = string_totypedesc(rtype);
  return typedesc_getproto(args, ret);
}

Vector *string_totypedescvec(char *s)
{
  if (!s)
    return NULL;

  Vector *list = kmalloc(sizeof(*list));
  TypeDesc *desc;
  while (*s) {
    desc = __to_desc(&s, 0, 0);
    vector_push_back(list, desc);
  }
  return list;
}
