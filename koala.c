
#include "koala.h"
#include "symbol.h"

KoalaState ks;

/* koala/lang.klc */
static void init_lang_module(void)
{
  Object *ob = Module_New("lang", "koala/lang", 0);
  Init_Klass_Klass(ob);
  Init_String_Klass(ob);
  Init_Tuple_Klass(ob);
  Init_Table_Klass(ob);
  Init_Module_Klass(ob);
  Init_Method_Klass(ob);
}

/* koala/io.klc */
static void init_io_module(void)
{

}

static void init_modules(void)
{
  init_lang_module();
  init_io_module();
}

/*-------------------------------------------------------------------------*/

void Koala_Init(void)
{
  KState_Init(&ks);
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

void Koala_Fini(void)
{
  KState_Fini(&ks);
}
