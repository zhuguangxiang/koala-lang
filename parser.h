
#ifndef _KOALA_PARSER_H_
#define _KOALA_PARSER_H_

#include "ast.h"
#include "codeblock.h"

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

KImage *Compile(FILE *in);
void Parse_Statements(ParserState *ps, Vector *stmts);

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
