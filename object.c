
#include "codeobject.h"
#include "stringobject.h"
#include "moduleobject.h"
#include "tupleobject.h"
#include "listobject.h"
#include "koalastate.h"
#include "gc.h"
#include "hash.h"
#include "log.h"

TValue NilValue  = NIL_VALUE_INIT();
TValue TrueValue = BOOL_VALUE_INIT(1);
TValue FalseValue = BOOL_VALUE_INIT(0);

static void va_integer_build(TValue *v, va_list *ap)
{
  v->klazz = &Int_Klass;
  v->ival = (int64)va_arg(*ap, uint32);
}

static void va_integer_parse(TValue *v, va_list *ap)
{
  assert(v->klazz == &Int_Klass);
  int32 *i = va_arg(*ap, int32 *);
  *i = (int32)VALUE_INT(v);
}

static void va_long_build(TValue *v, va_list *ap)
{
  v->klazz = &Int_Klass;
  v->ival = va_arg(*ap, uint64);
}

static void va_long_parse(TValue *v, va_list *ap)
{
  assert(v->klazz == &Int_Klass);
  int64 *i = va_arg(*ap, int64 *);
  *i = VALUE_INT(v);
}

static void va_float_build(TValue *v, va_list *ap)
{
  v->klazz = &Float_Klass;
  v->fval = va_arg(*ap, float64);
}

static void va_float_parse(TValue *v, va_list *ap)
{
  assert(v->klazz == &Float_Klass);
  float64 *f = va_arg(*ap, float64 *);
  *f = VALUE_FLOAT(v);
}

static void va_bool_build(TValue *v, va_list *ap)
{
  v->klazz = &Bool_Klass;
  v->bval = va_arg(*ap, int);
}

static void va_bool_parse(TValue *v, va_list *ap)
{
  assert(v->klazz == &Bool_Klass);
  int *i = va_arg(*ap, int *);
  *i = VALUE_BOOL(v);
}

static void va_string_build(TValue *v, va_list *ap)
{
  v->klazz = &String_Klass;
  v->ob = String_New(va_arg(*ap, char *));
}

static void va_string_parse(TValue *v, va_list *ap)
{
  assert(v->klazz == &String_Klass);
  char **str = va_arg(*ap, char **);
  *str = String_RawString(v->ob);
}

static void va_object_build(TValue *v, va_list *ap)
{
  v->ob = va_arg(*ap, Object *);
  v->klazz = OB_KLASS(v->ob);
}

static void va_object_parse(TValue *v, va_list *ap)
{
  Object **o = va_arg(*ap, Object **);
  *o = v->ob;
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
  kassert(0, "unsupported type: %d\n", ch);
  return NULL;
}

TValue Va_Build_Value(char ch, va_list *ap)
{
  TValue val = NilValue;
  va_convert_t *convert = get_convert(ch);
  convert->build(&val, ap);
  return val;
}

TValue TValue_Build(int ch, ...)
{
  va_list vp;
  va_start(vp, ch);
  TValue val = Va_Build_Value(ch, &vp);
  va_end(vp);
  return val;
}

int Va_Parse_Value(TValue *val, char ch, va_list *ap)
{
  va_convert_t *convert = get_convert(ch);
  convert->parse(val, ap);
  return 0;
}

int TValue_Parse(TValue *val, int ch, ...)
{
  int res;
  va_list vp;
  va_start(vp, ch);
  res = Va_Parse_Value(val, ch, &vp);
  va_end(vp);
  return res;
}

/*---------------------------------------------------------------------------*/

static void object_mark(Object *ob)
{
  Check_Klass(OB_KLASS(ob));
  assert(OB_Head(ob) && OB_Head(ob) == ob);
  //FIXME: ob_ref_inc
}

static int get_object_size(Klass *klazz)
{
  if (klazz == &Any_Klass) return 0;

  int size = 0;

  debug(">>>>fields of '%s':%d", klazz->name, klazz->itemsize);
  size += OBJECT_SIZE(klazz);

  Klass *trait;
  Vector_ForEach_Reverse(trait, &klazz->traits) {
    debug(">>>>fields of '%s':%d", trait->name, trait->itemsize);
    size += OBJECT_SIZE(trait);
  }

  if (OB_HasBase(klazz)) {
    debug(">>>>base");
    size += get_object_size((Klass *)OB_Base(klazz));
  }

  return size;
}

