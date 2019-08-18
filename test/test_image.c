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
  int val = 1234;
  Image_Add_Integer(image, val);
#if 0
  TypeDesc *desc = desc_getbase(BASE_STR);
  TypeDesc *proto = desc_getproto(NULL, desc);
  TYPE_DECREF(desc);
  uint8_t *psudo = kmalloc(4);
  psudo[0] = LOAD;
  psudo[1] = LOAD_CONST;
  psudo[2] = ADD;
  psudo[3] = RETURN_VALUE;
  Image_Add_Func(image, "Foo", proto, psudo, 4, 0);
  TYPE_DECREF(proto);

  desc = desc_getbase(BASE_STR);
  Image_Add_Var(image, "Greeting", desc);
  TYPE_DECREF(desc);
/*
  desc = desc_getbase(BASE_INT);
  ConstValue val = {.kind = BASE_INT, .ival = 1000};
  Image_Add_Const(image, "MAX_LENGTH", desc, &val);
  TYPE_DECREF(desc);
*/
  ConstValue val2 = {.kind = BASE_STR, .str = "hello"};
  desc = desc_getbase(BASE_STR);
  Image_Add_Const(image, "hello", desc, &val2);
  TYPE_DECREF(desc);

  Image_Finish(image);
  Image_Show(image);
  Image_Write_File(image, "foo.klc");
  Image_Free(image);
  image = Image_Read_File("foo.klc", 0);
  Image_Show(image);
  Image_Free(image);
#endif
  Image_Free(image);
}

int main(int argc, char *argv[])
{
  //Koala_Initialize();
  test_image();
  //Koala_Finalize();
  return 0;
}