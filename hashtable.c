
#include "hashtable.h"
#include "log.h"

#define HT_LOAD_FACTOR      0.65
#define DEFAULT_PRIME_INDEX 0

/*
 * This is a set of good hash table prime numbers, from:
 * http://planetmath.org/GoodHashTablePrimes
 * Each prime is roughly double the previous value, and as far as
 * possible from the nearest powers of two.
 */
static uint32 good_primes[] = {
	19,
	31,
	53,
	97,
	193,
	389,
	769,
	1543,
	3079,
	6151,
	12289,
	24593,
	49157,
	98317,
	196613,
	393241,
	786433,
	1572869,
	3145739,
	6291469,
	12582917,
	25165843,
	50331653,
	100663319,
	201326611,
	402653189,
	805306457,
	1610612741
};

#define get_prime(idx)        good_primes[idx]
#define prime_array_length()  nr_elts(good_primes)
#define get_nr_entries(table) good_primes[((table)->prime)]

static uint32 get_proper_prime(HashTable *table)
{
	uint32 new_prime = table->prime;

	while (new_prime < prime_array_length() &&
				 table->nodes >= get_prime(new_prime) * HT_LOAD_FACTOR)
		++new_prime;

	if (new_prime >= prime_array_length())
		new_prime = prime_array_length() - 1;

	return new_prime;
}

static struct hlist_head *new_entries(int nr_entries)
{
	struct hlist_head *entries;

	entries = calloc(nr_entries, sizeof(struct hlist_head));
	if (!entries) return NULL;

	for (int i = 0; i < nr_entries; i++)
		init_hlist_head(entries + i);

	return entries;
}

static void free_entries(struct hlist_head *entries)
{
	if (entries) free(entries);
}

int HashTable_SlotSize(HashTable *table)
{
	return get_nr_entries(table);
}

int HashTable_Init(HashTable *table, HashInfo *hashinfo)
{
	struct hlist_head *entries = new_entries(get_prime(DEFAULT_PRIME_INDEX));
	if (!entries) return -1;

	table->prime = DEFAULT_PRIME_INDEX;
	table->nodes = 0;
	table->hash = hashinfo->hash;
	table->equal = hashinfo->equal;
	table->entries = entries;
	init_list_head(&table->head);
	return 0;
}

void HashTable_Fini(HashTable *table, ht_visitfunc fn, void *arg)
{
	HashNode *hnode, *nxt;
	list_for_each_entry_safe(hnode, nxt, &table->head, llink) {
		list_del(&hnode->llink);
		hlist_del(&hnode->hlink);
		if (fn) fn(hnode, arg);
	}

	free_entries(table->entries);
	table->prime = 0;
	table->nodes = 0;
	table->entries = NULL;
}

HashTable *HashTable_New(HashInfo *hashinfo)
{
	HashTable *table = malloc(sizeof(HashTable));
	if (!table) return NULL;

	if (HashTable_Init(table, hashinfo)) {
		free(table);
		return NULL;
	}

	return table;
}

void HashTable_Free(HashTable *table, ht_visitfunc fn, void *arg)
{
	if (!table) return;
	HashTable_Fini(table, fn, arg);
	free(table);
}

HashNode *__HashTable_Find(HashTable *table, uint32 hash, void *key)
{
	uint32 idx  = hash % get_nr_entries(table);
	HashNode *hnode;
	struct hlist_node *node;
	hlist_for_each(node, table->entries + idx) {
		hnode = hlist_entry(node, HashNode, hlink);
		if (table->equal(hnode->key, key))
			return hnode;
	}

	return NULL;
}

int HashTable_Remove(HashTable *table, HashNode *hnode)
{
	if (HashNode_Unhashed(hnode)) return -1;

	HashNode *temp = __HashTable_Find(table, hnode->hash, hnode->key);
	if (!temp) {
		error("it is not in the hash table");
		return -1;
	}

	assert(temp == hnode);

	hlist_del(&hnode->hlink);
	list_del(&hnode->llink);
	--table->nodes;

	return 0;
}

static void htable_expand(HashTable *table, uint32 new_prime)
{
	HashNode *hnode;
	struct hlist_node *node;
	struct hlist_node *nxt;
	int nr_entries = get_nr_entries(table);
	uint32 index;
	int new_nr_entries = get_prime(new_prime);
	struct hlist_head *entries = new_entries(new_nr_entries);
	if (!entries) {
		error("expand table failed");
		return;
	}

	for (int i = 0; i < nr_entries; i++) {
		hlist_for_each_safe(node, nxt, table->entries + i) {
			hlist_del(node);
			hnode = hlist_entry(node, HashNode, hlink);
			index = hnode->hash % new_nr_entries;
			hlist_add_head(&hnode->hlink, entries + index);
		}
	}

	free_entries(table->entries);
	table->prime = new_prime;
	table->entries = entries;
}

static void htable_maybe_expand(HashTable *table)
{
	int new_prime;

	if (table->prime >= prime_array_length()) return;

	if (table->nodes < get_nr_entries(table) * HT_LOAD_FACTOR) return;

	new_prime = get_proper_prime(table);
	if (new_prime <= table->prime) return;

	htable_expand(table, new_prime);
}

int HashTable_Insert(HashTable *table, HashNode *hnode)
{
	if (!HashNode_Unhashed(hnode)) return -1;

	uint32 hash = hnode->hash = table->hash(hnode->key);
	if (__HashTable_Find(table, hash, hnode->key)) {
		warn("key is duplicated");
		return -1;
	}

	htable_maybe_expand(table);

	uint32 index = hash % get_nr_entries(table);

	hlist_add_head(&hnode->hlink, table->entries + index);
	list_add_tail(&hnode->llink, &table->head);
	++table->nodes;

	return 0;
}

void HashTable_Traverse(HashTable *table, ht_visitfunc fn, void *arg)
{
	if (!table) return;

	HashNode *hnode;
	list_for_each_entry(hnode, &table->head, llink)
		fn(hnode, arg);
}
