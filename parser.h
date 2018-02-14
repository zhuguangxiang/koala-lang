
#ifndef _KOALA_PARSER_H_
#define _KOALA_PARSER_H_

#include "hashtable.h"
#include "object.h"
#include "ast.h"

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

typedef struct code {
  struct list_head link;
  uint8 op;
  TValue arg;
} Code;

typedef struct codeblock {
  char *name; /* for debugging */
  struct list_head link;
  STable stbl;
  struct list_head codes;
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
} ParserUnit;

typedef struct parserstate {
  char *outfile;
  char *package;
  HashTable imports;  /* external types */
  STable extstbl;   /* external symbol table */
  ParserUnit *u;
  int nestlevel;
  struct list_head ustack;
  Vector errors;
} ParserState;

char *userdef_get_path(ParserState *ps, char *mod);
void parse_module(ParserState *parser, struct mod *mod);
Symbol *parse_import(ParserState *parser, char *id, char *path);
void parse_vardecl(ParserState *parser, struct stmt *stmt);
void parse_funcdecl(ParserState *parser, struct stmt *stmt);
void parse_typedecl(ParserState *parser, struct stmt *stmt);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_PARSER_H_ */
