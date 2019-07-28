/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "common.h"
#include "typedesc.h"
#include "vector.h"
#include "node.h"
#include "atom.h"
#include "memory.h"

struct {
  BaseDesc type;
  char *str;
} basearray[] = {
  { {TYPE_BYTE,  1}, "byte"   },
  { {TYPE_CHAR,  1}, "char"   },
  { {TYPE_INT,   1}, "int"    },
  { {TYPE_FLOAT, 1}, "float"  },
  { {TYPE_BOOL,  1}, "bool"   },
  { {TYPE_STR,   1}, "string" },
  { {TYPE_ANY,   1}, "any"    },
};

TypeDesc *get_basedesc(int kind)
{
  BaseDesc *p;
  for (int i = 0; i < COUNT_OF(basearray); ++i) {
    p = &basearray[i].type;
    if (p->kind == kind)
      return (TypeDesc *)p;
  }
  return NULL;
}

static inline void check_base_refcnt(void)
{
  BaseDesc *p;
  for (int i = 0; i < COUNT_OF(basearray); ++i) {
    p = &basearray[i].type;
    assert(p->refcnt == 1);
  }
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

TypeDesc *new_klassdesc(char **pathes, char *type)
{
  DECLARE_TYPEDESC(desc, TYPE_KLASS, KlassDesc);
  desc->pathes = pathes;
  desc->type = type;
  return (TypeDesc *)desc;
}

TypeDesc *new_protodesc(TypeDesc **args, TypeDesc *ret)
{
  DECLARE_TYPEDESC(desc, TYPE_PROTO, ProtoDesc);
  desc->args = args;
  desc->ret = TYPE_INCREF(ret);
  return (TypeDesc *)desc;
}

TypeDesc *TypeStr_ToProto(char *ptype, char *rtype)
{
  TypeDesc **args = typestr_toarr(ptype);
  TypeDesc *ret = TypeStr_ToDesc(rtype);
  return new_protodesc(args, ret);
}

void typedesc_free(TypeDesc *desc)
{
  int kind = desc->kind;
  assert(kind > 127);
  switch (kind) {
  case TYPE_KLASS: {
    KlassDesc *type = (KlassDesc *)desc;
    kfree(type->pathes);
    kfree(type);
    break;
  }
  case TYPE_PROTO: {
    ProtoDesc *proto = (ProtoDesc *)desc;
    TypeDesc **type = proto->args;
    while (*type) {
      TYPE_DECREF(*type);
      type++;
    }
    kfree(proto->args);
    TYPE_DECREF(proto->ret);
    kfree(proto);
    break;
  }
  default:
    assert(0);
    break;
  }
}

void typedesc_tostr(TypeDesc *desc, struct strbuf *buf)
{

}

int typedesc_equal(TypeDesc *desc1, TypeDesc *desc2)
{

}

static TypeDesc *__to_klassdesc(char *s, int len)
{
  char *dot = strrchr(s, '.');
  assert(dot);
  char **pathes = path_toarr(s, dot - s);
  char *type = atom_nstring(dot + 1, len - (dot - s) - 1);
  return new_klassdesc(pathes, type);
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
    desc = __to_klassdesc(k, s - k);
    s++;
    break;
  }
  default: {
    assert(!(_dims > 0 && _varg > 0));
    desc = get_basedesc(ch);
    TYPE_INCREF(desc);
    s++;
    break;
  }
  }
  *str = s;
  return desc;
}

TypeDesc *TypeStr_ToDesc(char *s)
{
  if (!s) return NULL;
  return __to_desc(&s, 0, 0);
}

TypeDesc **typestr_toarr(char *s)
{
  if (!s)
    return NULL;

  VECTOR_PTR(vec);
  TypeDesc *desc;
  while (*s) {
    desc = __to_desc(&s, 0, 0);
    vector_push_back(&vec, &desc);
  }
  TypeDesc **arr = vector_toarr(&vec);
  vector_free(&vec, NULL, NULL);
  return arr;
}
