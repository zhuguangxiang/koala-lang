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

static void code_in_mod_cls(ParserState *ps, void *arg)
{
  ParserUnit *u = ps->u;
  IdentExpr *exp = arg;
  Symbol *sym = exp->sym;
  TypeDesc *desc = exp->desc;
  ExprCtx ctx = exp->ctx;
  Expr *right = exp->right;

  /* id, in module/class, is in variable/field declaration's expr */
  assert(ctx == EXPR_LOAD);
  if (sym->kind == SYM_CONST || sym->kind == SYM_VAR) {
    Log_Debug("id '%s' is const/var", sym->name);
    if (desc->kind == TYPE_PROTO && Expr_Is_Call(right)) {
      /* id is const/var, but it's a func reference, call function */
      Log_Debug("call '%s' function(var)", sym->name);
      int argc = Vector_Size(((CallExpr *)right)->args);
      CODE_CALL(u->block, sym->name, argc);
    } else {
      /* load variable */
      Log_Debug("load '%s' variable", sym->name);
      assert(ctx == EXPR_LOAD);
      CODE_GET_ATTR(u->block, sym->name);
    }
  } else {
    assert(sym->kind == SYM_FUNC);
    Log_Debug("id '%s' is function", sym->name);
    if (Expr_Is_Call(right)) {
      /* call function */
      Log_Debug("call '%s' function", sym->name);
      int argc = Vector_Size(((CallExpr *)right)->args);
      CODE_CALL(u->block, sym->name, argc);
    } else {
      /* load function */
      Log_Debug("load '%s' function", sym->name);
      CODE_GET_ATTR(u->block, sym->name);
    }
  }
}

static void code_current_function_block_closure(ParserState *ps, void *arg)
{
  ParserUnit *u = ps->u;
  IdentExpr *exp = arg;
  Symbol *sym = exp->sym;
  TypeDesc *desc = exp->desc;
  ExprCtx ctx = exp->ctx;
  Expr *right = exp->right;

  /*
   * expr, in function/block/closure scope, is local variable or parameter
   * it's context is load or store
   */
  assert(sym->kind == SYM_VAR);
  VarSymbol *varSym = (VarSymbol *)sym;
  Log_Debug("id '%s' is variable", varSym->name);
  if (desc->kind == TYPE_PROTO && Expr_Is_Call(right)) {
    /* id is const/var, but it's a func reference, call function */
    Log_Debug("call '%s' function(var)", sym->name);
    assert(ctx == EXPR_LOAD);
    int argc = Vector_Size(((CallExpr *)right)->args);
    CODE_CALL(u->block, sym->name, argc);
  } else {
    /* load variable */
    assert(ctx == EXPR_LOAD || ctx == EXPR_STORE);
    if (ctx == EXPR_LOAD) {
      Log_Debug("load '%s' variable", varSym->name);
      CODE_LOAD(u->block, varSym->index);
    } else {
      Log_Debug("store '%s variable", varSym->name);
      CODE_STORE(u->block, varSym->index);
    }
  }
}

static void code_up_class(ParserState *ps, void *arg)
{
  ParserUnit *u = ps->u;
  IdentExpr *exp = arg;
  Symbol *sym = exp->sym;
  TypeDesc *desc = exp->desc;
  ExprCtx ctx = exp->ctx;
  Expr *right = exp->right;
  ParserUnit *uu = exp->scope;

  /*
   * up scope MUST be module
   * id, in class, is in field declaration's expr
   */
  Log_Debug("id '%s' is in module", sym->name);
  assert(uu->scope == SCOPE_MODULE);
  assert(ctx == EXPR_LOAD);
  if (sym->kind == SYM_CONST || sym->kind == SYM_VAR) {
    Log_Debug("id '%s' is const/var", sym->name);
    if (desc->kind == TYPE_PROTO && Expr_Is_Call(right)) {
      /* id is const/var, but it's a func reference, call function */
      Log_Debug("call '%s' function(var)", sym->name);
      int argc = Vector_Size(((CallExpr *)right)->args);
      //FIXME:
      //CODE_PKG_CALL(u->block, sym->name, argc);
    } else {
      /* load variable */
      Log_Debug("load '%s' variable", sym->name);
      //FIXME:
      //CODE_PKG_GETFIELD(u->block, sym->name);
    }
  } else {
    assert(sym->kind == SYM_FUNC);
    Log_Debug("id '%s' is function", sym->name);
    if (Expr_Is_Call(right)) {
      /* call function */
      Log_Debug("call '%s' function", sym->name);
      int argc = Vector_Size(((CallExpr *)right)->args);
      //FIXME:
      //CODE_PKG_CALL(u->block, sym->name, argc);
    } else {
      /* load function */
      Log_Debug("load '%s' function", sym->name);
      //FIXME:
      //CODE_PKG_GETFIELD(u->block, sym->name);
    }
  }
}

