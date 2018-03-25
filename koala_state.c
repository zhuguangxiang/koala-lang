
#include "koala_state.h"
#include "moduleobject.h"
#include "stringobject.h"
#include "tupleobject.h"
#include "tableobject.h"
#include "classobject.h"
#include "mod_io.h"
#include "routine.h"
#include "log.h"

struct mod_entry {
	HashNode hnode;
	char *path;
	Object *ob;
};

static struct mod_entry *new_mod_entry(char *path, Object *ob)
{
	struct mod_entry *e = malloc(sizeof(struct mod_entry));
	Init_HashNode(&e->hnode, e);
	e->path = path;
	e->ob = ob;
	return e;
}

static void free_mod_entry(struct mod_entry *e)
{
	assert(HashNode_Unhashed(&e->hnode));
	Module_Free(e->ob);
	free(e);
}

static uint32 mod_entry_hash(void *k)
{
	struct mod_entry *e = k;
	return hash_string(e->path);
}

static int mod_entry_equal(void *k1, void *k2)
{
	struct mod_entry *e1 = k1;
	struct mod_entry *e2 = k2;
	return !strcmp(e1->path, e2->path);
}

/*-------------------------------------------------------------------------*/

KoalaState gs;

static int add_module(char *path, Object *ob)
{
	struct mod_entry *e = new_mod_entry(path, ob);
	if (HashTable_Insert(&gs.modules, &e->hnode) < 0) {
		error("add module '%s' failed", path);
		free_mod_entry(e);
		return -1;
	}
	return 0;
}

Object *Koala_New_Module(char *name, char *path)
{
	Object *ob = Module_New(name, NULL);
	if (add_module(path, ob) < 0) {
		Module_Free(ob);
		return NULL;
	}
	return ob;
}

static void run_code(Object *code, Object *ob, Object *args)
{
	Routine *rt = Routine_New(code, ob, args);
	Routine_Run(rt);
}

Object *Koala_Get_Module(char *path)
{
	struct mod_entry e = {.path = path};
	struct mod_entry *entry = HashTable_Find(&gs.modules, &e);
	if (!entry) return NULL;
	return entry->ob;
}

Klass *Koala_Get_Klass(Object *ob, char *path, char *type)
{
	if (path) {
		//different module
		assert(!ob);
		ob = Koala_Get_Module(path);
		if (!ob) return NULL;
	} else {
		//the same module
		assert(ob);
	}
	return Module_Get_Class(ob, type);
}

/*-------------------------------------------------------------------------*/

static Object *load_module(char *path);

struct classindex {
	int index;
	Klass *klazz;
};

struct locvarindex {
	int index;
	Object *func;
};

static Klass *get_klazz(struct classindex *indexes, int size, int index)
{
	struct classindex *clsidx;
	for (int i = 0; i < size; i++) {
		clsidx = indexes + i;
		if (clsidx->index == index) {
			return clsidx->klazz;
		}
	}
	return NULL;
}

static Object *get_kfunc(struct locvarindex *indexes, int size, int index)
{
	struct locvarindex *locvaridx;
	for (int i = 0; i < size; i++) {
		locvaridx = indexes + i;
		if (locvaridx->index == index) {
			return locvaridx->func;
		}
	}
	return NULL;
}

static void load_variables(AtomTable *table, Object *m)
{
	int sz = AtomTable_Size(table, ITEM_VAR);
	VarItem *var;
	StringItem *id;
	TypeItem *type;
	TypeDesc *desc;

	for (int i = 0; i < sz; i++) {
		var = AtomTable_Get(table, ITEM_VAR, i);
		id = StringItem_Index(table, var->nameindex);
		type = TypeItem_Index(table, var->typeindex);
		//FIXME
		desc = TypeDesc_New(0);
		TypeItem_To_Desc(table, type, desc);
		Module_Add_Var(m, id->data, desc, var->access & ACCESS_CONST);
	}
}

static void load_locvar(LocVarItem *locvar, AtomTable *table, Object *code)
{
	StringItem *stritem = StringItem_Index(table, locvar->nameindex);;
	TypeItem *typeitem = TypeItem_Index(table, locvar->typeindex);
	char *name = stritem->data;
	TypeDesc *desc = TypeDesc_New(0);
	TypeItem_To_Desc(table, typeitem, desc);
	KFunc_Add_LocVar(code, name, desc, locvar->pos);
}

