
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "integer_object.h"
#include "tuple_object.h"
#include "module_object.h"
#include "string_object.h"

struct object *integer_from_int64(int64_t val)
{
  struct integer_object *io = malloc(sizeof(*io));
  init_object_head(io, &integer_klass);
  io->val   = val;
  return (struct object *)io;
}

int64_t integer_to_int64(struct object *ob)
{
  assert(OB_KLASS(ob) == &integer_klass);

  struct integer_object *integer = (struct integer_object *)ob;
  return integer->val;
}

static struct object *integer_new(struct object *klass, struct object *args)
{
  assert(OB_KLASS(klass) == &integer_klass);
  assert(tuple_size(args) == 1);
  int64_t val = 0;
  tuple_parse(args, "i", &val);

  return integer_from_int64(val);
}

struct klass_object integer_klass = {
  OBJECT_HEAD_INIT(&klass_klass),
  .name  = "Integer",
  .bsize = sizeof(struct integer_object),

  .ob_scan  = NULL,
  .ob_new   = integer_new,
  .ob_free  = NULL,
};

static struct object *integer_tostring(struct object *ob, struct object *args)
{
  assert(OB_KLASS(ob) == &integer_klass);
  assert(args == NULL);

  struct integer_object *integer = (struct integer_object *)ob;
  char temp[32];
  snprintf(temp, 32, "%lld", integer->val);
  return string_from_cstr(temp);
}

static struct cfunc_struct integer_functions[] = {
  {"Operator_add",        "(I)(I)",   0, NULL},
  {"Operator_subtract",   "(I)(I)",   0, NULL},
  {"Operator_multiply",   "(I)(I)",   0, NULL},
  {"Operator_divide",     "(I)(I)",   0, NULL},
  {"Operator_remainder",  "(I)(I)",   0, NULL},
  {"Operator_divmod",     "(I)(II)",  0, NULL},
  {"Operator_positive",   "(I)(I)",   0, NULL},
  {"Operator_negative",   "(I)(I)",   0, NULL},
  {"Operator_lshift",     "(I)(I)",   0, NULL},
  {"Operator_rshift",     "(I)(I)",   0, NULL},
  {"Operator_and",        "(I)(I)",   0, NULL},
  {"Operator_or",         "(I)(I)",   0, NULL},
  {"ToString",            "(V)(Okoala/lang.String)", 0, integer_tostring},
  {NULL, NULL, 0, NULL}
};

void init_integer_klass(void)
{
  struct object *ko = (struct object *)&integer_klass;
  klass_add_cfunctions(ko, integer_functions);
  struct object *mo = find_module("koala/lang");
  assert(mo);
  module_add(mo, new_klass_namei("Integer", 0), ko);
}
