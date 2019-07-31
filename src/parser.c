/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "parser.h"
#include "ast.h"

typedef void (*stmt_parser)(struct parser_state *, struct stmt *);

void parse_stmt(struct parser_state *ps, struct stmt *stmt)
{
  if (ps->errnum >= MAX_ERRORS)
    return;
  int nr = sizeof(stmt_parsers)/sizeof(stmt_parsers[0]);
  panic(stmt->kind <= 0 || stmt->kind >= nr,
        "invalid statement '%d'", stmt->kind);
  stmt_parser fn =  stmt_parsers[stmt->kind];
  fn(ps, stmt);
}
