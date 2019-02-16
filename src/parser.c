
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
#include "koala_lex.h"
#include "stringex.h"
#include "mem.h"
#include "log.h"

LOGGER(0)

void Parser_Set_PkgName(ParserState *ps, Ident *id)
{
  ps->pkgname = id->name;

  ParserGroup *grp = ps->grp;
  if (grp->pkg->pkgname == NULL) {
    grp->pkg->pkgname = id->name;
    return;
  }

  /* check all modules have the same package-name */
  if (strcmp(grp->pkg->pkgname, id->name)) {
    ParserState *m = Vector_Get(&grp->modules, 0);
    Syntax_Error(ps, &id->pos,
                 "found different packages %s(%s) and %s(%s)",
                 m->pkgname, m->filename, ps->pkgname, ps->filename);
  }
}

static ParserUnit *new_parser_unit(ScopeKind scope)
{
  ParserUnit *u = Malloc(sizeof(ParserUnit));
	init_list_head(&u->link);
	u->stbl = NULL;
	u->sym = NULL;
	u->block = CodeBlock_New();
	u->scope = scope;
	Vector_Init(&u->jmps);
  return u;
}

static void free_parser_unit(ParserUnit *u)
{
  assert(list_unlinked(&u->link));
  assert(u->sym == NULL);
  assert(u->stbl == NULL);
  assert(u->block == NULL);
  assert(Vector_Size(&u->jmps) == 0);
  Vector_Fini_Self(&u->jmps);
  Mfree(u);
}

static ParserUnit *up_parser_unit(ParserState *ps)
{
  struct list_head *pos = list_first(&ps->ustack);
  return (pos != NULL) ? container_of(pos, ParserUnit, link) : NULL;
}

static void save_locvar_func(Symbol *sym, void *arg)
{
  assert(sym->kind = SYM_VAR);
  Remove_HashNode(&sym->hnode);
  FuncSymbol *funcSym = arg;
  Vector_Append(&funcSym->locvec, sym);
}

static void merge_parser_unit(ParserState *ps)
{
  ParserUnit *u = ps->u;

  switch (u->scope) {
  case SCOPE_MODULE: {
    /* module has codes for __init__ */
    FuncSymbol *funcSym = (FuncSymbol *)STable_Get(u->stbl, "__init__");
    if (funcSym == NULL) {
      Log_Debug("create __init__ function");
      TypeDesc *proto = TypeDesc_Get_Proto(NULL, NULL);
      funcSym = STable_Add_Func(u->stbl, "__init__", proto);
      assert(funcSym != NULL);
      funcSym->code = u->block;
    } else {
      //FIXME: merge codeblock
      CodeBlock_Free(u->block);
    }
    u->block = NULL;
    u->stbl = NULL;
    break;
  }
  case SCOPE_CLASS: {
    ClassSymbol *clsSym = (ClassSymbol *)(u->sym);
    assert(clsSym != NULL);
    assert(clsSym->kind == SYM_CLASS || clsSym->kind == SYM_TRAIT);
    assert(u->stbl == clsSym->stbl);
    assert(u->block == NULL);
    u->sym = NULL;
    u->stbl = NULL;
    break;
  }
  case SCOPE_FUNCTION: {
    FuncSymbol *funcSym = (FuncSymbol *)u->sym;
    assert(funcSym && funcSym->kind == SYM_FUNC);
    CodeBlock *b = u->block;
    /* if no return opcode, then add one */
    if (!b->ret) {
      Log_Debug("add 'return' to function '%s'", funcSym->name);
      Inst_Append_NoArg(b, OP_RET);
    }
    /* save codeblock to function symbol */
    funcSym->code = u->block;
    u->block = NULL;
    /* free local symbol table */
    STable_Free(u->stbl);
    u->stbl = NULL;
    /* simple clear symbol, it's stored in module's symbol table */
    u->sym = NULL;
    break;
  }
  case SCOPE_BLOCK: {
    /* FIXME: if-else, while and switch-case */
    ParserUnit *uu = up_parser_unit(ps);
    Log_Debug("merge code into up scope-%d(%s)", ps->depth-1, scope_name(uu));
    CodeBlock_Merge(u->block, uu->block);
    CodeBlock_Free(u->block);
    u->block = NULL;
    STable_Free(u->stbl);
    u->stbl = NULL;
    break;
  }
  default:
    assert(0);
    break;
  }
}

