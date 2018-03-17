
#ifndef _KOALA_CLASSOBJECT_H_
#define _KOALA_CLASSOBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct classobject {
	OBJECT_HEAD
	Object *super;
	int size;
	TValue items[0];
} ClassObject;

TValue Object_Get_Value(Object *ob, char *name);
int Object_Set_Value(Object *ob, char *name, TValue *val);
Object *Object_Get_Method(Object *ob, char *name);
Klass *Class_New(char *name, Klass *super);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_CLASSOBJECT_H_ */
