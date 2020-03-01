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

#include "stringobject.h"
#include "hashmap.h"
#include "intobject.h"
#include "floatobject.h"
#include "arrayobject.h"
#include "valistobject.h"
#include "fmtmodule.h"
#include "utf8.h"
#include "log.h"

static Object *str_num_add(Object *x, Object *y)
{
  if (!string_check(x)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(x));
    return NULL;
  }

  if (!string_check(y)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(y));
    return NULL;
  }

  Object *z;
  STRBUF(sbuf);
  strbuf_append(&sbuf, string_asstr(x));
  strbuf_append(&sbuf, string_asstr(y));
  z = string_new(strbuf_tostr(&sbuf));
  strbuf_fini(&sbuf);
  return z;
}

static Object *string_length(Object *self, Object *args)
{
  if (!string_check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return NULL;
  }

  if (args != NULL) {
    error("length() of 'String' has no arguments");
    return NULL;
  }

  StringObject *s = (StringObject *)self;
  return integer_new(s->len);
}

static Object *string_fmt(Object *self, Object *ob)
{
  if (!string_check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return NULL;
  }

  STRBUF(sbuf);
  strbuf_append_char(&sbuf, '\'');
  strbuf_append(&sbuf, string_asstr(self));
  strbuf_append_char(&sbuf, '\'');
  Object *str = string_new(strbuf_tostr(&sbuf));
  strbuf_fini(&sbuf);
  Fmtter_WriteString(ob, str);
  OB_DECREF(str);
  return NULL;
}

static Object *string_hash(Object *self, Object *args)
{
  if (!string_check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return NULL;
  }

  return integer_new(strhash(string_asstr(self)));
}

static Object *string_equal(Object *self, Object *other)
{
  if (!string_check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return NULL;
  }

  if (!string_check(other)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(other));
    return NULL;
  }

  if (self == other)
    return bool_true();

  StringObject *s1 = (StringObject *)self;
  StringObject *s2 = (StringObject *)other;

  return !strcmp(s1->wstr, s2->wstr) ? bool_true() : bool_false();
}

/* func as_bytes() [byte] */
Object *string_asbytes(Object *self, Object *args)
{
  if (!string_check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return NULL;
  }

  StringObject *s = (StringObject *)self;
  GVector *vec = gvector_new(s->len, 1);
  gvector_append_array(vec, s->wstr, s->len);
  TypeDesc *desc = desc_from_byte;
  Object *ob = array_new(desc, vec);
  TYPE_DECREF(desc);
  if (ob == NULL) {
    gvector_free(vec);
    error("memory allocated failed.");
    return NULL;
  }
  return ob;
}

static void obj_tostr(Object *ob, StrBuf *buf)
{
  TypeDesc *desc = OB_TYPE(ob)->desc;
  char base = desc->base;
  switch (base) {
  case BASE_BYTE:
    strbuf_append_int(buf, byte_asint(ob));
    break;
  case BASE_INT:
    strbuf_append_int(buf, integer_asint(ob));
    break;
  case BASE_CHAR:
    strbuf_append_int(buf, char_asch(ob));
    break;
  case BASE_FLOAT:
    strbuf_append_float(buf, float_asflt(ob));
    break;
  case BASE_BOOL:
    strbuf_append(buf, bool_istrue(ob) ? "true" : "false");
    break;
  case BASE_STR:
    strbuf_append(buf, string_asstr(ob));
    break;
  default:
    break;
  }
}

static Object *__format__(char *format, Object *vargs)
{
  STRBUF(buf);
  char *s = format;
  char ch;
  int idx = 0;
  Object *ob;

  while ((ch = *s++)) {
    if (ch == '{') {
      if ((ch = *s++)) {
        if (ch == '}') {
          ob = valist_get(vargs, idx++);
          obj_tostr(ob, &buf);
          OB_DECREF(ob);
        } else {
          strbuf_append_char(&buf, ch);
        }
      } else {
        goto exit_label;
      }
    } else {
      strbuf_append_char(&buf, ch);
    }
  }

exit_label:
  ob = string_new(strbuf_tostr(&buf));
  strbuf_fini(&buf);
  return ob;
}

/* func format(args ...) string */
Object *string_format(Object *self, Object *args)
{
  if (!string_check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return NULL;
  }

  if (args == NULL) {
    // no any arguments, just return string self.
    return OB_INCREF(self);
  }

  if (!valist_check(args)) {
    error("object of '%.64s' is not a VaList", OB_TYPE_NAME(args));
    return NULL;
  }

  return __format__(string_asstr(self), args);
}

static void string_clean(Object *self)
{
  if (!string_check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return;
  }
  debug("clean String '%.64s'", string_asstr(self));
  StringObject *s = (StringObject *)self;
  kfree(s->wstr);
}

static void string_free(Object *self)
{
  string_clean(self);
  gcfree(self);
}

