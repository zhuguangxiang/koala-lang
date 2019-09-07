/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "koala.h"

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
  Module_Add_Type(m, &Byte_Type);
  Module_Add_Type(m, &Integer_Type);
  Module_Add_Type(m, &Bool_Type);
  Module_Add_Type(m, &String_Type);
  Module_Add_Type(m, &Char_Type);
  Module_Add_Type(m, &Float_Type);
  Module_Add_Type(m, &array_type);
  Module_Add_Type(m, &tuple_type);
  Module_Add_Type(m, &Dict_Type);
  Module_Add_Type(m, &Field_Type);
  Module_Add_Type(m, &method_type);
  Module_Add_Type(m, &proto_type);
  Module_Add_Type(m, &class_type);
  Module_Add_Type(m, &Module_Type);
  Module_Add_Type(m, &Code_Type);
  //Module_Add_FuncDefs(m, lang_methods);
  Module_Install("lang", m);
  OB_DECREF(m);
}

void fini_lang_module(void)
{
  Module_Uninstall("lang");
}