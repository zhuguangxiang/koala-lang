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

#ifndef _KOALA_PKGNODE_H_
#define _KOALA_PKGNODE_H_

#include "hashtable.h"

#ifdef __cplusplus
extern "C" {
#endif

/* packege path node, like linux's inode */
typedef struct pkgnode {
  HashNode hnode;
  /* the node's name, it's atom string */
  char *name;
  /* the node's parent node */
  struct pkgnode *parent;
  /* leaf node flag */
  int leaf;
  /* node's payload */
  void *data;
} PkgNode;

typedef struct pkgstate {
  /* package hash table */
  HashTable pkgtbl;
  /* root package node */
  PkgNode root;
} PkgState;

typedef void (*leafnode_func)(void *data, void *arg);
void Init_PkgState(PkgState *ps);
void Fini_PkgState(PkgState *ps, leafnode_func func, void *arg);
void Show_PkgNode(PkgState *ps, PkgNode *node, int depth, int y);
#define Show_PkgState(ps) \
  Show_PkgNode((ps), &(ps)->root, 0, 0)
PkgNode *Add_PkgNode(PkgState *ps, char *path, void *data);
PkgNode *Find_PkgNode(PkgState *ps, char *path);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_PKGNODE_H_ */
