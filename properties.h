
#ifndef _KOALA_PROPERTIES_H_
#define _KOALA_PROPERTIES_H_

#include "hashtable.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct prop_entry {
	HashNode hnode;
	char *key;
	int index;
	int count;
} PropEntry;

typedef struct properties {
	HashTable table;
	Vector vec;
} Properties;

Properties *Properties_New(void);
void Properties_Free(Properties *prop);
int Properties_Init(Properties *prop);
void Properties_Fini(Properties *prop);
int Properties_Put(Properties *prop, char *key, char *val);
char **Properties_Get(Properties *prop, char *key);
PropEntry *Properties_Get_Entry(Properties *prop, char *key);
char *Properties_Next(Properties *prop, PropEntry *entry, int next);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_PROPERTIES_H_ */
