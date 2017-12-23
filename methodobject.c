
#include "methodobject.h"
#include "tupleobject.h"
#include "nameobject.h"

static MethodObject *method_new(int type)
{
  MethodObject *m = malloc(sizeof(*m));
  init_object_head(m, &Method_Klass);
  m->type = type;
  Object_Add_GCList((Object *)m);
  return m;
}

Object *CMethod_New(cfunc cf)
{
  MethodObject *m = method_new(METH_CFUNC);
  m->cf = cf;
  return (Object *)m;
}

Object *KMethod_New(Instruction *codes, TValue *k, Object *closure)
{
  MethodObject *m = method_new(METH_KFUNC);
  m->kf.codes = codes;
  m->kf.k = k;
  m->kf.closure = closure;
  return (Object *)m;
}

void Method_Set_Info(Object *ob, int nr_rets, int nr_args, int nr_locals)
{
  MethodObject *m = (MethodObject *)ob;
  m->nr_rets = nr_rets;
  m->nr_args = nr_args;
  m->nr_locals = nr_locals;
}

Object *Method_Invoke(Object *method, Object *ob, Object *args)
{
  assert(OB_KLASS(method) == &Method_Klass);
  MethodObject *m = (MethodObject *)method;
  if (m->type == METH_CFUNC) {
    return m->cf(ob, args);
  } else {
    // new frame and execute it
    return NULL;
  }
}

/*-------------------------------------------------------------------------*/
static Object *__method_invoke(Object *ob, Object *args)
{
  assert(OB_KLASS(ob) == &Method_Klass);
  MethodObject *m = (MethodObject *)ob;
  if (m->type == METH_CFUNC) {
    TValue v = Tuple_Get(args, 0);
    assert(TVAL_ISOBJ(v));
    Object *newargs = Tuple_Get_Slice(args, 1, Tuple_Size(args) - 1);
    Object *res = Method_Invoke(ob, OBJECT_TVAL(v), newargs);
    //ob_decref(newargs);
    return res;
  } else {
    // new frame and execute it
    return NULL;
  }
}

static MethodStruct method_methods[] = {
  {
    "Invoke",
    "T",
    "T[T",
    ACCESS_PUBLIC,
    __method_invoke
  },
  {NULL, NULL, NULL, 0, NULL}
};

void Init_Method_Klass(void)
{
  Klass_Add_Methods(&Method_Klass, method_methods);
}

/*-------------------------------------------------------------------------*/

static void method_free(Object *ob)
{
  assert(OB_KLASS(ob) == &Method_Klass);
  MethodObject *m = (MethodObject *)ob;
  free(m);
}

Klass Method_Klass = {
  OBJECT_HEAD_INIT(&Klass_Klass),
  .name  = "Method",
  .bsize = sizeof(MethodObject),

  .ob_free = method_free
};
