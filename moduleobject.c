
#include "moduleobject.h"
#include "tupleobject.h"
#include "log.h"

/*-------------------------------------------------------------------------*/

Object *Module_New(char *name)
{
	ModuleObject *m = calloc(1, sizeof(ModuleObject));
	Init_Object_Head(m, &Module_Klass);
	m->name = strdup(name);
	m->varcnt = 1; //FIXME
	return (Object *)m;
}

void Module_Free(Object *ob)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	Tuple_Free(m->values);
	Tuple_Free(m->consts);
	HashTable_Free(m->table, NULL, NULL);
	free(m->name);
	free(m);
}

static HashTable *__get_table(ModuleObject *m)
{
	if (!m->table) {
		HashInfo hashinfo = {
			.hash = (ht_hashfunc)Member_Hash,
			.equal = (ht_equalfunc)Member_Equal
		};
		m->table = HashTable_New(&hashinfo);
	}
	return m->table;
}

int Module_Add_Var(Object *ob, char *name, TypeDesc *desc, int bconst)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	MemberDef *member = Member_Var_New(name, desc, bconst);
	int res = HashTable_Insert(__get_table(m), &member->hnode);
	if (!res) {
		member->offset = m->varcnt++;
		return 0;
	} else {
		Member_Free(member);
		return -1;
	}
}

int Module_Add_Func(Object *ob, char *name, Object *code)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	CodeObject *co = OBJ_TO_CODE(code);
	MemberDef *member = Member_Code_New(name, co->proto);
	int res = HashTable_Insert(__get_table(m), &member->hnode);
	if (!res) {
		member->code = code;
		if (CODE_ISKFUNC(code)) {
			co->kf.consts = m->consts;
		}
		return 0;
	} else {
		Member_Free(member);
		return -1;
	}
}

int Module_Add_CFunc(Object *ob, FuncDef *f)
{
	Vector *rdesc = TypeString_To_Vector(f->rdesc);
	Vector *pdesc = TypeString_To_Vector(f->pdesc);
	TypeDesc *proto = TypeDesc_From_Proto(rdesc, pdesc);
	Object *code = CFunc_New(f->fn, proto);
	return Module_Add_Func(ob, f->name, code);
}

int Module_Add_Class(Object *ob, Klass *klazz)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	MemberDef *member = Member_Class_New(klazz->name);
	int res = HashTable_Insert(__get_table(m), &member->hnode);
	if (!res) {
		member->klazz = klazz;
		klazz->module = ob;
		return 0;
	} else {
		Member_Free(member);
		return -1;
	}
}

int Module_Add_Trait(Object *ob, Klass *klazz)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	MemberDef *member = Member_Trait_New(klazz->name);
	int res = HashTable_Insert(__get_table(m), &member->hnode);
	if (!res) {
		member->klazz = klazz;
		klazz->module = ob;
		return 0;
	} else {
		Member_Free(member);
		return -1;
	}
}

static int __get_value_index(ModuleObject *m, char *name)
{
	MemberDef key = {.name = name};
	MemberDef *member = HashTable_Find(__get_table(m), &key);
	if (!member) return -1;
	if (member->kind != MEMBER_VAR) {
		error("'%s' is not a variable", name);
		return -1;
	}
	return member->offset;
}

static Object *__get_tuple(ModuleObject *m)
{
	if (!m->values) {
		m->values = Tuple_New(m->varcnt);
	}
	return m->values;
}

TValue Module_Get_Value(Object *ob, char *name)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	int index = __get_value_index(m, name);
	if (index < 0) return NilValue;
	assert(index < m->varcnt);
	return Tuple_Get(__get_tuple(m), index);
}

int Module_Set_Value(Object *ob, char *name, TValue *val)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	int index = __get_value_index(m, name);
	assert(index >= 0 && index < m->varcnt);
	return Tuple_Set(__get_tuple(m), index, val);
}

Object *Module_Get_Function(Object *ob, char *name)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	MemberDef key = {.name = name};
	MemberDef *member = HashTable_Find(__get_table(m), &key);
	if (!member) return NULL;
	if (member->kind != MEMBER_CODE) {
		error("'%s' is not a function", name);
		return NULL;
	}
	return member->code;
}

Klass *Module_Get_Class(Object *ob, char *name)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	MemberDef key = {.name = name};
	MemberDef *member = HashTable_Find(__get_table(m), &key);
	if (!member) return NULL;
	if (member->kind != MEMBER_CLASS) {
		error("'%s' is not a class", name);
		return NULL;
	}
	return member->klazz;
}

Klass *Module_Get_Trait(Object *ob, char *name)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	MemberDef key = {.name = name};
	MemberDef *member = HashTable_Find(__get_table(m), &key);
	if (!member) return NULL;
	if (member->kind != MEMBER_TRAIT) {
		error("'%s' is not a trait", name);
		return NULL;
	}
	return member->klazz;
}

Klass *Module_Get_ClassOrTrait(Object *ob, char *name)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	MemberDef key = {.name = name};
	MemberDef *member = HashTable_Find(__get_table(m), &key);
	if (!member) return NULL;
	if (member->kind != MEMBER_CLASS && member->kind != MEMBER_TRAIT) {
		error("'%s' is not a class and trait", name);
		return NULL;
	}
	return member->klazz;
}

int Module_Add_CFunctions(Object *ob, FuncDef *funcs)
{
	int res;
	FuncDef *f = funcs;
	while (f->name) {
		res = Module_Add_CFunc(ob, f);
		assert(res == 0);
		++f;
	}
	return 0;
}

/*-------------------------------------------------------------------------*/

static void module_free(Object *ob)
{
	Module_Free(ob);
}

Klass Module_Klass = {
	OBJECT_HEAD_INIT(&Module_Klass, &Klass_Klass)
	.name = "Module",
	.basesize = sizeof(ModuleObject),
	.ob_free = module_free
};
