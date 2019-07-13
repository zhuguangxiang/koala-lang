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

#include "hashmap.h"
#include "vector.h"
#include "atom.h"
#include "debug.h"

char **path_toarr(char *path)
{
  VECTOR_PTR(strs);
  char *p;
  char *s;
  char ch;
  int len;
  char **arr;

  while (*path) {
    p = path;
    while ((ch = *path) && ch != '/')
      path++;

    len = path - p;
    if (len > 0) {
      s = atom_string(p, len);
      vector_push_back(&strs, &s);
    }

    /* remove trailing slashes */
    while (*path && *path == '/')
      path++;
  }

  arr = vector_toarr(&strs);
  vector_free(&strs, NULL, NULL);
  return arr;
}

struct node {
  struct hashmap_entry entry;
  /* node name */
  char *name;
  /* parent node */
  struct node *parent;
  /* leaf node */
  int leaf;
  /* leaf data or directory */
  void *data;
};

static struct hashmap nodetbl;
static struct node root;
static VECTOR_PTR(dir);

static int __node_cmp_cb__(void *k1, void *k2)
{
  struct node *n1 = k1;
  struct node *n2 = k2;
  if (n1->parent != n2->parent)
    return -1;
  if (n1->name == n2->name)
    return 0;
  return strcmp(n1->name, n2->name);
}

static void __node_free_cb__(void *entry, void *data)
{
  kfree(entry);
}

void node_initialize(void)
{
  hashmap_init(&nodetbl, __node_cmp_cb__);
  root.name = "/";
  root.parent = &root;
  root.data = &dir;
  hashmap_entry_init(&root, strhash(root.name));
}

void node_destroy(void)
{
  hashmap_free(&nodetbl, __node_free_cb__, NULL);
  vector_free(&dir, NULL, NULL);
}

static inline struct node *find_node(char *name, struct node *parent)
{
  struct node key = {.name = name, .parent = parent};
  hashmap_entry_init(&key, strhash(name));
  return hashmap_get(&nodetbl, &key);
}

static struct node *__add_dirnode(char *name, struct node *parent)
{
  struct vector *vec;
  struct node *node = kmalloc(sizeof(*node) + sizeof(*vec));
  hashmap_entry_init(node, strhash(name));
  node->name = name;
  node->parent = parent;
  vec = (struct vector *)(node + 1);
  vector_init(vec, 16);
  node->data = vec;
  hashmap_add(&nodetbl, node);
  return node;
}

static struct node *__add_leafnode(char *name, struct node *parent)
{
  struct node *node = kmalloc(sizeof(*node));
  hashmap_entry_init(node, strhash(name));
  node->name = name;
  node->parent = parent;
  node->leaf = 1;
  hashmap_add(&nodetbl, node);
  return node;
}

void add_leaf(char *pathes[], void *data)
{
  struct node *parent = &root;
  struct node *node;
  char **s;

  for (s = pathes; *(s + 1); s++) {
    node = find_node(*s, parent);
    if (!node) {
      /* new directory node */
      debug("new node '%s'.", *s);
      node = __add_dirnode(*s, parent);
    }
    parent = node;
  }

  /* new leaf data node */
  node = find_node(*s, parent);
  if (node) {
    assert(node->leaf);
    debug("leaf '%s' exist.", *s);
  } else {
    debug("new leaf '%s'.", *s);
    node = __add_leafnode(*s, parent);
    node->data = data;
  }
}

void *get_leaf(char *pathes[])
{
  struct node *parent = &root;
  struct node *node;
  char **s;

  for (s = pathes; *s; s++) {
    node = find_node(*s, parent);
    if (!node) {
      error("cannot find node '%s'.", *s);
      return NULL;
    }
    parent = node;
  }

  assert(node->leaf);
  return node->data;
}
