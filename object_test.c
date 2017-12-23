
#include "object.h"
#include "tupleobject.h"
#include "tableobject.h"
#include "methodobject.h"
#include "stringobject.h"
#include "moduleobject.h"
#include "globalstate.h"
#include "koala.h"
#include <unistd.h>

/* gcc -g -std=c99 object_test.c -lkoala -L. */

void test_object(void)
{
  TValue k = NIL_TVAL, v = NIL_TVAL;
  Klass_Get(&Klass_Klass, "GetMethod", &k, &v);
  assert(TVAL_ISOBJ(k) && TVAL_ISOBJ(v));
  Object *ob;
  int res = TValue_Parse(v, 'O', &ob);
  assert(!res);
  assert(OB_KLASS(ob) == &Method_Klass);
  MethodObject *m = (MethodObject *)ob;
  assert(m->type == METH_CFUNC);

  ob = Tuple_Build("if", 100, 3.14);
  k = NIL_TVAL; v = NIL_TVAL;
  Klass_Get(OB_KLASS(ob), "Size", &k, &v);

  Object *m1;
  res = TValue_Parse(v, 'O', &m1);
  assert(!res);

  ob = Method_Invoke(m1, ob, NULL);
  v = Tuple_Get(ob, 0);
  assert(TVAL_TYPE(v) == TYPE_INT);
  assert(INT_TVAL(v) == 2);
}

void test_method(void)
{
  /*
    var clazz = Tuple.Class;
    var m Method = klazz.GetMethod("Size");
    var tuple = Tuple(123, true);
    var size = m.invoke(tuple);
   */

  // var tuple = Tuple(123, true);
  Object *tuple = Tuple_Build("iz", 123, TRUE);

  // var clazz = Tuple.Class;
  // var m Method = klazz.GetMethod("Size");
  TValue k = NIL_TVAL, v = NIL_TVAL;
  Klass_Get(&Tuple_Klass, "Size", &k, &v);
  assert(TVAL_ISOBJ(v));
  Object *ob = OBJECT_TVAL(v);
  assert(OB_KLASS(ob) == &Method_Klass);
  MethodObject *m_Size = (MethodObject *)ob;
  assert(m_Size->type == METH_CFUNC);

  // var size = m.invoke(tuple);
  Klass *klazz = OB_KLASS(m_Size);
  k = NIL_TVAL; v = NIL_TVAL;
  Klass_Get(klazz, "Invoke", &k, &v);
  MethodObject *m_Invoke = (MethodObject *)OBJECT_TVAL(v);
  assert(m_Invoke && m_Invoke->type == METH_CFUNC);
  tuple = Method_Invoke((Object *)m_Invoke, (Object *)m_Size, Tuple_Build("O", tuple));
  TValue v4 = Tuple_Get(tuple, 0);
  assert(TVAL_TYPE(v4) == TYPE_INT && INT_TVAL(v4) == 2);
}

void test_klass_get_method(void)
{
  // var clazz = Tuple.Class;
  // var m Method = clazz.GetMethod("Size");
  Object *ob = (Object *)&Tuple_Klass;
  TValue k = NIL_TVAL, v = NIL_TVAL;
  Klass_Get(OB_KLASS(ob), "GetMethod", &k, &v);
  MethodObject *meth = (MethodObject *)OBJECT_TVAL(v);  //clazz.GetMethod()
  assert(OB_KLASS(meth) == &Method_Klass);
  // call clazz.GetMethod()
  ob = Method_Invoke((Object *)meth, ob, Tuple_Build("O", String_New("Size")));
  v = Tuple_Get(ob, 0);
  meth = (MethodObject *)OBJECT_TVAL(v);  //tuple.Size()
  assert(OB_KLASS(meth) == &Method_Klass);
  // var tuple = Tuple(123, true);
  Object *tuple = Tuple_Build("iz", 123, TRUE);
  // var i int = m.invoke(tuple);
  ob = Method_Invoke((Object *)meth, tuple, NULL);
  v = Tuple_Get(ob, 0);
  assert(TVAL_TYPE(v) == TYPE_INT && INT_TVAL(v) == 2);
}

void test_string(void)
{
  /*
   * var s = string("hello");
   * var len = s.Length();
   */
  Object *ob = String_New("hello");
  Object *method = NULL;
  TValue k = NIL_TVAL, v = NIL_TVAL;
  Klass_Get(OB_KLASS(ob), "Length", &k, &v);
  TValue_Parse(v, 'O', &method);
  ob = Method_Invoke(method, ob, NULL);
  int size = 0;
  Tuple_Parse(ob, "i", &size);
  assert(size == 5);
}

void test_module(void)
{
  Object *ob = GState_Find_Module("koala/lang");
  Module_Display(ob);
  TValue v = TVAL_INIT_NIL;
  Module_Get(ob, "String", NULL, &v);
  TValue_Parse(v, 'O', &ob);
  OB_CHECK_KLASS(ob, Klass_Klass);
  ob = GState_Find_Module("koala/reflect");
  Module_Display(ob);
}