static inline void show_module_symbol(Vector *vec)
{
  Symbol *sym;
  Vector_ForEach(sym, vec)
    Show_Symbol(sym);
}

static void show_parser_unit(ParserState *ps)
{
  ParserUnit *u = ps->u;
  const char *scope = scope_name(u);
  char *name = u->sym != NULL ? u->sym->name : NULL;
  name = (name == NULL) ? ps->filename : name;
  Log_Puts("\n----------------------------------------");
  Log_Printf("scope-%d(%s, %s) symbols:\n", ps->depth, scope, name);
  if (ps->depth == 1) {
    show_module_symbol(&ps->symbols);
  } else {
    STable_Show(u->stbl);
  }
  CodeBlock_Show(u->block);
  Log_Puts("----------------------------------------\n");
}

static void check_unused_symbols(ParserUnit *u)
{
  /* FIXME */
}

void Parser_Enter_Scope(ParserState *ps, ScopeKind scope)
{
  ParserUnit *u = new_parser_unit(scope);
  /* push old unit into stack */
  if (ps->u != NULL)
    list_add(&ps->u->link, &ps->ustack);
  ps->u = u;
  ps->depth++;

  Log_Debug("\x1b[35mEnter scope-%d(%s)\x1b[0m",
            ps->depth, scope_strings[scope]);
}

void Parser_Exit_Scope(ParserState *ps)
{
  show_parser_unit(ps);
  check_unused_symbols(ps->u);

  Log_Debug("\x1b[35mExit scope-%d(%s)\x1b[0m",
            ps->depth, scope_name(ps->u));

  merge_parser_unit(ps);

  /* free current parserunit */
  free_parser_unit(ps->u);
  ps->u = NULL;

  /* restore ps->u to top of ps->ustack */
  struct list_head *head = list_first(&ps->ustack);
  if (head != NULL) {
    list_del(head);
    ps->u = container_of(head, ParserUnit, link);
  }
  ps->depth--;
}

ParserState *New_Parser(ParserGroup *grp, char *filename)
{
  ParserState *ps = Malloc(sizeof(ParserState));
  ps->filename = AtomString_New(filename).str;
  ps->grp = grp;
  Vector_Init(&ps->stmts);
  ps->extstbl = STable_New();
  ps->extdots = STable_New();
  Vector_Init(&ps->symbols);
  Vector_Init(&ps->imports);
  init_list_head(&ps->ustack);
  Vector_Init(&ps->errors);
  return ps;
}

void Destroy_Parser(ParserState *ps)
{
  assert(ps->u == NULL);
  assert(list_empty(&ps->ustack));
  Vector_Fini(&ps->stmts, Free_Stmt_Func, NULL);
  STable_Free(ps->extstbl);
  STable_Free(ps->extdots);
  Vector_Fini_Self(&ps->symbols);
  Vector_Fini_Self(&ps->imports);
  Mfree(ps);
}

static int __type_is_compatible(TypeDesc *t1, TypeDesc *t2)
{
  /* FIXME: class inheritance */
  return TypeDesc_Equal(t1, t2);
}

static void code_const_expr(ParserState *ps, Expr *exp)
{
  assert(exp->ctx == EXPR_LOAD);
  ParserUnit *u = ps->u;
  ConstValue *val = &((ConstExpr *)exp)->value;
  Inst_Append(u->block, OP_LOADK, val);
}

static void parse_self_expr(ParserState *ps, Expr *exp)
{
  assert(0);
}

static void parse_super_expr(ParserState *ps, Expr *exp)
{
  assert(0);
}

static void parse_attribute_expr(ParserState *ps, Expr *exp)
{
  assert(0);
}

static void parse_subscript_expr(ParserState *ps, Expr *exp)
{
  assert(0);
}

static int __check_expr_types(Vector *descs, Vector *exps)
{
  /* FIXME: optimize this checker */
  int ndesc = Vector_Size(descs);
  int nexp = Vector_Size(exps);

  if (ndesc != nexp)
    return 0;

  Expr *e;
  TypeDesc *desc;
  Vector_ForEach(desc, descs) {
    e = Vector_Get(exps, i);
    if (!TypeDesc_Equal(desc, e->desc))
      return 0;
  }

  return 1;
}

