
#include "mod_io.h"

static int nil_print(char *buf, int sz, TValue *val)
{
  UNUSED_PARAMETER(val);
  return snprintf(buf, sz, "(nil)");
}

static int int_print(char *buf, int sz, TValue *val)
{
  return snprintf(buf, sz, "%lld", VALUE_INT(val));
}

static int flt_print(char *buf, int sz, TValue *val)
{
  return snprintf(buf, sz, "%f", VALUE_FLOAT(val));
}

static int bool_print(char *buf, int sz, TValue *val)
{
  return snprintf(buf, sz, "%s", VALUE_BOOL(val) ? "true" : "false");
}

static int object_print(char *buf, int sz, TValue *val)
{
  Object *ob = VALUE_OBJECT(val);
  Klass *klazz = OB_KLASS(ob);
  ob = klazz->ob_tostr(val);
  OB_ASSERT_KLASS(ob, String_Klass);
  return snprintf(buf, sz, "%s", String_RawString(ob));
}

static int cstr_print(char *buf, int sz, TValue *val)
{
  return snprintf(buf, sz, "%s", VALUE_CSTR(val));
}

static struct io_print_func {
  int type;
  int (*print)(char *, int, TValue *);
} io_print_funcs[] = {
  {TYPE_NIL, nil_print},
  {TYPE_INT, int_print},
  {TYPE_FLOAT, flt_print},
  {TYPE_BOOL, bool_print},
  {TYPE_OBJECT, object_print},
  {TYPE_CSTR, cstr_print},
};

static void *__get_print_func(TValue *v)
{
  for (int i = 0; i < nr_elts(io_print_funcs); i++) {
    if (io_print_funcs[i].type == VALUE_TYPE(v))
      return io_print_funcs[i].print;
  }
  return NULL;
}

static Object *__io_print(Object *ob, Object *args)
{
  UNUSED_PARAMETER(ob);
  if (args == NULL) {
    fprintf(stdout, "(nil)");
    return NULL;
  }

#define BUF_SIZE  512

  char temp[BUF_SIZE];
  char *buf = temp;
  int sz = Tuple_Size(args);
  TValue val;
  int (*print_func)(char *, int, TValue *);
  int avail = 0;
  for (int i = 0; i < sz; i++) {
    val = Tuple_Get(args, i);
    print_func = __get_print_func(&val);
    if (print_func != NULL) {
      avail = print_func(buf, BUF_SIZE - (buf - temp), &val);
      buf += avail;
      avail = snprintf(buf, BUF_SIZE - (buf - temp), " ");
      buf += avail;
    }
  }
  temp[buf - temp] = '\0';

  fwrite(temp, strlen(temp), 1, stdout);
  fflush(stdout);
  return NULL;
}

static FuncDef io_funcs[] = {
  {"Print", {0, NULL, 1, "[A"}, __io_print},
  {NULL}
};

void Init_IO_Module(void)
{
  Object *ob = Module_New("io", "koala/io");
  Module_Add_CFunctions(ob, io_funcs);
}
