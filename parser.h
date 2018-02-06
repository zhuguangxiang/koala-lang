
#ifndef _KOALA_PARSER_H_
#define _KOALA_PARSER_H_

#include "hashtable.h"
#include "symbol.h"
#include "object.h"
#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct error {
  char *msg;
  int line;
} Error;

typedef struct instr {
  uint8 opcode;
  uint32 oparg;
} Instr;

typedef struct codeblock {
  struct list_head link;
  /* upper code block for seraching symbols */
  struct codeblock *up;
  /* symbol table */
  STable stbl;
  /* struct instr list */
  Vector instvec;
  /* true if a OP_RET opcode is inserted. */
  int bret;
} CodeBlock;

enum {
  SCOPE_MODULE = 1,
  SCOPE_CLASS,
  SCOPE_FUNCTION,
  SCOPE_BLOCK,
};

typedef struct parserunit {
  int scope;
  struct list_head link;
  ProtoInfo proto;
  STable stbl;
  CodeBlock *block;
  struct list_head blocks;
} ParserUnit;

typedef struct parserstate {
  char *package;
  STable extstbl; /* external symbol table */
  ParserUnit *u;
  int nestlevel;
  struct list_head ustack;
  Vector errors;
} ParserState;

void parse_module(ParserState *parser, struct mod *mod);
Symbol *parse_import(ParserState *parser, char *id, char *path);
void parse_vardecl(ParserState *parser, struct stmt *stmt);
void parse_funcdecl(ParserState *parser, struct stmt *stmt);
void parse_typedecl(ParserState *parser, struct stmt *stmt);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_PARSER_H_ */
