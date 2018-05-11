
#include "kstate.h"
#include "moduleobject.h"
#include "stringobject.h"
#include "tupleobject.h"
#include "mod_lang.h"
#include "mod_io.h"
#include "routine.h"
#include "gc.h"
#include "klc.h"
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

static void collect_modules_fn(HashNode *hnode, void *arg)
{
	Vector *vec = arg;
	struct mod_entry *e = container_of(hnode, struct mod_entry, hnode);
	Vector_Append(vec, e->ob);
}

void Koala_Collect_Modules(Vector *vec)
{
	HashTable_Traverse(&gs.modules, collect_modules_fn, vec);
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
	Object *ob = Module_New(name);
	if (add_module(path, ob) < 0) {
		Module_Free(ob);
		return NULL;
	}
	return ob;
}

Object *Koala_Run_Code(Object *code, Object *ob, Object *args)
{
	Routine *rt = Routine_New(code, ob, args);
	Routine_Run(rt);
	return Tuple_From_TValues(rt->stack, rt->top + 1);
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
static Klass *load_trait(TraitItem *trait, AtomTable *table, Object *m);
static void load_trait_vector(int32 index, AtomTable *table, Object *m,
	Vector *vec);

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
		desc = TypeItem_To_Desc(type, table);
		Module_Add_Var(m, id->data, desc, var->access & ACCESS_CONST);
	}
}

static void load_locvar(LocVarItem *locvar, AtomTable *table, Object *code)
{
	StringItem *stritem = StringItem_Index(table, locvar->nameindex);;
	TypeItem *typeitem = TypeItem_Index(table, locvar->typeindex);
	char *name = stritem->data;
	TypeDesc *desc = TypeItem_To_Desc(typeitem, table);
	KFunc_Add_LocVar(code, name, desc, locvar->pos);
}

static void load_functions(AtomTable *table, Object *m)
{
	int sz = AtomTable_Size(table, ITEM_FUNC);
	int num = sz;
	FuncItem *func;
	StringItem *id;
	ProtoItem *protoitem;
	TypeDesc *proto;
	CodeItem *codeitem;
	Object *code;
	struct locvarindex indexes[num];

	for (int i = 0; i < sz; i++) {
		func = AtomTable_Get(table, ITEM_FUNC, i);
		id = StringItem_Index(table, func->nameindex);
		protoitem = ProtoItem_Index(table, func->protoindex);
		proto = ProtoItem_To_TypeDesc(protoitem, table);
		codeitem = CodeItem_Index(table, func->codeindex);
		code = KFunc_New(func->locvars, codeitem->codes, codeitem->size, proto);
		Module_Add_Func(m, id->data, code);
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
	Klass *base = NULL;

	debug("load class:'%s'", cls_name);

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
			base = Module_Get_Class(ob, super_name);
			if (!base) {
				error("cannot find base class '%s.%s'", path, super_name);
				exit(-1);
			}
		} else {
			debug("base class '%s' in current module", super_name);
			base = Module_Get_Class(m, super_name);
			if (!base) {
				int sz = AtomTable_Size(table, ITEM_CLASS);
				for (int i = 0; i < sz; i++) {
					cls = AtomTable_Get(table, ITEM_CLASS, i);
					type = TypeItem_Index(table, cls->classindex);
					assert(type->protoindex == -1);
					id = StringItem_Index(table, type->typeindex);
					if (!strcmp(super_name, id->data)) {
						base = load_class(cls, table, m);
						break;
					}
				}
				if (!base) {
					error("cannot find base class '%s'", super_name);
					exit(-1);
				}
			}
		}
	}

	Vector traits = VECTOR_INIT;
	load_trait_vector(cls->traitsindex, table, m, &traits);
	klazz = Class_New(cls_name, base, &traits);
	Module_Add_Class(m, klazz);
	Vector_Fini(&traits, NULL, NULL);
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
	desc = TypeItem_To_Desc(type, table);
	Klass_Add_Field(klazz, id->data, desc);
}

static Object *load_method(MethodItem *mth, AtomTable *table, Klass *klazz)
{
	StringItem *id;
	ProtoItem *protoitem;
	TypeDesc *proto;
	CodeItem *codeitem;
	Object *code;

	id = StringItem_Index(table, mth->nameindex);
	protoitem = ProtoItem_Index(table, mth->protoindex);
	proto = ProtoItem_To_TypeDesc(protoitem, table);
	codeitem = CodeItem_Index(table, mth->codeindex);
	code = KFunc_New(mth->locvars, codeitem->codes, codeitem->size, proto);
	Klass_Add_Method(klazz, id->data, code);
	return code;
}

