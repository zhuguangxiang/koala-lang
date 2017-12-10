
#include "globalstate.h"
#include "nameobject.h"
#include "moduleobject.h"
#include "tupleobject.h"
#include "tableobject.h"
#include "stringobject.h"
#include "methodobject.h"

// koala/lang.klc
static void init_lang_module(void)
{
  Object *ob = Module_New("lang", 0);

  Init_Klass_Klass();
  Init_Name_Klass();
  Init_Module_Klass();
  Init_Tuple_Klass();
  Init_Table_Klass();
  Init_String_Klass();

  Module_Add_Klass(ob, &Klass_Klass,  0, 0);
  Module_Add_Klass(ob, &Name_Klass,   0, 0);
  Module_Add_Klass(ob, &Module_Klass, 0, 0);
  Module_Add_Klass(ob, &Tuple_Klass,  0, 0);
  Module_Add_Klass(ob, &Table_Klass,  0, 0);
  Module_Add_Klass(ob, &String_Klass, 0, 0);

  GState_Add_Module("koala/lang", ob);
}

// koala/reflect.klc
static void init_reflect_module(void)
{
  Object *ob = Module_New("reflect", 0);

  Init_Method_Klass();

  Module_Add_Klass(ob, &Method_Klass, 0, 0);

  GState_Add_Module("koala/reflect", ob);
}

static void init_modules(void)
{
  init_lang_module();
  init_reflect_module();
  //init_io_module();
}

/*-------------------------------------------------------------------------*/

void Koala_Initialize(void)
{
  Init_GlobalState();
  init_modules();
}

void Koala_Run_String(char *str)
{

}

void Koala_Run_File(char *path_name)
{

}

void Koala_Finalize(void)
{
  Fini_GlobalState();
}
