
#ifndef _KOALA_HASH_TABLE_H_
#define _KOALA_HASH_TABLE_H_

#include "list.h"
#include "hash.h"

#ifdef __cplusplus
extern "C" {
#endif

struct hash_node {
  struct hlist_node link;     /* conflict list */
  uint32 hash;                /* hash value */
  void *key;                  /* hash key */
};

#define init_hash_node(hnode, k) do { \
  init_hlist_node(&(hnode)->link);    \
  (hnode)->hash  = 0;                 \
  (hnode)->key   = k;                 \
} while (0)

#define hash_node_unhashed(hnode) hlist_unhashed(&(hnode)->link)

#define hash_list_for_each(hnode, head) \
  hlist_for_each_entry(hnode, head, link)

typedef uint32 (*ht_hash_t)(void *key);
typedef int (*ht_equal_t)(void *key1, void *key2);

struct hash_table {
  uint32 prime_index;           /* prime array index, internal used */
  uint32 nr_nodes;              /* number of nodes in hash table */
  ht_hash_t hash;               /* hash function */
  ht_equal_t equal;             /* equal function */
  struct hlist_head *entries;   /* conflict list array */
};

/**
 * Create a hash table,
 * which is automatically resized, although this incurs a performance penalty.
 */
struct hash_table *hash_table_create(ht_hash_t hash, ht_equal_t equal);

/* Free a hash table */
void hash_table_destroy(struct hash_table *table,
                        void (*fini)(struct hash_node *, void *), void *arg);

/* Find a node with its key  */
struct hash_node *hash_table_find(struct hash_table *table, void *key);

/* Remove a node from the hash table by its node */
int hash_table_remove(struct hash_table *table, struct hash_node *hnode);

/* Insert a node to the hash table */
int hash_table_insert(struct hash_table *table, struct hash_node *hnode);

/* Traverse all nodes in the hash table */
void hash_table_traverse(struct hash_table *table,
  void (*visit)(struct hlist_head *, int, void *), void *arg);

int hash_table_init(struct hash_table *table,
                    ht_hash_t hash, ht_equal_t equal);

void hash_table_fini(struct hash_table *table,
                     void (*fini)(struct hash_node *, void *), void *arg);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_HASH_TABLE_H_ */
