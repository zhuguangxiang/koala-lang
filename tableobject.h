
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
extern Object *Table_New(void);
extern TValue *Table_Get(Object *ob, TValue *key);
extern int Table_Put(Object *ob, TValue *key, TValue *value);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_TABLEOBJECT_H_ */
