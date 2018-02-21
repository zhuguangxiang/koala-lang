
#include "koalastate.h"
#include "moduleobject.h"
#include "stringobject.h"
#include "tupleobject.h"
#include "tableobject.h"
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
	if (HTable_Insert(&gs.modules, &e->hnode) < 0) {
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
	HashNode *hnode = HTable_Find(&gs.modules, &e);
	if (!hnode) return NULL;
	return (Object *)(container_of(hnode, struct mod_entry, hnode)->ob);
}

static Object *load_module(char *path)
{
	KImage *image = KImage_Read_File(path);
	if (!image) return NULL;
	Object *ob = Module_From_Image(image);
	if (ob) {
		/* initialize this module */
		Object *code = Module_Get_Function(ob, "__init__");
		if (code) {
			run_code(code, ob, NULL);
		} else {
			debug("cannot find '__init__' in module '%s'.", path);
		}
	}
	return ob;
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
	HashInfo hashinfo;
	Init_HashInfo(&hashinfo, mod_entry_hash, mod_entry_equal);
	HTable_Init(&gs.modules, &hashinfo);

	/* koala/lang.klc */
	Init_Lang_Module();

	/* koala/io.klc */
	Init_IO_Module();
}

/*-------------------------------------------------------------------------*/

void Koala_Init(void)
{
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

void Koala_Fini(void)
{
	HTable_Fini(&gs.modules, __mod_entry_free_fn, NULL);
}
