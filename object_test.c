
#include "object.h"
#include "tupleobject.h"
#include "tableobject.h"
#include "methodobject.h"

/* gcc -g -std=c99 object_test.c -lkoala -L. */

void test_object(void)
{
  TValue *tv = Klass_Get(&Klass_Klass, "GetMethod");
  assert(tval_isobject(tv));
  Object *ob = TVAL_OVAL(tv);
  assert(OB_KLASS(ob) == &Method_Klass);
  MethodObject *m = (MethodObject *)ob;
  assert(m->type == METH_CFUNC);
}

void test_method(void)
{
  /*
    var klazz = Tuple.klass;
    var m Method = klazz.GetMethod("Size");
    var tuple = Tuple(123, true);
    var size = m.invoke(tuple);
   */

  // var tuple = Tuple(123, true);
  TValue tv1 = {.type = TYPE_INT, .ival = 123};
  TValue tv2 = {.type = TYPE_BOOL, .bval = 1};
  Object *tuple = Tuple_Pack_Many(2, &tv1, &tv2);

  // var klazz = Tuple.klass;
  // var m Method = klazz.GetMethod("Size");
  TValue *tv = Klass_Get(&Tuple_Klass, "Size");
  assert(tval_isobject(tv));
  Object *ob = TVAL_OVAL(tv);
  assert(OB_KLASS(ob) == &Method_Klass);
  MethodObject *m_Size = (MethodObject *)ob;
  assert(m_Size->type == METH_CFUNC);

  // var size = m.invoke(tuple);
  Klass *klazz = OB_KLASS(m_Size);
  tv = Klass_Get(klazz, "Invoke");
  MethodObject *m_Invoke = (MethodObject *)TVAL_OVAL(tv);
  assert(m_Invoke && m_Invoke->type == METH_CFUNC);
  cfunc cf = m_Invoke->cf;
  TValue tv3 = {.type = TYPE_OBJECT, .ob = tuple};
  tuple = cf((Object *)m_Size, Tuple_Pack(&tv3));
  TValue *tv4 = Tuple_Get(tuple, 0);
  assert(tv4 && tv4->type == TYPE_INT && tv4->ival == 2);
}

int main(int argc, char *argv[])
{
  Init_Klass_Klass();
  Init_Tuple_Klass();
  Init_Table_Klass();
  Init_Method_Klass();

  test_object();
  test_method();
  return 0;
}
