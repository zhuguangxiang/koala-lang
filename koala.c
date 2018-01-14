
#include "koala.h"
#include "mod_io.h"
#include "thread.h"

KoalaState ks;

static void Init_Lang_Module(void)
{
  Object *ob = Module_New("lang", "koala/lang", 0);
  Init_Klass_Klass(ob);
  Init_String_Klass(ob);
  Init_Tuple_Klass(ob);
  Init_Table_Klass(ob);
  Init_Module_Klass(ob);
  Init_Method_Klass(ob);
}

static void Init_Modules(void)
{
  /* koala/lang.klc */
  Init_Lang_Module();

  /* koala/io.klc */
  Init_IO_Module();
}

/*-------------------------------------------------------------------------*/

void Koala_Init(void)
{
  KState_Init(&ks);
  Init_Modules();
  sched_init();
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
