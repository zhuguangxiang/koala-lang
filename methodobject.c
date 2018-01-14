
#include "methodobject.h"
#include "tupleobject.h"
#include "moduleobject.h"
#include "symbol.h"

static MethodObject *method_new(int type)
{
  MethodObject *m = malloc(sizeof(*m));
  init_object_head(m, &Method_Klass);
  m->type = type;
  //Object_Add_GCList(m);
  return m;
}

Object *CMethod_New(cfunc cf)
{
  MethodObject *m = method_new(METH_CFUNC);
  m->cf = cf;
  return (Object *)m;
}

Object *KMethod_New(uint8 *codes, ConstItem *k, Object *closure)
{
  MethodObject *m = method_new(METH_KFUNC);
  m->kf.codes = codes;
  m->kf.k = k;
  m->kf.closure = closure;
  return (Object *)m;
}

Object *Method_Invoke(Object *method, Object *ob, Object *args)
{
  OB_ASSERT_KLASS(method, Method_Klass);
  MethodObject *m = (MethodObject *)method;
  if (m->type == METH_CFUNC) {
    return m->cf(ob, args);
  } else {
    // new frame and execute it
    return NULL;
  }
}

/*-------------------------------------------------------------------------*/
// static Object *__method_invoke(Object *ob, Object *args)
// {
//   ASSERT(OB_KLASS(ob) == &Method_Klass);
//   MethodObject *m = (MethodObject *)ob;
//   if (m->type == METH_CFUNC) {
//     TValue v = Tuple_Get(args, 0);
//     ASSERT(TV_ISOBJECT(&v));
//     Object *newargs = Tuple_Get_Slice(args, 1, Tuple_Size(args) - 1);
//     Object *res = Method_Invoke(ob, TV_OBJECT(&v), newargs);
//     //ob_decref(newargs);
//     return res;
//   } else {
//     // new frame and execute it
//     return NULL;
//   }
// }

// static CMethodStruct method_cmethods[] = {
//   {
//     "Invoke",
//     "a",
//     "a[a",
//     ACCESS_PUBLIC,
//     __method_invoke
//   },
//   {NULL, NULL, NULL, 0, NULL}
// };

void Init_Method_Klass(Object *ob)
{
  ModuleObject *mo = (ModuleObject *)ob;
  Method_Klass.itable = mo->itable;
  //Klass_Add_CFunctions(&Method_Klass, method_functions);
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