static void code_up_func(ParserState *ps, void *arg)
{
  ParserUnit *u = ps->u;
  IdentExpr *exp = arg;
  Symbol *sym = exp->sym;
  TypeDesc *desc = exp->desc;
  ExprCtx ctx = exp->ctx;
  Expr *right = exp->right;
  ParserUnit *up = exp->scope;
  ParserUnit *uu = Parser_Get_UpScope(ps);

  /* its up scope is module/class, maybe variable(field), eval or func */
  assert(up->scope == SCOPE_MODULE || up->scope == SCOPE_CLASS);
  if (sym->kind == SYM_CONST || sym->kind == SYM_VAR) {
    if (uu->scope == SCOPE_CLASS && up->scope == SCOPE_MODULE)
      CODE_LOAD_PKG(u->block, ".");
    else
      CODE_LOAD(u->block, 0);
    Log_Debug("id '%s' is const/var", sym->name);
    if (desc->kind == TYPE_PROTO && Expr_Is_Call(right)) {
      /* id is const/var, but it's a func reference, call function */
      Log_Debug("call '%s' function(var)", sym->name);
      int argc = Vector_Size(((CallExpr *)right)->args);
      CODE_CALL(u->block, sym->name, argc);
    } else {
      if (ctx == EXPR_LOAD) {
        /* load variable */
        Log_Debug("load '%s' variable", sym->name);
        CODE_GET_ATTR(u->block, sym->name);
      } else {
        assert(ctx == EXPR_STORE);
        Log_Debug("store '%s' variable", sym->name);
        CODE_SET_ATTR(u->block, sym->name);
      }
    }
  } else if (sym->kind == SYM_EVAL) {
    if (uu->scope == SCOPE_CLASS && up->scope == SCOPE_MODULE)
      CODE_LOAD_PKG(u->block, ".");
    else
      CODE_LOAD(u->block, 0);
    EnumSymbol *eSym = ((EnumValSymbol *)sym)->esym;
    CODE_GET_ATTR(u->block, eSym->name);
    Log_Debug("id '%s' is eval", sym->name);

    int argc = 0;
    if (Expr_Is_Call(right)) {
      if (((EnumValSymbol *)sym)->types == NULL) {
        Syntax_Error(&exp->pos,
                     "enum value '%s' has no associated types.", exp->name);
      } else {
        argc = Vector_Size(((CallExpr *)right)->args);
      }
    } else {
      if (((EnumValSymbol *)sym)->types != NULL) {
        TYPE_DECREF(exp->desc);
        exp->desc = NULL;
        Syntax_Error(&exp->pos,
                     "enum value '%s' have associated types.", exp->name);
      } else {
        exp->desc = New_EVal_Type((EnumValSymbol *)sym, NULL);
      }
    }

    if (ctx == EXPR_LOAD) {
      /* load enum value */
      Log_Debug("new eval '%s' with %d args", sym->name, argc);
      CODE_NEW_EVAL(u->block, sym->name, argc);
    } else {
      assert(ctx == EXPR_STORE);
      Syntax_Error(&exp->pos, "enum value '%s' is readonly.", exp->name);
    }
  } else if (sym->kind == SYM_ENUM) {
    Log_Debug("id '%s' is enum", sym->name);
    if (right == NULL) {
      Syntax_Error(&exp->pos,
                   "enum '%s' cannot as right expression.", exp->name);
    } else {
      if (uu->scope == SCOPE_CLASS && up->scope == SCOPE_MODULE)
        CODE_LOAD_PKG(u->block, ".");
      else
        CODE_LOAD(u->block, 0);
      CODE_GET_ATTR(u->block, sym->name);
    }
  } else if (sym->kind == SYM_CLASS) {
    Log_Debug("id '%s' is class", sym->name);
    assert(up->scope == SCOPE_MODULE);
    if (uu->scope == SCOPE_CLASS)
      CODE_LOAD_PKG(u->block, ".");
    else
      CODE_LOAD(u->block, 0);

    assert(ctx == EXPR_LOAD);
    CODE_NEW_OBJECT(u->block, sym->name);

    int argc = 0;
    if (Expr_Is_Call(right))
      argc = Vector_Size(((CallExpr *)right)->args);
    CODE_CALL(u->block, "__init__", argc);
  } else if (sym->kind == SYM_FUNC) {
    if (uu->scope == SCOPE_CLASS && up->scope == SCOPE_MODULE)
      CODE_LOAD_PKG(u->block, ".");
    else
      CODE_LOAD(u->block, 0);
    Log_Debug("id '%s' is function", sym->name);
    if (Expr_Is_Call(right)) {
      /* call function */
      Log_Debug("call '%s' function", sym->name);
      int argc = Vector_Size(((CallExpr *)right)->args);
      CODE_CALL(u->block, sym->name, argc);
    } else {
      /* load function */
      Log_Debug("load '%s' function", sym->name);
      CODE_GET_ATTR(u->block, sym->name);
    }
  } else {
    Syntax_Error(&exp->pos,"invalid expression '%s'", exp->name);
  }
}

