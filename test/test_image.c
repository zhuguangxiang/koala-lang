
#include "image.h"
#include "atomstring.h"
#include "intobject.h"
#include "stringobject.h"
#include "opcode.h"

void test_image(void)
{
  KImage *image = KImage_New();
  Vector *ret = Vector_New();
  TypeDesc *desc = TypeDesc_Get_Basic(BASIC_STRING);
  TYPE_INCREF(desc);
  Vector_Append(ret, desc);
  desc = TypeDesc_Get_Basic(BASIC_INT);
  TYPE_INCREF(desc);
  Vector_Append(ret, desc);
  TypeDesc *proto = TypeDesc_Get_Proto(NULL, ret);
  uint8 psudo[4] = {OP_LOAD, OP_LOADK, OP_ADD, OP_RET};
  KImage_Add_Func(image, "Foo", proto, psudo, 4);
  desc = TypeDesc_Get_Basic(BASIC_STRING);
  KImage_Add_Var(image, "Greeting", desc, 0);
  desc = TypeDesc_Get_Basic(BASIC_INT);
  KImage_Add_Var(image, "MAX_LENGTH", desc, 1);
  KImage_Finish(image);
  KImage_Show(image);
  KImage_Write_File(image, "foo.klc");
  KImage_Free(image);
  image = KImage_Read_File("foo.klc");
  KImage_Show(image);
  KImage_Free(image);
}

int main(int argc, char *argv[])
{
  UNUSED_PARAMETER(argc);
  UNUSED_PARAMETER(argv);
  AtomString_Init();
  Init_TypeDesc();
  Init_String_Klass();
  Init_Integer_Klass();
  test_image();
  Fini_String_Klass();
  Fini_Integer_Klass();
  Fini_TypeDesc();
  AtomString_Fini();
  return 0;
}