static int __check_call_arguments(TypeDesc *desc, Vector *args)
{
  assert(desc->kind == TYPE_PROTO);
  ProtoDesc *proto = (ProtoDesc *)desc;
  return __check_expr_types(proto->arg, args);
}

static void parse_call_expr(ParserState *ps, Expr *exp)
{
  ParserUnit *u = ps->u;
  CallExpr *callExp = (CallExpr *)exp;
  Expr *lexp = callExp->left;

  if (lexp->kind == SUPER_KIND) {
    /*
     * call, like super(1, 2),
     * must be in subclass's __init__ func and is first statement
     */
    ParserUnit *uu = up_parser_unit(ps);
    if (u->scope != SCOPE_FUNCTION || uu->scope != SCOPE_CLASS ||
        strcmp(u->sym->name, "__init__") || !list_empty(&u->block->insts)) {
      Syntax_Error(ps, &lexp->pos,
                   "call super __init__ must be in subclass's __init__");
      return;
    }
  }

  /* parse argument-list */
  Expr *e;
  Vector_ForEach_Reverse(e, callExp->args) {
    e->ctx = EXPR_LOAD;
    Parse_Expression(ps, e);
  }

  /* parse left expression */
  lexp->ctx = EXPR_LOAD;
  lexp->right = exp;
  Parse_Expression(ps, lexp);
  if (lexp->sym == NULL)
    return;

  /* set call expression's descriptor as its left expr's descriptor */
  exp->desc = lexp->desc;
  TYPE_INCREF(exp->desc);

  /* set call expression's symbol as its left expr's symbol */
  exp->sym = lexp->sym;

  /*check call's arguments */
  SymKind kind = exp->sym->kind;
  TypeDesc *proto;
  if (kind == SYM_CLASS) {
    ClassSymbol *clsSym = (ClassSymbol *)exp->sym;
    Log_Debug("call class '%s' __init__ function", clsSym->name);
    Symbol *__init__ = STable_Get(clsSym->stbl, "__init__");
    int arguments = Vector_Size(callExp->args);
    if (__init__ == NULL && arguments > 0) {
      Syntax_Error(ps, &lexp->pos, "__init__ function needs no arguments");
      return;
    }
    proto = ((FuncSymbol *)__init__)->desc;
  } else {
    /* var(func type), func, ifunc and nfunc */
    Log_Debug("call var(proto)/interface/native function '%s'", exp->sym->name);
    proto = exp->desc;
  }

  assert(proto->kind == TYPE_PROTO);

  if (!__check_call_arguments(proto, callExp->args)) {
    Syntax_Error(ps, &lexp->pos,
                 "argument of function '%s' are not matched", exp->sym->name);
    return;
  }
}

static void code_call_expr(ParserState *ps, Expr *exp)
{

}

static void parse_slice_expr(ParserState *ps, Expr *exp)
{
  assert(0);
}

/* array, map and set intializer */
static void parse_list_expr(ParserState *ps, Expr *exp)
{
  assert(0);
}

static void parse_mapentry_expr(ParserState *ps, Expr *exp)
{
  assert(0);
}

static void parse_array_expr(ParserState *ps, Expr *exp)
{
  assert(0);
}

static void parse_map_expr(ParserState *ps, Expr *exp)
{
  assert(0);
}

static void parse_set_expr(ParserState *ps, Expr *exp)
{
  assert(0);
}

static void parse_anonymous_expr(ParserState *ps, Expr *exp)
{
  assert(0);
}

typedef void (*expr_func)(ParserState *, Expr *);

