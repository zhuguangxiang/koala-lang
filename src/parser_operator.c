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
#include "stringex.h"
#include "log.h"

LOGGER(0)

/*
 * optimization:
 *  1. literal constant
 *  2. constant variable
 *  3. unchanged variable(local variable)
 */

static const char *binary_strings[] = {
  "<UNKNOWN>", "BINARY_ADD", "BINARY_SUB", "BINARY_MULT", "BINARY_DIV"
};

/* check type has this operator or not */
static int __check_binary_expr(ParserState *ps, BinaryExpr *exp)
{
  /* FIXME */
  return 1;
}

typedef void (*Optimize)(ParserState *, BinaryExpr *);

static BaseExpr *__get_baseexp(Expr *exp)
{
  ExprKind kind = exp->kind;

  if (kind == BINARY_KIND) {
    BinaryExpr *binExp = (BinaryExpr *)exp;
    assert(binExp->val != NULL);
    return binExp->val;
  } else if (kind == UNARY_KIND) {
    UnaryExpr *unExp = (UnaryExpr *)exp;
    assert(unExp->val != NULL);
    return unExp->val;
  } else {
    assert(kind == INT_KIND || kind == FLOAT_KIND || kind == BOOL_KIND ||
           kind == STRING_KIND || kind == CHAR_KIND);
    return (BaseExpr *)exp;
  }
}

static void __optimize_int_add(ParserState *ps, BinaryExpr *exp)
{
  BaseExpr *lexp = __get_baseexp(exp->lexp);
  BaseExpr *rexp = __get_baseexp(exp->rexp);

  int64 val;
  switch (rexp->kind) {
  case INT_KIND:
    val = lexp->ival + rexp->ival;
    Log_Printf("%lld = (+ %lld %lld)\n", val, lexp->ival, rexp->ival);
    exp->val = (BaseExpr *)Expr_From_Integer(val);
    break;
  case FLOAT_KIND:
    val = lexp->ival + rexp->fval;
    Log_Printf("%lld = (+ %lld %.15f)\n", val, lexp->ival, rexp->fval);
    exp->val = (BaseExpr *)Expr_From_Integer(val);
    break;
  case BOOL_KIND:
    Syntax_Error(ps, &rexp->pos, "cannot convert '%s'(bool) to int",
                 rexp->bval ? "true" : "false");
    break;
  case STRING_KIND: {
    int err = string_to_int64(rexp->str, &val);
    if (!err) {
      val = lexp->ival + val;
      Log_Printf("%lld = (+ %lld \"%s\")\n", val, lexp->ival, rexp->str);
      exp->val = (BaseExpr *)Expr_From_Integer(val);
    } else {
      Syntax_Error(ps, &rexp->pos, "cannot convert '%s'(string) to int",
                   rexp->str);
    }
    break;
  }
  case CHAR_KIND:
    val = lexp->ival + rexp->ch.val;
    Log_Printf("%lld = (+ %lld %s)\n", val, lexp->ival, rexp->ch.data);
    exp->val = (BaseExpr *)Expr_From_Integer(val);
    break;
  case ID_KIND:
    break;
  default:
    assert(0);
    break;
  }
}

static void __optimize_int_sub(ParserState *ps, BinaryExpr *exp)
{
  BaseExpr *lexp = __get_baseexp(exp->lexp);
  BaseExpr *rexp = __get_baseexp(exp->rexp);

  switch (rexp->kind) {
  case INT_KIND: {
    int64 val = lexp->ival - rexp->ival;
    Log_Printf("(- %lld %lld)\n", lexp->ival, rexp->ival);
    exp->val = (BaseExpr *)Expr_From_Integer(val);
    break;
  }
  case FLOAT_KIND:
    break;
  case BOOL_KIND:
    break;
  case STRING_KIND:
    break;
  case CHAR_KIND:
    break;
  case ID_KIND:
    break;
  default:
    assert(0);
    break;
  }
}

