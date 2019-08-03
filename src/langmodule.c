/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */


#include "moduleobject.h"
#include "intobject.h"
#include "stringobject.h"
#include "tupleobject.h"
#include "mapobject.h"

void init_lang_module(void)
{
  Object *m = Module_New("lang");
  Module_Add_Type(m, &Any_Type);
  Module_Add_Type(m, &Integer_Type);
  Module_Add_Type(m, &String_Type);
  //Module_Add_Type(m, &Float_Type);
  //Module_Add_Type(m, &Array_Type);
  Module_Add_Type(m, &Tuple_Type);
  Module_Add_Type(m, &Map_Type);
  Module_Install("lang", m);
}
