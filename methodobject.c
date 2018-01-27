
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

Object *CMethod_New(cfunc cf, ProtoInfo *proto)
{
  MethodObject *m = __method_new(METH_CFUNC);
  m->cf = cf;
  m->rets = proto->rsz;
  m->args = proto->psz;
  m->locals = 0;
  return (Object *)m;
}

Object *Method_New(FuncInfo *info, ConstItem *k, ItemTable *itable)
{
  MethodObject *m = __method_new(METH_KFUNC);
  m->rets = info->proto.rsz;
  m->args = info->proto.psz;
  m->locals = info->proto.psz + info->locals;
  m->kf.codes = info->codes;
  m->kf.itable = itable;
  m->kf.k = k;
  return (Object *)m;
}

void Init_Method_Klass(Object *ob)
{
  Module_Add_Class(ob, &Method_Klass);
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
