/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "koala.h"

int main(int argc, char *argv[])
{
  Koala_Initialize();

  TypeObject Person_Type = {
    OBJECT_HEAD_INIT(&Type_Type)
    .name = "Person",
  };

  //FieldDef name = {"name", "s"};
  //Type_Add_FieldDef(&Person_Type, &name);

  Koala_Finalize();
  return 0;
}
