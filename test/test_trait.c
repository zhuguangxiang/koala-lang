
#include "koala.h"

void test_trait(void)
{
  LRONode *lro;
  /*
    trait A;
   */
  Klass *A = Trait_New_Self("A");

  /*
    trait B extends A;
   */
  VECTOR(v1);
  Vector_Append(&v1, A);
  Klass *B = Trait_New("B", &v1);
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
  Klass *C = Trait_New("C", &v2);
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
  Klass *D = Trait_New("D", &v1);
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
  Klass *E = Trait_New("E", &v3);
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

  Vector_Fini_Self(&v1);
  Vector_Fini_Self(&v2);
  Vector_Fini_Self(&v3);

  OB_DECREF(A);
  OB_DECREF(B);
  OB_DECREF(C);
  OB_DECREF(D);
  OB_DECREF(E);
}

int main(int argc, char *argv[])
{
  Koala_Initialize();
  test_trait();
  Koala_Finalize();
  return 0;
}
