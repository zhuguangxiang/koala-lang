
#include "koala.h"
#include "symbol.h"

GState gs;

// koala/lang.klc
static void init_lang_module(void)
{
  Object *ob = Module_New("lang", 0);

  Init_Klass_Klass();
  Init_String_Klass();
  Init_Tuple_Klass();
  Init_Table_Klass();
  // Init_Module_Klass();

  Module_Add_Class(ob, &Klass_Klass,  ACCESS_PUBLIC);
  Module_Add_Class(ob, &String_Klass, ACCESS_PUBLIC);
  Module_Add_Class(ob, &Tuple_Klass,  ACCESS_PUBLIC);
  Module_Add_Class(ob, &Table_Klass,  ACCESS_PUBLIC);
  Module_Add_Class(ob, &Module_Klass, ACCESS_PUBLIC);

  GState_Add_Module(&gs, "koala/lang", ob);
}

static void init_modules(void)
{
  init_lang_module();
  //init_io_module();
}

/*-------------------------------------------------------------------------*/

void Koala_Initialize(void)
{
  GState_Initialize(&gs);
  init_modules();
}

void Koala_Run_String(char *str)
{
  UNUSED_PARAMETER(str);
}

void Koala_Run_File(char *path)
{
  UNUSED_PARAMETER(path);
}

void Koala_Run_Module(Object *ob)
{
  UNUSED_PARAMETER(ob);
}

void Koala_Finalize(void)
{
  GState_Finalize(&gs);
}
