
#ifndef _KOALA_TABLEOBJECT_H_
#define _KOALA_TABLEOBJECT_H_

#include "hash_table.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tableobject {
  OBJECT_HEAD
  struct hash_table table;
} TableObject;

/* Exported symbols */
extern Klass Table_Klass;
void Init_Table_Klass(void);
Object *Table_New(void);
TValue *Table_Get(Object *ob, TValue *key);
int Table_Put(Object *ob, TValue *key, TValue *value);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_TABLEOBJECT_H_ */
