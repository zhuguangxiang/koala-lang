
#ifndef _KOALA_TUPLE_OBJECT_H_
#define _KOALA_TUPLE_OBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

struct tuple_object {
  OBJECT_HEAD
  size_t size;
  struct object *items[0];
};

extern struct klass_object tuple_klass;
void init_tuple_klass(void);

struct object *new_tuple(int size);
struct object *tuple_get(struct object *ob, int index);
int tuple_set(struct object *ob, int index, struct object *val);
int tuple_get_range(struct object *ob, int min, int max, char *format, ...);
int tuple_parse(struct object *ob, char *format, ...);
struct object *dup_tuple(struct object *ob, int new_size);
size_t tuple_size(struct object *ob);
struct object *tuple_build(char *format, ...);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_TUPLE_OBJECT_H_ */
