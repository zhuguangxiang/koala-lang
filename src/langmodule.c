/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "moduleobject.h"
#include "intobject.h"
#include "stringobject.h"
#include "arrayobject.h"
#include "tupleobject.h"
#include "mapobject.h"
#include "fieldobject.h"
#include "methodobject.h"
#include "codeobject.h"
#include "classobject.h"

static Object *load_module(Object *self, Object *args)
{

}

static Object *new_instance(Object *self, Object *args)
{

}

static Object *is_module(Object *self, Object *args)
{

}

static MethodDef lang_methods[] = {
  {"loadmodule", "s",    "Llang.Module;", load_module },
  {"new",        "s...", "A",             new_instance},
  {"ismodule",   "A",    "z",             is_module   },
  {"isclass",    "A",    "z",             is_module   },
  {"ismethod",   "A",    "z",             is_module   },
  {"isfield",    "A",    "z",             is_module   },
  {NULL}
};

void init_lang_module(void)
{
  Object *m = Module_New("lang");

  Module_Add_Type(m, &Any_Type);
  Module_Add_Type(m, &Integer_Type);
  Module_Add_Type(m, &String_Type);
  //Module_Add_Type(m, &Float_Type);
  Module_Add_Type(m, &Array_Type);
  Module_Add_Type(m, &Tuple_Type);
  Module_Add_Type(m, &Map_Type);

  Module_Add_Type(m, &Field_Type);
  Module_Add_Type(m, &Method_Type);
  Module_Add_Type(m, &Proto_Type);
  Module_Add_Type(m, &Code_Type);
  Module_Add_Type(m, &Class_Type);
  Module_Add_Type(m, &Module_Type);

  Module_Add_FuncDefs(m, lang_methods);

  Module_Install("lang", m);
}