static void init_object(Object *ob, Klass *klazz)
{
  Object *base = ob;
  Init_Object_Head(base, klazz);
  base->ob_base = NEXT_OBJECT(base, klazz);
  OB_Head(base) = ob;
  //init_fields();

  Klass *trait;
  Vector_ForEach_Reverse(trait, &klazz->traits) {
    base = OB_Base(base);
    Init_Object_Head(base, trait);
    base->ob_base = NEXT_OBJECT(base, trait);
    OB_Head(base) = ob;
    //init_fields();
  }

  if (OB_HasBase(klazz)) {
    init_object(OB_Base(base), (Klass *)OB_Base(klazz));
  } else {
    base->ob_base = base;
  }
}

static Object *object_alloc(Klass *klazz)
{
  Object *ob = GC_Alloc(get_object_size(klazz));
  init_object(ob, klazz);
  printf("++++++++line-order++++++++\n");
  Object *base = ob;
  printf("%s", base->ob_klass->name);
  while (OB_HasBase(base)) {
    base = OB_Base(base);
    printf(" -> %s", base->ob_klass->name);
  }
  printf("\n++++++++++++++++++++++++++\n");
  return ob;
}

#if 0
static int get_object_size(Klass *klazz)
{
  if (klazz == &Any_Klass) return 0;
  int size = klazz->stbl.varcnt;
  debug(">>>>fields of '%s':%d", klazz->name, klazz->itemsize);
  Klass *line;
  Vector_ForEach_Reverse(line, &klazz->lines) {
    debug(">>>>fields of '%s':%d", line->name, line->itemsize);
    size += line->stbl.varcnt;
  }

  debug(">>>>all fields:%d", size);
  return size;
}

static Object *object_alloc(Klass *klazz)
{
  int nr = get_object_size(klazz);
  int size = sizeof(Object) + sizeof(TValue) * nr;
  Object *ob = calloc(1, size);
  Init_Object_Head(ob, klazz);
  ob->ob_size = nr;
  return ob;
}
#endif

static void object_init(Object *ob)
{
  Klass *klazz = OB_KLASS(ob);
  TValue *values = (TValue *)(ob + 1);
  for (int i = 0; i < klazz->itemsize; i++) {
    initnilvalue(values + i);
  }
}

static void object_free(Object *ob)
{
  Check_Klass(OB_KLASS(ob));
  assert(OB_Head(ob) && OB_Head(ob) == ob);
}

static uint32 object_hash(TValue *v)
{
  Object *ob = v->ob;
  Check_Klass(OB_KLASS(ob));
  assert(OB_Head(ob) && OB_Head(ob) == ob);
  return hash_uint32(ptr2int(OB_Head(ob), uint32), 32);
}

static int object_equal(TValue *v1, TValue *v2)
{
  Object *ob1 = v1->ob;
  Check_Klass(OB_KLASS(ob1));
  Object *ob2 = v2->ob;
  Check_Klass(OB_KLASS(ob2));
  return OB_Head(ob1) == OB_Head(ob2);
}

static Object *object_tostring(TValue *v)
{
  Object *ob = v->ob;
  assert(OB_Head(ob) && OB_Head(ob) == ob);
  Klass *klazz = OB_KLASS(OB_Head(ob));
  char buf[128];
  snprintf(buf, 128, "%s@%x", klazz->name, ptr2int(OB_Head(ob), uint32));
  return Tuple_Build("O", String_New(buf));
}

/*---------------------------------------------------------------------------*/

void LineBaseKlass(Klass *klazz, Klass *base)
{
  if (!base) return;
  Klass *line;
  Vector_ForEach(line, &base->lines) {
    Vector_Append(&klazz->lines, line);
  }
  Vector_Append(&klazz->lines, base);
}