static struct expr_func_s {
  expr_func parse;
  expr_func code;
} expr_funcs[] = {
  { NULL, NULL },                                       /* INVALID         */
  { NULL, NULL },                                       /* NIL_KIND        */
  { parse_self_expr,      NULL },                       /* SELF_KIND       */
  { parse_super_expr,     NULL },                       /* SUPER_KIND      */
  { NULL,                 code_const_expr  },           /* CONST_KIND      */
  { Parse_Ident_Expr,     Code_Ident_Expr  },           /* ID_KIND         */
  { Parse_Unary_Expr,     Code_Unary_Expr  },           /* UNARY_KIND      */
  { Parse_Binary_Expr,    Code_Binary_Expr },           /* BINARY_KIND     */
  { parse_attribute_expr, NULL },                       /* ATTRIBUTE_KIND  */
  { parse_subscript_expr, NULL },                       /* SUBSCRIPT_KIND  */
  { parse_call_expr,      code_call_expr },             /* CALL_KIND       */
  { parse_slice_expr,     NULL },                       /* SLICE_KIND      */
  { parse_list_expr,      NULL },                       /* ARRAY_LIST_KIND */
  { parse_list_expr,      NULL },                       /* MAP_LIST_KIND   */
  { parse_mapentry_expr,  NULL },                       /* MAP_ENTRY_KIND  */
  { parse_array_expr,     NULL },                       /* ARRAY_KIND      */
  { parse_map_expr,       NULL },                       /* MAP_KIND        */
  { parse_set_expr,       NULL },                       /* SET_KIND        */
  { parse_anonymous_expr, NULL },                       /* ANONY_FUNC_KIND */
};

void Parse_Expression(ParserState *ps, Expr *exp)
{
  /* if errors is greater than MAX_ERRORS, stop parsing */
  if (ps->errnum > MAX_ERRORS)
    return;
  assert(exp->kind > 0 && exp->kind < nr_elts(expr_funcs));
  expr_func parse = (expr_funcs + exp->kind)->parse;
  if (parse != NULL)
    parse(ps, exp);
}

void Code_Expression(ParserState *ps, Expr *exp)
{
  /* if errors is greater than MAX_ERRORS, stop parsing */
  if (ps->errnum > MAX_ERRORS)
    return;
  assert(exp->kind > 0 && exp->kind < nr_elts(expr_funcs));
  expr_func code = (expr_funcs + exp->kind)->code;
  if (code != NULL)
    code(ps, exp);
}

/*
 * visiting expr has two stage:
 * 1. parse expr
 * 2. generate code
 */
static inline void parser_visit_expr(ParserState *ps, Expr *exp)
{
  assert(exp->kind > 0 && exp->kind < nr_elts(expr_funcs));
  struct expr_func_s *func = expr_funcs + exp->kind;
  if (ps->errnum <= MAX_ERRORS && func->parse != NULL)
    func->parse(ps, exp);
  if (ps->errnum <= MAX_ERRORS && func->code != NULL)
    func->code(ps, exp);
}

static ParserUnit *parse_block_variable(ParserState *ps, Ident *id)
{
  ParserUnit *uu;
  Symbol *sym;
  int depth = ps->depth;
  list_for_each_entry(uu, &ps->ustack, link) {
    depth -= 1;
    sym = STable_Get(uu->stbl, id->name);
    if (sym != NULL) {
      Syntax_Error(ps, &id->pos,
                   "var '%s' is already delcared in scope-%d(%s)",
                   id->name, depth, scope_name(uu));
      return NULL;
    }
    if (uu->scope == SCOPE_FUNCTION || uu->scope == SCOPE_CLOSURE)
      return uu;
  }
  return NULL;
}

static
VarSymbol *update_add_variable(ParserState *ps, Ident *id, TypeDesc *desc)
{
  ParserUnit *u = ps->u;
  VarSymbol *varSym;
  switch (u->scope) {
  case SCOPE_MODULE:
  case SCOPE_CLASS:
    /*
     * variables in module or class canbe accessed by other packages,
     * during building AST, it is already saved in symbol table.
     */
    varSym = (VarSymbol *)STable_Get(u->stbl, id->name);
    assert(varSym != NULL);
    assert(varSym->kind == SYM_CONST || varSym->kind == SYM_VAR);
    if (varSym->desc == NULL) {
      char buf[64];
      TypeDesc_ToString(desc, buf);
      Log_Debug("update symbol '%s' type as '%s'", id->name, buf);
      varSym->desc = desc;
      TYPE_INCREF(varSym->desc);
    }
    break;
  case SCOPE_FUNCTION:
  case SCOPE_CLOSURE: {
    /* function scope has independent space for save variables. */
    Log_Debug("var '%s' declaration in function", id->name);
    varSym = STable_Add_Var(u->stbl, id->name, desc);
    if (varSym == NULL) {
      Syntax_Error(ps, &id->pos, "var '%s' is duplicated", id->name);
      return NULL;
    }
    FuncSymbol *funcSym = (FuncSymbol *)u->sym;
    Vector_Append(&funcSym->locvec, varSym);
    varSym->refcnt++;
    break;
  }
  case SCOPE_BLOCK: {
    /*
     * variables in these socpes must not be duplicated
     * within function or method scopes
     */
    Log_Debug("var '%s' declaration in block", id->name);

    /* check there is no the same symbol in up unit until function scope. */
    ParserUnit *uu = parse_block_variable(ps, id);
    if (uu == NULL)
      return NULL;
    varSym = STable_Add_Var(u->stbl, id->name, desc);
    if (varSym == NULL) {
      Syntax_Error(ps, &id->pos, "var '%s' is duplicated", id->name);
      return NULL;
    }

    /* local variables' index is up function(closure)'s index */
    varSym->index = uu->stbl->varindex++;
    Vector *locvec;
    if (uu->sym->kind == SYM_FUNC) {
      locvec = &((FuncSymbol *)uu->sym)->locvec;
    } else {
      locvec = &((AFuncSymbol *)uu->sym)->locvec;
    }
    Vector_Append(locvec, varSym);
    varSym->refcnt++;
    break;
  }
  default:
    assert(0);
    break;
  }

  return varSym;
}

