
#include "codeformat.h"

/*
 make lib
 gcc -g -std=c99 test_kcodeformat.c -lkoala -L.
 */
int main(int argc, char *argv[])
{
  UNUSED_PARAMETER(argc);
  UNUSED_PARAMETER(argv);

  KImage *image = KImage_New("lang");
  TypeDesc desc;
  INIT_USERDEF_DESC(&desc, 0, "koala/lang.String");
  KImage_Add_Var(image, "greeting", &desc);
  INIT_USERDEF_DESC(&desc, 0, "koala/lang.Integer");
  KImage_Add_Var(image, "message", &desc);
  INIT_PRIMITIVE_DESC(&desc, 0, PRIMITIVE_FLOAT);
  KImage_Add_Var(image, "weight", &desc);
  KImage_Finish(image);

  KImage_Show(image);
  KImage_Write_File(image, "lang.klc");
  return 0;
}
