
#ifndef _KOALA_NAMEOBJECT_H_
#define _KOALA_NAMEOBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NT_CONST    1
#define NT_VAR      2
#define NT_FUNC     3
#define NT_KLASS    4
#define NT_INTF     5
#define NT_FIELD    6
#define NT_METHOD   7
#define NT_MODULE   8

#define ACCESS_PUBLIC   0
#define ACCESS_PRIVATE  1

typedef struct typeindex {
  int offset;
  int size;
} TypeIndex;

typedef struct typelist {
  char *desc;
  int size;
  TypeIndex index[1];
} TypeList;

typedef struct nameobject {
  OBJECT_HEAD
  char *name;
  uint8 type;
  uint8 access;
  uint8 unused;
  uint8 size;
  TypeList *tlist[1];
} NameObject;

/* Exported symbols */
extern Klass Name_Klass;
void Init_Name_Klass(void);
Object *Name_New(char *name, uint8 type, uint8 access,
                 char *desc, char *pdesc);
void Name_Free(Object *ob);
void Name_Display(Object *ob);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_NAMEOBJECT_H_ */
