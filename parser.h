
#ifndef _KOALA_PARSER_H_
#define _KOALA_PARSER_H_

#include "hashtable.h"
#include "symbol.h"
#include "object.h"
#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct import {
  HashNode hnode;
  char *id;
  char *path;
  STable *stable;
  int refcnt;
} Import;

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
  STable stable;
  /* struct instr list */
  Vector instvec;
  /* true if a OP_RET opcode is inserted. */
  int bret;
} CodeBlock;

typedef struct parserunit {
  struct list_head link;
  struct parserunit *up;
  STable stable;
  CodeBlock *block;
  struct list_head blocks;
} ParserUnit;

typedef struct parserstate {
  char *package;
  HashTable imports;
  ParserUnit *u;
  int scope;
  struct list_head ustack;
  Vector errors;
} ParserState;

extern ParserState parser;
void parse_module(ParserState *parser, struct mod *mod);
void parse_import(ParserState *parser, char *id, char *path);
void parse_vardecl(ParserState *parser, struct var *var);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_PARSER_H_ */
