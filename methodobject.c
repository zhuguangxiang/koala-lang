
#include "methodobject.h"
#include "tupleobject.h"
#include "moduleobject.h"
#include "symbol.h"

static MethodObject *__method_new(int type)
{
  MethodObject *m = malloc(sizeof(*m));
  init_object_head(m, &Method_Klass);
  m->type = type;
  return m;
}

Object *CMethod_New(cfunc cf)
{
  MethodObject *m = __method_new(METH_CFUNC);
  m->cf = cf;
  return (Object *)m;
}

Object *Method_New(uint8 *codes, ConstItem *k, ItemTable *itable)
{
  MethodObject *m = __method_new(METH_KFUNC);
  m->kf.codes = codes;
  m->kf.itable = itable;
  m->kf.k = k;
  return (Object *)m;
}

void Method_Set_Proto(Object *ob, int retc, int argc, int locals)
{
  MethodObject *m = OB_TYPE_OF(ob, MethodObject, Method_Klass);
  m->nr_rets = retc;
  m->nr_args = argc;
  m->nr_locals = argc + locals;
}

/*-------------------------------------------------------------------------*/

void Init_Method_Klass(Object *ob)
{
  ModuleObject *mo = (ModuleObject *)ob;
  Method_Klass.itable = mo->itable;
  Module_Add_Class(ob, &Method_Klass, ACCESS_PUBLIC);
}

/*-------------------------------------------------------------------------*/

static void method_free(Object *ob)
{
  OB_ASSERT_KLASS(ob, Method_Klass);
  MethodObject *m = (MethodObject *)ob;
  free(m);
}

Klass Method_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass),
  .name  = "Method",
  .bsize = sizeof(MethodObject),

  .ob_free = method_free
};
