
#include "types.h"
#include "debug.h"
#include "vector.h"

#define DEFAULT_CAPACTIY 16

int Vector_Initialize(Vector *vec, int capacity, int isize)
{
  if (capacity <= 0) capacity = DEFAULT_CAPACTIY;

  void *objs = calloc(capacity, isize);
  if (objs == NULL) return -1;

  vec->capacity = capacity;
  vec->isize = isize;
  vec->size  = 0;
  vec->objs  = objs;

  return 0;
}

void Vector_Finalize(Vector *vec, vec_fini_func fini, void *arg)
{
  if (vec->objs == NULL) return;

  if (fini != NULL) {
    debug_info("finalizing objs.\n");
    for (int i = 0; i < vec->capacity; i++) {
      void *obj = (void *)((char *)vec->objs + i * vec->isize);
      fini(obj, arg);
    }
  }

  free(vec->objs);

  vec->capacity = 0;
  vec->isize = 0;
  vec->size  = 0;
  vec->objs  = NULL;
}

static int __vector_expand(Vector *vec)
{
  int new_capactiy = vec->capacity * 2;
  void *objs = calloc(new_capactiy, vec->isize);
  if (objs == NULL) return -1;

  memcpy(objs, vec->objs, vec->capacity * vec->isize);
  free(vec->objs);
  vec->objs = objs;
  vec->capacity = new_capactiy;

  return 0;
}

static int __vector_item_empty(void *obj, int sz)
{
  char *data = obj;
  for (int i = 0; i < sz; i++) {
    if (data[i]) return 0;
  }
  return 1;
}

int Vector_Set(Vector *vec, int index, void *obj)
{
  if (index >= vec->capacity) {
    if (index >= 2 * vec->capacity) {
      debug_error("Are you kidding me? Is need expand double size?\n");
      return -1;
    }

    int res = __vector_expand(vec);
    if (res != 0) {
      debug_error("expanded vector failed\n");
      return res;
    } else {
      debug_info("expanded vector successfully\n");
    }
  }

  if (__vector_item_empty((void *)((char *)vec->objs + index * vec->isize),
      vec->isize))
    ++vec->size;
  else
    debug_warn("Position %d already has an item.\n", index);
  memcpy((void *)((char *)vec->objs + index * vec->isize), obj, vec->isize);
  return index;
}

void *Vector_Get(Vector *vec, int index)
{
  if (index < vec->capacity) {
    return (void *)((char *)vec->objs + index * vec->isize);
  } else {
    debug_error("argument's index is %d out of bound\n", index);
    return NULL;
  }
}

Vector *Vector_Create(int isize)
{
  Vector *vec = malloc(sizeof(*vec));
  if (vec == NULL) return NULL;
  if (Vector_Initialize(vec, DEFAULT_CAPACTIY, isize)) {
    debug_error("initialize vector failed\n");
    free(vec);
    return NULL;
  } else {
    return vec;
  }
}

void Vector_Destroy(Vector *vec, vec_fini_func fini, void *arg)
{
  Vector_Finalize(vec, fini, arg);
  free(vec);
}