static void __save_const_to_symbol(VarSymbol *varSym, Expr *exp)
{
  if (exp->kind == CONST_KIND) {
    varSym->value = ((ConstExpr *)exp)->value;
  } else if (exp->kind == ID_KIND) {
    VarSymbol *sym = (VarSymbol *)exp->sym;
    assert(sym != NULL && sym->kind == SYM_CONST);
    varSym->value = sym->value;
  } else {
    assert(0);
  }
}

/*
 * parse single variable declaration for:
 * 1. module's variable & constant declaration
 * 2. class or trait's field declaration
 * 3. function argument & return & local variable declaration
 */
static void parse_variable(ParserState *ps, Ident *id, TypeWrapper *type,
                           Expr *rexp, int konst)
{
  Log_Debug("parse variable '%s' declaration", id->name);

  /* check expression */
  if (rexp != NULL) {
    /* parse right expression of variable declaration */
    rexp->ctx = EXPR_LOAD;
    parser_visit_expr(ps, rexp);
    if (rexp->desc == NULL) {
      Syntax_Error(ps, &rexp->pos, "cannot resolve right expression's type");
      return;
    }

    /* constant */
    if (konst && !Expr_Is_Const(rexp)) {
      Syntax_Error(ps, &rexp->pos, "not a valid constant expression");
      return;
    }

    TypeDesc *rdesc = rexp->desc;

    /*
     * right expr's type is func proto, there are two exprs:
     * 1. function call
     * 2. anonymous function declaration
     * check function call expr only, anonymous needs not check
     */
    if (rexp->kind == CALL_KIND) {
      assert(rdesc->kind == TYPE_PROTO);
      ProtoDesc *proto = (ProtoDesc *)rdesc;
      if (Vector_Size(proto->ret) != 1) {
        Syntax_Error(ps, &rexp->pos, "multiple-value in single-value context");
        return;
      }

      rdesc = Vector_Get(proto->ret, 0);
      if (type->desc == NULL) {
        char buf[64];
        TypeDesc_ToString(rdesc, buf);
        Log_Debug("var '%s' type is none, set it as '%s'", id->name, buf);
        TYPE_INCREF(rdesc);
        type->desc = rdesc;
      }
    }

    if (type->desc != NULL) {
      /* check type equals or not */
      if (!__type_is_compatible(type->desc, rdesc)) {
        Syntax_Error(ps, &rexp->pos,
                     "right expression's type is not compatible");
        return;
      }
    } else {
      char buf[64];
      TypeDesc_ToString(rdesc, buf);
      Log_Debug("var '%s' type is none, set it as '%s'", id->name, buf);
      TYPE_INCREF(rdesc);
      type->desc = rdesc;
    }
  }

  /* update or add symbol */
  VarSymbol *varSym = update_add_variable(ps, id, type->desc);
  if (varSym == NULL)
    return;

  /* if it is constant, save the value to its symbol */
  if (konst)
    __save_const_to_symbol(varSym, rexp);

  /* generate code */
  if (rexp == NULL)
    return;

  ParserUnit *u = ps->u;
  if (u->scope == SCOPE_MODULE || u->scope == SCOPE_CLASS) {
    /* module or class variable, global variable */
    Inst_Append_NoArg(u->block, OP_LOAD0);
    ConstValue val = {.kind = BASE_STRING, .str = varSym->name};
    Inst_Append(u->block, OP_SETFIELD, &val);
  } else {
    /* others are local variables */
    ConstValue val = {.kind = BASE_INT, .ival = varSym->index};
    Inst_Append(u->block, OP_STORE, &val);
  }
}

