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

#include "hashtable.h"
#include "mem.h"

#define LOAD_FACTOR 0.65
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

#define get_prime(idx) good_primes[idx]
#define get_entries_size(table) good_primes[((table)->prime_index)]

static uint32 get_proper_prime_index(HashTable *table)
{
  uint32 new_prime_index = table->prime_index;
  uint32 size = nr_elts(good_primes);
  while (new_prime_index < size) {
    if (table->nr_nodes < get_prime(new_prime_index) * LOAD_FACTOR)
      break;
    ++new_prime_index;
  }

  /* too many nodes? */
  if (new_prime_index >= size)
    new_prime_index = size - 1;

  return new_prime_index;
}

static struct hlist_head *new_entries(int nr_entries)
{
  struct hlist_head *entries;

  entries = Malloc(nr_entries * sizeof(struct hlist_head));
  if (!entries)
    return NULL;

  for (int i = 0; i < nr_entries; i++)
    init_hlist_head(entries + i);

  return entries;
}

static void free_entries(struct hlist_head *entries)
{
  Mfree(entries);
}

int HashTable_Init(HashTable *table, hashfunc hash, equalfunc equal)
{
  struct hlist_head *entries = new_entries(get_prime(DEFAULT_PRIME_INDEX));
  if (!entries)
    return -1;

  table->prime_index = DEFAULT_PRIME_INDEX;
  table->nr_nodes = 0;
  table->hash = hash;
  table->equal = equal;
  table->entries = entries;
  init_list_head(&table->head);
  return 0;
}

void HashTable_Fini(HashTable *table, visitfunc visit, void *arg)
{
  struct list_head *pos, *n;
  HashNode *hnode;
  list_for_each_safe(pos, n, &table->head) {
    hnode = container_of(pos, HashNode, llink);
    list_del(&hnode->llink);
    hlist_del(&hnode->hlink);
    if (visit)
      visit(hnode, arg);
  }

  free_entries(table->entries);
  table->prime_index = 0;
  table->nr_nodes = 0;
  table->entries = NULL;
}

HashTable *HashTable_New(hashfunc hash, equalfunc equal)
{
  HashTable *table = Malloc(sizeof(HashTable));
  if (!table)
    return NULL;

  if (HashTable_Init(table, hash, equal)) {
    Mfree(table);
    return NULL;
  }

  return table;
}

void HashTable_Free(HashTable *table, visitfunc fn, void *arg)
{
  if (!table)
    return;
  HashTable_Fini(table, fn, arg);
  Mfree(table);
}

HashNode *__HashTable_Find(HashTable *table, uint32 hash, void *key)
{
  uint32 idx = hash % get_entries_size(table);
  HashNode *hnode;
  struct hlist_node *node;
  hlist_for_each(node, table->entries + idx) {
    hnode = container_of(node, HashNode, hlink);
    if (table->equal(hnode->key, key))
      return hnode;
  }

  return NULL;
}

int HashTable_Remove(HashTable *table, HashNode *hnode)
{
  if (hlist_unhashed(&(hnode)->hlink))
    return -1;

  HashNode *temp = __HashTable_Find(table, hnode->hash, hnode->key);
  if (!temp || temp != hnode)
    return -1;

  hlist_del(&hnode->hlink);
  list_del(&hnode->llink);
  --table->nr_nodes;

  return 0;
}

static void htable_expand(HashTable *table, uint32 new_prime_index)
{
  HashNode *hnode;
  struct hlist_node *node;
  struct hlist_node *nxt;
  int nr_entries = get_entries_size(table);
  int new_nr_entries = get_prime(new_prime_index);
  struct hlist_head *entries = new_entries(new_nr_entries);
  if (!entries)
    return;

  uint32 index;
  for (int i = 0; i < nr_entries; i++) {
    hlist_for_each_safe(node, nxt, table->entries + i) {
      hlist_del(node);
      hnode = container_of(node, HashNode, hlink);
      index = hnode->hash % new_nr_entries;
      hlist_add(&hnode->hlink, entries + index);
    }
  }

  free_entries(table->entries);
  table->prime_index = new_prime_index;
  table->entries = entries;
}

static void htable_maybe_expand(HashTable *table)
{
  int new_prime_index;

  if (table->prime_index >= nr_elts(good_primes))
    return;

  if (table->nr_nodes < get_entries_size(table) * LOAD_FACTOR)
    return;

  new_prime_index = get_proper_prime_index(table);
  if (new_prime_index <= table->prime_index)
    return;

  htable_expand(table, new_prime_index);
}

int HashTable_Insert(HashTable *table, HashNode *hnode)
{
  if (!hlist_unhashed(&(hnode)->hlink))
    return -1;

  uint32 hash = hnode->hash = table->hash(hnode->key);
  if (__HashTable_Find(table, hash, hnode->key))
    return -1;

  htable_maybe_expand(table);

  uint32 index = hash % get_entries_size(table);

  hlist_add(&hnode->hlink, table->entries + index);
  list_add_tail(&hnode->llink, &table->head);
  ++table->nr_nodes;

  return 0;
}

void HashTable_Visit(HashTable *table, visitfunc visit, void *arg)
{
  if (table == NULL)
    return;

  struct list_head *pos, *nxt;
  HashNode *hnode;
  list_for_each_safe(pos, nxt, &table->head) {
    hnode = container_of(pos, HashNode, llink);
    visit(hnode, arg);
  }
}