Klass *Klass_New(char *name, Klass *base, Vector *traits, Klass *type)
{
  Klass *klazz = calloc(1, sizeof(Klass));
  Init_Object_Head(klazz, type);
  if (base) OB_Base(klazz) = (Object *)base;
  klazz->name = strdup(name);
  klazz->basesize = sizeof(Object);
  klazz->itemsize = 1;
  klazz->ob_mark  = object_mark;
  klazz->ob_alloc = object_alloc;
  klazz->ob_init  = object_init;
  klazz->ob_free  = object_free,
  klazz->ob_hash  = object_hash;
  klazz->ob_equal = object_equal;
  klazz->ob_tostr = object_tostring;
  klazz->varcnt = 1;
  Vector_Init(&klazz->traits);
  Vector_Concat(&klazz->traits, traits);

  Vector_Init(&klazz->lines);
  Vector_Append(&klazz->lines, &Any_Klass);
  LineBaseKlass(klazz, base);
  Vector_Concat(&klazz->lines, traits);

  puts("++++++++++++++++line-order in loading++++++++++++++++");
  printf("%s", klazz->name);
  Klass *line;
  Vector_ForEach_Reverse(line, &klazz->lines) {
    printf(" -> %s", line->name);
  }
  puts("\n+++++++++++++++++++++++++++++++++++++++++++++++++++++");
  return klazz;
}

void Fini_Klass(Klass *klazz)
{
  UNUSED_PARAMETER(klazz);
  // STable_Fini(&klazz->stbl);
}

void Check_Klass(Klass *klazz)
{
  while (klazz) {
    assert(OB_KLASS(klazz) == &Klass_Klass || OB_KLASS(klazz) == &Trait_Klass);
    klazz = OB_HasBase(klazz) ? (Klass *)OB_Base(klazz) : NULL;
  }
}

static HashTable *__get_table(Klass *klazz)
{
  if (!klazz->table) {
    HashInfo hashinfo = {
      .hash = (ht_hashfunc)Member_Hash,
      .equal = (ht_equalfunc)Member_Equal
    };
    klazz->table = HashTable_New(&hashinfo);
  }
  return klazz->table;
}

int Klass_Add_Field(Klass *klazz, char *name, TypeDesc *desc)
{
  Check_Klass(klazz);
  MemberDef *member = Member_Var_New(name, desc, 0);
  int res = HashTable_Insert(__get_table(klazz), &member->hnode);
  if (!res) {
    member->offset = klazz->varcnt++;
    klazz->itemsize++;
    return 0;
  } else {
    Member_Free(member);
    return -1;
  }
}

int Klass_Add_Method(Klass *klazz, char *name, Object *code)
{
  Check_Klass(klazz);
  CodeObject *co = OBJ_TO_CODE(code);
  MemberDef *member = Member_Code_New(name, co->proto);
  int res = HashTable_Insert(__get_table(klazz), &member->hnode);
  if (!res) {
    member->code = code;
    if (CODE_ISKFUNC(code)) {
      co->kf.consts = klazz->consts;
    }
    return 0;
  } else {
    Member_Free(member);
    return -1;
  }
}

int Klass_Add_Proto(Klass *klazz, char *name, TypeDesc *proto)
{
  Check_Klass(klazz);
  MemberDef *member = Member_Proto_New(name, proto);
  int res = HashTable_Insert(__get_table(klazz), &member->hnode);
  if (!res) {
    return 0;
  } else {
    Member_Free(member);
    return -1;
  }
}

static MemberDef *Klass_Get_Member(Klass *klazz, char *name)
{
  MemberDef key = {.name = name};
  MemberDef *member = HashTable_Find(__get_table(klazz), &key);
  if (member) return member;

  Klass *k;
  Vector_ForEach_Reverse(k, &klazz->traits) {
    member = Klass_Get_Member(k, name);
    if (member) return member;
  }

  return NULL;
}

Object *Klass_Get_Method(Klass *klazz, char *name, Klass **trait)
{
  MemberDef key = {.name = name};
  MemberDef *member = HashTable_Find(__get_table(klazz), &key);
  if (member) {
    if (member->kind != MEMBER_CODE) return NULL;
    if (trait) *trait = klazz;
    return member->code;
  }

  Object *ob;
  Klass *k;
  Vector_ForEach_Reverse(k, &klazz->traits) {
    ob = Klass_Get_Method(k, name, trait);
    if (ob) return ob;
  }

  return NULL;
}