static void parse_vardecl_stmt(ParserState *ps, Stmt *stmt)
{
  VarDeclStmt *varStmt = (VarDeclStmt *)stmt;
  parse_variable(ps, &varStmt->id, &varStmt->type,
                 varStmt->exp, varStmt->konst);
}

static void parse_varlistdecl_stmt(ParserState *ps, Stmt *stmt)
{
  VarListDeclStmt *varListStmt = (VarListDeclStmt *)stmt;
  Expr *rexp = varListStmt->exp;
  assert(rexp != NULL);
  assert(!varListStmt->konst);

  if (rexp->kind != CALL_KIND) {
    Syntax_Error(ps, &rexp->pos, "right expression is not a multi-value");
    return;
  }

  rexp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, rexp);
  if (rexp->desc == NULL)
    return;

  assert(rexp->desc->kind == TYPE_PROTO);
  ProtoDesc *proto = (ProtoDesc *)rexp->desc;

  /* check count */
  int nret = Vector_Size(proto->ret);
  int nvar = Vector_Size(varListStmt->ids);
  if (nret != nvar) {
    Syntax_Error(ps, &rexp->pos,
                 "assignment mismatch: %d variables but %d values",
                 nvar, nret);
    return;
  }

  /* check type */
  if (varListStmt->type.desc != NULL) {
    TypeDesc *vdesc = varListStmt->type.desc;
    TypeDesc *desc;
    Vector_ForEach(desc, proto->ret) {
      if (!__type_is_compatible(vdesc, desc)) {
        Syntax_Error(ps, &rexp->pos,
                     "right expression's type is not compatible");
        return;
      }
    }
  }

  /* update or add symbol */
  assert(ps->u->scope != SCOPE_CLASS);
  VarSymbol *varSym;
  TypeDesc *desc;
  Ident *ident;
  Vector_ForEach(ident, varListStmt->ids) {
    desc = Vector_Get(proto->ret, i);
    varSym = update_add_variable(ps, ident, desc);
    if (varSym == NULL)
      return;
  }

  /* FIXME: generator code */
  ParserUnit *u = ps->u;
  if (u->scope == SCOPE_MODULE) {

  } else {
  }
}

/*
  a = 100
  a.b = 200
  a.b.c = 300
  a.b[1].c = 400
  a.b(1,2).c = 500
  leftmost identifier is variable or imported external package name
 */
static void parse_assign_stmt(ParserState *ps, Stmt *stmt)
{
  AssignStmt *assignStmt = (AssignStmt *)stmt;
  Expr *lexp = assignStmt->left;
  Expr *rexp = assignStmt->right;
  if (assignStmt->op == OP_ASSIGN) {
    rexp->ctx = EXPR_LOAD;
    parser_visit_expr(ps, rexp);
    lexp->ctx = EXPR_STORE;
    parser_visit_expr(ps, lexp);
  } else {
    /* FIXME: */
    assert(0);
    //do_binary(BinaryOpKind kind, Expr *left, Expr *right)
    lexp->ctx = EXPR_STORE;
    parser_visit_expr(ps, lexp);
  }

  TypeDesc *rdesc = rexp->desc;
  if (rdesc == NULL) {
    Syntax_Error(ps, &rexp->pos, "cannot resolve right expression's type");
    return;
  }
  if (rdesc->kind == TYPE_PROTO) {
    ProtoDesc *proto = (ProtoDesc *)rdesc;
    int n = Vector_Size(proto->ret);
    if (n != 1) {
      Syntax_Error(ps, &rexp->pos, "multiple-value in single-value context");
      return;
    }
    rdesc = Vector_Get(proto->ret, 0);
  }

  TypeDesc *ldesc = lexp->desc;
  if (!__type_is_compatible(ldesc, rdesc)) {
    Syntax_Error(ps, &rexp->pos, "right expression's type is not compatible");
    return;
  }
}

