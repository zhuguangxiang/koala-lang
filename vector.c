
#include "types.h"
#include "debug.h"
#include "vector.h"

#define DEFAULT_CAPACTIY 16

int vector_initialize(struct vector *vec, int capacity)
{
  if (capacity <= 0) capacity = DEFAULT_CAPACTIY;

  void **objs = (void **)calloc(capacity, sizeof(void *));
  if (objs == NULL) return -1;

  vec->capacity = capacity;
  vec->size = 0;
  vec->objs = objs;

  return 0;
}

void vector_finalize(struct vector *vec, vec_fini_func fini, void *arg)
{
  if (vec->objs == NULL) return;

  if (fini != NULL) {
    debug_info("finalizing objs.\n");
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

static int __vector_expand(struct vector *vec)
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

int vector_set(struct vector *vec, int index, void *obj)
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

  if (vec->objs[index] == NULL)
    ++vec->size;
  else
    debug_warn("Position %d already has an item.\n", index);
  vec->objs[index] = obj;
  return index;
}

void *vector_get(struct vector *vec, int index)
{
  if (index < vec->capacity) {
    return vec->objs[index];
  } else {
    debug_error("argument's index is %d out of bound\n", index);
    return NULL;
  }
}

struct vector *vector_create(void)
{
  struct vector *vec = malloc(sizeof(*vec));
  if (vec == NULL) return NULL;
  if (vector_initialize(vec, DEFAULT_CAPACTIY)) {
    debug_error("initialize vector failed\n");
    free(vec);
    return NULL;
  } else {
    return vec;
  }
}

void vector_destroy(struct vector *vec, vec_fini_func fini, void *arg)
{
  vector_finalize(vec, fini, arg);
  free(vec);
}
