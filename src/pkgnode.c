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

#include "pkgnode.h"
#include "vector.h"
#include "atomstring.h"
#include "mem.h"
#include "log.h"

LOGGER(0)

static void init_pkgnode(PkgNode *node, char *name)
{
  Init_HashNode(&node->hnode, node);
  node->name = name;
  node->parent = node;
  node->leaf = 0;
  node->data = NULL;
}

static void fini_pkgnode(PkgNode *node, fini_leafnode_func func, void *arg)
{
  if (node->leaf) {
    if (func != NULL)
      func(node->data, arg);
  } else {
    Vector_Free_Self(node->data);
  }
}

static PkgNode *new_pkgnode(char *name, PkgNode *parent)
{
  PkgNode *node = Malloc(sizeof(PkgNode));
  init_pkgnode(node, name);
  node->parent = parent;
  return node;
}

static void free_pkgnode(PkgNode *node, fini_leafnode_func func, void *arg)
{
  fini_pkgnode(node, func, arg);
  Mfree(node);
}

static uint32 node_hash_func(void *obj)
{
  PkgNode *node = obj;
  return hash_string(node->name);
}

static int node_equal_func(void *obj1, void *obj2)
{
  PkgNode *node1 = obj1;
  PkgNode *node2 = obj2;

  if (node1->parent != node2->parent)
    return 0;
  if (node1->name == node2->name)
    return 1;
  return !strcmp(node1->name, node2->name);
}

struct node_fini_param {
  fini_leafnode_func func;
  void *arg;
};

static void node_fini_func(HashNode *hnode, void *arg)
{
  struct node_fini_param *param = arg;
  PkgNode *node = HashNode_Entry(hnode, PkgNode, hnode);
  free_pkgnode(node, param->func, param->arg);
}

void Init_PkgState(PkgState *ps)
{
  HashTable_Init(&ps->pkgtbl, node_hash_func, node_equal_func);
  String name = AtomString_New("/");
  init_pkgnode(&ps->root, name.str);
  ps->root.data = Vector_New();
}

void Fini_PkgState(PkgState *ps, fini_leafnode_func func, void *arg)
{
  struct node_fini_param param = {func, arg};
  HashTable_Fini(&ps->pkgtbl, node_fini_func, &param);
  fini_pkgnode(&ps->root, NULL, NULL);
}

static PkgNode *find_node(PkgState *ps, char *name, PkgNode *parent)
{
  PkgNode key = {.name = name, .parent = parent};
  HashNode *hnode = HashTable_Find(&ps->pkgtbl, &key);
  if (hnode != NULL) {
    Log_Debug("node '%s' exist", name);
    return HashNode_Entry(hnode, PkgNode, hnode);
  } else {
    return NULL;
  }
}

static PkgNode *add_node(PkgState *ps, char *name, PkgNode *parent)
{
  PkgNode *node = new_pkgnode(name, parent);
  HashTable_Insert(&ps->pkgtbl, &node->hnode);
  Log_Debug("add node '%s' successfully", name);
  return node;
}

PkgNode *Add_PkgNode(PkgState *ps, char *path, void *data)
{
  char ch;
  char *p;
  char *s;
  int len = 0;
  PkgNode *parent = &ps->root;
  PkgNode *node;

  while (1) {
    p = path;
    do {
      ch = *++path;
    } while (ch && ch != '/');
    len = path - p;

    /* remove trailing slashes? */
    while (*path == '/')
      path++;

    /* last one */
    if (!*path)
      break;

    s = AtomString_New_NStr(p, len).str;
    node = find_node(ps, s, parent);
    if (node == NULL) {
      node = add_node(ps, s, parent);
      node->data = Vector_New();
      assert(!parent->leaf);
      Vector_Append((Vector *)parent->data, node);
    }
    parent = node;
  }

  s = AtomString_New_NStr(p, len).str;
  node = find_node(ps, s, parent);
  if (node == NULL) {
    node = add_node(ps, s, parent);
    node->leaf = 1;
    node->data = data;
    assert(!parent->leaf);
    Vector_Append((Vector *)parent->data, node);
    return node;
  }

  return NULL;
}

PkgNode *Find_PkgNode(PkgState *ps, char *path)
{
  char ch;
  char *p;
  char *s;
  int len = 0;
  PkgNode *parent = &ps->root;
  PkgNode *node;
  while (1) {
    p = path;
    do {
      ch = *++path;
    } while (ch && ch != '/');
    len = path - p;
    s = AtomString_New_NStr(p, len).str;
    node = find_node(ps, s, parent);
    if (node == NULL)
      break;
    while (*path == '/')
      path++;
    if (!*path)
      break;
    parent = node;
  }
  return node;
}

void Show_PkgNode(PkgState *ps, PkgNode *node, int depth, int y)
{
  #define LAST_ONE(vec) ((i + 1) == Vector_Size(vec))
  int bit;
  int t = y;
  int i = depth;
  while (i > 1) {
    if (t & 1)
      Log_Printf("    ");
    else
      Log_Printf("│   ");
    t = t >> 1;
    --i;
  }
  if (i == 1) {
    if (t & 1)
      Log_Printf("└── ");
    else
      Log_Printf("├── ");
  }
  if (node != &ps->root)
    Log_Printf("%s\n", node->name);
  else
    Log_Puts(".");
  if (!node->leaf) {
    PkgNode *item;
    Vector_ForEach(item, (Vector *)node->data) {
      bit = LAST_ONE((Vector *)node->data) << (depth);
      Show_PkgNode(ps, item, depth + 1, bit + y);
    }
  }
}
