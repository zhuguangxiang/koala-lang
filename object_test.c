
#include "object.h"
#include "tupleobject.h"
#include "tableobject.h"
#include "methodobject.h"
#include "stringobject.h"
#include "moduleobject.h"
#include "globalstate.h"
#include "koala.h"

/* gcc -g -std=c99 object_test.c -lkoala -L. */

void test_object(void)
{
  TValue v = Klass_Get(&Klass_Klass, "GetMethod");
  assert(tval_isobject(v));
  Object *ob;
  int res = TValue_Parse(v, 'O', &ob);
  assert(!res);
  assert(OB_KLASS(ob) == &Method_Klass);
  MethodObject *m = (MethodObject *)ob;
  assert(m->type == METH_CFUNC);

  ob = Tuple_Build("if", 100, 3.14);
  v = Klass_Get(OB_KLASS(ob), "Size");
  Object *m1;
  res = TValue_Parse(v, 'O', &m1);
  assert(!res);
  ob = Method_Invoke(m1, ob, NULL);
  v = Tuple_Get(ob, 0);
  assert(v.type == TYPE_INT);
  assert(v.ival == 2);
}

void test_method(void)
{
  /*
    var klazz = Tuple.klazz;
    var m Method = klazz.GetMethod("Size");
    var tuple = Tuple(123, true);
    var size = m.invoke(tuple);
   */

  // var tuple = Tuple(123, true);
  Object *tuple = Tuple_Build("iz", 123, TRUE);

  // var klazz = Tuple.klazz;
  // var m Method = klazz.GetMethod("Size");
  TValue v = Klass_Get(&Tuple_Klass, "Size");
  assert(tval_isobject(v));
  Object *ob = TVAL_OBJECT(v);
  assert(OB_KLASS(ob) == &Method_Klass);
  MethodObject *m_Size = (MethodObject *)ob;
  assert(m_Size->type == METH_CFUNC);

  // var size = m.invoke(tuple);
  Klass *klazz = OB_KLASS(m_Size);
  v = Klass_Get(klazz, "Invoke");
  MethodObject *m_Invoke = (MethodObject *)TVAL_OBJECT(v);
  assert(m_Invoke && m_Invoke->type == METH_CFUNC);
  tuple = Method_Invoke((Object *)m_Invoke, (Object *)m_Size, Tuple_Build("O", tuple));
  TValue v4 = Tuple_Get(tuple, 0);
  assert(TVAL_TYPE(v4) == TYPE_INT && TVAL_INT(v4) == 2);
}

void test_klass_get_method(void)
{
  // var klazz = Tuple.klazz;
  // var m Method = klazz.GetMethod("Size");
  Object *ob = (Object *)&Tuple_Klass;
  TValue v = Klass_Get(OB_KLASS(ob), "GetMethod");
  MethodObject *meth = (MethodObject *)TVAL_OBJECT(v);  //kalss.GetMethod()
  assert(OB_KLASS(meth) == &Method_Klass);
  // call klass.GetMethod()
  ob = Method_Invoke((Object *)meth, ob, Tuple_Build("O", String_New("Size")));
  v = Tuple_Get(ob, 0);
  meth = (MethodObject *)TVAL_OBJECT(v);  //tuple.Size()
  assert(OB_KLASS(meth) == &Method_Klass);
  // var tuple = Tuple(123, true);
  Object *tuple = Tuple_Build("iz", 123, TRUE);
  // var i int = m.invoke(tuple);
  ob = Method_Invoke((Object *)meth, tuple, NULL);
  v = Tuple_Get(ob, 0);
  assert(TVAL_TYPE(v) == TYPE_INT && TVAL_INT(v) == 2);
}

void test_module(void)
{
  Object *ob = GState_Find_Module("koala/lang");
  Module_Display(ob);
  ob = GState_Find_Module("koala/reflect");
  Module_Display(ob);
}

int main(int argc, char *argv[])
{
  UNUSED_PARAMETER(argc);
  UNUSED_PARAMETER(argv);

  Koala_Initialize();

  test_object();
  test_method();
  test_klass_get_method();
  test_module();
  Object_Clean();
  Object_Clean();

  Koala_Finalize();

  return 0;
}
