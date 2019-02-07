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
#include "log.h"

LOGGER(0)

/* symbol is found in current scope */
static void parse_ident_current_scope(ParserUnit *u, Symbol *sym, Expr *exp)
{
  switch (u->scope) {
  case SCOPE_MODULE:
  case SCOPE_CLASS:
    /* expr, in module scope, is variable declaration's right expr */
    /* expr, in class scope, is field declaration's right expr */
    assert(exp->ctx == EXPR_LOAD);
    if (sym->kind == SYM_CONST || sym->kind == SYM_VAR) {
      Log_Debug("symbol '%s' is variable(constant)", sym->name);
      if (exp->desc == NULL) {
        exp->desc = ((VarSymbol *)sym)->desc;
        TYPE_INCREF(exp->desc);
      }

      Expr *right = exp->right;
      if (exp->desc->kind == TYPE_PROTO &&
          right != NULL && right->kind == CALL_KIND) {
        /* call function(variable) */
        /* FIXME: anonymous */
        Log_Debug("call '%s' function(variable)", sym->name);
        Inst_Append_NoArg(u->block, OP_LOAD0);
        Argument val = {.kind = ARG_STR, .str = sym->name};
        Inst *i = Inst_Append(u->block, OP_CALL, &val);
        i->argc = exp->argc;
      } else {
        /* load variable */
        Log_Debug("load '%s' variable", sym->name);
        Inst_Append_NoArg(u->block, OP_LOAD0);
        Argument val = {.kind = ARG_STR, .str = sym->name};
        Inst_Append(u->block, OP_GETFIELD, &val);
      }
    } else {
      assert(sym->kind == SYM_FUNC);
      Log_Debug("symbol '%s' is function", sym->name);
      exp->desc = ((FuncSymbol *)sym)->desc;
      TYPE_INCREF(exp->desc);

      Expr *right = exp->right;
      if (right != NULL && right->kind == CALL_KIND) {
        /* call function */
        Log_Debug("call '%s' function", sym->name);
        Inst_Append_NoArg(u->block, OP_LOAD0);
        Argument val = {.kind = ARG_STR, .str = sym->name};
        Inst *i = Inst_Append(u->block, OP_CALL, &val);
        i->argc = exp->argc;
      } else {
        /* load function */
        Log_Debug("load '%s' function", sym->name);
        Inst_Append_NoArg(u->block, OP_LOAD0);
        Argument val = {.kind = ARG_STR, .str = sym->name};
        Inst_Append(u->block, OP_GETFIELD, &val);
      }
    }
    break;
  case SCOPE_FUNCTION:
  case SCOPE_BLOCK:
  case SCOPE_CLOSURE:
    /*
     * expr, in function scope, is local variable or parameter
     * expr, in block scope, is local variable
     * expr, in closure scope, is local variable
     * load or store
     */
    assert(sym->kind == SYM_VAR);
    VarSymbol *varSym = (VarSymbol *)sym;
    Log_Debug("symbol '%s' is local variable(parameter)", varSym->name);
    exp->desc = varSym->desc;
    TYPE_INCREF(exp->desc);

    Expr *right = exp->right;
    if (exp->desc->kind == TYPE_PROTO &&
        right != NULL && right->kind == CALL_KIND) {
      /* call function(variable) */
      /* FIXME: anonymous */
      Log_Debug("call '%s' function(variable)", varSym->name);
      assert(exp->ctx == EXPR_LOAD);
      Inst_Append_NoArg(u->block, OP_LOAD0);
      Argument val = {.kind = ARG_STR, .str = varSym->name};
      Inst *i = Inst_Append(u->block, OP_CALL, &val);
      i->argc = exp->argc;
    } else {
      /* load variable */
      assert(exp->ctx == EXPR_LOAD || exp->ctx == EXPR_STORE);
      if (exp->ctx == EXPR_LOAD)
        Log_Debug("load '%s' variable", varSym->name);
      else
        Log_Debug("store '%s' variable", varSym->name);
      int opcode = (exp->ctx == EXPR_LOAD) ? OP_LOAD : OP_STORE;
      Argument val = {.kind = ARG_INT, .ival = varSym->index};
      Inst_Append(u->block, opcode, &val);
    }
    break;
  default:
    assert(0);
    break;
  }
}

/*
  a.b.c.attribute
  a.b.c()
  a.b.c[10]
  a.b.c[1:10]
  leftmost identifier is variable or imported external package name
 */
void Parse_Ident_Expr(ParserState *ps, Expr *exp)
{
  ParserUnit *u = ps->u;
  BaseExpr *baseExp = (BaseExpr *)exp;
  char *name = baseExp->value.str;
  Symbol *sym;

  /* find ident from current scope */
  sym = STable_Get(u->stbl, name);
  if (sym != NULL) {
    Log_Debug("find symbol '%s' in local scope-%d(%s)",
              name, ps->depth, scope_strings[u->scope]);
    exp->sym = sym;
    parse_ident_current_scope(u, sym, exp);
    return;
  }

  Syntax_Error(ps, &exp->pos, "cannot find symbol '%s'", name);
}

void Code_Ident_Expr(ParserState *ps, Expr *exp)
{

}
