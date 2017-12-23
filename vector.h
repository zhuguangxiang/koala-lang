
#ifndef _KOALA_VECTOR_H_
#define _KOALA_VECTOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#define VECTOR_DEFAULT_CAPACTIY 16

typedef struct vector {
  int capacity;
  void **objs;
} Vector;

Vector *Vector_Create(void);
void Vector_Destroy(Vector *vec, void (*fini)(void *, void *), void *arg);
int Vector_Init(Vector *vec, int size);
void Vector_Fini(Vector *vec, void (*fini)(void *, void *), void *arg);
int Vector_Set(Vector *vec, int index, void *obj);
void *Vector_Get(Vector *vec, int index);

#define Vector_Size(vec)  ((vec)->capacity)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_VECTOR_H_ */
