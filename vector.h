
#ifndef _KOALA_VECTOR_H_
#define _KOALA_VECTOR_H_
#ifdef __cplusplus
extern "C" {
#endif

#define VECTOR_DEFAULT_CAPACTIY 16

struct vector {
  int capacity;
  void **objs;
};

struct vector *vector_create(void);
void vector_destroy(struct vector *vec,
                    void (*fini)(void *, void *), void *arg);
int vector_init(struct vector *vec, int size);
void vector_fini(struct vector *vec, void (*fini)(void *, void *), void *arg);
int vector_set(struct vector *vec, int index, void *obj);
void *vector_get(struct vector *vec, int index);

#define vector_size(vec)  ((vec)->capacity)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_VECTOR_H_ */
