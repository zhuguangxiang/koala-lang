
#include "debug.h"
#include "hash_table.h"

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
#define get_nr_entries(table) good_primes[((table)->prime_index)]

static uint32 get_proper_prime(struct hash_table *table)
{
  uint32 new_prime_index = table->prime_index;

  while (new_prime_index < prime_array_length() &&
         table->nr_nodes >= get_prime(new_prime_index) * HT_LOAD_FACTOR)
    ++new_prime_index;

  if (new_prime_index >= prime_array_length())
    new_prime_index = prime_array_length() - 1;

  return new_prime_index;
}

static struct hlist_head *new_entries(int nr_entries)
{
  struct hlist_head *entries;

  entries = malloc(sizeof(struct hlist_head) * nr_entries);
  if (entries == NULL) return NULL;

  for (int i = 0; i < nr_entries; i++)
    init_hlist_head(entries + i);

  return entries;
}

static void free_entries(struct hlist_head *entries)
{
  if (entries != NULL) free(entries);
}

int hash_table_initialize(struct hash_table *table,
                          ht_hash_func hash, ht_equal_func equal)
{
  struct hlist_head *entries = new_entries(get_prime(DEFAULT_PRIME_INDEX));
  if (entries == NULL) return -1;

  table->prime_index  = DEFAULT_PRIME_INDEX;
  table->nr_nodes     = 0;
  table->hash         = hash;
  table->equal        = equal;
  table->entries      = entries;

  return 0;
}

void hash_table_finalize(struct hash_table *table,
                         ht_fini_func fini, void *arg)
{
  struct hash_node *hnode;
  struct hlist_node *nxt;
  int nr_entries = get_nr_entries(table);

  for (int i = 0; i < nr_entries; i++) {
    hlist_for_each_entry_safe(hnode, nxt, table->entries + i, link) {
      hlist_del(&hnode->link);
      if (fini != NULL) fini(hnode, arg);
    }
  }

  free_entries(table->entries);
  table->prime_index = 0;
  table->nr_nodes    = 0;
  table->entries     = NULL;
}

struct hash_table *hash_table_create(ht_hash_func hash, ht_equal_func equal)
{
  struct hash_table *table = malloc(sizeof(*table));
  if (table == NULL) return NULL;

  if (hash_table_initialize(table, hash, equal)) {
    free(table);
    return NULL;
  }

  return table;
}

void hash_table_destroy(struct hash_table *table,
                        ht_fini_func fini, void *arg)
{
  if (table != NULL) {
    hash_table_finalize(table, fini, arg);
    free(table);
  }
}

static struct hash_node *__hash_table_find(struct hash_table *table,
                                           uint32 hash, void *key)
{
  uint32 idx  = hash % get_nr_entries(table);
  struct hash_node *hnode;

  hlist_for_each_entry(hnode, table->entries + idx, link) {
    if (table->equal(hnode->key, key))
      return hnode;
  }

  return NULL;
}

struct hash_node *hash_table_find(struct hash_table *table, void *key)
{
  return __hash_table_find(table, table->hash(key), key);
}

int hash_table_remove(struct hash_table *table, struct hash_node *hnode)
{
  if (hash_node_unhashed(hnode)) return -1;

  struct hash_node *temp = __hash_table_find(table, hnode->hash, hnode->key);
  if (temp == NULL) {
    debug_error("it is not in the hash table\n");
    return -1;
  }

  assert(temp == hnode);

  hlist_del(&hnode->link);
  --table->nr_nodes;

  return 0;
}

static void hash_table_expand(struct hash_table *table,
                              uint32 new_prime_index)
{
  struct hash_node *hnode;
  struct hlist_node *nxt;
  int nr_entries = get_nr_entries(table);
  uint32 index;
  int new_nr_entries = get_prime(new_prime_index);
  struct hlist_head *entries = new_entries(new_nr_entries);
  if (entries == NULL) {
    debug_error("expand table failed\n");
    return;
  }

  for (int i = 0; i < nr_entries; i++) {
    hlist_for_each_entry_safe(hnode, nxt, table->entries + i, link) {
      hlist_del(&hnode->link);
      index = hnode->hash % new_nr_entries;
      hlist_add_head(&hnode->link, entries + index);
    }
  }

  free_entries(table->entries);
  table->prime_index = new_prime_index;
  table->entries     = entries;
}

static void hash_table_maybe_expand(struct hash_table *table)
{
  uint32 new_prime_index;

  if (table->prime_index >= prime_array_length()) return;

  if (table->nr_nodes < get_nr_entries(table) * HT_LOAD_FACTOR) return;

  new_prime_index = get_proper_prime(table);
  if (new_prime_index <= table->prime_index) return;

  hash_table_expand(table, new_prime_index);
}

int hash_table_insert(struct hash_table *table, struct hash_node *hnode)
{
  if (!hash_node_unhashed(hnode)) return -1;

  uint32 hash = hnode->hash = table->hash(hnode->key);
  if (NULL != __hash_table_find(table, hash, hnode->key)) {
    debug_error("key is duplicated\n");
    return -1;
  }

  hash_table_maybe_expand(table);

  uint32 index = hash % get_nr_entries(table);

  hlist_add_head(&hnode->link, table->entries + index);
  ++table->nr_nodes;

  return 0;
}

void hash_table_traverse(struct hash_table *table,
                         ht_visit_func visit, void *arg)
{
  if (table != NULL) {
    visit(table->entries, get_nr_entries(table), arg);
  }
}
