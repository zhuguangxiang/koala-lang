/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _KOALA_HASHTABLE_H_
#define _KOALA_HASHTABLE_H_

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hash_node {
  /* list node */
  struct list_head llink;
  /* conflict list */
  struct hlist_node hlink;
  /* hash value */
  uint32 hash;
  /* hash key */
  void *key;
  /* hash object */
  void *object;
} HashNode;

#define HashNode_Entry(ptr, type, member) \
  container_of(ptr, type, member)

#define Init_HashNode(hnode, k) \
do { \
  init_list_head(&(hnode)->llink); \
  init_hlist_node(&(hnode)->hlink); \
  (hnode)->hash = 0; \
  (hnode)->key = k; \
} while (0)

#define Remove_HashNode(hnode) \
do { \
  list_del(&(hnode)->llink); \
  hlist_del(&(hnode)->hlink); \
} while (0)

typedef uint32 (*hashfunc)(void *);
typedef int (*equalfunc)(void *, void *);
typedef void (*visitfunc)(HashNode *, void *);

typedef struct hash_table {
  /* prime array index, internal used */
  int prime_index;
  /* number of nodes in hash table */
  int nr_nodes;
  /* hash function */
  hashfunc hash;
  /* equal function */
  equalfunc equal;
  /* conflict list array */
  struct hlist_head *entries;
  /* list, by added order */
  struct list_head head;
} HashTable;

/*
 * Create a hash table,
 * which is automatically resized, although this incurs a performance penalty.
 */
HashTable *HashTable_New(hashfunc hash, equalfunc equal);
/* Free a hash table */
void HashTable_Free(HashTable *table, visitfunc fn, void *arg);
/* Find a node with its key  */
HashNode *__HashTable_Find(HashTable *table, uint32 hash, void *key);
#define HashTable_Find(table, key) \
  __HashTable_Find(table, (table)->hash(key), key)
/* Remove a node from the hash table by its node */
int HashTable_Remove(HashTable *table, HashNode *hnode);
/* Insert a node to the hash table */
int HashTable_Insert(HashTable *table, HashNode *hnode);
/* initial a hash table */
int HashTable_Init(HashTable *table, hashfunc hash, equalfunc equal);
/* finalize a hash table */
void HashTable_Fini(HashTable *table, visitfunc visit, void *arg);
/* get number of nodes in the hash table */
#define HashTable_Count_Nodes(table) ((table != NULL) ? (table)->nr_nodes : 0)
void HashTable_Visit(HashTable *table, visitfunc visit,  void *arg);
#define HashTable_ForEach(hnode, table) \
  list_for_each_entry(hnode, &(table)->head, llink)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_HASHTABLE_H_ */
