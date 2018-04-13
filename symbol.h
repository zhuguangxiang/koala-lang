
#ifndef _KOALA_SYMBOL_H_
#define _KOALA_SYMBOL_H_

#include "klc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct symboltable {
	HashTable *htbl;
	AtomTable *atbl;
	int flag;
	int32 varcnt;
} STable;

/* Exported APIs */
STable *STable_New(AtomTable *atbl);
void STable_Free(STable *stbl);
int STable_Init(STable *stbl, AtomTable *atbl);
void STable_Fini(STable *stbl);

/*-------------------------------------------------------------------------*/

#define SYM_VAR     1
#define SYM_PROTO   2
#define SYM_CLASS   3
#define SYM_INTF    4
#define SYM_TRAIT   8
#define SYM_IPROTO  5
#define SYM_STABLE  6  /* for compiler */
#define SYM_MODULE  7

#define ACCESS_PUBLIC   0
#define ACCESS_PRIVATE  (1 << 0)
#define ACCESS_CONST    (1 << 1)

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
	Vector supers;
	Symbol *up;
	void *ptr;      /* CodeBlock and STable(import, class, interface) */
	char *path;     /* used for import */
	int32 locvars;  /* used in compiler, for function */
	HashTable *table; /* save base interfaces, for interface inheritance*/
};

/* Exported APIs */
Symbol *Symbol_New(int kind);
void Symbol_Free(Symbol *sym);
Symbol *STable_Add_Var(STable *stbl, char *name, TypeDesc *desc, int bconst);
Symbol *STable_Add_Proto(STable *stbl, char *name, Proto *proto);
#define STable_Add_IProto(stbl, name, proto) ({ \
	Symbol *__sym = STable_Add_Proto(stbl, name, proto); \
	if (__sym) __sym->kind = SYM_IPROTO; \
	__sym; \
})
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
#define SYMBOL_ACCESS(name, bconst) ({ \
	int access = isupper(name[0]) ? ACCESS_PRIVATE : ACCESS_PUBLIC; \
	access |= bconst ? ACCESS_CONST : 0; \
	access; \
})

typedef struct base_symbol {
	HashNode hnode;
	Symbol *sym;
} BaseSymbol;

int Symbol_Add_Base(Symbol *sym, Symbol *base);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_SYMBOL_H_ */
