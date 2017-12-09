
#include "globalstate.h"
#include "tableobject.h"
#include "nameobject.h"
#include "moduleobject.h"

static GlobalState GState;

void Object_Add_GCList(Object *ob)
{
  // lock
  Object *temp = GState.gcobjects;
  ob->ob_next = temp;
  GState.gcobjects = ob;
  // unlock
}

void Object_Clean(void)
{
  Object *ob = GState.gcobjects;
  Object *next;

  while (ob != NULL) {
    next = ob->ob_next;
    if (ob->ob_ref == 0) {
      OB_KLASS(ob)->ob_free(ob);
    } else {
      ob->ob_ref = 0;
    }
    ob = next;
  }
}

void Init_GlobalState(void)
{
  GState.modules = Table_New();
}

int GState_Add_Module(char *path_name, Object *mo)
{
  assert(OB_KLASS(mo) == &Module_Klass);
  Object *no = Name_New(path_name, NT_MODULE, 0, NULL, NULL);
  TValue key = TValue_Build('O', no);
  TValue val = TValue_Build('O', mo);
  return Table_Put(GState.modules, key, val);
}

Object *GState_Find_Module(char *path_name)
{
  Object *ret = NULL;
  Object *ob = Name_New(path_name, NT_MODULE, 0, NULL, NULL);
  TValue v = Table_Get(GState.modules, TValue_Build('O', ob));
  TValue_Parse(v, 'O', &ret);
  return ret;
}
