
#ifndef _KOALA_VECTOR_H_
#define _KOALA_VECTOR_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
  'items' contains space for 'allocated' elements.
  The number currently in use is 'size'.
 */
typedef struct vector {
  int allocated;
  int size;
  void **items;
} Vector;

#define VECTOR_INIT {.allocated = 0, .size = 0, .items = NULL}
typedef void (*itemfunc)(void *item, void *arg);
Vector *Vector_New(void);
void Vector_Free(Vector *vec, itemfunc fn, void *arg);
int Vector_Init(Vector *vec);
void Vector_Fini(Vector *vec, itemfunc fn, void *arg);
int Vector_Insert(Vector *vec, int index, void *item);
void *Vector_Set(Vector *vec, int index, void *item);
void *Vector_Get(Vector *vec, int index);
int Vector_Append(Vector *vec, void *item);
#define Vector_Size(vec) (((Vector *)(vec))->size)
int Vector_Concat(Vector *dest, Vector *src);
#define Vector_ForEach(item, type, vec) \
  type *item; \
  if ((vec) != NULL) \
    for (int i = 0, f; f = (i < (vec)->size), item = (vec)->items[i], f; i++)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_VECTOR_H_ */