int Klass_Add_CFunctions(Klass *klazz, FuncDef *funcs)
{
  int res;
  FuncDef *f = funcs;
  Object *meth;
  Vector *pdesc;
  Vector *rdesc;
  TypeDesc *proto;

  while (f->name) {
    rdesc = CString_To_TypeList(f->rdesc);
    pdesc = CString_To_TypeList(f->pdesc);
    proto = Type_New_Proto(rdesc, pdesc);
    meth = CFunc_New(f->fn, proto);
    res = Klass_Add_Method(klazz, f->name, meth);
    assert(res == 0);
    ++f;
  }
  return 0;
}

/*---------------------------------------------------------------------------*/

static void klass_mark(Object *ob)
{
  OB_ASSERT_KLASS(ob, Klass_Klass);
  //FIXME: ob_ref_inc
}

static void klass_free(Object *ob)
{
  OB_ASSERT_KLASS(ob, Klass_Klass);
  kassert(ob != (Object *)&Klass_Klass, "Why goes here?");
  free(ob);
}

Klass Klass_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass, &Klass_Klass)
  .name = "Klass",
  .basesize = sizeof(Klass),
  .ob_mark = klass_mark,
  .ob_free = klass_free,
};

Klass Any_Klass = {
  OBJECT_HEAD_INIT(&Any_Klass, &Klass_Klass)
  .name = "Any",
};

Klass Trait_Klass = {
  OBJECT_HEAD_INIT(&Trait_Klass, &Klass_Klass)
  .name = "Trait",
};

/*---------------------------------------------------------------------------*/

static MemberDef *get_field(Object *ob, char *name, Object **rob)
{
  Check_Klass(OB_KLASS(ob));
  Object *base;
  MemberDef *member = NULL;
  char *dot = strchr(name, '.');
  if (dot) {
    char *classname = strndup(name, dot - name);
    char *fieldname = strndup(dot + 1, strlen(name) - (dot - name) - 1);
    base = ob;
    while (OB_HasBase(base)) {
      if (!strcmp(OB_KLASS(base)->name, classname)) break;
      base = OB_Base(base);
    }
    member = Klass_Get_Member(OB_KLASS(base), fieldname);
    *rob = base;
    free(classname);
    free(fieldname);
  } else {
    base = ob;
    while (base) {
      member = Klass_Get_Member(OB_KLASS(base), name);
      if (member) {
        *rob = base;
        break;
      }
      base = OB_HasBase(base) ? OB_Base(base) : NULL;
    }
  }

  assert(member);
  assert(member->kind == MEMBER_VAR);
  return member;
}

TValue Object_Get_Value(Object *ob, char *name)
{
  Object *rob = NULL;
  MemberDef *member = get_field(ob, name, &rob);
  assert(rob);
  TValue *value = (TValue *)(rob + 1);
  int index = member->offset;
  debug("getvalue:%d",index);
  assert(index >= 0 && index < rob->ob_size);
  return value[index];
}

int Object_Set_Value(Object *ob, char *name, TValue *val)
{
  Object *rob = NULL;
  MemberDef *member = get_field(ob, name, &rob);
  assert(rob);
  TValue *value = (TValue *)(rob + 1);
  int index = member->offset;
  assert(index >= 0 && index < rob->ob_size);
  value[index] = *val;
  return 0;
}