static void load_functions(AtomTable *table, Object *m)
{
	int sz = AtomTable_Size(table, ITEM_FUNC);
	int num = sz;
	FuncItem *func;
	StringItem *id;
	ProtoItem *protoitem;
	Proto *proto;
	CodeItem *codeitem;
	Object *code;
	struct locvarindex indexes[num];

	for (int i = 0; i < sz; i++) {
		func = AtomTable_Get(table, ITEM_FUNC, i);
		id = StringItem_Index(table, func->nameindex);
		protoitem = ProtoItem_Index(table, func->protoindex);
		proto = Proto_From_ProtoItem(protoitem, table);
		codeitem = CodeItem_Index(table, func->codeindex);
		code = KFunc_New(func->locvars, codeitem->codes, codeitem->size);
		Module_Add_Func(m, id->data, proto, code);
		indexes[i].index = i;
		indexes[i].func = code;
	}

	// handle locvars
	sz = AtomTable_Size(table, ITEM_LOCVAR);
	LocVarItem *locvar;
	for (int i = 0; i < sz; i++) {
		locvar = AtomTable_Get(table, ITEM_LOCVAR, i);
		if (locvar->flags == FUNCLOCVAR) {
			load_locvar(locvar, table, get_kfunc(indexes, num, locvar->index));
		}
	}
}

static Klass *load_class(ClassItem *cls, AtomTable *table, Object *m)
{
	TypeItem *type = TypeItem_Index(table, cls->classindex);
	assert(type->protoindex == -1);
	StringItem *id = StringItem_Index(table, type->typeindex);
	assert(id);
	char *cls_name = id->data;
	Klass *klazz;
	Klass *super = NULL;

	klazz = Module_Get_Class(m, cls_name);
	if (klazz) {
		debug("class '%s' is already loaded", cls_name);
		return klazz;
	}

	if (cls->superindex >= 0) {
		type = TypeItem_Index(table, cls->superindex);
		assert(type);
		id = StringItem_Index(table, type->typeindex);
		assert(id);
		char *super_name = id->data;
		if (type->pathindex >= 0) {
			char *path;
			id = StringItem_Index(table, type->pathindex);
			assert(id);
			path = id->data;
			Object *ob = Koala_Load_Module(path);
			if (!ob) {
				error("cannot load module '%s'", path);
				exit(-1);
			}
			super = Module_Get_Class(ob, super_name);
			if (!super) {
				error("cannot find super class '%s.%s'", path, super_name);
				exit(-1);
			}
		} else {
			debug("super class '%s' in current module", super_name);
			super = Module_Get_Class(m, super_name);
			if (!super) {
				int sz = AtomTable_Size(table, ITEM_CLASS);
				for (int i = 0; i < sz; i++) {
					cls = AtomTable_Get(table, ITEM_CLASS, i);
					type = TypeItem_Index(table, cls->classindex);
					assert(type->protoindex == -1);
					id = StringItem_Index(table, type->typeindex);
					if (!strcmp(super_name, id->data)) {
						super = load_class(cls, table, m);
						break;
					}
				}
				if (!super) {
					error("cannot find super class '%s'", super_name);
					exit(-1);
				}
			}
		}
	}

	klazz = Class_New(cls_name, super);
	Module_Add_Class(m, klazz);
	return klazz;
}

static void load_field(FieldItem *fld, AtomTable *table, Klass *klazz)
{
	StringItem *id;
	TypeItem *type;
	TypeDesc *desc;

	id = StringItem_Index(table, fld->nameindex);
	debug("load field:'%s'", id->data);
	type = TypeItem_Index(table, fld->typeindex);
	//FIXME
	desc = TypeDesc_New(0);
	TypeItem_To_Desc(table, type, desc);
	Klass_Add_Field(klazz, id->data, desc);
}

static Object *load_method(MethodItem *mth, AtomTable *table, Klass *klazz)
{
	StringItem *id;
	ProtoItem *protoitem;
	Proto *proto;
	CodeItem *codeitem;
	Object *code;

	id = StringItem_Index(table, mth->nameindex);
	protoitem = ProtoItem_Index(table, mth->protoindex);
	proto = Proto_From_ProtoItem(protoitem, table);
	codeitem = CodeItem_Index(table, mth->codeindex);
	code = KFunc_New(mth->locvars, codeitem->codes, codeitem->size);
	Klass_Add_Method(klazz, id->data, proto, code);
	return code;
}

