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

#include "globalstate.h"
#include "eval.h"
#include "mem.h"
#include "hashfunc.h"
#include "tupleobject.h"
#include "stringobject.h"
#include "intobject.h"
#include "log.h"

LOGGER(0)

static void init_pkgnode(PkgNode *node, char *name, PkgNodeKind kind)
{
  Init_HashNode(&node->hnode, node);
  node->name = AtomString_New(name);
  node->parent = node;
  node->kind = kind;
}

static void fini_pkgnode(PkgNode *node)
{
  if (node->kind == PATH_NODE) {
    Vector_Free_Self(node->children);
  } else {
    assert(node->kind == LEAF_NODE);
    OB_DECREF(node->pkg);
  }
}

static inline PkgNode *pkgnode_new(void)
{
  return Malloc(sizeof(PkgNode));
}

static inline void pkgnode_free(PkgNode *node)
{
  fini_pkgnode(node);
  Mfree(node);
}

static uint32 __n_hash(PkgNode *n)
{
  return hash_string(n->name.str);
}

static int __n_equal(PkgNode *n1, PkgNode *n2)
{
  if (n1->parent != n2->parent)
    return 0;
  return !strcmp(n1->name.str, n2->name.str);
}

GlobalState gState;

static void Init_GlobalState(void)
{
  AtomString_Init();
  Init_TypeDesc();

  GlobalState *gs = &gState;
  Properties_Init(&gs->props);
  init_pkgnode(&gs->root, "/", PATH_NODE);
  HashTable_Init(&gs->pkgs, (hashfunc)__n_hash, (equalfunc)__n_equal);
  Vector_Init(&gs->vars);
  init_list_head(&gs->kslist);
}

static void __free_pkgnode_func(HashNode *hnode, void *arg)
{
  UNUSED_PARAMETER(arg);
  PkgNode *n = container_of(hnode, PkgNode, hnode);
  pkgnode_free(n);
}

static void __free_tuples_func(void *item, void *arg)
{
  UNUSED_PARAMETER(arg);
  Tuple_Free(item);
}

static void Fini_GlobalState(void)
{
  GlobalState *gs = &gState;
  struct list_head *p, *n;
  list_for_each_safe(p, n, &gs->kslist) {
    //FIXME: free KoalaState
  }
  Vector_Fini(&gs->vars, __free_tuples_func, NULL);
  HashTable_Fini(&gs->pkgs, __free_pkgnode_func, NULL);
  fini_pkgnode(&gs->root);
  Properties_Fini(&gs->props);

  Fini_TypeDesc();
  AtomString_Fini();
}

static void Init_Lang_Package(void)
{
  Init_String_Klass();
  Init_Integer_Klass();
  Init_Tuple_Klass();
  Init_Package_Klass();
  PackageObject *pkg = Package_New("lang");
  Package_Add_Class(pkg, &String_Klass);
  Package_Add_Class(pkg, &Int_Klass);
  Package_Add_Class(pkg, &Tuple_Klass);
  Package_Add_Class(pkg, &Package_Klass);
  Koala_Add_Package("/", pkg);
}

static void Fini_Lang_Package(void)
{
  Fini_Package_Klass();
  Fini_Tuple_Klass();
  Fini_Integer_Klass();
  Fini_String_Klass();
}

void Koala_Initialize(void)
{
  Init_GlobalState();
  Init_Lang_Package();
  Koala_Show_Packages();
}

void Koala_Finalize(void)
{
  Fini_Lang_Package();
  Fini_GlobalState();
}

static Vector *__pkgnode_get_vector(PkgNode *node)
{
  if (!node->children)
    node->children = Vector_New();
  return node->children;
}

static PkgNode *find_node(GlobalState *gs, PkgNode *parent, String name)
{
  PkgNode k = {.parent = parent, .name = name};
  HashNode *hnode = HashTable_Find(&gs->pkgs, &k);
  if (hnode) {
    PkgNode *node = container_of(hnode, PkgNode, hnode);
    Log_Debug("found node '%s'", name.str);
    return node;
  }
  Log_Debug("cannot find node '%s'", name.str);
  return NULL;
}

static PkgNode *add_node(GlobalState *gs, PkgNode *parent, String name)
{
  PkgNode *node = pkgnode_new();
  init_pkgnode(node, name.str, PATH_NODE);
  node->parent = parent;
  HashTable_Insert(&gs->pkgs, &node->hnode);
  Vector_Append(__pkgnode_get_vector(parent), node);
  Log_Debug("new path '%s'", name.str);
  return node;
}

static int add_leafnode(GlobalState *gs, PkgNode *parent, PackageObject *pkg)
{
  assert(parent->kind == PATH_NODE);
  PkgNode *node = pkgnode_new();
  init_pkgnode(node, pkg->name.str, LEAF_NODE);
  node->parent = parent;
  node->pkg = pkg;
  Log_Debug("new package '%s'", pkg->name.str);
  Vector_Append(__pkgnode_get_vector(parent), node);
  pkg->index = Vector_Size(&gs->vars);
  Vector_Append(&gs->vars, Tuple_New(pkg->varcnt));
  return HashTable_Insert(&gs->pkgs, &node->hnode);
}