Object *Object_Get_Method(Object *ob, char *name, Object **rob)
{
  Check_Klass(OB_KLASS(ob));
  Object *base;
  Object *code;
  char *dot = strchr(name, '.');
  if (dot) {
    char *classname = strndup(name, dot - name);
    char *funcname = strndup(dot + 1, strlen(name) - (dot - name) - 1);
    if (!strcmp(classname, "super")) {
      base = ob;
      assert(OB_HasBase(base));
      base = OB_Base(base);
      while (OB_HasBase(base)) {
        code = Klass_Get_Method(OB_KLASS(base), funcname, NULL);
        if (code) {
          debug("find method '%s' in '%s'", name, OB_KLASS(base)->name);
          break;
        }
        base = OB_Base(base);
      }
    } else {
      base = ob;
      assert(base);
      while (OB_HasBase(base)) {
        if (!strcmp(OB_KLASS(base)->name, classname)) {
          debug("find method '%s' in '%s'", name, OB_KLASS(base)->name);
          break;
        }
        base = OB_Base(base);
      }
      code = Klass_Get_Method(OB_KLASS(base), funcname, NULL);
    }
    assert(code);
    *rob = base;
    free(classname);
    free(funcname);
    return code;
  } else {
    if (!strcmp(name, "__init__")) {
      //__init__ method is allowed to be searched only in current class
      code = Klass_Get_Method(OB_KLASS(ob), name, NULL);
      //FIXME: class with no __init__ ?
      //assert(code);
      *rob = ob;
      return code;
    } else {
      Klass *trait = NULL;
      base = ob;
      while (base) {
        code = Klass_Get_Method(OB_KLASS(base), name, &trait);
        if (code) {
          Object *next = base;
          while (next) {
            if (OB_KLASS(next) == trait) break;
            next = OB_HasBase(next) ? OB_Base(next) : NULL;
          }
          *rob = next;
          return code;
        }
        base = OB_HasBase(base) ? OB_Base(base) : NULL;
      }

      Object *head = OB_Head(ob);
      while (head && head != ob) {
        code = Klass_Get_Method(OB_KLASS(head), name, &trait);
        if (code) {
          Object *next = head;
          while (next) {
            if (OB_KLASS(next) == trait) break;
            next = OB_HasBase(next) ? OB_Base(next) : NULL;
          }
          *rob = next;
          return code;
        }
        head = OB_HasBase(head) ? OB_Base(head) : NULL;
      }

      kassert(0, "cannot find func '%s'", name);
      return NULL;
    }
  }
}

/*---------------------------------------------------------------------------*/

static int object_print(char *buf, int sz, TValue *val)
{
  Object *ob = val->ob;
  Klass *klazz = OB_KLASS(ob);
  MemberDef key = {.name = "ToString"};
  MemberDef *member = HashTable_Find(__get_table(klazz), &key);
  if (member) {
    if (member->kind != MEMBER_CODE) return 0;
    ob = Koala_Run_Code(member->code, ob, NULL);
  } else {
    ob = klazz->ob_tostr(val);
  }
  OB_ASSERT_KLASS(ob, Tuple_Klass);
  TValue v = Tuple_Get(ob, 0);
  return snprintf(buf, sz, "%s", String_RawString(v.ob));
}

#if 0
static char escchar(char ch)
{
  static struct escmap {
    char esc;
    char ch;
  } escmaps[] = {
    {'a', 7},
    {'b', 8},
    {'f', 12},
    {'n', 10},
    {'r', 13},
    {'t', 9},
    {'v', 11}
  };

  for (int i = 0; i < nr_elts(escmaps); i++) {
    if (escmaps[i].esc == ch) return escmaps[i].ch;
  }

  return 0;
}

static char *string_escape(char *escstr, int len)
{
  char *str = calloc(1, len + 1);
  char ch, escch;
  int i = 0;
  while ((ch = *escstr) && (len > 0)) {
    if (ch != '\\') {
      ++escstr; len--;
      str[i++] = ch;
    } else {
      ++escstr; len--;
      ch = *escstr;
      escch = escchar(ch);
      if (escch > 0) {
        ++escstr; len--;
        str[i] = escch;
      } else {
        str[i] = '\\';
      }
    }
  }
  return str;
}

static int CStrValue_Print(char *buf, int sz, TValue *val, int escape)
{
  char *str = String_RawString(VALUE_OBJECT(val));
  if (!escape) return snprintf(buf, sz, "%s", str);
  // handle esc characters
  char *escstr = string_escape(str, strlen(str));
  int cnt = snprintf(buf, sz, "%s", escstr);
  free(escstr);
  return cnt;
}
#endif

