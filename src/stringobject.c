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
#include "utf8.h"
#include "atom.h"
#include "log.h"

static Object *str_init(Object *x, Object *y)
{
  // not run here!!
  expect(0);
  return NULL;
}

static char *__asstr(Object *self)
{
  StringObject *s = (StringObject *)self;
  return s->str;
}

static int __len(Object *self)
{
  StringObject *s = (StringObject *)self;
  return s->len;
}

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
  strbuf_append(&sbuf, __asstr(x));
  strbuf_append(&sbuf, __asstr(y));
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

  return integer_new(__len(self));
}

static Object *string_hash(Object *self, Object *args)
{
  if (!string_check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return NULL;
  }

  return integer_new(strhash(__asstr(self)));
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

  char *s1 = __asstr(self);
  char *s2 = __asstr(other);
  return !strcmp(s1, s2) ? bool_true() : bool_false();
}

/* func as_bytes() [byte] */
Object *string_asbytes(Object *self, Object *args)
{
  if (!string_check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return NULL;
  }

  Object *ob = byte_array_new();
  Slice *buf = array_slice(ob);
  slice_push_array(buf, __asstr(self), __len(self));
  return ob;
}

static void obj_tostr(Object *ob, StrBuf *buf)
{
  TypeDesc *desc = OB_TYPE(ob)->desc;
  char base = desc->base;
  switch (base) {
  case BASE_BYTE:
    strbuf_append_byte(buf, byte_asint(ob));
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
    strbuf_append(buf, __asstr(ob));
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
  int len = valist_len(vargs);

  while ((ch = *s++)) {
    if (ch == '{') {
      if ((ch = *s++)) {
        if (ch == '}') {
          if (len <= 0) {
            // output original
            strbuf_append(&buf, "{}");
          } else {
            if (idx >= len) {
              goto exit_label;
            } else {
              ob = valist_get(vargs, idx++);
              obj_tostr(ob, &buf);
              OB_DECREF(ob);
            }
          }
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
  s = strbuf_tostr(&buf);
  if (s == NULL) {
    strbuf_fini(&buf);
    return string_new("");
  } else {
    ob = string_new(s);
    strbuf_fini(&buf);
    return ob;
  }
}

/* func format(args ...) string */
Object *string_format(Object *self, Object *args)
{
  if (!string_check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return NULL;
  }

  if (!valist_check(args)) {
    error("object of '%.64s' is not a VaList", OB_TYPE_NAME(args));
    return NULL;
  }

  if (valist_len(args) <= 0) {
    // no any va-args, just return string self.
    return OB_INCREF(self);
  }

  return __format__(__asstr(self), args);
}

static void string_clean(Object *self)
{
  if (!string_check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return;
  }
  debug("clean String '%.64s'", __asstr(self));
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

  char *s1 = __asstr(x);
  char *s2 = __asstr(y);
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
  {"__init__", "s", NULL, str_init},
  {"concat",  "s",  "s", str_num_add},
  {"len",  NULL, "i", string_length},
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
  StringObject *s = gcnew(sizeof(StringObject));
  init_object_head(s, &string_type);
  s->len = strlen(str);
  s->str = atom(str);
  return (Object *)s;
}

Object *string_with_len(char *str, int len)
{
  StringObject *s = gcnew(sizeof(StringObject));
  init_object_head(s, &string_type);
  s->len = len;
  s->str = atom_nstring(str, len);
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

  return __asstr(self);
}

int string_len(Object *self)
{
  if (!string_check(self)) {
    error("object of '%.64s' is not a String", OB_TYPE_NAME(self));
    return -1;
  }

  return __len(self);
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
  char buf[8] = {'\''};
  int bytes;
  char *ptr;
  if (ch->value == 0) {
    bytes = 0;
  } else if (ch->value <= 0xFF) {
    ptr = (char *)&ch->value;
    buf[1] = ptr[0];
    bytes = 1;
  } else if (ch->value <= 0xFFFF) {
    ptr = (char *)&ch->value;
    buf[1] = ptr[0];
    buf[2] = ptr[1];
    bytes = 2;
  } else if (ch->value <= 0xFFFFFF) {
    ptr = (char *)&ch->value;
    buf[1] = ptr[0];
    buf[2] = ptr[1];
    buf[3] = ptr[2];
    bytes = 3;
  } else {
    ptr = (char *)&ch->value;
    buf[1] = ptr[0];
    buf[2] = ptr[1];
    buf[3] = ptr[2];
    buf[4] = ptr[3];
    bytes = 4;
  }
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
