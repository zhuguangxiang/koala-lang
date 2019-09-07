/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "typedesc.h"
#include "atom.h"
#include "memory.h"
#include "log.h"

void literal_show(Literal *val, StrBuf *sbuf)
{
  char buf[128];
  switch (val->kind) {
  case 0:
    strbuf_append(sbuf, "(nil)");
    break;
  case BASE_INT:
    sprintf(buf, "%ld", val->ival);
    strbuf_append(sbuf, buf);
    break;
  case BASE_FLOAT:
    sprintf(buf, "%.8f", val->fval);
    strbuf_append(sbuf, buf);
    break;
  case BASE_BOOL:
    strbuf_append(sbuf, val->bval ? "true" : "false");
    break;
  case BASE_STR:
    sprintf(buf, "'%s'", val->str);
    strbuf_append(sbuf, buf);
    break;
  case BASE_CHAR:
    sprintf(buf, "%d", val->cval.val);
    strbuf_append(sbuf, buf);
    break;
  default:
    panic("invalid branch %d", val->kind);
    break;
  }
}

TypeParaDef *new_typeparadef(char *name, Vector *types)
{
  TypeParaDef *typepara = kmalloc(sizeof(TypeParaDef));
  typepara->name = name;
  typepara->types = types;
  return typepara;
}

static void free_descs(Vector *vec)
{
  TypeDesc *tmp;
  vector_for_each(tmp, vec) {
    TYPE_DECREF(tmp);
  }
  vector_free(vec, NULL, NULL);
}

static void free_typeparadefs(Vector *vec)
{
  TypeParaDef *item;
  vector_for_each(item, vec) {
    free_descs(item->types);
    kfree(item);
  }
  vector_free(vec, NULL, NULL);
}

static TypeDesc bases[] = {
  {.kind = TYPE_BASE, .refcnt = 1, .base = BASE_INT  },
  {.kind = TYPE_BASE, .refcnt = 1, .base = BASE_STR  },
  {.kind = TYPE_BASE, .refcnt = 1, .base = BASE_ANY  },
  {.kind = TYPE_BASE, .refcnt = 1, .base = BASE_BOOL },
  {.kind = TYPE_BASE, .refcnt = 1, .base = BASE_BYTE },
  {.kind = TYPE_BASE, .refcnt = 1, .base = BASE_CHAR },
  {.kind = TYPE_BASE, .refcnt = 1, .base = BASE_FLOAT},
};

static struct {
  char base;
  char *str;
} basestrs[] = {
  {BASE_INT,   "int"   },
  {BASE_STR,   "string"},
  {BASE_ANY,   "any"   },
  {BASE_BOOL,  "bool"  },
  {BASE_BYTE,  "byte"  },
  {BASE_CHAR,  "char"  },
  {BASE_FLOAT, "float" },
};

char *base_str(int kind)
{
  for (int i = 0; i < COUNT_OF(basestrs); ++i) {
    if (basestrs[i].base == kind)
      return basestrs[i].str;
  }
  return NULL;
}

TypeDesc *desc_from_base(int kind)
{
  TypeDesc *p;
  for (int i = 0; i < COUNT_OF(bases); ++i) {
    p = bases + i;
    if (p->base == kind)
      return TYPE_INCREF(p);
  }
  return NULL;
}

static inline void check_base_refcnt(void)
{
  TypeDesc *p;
  for (int i = 0; i < COUNT_OF(bases); ++i) {
    p = bases + i;
    if (p->refcnt != 1)
      panic("expected type of '%s' refcnt is 1, but %d.",
            base_str(p->base), p->refcnt);
  }
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

TypeDesc *desc_from_array(TypeDesc *para)
{
  Vector *types = vector_new();
  vector_push_back(types, TYPE_INCREF(para));
  TypeDesc *desc = desc_from_klass("lang", "Array");
  desc->klass.types = types;
  return desc;
}

TypeDesc *desc_from_varg(TypeDesc *base)
{
  TypeDesc *desc = kmalloc(sizeof(TypeDesc));
  desc->kind = TYPE_VARG;
  desc->refcnt = 1;
  desc->varg.type = TYPE_INCREF(base);
  return desc;
}

TypeDesc *desc_from_map(TypeDesc *key, TypeDesc *val)
{
  Vector *types = vector_new();
  vector_push_back(types, TYPE_INCREF(key));
  vector_push_back(types, TYPE_INCREF(val));
  TypeDesc *desc = desc_from_klass("lang", "Dict");
  desc->klass.types = types;
  return desc;
}

void desc_free(TypeDesc *desc)
{
  int kind = desc->kind;
  free_typeparadefs(desc->typeparas);
  switch (kind) {
  case TYPE_KLASS: {
    free_descs(desc->klass.types);
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
    TYPE_DECREF(desc->varg.type);
    kfree(desc);
    break;
  }
  case TYPE_PARAREF: {
    kfree(desc);
    break;
  }
  default:
    panic("invalid type '%d' branch.", kind);
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
  if (desc1 == desc2)
    return 1;

  if (desc_isany(desc1))
    return 1;

  if (desc1->kind != desc2->kind)
    return 0;

  switch (desc1->kind) {
  case TYPE_KLASS:
    if (strcmp(desc1->klass.path, desc2->klass.path))
      return 0;
    if (strcmp(desc1->klass.type, desc2->klass.type))
      return 0;
    return 1;
  case TYPE_PROTO:
    break;
  default:
    break;
  }
  return 0;
}

static TypeDesc *__to_klass(char *s, int len)
{
  char *dot = strrchr(s, '.');
  if (!dot)
    panic("null pointer");
  char *path = atom_nstring(s, dot - s);
  char *type = atom_nstring(dot + 1, len - (dot - s) - 1);
  return desc_from_klass(path, type);
}

static TypeDesc *__to_pararef(char *s, int len)
{
  TypeDesc *desc = kmalloc(sizeof(*desc));
  desc->kind = TYPE_PARAREF;
  desc->refcnt = 1;
  desc->pararef.name = atom_nstring(s, len);
  desc->pararef.index = -1;
  return desc;
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
    desc = __to_pararef(k, s - k);
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
    if (desc == NULL)
      panic("invalid typestr:%s", s);
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
