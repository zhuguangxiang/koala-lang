
#ifndef _KOALA_VECTOR_H_
#define _KOALA_VECTOR_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vector {
  int capacity;
  int size;
  void **objs;
} Vector;

typedef void (*vec_fini_func)(void *obj, void *arg);

Vector *Vector_Create(void);
void Vector_Destroy(Vector *vec, vec_fini_func fini, void *arg);
int Vector_Init(Vector *vec);
void Vector_Fini(Vector *vec, vec_fini_func fini, void *arg);
int Vector_Set(Vector *vec, int index, void *obj);
void *Vector_Get(Vector *vec, int index);
#define Vector_Append(vec, obj) Vector_Set(vec, Vector_Size(vec), obj)
#define Vector_Size(vec)      ((vec)->size)
#define Vector_Capacity(vec)  ((vec)->capacity)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_VECTOR_H_ */
