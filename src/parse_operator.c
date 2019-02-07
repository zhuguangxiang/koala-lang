/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "parser.h"

/*
 * optimization:
 *  1. literal constant
 *  2. constant variable
 *  3. unchanged variable(local variable)
 */

typedef void (*mark_expr_func)(ParserState *, Expr *);

static mark_expr_func mark_expr_funcs[] = {
  NULL,
  NULL,
  NULL,
  NULL,

};

static void optimizer_visit_expr(ParserState *ps, Expr *exp)
{
  /* if errors is greater than MAX_ERRORS, stop parsing */
  if (ps->errnum >= MAX_ERRORS)
    return;

  switch (exp->kind) {
  case INT_KIND:
  case FLOAT_KIND:
  case BOOL_KIND:
  case STRING_KIND:
  case CHAR_KIND:
    //exp->konst = 1;
    break;
  case ID_KIND:
    /* maybe a constant */
    break;
  case UNARY_KIND:
    break;
  case BINARY_KIND:
    break;
  default:
    /* silently pass */
    break;
  }
}

/* binary operator */
void Parse_Binary_Expr(ParserState *ps, Expr *exp)
{
  BinaryExpr *binExp = (BinaryExpr *)exp;
}

void Code_Binary_Expr(ParserState *ps, Expr *exp)
{

}

/* unary operator */
void Parse_Unary_Expr(ParserState *ps, Expr *exp)
{
}

void Code_Unary_Expr(ParserState *ps, Expr *exp)
{

}