static void code_up_block(ParserState *ps, void *arg)
{
  ParserUnit *u = ps->u;
  IdentExpr *exp = arg;
  Symbol *sym = exp->sym;
  TypeDesc *desc = exp->desc;
  ExprCtx ctx = exp->ctx;
  Expr *right = exp->right;
}

static void code_up_closure(ParserState *ps, void *arg)
{
  ParserUnit *u = ps->u;
  IdentExpr *exp = arg;
  Symbol *sym = exp->sym;
  TypeDesc *desc = exp->desc;
  ExprCtx ctx = exp->ctx;
  Expr *right = exp->right;
}

/* identifier is found in current scope */
static CodeGenerator current_codes[] = {
  {SCOPE_MODULE,   code_in_mod_cls},
  {SCOPE_CLASS,    code_in_mod_cls},
  {SCOPE_FUNCTION, code_current_function_block_closure},
  {SCOPE_BLOCK,    code_current_function_block_closure},
  {SCOPE_CLOSURE,  code_current_function_block_closure},
};

/*
 * identifier is found in up scope
 * It's impossible that ID is in SCOPE_MODULE, is it right?
 */
static CodeGenerator up_codes[] = {
  {SCOPE_MODULE,   NULL},
  {SCOPE_CLASS,    code_up_class},
  {SCOPE_FUNCTION, code_up_func},
  {SCOPE_BLOCK,    code_up_block},
  {SCOPE_CLOSURE,  code_up_closure},
};

#define code_generate(codes_array, _scope, ps, arg) \
({ \
  CodeGenerator *gen = codes_array; \
  for (int i = 0; i < nr_elts(codes_array); i++) { \
    if (_scope == gen->scope) { \
      if (gen->code != NULL) \
        gen->code(ps, arg); \
      break; \
    } \
    gen++; \
  } \
})