int TValue_Print(char *buf, int sz, TValue *val, int escape)
{
  UNUSED_PARAMETER(escape);

  if (!val) {
    buf[0] = '\0';
    return 0;
  }

  int count;
  if (!val->klazz) {
    count = snprintf(buf, sz, "(nil)");
  } else if (val->klazz == &Int_Klass) {
    count = snprintf(buf, sz, "%lld", VALUE_INT(val));
  } else if (val->klazz == &Float_Klass) {
    count = snprintf(buf, sz, "%lf", VALUE_FLOAT(val));
  } else if (val->klazz == &Bool_Klass) {
    count = snprintf(buf, sz, "%s", VALUE_BOOL(val) ? "true" : "false");
  } else {
    count = object_print(buf, sz, val);
  }
  return count;
}

#if 0
static int check_class_inheritance(Klass *k1, Klass *k2)
{
  Klass *base = k1;
  while (base) {
    if (base == k2) return 0;
    base = OB_HasBase(base) ? (Klass *)OB_Base(base) : NULL;
  }

  base = k2;
  while (base) {
    if (base == k1) return 0;
    base = OB_HasBase(base) ? (Klass *)OB_Base(base) : NULL;
  }

  return -1;
}

struct intf_inherit_struct {
  Klass *klz;
  int result;
};

static void __intf_func_check_fn(Symbol *sym, void *arg)
{
  struct intf_inherit_struct *temp = arg;
  if (temp->result) return;

  Klass *klazz = temp->klz;
  assert(sym->kind == SYM_IPROTO);
  assert(!(klazz->flags & FLAGS_INTF));

  Symbol *method;
  Klass *base = klazz;
  while (base) {
    method = Klass_Get_Symbol(base, sym->name);
    if (!method) {
      if (!OB_HasBase(base)) {
        error("Is '%s' in '%s'? no", sym->name, base->name);
        temp->result = -1;
        break;
      } else {
        debug("go super class to check '%s' is in '%s'",
          sym->name, base->name);
      }
    } else {
      debug("Is '%s' in '%s'? yes", sym->name, base->name);
      if (!Proto_IsEqual(method->desc->proto, sym->desc->proto)) {
        error("intf-func check failed");
        temp->result = -1;
      } else {
        debug("intf-func check ok");
        temp->result = 0;
      }
      break;
    }
    base = OB_HasBase(base) ? (Klass *)OB_Base(base) : NULL;
  }
}

static int check_interface_inheritance(Klass *intf, Klass *klz)
{
  struct intf_inherit_struct temp = {klz, 0};
  STable_Traverse(&intf->stbl, __intf_func_check_fn, &temp);
  return temp.result;
}
#endif

int TValue_Check(TValue *v1, TValue *v2)
{
  //FIXME
  assert(v1->klazz);
  assert(v2->klazz);
  return 0;

  // if (v1->klazz != v2->klazz) {
  //    error("v1's type %s vs v2's type %s", v1->klazz->name, v2->klazz->name);
  //    return -1;
  // }

  // if (v1->type == TYPE_OBJECT) {
  //    if (v1->klazz == NULL || v2->klazz == NULL) {
  //      error("object klazz is not set");
  //      return -1;
  //    }
  //    if (v1->klazz != v2->klazz) {
  //      // if (!(v1->klazz->flags & FLAGS_INTF) &&
  //      //    !(v2->klazz->flags & FLAGS_INTF)) {
  //      //    return check_class_inheritance(v1->klazz, v2->klazz);
  //      // } else if ((v1->klazz->flags & FLAGS_INTF) &&
  //      //    !(v2->klazz->flags & FLAGS_INTF)) {
  //      //    return check_interface_inheritance(v1->klazz, v2->klazz);
  //      // } else if ((v1->klazz->flags & FLAGS_INTF) &&
  //      //    (v2->klazz->flags & FLAGS_INTF)) {
  //      //    return check_interface_inheritance(v1->klazz, OB_KLASS(v2->ob));
  //      // } else {
  //      //    kassert(0, "class '%s' <- intf '%s' is not allowed,
  //      //      use typeof() to down convertion", v1->klazz->name, v2->klazz->name);
  //      //    return -1;
  //      // }
  //    }
  // }
  return 0;
}

