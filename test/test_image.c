
#include "image.h"
#include "atomstring.h"
#include "intobject.h"
#include "stringobject.h"
#include "opcode.h"

void test_image(void)
{
  KImage *image = KImage_New("foo");
  Vector *ret = Vector_New();
  Vector_Append(ret, &String_Type);
  Vector_Append(ret, &Int_Type);
  TypeDesc *proto = TypeDesc_New_Proto(NULL, ret);
  uint8 psudo[4] = {OP_LOAD, OP_LOADK, OP_ADD, OP_RET};
  KImage_Add_Func(image, "Foo", proto, psudo, 4);
  KImage_Add_Var(image, "Greeting", &String_Type, 0);
  KImage_Add_Var(image, "MAX_LENGTH", &Int_Type, 1);
  KImage_Finish(image);
  KImage_Show(image);
  KImage_Write_File(image, "foo.klc");
  image = KImage_Read_File("foo.klc");
  KImage_Show(image);
}

int main(int argc, char *argv[])
{
  UNUSED_PARAMETER(argc);
  UNUSED_PARAMETER(argv);
  AtomString_Init();
  Init_String_Klass();
  Init_Integer_Klass();
  test_image();
  Fini_String_Klass();
  Fini_Integer_Klass();
  AtomString_Fini();
  return 0;
}
