
#include "object.h"
#include "tupleobject.h"
#include "tableobject.h"
#include "methodobject.h"
#include "kstate.h"

/* gcc -g -std=c99 object_test.c -lkoala -L. */

void test_object(void)
{
  TValue v = Klass_Get(&Klass_Klass, "GetMethod");
  assert(tval_isobject(v));
  Object *ob = TVAL_OBJECT(v);
  assert(OB_KLASS(ob) == &Method_Klass);
  MethodObject *m = (MethodObject *)ob;
  assert(m->type == METH_CFUNC);

  ob = Tuple_Build_Many(2, TValue_Build('i', 100), TValue_Build('f', 3.14));
  Object *m1 = Object_Get_Method(ob, "Size");
  Method_Invoke(m1, ob, NULL);
  m1 = Object_Get_Method(ob, "Get");
  //Method_Invoke(m1, ob, Tuple_Pack_Format("i", 0));
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
  Object *tuple = Tuple_Build_Many(2, TValue_Build('i', 123), TValue_Build('z', 1));

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
  cfunc cf = m_Invoke->cf;
  tuple = cf((Object *)m_Size, Tuple_Build_One(TValue_Build('O', tuple)));
  TValue v4 = Tuple_Get(tuple, 0);
  assert(TVAL_TYPE(v4) == TYPE_INT && TVAL_INT(v4) == 2);
}

int main(int argc, char *argv[])
{
  Init_Klass_Klass();
  Init_Tuple_Klass();
  Init_Table_Klass();
  Init_Method_Klass();

  test_object();
  test_method();
  Object_Clean();
  Object_Clean();
  return 0;
}
