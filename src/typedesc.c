/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "typedesc.h"
#include "vector.h"
#include "path.h"
#include "atom.h"
#include "memory.h"

struct basedesc_s {
  struct basedesc type;
  char *str;
} basedescs[] = {
  { {TYPE_BYTE,  1}, "byte"   },
  { {TYPE_CHAR,  1}, "char"   },
  { {TYPE_INT,   1}, "int"    },
  { {TYPE_FLOAT, 1}, "float"  },
  { {TYPE_BOOL,  1}, "bool"   },
  { {TYPE_STR,   1}, "string" },
  { {TYPE_ANY,   1}, "any"    },
};

struct typedesc *get_basedesc(int kind)
{
  struct basedesc *p;
  for (int i = 0; i < sizeof(basedescs)/sizeof(basedescs[0]); i++) {
    p = &basedescs[i].type;
    if (p->kind == kind)
      return (struct typedesc *)p;
  }
  return NULL;
}

static inline void check_base_refcnt(void)
{
  struct basedesc *p;
  for (int i = 0; i < sizeof(basedescs)/sizeof(basedescs[0]); i++) {
    p = &basedescs[i].type;
    assert(p->refcnt == 1);
  }
}

void typedesc_destroy(void)
{
  check_base_refcnt();
}

static inline void *new_typedesc(int kind, int size)
{
  struct typedesc *desc = kmalloc(size);
  desc->kind = kind;
  desc->refcnt = 1;
  return desc;
}

#define DECLARE_TYPEDESC(name, kind, type) \
  type *name = new_typedesc(kind, sizeof(type))

struct typedesc *new_klassdesc(char **pathes, char *type)
{
  DECLARE_TYPEDESC(desc, TYPE_KLASS, struct klassdesc);
  desc->pathes = pathes;
  desc->type = type;
  return (struct typedesc *)desc;
}

struct typedesc *new_protodesc(struct typedesc **args, struct typedesc *ret)
{
  DECLARE_TYPEDESC(desc, TYPE_PROTO, struct protodesc);
  desc->args = args;
  desc->ret = TYPE_INCREF(ret);
  return (struct typedesc *)desc;
}

void typedesc_free(struct typedesc *desc)
{
  int kind = desc->kind;
  assert(kind > 127);
  switch (kind) {
  case TYPE_KLASS: {
    struct klassdesc *type = (struct klassdesc *)desc;
    kfree(type->pathes);
    kfree(type);
    break;
  }
  case TYPE_PROTO: {
    struct protodesc *proto = (struct protodesc *)desc;
    struct typedesc **_d = proto->args;
    while (*_d) {
      TYPE_DECREF(*_d);
      _d++;
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

void typedesc_tostr(struct typedesc *desc, struct strbuf *buf)
{

}

int typedesc_equal(struct typedesc *desc1, struct typedesc *desc2)
{

}

static struct typedesc *__to_klassdesc(char *s, int len)
{
  char *dot = strrchr(s, '.');
  assert(dot);
  char **pathes = path_toarr(s, dot - s);
  char *type = atom_nstring(dot + 1, len - (dot - s) - 1);
  return new_klassdesc(pathes, type);
}

static struct typedesc *__to_typedesc(char **str, int _dims, int _varg)
{
  char *s = *str;
  char ch = *s;
  struct typedesc *desc;

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

struct typedesc **typestr_toarr(char *s)
{
  if (!s)
    return NULL;

  VECTOR_PTR(vec);
  struct typedesc *desc;
  while (*s) {
    desc = __to_typedesc(&s, 0, 0);
    vector_push_back(&vec, &desc);
  }
  struct typedesc **arr = vector_toarr(&vec);
  vector_free(&vec, NULL, NULL);
  return arr;
}
