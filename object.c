
#include "object.h"
#include "stringobject.h"
#include "methodobject.h"
#include "debug.h"

TValue NilValue  = NIL_VALUE_INIT();
TValue TrueValue = BOOL_VALUE_INIT(1);
TValue FalseValue = BOOL_VALUE_INIT(0);

static void va_integer_build(TValue *v, va_list *ap)
{
  v->type = TYPE_INT;
  v->ival = (int64)va_arg(*ap, uint32);
}

static void va_integer_parse(TValue *v, va_list *ap)
{
  VALUE_ASSERT_TYPE(v, TYPE_INT);
  int32 *i = va_arg(*ap, int32 *);
  *i = (int32)VALUE_INT(v);
}

static void va_long_build(TValue *v, va_list *ap)
{
  v->type = TYPE_INT;
  v->ival = va_arg(*ap, uint64);
}

static void va_long_parse(TValue *v, va_list *ap)
{
  VALUE_ASSERT_TYPE(v, TYPE_INT);
  int64 *i = va_arg(*ap, int64 *);
  *i = VALUE_INT(v);
}

static void va_float_build(TValue *v, va_list *ap)
{
  v->type = TYPE_FLOAT;
  v->fval = va_arg(*ap, float64);
}

static void va_float_parse(TValue *v, va_list *ap)
{
  VALUE_ASSERT_TYPE(v, TYPE_FLOAT);
  float64 *f = va_arg(*ap, float64 *);
  *f = VALUE_FLOAT(v);
}

static void va_bool_build(TValue *v, va_list *ap)
{
  v->type = TYPE_BOOL;
  v->bval = va_arg(*ap, int);
}

static void va_bool_parse(TValue *v, va_list *ap)
{
  VALUE_ASSERT_TYPE(v, TYPE_BOOL);
  int *i = va_arg(*ap, int *);
  *i = VALUE_BOOL(v);
}

static void va_string_build(TValue *v, va_list *ap)
{
  v->type = TYPE_OBJECT;
  v->ob = String_New(va_arg(*ap, char *));
}

static void va_string_parse(TValue *v, va_list *ap)
{
  VALUE_ASSERT_TYPE(v, TYPE_OBJECT);
  Object *ob = VALUE_OBJECT(v);
  OB_ASSERT_KLASS(ob, String_Klass);
  char **str = va_arg(*ap, char **);
  *str = String_To_CString(ob);
}

static void va_object_build(TValue *v, va_list *ap)
{
  v->type = TYPE_OBJECT;
  v->ob = va_arg(*ap, Object *);
}

static void va_object_parse(TValue *v, va_list *ap)
{
  VALUE_ASSERT_TYPE(v, TYPE_OBJECT);
  Object **o = va_arg(*ap, Object **);
  *o = VALUE_OBJECT(v);
}

typedef void (*va_build_t)(TValue *v, va_list *ap);
typedef void (*va_parse_t)(TValue *v, va_list *ap);

typedef struct {
  char ch;
  va_build_t build;
  va_parse_t parse;
} va_convert_t;

static va_convert_t va_convert_func[] = {
  {'i', va_integer_build, va_integer_parse},
  {'l', va_long_build,    va_long_parse},
  {'f', va_float_build,   va_float_parse},
  {'d', va_float_build,   va_float_parse},
  {'z', va_bool_build,    va_bool_parse},
  {'s', va_string_build,  va_string_parse},
  {'O', va_object_build,  va_object_parse},
};

va_convert_t *get_convert(char ch)
{
  va_convert_t *convert;
  for (int i = 0; i < nr_elts(va_convert_func); i++) {
    convert = va_convert_func + i;
    if (convert->ch == ch) {
      return convert;
    }
  }
  debug_error("unsupported type: %d\n", ch);
  assert(0);
  return NULL;
}

int Va_Build_Value(TValue *ret, char ch, va_list *ap)
{
  va_convert_t *convert = get_convert(ch);
  convert->build(ret, ap);
  return 0;
}

int TValue_Build(TValue *ret, char ch, ...)
{
  va_list vp;
  va_start(vp, ch);
  int res = Va_Build_Value(ret, ch, &vp);
  va_end(vp);
  return res;
}

int Va_Parse_Value(TValue *val, char ch, va_list *ap)
{
  va_convert_t *convert = get_convert(ch);
  convert->parse(val, ap);
  return 0;
}

int TValue_Parse(TValue *val, char ch, ...)
{
  int res;
  va_list vp;
  va_start(vp, ch);
  res = Va_Parse_Value(val, ch, &vp);
  va_end(vp);
  return res;
}

/*-------------------------------------------------------------------------*/

Klass *Klass_New(char *name, int bsize, int isize, Klass *parent)
{
  Klass *klazz = malloc(sizeof(*klazz));
  memset(klazz, 0, sizeof(*klazz));
  init_object_head(klazz, parent);
  klazz->name  = name;
  klazz->bsize = bsize;
  klazz->isize = isize;
  //Object_Add_GCList((Object *)klazz);
  return klazz;
}

