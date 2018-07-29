
#ifndef _KOALA_TUPLEOBJECT_H_
#define _KOALA_TUPLEOBJECT_H_

#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tupleobject {
	OBJECT_HEAD
	int size;
	TValue items[0];
} TupleObject;

extern Klass Tuple_Klass;
void Init_Tuple_Klass(void);
Object *Tuple_New(int size);
void Tuple_Free(Object *ob);
TValue Tuple_Get(Object *ob, int index);
Object *Tuple_Get_Slice(Object *ob, int min, int max);
int Tuple_Set(Object *ob, int index, TValue *val);
int Tuple_Size(Object *ob);
Object *Tuple_From_Va_TValues(int count, ...);
Object *Tuple_From_TValues(TValue *arr, int size);
Object *Tuple_Va_Build(char *format, va_list *vp);
Object *Tuple_Build(char *format, ...);
int Tuple_Parse(Object *ob, char *format, ...);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_TUPLEOBJECT_H_ */
