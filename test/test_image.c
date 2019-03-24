
#include "koala.h"
#include "opcode.h"

void test_image(void)
{
  KImage *image = KImage_New("test");
  TypeDesc *desc = TypeDesc_Get_Base(BASE_STRING);
  TypeDesc *proto = TypeDesc_Get_Proto(NULL, desc);
  uint8 *psudo = malloc(4);
  psudo[0] = LOAD;
  psudo[1] = LOAD_CONST;
  psudo[2] = ADD;
  psudo[3] = RETURN;
  KImage_Add_Func(image, "Foo", proto, psudo, 4);
  desc = TypeDesc_Get_Base(BASE_STRING);
  KImage_Add_Var(image, "Greeting", desc, 0);
  desc = TypeDesc_Get_Base(BASE_INT);
  KImage_Add_Var(image, "MAX_LENGTH", desc, 1);
  KImage_Finish(image);
  KImage_Show(image);
  KImage_Write_File(image, "foo.klc");
  KImage_Free(image);
  image = KImage_Read_File("foo.klc", 0);
  KImage_Show(image);
  KImage_Free(image);
}

int main(int argc, char *argv[])
{
  Koala_Initialize();
  test_image();
  Koala_Finalize();
  return 0;
}
