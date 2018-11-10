
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "object.h"

void test_trait(void)
{
  LRONode *temp;
  /*
    trait A;
   */
  Klass *A = Trait_New("A", NULL);
  Vector *v1 = Vector_New();
  Vector_Append(v1, A);
  temp = Vector_Get_Last(&A->lro);
  assert(temp->klazz == A);
  /*
    trait B with A;
   */
  Klass *B = Trait_New("B", v1);
  Vector *v2 = Vector_New();
  Vector_Append(v2, B);
  Vector_ForEach(temp, &B->lro) {
    if (i == 0)
      assert(temp->klazz == A);
    else if (i == 1)
      assert(temp->klazz == B);
    else
      assert(0);
  }
  /*
    trait C with B;
   */
  Klass *C = Trait_New("C", v2);
  Vector_ForEach(temp, &C->lro) {
    if (i == 0)
      assert(temp->klazz == A);
    else if (i == 1)
      assert(temp->klazz == B);
    else if (i == 2)
      assert(temp->klazz == C);
    else
      assert(0);
  }
  /*
    trait D with A;
   */
  Klass *D = Trait_New("D", v1);
  Vector_ForEach(temp, &D->lro) {
    if (i == 0)
      assert(temp->klazz == A);
    else if (i == 1)
      assert(temp->klazz == D);
    else
      assert(0);
  }
  Vector *v3 = Vector_New();
  Vector_Append(v3, A);
  Vector_Append(v3, D);
  Vector_Append(v3, C);
  Vector_Append(v3, B);
  /*
    trait E with A with D with C With B;
   */
  Klass *E = Trait_New("E", v3);
  Vector_ForEach(temp, &E->lro) {
    if (i == 0)
      assert(temp->klazz == A);
    else if (i == 1)
      assert(temp->klazz == D);
    else if (i == 2)
      assert(temp->klazz == B);
    else if (i == 3)
      assert(temp->klazz == C);
    else if (i == 4)
      assert(temp->klazz == E);
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