void update_fields_fn(Symbol *sym, void *arg)
{
	int nrfields = *(int *)arg;

	if (sym->kind == SYM_VAR) {
		debug("update field '%s' index from %d to %d",
			sym->name, sym->index, nrfields + sym->index);
		sym->index += nrfields;
	}
}

static void load_classes(AtomTable *table, Object *m)
{
	int sz = AtomTable_Size(table, ITEM_CLASS);
	int num = sz;
	ClassItem *cls;
	Klass *klazz;
	struct classindex indexes[num];
	for (int i = 0; i < sz; i++) {
		cls = AtomTable_Get(table, ITEM_CLASS, i);
		klazz = load_class(cls, table, m);
		indexes[i].index = cls->classindex;
		indexes[i].klazz = klazz;
	}

	//handle field and method inheritance
	// for (int i = 0; i < num; i++) {
	// 	klazz = indexes[i].klazz;
	// 	if (klazz->super) {
	// 		STbl_Traverse(&klazz->super->stbl, inherit_super_fn, klazz);
	// 	}
	// }

	sz = AtomTable_Size(table, ITEM_FIELD);
	FieldItem *fld;
	for (int i = 0; i < sz; i++) {
		fld = AtomTable_Get(table, ITEM_FIELD, i);
		load_field(fld, table, get_klazz(indexes, num, fld->classindex));
	}

	sz = AtomTable_Size(table, ITEM_METHOD);
	int locnum = sz;
	struct locvarindex locindexes[locnum];
	Object *ob;
	MethodItem *mth;
	for (int i = 0; i < sz; i++) {
		mth = AtomTable_Get(table, ITEM_METHOD, i);
		ob = load_method(mth, table, get_klazz(indexes, num, mth->classindex));
		locindexes[i].index = i;
		locindexes[i].func = ob;
	}

	// handle locvars
	sz = AtomTable_Size(table, ITEM_LOCVAR);
	LocVarItem *locvar;
	for (int i = 0; i < sz; i++) {
		locvar = AtomTable_Get(table, ITEM_LOCVAR, i);
		if (locvar->flags == METHLOCVAR) {
			load_locvar(locvar, table, get_kfunc(locindexes, locnum, locvar->index));
		}
	}

	//update subclass's fields index
#if 0
	for (int i = 0; i < num; i++) {
		klazz = indexes[i].klazz;
		if (klazz->super) {
			Klass *super = klazz->super;
			int nrfields = super->stbl.varcnt;
			Klass *k = super->super;
			while (k) {
				nrfields += k->stbl.varcnt;
				k = k->super;
			}
			debug("'%s' super class nrfields:%d", klazz->name, nrfields);
			STbl_Traverse(&klazz->stbl, update_fields_fn, &nrfields);
		}
	}

	//update subclass's symbol table
	Symbol *sym;
	for (int i = 0; i < num; i++) {
		klazz = indexes[i].klazz;
		if (klazz->super) {
			Klass *super = klazz->super;
			sym = STbl_Add_Symbol(&klazz->stbl, super->name, SYM_STABLE, 0);
			sym->ptr = &super->stbl;
			sym = STbl_Add_Symbol(&klazz->stbl, "super", SYM_STABLE, 0);
			sym->ptr = &super->stbl;
			debug("update '%s' class's symbol table from super '%s' class",
				klazz->name, super->name);
		}
	}
#endif
}

static Klass *load_interface(IntfItem *intf, AtomTable *table, Object *m)
{
	TypeItem *type = TypeItem_Index(table, intf->classindex);
	assert(type->protoindex == -1);
	StringItem *id = StringItem_Index(table, type->typeindex);
	Klass *klazz = Class_New(id->data, NULL);
	Module_Add_Interface(m, klazz);
	return klazz;
}

static void load_imethod(IMethItem *imth, AtomTable *table, Klass *klazz)
{
	StringItem *id;
	ProtoItem *protoitem;
	Proto *proto;
	TypeDesc *desc;

	id = StringItem_Index(table, imth->nameindex);
	protoitem = ProtoItem_Index(table, imth->protoindex);
	proto = Proto_From_ProtoItem(protoitem, table);
	desc = TypeDesc_From_Proto(proto);
	Klass_Add_Field(klazz, id->data, desc);
}