// void update_fields_fn(Symbol *sym, void *arg)
// {
// 	int nrfields = *(int *)arg;

// 	if (sym->kind == SYM_VAR) {
// 		debug("update field '%s' index from %d to %d",
// 			sym->name, sym->index, nrfields + sym->index);
// 		sym->index += nrfields;
// 	}
// }

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

	sz = AtomTable_Size(table, ITEM_FIELD);
	FieldItem *fld;
	for (int i = 0; i < sz; i++) {
		fld = AtomTable_Get(table, ITEM_FIELD, i);
		klazz = get_klazz(indexes, num, fld->classindex);
		if (klazz) load_field(fld, table, klazz);
	}

	sz = AtomTable_Size(table, ITEM_METHOD);
	int locnum = sz;
	struct locvarindex locindexes[locnum];
	Object *ob;
	MethodItem *mth;
	for (int i = 0; i < sz; i++) {
		mth = AtomTable_Get(table, ITEM_METHOD, i);
		klazz = get_klazz(indexes, num, mth->classindex);
		if (klazz) {
			ob = load_method(mth, table, klazz);
			locindexes[i].index = i;
			locindexes[i].func = ob;
		} else {
			locindexes[i].index = -1;
			locindexes[i].func = NULL;
		}
	}

	// handle locvars
	sz = AtomTable_Size(table, ITEM_LOCVAR);
	LocVarItem *locvar;
	for (int i = 0; i < sz; i++) {
		locvar = AtomTable_Get(table, ITEM_LOCVAR, i);
		if (locvar->flags == METHLOCVAR) {
			ob = get_kfunc(locindexes, locnum, locvar->index);
			if (ob) load_locvar(locvar, table, ob);
		}
	}
}

static void load_trait_vector(int32 index, AtomTable *table, Object *m,
	Vector *vec)
{
	if (index < 0) return;
	TypeListItem *list = TypeListItem_Index(table, index);
	assert(list);
	TypeItem *t;
	TypeItem *temp;
	TraitItem *tr;
	int sz = AtomTable_Size(table, ITEM_TRAIT);
	for (int i = 0; i < list->size; i++) {
		t = TypeItem_Index(table, list->index[i]);
		for (int i = 0; i < sz; i++) {
			tr = AtomTable_Get(table, ITEM_TRAIT, i);
			temp = TypeItem_Index(table, tr->classindex);
			assert(temp->protoindex == -1);
			assert(temp->pathindex == -1);
			if (t->typeindex == temp->typeindex) {
				Vector_Append(vec, load_trait(tr, table, m));
				break;
			}
		}
	}
}

static Klass *load_trait(TraitItem *trait, AtomTable *table, Object *m)
{
	TypeItem *type = TypeItem_Index(table, trait->classindex);
	assert(type->protoindex == -1);
	assert(type->pathindex == -1);
	StringItem *id = StringItem_Index(table, type->typeindex);
	assert(id);
	char *trait_name = id->data;
	Klass *klazz;

	debug("load trait:'%s'", trait_name);

	klazz = Module_Get_Trait(m, trait_name);
	if (klazz) {
		debug("trait '%s' is already loaded", trait_name);
		return klazz;
	}

	Vector traits = VECTOR_INIT;
	load_trait_vector(trait->traitsindex, table, m, &traits);
	klazz = Trait_New(id->data, &traits);
	Module_Add_Trait(m, klazz);
	Vector_Fini(&traits, NULL, NULL);
	return klazz;
}

static void load_imethod(IMethItem *imth, AtomTable *table, Klass *klazz)
{
	StringItem *id;
	ProtoItem *protoitem;
	TypeDesc *proto;

	id = StringItem_Index(table, imth->nameindex);
	protoitem = ProtoItem_Index(table, imth->protoindex);
	proto = ProtoItem_To_TypeDesc(protoitem, table);
	Klass_Add_Proto(klazz, id->data, proto);
}

