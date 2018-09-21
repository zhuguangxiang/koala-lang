
#include "gc.h"
#include "stringobject.h"
#include "log.h"
#include "koalastate.h"

GCState gcs;

void *GC_Alloc(int size)
{
  Object *ob = calloc(1, size);
  ob->ob_marked = gcs.currentwhite;
  ob->ob_next = gcs.gcobjs;
  gcs.gcobjs = ob;
  ++gcs.count;

  return ob;
}

void GC_Free(Object *ob)
{
  debug("free object:%s", OB_KLASS(ob)->name);
  OB_KLASS(ob)->ob_free(ob);
  free(ob);
}

void GC_Run(void)
{
  Vector stack = VECTOR_INIT;
  Koala_Collect_Modules(&stack);

  gcs.state = GC_MARK;

  Object *ob;
  Vector_ForEach(ob, &stack) {
    ob->ob_marked = GC_BLACK;
    if (OB_KLASS(ob)->ob_mark)
      OB_KLASS(ob)->ob_mark(ob);
  }

  gcs.state = GC_SWEEP;
  Object *head = NULL;
  Object *next;
  ob = gcs.gcobjs;
  while (ob) {
    next = ob->ob_next;
    if (ob->ob_marked == gcs.currentwhite) {
      GC_Free(ob);
    } else {
      ob->ob_next = head;
      head = ob;
    }
    ob = next;
  }
  gcs.gcobjs = head;
  gcs.state = GC_STOP;
}

void GC_Init(void)
{
  gcs.state = GC_STOP;
  gcs.count = 0;
  gcs.gcobjs = NULL;
  Vector_Init(&gcs.grayobjs);
  Vector_Init(&gcs.blackobjs);
}