static void load_interfaces(AtomTable *table, Object *m)
{
	int sz = AtomTable_Size(table, ITEM_INTF);
	int num = sz;
	IntfItem *intf;
	Klass *klazz;
	struct classindex indexes[num];
	for (int i = 0; i < sz; i++) {
		intf = AtomTable_Get(table, ITEM_INTF, i);
		klazz = load_interface(intf, table, m);
		indexes[i].index = intf->classindex;
		indexes[i].klazz = klazz;
	}

	sz = AtomTable_Size(table, ITEM_IMETH);
	IMethItem *imth;
	for (int i = 0; i < sz; i++) {
		imth = AtomTable_Get(table, ITEM_IMETH, i);
		load_imethod(imth, table, get_klazz(indexes, num, imth->classindex));
	}
}

static Object *module_from_image(KImage *image)
{
	AtomTable *table = image->table;
	char *package = image->package;
	debug("load module '%s' from image", package);
	Object *m = Module_New(package, table);

	load_variables(table, m);
	load_functions(table, m);
	load_classes(table, m);
	load_interfaces(table, m);
	return m;
}

/*-------------------------------------------------------------------------*/

static Object *load_module(char *path)
{
	debug("loading module '%s'", path);
	char **loadpathes = Properties_Get(&gs.config, "koala.path");
	char **loadpath = loadpathes;

	while (loadpath && *loadpath) {
		char file[strlen(*loadpath) + strlen(path) + 4 + 1];
		sprintf(file, "%s%s.klc", *loadpath, path);
		KImage *image = KImage_Read_File(file);
		if (image) {
			Object *ob = module_from_image(image);
			if (ob) {
				if (add_module(path, ob) < 0) {
					Module_Free(ob);
					ob = NULL;
				} else {
					Object *code = Module_Get_Function(ob, "__init__");
					if (code) {
						debug("run __init__ in module '%s'", path);
						//FIXME: new routine ?
						run_code(code, ob, NULL);
					} else {
						debug("cannot find '__init__' in module '%s'", path);
					}
					debug("load module '%s' successfully", path);
				}
				free(loadpathes);
				return ob;
			}
		}
		loadpath++;
	}
	free(loadpathes);
	return NULL;
}

Object *Koala_Load_Module(char *path)
{
	Object *ob = Koala_Get_Module(path);
	if (!ob) return load_module(path);
	else return ob;
}

void Koala_Run(char *path)
{
	Object *ob = Koala_Load_Module(path);
	if (!ob) return;
	Object *code = Module_Get_Function(ob, "Main");
	if (code) {
		run_code(code, ob, NULL);
	} else {
		error("No 'Main' in '%s'", Module_Name(ob));
	}
}

/*-------------------------------------------------------------------------*/

static void Init_Lang_Module(void)
{
	Object *m = Koala_New_Module("lang", "koala/lang");
	Module_Add_Class(m, &Klass_Klass);
	Module_Add_Class(m, &String_Klass);
	Module_Add_Class(m, &Tuple_Klass);
	Module_Add_Class(m, &Table_Klass);
	Init_String_Klass();
	Init_Tuple_Klass();
	Init_Table_Klass();
}

static void Init_Modules(void)
{
	/* koala/lang.klc */
	Init_Lang_Module();

	/* koala/io.klc */
	Init_IO_Module();
}

/*-------------------------------------------------------------------------*/

void Koala_Initialize(void)
{
	HashInfo hashinfo;
	Init_HashInfo(&hashinfo, mod_entry_hash, mod_entry_equal);
	HashTable_Init(&gs.modules, &hashinfo);
	Properties_Init(&gs.config);
	Properties_Put(&gs.config, "koala.path", "./");
	Properties_Put(&gs.config, "koala.path", "/home/zgx/koala-repo/");

	Init_Modules();
	//sched_init();
	//schedule();
}

static void __mod_entry_free_fn(HashNode *hnode, void *arg)
{
	UNUSED_PARAMETER(arg);
	struct mod_entry *e = container_of(hnode, struct mod_entry, hnode);
	free_mod_entry(e);
}

void Koala_Finalize(void)
{
	HashTable_Fini(&gs.modules, __mod_entry_free_fn, NULL);
}
