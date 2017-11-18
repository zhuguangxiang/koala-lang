
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
int Tuple_Get(Object *ob, int index, TValue **val);
int Tuple_Set(Object *ob, int index, TValue *val);
int Tuple_Get_Range(Object *ob, int min, int max, ...);
int Tuple_Size(Object *ob);
Object *Tuple_Pack(TValue *v);
Object *Tuple_Pack_Range(int count, ...);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_TUPLEOBJECT_H_ */