static int str_num_cmp(Object *x, Object *y)
{
  if (!string_check(x)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(x));
    return -1;
  }

  if (!string_check(y)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(y));
    return -1;
  }

  char *s1 = string_asstr(x);
  char *s2 = string_asstr(y);
  return strcmp(s1, s2);
}

static Object *str_num_gt(Object *x, Object *y)
{
  int r = str_num_cmp(x, y);
  return (r > 0) ? bool_true() : bool_false();
}

static Object *str_num_ge(Object *x, Object *y)
{
  int r = str_num_cmp(x, y);
  return (r >= 0) ? bool_true() : bool_false();
}

static Object *str_num_lt(Object *x, Object *y)
{
  int r = str_num_cmp(x, y);
  return (r < 0) ? bool_true() : bool_false();
}

static Object *str_num_le(Object *x, Object *y)
{
  int r = str_num_cmp(x, y);
  return (r < 0) ? bool_true() : bool_false();
}

static Object *str_num_eq(Object *x, Object *y)
{
  int r = str_num_cmp(x, y);
  return (r == 0) ? bool_true() : bool_false();
}

static Object *str_num_neq(Object *x, Object *y)
{
  int r = str_num_cmp(x, y);
  return (r != 0) ? bool_true() : bool_false();
}

static MethodDef string_methods[] = {
  {"concat",  "s",  "s", str_num_add},
  {"length",  NULL, "i", string_length},
  {"__fmt__", "Lfmt.Formatter;", NULL, string_fmt},
  {"__add__", "s", "s", str_num_add},
  {"__gt__", "s", "z", str_num_gt},
  {"__ge__", "s", "z", str_num_ge},
  {"__lt__", "s", "z", str_num_lt},
  {"__le__", "s", "z", str_num_le},
  {"__eq__", "s", "z", str_num_eq},
  {"__neq__", "s", "z", str_num_neq},
  {"__match__", "s", "z", string_equal},
  {"as_bytes", NULL, "[b", string_asbytes},
  {"format", "...", "s", string_format},
  {NULL}
};

static NumberMethods string_num_methods = {
  .add = str_num_add,

  .gt  = str_num_gt,
  .ge  = str_num_ge,
  .lt  = str_num_lt,
  .le  = str_num_le,
  .eq  = str_num_eq,
};

TypeObject string_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name    = "String",
  .hash    = string_hash,
  .equal   = string_equal,
  .clean   = string_clean,
  .free    = string_free,
  .number  = &string_num_methods,
  .methods = string_methods,
};

void init_string_type(void)
{
  string_type.desc = desc_from_str;
  if (type_ready(&string_type) < 0)
    panic("Cannot initalize 'string_type' type.");
}

Object *string_new(char *str)
{
  debug("string_new:%s", str);
  int len = strlen(str);
  StringObject *s = gcnew(sizeof(StringObject));
  init_object_head(s, &string_type);
  s->len = len;
  s->wstr = kmalloc(len + 1);
  strcpy(s->wstr, str);
  return (Object *)s;
}

char *string_asstr(Object *self)
{
  if (self == NULL)
    return NULL;

  if (!string_check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return NULL;
  }

  StringObject *s = (StringObject *)self;
  return s->wstr;
}

int string_isempty(Object *self)
{
  if (!string_check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return 0;
  }

  StringObject *s = (StringObject *)self;
  if (s->wstr == NULL)
    return 1;
  if (strlen(s->wstr) == 0)
    return 1;
  return 0;
}

void string_set(Object *self, char *str)
{
  if (!string_check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return;
  }

  StringObject *s = (StringObject *)self;
  kfree(s->wstr);
  s->wstr = kmalloc(strlen(str) + 1);
  strcpy(s->wstr, str);
}

static void char_free(Object *ob)
{
  if (!char_check(ob)) {
    error("object of '%.64s' is not a Char", OB_TYPE_NAME(ob));
    return;
  }
#if !defined(NLog)
  CharObject *ch = (CharObject *)ob;
  debug("[Freed] Char %s", (char *)&ch->value);
#endif
  kfree(ob);
}

static Object *char_str(Object *self, Object *ob)
{
  if (!char_check(self)) {
    error("object of '%.64s' is not a Char", OB_TYPE_NAME(self));
    return NULL;
  }

  CharObject *ch = (CharObject *)self;
  char buf[8] = {'\'', 0};
  int bytes = encode_one_utf8_char(ch->value, buf + 1);
  buf[bytes + 1] = '\'';
  return string_new(buf);
}

TypeObject char_type = {
  OBJECT_HEAD_INIT(&type_type)
  .name    = "Character",
  .free    = char_free,
  .str     = char_str,
};

void init_char_type(void)
{
  char_type.desc = desc_from_char;
  if (type_ready(&char_type) < 0)
    panic("Cannot initalize 'Character' type.");
}

Object *char_new(unsigned int val)
{
  CharObject *ch = kmalloc(sizeof(*ch));
  init_object_head(ch, &char_type);
  ch->value = val;
  return (Object *)ch;
}
