
#ifndef _KOALA_NAMEOBJECT_H_
#define _KOALA_NAMEOBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct name_struct {
  OBJECT_HEAD
  char *name;
  char *signature;
  uint8 type;
  uint8 access;
} NameObject;

extern Klass Name_Klass;
void Init_Name_Klass(void);
Object *Name_New(char *name, uint8 type, char *signature, uint8 access);
void Name_Free(Object *ob);

#define name_isprivate(n) ((n)->access & ACCESS_PRIVATE)
#define name_ispublic(n)  (!name_isprivate(n))
#define name_isconst(n)   ((n)->access & ACCESS_RDONLY)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_NAMEOBJECT_H_ */
