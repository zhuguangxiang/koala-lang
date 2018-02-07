
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
  DECL_STRUCTED_DESC(desc, 0, "koala/lang.String");
  KImage_Add_Var(image, "greeting", &desc);
  INIT_STRUCTED_DESC(&desc, 0, "koala/lang.Integer");
  KImage_Add_Var(image, "message", &desc);
  INIT_PRIMITIVE_DESC(&desc, 0, PRIMITIVE_FLOAT);
  KImage_Add_Var(image, "weight", &desc);
  // char *rdesc[] = {
  //   "Okoala/io.Socket;",
  // };

  // char *pdesc[] = {
  //   "Okoala/lang.String;",
  //   "Okoala/lang.Integer;"
  // };

  // KImage_Add_Func(image, "SayHello", 0, 3,
  //                   rdesc, nr_elts(rdesc), pdesc, nr_elts(pdesc),
  //                   NULL, 0);

  // KLCFile_Add_Func(filp, "SayHello22", 0, 3,
  //                  NULL, 0,
  //                  rdesc, nr_elts(rdesc),
  //                  pdesc, nr_elts(pdesc));
  KImage_Finish(image);

  KImage_Show(image);
  KImage_Write_File(image, "lang.klc");

  // int nr_var = Count_Vars(filp);
  // int nr_konsts = Count_Konsts(filp);
  // int nr_funcs = Count_Funcs(filp);
  // Object *ob = Module_New(filp->package, nr_var, nr_konsts);

  // Vector *strvec = &filp->items[ITYPE_STRING];
  // Vector *typevec = &filp->items[ITYPE_TYPE];
  // Vector *varvec = &filp->items[ITYPE_VAR];
  // Vector *funcvec = &filp->items[ITYPE_FUNC];
  // VarItem *varitem;
  // StringItem *name_stritem;
  // StringItem *desc_stritem;
  // TypeItem *typeitem;
  // int access = 0;
  // int konst = 0;
  // for (int i = 0; i < nr_var; i++) {
  //   varitem = Vector_Get(varvec, i);
  //   name_stritem = Vector_Get(strvec, varitem->name_index);
  //   typeitem = Vector_Get(typevec, varitem->type_index);
  //   desc_stritem = Vector_Get(strvec, typeitem->desc_index);
  //   access = (varitem->flags & FLAGS_ACCESS_PRIVATE) ? ACCESS_PRIVATE : ACCESS_PUBLIC;
  //   konst = varitem->flags & FLAGS_ACCESS_CONST;
  //   Module_Add_Variable(ob, name_stritem->data, desc_stritem->data, access, konst);
  // }

  // FuncItem *funcitem;
  // for (int i = 0; i < nr_funcs; i++) {
  //   funcitem = Vector_Get(funcvec, i);
  //   name_stritem = Vector_Get(strvec, varitem->name_index);
  //   typeitem = Vector_Get(typevec, varitem->type_index);
  //   desc_stritem = Vector_Get(strvec, typeitem->desc_index);
  //   access = (varitem->flags & FLAGS_ACCESS_PRIVATE) ? ACCESS_PRIVATE : ACCESS_PUBLIC;
  //   konst = varitem->flags & FLAGS_ACCESS_CONST;
  //   Module_Add_Variable(ob, name_stritem->data, desc_stritem->data, access, konst);
  // }

  //Module_Show(ob);
  //image = KImage_Read_File("lang.klc");
  //KImage_Show(image);
  return 0;
}
