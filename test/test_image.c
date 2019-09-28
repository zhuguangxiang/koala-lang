/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
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
  TypeDesc *desc = desc_from_base(BASE_STR);
  TypeDesc *proto = desc_from_proto(NULL, desc);
  TYPE_DECREF(desc);
  uint8_t *psudo = kmalloc(4);
  psudo[0] = LOAD;
  psudo[1] = LOAD_CONST;
  psudo[2] = ADD;
  psudo[3] = RETURN_VALUE;
  image_add_func(image, "Foo", proto, psudo, 4, 0);
  TYPE_DECREF(proto);

  desc = desc_from_base(BASE_STR);
  Image_Add_Var(image, "Greeting", desc);
  TYPE_DECREF(desc);
/*
  desc = desc_from_base(BASE_INT);
  ConstValue val = {.kind = BASE_INT, .ival = 1000};
  Image_Add_Const(image, "MAX_LENGTH", desc, &val);
  TYPE_DECREF(desc);
*/
  ConstValue val2 = {.kind = BASE_STR, .str = "hello"};
  desc = desc_from_base(BASE_STR);
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
  //koala_initialize();
  test_image();
  //koala_finalize();
  return 0;
}
