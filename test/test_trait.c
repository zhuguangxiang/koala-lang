
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "object.h"

void test_trait(void)
{
  LRONode *lro;
  /*
    trait A;
   */
  Klass *A = Klass_New("A", NULL);

  /*
    trait B extends A;
   */
  VECTOR(v1);
  Vector_Append(&v1, A);
  Klass *B = Klass_New("B", &v1);
  Vector_ForEach(lro, &B->lro) {
    if (i == 0)
      assert(lro->klazz == &Any_Klass);
    else if (i == 1)
      assert(lro->klazz == A);
    else if (i == 2)
      assert(lro->klazz == B);
    else
      assert(0);
  }

  /*
    trait C extends B;
   */
  VECTOR(v2);
  Vector_Append(&v2, B);
  Klass *C = Klass_New("C", &v2);
  Vector_ForEach(lro, &C->lro) {
    if (i == 0)
      assert(lro->klazz == &Any_Klass);
    else if (i == 1)
      assert(lro->klazz == A);
    else if (i == 2)
      assert(lro->klazz == B);
    else if (i == 3)
      assert(lro->klazz == C);
    else
      assert(0);
  }

  /*
    trait D extends A;
   */
  Klass *D = Klass_New("D", &v1);
  Vector_ForEach(lro, &D->lro) {
    if (i == 0)
      assert(lro->klazz == &Any_Klass);
    else if (i == 1)
      assert(lro->klazz == A);
    else if (i == 2)
      assert(lro->klazz == D);
    else
      assert(0);
  }

  /*
    trait E extends A with D with C With B;
   */
  VECTOR(v3);
  Vector_Append(&v3, A);
  Vector_Append(&v3, D);
  Vector_Append(&v3, C);
  Vector_Append(&v3, B);
  Klass *E = Klass_New("E", &v3);
  Vector_ForEach(lro, &E->lro) {
    if (i == 0)
      assert(lro->klazz == &Any_Klass);
    else if (i == 1)
      assert(lro->klazz == A);
    else if (i == 2)
      assert(lro->klazz == D);
    else if (i == 3)
      assert(lro->klazz == B);
    else if (i == 4)
      assert(lro->klazz == C);
    else if (i == 5)
      assert(lro->klazz == E);
    else
      assert(0);
  }
}

int main(int argc, char *argv[])
{
  AtomString_Init();
  test_trait();
  AtomString_Fini();
  return 0;
}