static PkgNode *find_leafnode(GlobalState *gs, char *path)
{
  char *p;
  char c;
  String s;
  PkgNode *node = &gs->root;

  while (*path) {
    p = path;
    do {
      c = *++path;
    } while (c && c != '/');
    s = AtomString_New_NStr(p, path - p);
    node = find_node(gs, node, s);
    if (node == NULL)
      break;
    while (*path && *++path == '/');
  }

  if (!node || node->kind != LEAF_NODE)
    return NULL;
  return node;
}

int Koala_Add_Package(char *path, PackageObject *pkg)
{
  char *p;
  char c;
  String s;
  PkgNode *parent = &gState.root;
  PkgNode *node;

  while (*path && *path != '/') {
    p = path;
    do {
      c = *++path;
    } while (c && c != '/');
    s = AtomString_New_NStr(p, path - p);
    node = find_node(&gState, parent, s);
    if (node == NULL)
      node = add_node(&gState, parent, s);
    parent = node;
    while (*path && *++path == '/');
  }

  return add_leafnode(&gState, parent, pkg);
}

#define MAX_FILE_PATH_LEN 1024

static PackageObject *load_package(char *path)
{
  char fullpath[MAX_FILE_PATH_LEN];
  PackageObject *pkg = NULL;
  struct list_head *list;
  list = Properties_Get_List(&gState.props, KOALA_PATH);
  assert(list);
  Property *property;
  struct list_head *pos;
  list_for_each(pos, list) {
    property = container_of(pos, Property, link);
    snprintf(fullpath, MAX_FILE_PATH_LEN - 1, "%s/%s.klc",
             Property_Value(property), path);
    KImage *image = KImage_Read_File(fullpath);
    if (!image)
      continue;
    Log_Debug("load package '%s' from '%s'", path, fullpath);
    char *name = strrchr(path, '/');
    if (name == NULL)
      name = path;
    pkg = Package_From_Image(image, name);
    KImage_Free(image);
  }
  return pkg;
}

Object *Koala_Load_Package(char *path)
{
  PackageObject *pkg = load_package(path);
  if (!pkg) {
    Log_Error("cannot load package '%s'", path);
    return NULL;
  }
  char *lastslash = strrchr(path, '/');
  if (lastslash != NULL)
    path = strndup(path, lastslash - path);
  else
    path = "";
  Koala_Add_Package(path, pkg);
  return (Object *)pkg;
}

Object *Koala_Get_Package(char *path)
{
  PkgNode *node = find_leafnode(&gState, path);
  if (node)
    return (Object *)node->pkg;

  return Koala_Load_Package(path);
}

void Koala_Add_Property(char *key, char *value)
{
  Properties_Put(&gState.props, key, value);
}

static int run_function(KoalaState *ks, Object *ob, Object *args, char *name)
{
  MemberDef *m = Package_Find_MemberDef((PackageObject *)ob, "__init__");
  if (!m || m->kind != MEMBER_CODE) {
    fprintf(stderr, "No '%s' function in '%s'.", name, Package_Name(ob));
    return -1;
  }
  return Koala_Run_Code(ks, m->code, ob, args);
}

int Koala_Run_File(KoalaState *ks, char *path, Vector *args)
{
  /* find or load package */
  Object *pkg = Koala_Get_Package(path);
  if (pkg == NULL) {
    fprintf(stderr, "error: cannot open '%s' package\n", path);
    return -1;
  }

  /* initial package's variables */
  run_function(ks, pkg, NULL, "__init__");

  /* run 'Main' function in the package */
  return run_function(ks, pkg, NULL, "Main");
}

int Koala_Set_Value(PackageObject *pkg, char *name, Object *value)
{
  int index = pkg->index;
  MemberDef *m = Package_Find_MemberDef(pkg, name);
  assert(m && m->kind == MEMBER_VAR);
  Log_Debug("set var '%s'(%d) in pkg '%s'(%d)", name, m->offset,
            pkg->name.str, index);
  Object *tuple = Vector_Get(&gState.vars, index);
  OB_INCREF(value);
  return Tuple_Set(tuple, m->offset, value);
}

Object *Koala_Get_Value(PackageObject *pkg, char *name)
{
  int index = pkg->index;
  MemberDef *m = Package_Find_MemberDef(pkg, name);
  assert(m && m->kind == MEMBER_VAR);
  Log_Debug("get var '%s'(%d) in pkg '%s'(%d)", name, m->offset,
            pkg->name.str, index);
  Object *tuple = Vector_Get(&gState.vars, index);
  return Tuple_Get(tuple, m->offset);
}

static void show_pathes(void)
{
  struct list_head *list;
  list = Properties_Get_List(&gState.props, KOALA_PATH);
  if (list) {
    Property *property;
    struct list_head *pos;
    list_for_each(pos, list) {
      property = container_of(pos, Property, link);
      Log_Printf("%s\n", Property_Value(property));
    }
  }
}

static void show_pkgnode(PkgNode *node, int depth, int y)
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
  if (node != &gState.root)
    Log_Printf("%s\n", node->name.str);
  else
    Log_Puts(".");
  if (node->kind == PATH_NODE) {
    PkgNode *item;
    Vector_ForEach(item, node->children) {
      bit = LAST_ONE(node->children) << (depth);
      show_pkgnode(item, depth + 1, bit + y);
    }
  }
}

void Koala_Show_Packages(void)
{
  show_pathes();
  show_pkgnode(&gState.root, 0, 0);
}
