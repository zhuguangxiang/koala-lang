
#include "types.h"
#include "vector.h"

int Vector_Init(Vector *vec, int capacity)
{
  if (capacity <= 0) capacity = VECTOR_DEFAULT_CAPACTIY;

  void **objs = (void **)calloc(capacity, sizeof(void *));
  if (objs == NULL) return -1;

  vec->capacity = capacity;
  vec->objs = objs;

  return 0;
}

void Vector_Fini(Vector *vec, void (*fini)(void *, void *), void *arg)
{
  if (vec->objs == NULL) return;

  if (fini != NULL) {
    fprintf(stdout, "[INFO] fini objs.\n");
    for (int i = 0; i < vec->capacity; i++)
      fini(vec->objs[i], arg);
  }

  free(vec->objs);
  vec->objs = NULL;
}

static int vector_expand(Vector *vec)
{
  int new_capactiy = vec->capacity * 2;
  void **objs = (void **)calloc(new_capactiy, sizeof(void *));
  if (objs == NULL) return -1;

  memcpy(objs, vec->objs, vec->capacity * sizeof(void *));
  free(vec->objs);
  vec->objs = objs;
  vec->capacity = new_capactiy;

  return 0;
}

int Vector_Set(Vector *vec, int index, void *obj)
{
  if (index >= vec->capacity) {
    if (index >= 2 * vec->capacity) {
      fprintf(stderr, "[ERROR] are you kidding me? more than double?\n");
      return -1;
    }

    int res = vector_expand(vec);
    if (res != 0) {
      fprintf(stderr, "[ERROR] expand vector failed\n");
      return res;
    } else {
      fprintf(stderr, "[INFO] expand vector successfully\n");
    }
  }

  vec->objs[index] = obj;
  return index;
}

void *Vector_Get(Vector *vec, int index)
{
  if (index < vec->capacity) {
    return vec->objs[index];
  } else {
    fprintf(stderr, "[ERROR] para index %d out of bound\n", index);
    return NULL;
  }
}

Vector *Vector_Create(void)
{
  Vector *vec = malloc(sizeof(*vec));
  if (vec == NULL) return NULL;
  if (Vector_Init(vec, VECTOR_DEFAULT_CAPACTIY)) {
    fprintf(stderr, "[ERROR] init vector failed\n");
    free(vec);
    return NULL;
  } else {
    return vec;
  }
}

void Vector_Destroy(Vector *vec, void (*fini)(void *, void *), void *arg)
{
  Vector_Fini(vec, fini, arg);
  free(vec);
}
