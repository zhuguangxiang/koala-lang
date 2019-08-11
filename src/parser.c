/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "parser.h"

void parse_expr(ParserState *ps, Expr *exp)
{
  /* if errors is greater than MAX_ERRORS, stop parsing */
  if (ps->errnum > MAX_ERRORS)
    return;
}