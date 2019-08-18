/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "typedesc.h"
#include "atom.h"
#include "memory.h"
#include "log.h"

void constvalue_show(ConstValue *val, StrBuf *sbuf)
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

static TypeDesc bases[] = {
  {TYPE_BASE, 1, .base = {BASE_INT,   "int"   }},
  {TYPE_BASE, 1, .base = {BASE_STR,   "string"}},
  {TYPE_BASE, 1, .base = {BASE_ANY,   "any"   }},
  {TYPE_BASE, 1, .base = {BASE_BYTE,  "byte"  }},
  {TYPE_BASE, 1, .base = {BASE_CHAR,  "char"  }},
  {TYPE_BASE, 1, .base = {BASE_FLOAT, "float" }},
  {TYPE_BASE, 1, .base = {BASE_BOOL,  "bool"  }},
};

TypeDesc *desc_getbase(int kind)
{
  TypeDesc *p;
  for (int i = 0; i < COUNT_OF(bases); ++i) {
    p = bases + i;
    if (p->base.type == kind)
      return TYPE_INCREF(p);
  }
  return NULL;
}

char *basedesc_str(int kind)
{
  TypeDesc *p;
  for (int i = 0; i < COUNT_OF(bases); ++i) {
    p = bases + i;
    if (p->base.type == kind)
      return p->base.str;
  }
  return NULL;
}

static inline void check_base_refcnt(void)
{
  TypeDesc *p;
  for (int i = 0; i < COUNT_OF(bases); ++i) {
    p = bases + i;
    if (p->refcnt != 1)
      panic("type of '%s' refcnt checked failed.",
            p->base.str);
  }
}

void init_typedesc(void)
{

}

void fini_typedesc(void)
{
  check_base_refcnt();
}

TypeDesc *desc_getklass(char *path, char *type)
{
  TypeDesc *desc = kmalloc(sizeof(TypeDesc));
  desc->kind = TYPE_KLASS;
  desc->refcnt = 1;
  desc->klass.path = path;
  desc->klass.type = type;
  return desc;
}

TypeDesc *desc_getproto(Vector *args, TypeDesc *ret)
{
  TypeDesc *desc = kmalloc(sizeof(TypeDesc));
  desc->kind = TYPE_PROTO;
  desc->refcnt = 1;
  desc->proto.args = args;
  desc->proto.ret  = TYPE_INCREF(ret);
  return desc;
}

void desc_free(TypeDesc *desc)
{
  int kind = desc->kind;
  switch (kind) {
  case TYPE_KLASS: {
    kfree(desc);
    break;
  }
  case TYPE_PROTO: {
    vector_free(desc->proto.args, NULL, NULL);
    TYPE_DECREF(desc->proto.ret);
    kfree(desc);
    break;
  }
  default:
    panic("invalid type '%d' branch.", kind);
    break;
  }
}

void desc_tostring(TypeDesc *desc, StrBuf *buf)
{

}

void desc_show(TypeDesc *desc)
{
  if (desc != NULL) {
    STRBUF(sbuf);
    desc_tostring(desc, &sbuf);
    puts(strbuf_tostr(&sbuf)); /* with newline */
    strbuf_fini(&sbuf);
  } else {
    puts("<undefined-type>");  /* with newline */
  }
}

int desc_equal(TypeDesc *desc1, TypeDesc *desc2)
{

}

static TypeDesc *__to_klass(char *s, int len)
{
  char *dot = strrchr(s, '.');
  if (!dot)
    panic("null pointer");
  char *path = atom_nstring(s, dot - s);
  char *type = atom_nstring(dot + 1, len - (dot - s) - 1);
  return desc_getklass(path, type);
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
    if (_dims > 0 && _varg > 0)
     panic("invalid dims '%d' & varg '%d'.", _dims, _varg);
    desc = desc_getbase(ch);
    s++;
    break;
  }
  }
  *str = s;
  return desc;
}

TypeDesc *string_todesc(char *s)
{
  if (!s)
    return NULL;
  return __to_desc(&s, 0, 0);
}

TypeDesc *string_toproto(char *ptype, char *rtype)
{
  Vector *args = string_todescs(ptype);
  TypeDesc *ret = string_todesc(rtype);
  return desc_getproto(args, ret);
}

Vector *string_todescs(char *s)
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
