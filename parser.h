
#ifndef _KOALA_PARSER_H_
#define _KOALA_PARSER_H_

#include "ast.h"
#include "object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct inst {
	struct list_head link;
	int bytes;
	int argc;
	uint8 op;
	TValue arg;
	int upbytes;  // break and continue statements
} Inst;

#define JMP_BREAK    1
#define JMP_CONTINUE 2

typedef struct jmp_inst {
	Inst *inst;
	int type;
} JmpInst;

typedef struct codeblock {
	//struct list_head link;
	int bytes;
	struct list_head insts;
	struct codeblock *next;  /* control flow */
	int bret;  /* false, no OP_RET, needs add one */
} CodeBlock;

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
	int8 merge;
	int8 loop;
	struct list_head link;
	Symbol *sym;
	STable *stbl;
	CodeBlock *block;
	Vector jmps;
} ParserUnit;

typedef struct parserstate {
	Vector stmts;       /* all statements */
	char *package;
	HashTable imports;  /* external types */
	STable *extstbl;    /* external symbol table */
	Symbol *sym;        /* current module's symbol */
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
