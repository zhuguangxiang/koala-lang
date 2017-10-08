
#ifndef _KOALA_INTEGER_OBJECT_H_
#define _KOALA_INTEGER_OBJECT_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "object.h"

struct integer_object {
  OBJECT_HEAD
  int64_t val;
};

extern struct klass_object integer_klass;
void init_integer_klass(void);
struct object *integer_from_int64(int64_t val);
int64_t integer_to_int64(struct object *ob);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_INTEGER_OBJECT_H_ */
