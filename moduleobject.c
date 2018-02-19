
#include "koala.h"

/*-------------------------------------------------------------------------*/

Object *Module_New(char *name, AtomTable *atbl)
{
	ModuleObject *ob = malloc(sizeof(ModuleObject));
	init_object_head(ob, &Module_Klass);
	ob->name = strdup(name);
	STbl_Init(&ob->stbl, atbl);
	ob->tuple = NULL;
	return (Object *)ob;
}

void Module_Free(Object *ob)
{
	ModuleObject *mob = OBJ_TO_MOD(ob);
	free(mob->name);
	if (mob->tuple) Tuple_Free(mob->tuple);
	STbl_Fini(&mob->stbl);
	free(ob);
}

int Module_Add_Var(Object *ob, char *name, TypeDesc *desc, int bconst)
{
	ModuleObject *mob = OBJ_TO_MOD(ob);
	Symbol *sym = STbl_Add_Var(&mob->stbl, name, desc, bconst);
	return sym ? 0 : -1;
}

int Module_Add_Func(Object *ob, char *name, Proto *proto, Object *code)
{
	ModuleObject *mob = OBJ_TO_MOD(ob);
	Symbol *sym = STbl_Add_Proto(&mob->stbl, name, proto);
	if (sym) {
		sym->code = code;
		if (CODE_ISKFUNC(code)) {
			CodeObject *co = OB_TYPE_OF(code, CodeObject, Code_Klass);
			co->kf.stbl = &mob->stbl;
			co->kf.proto = proto;
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
	ModuleObject *mob = OBJ_TO_MOD(ob);
	Symbol *sym = STbl_Add_Class(&mob->stbl, klazz->name);
	if (sym) {
		sym->klazz = klazz;
		STbl_Init(&klazz->stbl, Module_AtomTable(mob));
		return 0;
	}
	return -1;
}

int Module_Add_Interface(Object *ob, Klass *klazz)
{
	ModuleObject *mob = OBJ_TO_MOD(ob);
	Symbol *sym = STbl_Add_Intf(&mob->stbl, klazz->name);
	if (sym) {
		sym->klazz = klazz;
		STbl_Init(&klazz->stbl, Module_AtomTable(mob));
		return 0;
	}
	return -1;
}

static int __get_value_index(ModuleObject *mob, char *name)
{
	Symbol *sym = STbl_Get(&mob->stbl, name);
	if (sym) {
		if (sym->kind == SYM_VAR) {
			return sym->index;
		} else {
			error("symbol is not a variable");
		}
	}
	return -1;
}

#if 0
static void init_mod_var(Symbol *sym, void *arg)
{
	ModuleObject *mob = arg;
	if (sym->kind == SYM_VAR) {
		TValue val = NIL_VALUE_INIT();
		TypeDesc *type = sym->type;
		assert(type);
		switch (type->kind) {
			case TYPE_PRIMITIVE: {
				if (type->primitive == PRIMITIVE_INT) {
					setivalue(&val, 0);
				} else if (type->primitive == PRIMITIVE_FLOAT) {
					setfltvalue(&val, 0.0);
				} else if (type->primitive == PRIMITIVE_BOOL) {
					setbvalue(&val, 0);
				} else if (type->primitive == PRIMITIVE_STRING) {
					setcstrvalue(&val, NULL);
				}
				break;
			}
			case TYPE_USERDEF: {
				break;
			}
			case TYPE_PROTO: {
				break;
			}
			default: {
				assert(0);
				break;
			}
		}
		Tuple_Set(mob->tuple, sym->index, &val);
	}
}

#endif

static Object *__get_tuple(ModuleObject *mob)
{
	if (!mob->tuple) {
		mob->tuple = Tuple_New(mob->stbl.next);
		//STbl_Traverse(&mob->stbl, init_mod_var, mob);
	}
	return mob->tuple;
}

TValue Module_Get_Value(Object *ob, char *name)
{
	ModuleObject *mob = OBJ_TO_MOD(ob);
	int index = __get_value_index(mob, name);
	if (index < 0) return NilValue;
	assert(index < mob->stbl.next);
	return Tuple_Get(__get_tuple(mob), index);
}

int Module_Set_Value(Object *ob, char *name, TValue *val)
{
	ModuleObject *mob = OBJ_TO_MOD(ob);
	int index = __get_value_index(mob, name);
	assert(index >= 0 && index < mob->stbl.next);
	return Tuple_Set(__get_tuple(mob), index, val);
}

Object *Module_Get_Function(Object *ob, char *name)
{
	ModuleObject *mob = OBJ_TO_MOD(ob);
	Symbol *sym = STbl_Get(&mob->stbl, name);
	if (sym) {
		if (sym->kind == SYM_PROTO) {
			return sym->code;
		} else {
			error("symbol is not a function");
		}
	}

	return NULL;
}

Klass *Module_Get_Class(Object *ob, char *name)
{
	ModuleObject *mob = OBJ_TO_MOD(ob);
	Symbol *sym = STbl_Get(&mob->stbl, name);
	if (sym) {
		if (sym->kind == SYM_CLASS) {
			return sym->klazz;
		} else {
			error("symbol is not a class");
		}
	}
	return NULL;
}

Klass *Module_Get_Intf(Object *ob, char *name)
{
	ModuleObject *mob = OBJ_TO_MOD(ob);
	Symbol *sym = STbl_Get(&mob->stbl, name);
	if (sym) {
		if (sym->kind == SYM_INTF) {
			return sym->klazz;
		} else {
			error("symbol is not a interface");
		}
	}
	return NULL;
}

Klass *Module_Get_Klass(Object *ob, char *name)
{
	ModuleObject *mob = OBJ_TO_MOD(ob);
	Symbol *sym = STbl_Get(&mob->stbl, name);
	if (sym) {
		if (sym->kind == SYM_CLASS || sym->kind == SYM_INTF) {
			return sym->klazz;
		} else {
			error("symbol is not a class");
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

static void mod_to_stbl(Symbol *sym, void *arg)
{
	STable *stbl = arg;
	Symbol *tmp;
	if (sym->kind == SYM_CLASS || sym->kind == SYM_INTF) {
		tmp = STbl_Add_Symbol(stbl, sym->str, SYM_STABLE, 0);
		tmp->stbl = STbl_New(stbl->atbl);
		STbl_Traverse(Klass_STable(sym->klazz), mod_to_stbl, tmp->stbl);
	} else if (sym->kind == SYM_VAR) {
		STbl_Add_Var(stbl, sym->str, sym->type, sym->konst);
	} else if (sym->kind == SYM_PROTO) {
		STbl_Add_Proto(stbl, sym->str, sym->type->proto);
	} else if (sym->kind == SYM_IPROTO) {
		STbl_Add_IProto(stbl, sym->str, sym->type->proto);
	} else {
		assert(0);
	}
}

/* for compiler only */
STable *Module_To_STable(Object *ob, AtomTable *atbl)
{
	ModuleObject *mob = OBJ_TO_MOD(ob);
	STable *stbl = STbl_New(atbl);
	STbl_Traverse(&mob->stbl, mod_to_stbl, stbl);
	return stbl;
}

/*-------------------------------------------------------------------------*/

static void module_free(Object *ob)
{
	Module_Free(ob);
}

Klass Module_Klass = {
	OBJECT_HEAD_INIT(&Klass_Klass),
	.name  = "Module",
	.bsize = sizeof(ModuleObject),

	.ob_free = module_free
};

/*-------------------------------------------------------------------------*/

void Module_Show(Object *ob)
{
	ModuleObject *mob = OBJ_TO_MOD(ob);
	printf("package:%s\n", mob->name);
	STbl_Show(&mob->stbl, 1);
}

Object *Module_From_Image(KImage *image)
{
	Object *ob = Module_New(image->package, image->table);
	AtomTable *table = image->table;
	int sz;
	VarItem *var;
	FuncItem *func;
	StringItem *name;
	TypeItem *type;
	ProtoItem *protoitem;
	CodeItem *codeitem;
	TypeDesc *desc;
	Proto *proto;
	Object *code;

	//load variables
	sz = AtomTable_Size(table, ITEM_VAR);
	for (int i = 0; i < sz; i++) {
		var = AtomTable_Get(table, ITEM_VAR, i);
		name = StringItem_Index(table, var->nameindex);
		type = TypeItem_Index(table, var->typeindex);
		desc = TypeDesc_New(0);
		TypeItem_To_Desc(table, type, desc);
		Module_Add_Var(ob, name->data, desc, var->flags & VAR_FLAG_CONST);
	}

	//load functions
	sz = AtomTable_Size(table, ITEM_FUNC);
	for (int i = 0; i < sz; i++) {
		func = AtomTable_Get(table, ITEM_FUNC, i);
		name = StringItem_Index(table, func->nameindex);
		protoitem = ProtoItem_Index(table, func->protoindex);
		proto = Proto_From_ProtoItem(protoitem, table);
		codeitem = CodeItem_Index(table, func->codeindex);
		code = KFunc_New(func->locvars, codeitem->codes, codeitem->size);
		Module_Add_Func(ob, name->data, proto, code);
	}

	return ob;
}
