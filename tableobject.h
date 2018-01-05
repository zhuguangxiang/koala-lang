
#ifndef _KOALA_TABLEOBJECT_H_
#define _KOALA_TABLEOBJECT_H_

#include "hashtable.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tableobject {
  OBJECT_HEAD
  HashTable table;
} TableObject;

/* Exported symbols */
extern Klass Table_Klass;
void Init_Table_Klass(void);
Object *Table_New(void);
int Table_Get(Object *ob, TValue *key, TValue *rk, TValue *rv);
int Table_Put(Object *ob, TValue *key, TValue *value);
int Table_Count(Object *ob);
typedef void (*table_visit_func)(TValue *key, TValue *val, void *arg);
void Table_Traverse(Object *ob, table_visit_func visit, void *arg);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_TABLEOBJECT_H_ */
