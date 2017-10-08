
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "vector.h"

#define VECTOR_DEFAULT_CAPACTIY 16

int vector_init(struct vector *vec)
{
  void **objs = (void **)calloc(VECTOR_DEFAULT_CAPACTIY, sizeof(void *));
  if (objs == NULL) return -1;

  vec->objs     = objs;
  vec->capacity = VECTOR_DEFAULT_CAPACTIY;

  return 0;
}

void vector_fini(struct vector *vec, void (*fini)(void *, void *), void *arg)
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

static int vector_expand(struct vector *vec)
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
      fprintf(stderr, "[ERROR] are you kidding me? more than double?");
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
  return 0;
}

void *vector_get(struct vector *vec, int index)
{
  if (index < vec->capacity) {
    return vec->objs[index];
  } else {
    fprintf(stderr, "[ERROR] para index %d out of bound\n", index);
    return NULL;
  }
}

struct vector *vector_create(void)
{
  struct vector *vec = malloc(sizeof(*vec));
  if (vec == NULL) return NULL;
  if (vector_init(vec)) {
    fprintf(stderr, "[ERROR] init vector failed\n");
    free(vec);
    return NULL;
  } else {
    return vec;
  }
}

void vector_destroy(struct vector *vec,
                    void (*fini)(void *, void *), void *arg)
{
  vector_fini(vec, fini, arg);
  free(vec);
}
