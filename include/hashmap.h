/*
MIT License

Copyright (c) 2018 James, https://github.com/zhuguangxiang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _KOALA_HASHMAP_H_
#define _KOALA_HASHMAP_H_

#include "memory.h"
#include "iterator.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Generic implementation of hash-based key-value mappings.
 *
 * An example of using hashmap as hashset.
 *
 * struct string {
 *   struct hashmap_entry entry;
 *   int length;
 *   char *value;
 * };
 *
 * struct hashmap map;
 * int string_compare(void *k1, void *k2)
 * {
 *   struct string *s1 = k1;
 *   struct string *s2 = k2;
 *   if (s1->length != s2->length || strcmp(s1->value, s2->value))
 *     return 0;
 *   else
 *     return 1;
 * }
 *
 * int main(int argc, char *argv[])
 * {
 *   hashmap_init(&map, string_compare);
 *   struct string *s = malloc(sizeof(*s));
 *   s->length = strlen("some string");
 *   s->value = "some string";
 *   hashmap_entry_init(&s->entry, strhash(s->value), s, s);
 *   hashmap_add(&map, &s->entry);
 * }
 */

/* a hash map entry is in the hash table. */
struct hashmap_entry {
  /* pointer to the next entry in collision list,
   * if multiple entries map to the same slots.
   */
  struct hashmap_entry *next;
  /* entry's hash code */
  unsigned int hash;
};

/*
 * Compare function to test two keys for equality.
 *
 * k1 - The key of struct hashmap_entry.
 * k2 - The key of struct hashmap_entry.
 *
 * Returns 0 if the twoe entries are equal.
 */
typedef int (*hashmap_cmp_fn)(void *k1, void *k2);

/* a hash map structure. */
struct hashmap {
  /* collision list array */
  struct hashmap_entry **entries;
  /* entries array size */
  int size;
  /* comparison function */
  hashmap_cmp_fn cmpfn;
  /* total number of entries */
  int nr_entries;
  /* expand entries array point */
  int grow_at;
  /* shrink entries array point */
  int shrink_at;
};

/*
 * Initialize a hash map.
 *
 * self  - The hashmap to be initialized.
 * equal - The equal function to compare the two entries for equality.
 *
 * Returns nothing.
 */
void hashmap_init(struct hashmap *self, hashmap_cmp_fn equal);

/*
 * Free function for hashmap entry, when the hashmap is destroyed.
 *
 * e    - The entry to be freed.
 * data - The private data passed to free function.
 *
 * Returns nothing.
 */
typedef void (*hashmap_free_fn)(struct hashmap_entry *e, void *data);

/*
 * Destroy the hashmap and free its allocated memory.
 *
 * self    - The hashmap to be destroyed.
 * free_fn - The free function called via every hashmap_entry.
 * data    - The private data passed to free function.
 */
void hashmap_free(struct hashmap *self, hashmap_free_fn free_fn, void *data);

/*
 * Determine if the key is contained within the hashmap.
 *
 * self - The hashmap to query.
 * key  - The key to find.
 *
 * Returns 1 if the key is stored in the map.
 */
int hashmap_contains(struct hashmap *self, struct hashmap_entry *key);

/*
 * Retrieve the hashmap entry for the specified hash code.
 *
 * self - The hashmap from which to retrieve the hashmap entry.
 * key  - The key with hash code to look up in the map.
 *
 * Returns the hashmap entry or null if not found.
 */
struct hashmap_entry *hashmap_get(struct hashmap *self,
                                  struct hashmap_entry *key);

/*
 * Add a hashmap entry. If the hashmap contains duplicate entries, it will
 * return -1 failure.
 *
 * self - The hashmap to which to add the hashmap entry.
 * e    - The entry to be added.
 *
 * Returns -1 failure, 0 successful.
 */
int hashmap_add(struct hashmap *self, struct hashmap_entry *e);

/*
 * Add or replace a hashmap entry. If the hashmap contains duplicate entries,
 * the old entry will be replaced and returned.
 *
 * self - The hashmap to which to add the hashmap entry.
 * e    - The entry to be added or replaced.
 *
 * Returns the replaced entry or null if no duplicated entry
 */
struct hashmap_entry *hashmap_put(struct hashmap *self,
                                  struct hashmap_entry *key);

/*
 * Removes a hashmap entry matching the specified key.
 *
 * self - The hashmap from which to remove the hashmap entry.
 * key  - The key with hash code to look up in the map.
 *
 * Returns the hashmap entry or null if not found.
 */
struct hashmap_entry *hashmap_remove(struct hashmap *self,
                                     struct hashmap_entry *key);

/*
 * Iterator callback function for hashmap iteration.
 * See iterator.h.
 */
int hashmap_iter_next(struct iterator *iter);

/*
 * Declare an iterator of the hashmap. Deletion is not safe.
 *
 * name    - The name of the hashmap iterator
 * hashmap - The container to iterate.
 */
#define HASHMAP_ITERATOR(name, hashmap) \
  ITERATOR(name, hashmap, hashmap_iter_next)

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_HASHMAP_H_ */
