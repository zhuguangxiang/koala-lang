
#ifndef _KOALA_CLASSOBJECT_H_
#define _KOALA_CLASSOBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CLASS_OBJECT_HEAD  OBJECT_HEAD Object *super;
typedef struct classobject {
	CLASS_OBJECT_HEAD
} ClassObject;

typedef struct classvalue {
	int size;
	TValue items[0];
} ClassValue;

TValue Object_Get_Value(Object *ob, char *name);
int Object_Set_Value(Object *ob, char *name, TValue *val);
Klass *Class_New(char *name, Klass *super);
Object *Object_Get_Method(Object *ob, char *name, Object **rob);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_CLASSOBJECT_H_ */
