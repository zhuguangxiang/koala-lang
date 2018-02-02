
#include "vector.h"
#include "common.h"
#include "log.h"

#define DEFAULT_CAPACTIY 16

int Vector_Init(Vector *vec)
{
  vec->capacity = 0;
  vec->size  = 0;
  vec->objs  = NULL;
  return 0;
}

void Vector_Fini(Vector *vec, vec_fini_func fini, void *arg)
{
  if (vec->objs == NULL) return;

  if (fini != NULL) {
    info("finalizing objs.\n");
    for (int i = 0; i < vec->capacity; i++) {
      void *obj = vec->objs[i];
      if (obj != NULL) fini(obj, arg);
    }
  }

  free(vec->objs);

  vec->capacity = 0;
  vec->size = 0;
  vec->objs = NULL;
}

static int __vector_expand(Vector *vec)
{
  int new_capactiy = vec->capacity * 2;
  void **objs = calloc(new_capactiy, sizeof(void **));
  if (objs == NULL) return -1;

  memcpy(objs, vec->objs, vec->capacity * sizeof(void **));
  free(vec->objs);
  vec->objs = objs;
  vec->capacity = new_capactiy;

  return 0;
}

int Vector_Set(Vector *vec, int index, void *obj)
{
  if (vec->objs == NULL) {
    void **objs = calloc(DEFAULT_CAPACTIY, sizeof(void **));
    if (objs == NULL) {
      error("calloc failed\n");
      return -1;
    }
    vec->capacity = DEFAULT_CAPACTIY;
    vec->size = 0;
    vec->objs = objs;
  }

  if (index >= vec->capacity) {
    if (index >= 2 * vec->capacity) {
      error("Are you kidding me? Is need expand double size?\n");
      return -1;
    }

    int res = __vector_expand(vec);
    if (res != 0) {
      error("expanded vector failed\n");
      return res;
    } else {
      info("expanded vector successfully\n");
    }
  }

  if (vec->objs[index] == NULL)
    ++vec->size;
  else
    warn("Position %d already has an item.\n", index);

  vec->objs[index] = obj;
  return index;
}

void *Vector_Get(Vector *vec, int index)
{
  if (vec->objs == NULL) {
    warn("vector is empty\n");
    return NULL;
  }

  if (index < vec->capacity) {
    return vec->objs[index];
  } else {
    error("argument's index is %d out of bound\n", index);
    return NULL;
  }
}

int Vector_Append_All(Vector *vec, Vector *from)
{
  void *obj;
  for (int i = 0; i < Vector_Size(from); i++) {
    obj = Vector_Get(from, i);
    Vector_Append(vec, obj);
  }
  return 0;
}

Vector *Vector_Create()
{
  Vector *vec = malloc(sizeof(*vec));
  if (vec == NULL) return NULL;
  Vector_Init(vec);
  return vec;
}

void Vector_Destroy(Vector *vec, vec_fini_func fini, void *arg)
{
  if (vec == NULL) return;
  Vector_Fini(vec, fini, arg);
  free(vec);
}
