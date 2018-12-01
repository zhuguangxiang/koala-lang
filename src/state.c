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

#include "state.h"
#include "mem.h"
#include "hashfunc.h"
#include "tupleobject.h"
#include "image.h"
#include "log.h"
#include "env.h"

static void init_pkgnode(PkgNode *node, char *name, PkgNodeKind kind)
{
	Init_HashNode(&node->hnode, node);
	node->name = AtomString_New(name);
	node->parent = node;
	node->kind = kind;
}

static inline PkgNode *pkgnode_new(void)
{
	return mm_alloc(sizeof(PkgNode));
}

static inline void pkgnode_free(PkgNode *node)
{
	mm_free(node);
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

static void init_pkgnode_hashtable(HashTable *table)
{
	HashTable_Init(table, (ht_hashfunc)__n_hash, (ht_equalfunc)__n_equal);
}

GlobalState gState;

static void init_state(GlobalState *gs)
{
	Properties_Init(&gs->envs);
	init_pkgnode(&gs->root, "/", PATH_NODE);
	init_pkgnode_hashtable(&gs->pkgs);
	Vector_Init(&gs->vars);
}

static void init_lang_package(GlobalState *gs)
{
	Koala_Add_Package("koala", Package_New("lang"));
}

void Koala_Initialize(void)
{
	AtomString_Init();
	init_state(&gState);
	init_lang_package(&gState);
}

static void __n_freefunc(HashNode *hnode, void *arg)
{
	UNUSED_PARAMETER(arg);
	PkgNode *n = container_of(hnode, PkgNode, hnode);
	pkgnode_free(n);
}

void Koala_Finalize(void)
{
	HashTable_Fini(&gState.pkgs, __n_freefunc, NULL);
	Vector_Free(gState.root.children, NULL, NULL);
	Vector_Fini(&gState.vars, NULL, NULL);
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
	Log_Error("canot find node '%s'", name.str);
	return NULL;
}

static
PkgNode *find_or_create_pathnode(GlobalState *gs, PkgNode *parent, String name)
{
	PkgNode *node = find_node(gs, parent, name);
	if (node)
		return node;
	node = pkgnode_new();
	init_pkgnode(node, name.str, PATH_NODE);
	node->parent = parent;
	HashTable_Insert(&gs->pkgs, &node->hnode);
	Vector_Append(__pkgnode_get_vector(parent), node);
	Log_Debug("new path '%s'", name.str);
	return node;
}

static int add_leafnode(GlobalState *gs, PkgNode *parent, Package *pkg)
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
		if (!node)
			break;
		while (*path && *++path == '/');
	}

	if (!node || node->kind != LEAF_NODE) {
		Log_Error("cannot find leafnode '%s'", path);
		return NULL;
	}
	return node;
}

int Koala_Add_Package(char *path, Package *pkg)
{
	char *p;
	char c;
	String s;
	PkgNode *node = &gState.root;

	while (*path) {
		p = path;
		do {
			c = *++path;
		} while (c && c != '/');
		s = AtomString_New_NStr(p, path - p);
		node = find_or_create_pathnode(&gState, node, s);
		while (*path && *++path == '/');
	}

	return add_leafnode(&gState, node, pkg);
}

#define MAX_FILE_PATH_LEN 1024

static Package *load_package(char *path)
{
	char fullpath[MAX_FILE_PATH_LEN];
	Package *pkg = NULL;
	struct list_head *list;
	list = Properties_Get_List(&gState.envs, KOALA_PATH);
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
		Log_Debug("load package '%s' from image", path);
		pkg = Package_From_Image(image);
		KImage_Free(image);
	}
	return pkg;
}

static void run_init_func(Package *pkg)
{
	MemberDef *m = Package_Find_MemberDef(pkg, "__init__");
	assert(m && m->kind == MEMBER_CODE);
	//Koala_Run_Code(m->code, (Object *)pkg, NULL);
}

Object *Koala_Load_Package(char *path)
{
	Package *pkg = load_package(path);
	if (!pkg) {
		Log_Error("cannot load package '%s'", path);
		return NULL;
	}
	Koala_Add_Package(path, pkg);
	run_init_func(pkg);
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
	Properties_Put(&gState.envs, key, value);
}

int Koala_Run_File(char *path)
{
	return -1;
}

int Koala_Set_Value(Package *pkg, char *name, Object *value)
{
	int index = pkg->index;
	MemberDef *m = Package_Find_MemberDef(pkg, name);
	assert(m && m->kind == MEMBER_VAR);
	Log_Debug("set var '%s'(%d) in pkg '%s'(%d)", name, m->offset,
						pkg->name.str, index);
	Object *tuple = Vector_Get(&gState.vars, index);
	return Tuple_Set(tuple, m->offset, value);
}

Object *Koala_Get_Value(Package *pkg, char *name)
{
	int index = pkg->index;
	MemberDef *m = Package_Find_MemberDef(pkg, name);
	assert(m && m->kind == MEMBER_VAR);
	Log_Debug("get var '%s'(%d) in pkg '%s'(%d)", name, m->offset,
						pkg->name.str, index);
	Object *tuple = Vector_Get(&gState.vars, index);
	return Tuple_Get(tuple, m->offset);
}

static void show_pkgnode(PkgNode *node, int depth, int y)
{
	#define LAST_ONE(vec) ((i + 1) == Vector_Size(vec))
	int bit;
	int t = y;
	int i = depth;
	while (i > 1) {
		if (t & 1)
			printf("    ");
		else
			printf("│   ");
		t = t >> 1;
		--i;
	}
	if (i == 1) {
		if (t & 1)
			printf("└── ");
		else
			printf("├── ");
	}
	printf("%s\n", node->name.str);
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
	show_pkgnode(&gState.root, 0, 0);
}