static void parse_assignlist_stmt(ParserState *ps, Stmt *stmt)
{
  AssignListStmt *assignListStmt = (AssignListStmt *)stmt;
  Expr *rexp = assignListStmt->right;

  if (rexp->kind != CALL_KIND) {
    Syntax_Error(ps, &rexp->pos, "right expression is not a multi-value");
    return;
  }

  rexp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, rexp);

  assert(rexp->desc != NULL && rexp->desc->kind == TYPE_PROTO);
  ProtoDesc *proto = (ProtoDesc *)rexp->desc;

  /* check count */
  int nret = Vector_Size(proto->ret);
  int nleft = Vector_Size(assignListStmt->left);
  if (nret != nleft) {
    Syntax_Error(ps, &rexp->pos,
                 "assignment mismatch: %d expressions but %d values",
                 nleft, nret);
    return;
  }

  /* type check */
  TypeDesc *desc;
  Expr *e;
  Vector_ForEach(e, assignListStmt->left) {
    e->ctx = EXPR_STORE;
    parser_visit_expr(ps, e);
    desc = Vector_Get(proto->ret, i);
    if (!__type_is_compatible(e->desc, desc)) {
      Syntax_Error(ps, &rexp->pos, "right expression's type is not compatible");
      return;
    }
  }

  /* FIXME: generate code */
}

static void parse_statements(ParserState *ps, Vector *stmts);

static void parse_funcdecl_stmt(ParserState *ps, Stmt *stmt)
{
  FuncDeclStmt *funStmt = (FuncDeclStmt *)stmt;

  Parser_Enter_Scope(ps, SCOPE_FUNCTION);
  ps->u->stbl = STable_New();

  ParserUnit *uu = up_parser_unit(ps);
  assert(uu != NULL);
  assert(uu->scope == SCOPE_MODULE || uu->scope == SCOPE_CLASS);
  Symbol *sym = STable_Get(uu->stbl, funStmt->id.name);
  assert(sym != NULL);
  ps->u->sym = sym;

  Log_Debug("----parse function '%s'----", funStmt->id.name);

  /*
   * return-list has variable declarations
   * FIXME: how to order argument and return
   */
  IdType *idtype;
  Vector_ForEach(idtype, funStmt->rets) {
    if (idtype->id.name != NULL) {
      /* return-list is variable declaration */
      parse_variable(ps, &idtype->id, &idtype->type, NULL, 0);
    }
  }

  /* parameter-list */
  Vector_ForEach(idtype, funStmt->args) {
    parse_variable(ps, &idtype->id, &idtype->type, NULL, 0);
  }

  parse_statements(ps, funStmt->body);

  Log_Debug("----end of function '%s'----", funStmt->id.name);

  Parser_Exit_Scope(ps);
}

static void parse_protodecl_stmt(ParserState *ps, Stmt *stmt)
{

}

static void parse_expr_stmt(ParserState *ps, Stmt *stmt)
{
  ExprStmt *expStmt = (ExprStmt *)stmt;
  Expr *exp = expStmt->exp;
  exp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, exp);
}

static int check_returns(TypeDesc *desc, Vector *exps)
{
  assert(desc->kind == TYPE_PROTO);
  ProtoDesc *proto = (ProtoDesc *)desc;
  return __check_expr_types(proto->ret, exps);
}

static void parse_return_stmt(ParserState *ps, Stmt *stmt)
{
  ParserUnit *u = ps->u;
  FuncSymbol *funcSym;

  if (u->scope == SCOPE_FUNCTION) {
    funcSym = (FuncSymbol *)u->sym;
  } else {
    ParserUnit *uu;
    list_for_each_entry(uu, &ps->ustack, link) {
      if (uu->scope == SCOPE_FUNCTION) {
        funcSym = (FuncSymbol *)uu->sym;
        break;
      }
    }
  }

  assert(funcSym != NULL && funcSym->kind == SYM_FUNC);

  ReturnStmt *retStmt = (ReturnStmt *)stmt;

  Expr *e;
  Vector_ForEach(e, retStmt->exps) {
    e->ctx = EXPR_LOAD;
    parser_visit_expr(ps, e);
  }

  if (!check_returns(funcSym->desc, retStmt->exps)) {
    Syntax_Error(ps, &retStmt->pos, "func %s: returns are not matched.",
                 funcSym->name);
    return;
  }

  /* FIXME: all control flow branches to check
     need include returns' count in OP_RET n?
   */
  Inst_Append_NoArg(u->block, OP_RET);
  u->block->ret = 1;
}

