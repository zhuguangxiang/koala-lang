
#include "kstate.h"

static KoalaState KState;

void Object_Add_GCList(Object *ob)
{
  // lock
  Object *temp = KState.gcobjects;
  ob->ob_next = temp;
  KState.gcobjects = ob;
  // unlock
}

void Object_Clean(void)
{
  Object *ob = KState.gcobjects;
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
