
#ifndef _KOALA_LISTOBJECT_H_
#define _KOALA_LISTOBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct listobject {
  OBJECT_HEAD
  int size;
  int capacity;
  Klass *type;
  TValue *items;
} ListObject;

#define LIST_DEFAULT_SIZE 32
extern Klass List_Klass;
void Init_List_Klass(void);
Object *List_New(Klass *klazz);
int List_Set(Object *ob, int index, TValue *val);

#ifdef __cplusplus
}
#endif
#endif /*_KOALA_LISTOBJECT_H_*/
