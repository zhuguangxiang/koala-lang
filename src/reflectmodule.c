/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */


#include "moduleobject.h"

static MethodDef reflect_methods = {
  {"Import", "s", "Llang.Module;", load_module},
  {"New", "s...", "A", new_instance},
  {NULL}
};

void init_reflect_module(void)
{
  Object *m = Module_New("reflect");
  Module_Add_Type(m, &Field_Type);
  Module_Add_Type(m, &Method_Type);
  Module_Add_Type(m, &Proto_Type);
  Module_Add_Type(m, &Code_Type);
  Module_Add_Type(m, &Class_Type);
  Module_Add_Type(m, &Module_Type);
  Module_Install("reflect", m);
}