int Klass_Add_Field(Klass *klazz, char *name, char *desc, uint8 access)
{
  OB_ASSERT_KLASS(klazz, Klass_Klass);
  int name_index = StringItem_Set(klazz->itable, name);
  int desc_index = TypeItem_Set(klazz->itable, desc);
  Symbol *sym = Symbol_New(name_index, SYM_FIELD, access, desc_index);
  sym->value.index = klazz->avail_index++;
  return HashTable_Insert(klazz->stable, &sym->hnode);
}

int Klass_Add_Method(Klass *klazz, char *name, char *rdesc[], int rsz,
                     char *pdesc[], int psz, uint8 access, Object *method)
{
  OB_ASSERT_KLASS(klazz, Klass_Klass);
  int name_index = StringItem_Set(klazz->itable, name);
  int desc_index = ProtoItem_Set(klazz->itable, rdesc, rsz, pdesc, psz);
  Symbol *sym = Symbol_New(name_index, SYM_METHOD, access, desc_index);
  sym->value.obj = method;
  return HashTable_Insert(klazz->stable, &sym->hnode);
}

int Klass_Add_IMethod(Klass *klazz, char *name, char *rdesc[], int rsz,
                      char *pdesc[], int psz, uint8 access)
{
  OB_ASSERT_KLASS(klazz, Klass_Klass);
  int name_index = StringItem_Set(klazz->itable, name);
  int desc_index = ProtoItem_Set(klazz->itable, rdesc, rsz, pdesc, psz);
  Symbol *sym = Symbol_New(name_index, SYM_IMETHOD, access, desc_index);
  sym->value.index = klazz->avail_index++;
  return HashTable_Insert(klazz->stable, &sym->hnode);
}

Symbol *Klass_Get(Klass *klazz, char *name)
{
  OB_ASSERT_KLASS(klazz, Klass_Klass);
  int index = StringItem_Get(klazz->itable, name);
  if (index < 0) return NULL;
  Symbol sym = {.name_index = index};
  HashNode *hnode = HashTable_Find(klazz->stable, &sym);
  if (hnode != NULL) {
    return container_of(hnode, Symbol, hnode);
  } else {
    return NULL;
  }
}

Object *Klass_Get_Method(Klass *klazz, char *name)
{
  Symbol *s = Klass_Get(klazz, name);
  if (s == NULL) return NULL;
  if (s->kind != SYM_METHOD) return NULL;
  Object *ob = s->value.obj;
  OB_ASSERT_KLASS(ob, Method_Klass);
  return ob;
}

int Klass_Add_CFunctions(Klass *klazz, FunctionStruct *funcs)
{
  int res;
  FunctionStruct *f = funcs;
  Object *meth;
  while (f->name != NULL) {
    meth = CMethod_New(f->func);
    res = Klass_Add_Method(klazz, f->name, f->rdesc, f->rsz, f->pdesc, f->psz,
                           (uint8)f->access, meth);
    assert(res == 0);
    ++f;
  }
  return 0;
}



/*-------------------------------------------------------------------------*/

// static Object *__klass_get_method(Object *klazz, Object *args)
// {
//   assert(OB_KLASS(klazz) == &Klass_Klass);
//   char *str = NULL;
//   Tuple_Parse(args, "s", &str);
//   TValue v;
//   if (Klass_Get((Klass *)klazz, str, NULL, &v)) return NULL;
//   return Tuple_From_Va_TValues(1, &v);
// }

// static FunctionStruct klass_cmethods[] = {
//   {
//     "GetField",
//     "okoala/reflect.Field;",
//     "s",
//     ACCESS_PUBLIC,
//     NULL
//   },
//   {
//     "GetMethod",
//     "okoala/reflect.Method;",
//     "s",
//     ACCESS_PUBLIC,
//     __klass_get_method
//   },
//   {
//     "NewInstance",
//     NULL,
//     NULL,
//     ACCESS_PUBLIC,
//     NULL
//   },
//   {NULL, NULL, NULL, 0, NULL}
// };

void Init_Klass_Klass(void)
{
  //Klass_Add_CMethods(&Klass_Klass, klass_cmethods);
}

/*-------------------------------------------------------------------------*/

static void klass_mark(Object *ob)
{
  OB_ASSERT_KLASS(ob, Klass_Klass);
  //ob_incref(ob);
  //FIXME
}

static void klass_free(Object *ob)
{
  // Klass_Klass cannot be freed.
  if (ob == (Object *)&Klass_Klass) {
    debug_error("Klass_Klass cannot be freed\n");
    assert(0);
  }

  OB_ASSERT_KLASS(ob, Klass_Klass);
  free(ob);
}

Klass Klass_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass),
  .name  = "Class",
  .bsize = sizeof(Klass),

  .ob_mark = klass_mark,

  .ob_free = klass_free,
};
