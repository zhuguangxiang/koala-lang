
#include "tuple_object.h"
#include "integer_object.h"
#include "module_object.h"

struct object *new_tuple(int size)
{
  int sz = sizeof(struct tuple_object) + size * sizeof(struct object *);
  struct tuple_object *tuple = malloc(sz);

  init_object_head(tuple, &tuple_klass);
  tuple->size = size;
  for (int i = 0; i < size; i++) {
    tuple->items[i] = NULL;
  }

  return (struct object *)tuple;
}

size_t tuple_size(struct object *ob)
{
  struct tuple_object *tuple = (struct tuple_object *)ob;
  return tuple->size;
}

struct object *tuple_get(struct object *ob, int index)
{
  struct tuple_object *tuple = (struct tuple_object *)ob;

  if (index < 0 || index >= tuple->size)
    return NULL;

  return tuple->items[index];
}

int tuple_set(struct object *ob, int index, struct object *val)
{
  struct tuple_object *tuple = (struct tuple_object *)ob;

  if (index < 0 || index >= tuple->size)
    return -1;

  tuple->items[index] = val;
  return 0;
}

static int get_simple_arg(char ch, struct object *tuple, int idx, void *v)
{
  int ret = 0;
  struct object *ob = NULL;

  switch (ch) {
    case 'i': {
      ob = tuple_get(tuple, idx);
      if (ob == NULL || (OB_KLASS(ob) != &integer_klass)) {
        fprintf(stderr, "[ERROR]get arg failed\n");
        ret = -1;
      } else {
        *(int64_t *)v = integer_to_int64(ob);
      }
      break;
    }

    case 'b': {

      break;
    }

    case 's': {

      break;
    }

    case 'O': {

      break;
    }

    default: {
      assert(0);
      break;
    }
  }

  return ret;
}

static int get_args(struct object *tuple, int min, int max,
                    char *format, va_list *var_list)
{
  char ch;
  int index = min;
  while ((ch = *format++) && (index <= max)) {
    void *addr = va_arg(*var_list, void *);
    get_simple_arg(ch, tuple, index, addr);
    ++index;
  }

  return 0;
}

int tuple_get_range(struct object *ob, int min, int max, char *format, ...)
{
  int ret;
  va_list va;
  va_start(va, format);
  ret = get_args(ob, min, max, format, &va);
  va_end(va);
  return ret;
}

int tuple_parse(struct object *tuple, char *format, ...)
{
  int ret;
  va_list va;
  va_start(va, format);
  ret = get_args(tuple, 0, tuple_size(tuple) - 1, format, &va);
  va_end(va);
  return ret;
}

int tuple_unpack_range(struct object *ob, int min, int max, ...)
{
  return 0;
}

int tuple_unpack(struct object *ob, ...)
{
  return 0;
}

struct object *tuple_pack(size_t size, ...)
{
  return NULL;
}

struct klass_object tuple_klass = {
  OBJECT_HEAD_INIT(&klass_klass),
  .name = "Tuple",
  .bsize = sizeof(struct tuple_object),
  .isize = sizeof(struct object *),
};

static struct object *__tuple_get(struct object *ob, struct object *args)
{
  int index;
  if (tuple_parse(args, "i", &index)) return NULL;

  return tuple_get(ob, index);
}

static struct object *__tuple_size(struct object *ob, struct object *args)
{
  struct tuple_object *tuple = (struct tuple_object *)ob;
  return integer_from_int64(tuple->size);
}

static struct cfunc_struct tuple_functions[] = {
  {"Get", "(I)(Okoala/lang.Any)", 0, __tuple_get},
  {"Size", "(V)(I)", 0, __tuple_size},
  {NULL, NULL, 0, NULL}
};

void init_tuple_klass(void)
{
  struct object *ko = (struct object *)&tuple_klass;
  klass_add_cfunctions(ko, tuple_functions);
  struct object *mo = find_module("koala/lang");
  assert(mo);
  module_add(mo, new_klass_namei("Tuple", 0), ko);
}
