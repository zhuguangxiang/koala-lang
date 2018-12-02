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

#include "package.h"
#include "tupleobject.h"
#include "stringobject.h"
#include "intobject.h"
#include "mem.h"
#include "log.h"

Package *Package_New(char *name)
{
	Package *pkg = mm_alloc(sizeof(Package));
	Init_Object_Head(pkg, &Package_Klass);
	pkg->name = AtomString_New(name);
	return pkg;
}

static void pkg_freefunc(HashNode *hnode, void *arg)
{
	MemberDef *m = container_of(hnode, MemberDef, hnode);
	MemberDef_Free(m);
}

void Package_Free(Package *pkg)
{
	HashTable_Free(pkg->table, pkg_freefunc, NULL);
	Tuple_Free(pkg->consts);
	mm_free(pkg);
}

static HashTable *__get_table(Package *pkg)
{
	if (!pkg->table)
		pkg->table = MemberDef_Build_HashTable();
	return pkg->table;
}

int Package_Add_Var(Package *pkg, char *name, TypeDesc *desc, int k)
{
	MemberDef *m = MemberDef_Var_New(__get_table(pkg), name, desc, k);
	if (m) {
		m->offset = pkg->varcnt++;
		return 0;
	}
	return -1;
}

int Package_Add_Func(Package *pkg, char *name, Object *code)
{
	MemberDef *m = MemberDef_Code_New(__get_table(pkg), name, code);
	if (m) {
		m->code = code;
		if (CODE_IS_K(code)) {
			((CodeObject *)code)->kf.consts = pkg->consts;
		}
		return 0;
	}
	return -1;
}

int Package_Add_Klass(Package *pkg, Klass *klazz, int trait)
{
	MemberDef *m;
	if (trait)
		m = MemberDef_Trait_New(klazz->name);
	else
		m = MemberDef_Class_New(klazz->name);
	int res = HashTable_Insert(__get_table(pkg), &m->hnode);
	if (res) {
		MemberDef_Free(m);
		return -1;
	}
	m->klazz = klazz;
	klazz->pkg = pkg;
	klazz->consts = pkg->consts;
	return 0;
}

MemberDef *Package_Find_MemberDef(Package *pkg, char *name)
{
	MemberDef *m = MemberDef_Find(__get_table(pkg), name);
	if (!m) {
		Log_Error("cannot find symbol '%s'", name);
	}
	return m;
}

int Package_Add_CFunctions(Package *pkg, FuncDef *funcs)
{
	int res;
	FuncDef *f = funcs;
	Object *code;
	while (f->name) {
		code = FuncDef_Build_Code(f);
		res = Package_Add_Func(pkg, f->name, code);
		assert(!res);
		++f;
	}
	return 0;
}

static void load_consts(Package *pkg, AtomTable *table)
{
	Vector *vec = &table->items[ITEM_CONST];
	Object *tuple = Tuple_New(Vector_Size(vec));
	pkg->consts = tuple;
	Object *ob;
	ConstItem *item;
	Vector_ForEach(item, vec) {
		switch (item->type) {
			case CONST_INT: {
				ob = Integer_New(item->ival);
				break;
			}
			case CONST_FLOAT: {
				//ob = Float_New(item->fval);
				break;
			}
			case CONST_BOOL: {
				//ob = Bool_New(item->bval);
				break;
			}
			case CONST_STRING: {
				StringItem *s = AtomTable_Get(table, ITEM_STRING, item->index);
				ob = String_New(s->data);
				break;
			}
			default: {
				assert(0);
				break;
			}
		}
		Tuple_Set(tuple, i, ob);
	}
}

