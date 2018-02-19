
#include "atom.h"

AtomTable *AtomTable_New(HashInfo *hashinfo, int size)
{
	AtomTable *table = malloc(sizeof(AtomTable) + size * sizeof(Vector));
	AtomTable_Init(table, hashinfo, size);
	return table;
}

void AtomTable_Free(AtomTable *table, atomfunc fn, void *arg)
{
	if (!table) return;
	AtomTable_Fini(table, fn, arg);
	free(table);
}

static AtomEntry *itementry_new(int type, int index, void *data)
{
	AtomEntry *e = malloc(sizeof(AtomEntry));
	Init_HashNode(&e->hnode, e);
	e->type  = type;
	e->index = index;
	e->data  = data;
	return e;
}

static void itementry_free(AtomEntry *e)
{
	free(e);
}

int AtomTable_Init(AtomTable *table, HashInfo *hashinfo, int size)
{
	HashTable_Init(&table->table, hashinfo);
	table->size = size;
	for (int i = 0; i < size; i++)
		Vector_Init(table->items + i);
	return 0;
}

typedef struct atomdata {
	atomfunc fn;
	void *arg;
	int type;
} AtomData;

static void atomdata_fini(void *data, void *arg)
{
	AtomData *atomdata = arg;
	atomdata->fn(atomdata->type, data, atomdata->arg);
}

static void ht_atomentry_free(HashNode *hnode, void *arg)
{
	UNUSED_PARAMETER(arg);
	AtomEntry *e = container_of(hnode, AtomEntry, hnode);
	itementry_free(e);
}

void AtomTable_Fini(AtomTable *table, atomfunc fn, void *arg)
{
	AtomData itemdata = {fn, arg, 0};
	for (int i = 0; i < table->size; i++) {
		itemdata.type = i;
		Vector_Fini(table->items + i, atomdata_fini, &itemdata);
	}

	HashTable_Fini(&table->table, ht_atomentry_free, NULL);
}

int AtomTable_Append(AtomTable *table, int type, void *data, int unique)
{
	Vector *vec = table->items + type;
	Vector_Append(vec, data);
	int index = Vector_Size(vec) - 1;
	assert(index >= 0);
	if (unique) {
		AtomEntry *e = itementry_new(type, index, data);
		int res = HashTable_Insert(&table->table, &e->hnode);
		assert(!res);
	}
	return index;
}

#define ATOM_ENTRY_INIT(t, i, d)  {.type = (t), .index = (i), .data = (d)}

int AtomTable_Index(AtomTable *table, int type, void *data)
{
	AtomEntry e = ATOM_ENTRY_INIT(type, 0, data);
	HashNode *hnode = HashTable_Find(&table->table, &e);
	if (!hnode) return -1;
	return container_of(hnode, AtomEntry, hnode)->index;
}

void *AtomTable_Get(AtomTable *table, int type, int index)
{
	assert(type >= 0 && type < table->size);
	return Vector_Get(table->items + type, index);
}

int AtomTable_Size(AtomTable *table, int type)
{
	assert(type >= 0 && type < table->size);
	return Vector_Size(table->items + type);
}