int TValue_Check_TypeDesc(TValue *val, TypeDesc *desc)
{
  switch (desc->kind) {
    case TYPE_PRIMITIVE: {
      if (Type_IsInt(desc) && VALUE_ISINT(val))
        return 0;
      if (Type_IsFloat(desc) && VALUE_ISFLOAT(val))
        return 0;
      if (Type_IsBool(desc) && VALUE_ISBOOL(val))
        return 0;
      if (Type_IsString(desc)) {
        Object *ob = val->ob;
        if (OB_CHECK_KLASS(ob, String_Klass)) return 0;
      }
      break;
    }
    case TYPE_USRDEF: {
      Object *ob = OB_KLASS(val->ob)->module;
      Klass *klazz = Koala_Get_Klass(ob, desc->usrdef.path, desc->usrdef.type);
      if (!klazz) return -1;
      Klass *k = OB_KLASS(val->ob);
      while (k) {
        if (k == klazz) return 0;
        k = OB_HasBase(k) ? (Klass *)OB_Base(k) : NULL;
      }
      break;
    }
    case TYPE_PROTO: {
      break;
    }
    default: {
      kassert(0, "unknown type's kind %d\n", desc->kind);
      break;
    }
  }
  return -1;
}

static void init_primitive(TValue *val, int primitive)
{
  switch (primitive) {
    case PRIMITIVE_INT: {
      setivalue(val, 0);
      break;
    }
    case PRIMITIVE_FLOAT: {
      setfltvalue(val, 0.0);
      break;
    }
    case PRIMITIVE_BOOL: {
      setbvalue(val, 0);
      break;
    }
    case PRIMITIVE_STRING: {
      setobjvalue(val, String_New(""));
      break;
    }
    case PRIMITIVE_ANY:
    //fallthrough
    default: {
      kassert(0, "unknown primitive %c", primitive);
      break;
    }
  }
}

void TValue_Set_TypeDesc(TValue *val, TypeDesc *desc, Object *ob)
{
  assert(ob);
  assert(desc);

  switch (desc->kind) {
    case TYPE_PRIMITIVE: {
      init_primitive(val, desc->primitive);
      break;
    }
    case TYPE_USRDEF: {
      Object *module = NULL;
      if (desc->usrdef.path) {
        module = Koala_Load_Module(desc->usrdef.path);
      } else {
        if (OB_CHECK_KLASS(ob, Module_Klass)) {
          module = ob;
        } else if (OB_CHECK_KLASS(OB_KLASS(ob), Klass_Klass)) {
          module = OB_KLASS(ob)->module;
        } else {
          assert(0);
        }
      }

      Klass *klazz = Module_Get_ClassOrTrait(module, desc->usrdef.type);
      assert(klazz);
      debug("find class or interface: %s", klazz->name);
      setobjtype(val, klazz);
      break;
    }
    case TYPE_PROTO: {
      setobjtype(val, &Code_Klass);
      break;
    }
    case TYPE_ARRAY: {
      setobjtype(val, &List_Klass);
      break;
    }
    default: {
      assert(0);
      break;
    }
  }
}

void TValue_Set_Value(TValue *val, TValue *v)
{
  if (VALUE_ISNIL(val)) {
    debug("TValue is nil");
    *val = *v;
    return;
  }

  if (v->klazz == &Int_Klass) {
    VALUE_ASSERT_INT(val);
    val->ival = v->ival;
  } else if (v->klazz == &Float_Klass) {
    VALUE_ASSERT_FLOAT(val);
    val->fval = v->fval;
  } else if (v->klazz == &Bool_Klass) {
    VALUE_ASSERT_BOOL(val);
    val->bval = v->bval;
  } else {
    //FIXME:
    //assert(v->klazz == val->klazz);
    val->ob = v->ob;
  }
}

MemberDef *Member_New(int kind, char *name, TypeDesc *desc, int konst)
{
  MemberDef *member = calloc(1, sizeof(MemberDef));
  assert(member);
  Init_HashNode(&member->hnode, member);
  member->kind = kind;
  member->name = name;
  member->desc = desc;
  member->konst = konst;
  return member;
}

void Member_Free(MemberDef *m)
{
  //FIXME
  free(m);
}

uint32 Member_Hash(MemberDef *m)
{
  return hash_string(m->name);
}

int Member_Equal(MemberDef *m1, MemberDef *m2)
{
  return !strcmp(m1->name, m2->name);
}
