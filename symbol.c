
#include "object.h"
#include "codeobject.h"
#include "hash.h"
#include "log.h"
#include "codeblock.h"

static Symbol *symbol_new(void)
{
	Symbol *sym = calloc(1, sizeof(Symbol));
	Init_HashNode(&sym->hnode, sym);
	return sym;
}

static void symbol_free(Symbol *sym)
{
	if (sym->kind == SYM_VAR) {
		//FIXME:share with struct var
		//TypeDesc_Free(sym->desc);
	} else if (sym->kind == SYM_PROTO) {
		TypeDesc_Free(sym->desc);
		if (sym->ob) CodeObject_Free(sym->ob);
		if (sym->ptr) CodeBlock_Free(sym->ptr);
	} else if (sym->kind == SYM_CLASS || sym->kind == SYM_INTF) {
		Klass *klazz = sym->ob;
		if (klazz->dynamic) {
			//Klass_Free(klazz);
		} else {
			Fini_Klass(klazz);
		}
	} else if (sym->kind == SYM_STABLE) {
		STbl_Free(sym->ptr);
		free(sym->path);
	} else {
		assert(0);
	}

	free(sym->name);
	free(sym);
}

static uint32 symbol_hash(void *k)
{
	Symbol *s = k;
	return hash_uint32(s->nameidx, 0);
}

static int symbol_equal(void *k1, void *k2)
{
	Symbol *s1 = k1;
	Symbol *s2 = k2;
	return s1->nameidx == s2->nameidx;
}

static void ht_symbol_free(HashNode *hnode, void *arg)
{
	UNUSED_PARAMETER(arg);
	Symbol *sym = container_of(hnode, Symbol, hnode);
	symbol_free(sym);
}

/*-------------------------------------------------------------------------*/

static HashTable *__get_hashtable(STable *stbl)
{
	if (!stbl->htbl) {
		HashInfo hashinfo;
		Init_HashInfo(&hashinfo, symbol_hash, symbol_equal);
		stbl->htbl = HashTable_New(&hashinfo);
	}
	return stbl->htbl;
}

int STbl_Init(STable *stbl, AtomTable *atbl)
{
	stbl->htbl = NULL;
	if (!atbl) {
		HashInfo hashinfo;
		Init_HashInfo(&hashinfo, item_hash, item_equal);
		stbl->atbl = AtomTable_New(&hashinfo, ITEM_MAX);
		stbl->flag = 1;
	} else {
		stbl->atbl = atbl;
		stbl->flag = 0;
	}
	//FIXME: start from 1st position, because position-0 is for object
	stbl->varcnt = 1;
	return 0;
}

void STbl_Fini(STable *stbl)
{
	HashTable_Free(stbl->htbl, ht_symbol_free, NULL);
	stbl->htbl = NULL;
	if (stbl->flag) AtomTable_Free(stbl->atbl, item_free, NULL);
	stbl->atbl = NULL;
	stbl->varcnt = 1;
}

STable *STbl_New(AtomTable *atbl)
{
	STable *stbl = malloc(sizeof(STable));
	STbl_Init(stbl, atbl);
	return stbl;
}

void STbl_Free(STable *stbl)
{
	STbl_Fini(stbl);
	free(stbl);
}

/*-------------------------------------------------------------------------*/

Symbol *STbl_Add_Var(STable *stbl, char *name, TypeDesc *desc, int bconst)
{
	Symbol *sym = STbl_Add_Symbol(stbl, name, SYM_VAR, bconst);
	if (!sym) return NULL;

	int32 idx = -1;
	if (desc) {
		idx = TypeItem_Set(stbl->atbl, desc);
		assert(idx >= 0);
	}
	sym->descidx = idx;
	sym->desc = desc;
	sym->index = stbl->varcnt++;
	return sym;
}

int STbl_Update_Symbol(STable *stbl, Symbol *sym, TypeDesc *desc)
{
	int32 idx = TypeItem_Set(stbl->atbl, desc);
	assert(idx >= 0);
	sym->descidx = idx;
	sym->desc = desc;
	return 0;
}

Symbol *STbl_Add_Proto(STable *stbl, char *name, Proto *proto)
{
	Symbol *sym = STbl_Add_Symbol(stbl, name, SYM_PROTO, 0);
	if (!sym) return NULL;

	int32 idx = ProtoItem_Set(stbl->atbl, proto);
	assert(idx >= 0);
	sym->descidx = idx;
	sym->desc = TypeDesc_From_Proto(proto);
	return sym;
}

Symbol *STbl_Add_Symbol(STable *stbl, char *name, int kind, int bconst)
{
	Symbol *sym = symbol_new();
	int32 idx = StringItem_Set(stbl->atbl, name);
	assert(idx >= 0);
	sym->nameidx = idx;
	sym->kind = kind;
	sym->access = SYMBOL_ACCESS(name, bconst);
	if (HashTable_Insert(__get_hashtable(stbl), &sym->hnode) < 0) {
		symbol_free(sym);
		return NULL;
	}
	sym->name = strdup(name);
	return sym;
}

Symbol *STbl_Get(STable *stbl, char *name)
{
	int32 index = StringItem_Get(stbl->atbl, name);
	if (index < 0) return NULL;
	Symbol sym = {.nameidx = index};
	HashNode *hnode = HashTable_Find(__get_hashtable(stbl), &sym);
	if (hnode) {
		return container_of(hnode, Symbol, hnode);
	} else {
		return NULL;
	}
}

/*-------------------------------------------------------------------------*/

struct visit_entry {
	symbolfunc fn;
	void *arg;
};

static void symbol_visit(HashNode *hnode, void *arg)
{
	struct visit_entry *data = arg;
	Symbol *sym = container_of(hnode, Symbol, hnode);
	data->fn(sym, data->arg);
}

void STbl_Traverse(STable *stbl, symbolfunc fn, void *arg)
{
	struct visit_entry data = {fn, arg};
	HashTable_Traverse(stbl->htbl, symbol_visit, &data);
}

/*-------------------------------------------------------------------------*/

static void desc_show(TypeDesc *desc)
{
	char *str = TypeDesc_ToString(desc);
	printf("%s", str);
	free(str);
}

static void symbol_show(HashNode *hnode, void *arg)
{
	UNUSED_PARAMETER(arg);
	Symbol *sym = container_of(hnode, Symbol, hnode);
	switch (sym->kind) {
		case SYM_VAR: {
			/* show's format: "type name desc;" */
			printf("%s %s ", sym->access & ACCESS_CONST ? "const":"var", sym->name);
			desc_show(sym->desc);
			puts(";"); /* with newline */
			break;
		}
		case SYM_PROTO: {
			/* show's format: "func name args rets;" */
			printf("func %s", sym->name);
			//FIXME
			//proto_show(sym->proto);
			puts(""); /* with newline */
			break;
		}
		case SYM_CLASS: {
			printf("class %s;\n", sym->name);
			break;
		}
		case SYM_INTF: {
			printf("interface %s;\n", sym->name);
			break;
		}
		default: {
			assert(0);
			break;
		}
	}
}

void STbl_Show(STable *stbl, int detail)
{
	HashTable_Traverse(stbl->htbl, symbol_show, stbl);
	if (detail) AtomTable_Show(stbl->atbl);
}