static void load_traits(AtomTable *table, Object *m)
{
	int sz = AtomTable_Size(table, ITEM_TRAIT);
	int num = sz;
	TraitItem *trait;
	Klass *klazz;
	struct classindex indexes[num];
	for (int i = 0; i < sz; i++) {
		trait = AtomTable_Get(table, ITEM_TRAIT, i);
		klazz = load_trait(trait, table, m);
		indexes[i].index = trait->classindex;
		indexes[i].klazz = klazz;
	}

	sz = AtomTable_Size(table, ITEM_FIELD);
	FieldItem *fld;
	for (int i = 0; i < sz; i++) {
		fld = AtomTable_Get(table, ITEM_FIELD, i);
		klazz = get_klazz(indexes, num, fld->classindex);
		if (klazz) load_field(fld, table, klazz);
	}

	sz = AtomTable_Size(table, ITEM_IMETH);
	IMethItem *imth;
	for (int i = 0; i < sz; i++) {
		imth = AtomTable_Get(table, ITEM_IMETH, i);
		klazz = get_klazz(indexes, num, imth->classindex);
		if (klazz) load_imethod(imth, table, klazz);
	}

	sz = AtomTable_Size(table, ITEM_METHOD);
	int locnum = sz;
	struct locvarindex locindexes[locnum];
	Object *ob;
	MethodItem *mth;
	for (int i = 0; i < sz; i++) {
		mth = AtomTable_Get(table, ITEM_METHOD, i);
		klazz = get_klazz(indexes, num, mth->classindex);
		if (klazz) {
			ob = load_method(mth, table, klazz);
			locindexes[i].index = i;
			locindexes[i].func = ob;
		} else {
			locindexes[i].index = -1;
			locindexes[i].func = NULL;
		}
	}

	// handle locvars
	sz = AtomTable_Size(table, ITEM_LOCVAR);
	LocVarItem *locvar;
	for (int i = 0; i < sz; i++) {
		locvar = AtomTable_Get(table, ITEM_LOCVAR, i);
		if (locvar->flags == METHLOCVAR) {
			ob = get_kfunc(locindexes, locnum, locvar->index);
			if (ob) load_locvar(locvar, table, ob);
		}
	}
}

static Object *__get_consts(KImage *image)
{
	Vector *vec = image->table->items + ITEM_CONST;
	Object *tuple = Tuple_New(Vector_Size(vec));
	TValue value;
	StringItem *stritem;
	ConstItem *item;
	Vector_ForEach(item, vec) {
		switch (item->type) {
			case CONST_INT: {
				setivalue(&value, item->ival);
				break;
			}
			case CONST_FLOAT: {
				setfltvalue(&value, item->fval);
				break;
			}
			case CONST_BOOL: {
				setbvalue(&value, item->bval);
				break;
			}
			case CONST_STRING: {
				stritem = StringItem_Index(image->table, item->index);
				setobjvalue(&value, String_New_NoGC(stritem->data));
				break;
			}
			default: {
				assert(0);
				break;
			}
		}
		Tuple_Set(tuple, i, &value);
	}
	return tuple;
}

static Object *module_from_image(KImage *image)
{
	AtomTable *table = image->table;
	char *package = image->package;
	debug("load module '%s' from image", package);
	Object *m = Module_New(package);
	Module_Set_Consts(m, __get_consts(image));
	load_variables(table, m);
	load_functions(table, m);
	load_traits(table, m);
	load_classes(table, m);
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
						Koala_Run_Code(code, ob, NULL);
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
		Koala_Run_Code(code, ob, NULL);
		GC_Run();
	} else {
		error("No 'Main' in '%s'", Module_Name(ob));
	}
}

/*-------------------------------------------------------------------------*/

static void Init_Environment(void)
{
	Properties_Init(&gs.config);
	Properties_Put(&gs.config, "koala.path", "./");
	char *home = getenv("HOME");
	char *repo = "/.koala-repo/";
	char *path = malloc(strlen(home) + strlen(repo) + 1);
	strcpy(path, home);
	strcat(path, repo);
	Properties_Put(&gs.config, "koala.path", path);
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
	//init gs
	HashInfo hashinfo;
	Init_HashInfo(&hashinfo, mod_entry_hash, mod_entry_equal);
	HashTable_Init(&gs.modules, &hashinfo);
	init_list_head(&gs.routines);

	//init env
	Init_Environment();

	//init builtin modules
	Init_Modules();

	//sched_init();
	//schedule();
	GC_Init();
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
