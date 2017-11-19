
#ifndef _KOALA_TUPLEOBJECT_H_
#define _KOALA_TUPLEOBJECT_H_

#include "structobject.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef StructObject TupleObject;

extern Klass Tuple_Klass;
void Init_Tuple_Klass(void);

Object *Tuple_New(int size);
TValue Tuple_Get(Object *ob, int index);
Object *Tuple_Get_Slice(Object *ob, int min, int max);
int Tuple_Set(Object *ob, int index, TValue val);
int Tuple_Size(Object *ob);
Object *Tuple_From_TValues(int count, ...);
Object *Tuple_Build(char *format, ...);
int Tuple_Parse(Object *ob, char *format, ...);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_TUPLEOBJECT_H_ */
