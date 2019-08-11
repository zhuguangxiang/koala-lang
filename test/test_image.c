/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "koala.h"
#include "opcode.h"
#include "image.h"

void test_image(void)
{
  Image *image = Image_New("test");
  TypeDesc *desc = typedesc_getbase(BASE_STR);
  TypeDesc *proto = typedesc_getproto(NULL, desc);
  TYPE_DECREF(desc);
  uint8_t *psudo = kmalloc(4);
  psudo[0] = LOAD;
  psudo[1] = LOAD_CONST;
  psudo[2] = ADD;
  psudo[3] = OP_RETURN;
  Image_Add_Func(image, "Foo", proto, psudo, 4, 0);
  TYPE_DECREF(proto);
  desc = typedesc_getbase(BASE_STR);
  Image_Add_Var(image, "Greeting", desc);
  TYPE_DECREF(desc);
  desc = typedesc_getbase(BASE_INT);
  ConstValue val = {.kind = BASE_INT, .ival = 1000};
  Image_Add_Const(image, "MAX_LENGTH", desc, &val);
  TYPE_DECREF(desc);
  Image_Finish(image);
  Image_Show(image);
  Image_Write_File(image, "foo.klc");
  Image_Free(image);
  image = Image_Read_File("foo.klc", 0);
  Image_Show(image);
  Image_Free(image);
}

int main(int argc, char *argv[])
{
  Koala_Initialize();
  test_image();
  //Koala_Finalize();
  return 0;
}