static void code_extdot_ident(ParserState *ps, IdentExpr *exp)
{
  ParserUnit *u = ps->u;
  RefSymbol *refSym = (RefSymbol *)exp->sym;
  Symbol *sym = refSym->sym;
  TypeDesc *desc = exp->desc;
  ExprCtx ctx = exp->ctx;
  Expr *right = exp->right;

  CODE_LOAD_PKG(u->block, refSym->path);

  if (sym->kind == SYM_CONST) {

  } else if (sym->kind == SYM_VAR) {

  } else if (sym->kind == SYM_FUNC || sym->kind == SYM_NFUNC) {
    Log_Debug("id '%s' is function", sym->name);
    if (Expr_Is_Call(right)) {
      /* call function */
      Log_Debug("call '%s' function", sym->name);
      int argc = Vector_Size(((CallExpr *)right)->args);
      CODE_CALL(u->block, sym->name, argc);
    } else {
      /* load function */
      Log_Debug("load '%s' function", sym->name);
      CODE_GET_ATTR(u->block, sym->name);
    }
  } else {
    assert(0);
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
  assert(exp->kind == ID_KIND);
  ParserUnit *u = ps->u;
  IdentExpr *idExp = (IdentExpr *)exp;
  char *name = idExp->name;
  Symbol *sym;
  int depth;

  assert(idExp->sym == NULL);
  assert(idExp->desc == NULL);

  /* find ident from current scope */
  sym = STable_Get(u->stbl, name);
  if (sym != NULL) {
    depth = ps->depth;
    Log_Debug("find symbol '%s' in local scope-%d(%s)",
              name, depth, scope_name(u));
    sym->used++;
    idExp->sym = sym;
    idExp->desc = sym->desc;
    TYPE_INCREF(idExp->desc);
    idExp->where = CURRENT_SCOPE;
    idExp->scope = u;
    return;
  }

  /* find ident from up scope */
  ParserUnit *uu;
  depth = ps->depth;
  list_for_each_entry(uu, &ps->ustack, link) {
    depth -= 1;
    sym = STable_Get(uu->stbl, name);
    if (sym != NULL) {
      Log_Debug("find symbol '%s' in up scope-%d(%s)",
                name, depth, scope_name(uu));
      sym->used++;
      idExp->sym = sym;
      idExp->desc = sym->desc;
      TYPE_INCREF(idExp->desc);
      idExp->where = UP_SCOPE;
      idExp->scope = uu;
      return;
    }
  }

  /* find ident from external scope (imported) */
  sym = STable_Get(ps->extstbl, name);
  if (sym != NULL) {
    Log_Debug("find symbol '%s' as imported name", name);
    sym->used++;
    idExp->sym = sym;
    idExp->desc = sym->desc;
    TYPE_INCREF(idExp->desc);
    idExp->where = EXT_SCOPE;
    idExp->scope = NULL;
    return;
  }

  /* find ident from external scope (imported dot) */
  sym = STable_Get(ps->extdots, name);
  if (sym != NULL) {
    Log_Debug("find symbol '%s' in imported symbols", name);
    sym->used++;
    RefSymbol *refSym = (RefSymbol *)sym;
    refSym->sym->used++;
    idExp->sym = sym;
    idExp->desc = refSym->sym->desc;
    TYPE_INCREF(idExp->desc);
    idExp->where = EXTDOT_SCOPE;
    idExp->scope = NULL;
    return;
  }

  Syntax_Error(&exp->pos, "cannot find symbol '%s'", name);
}

void Code_Ident_Expr(ParserState *ps, Expr *exp)
{
  assert(exp->kind == ID_KIND);
  ParserUnit *u = ps->u;
  IdentExpr *idExp = (IdentExpr *)exp;
  if (idExp->where == CURRENT_SCOPE) {
    /* current scope */
    assert(idExp->desc != NULL && idExp->sym != NULL);
    code_generate(current_codes, u->scope, ps, idExp);
  } else if (idExp->where == UP_SCOPE) {
    /* up scope */
    code_generate(up_codes, u->scope, ps, idExp);
  } else if (idExp->where == EXT_SCOPE) {
    /* external scope */
    assert(idExp->sym != NULL);
    Package *pkg = ((PkgSymbol *)idExp->sym)->pkg;
    CODE_LOAD_PKG(u->block, pkg->path);
  } else if (idExp->where == EXTDOT_SCOPE) {
    /* external dot imported */
    code_extdot_ident(ps, idExp);
  } else {
    assert(0);
  }
}
