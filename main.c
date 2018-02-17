
#include "koala.h"

int main(int argc, char *argv[])
{
  if (argc < 2) {
    printf("error: no input files\n");
    return -1;
  }

  Koala_Init();
  Object *ob = Koala_Load_Module(argv[1]);
  if (ob != NULL) {
    Object *code = Module_Get_Function(ob, "Main");
    if (code != NULL) {
      Run_Code(code, ob, NULL);
    } else {
      printf("error:There isn't 'Main' func in module '%s'", Module_Name(ob));
    }
  }
  Koala_Fini();

  puts("\nKoala Machine Exits");

  return 0;
}
