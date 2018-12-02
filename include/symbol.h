
#ifndef _KOALA_SYMBOL_H_
#define _KOALA_SYMBOL_H_

#include "image.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct symboltable {
	HashTable *htbl;
	AtomTable *atbl;
	int flag;
	int32 varcnt;
} STable;

STable *STable_New(AtomTable *atbl);
void STable_Free(STable *stbl);

#define SYM_VAR     1
#define SYM_PROTO   2
#define SYM_CLASS   3
#define SYM_TRAIT   4
#define SYM_IPROTO  5
#define SYM_STABLE  6  /* for compiler */
#define SYM_MODULE  7
#define SYM_TYPEALIAS 8

typedef struct symbol Symbol;

struct symbol {
	HashNode hnode;
	int32 nameidx;
	int32 descidx;

	int8 kind;
	int8 access;
	int8 refcnt;
	int8 inherited;

	char *name;     /* ->nameidx */
	TypeDesc *desc; /* ->descidx */

	union {
		void *ob;     /* CodeObject or Klass */
		int32 index;  /* variable */
	};

	Vector locvec;  /* save locvars for function and method */

	/* extra for compiler */
	Symbol *super;
	Symbol *up;
	void *ptr;      /* CodeBlock and STable(import, class, trait) */
	char *path;     /* used for import */
	void *import;   /* save Import */
	int32 locvars;  /* used in compiler, for function */
	Vector traits;  /* for traits in correct order */
};

/* Exported APIs */
Symbol *Symbol_New(int kind);
void Symbol_Free(Symbol *sym);
Symbol *STable_Add_Var(STable *stbl, char *name, TypeDesc *desc, int bconst);
Symbol *STable_Add_Proto(STable *stbl, char *name, TypeDesc *proto);
#define STable_Add_IProto(stbl, name, proto) ({ \
	Symbol *__sym = STable_Add_Proto(stbl, name, proto); \
	if (__sym) \
		__sym->kind = SYM_IPROTO; \
	__sym; \
})
Symbol *STable_Add_TypeAlias(STable *stbl, char *name, TypeDesc *desc);
#define STable_Add_Class(stbl, name) \
	STable_Add_Symbol(stbl, name, SYM_CLASS, 0)
#define STable_Add_Trait(stbl, name) \
	STable_Add_Symbol(stbl, name, SYM_TRAIT, 0)
Symbol *STable_Add_Symbol(STable *stbl, char *name, int kind, int bconst);
Symbol *STable_Get(STable *stbl, char *name);
typedef void (*symbolfunc)(Symbol *sym, void *arg);
void STable_Traverse(STable *stbl, symbolfunc fn, void *arg);
void STable_Show(STable *stbl, int detail);
#define STable_Count(stbl) HashTable_Count((stbl)->htbl)
int STable_Update_Symbol(STable *stbl, Symbol *sym, TypeDesc *desc);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_SYMBOL_H_ */