static void load_variables(Package *pkg, AtomTable *table)
{
	int sz = AtomTable_Size(table, ITEM_VAR);
	VarItem *var;
	StringItem *id;
	TypeItem *type;
	TypeDesc *desc;

	for (int i = 0; i < sz; i++) {
		var = AtomTable_Get(table, ITEM_VAR, i);
		id = AtomTable_Get(table, ITEM_STRING, var->nameindex);
		type = AtomTable_Get(table, ITEM_TYPE, var->typeindex);
		desc = TypeItem_To_TypeDesc(type, table);
		Package_Add_Var(pkg, id->data, desc, var->access & ACCESS_CONST);
	}
}

typedef struct locvarindex {
	int index;
	Object *func;
} LocVarIndex;

static Object *get_func(LocVarIndex *indexes, int size, int index)
{
	LocVarIndex *locvaridx;
	for (int i = 0; i < size; i++) {
		locvaridx = &indexes[i];
		if (locvaridx->index == index)
			return locvaridx->func;
	}
	return NULL;
}

static void load_locvar(LocVarItem *locvar, AtomTable *table, Object *code)
{
	StringItem *stritem = AtomTable_Get(table, ITEM_STRING, locvar->nameindex);;
	TypeItem *typeitem = AtomTable_Get(table, ITEM_TYPE, locvar->typeindex);
	char *name = stritem->data;
	TypeDesc *desc = TypeItem_To_TypeDesc(typeitem, table);
	KCode_Add_LocVar(code, name, desc, locvar->pos);
}

static void load_functions(Package *pkg, AtomTable *table)
{
	int sz = AtomTable_Size(table, ITEM_FUNC);
	FuncItem *funcitem;
	StringItem *id;
	ProtoItem *protoitem;
	TypeDesc *proto;
	CodeItem *codeitem;
	Object *func;
	LocVarIndex indexes[sz];

	/* load functions */
	for (int i = 0; i < sz; i++) {
		funcitem = AtomTable_Get(table, ITEM_FUNC, i);
		id = AtomTable_Get(table, ITEM_STRING, funcitem->nameindex);
		protoitem = AtomTable_Get(table, ITEM_PROTO, funcitem->protoindex);
		proto = ProtoItem_To_TypeDesc(protoitem, table);
		codeitem = AtomTable_Get(table, ITEM_CODE, funcitem->codeindex);
		func = KCode_New(codeitem->codes, codeitem->size, proto);
		Package_Add_Func(pkg, id->data, func);
		indexes[i].index = i;
		indexes[i].func = func;
	}

	/* load local variables */
	sz = AtomTable_Size(table, ITEM_LOCVAR);
	LocVarItem *locvar;
	for (int i = 0; i < sz; i++) {
		locvar = AtomTable_Get(table, ITEM_LOCVAR, i);
		func = get_func(indexes, sz, locvar->index);
		if (func)
			load_locvar(locvar, table, func);
	}
}

Package *Package_From_Image(KImage *image)
{
	Package *pkg = Package_New(image->header.pkgname);
	load_consts(pkg, image->table);
	load_variables(pkg, image->table);
	load_functions(pkg, image->table);
	//load_traits(pkg, image->table);
	//load_classes(pkg, image->table);
	return pkg;
}

static Object *package_tostring(Object *ob)
{
	OB_ASSERT_KLASS(ob, Package_Klass);
	Package *pkg = (Package *)ob;
	return MemberDef_HashTable_ToString(pkg->table);
}

Klass Package_Klass = {
	OBJECT_HEAD_INIT(&Klass_Klass)
	.name = "Package",
	.basesize = sizeof(Package),
	.ob_str = package_tostring
};

static Object *__package_display(Object *ob, Object *args)
{
	assert(!args);
	Object *s = package_tostring(ob);
	printf("%s\n", String_RawString(s));
	OB_DECREF(s);
	return NULL;
}

static FuncDef package_funcs[] = {
	{"Display", NULL, NULL, __package_display},
	{NULL}
};

void Init_Package_Klass(void)
{
	Klass_Add_CFunctions(&Package_Klass, package_funcs);
}

void Fini_Package_Klass(void)
{
	Fini_Klass(&Package_Klass);
}
