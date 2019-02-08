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

static ConstValue __get_constvalue(Expr *exp)
{
  ExprKind kind = exp->kind;

  if (kind == BINARY_KIND) {
    BinaryExpr *binExp = (BinaryExpr *)exp;
    return binExp->val;
  } else if (kind == UNARY_KIND) {
    UnaryExpr *unExp = (UnaryExpr *)exp;
    return unExp->val;
  } else if (kind == ID_KIND) {
    VarSymbol *sym = (VarSymbol *)exp->sym;
    assert(sym->kind == SYM_CONST);
    return sym->value;
  } else {
    assert(kind == CONST_KIND);
    return ((ConstExpr *)exp)->value;
  }
}

static void __optimize_int_add(ParserState *ps, BinaryExpr *exp)
{
  ConstValue lval = __get_constvalue(exp->lexp);
  ConstValue rval = __get_constvalue(exp->rexp);
  int64 val;

  switch (rval.kind) {
  case BASE_INT:
    val = lval.ival + rval.ival;
    Log_Printf("%lld = (+ %lld %lld)\n", val, lval.ival, rval.ival);
    break;
  case BASE_FLOAT:
    val = lval.ival + rval.fval;
    Log_Printf("%lld = (+ %lld %.15f)\n", val, lval.ival, rval.fval);
    break;
  case BASE_BOOL:
    val = 0;
    Syntax_Error(ps, &exp->rexp->pos, "cannot convert '%s'(bool) to int",
                 rval.bval ? "true" : "false");
    break;
  case BASE_STRING: {
    int err = string_to_int64(rval.str, &val);
    if (!err) {
      val = lval.ival + val;
      Log_Printf("%lld = (+ %lld \"%s\")\n", val, lval.ival, rval.str);
    } else {
      val = 0;
      Syntax_Error(ps, &exp->rexp->pos, "cannot convert '%s'(string) to int",
                   rval.str);
    }
    break;
  }
  case BASE_CHAR:
    val = lval.ival + rval.ch.val;
    Log_Printf("%lld = (+ %lld '%s')\n", val, lval.ival, rval.ch.data);
    break;
  default:
    val = 0;
    assert(0);
    break;
  }

  exp->val.kind = BASE_INT;
  exp->val.ival = val;
}

static void __optimize_int_sub(ParserState *ps, BinaryExpr *exp)
{
  ConstValue lval = __get_constvalue(exp->lexp);
  ConstValue rval = __get_constvalue(exp->rexp);
  int64 val;

  switch (rval.kind) {
  case BASE_INT:
    val = lval.ival - rval.ival;
    Log_Printf("%lld = (- %lld %lld)\n", val, lval.ival, rval.ival);
    break;
  case BASE_FLOAT:
    break;
  case BASE_BOOL:
    break;
  case BASE_STRING:
    break;
  case BASE_CHAR:
    break;
  default:
    assert(0);
    break;
  }

  exp->val.kind = BASE_INT;
  exp->val.ival = val;
}

static void __optimize_int_mul(ParserState *ps, BinaryExpr *exp)
{
  ConstValue lval = __get_constvalue(exp->lexp);
  ConstValue rval = __get_constvalue(exp->rexp);
  int64 val;

  switch (rval.kind) {
  case BASE_INT:
    break;
  case BASE_FLOAT:
    break;
  case BASE_BOOL:
    break;
  case BASE_STRING:
    break;
  case BASE_CHAR:
    break;
  default:
    assert(0);
    break;
  }
}

static void __optimize_int_div(ParserState *ps, BinaryExpr *exp)
{
  ConstValue lval = __get_constvalue(exp->lexp);
  ConstValue rval = __get_constvalue(exp->rexp);
  int64 val;

  switch (rval.kind) {
  case BASE_INT:
    break;
  case BASE_FLOAT:
    break;
  case BASE_BOOL:
    break;
  case BASE_STRING:
    break;
  case BASE_CHAR:
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

  if (binExp->val.kind != 0) {
    Log_Debug("binary expr is optimized");
    ConstExpr constExp = {.kind = CONST_KIND, .value = binExp->val};
    constExp.ctx = EXPR_LOAD;
    Code_Expression(ps, (Expr *)&constExp);
  }
}

/* unary operator */
void Parse_Unary_Expr(ParserState *ps, Expr *exp)
{
}

void Code_Unary_Expr(ParserState *ps, Expr *exp)
{

}