static void parse_list_stmt(ParserState *ps, Stmt *stmt)
{
  ListStmt *listStmt = (ListStmt *)stmt;
  if (listStmt->block) {
    Parser_Enter_Scope(ps, SCOPE_BLOCK);
    ps->u->stbl = STable_New();
    parse_statements(ps, listStmt->vec);
    Parser_Exit_Scope(ps);
  } else {
    parse_statements(ps, listStmt->vec);
  }
}

static void parse_typealias_stmt(ParserState *ps, Stmt *stmt)
{
}

static void parse_class_supers(ParserState *ps, Vector *supers)
{

}

static void parse_class_stmt(ParserState *ps, Stmt *stmt)
{
  KlassStmt *clsStmt = (KlassStmt *)stmt;
  Log_Debug("----parse class '%s'----", clsStmt->id.name);

  Symbol *sym = STable_Get(ps->u->stbl, clsStmt->id.name);
  assert(sym);

  parse_class_supers(ps, clsStmt->super);

  Parser_Enter_Scope(ps, SCOPE_CLASS);
  ps->u->sym = sym;
  ps->u->stbl = ((ClassSymbol *)sym)->stbl;

  /* parse class's body statements */
  parse_statements(ps, clsStmt->body);

  Parser_Exit_Scope(ps);

  Log_Debug("----end of class '%s'----", clsStmt->id.name);
}

typedef void (*parse_stmt_func)(ParserState *, Stmt *);

static parse_stmt_func parse_stmt_funcs[] = {
  NULL,                     /* INVALID         */
  parse_vardecl_stmt,       /* VAR_KIND        */
  parse_varlistdecl_stmt,   /* VARLIST_KIND    */
  parse_assign_stmt,        /* ASSIGN_KIND     */
  parse_assignlist_stmt,    /* ASSIGNLIST_KIND */
  parse_funcdecl_stmt,      /* FUNC_KIND       */
  parse_protodecl_stmt,     /* PROTO_KIND      */
  parse_expr_stmt,          /* EXPR_KIND       */
  parse_return_stmt,        /* RETURN_KIND     */
  parse_list_stmt,          /* LIST_KIND       */
  parse_typealias_stmt,     /* TYPEALIAS_KIND  */
  parse_class_stmt,         /* CLASS_KIND      */
};

static void parse_statement(ParserState *ps, Stmt *stmt)
{
  if (ps->errnum > MAX_ERRORS)
    return;
  assert(stmt->kind > 0 && stmt->kind < nr_elts(parse_stmt_funcs));
  parse_stmt_func parse_func = parse_stmt_funcs[stmt->kind];
  assert(parse_func != NULL);
  parse_func(ps, stmt);
}

static void parse_statements(ParserState *ps, Vector *stmts)
{
  Stmt *stmt;
  Vector_ForEach(stmt, stmts) {
    parse_statement(ps, stmt);
    Free_Stmt_Func(stmt, NULL);
    Show_MemStat();
  }
  Vector_Fini_Self(stmts);
}

void Build_AST(ParserState *ps, FILE *in)
{
  Log_Debug("\x1b[34m----STARTING BUILDING AST------\x1b[0m");
  yyscan_t scanner;
  yylex_init_extra(ps, &scanner);
  yyset_in(in, scanner);
  Parser_Enter_Scope(ps, SCOPE_MODULE);
  ps->u->stbl = ps->grp->pkg->stbl;
  yyparse(ps, scanner);
  Parser_Exit_Scope(ps);
  yylex_destroy(scanner);
  Log_Debug("\x1b[34m----END OF BUILDING AST--------\x1b[0m");
}

void Parse_AST(ParserState *ps)
{
  Log_Debug("\x1b[32m----STARTING SEMANTIC ANALYSIS & CODE GEN----\x1b[0m");
  Parse_Imports(ps);
  CheckConflictWithExternal(ps);
  Parser_Enter_Scope(ps, SCOPE_MODULE);
  ps->u->stbl = ps->grp->pkg->stbl;
  parse_statements(ps, &ps->stmts);
  Parser_Exit_Scope(ps);
  Log_Debug("\x1b[32m----END OF SEMANTIC ANALYSIS & CODE GEN------\x1b[0m");
}
