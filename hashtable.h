
#ifndef _KOALA_HASHTABLE_H_
#define _KOALA_HASHTABLE_H_

#include "list.h"
#include "hash.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hlist_head HashList;

typedef struct hash_node {
  struct hlist_node hlink;    /* conflict list */
  struct list_head llink;     /* list node */
  uint32 hash;                /* hash value */
  void *key;                  /* hash key */
} HashNode;

#define Init_HashNode(hnode, k) do {  \
  init_hlist_node(&(hnode)->hlink);   \
  init_list_head(&(hnode)->llink);    \
  (hnode)->hash  = 0;                 \
  (hnode)->key   = k;                 \
} while (0)

#define HashNode_Unhashed(hnode) hlist_unhashed(&(hnode)->hlink)
#define HashList_ForEach(hnode, head) \
  hlist_for_each_entry(hnode, head, hlink)
#define HashList_Empty(head) hlist_empty(head)
typedef uint32 (*ht_hashfunc)(void *key);
typedef int (*ht_equalfunc)(void *key1, void *key2);
typedef void (*ht_visitfunc)(HashNode *hnode, void *arg);

typedef struct hashinfo {
  ht_hashfunc hash;
  ht_equalfunc equal;
} HashInfo;

#define Decl_HashInfo(hashinfo, h, e) \
  HashInfo hashinfo = {.hash = (h), .equal = (e)}

#define Init_HashInfo(info, h, e) do { \
  (info)->hash = (h); (info)->equal = (e); \
} while (0)

typedef struct hash_table {
  int prime;                  /* prime array index, internal used */
  int nodes;                  /* number of nodes in hash table */
  ht_hashfunc hash;           /* hash function */
  ht_equalfunc equal;         /* equal function */
  struct hlist_head *entries; /* conflict list array */
  struct list_head head;      /* list head */
} HashTable;

/**
 * Create a hash table,
 * which is automatically resized, although this incurs a performance penalty.
 */
HashTable *HashTable_New(HashInfo *hashinfo);

/* Free a hash table */
void HashTable_Free(HashTable *table, ht_visitfunc fn, void *arg);

/* Find a node with its key  */
HashNode *HashTable_Find(HashTable *table, void *key);

#define HashTable_FindObject(table, key, type) ({ \
  HashNode *node = HashTable_Find(table, key); \
  (node != NULL) ? container_of(node, type, hnode) : NULL; \
})

/* Remove a node from the hash table by its node */
int HashTable_Remove(HashTable *table, HashNode *hnode);

/* Insert a node to the hash table */
int HashTable_Insert(HashTable *table, HashNode *hnode);

/* Traverse all nodes in the hash table */
void HashTable_Traverse(HashTable *table, ht_visitfunc fn, void *arg);

int HashTable_Init(HashTable *table, HashInfo *hashinfo);

void HashTable_Fini(HashTable *table, ht_visitfunc fn, void *arg);

#define HashTable_NodeCount(table)  ((table)->nodes)
int HashTable_SlotSize(HashTable *table);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_HASHTABLE_H_ */