static void __optimize_int_mul(ParserState *ps, BinaryExpr *exp)
{
  BaseExpr *lexp = __get_baseexp(exp->lexp);
  BaseExpr *rexp = __get_baseexp(exp->rexp);

  switch (rexp->kind) {
  case INT_KIND:
    break;
  case FLOAT_KIND:
    break;
  case BOOL_KIND:
    break;
  case STRING_KIND:
    break;
  case CHAR_KIND:
    break;
  case ID_KIND:
    break;
  default:
    assert(0);
    break;
  }
}

static void __optimize_int_div(ParserState *ps, BinaryExpr *exp)
{
  BaseExpr *lexp = __get_baseexp(exp->lexp);
  BaseExpr *rexp = __get_baseexp(exp->rexp);

  switch (rexp->kind) {
  case INT_KIND:
    break;
  case FLOAT_KIND:
    break;
  case BOOL_KIND:
    break;
  case STRING_KIND:
    break;
  case CHAR_KIND:
    break;
  case ID_KIND:
    break;
  default:
    assert(0);
    break;
  }
}

static Optimize int_ops[] = {
  NULL,
  __optimize_int_add,
  __optimize_int_sub,
  __optimize_int_mul,
  __optimize_int_div
};

typedef struct const_binary_optimizer {
  int kind;
  int size;
  Optimize *ops;
} BinaryOptimizer;

static BinaryOptimizer biops[] = {
  { BASE_BYTE,   nr_elts(int_ops), int_ops },
  { BASE_CHAR,   nr_elts(int_ops), int_ops },
  { BASE_INT,    nr_elts(int_ops), int_ops },
  { BASE_FLOAT,  nr_elts(int_ops), int_ops },
  { BASE_BOOL,   nr_elts(int_ops), int_ops },
  { BASE_STRING, nr_elts(int_ops), int_ops },
};

/* optimize binary operation */
static inline void __optimize_binary_expr(ParserState *ps, BinaryExpr *exp)
{
  BaseDesc *desc = (BaseDesc *)exp->desc;
  BinaryOptimizer *optimizer;
  for (int i = 0; i < nr_elts(biops); i++) {
    optimizer = biops + i;
    if (optimizer->kind == desc->type) {
      assert(exp->op > 0 && exp->op < optimizer->size);
      optimizer->ops[exp->op](ps, exp);
      return;
    }
  }
}

/* binary operator */
void Parse_Binary_Expr(ParserState *ps, Expr *exp)
{
  BinaryExpr *binExp = (BinaryExpr *)exp;
  Log_Debug("binary: \x1b[34m%s\x1b[0m", binary_strings[binExp->op]);
  Expr *lexp = binExp->lexp;
  Expr *rexp = binExp->rexp;

  lexp->ctx = EXPR_LOAD;
  Parse_Expression(ps, lexp);
  assert(lexp->desc != NULL);

  rexp->ctx = EXPR_LOAD;
  Parse_Expression(ps, rexp);
  assert(rexp->desc != NULL);

  if (exp->desc == NULL) {
    char buf[64];
    TypeDesc_ToString(lexp->desc, buf);
    Log_Debug("update bianry expr's type:%s", buf);
    exp->desc = lexp->desc;
    TYPE_INCREF(exp->desc);
  }

  if (!__check_binary_expr(ps, binExp))
    return;

  if (Expr_Is_Const(lexp) && Expr_Is_Const(rexp))
    __optimize_binary_expr(ps, binExp);
}

void Code_Binary_Expr(ParserState *ps, Expr *exp)
{
  BinaryExpr *binExp = (BinaryExpr *)exp;
  Log_Debug("codegen binary: \x1b[34m%s\x1b[0m", binary_strings[binExp->op]);

  if (binExp->val != NULL) {
    Log_Debug("binary expr is optimized");
    binExp->val->ctx = EXPR_LOAD;
    Code_Expression(ps, (Expr *)binExp->val);
  }
}

/* unary operator */
void Parse_Unary_Expr(ParserState *ps, Expr *exp)
{
}

void Code_Unary_Expr(ParserState *ps, Expr *exp)
{

}
