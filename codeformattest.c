
#include "codeformat.h"

int main(int argc, char *argv[])
{
  KLCFile *filp = KLCFile_New("lang");
  KLCFile_Add_Var(filp, "greeting", FLAGS_ACCESS_PRIVATE, "koala/lang.String");
  //KLCFile_Add_Var(filp, "message", FLAGS_ACCESS_PRIVATE, "koala/lang.Integer");
  char *rdesc[] = {
    "koala/io.Socket",
  };

  char *pdesc[] = {
    "koala/lang.String",
    "koala/lang.Integer"
  };

  KLCFile_Add_Func(filp, "SayHello", 0, 3,
                   NULL, 0,
                   rdesc, nr_elts(rdesc),
                   pdesc, nr_elts(pdesc));

  // KLCFile_Add_Func(filp, "SayHello22", 0, 3,
  //                  NULL, 0,
  //                  rdesc, nr_elts(rdesc),
  //                  pdesc, nr_elts(pdesc));
  KLCFile_Finish(filp);
  KCLFile_Display(filp);
  KLCFile_Write_File(filp, "lang.klc");
  filp = KLCFile_Read_File("lang.klc");
  KCLFile_Display(filp);
  return 0;
}
