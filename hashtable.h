
#ifndef _KOALA_HASHTABLE_H_
#define _KOALA_HASHTABLE_H_

#include "list.h"
#include "hash.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hash_node {
  struct hlist_node link;     /* conflict list */
  uint32 hash;                /* hash value */
  void *key;                  /* hash key */
} HashNode;

#define init_hash_node(hnode, k) do {  \
  init_hlist_node(&(hnode)->link);    \
  (hnode)->hash  = 0;                 \
  (hnode)->key   = k;                 \
} while (0)

#define hash_node_unhashed(hnode) hlist_unhashed(&(hnode)->link)

#define hash_list_for_each(hnode, head) \
  hlist_for_each_entry(hnode, head, link)

typedef uint32 (*ht_hash_func)(void *key);
typedef int (*ht_equal_func)(void *key1, void *key2);
typedef void (*ht_fini_func)(struct hash_node *hnode, void *arg);
typedef void (*ht_visit_func)(struct hlist_head *hlist, int count, void *arg);

typedef struct hash_table {
  uint32 prime_index;           /* prime array index, internal used */
  uint32 nr_nodes;              /* number of nodes in hash table */
  ht_hash_func hash;            /* hash function */
  ht_equal_func equal;          /* equal function */
  struct hlist_head *entries;   /* conflict list array */
} HashTable;

/**
 * Create a hash table,
 * which is automatically resized, although this incurs a performance penalty.
 */
HashTable *HashTable_Create(ht_hash_func hash, ht_equal_func equal);

/* Free a hash table */
void HashTable_Destroy(HashTable *table, ht_fini_func fini, void *arg);

/* Find a node with its key  */
HashNode *HashTable_Find(HashTable *table, void *key);

/* Remove a node from the hash table by its node */
int HashTable_Remove(HashTable *table, HashNode *hnode);

/* Insert a node to the hash table */
int HashTable_Insert(HashTable *table, HashNode *hnode);

/* Traverse all nodes in the hash table */
void HashTable_Traverse(HashTable *table, ht_visit_func visit, void *arg);

int HashTable_Initialize(HashTable *table,
                         ht_hash_func hash, ht_equal_func equal);

void HashTable_Finalize(HashTable *table, ht_fini_func fini, void *arg);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_HASHTABLE_H_ */
