
#include "methodobject.h"
#include "tupleobject.h"
#include "moduleobject.h"
#include "symbol.h"

void FuncStruct_Get_Proto(MethodProto *proto, FuncStruct *f)
{
  proto->nr_rets = Desc_Count(f->rdesc);
  proto->nr_args = Desc_Count(f->pdesc);
  proto->nr_locals = 0;
}

static MethodObject *__method_new(int type)
{
  MethodObject *m = malloc(sizeof(*m));
  init_object_head(m, &Method_Klass);
  m->type = type;
  return m;
}

Object *CMethod_New(cfunc cf, MethodProto *proto)
{
  MethodObject *m = __method_new(METH_CFUNC);
  m->cf = cf;
  m->nr_rets = proto->nr_rets;
  m->nr_args = proto->nr_args;
  m->nr_locals = proto->nr_locals;
  return (Object *)m;
}

Object *Method_New(MethodProto *proto, uint8 *codes,
                   ConstItem *k, ItemTable *itable)
{
  MethodObject *m = __method_new(METH_KFUNC);
  m->nr_rets = proto->nr_rets;
  m->nr_args = proto->nr_args;
  m->nr_locals = proto->nr_args + proto->nr_locals;
  m->kf.codes = codes;
  m->kf.itable = itable;
  m->kf.k = k;
  return (Object *)m;
}

/*-------------------------------------------------------------------------*/

static void method_free(Object *ob)
{
  MethodObject *m = OB_TYPE_OF(ob, MethodObject, Method_Klass);
  free(m);
}

Klass Method_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass),
  .name  = "Method",
  .bsize = sizeof(MethodObject),

  .ob_free = method_free
};
