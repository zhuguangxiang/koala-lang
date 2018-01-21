
#ifndef _KOALA_STRINGOBJECT_H_
#define _KOALA_STRINGOBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stringobject {
  OBJECT_HEAD
  int len;
  char *str;
} StringObject;

extern Klass String_Klass;
void Init_String_Klass(void);
Object *String_New(char *str);
void String_Free(Object *ob);
char *String_RawString(Object *ob);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_STRINGOBJECT_H_ */
