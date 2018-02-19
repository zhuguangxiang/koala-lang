
#ifndef _KOALA_ATOM_H_
#define _KOALA_ATOM_H_

#include "hashtable.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct atomentry{
	HashNode hnode;
	int type;
	int index;
	void *data;
} AtomEntry;

typedef struct atomtable {
	HashTable table;
	int size;
	Vector items[0];
} AtomTable;

typedef void (*atomfunc)(int type, void *data, void *arg);
AtomTable *AtomTable_New(HashInfo *hashinfo, int size);
void AtomTable_Free(AtomTable *table, atomfunc fn, void *arg);
int AtomTable_Init(AtomTable *table, HashInfo *hashinfo, int size);
void AtomTable_Fini(AtomTable *table, atomfunc fn, void *arg);
int AtomTable_Append(AtomTable *table, int type, void *data, int unique);
int AtomTable_Index(AtomTable *table, int type, void *data);
void *AtomTable_Get(AtomTable *table, int type, int index);
int AtomTable_Size(AtomTable *table, int type);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_ATOM_H_ */
