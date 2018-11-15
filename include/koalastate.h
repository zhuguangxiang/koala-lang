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

#ifndef _KOALA_STATE_H_
#define _KOALA_STATE_H_

#include "atomstring.h"
#include "package.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum pkgnodekind {
  PATH_NODE,
  LEAF_NODE
} PkgNodeKind;

/* packege path node, like linux's inode */
typedef struct pkgnode {
  HashNode hnode;
  /* the node's name, no need free its memory */
  String name;
  /* the node's parent node */
  struct pkgnode *parent;
  /* the node is pathnode or leafnode */
  PkgNodeKind kind;
  /* see PkgNodeKind */
  union {
    Vector *children;
    Package *pkg;
  };
} PkgNode;

typedef struct koalastate {
  /* root package node */
  PkgNode root;
  /* package hash table */
  HashTable pkgs;
  /* global variables' pool, one slot per package, stored in ->index */
  Vector gvars;
} KoalaState;

void Koala_Initialize(void);
void Koala_Finalize(void);
int Koala_Set_Object(Package *pkg, char *name, Object *newobj);
Object *Koala_Get_Object(Package *pkg, char *name);
int Koala_Install_Package(char *path, Package *pkg);
void Koala_Uninstall_Package(char *path);
void Koala_Show_Packages(void);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_STATE_H_ */
