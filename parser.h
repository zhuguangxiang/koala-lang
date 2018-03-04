
#ifndef _KOALA_PARSER_H_
#define _KOALA_PARSER_H_

#include "ast.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct codeblock {
	struct list_head link;
	STable stbl;
	int bytes;
	struct list_head insts;
	struct codeblock *next;  /* control flow */
	 /* true if a OP_RET opcode is inserted. */
	int bret;
} CodeBlock;

typedef struct inst {
	struct list_head link;
	int bytes;
	uint8 op;
	TValue arg;
} Inst;

void codeblock_free(CodeBlock *b);

/*-------------------------------------------------------------------------*/

typedef struct import {
	HashNode hnode;
	char *path;
	Symbol *sym;
} Import;

typedef struct error {
	char *msg;
	int line;
} Error;

enum scope {
	SCOPE_MODULE = 1,
	SCOPE_CLASS,
	SCOPE_FUNCTION,
	SCOPE_METHOD,
	SCOPE_CLOSURE,
	SCOPE_BLOCK
};

typedef struct parserunit {
	enum scope scope;
	struct list_head link;
	Symbol *sym;
	STable *stbl;
	CodeBlock *block;
	struct list_head blocks;
} ParserUnit;

typedef struct parserstate {
	Vector stmts;       /* all statements */
	char *package;
	HashTable imports;  /* external types */
	STable *extstbl;    /* external symbol table */
	STable *stbl;       /* current module's symbol table */
	ParserUnit *u;
	int nestlevel;
	struct list_head ustack;
	int olevel;         /* optimization level */
	Vector errors;
} ParserState;

void init_parser(ParserState *ps);
void fini_parser(ParserState *ps);
void parser_module(ParserState *ps, FILE *in);

// API used by yacc
Symbol *Parse_Import(ParserState *ps, char *id, char *path);
void Parse_VarDecls(ParserState *ps, struct stmt *stmt);
void Parse_Proto(ParserState *ps, struct stmt *stmt);
void Parse_UserDef(ParserState *ps, struct stmt *stmt);
char *Import_Get_Path(ParserState *ps, char *id);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_PARSER_H_ */
