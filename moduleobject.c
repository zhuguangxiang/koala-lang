
#include "moduleobject.h"
#include "tupleobject.h"
#include "log.h"

/*-------------------------------------------------------------------------*/

Object *Module_New(char *name, AtomTable *atbl)
{
	ModuleObject *m = malloc(sizeof(ModuleObject));
	Init_Object_Head(m, &Module_Klass);
	m->name = strdup(name);
	STable_Init(&m->stbl, atbl);
	m->tuple = NULL;
	return (Object *)m;
}

void Module_Free(Object *ob)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	free(m->name);
	if (m->tuple) Tuple_Free(m->tuple);
	STable_Fini(&m->stbl);
	free(m);
}

int Module_Add_Var(Object *ob, char *name, TypeDesc *desc, int bconst)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	Symbol *sym = STable_Add_Var(&m->stbl, name, desc, bconst);
	return sym ? 0 : -1;
}

int Module_Add_Func(Object *ob, char *name, Proto *proto, Object *code)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	Symbol *sym = STable_Add_Proto(&m->stbl, name, proto);
	if (sym) {
		CodeObject *co = (CodeObject *)code;
		sym->ob = code;
		if (CODE_ISKFUNC(code)) {
			co->kf.atbl = m->stbl.atbl;
			co->kf.proto = Proto_Dup(proto);
		}
		return 0;
	}
	return -1;
}

int Module_Add_CFunc(Object *ob, FuncDef *f)
{
	Proto *proto = Proto_New(f->rsz, f->rdesc, f->psz, f->pdesc);
	Object *code = CFunc_New(f->fn);
	return Module_Add_Func(ob, f->name, proto, code);
}

int Module_Add_Class(Object *ob, Klass *klazz)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	Symbol *sym = STable_Add_Class(&m->stbl, klazz->name);
	if (sym) {
		sym->ob = klazz;
		STable_Init(&klazz->stbl, Module_AtomTable(m));
		klazz->module = ob;
		return 0;
	}
	return -1;
}

int Module_Add_Trait(Object *ob, Klass *klazz)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	Symbol *sym = STable_Add_Trait(&m->stbl, klazz->name);
	if (sym) {
		sym->ob = klazz;
		STable_Init(&klazz->stbl, Module_AtomTable(m));
		return 0;
	}
	return -1;
}

static int __get_value_index(ModuleObject *m, char *name)
{
	Symbol *sym = STable_Get(&m->stbl, name);
	if (sym) {
		if (sym->kind == SYM_VAR) {
			return sym->index;
		} else {
			error("symbol is not a variable");
		}
	}
	return -1;
}

static Object *__get_tuple(ModuleObject *m)
{
	if (!m->tuple) {
		m->tuple = Tuple_New(m->stbl.varcnt);
	}
	return m->tuple;
}

TValue Module_Get_Value(Object *ob, char *name)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	int index = __get_value_index(m, name);
	if (index < 0) return NilValue;
	assert(index < m->stbl.varcnt);
	return Tuple_Get(__get_tuple(m), index);
}

int Module_Set_Value(Object *ob, char *name, TValue *val)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	int index = __get_value_index(m, name);
	assert(index >= 0 && index < m->stbl.varcnt);
	return Tuple_Set(__get_tuple(m), index, val);
}

Object *Module_Get_Function(Object *ob, char *name)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	Symbol *sym = STable_Get(&m->stbl, name);
	if (sym) {
		if (sym->kind == SYM_PROTO) {
			return sym->ob;
		} else {
			error("symbol is not a function");
		}
	}

	return NULL;
}

Klass *Module_Get_Class(Object *ob, char *name)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	Symbol *sym = STable_Get(&m->stbl, name);
	if (sym) {
		if (sym->kind == SYM_CLASS) {
			return sym->ob;
		} else {
			warn("symbol is not a class");
		}
	}
	return NULL;
}

Klass *Module_Get_ClassOrTrait(Object *ob, char *name)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	Symbol *sym = STable_Get(&m->stbl, name);
	if (sym) {
		if (sym->kind == SYM_CLASS || sym->kind == SYM_TRAIT) {
			return sym->ob;
		} else {
			error("symbol is not a class or trait");
		}
	}
	return NULL;
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

struct path_stbl_struct {
	char *path;
	STable *stbl;
};

static void __to_stbl_fn(Symbol *sym, void *arg)
{
	struct path_stbl_struct *path_struct = arg;
	STable *stbl = path_struct->stbl;
	char *path = path_struct->path;

	if (sym->kind == SYM_CLASS || sym->kind == SYM_INTF) {
		Symbol *s = STable_Add_Symbol(stbl, sym->name, SYM_STABLE, 0);
		s->desc = TypeDesc_From_UserDef(strdup(path), sym->name);
		s->ptr = STable_New(stbl->atbl);
		struct path_stbl_struct tmp = {path, s->ptr};
		STable_Traverse(Klass_STable(sym->ob), __to_stbl_fn, &tmp);
	} else if (sym->kind == SYM_VAR) {
		STable_Add_Var(stbl, sym->name, sym->desc, sym->access & ACCESS_CONST);
	} else if (sym->kind == SYM_PROTO) {
		//FIXME
		STable_Add_Proto(stbl, sym->name, sym->desc->proto);
	} else if (sym->kind == SYM_IPROTO) {
		//FIXME
		STable_Add_IProto(stbl, sym->name, sym->desc->proto);
	} else {
		assert(0);
	}
}

/* for compiler only */
STable *Module_To_STable(Object *ob, AtomTable *atbl, char *path)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	STable *stbl = STable_New(atbl);
	struct path_stbl_struct path_struct = {path, stbl};
	STable_Traverse(&m->stbl, __to_stbl_fn, &path_struct);
	return stbl;
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

/*-------------------------------------------------------------------------*/

void Module_Show(Object *ob)
{
	ModuleObject *m = OBJ_TO_MOD(ob);
	printf("package:%s\n", m->name);
	STable_Show(&m->stbl, 0);
}
