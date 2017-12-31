
#ifndef _KOALA_VECTOR_H_
#define _KOALA_VECTOR_H_

#ifdef __cplusplus
extern "C" {
#endif

struct vector {
  int capacity;
  int size;
  void **objs;
};

typedef void (*vec_fini_func)(void *obj, void *arg);

struct vector *vector_create(void);
void vector_destroy(struct vector *vec, vec_fini_func fini, void *arg);
int vector_initialize(struct vector *vec, int capacity);
void vector_finalize(struct vector *vec, vec_fini_func fini, void *arg);
int vector_set(struct vector *vec, int index, void *obj);
void *vector_get(struct vector *vec, int index);
#define vector_size(vec)      ((vec)->size)
#define vector_capacity(vec)  ((vec)->capacity)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_VECTOR_H_ */
