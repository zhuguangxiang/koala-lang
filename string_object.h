
#ifndef _KOALA_STRING_OBJECT_H_
#define _KOALA_STRING_OBJECT_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "object.h"

struct string_object {
  OBJECT_HEAD
  int length;
  char *str;
};

extern struct klass_object string_klass;
extern struct object *string_from_cstr(char *cstr);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_STRING_OBJECT_H_ */
