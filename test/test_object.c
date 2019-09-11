/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "koala.h"

int main(int argc, char *argv[])
{
  koala_initialize();

  TypeObject Person_Type = {
    OBJECT_HEAD_INIT(&type_type)
    .name = "Person",
  };

  //FieldDef name = {"name", "s"};
  //Type_Add_FieldDef(&Person_Type, &name);

  koala_finalize();
  return 0;
}