void test_coroutine(void)
{
  // var p = String("hello");
  // go p.Length();
  Object *ob = String_New("hello");
  Object *method = NULL;
  TValue k = NIL_TVAL, v = NIL_TVAL;
  Klass_Get(OB_KLASS(ob), "Length", &k, &v);
  TValue_Parse(v, 'O', &method);
  ob = CoRoutine_New(1, method, ob, NULL);
  sleep(2);
  OB_CHECK_KLASS(ob, CoRoutine_Klass);
  CoRoutine *rt = (CoRoutine *)ob;
  int len = 0;
  TValue_Parse(CoRoutine_StackGet(rt, 0), 'i', &len);
  printf("String '%s', Length:%d\n", "hello", len);
}

void test_instruction(void)
{
  /*
   * func add(a int, b int) int, int, int {
   *  return a,b,a+b;
   * }
   */
  Instruction codes[] = {
    OP_LOAD, 1,
    OP_LOAD, 2,
    OP_LOAD, 1,
    OP_LOAD, 2,
    OP_ADD,
    OP_RET,
  };
  TValue k[5] = {
  };
  Object *mo = KMethod_New(codes, k, NULL);
  Method_Set_Info(mo, 3, 2, 0);

  Object *args = Tuple_Build("ii", -100, 2000);

  Object *ob = CoRoutine_New(1, mo, NULL, args);
  sleep(3);
  assert(((CoRoutine *)ob)->state == STATE_DEAD);
  OB_CHECK_KLASS(ob, CoRoutine_Klass);
  CoRoutine *rt = (CoRoutine *)ob;
  int a = 0, b = 0, c = 0;
  TValue_Parse(CoRoutine_StackGet(rt, 0), 'i', &a);
  TValue_Parse(CoRoutine_StackGet(rt, 1), 'i', &b);
  TValue_Parse(CoRoutine_StackGet(rt, 2), 'i', &c);
  printf("a = %d, b = %d, a + b = %d\n", a, b, c);
}

void test_module2(void)
{
  printf("test_module2\n");
  /*
   * test.kl
   *
   * func add(a int, b int) int, int, int {
   *  return a,b,a+b;
   * }
   *
   * go add(-10, 200);
   */

  // new module
  Object *mod = Module_New("test", 0);

  // add function
  static Instruction codes[] = {
    OP_LOAD, 1,
    OP_LOAD, 2,
    OP_LOAD, 1,
    OP_LOAD, 2,
    OP_ADD,
    OP_RET,
  };
  static TValue k[1] = {
    TVAL_INIT_NIL,
  };
  Object *method = KMethod_New(codes, k, NULL);
  Method_Set_Info(method, 3, 2, 0);
  Module_Add_Function(mod, "add", "III", "II", 0, method);

  static Instruction codes1[] = {
    OP_LOADK, 0, 0,
    OP_LOADK, 1, 0,
    OP_GO, 2,
    OP_RET,
  };
  static TValue k1[3] = {
    TVAL_INIT_INT(-10),
    TVAL_INIT_INT(200),
  };

  k1[2].type = TYPE_OBJECT;
  k1[2].ob = String_New("add");

  Object *__init__ = KMethod_New(codes1, k1, NULL);
  Method_Set_Info(__init__, 0, 0, 0);
  Module_Add_Function(mod, "__init__", NULL, "T", 0, __init__);
  Module_Display(mod);
  GState_Add_Module("demo/test", mod);

  printf("test_module2....\n");
  //Method_Invoke(__init__, mod, NULL);
  CoRoutine_New(1, __init__, mod, NULL);

  sleep(10);
}

void test_call(void)
{
  printf("test_module2\n");
  /*
   * test.kl
   *
   * func add(a int, b int) int {
   *  return a+b;
   * }
   *
   * var result = add(-10, 200);
   */

  // new module
  Object *mod = Module_New("test2", 1);

  // add function
  static Instruction codes[] = {
    OP_LOAD, 1,
    OP_LOAD, 2,
    OP_LOAD, 1,
    OP_LOAD, 2,
    OP_ADD,
    OP_RET,
  };
  static TValue k[1] = {
    TVAL_INIT_NIL,
  };
  Object *method = KMethod_New(codes, k, NULL);
  Method_Set_Info(method, 3, 2, 0);
  Module_Add_Function(mod, "add", "III", "II", 0, method);
  Module_Add_Variable(mod, "result", "I", 0, 0);

  static Instruction codes1[] = {
    OP_LOADK, 0, 0,
    OP_LOADK, 1, 0,
    OP_CALL,  2,
    OP_STORE, 0,
    OP_RET,
  };
  static TValue k1[3] = {
    TVAL_INIT_INT(-10),
    TVAL_INIT_INT(200),
  };
  k1[2].type = TYPE_OBJECT;
  k1[2].ob = String_New("add");

  Object *__init__ = KMethod_New(codes1, k1, NULL);
  Method_Set_Info(__init__, 0, 0, 0);
  Module_Add_Function(mod, "__init__", NULL, "T", 0, __init__);
  Module_Display(mod);
  GState_Add_Module("demo/test2", mod);

  CoRoutine_New(1, __init__, mod, NULL);
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
  test_string();
  test_coroutine();
  test_instruction();
  test_module2();
  test_call();

  //Object_Clean();
  //Object_Clean();

  Koala_Finalize();

  return 0;
}
