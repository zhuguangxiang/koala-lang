
#include "properties.h"
#include "hash.h"
#include "log.h"

PropEntry *PropEntry_New(char *key)
{
	PropEntry *e = malloc(sizeof(PropEntry));
	Init_HashNode(&e->hnode, e);
	e->key = key;
	return e;
}

static uint32 prop_hash(void *k)
{
	PropEntry *e = k;
	return hash_string(e->key);
}

static int prop_equal(void *k1, void *k2)
{
	PropEntry *e1 = k1;
	PropEntry *e2 = k2;
	return !strcmp(e1->key, e2->key);
}

int Properties_Init(Properties *prop)
{
	HashInfo hashinfo;
	Init_HashInfo(&hashinfo, prop_hash, prop_equal);
	HashTable_Init(&prop->table, &hashinfo);
	Vector_Init(&prop->vec);
	return 0;
}

void Properties_Fini(Properties *prop)
{
	HashTable_Fini(&prop->table, NULL, NULL);
	Vector_Fini(&prop->vec, NULL, NULL);
}

int Properties_Put(Properties *prop, char *key, char *val)
{
	PropEntry e = {.key = key};
	PropEntry *entry = HashTable_Find(&prop->table, &e);
	if (entry) {
		if (entry->count == 1) {
			char *v = Vector_Get(&prop->vec, entry->index);
			Vector *vec = Vector_New();
			Vector_Append(vec, v);
			Vector_Append(vec, val);
			Vector_Set(&prop->vec, entry->index, vec);
			entry->count++;
		} else {
			Vector *vec = Vector_Get(&prop->vec, entry->index);
			Vector_Append(vec, val);
			entry->count++;
		}
	} else {
		entry = PropEntry_New(key);
		entry->count = 1;
		entry->index = Vector_Size(&prop->vec);
		HashTable_Insert(&prop->table, &entry->hnode);
		Vector_Append(&prop->vec, val);
	}

	return 0;
}

char **Properties_Get(Properties *prop, char *key)
{
	PropEntry *e = Properties_Get_Entry(prop, key);
	if (!e) return NULL;
	if (e->count > 1) {
		warn("multi-values with key '%s'", key);
	}

	char **res = malloc(sizeof(char *) * (e->count + 1));
	char *val;
	int i = 0;
	while ((val = Properties_Next(prop, e, i))) {
		res[i++] = val;
	}
	res[i] = NULL;
	return res;
}

PropEntry *Properties_Get_Entry(Properties *prop, char *key)
{
	PropEntry e = {.key = key};
	return HashTable_Find(&prop->table, &e);
}

char *Properties_Next(Properties *prop, PropEntry *entry, int next)
{
	if (entry->count > 1) {
		Vector *vec = Vector_Get(&prop->vec, entry->index);
		if (next < entry->count) {
			return Vector_Get(vec, next);
		} else {
			return NULL;
		}
	} else {
		if (!next) {
			return Vector_Get(&prop->vec, entry->index);
		} else {
			return NULL;
		}
	}
}
