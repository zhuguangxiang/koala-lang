
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
STable *STbl_New(AtomTable *atbl);
void STbl_Free(STable *stbl);
int STbl_Init(STable *stbl, AtomTable *atbl);
void STbl_Fini(STable *stbl);

/*-------------------------------------------------------------------------*/

#define SYM_VAR     1
#define SYM_PROTO   2
#define SYM_CLASS   3
#define SYM_INTF    4
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
	int8 unused;

	char *name;     /* ->nameidx */
	TypeDesc *desc; /* ->descidx */

	union {
		void *ob;     /* CodeObject or Klass */
		int32 index;  /* variable */
	};

	/* extra for compiler */
	Symbol *up;
	void *ptr;      /* CodeBlock and STable(import, class, interface) */
	char *path;     /* used for import */
	int32 locvars;  /* used in compiler, for function */

};

/* Exported APIs */
Symbol *Symbol_New(void);
void Symbol_Free(Symbol *sym);
Symbol *STbl_Add_Var(STable *stbl, char *name, TypeDesc *desc, int bconst);
Symbol *STbl_Add_Proto(STable *stbl, char *name, Proto *proto);
#define STbl_Add_IProto(stbl, name, proto) ({ \
	Symbol *__sym = STbl_Add_Proto(stbl, name, proto); \
	if (__sym) __sym->kind = SYM_IPROTO; \
	__sym; \
})
#define STbl_Add_Class(stbl, name) STbl_Add_Symbol(stbl, name, SYM_CLASS, 0)
#define STbl_Add_Intf(stbl, name) STbl_Add_Symbol(stbl, name, SYM_INTF, 0)
Symbol *STbl_Add_Symbol(STable *stbl, char *name, int kind, int bconst);
Symbol *STbl_Get(STable *stbl, char *name);
typedef void (*symbolfunc)(Symbol *sym, void *arg);
void STbl_Traverse(STable *stbl, symbolfunc fn, void *arg);
void STbl_Show(STable *stbl, int detail);
#define STbl_Count(stbl) HTable_Count((stbl)->htbl)
int STbl_Update_Symbol(STable *stbl, Symbol *sym, TypeDesc *desc);
#define SYMBOL_ACCESS(name, bconst) ({ \
	int access = isupper(name[0]) ? ACCESS_PRIVATE : ACCESS_PUBLIC; \
	access |= bconst ? ACCESS_CONST : 0; \
	access; \
})

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_SYMBOL_H_ */
