
#ifndef _KOALA_PARSER_H_
#define _KOALA_PARSER_H_

#include "ast.h"
#include "hashtable.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct import {
	HashNode hnode;
	char *path;
	Symbol *sym;
} Import;

typedef struct error {
	char *msg;
	int line;
} Error;

typedef struct inst {
	struct list_head link;
	uint8 op;
	TValue arg;
} Inst;

typedef struct codeblock {
	char *name; /* for debugging */
	struct list_head link;
	STable stbl;
	struct list_head insts;
	 /* true if a OP_RET opcode is inserted. */
	int bret;
} CodeBlock;

enum {
	SCOPE_MODULE = 1,
	SCOPE_CLASS,
	SCOPE_FUNCTION,
	SCOPE_BLOCK
};

typedef struct parserunit {
	int scope;
	struct list_head link;
	Symbol *sym;
	STable stbl;
	CodeBlock *block;
	struct list_head blocks;
} ParserUnit;

typedef struct parserstate {
	Vector stmts;       /* all statements */
	char *outfile;
	char *package;
	HashTable imports;  /* external types */
	STable extstbl;     /* external symbol table */
	ParserUnit *u;
	int nestlevel;
	struct list_head ustack;
	ParserUnit mu;      /* module parser unit */
	int olevel;         /* optimization level */
	int gencode;        /* for code generator */
	Vector errors;
} ParserState;

char *UserDef_Get_Path(ParserState *ps, char *mod);
Symbol *Parse_Import(ParserState *ps, char *id, char *path);
void Parse_VarDecls(ParserState *ps, struct stmt *stmt);
void Parse_Proto(ParserState *ps, struct stmt *stmt);
void parse_typedecl(ParserState *ps, struct stmt *stmt);
void Parse_Body(ParserState *ps, Vector *stmts);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_PARSER_H_ */
