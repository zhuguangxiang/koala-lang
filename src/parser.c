
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
#include "mem.h"
#include "log.h"

static const char *scope_strings[] = {
  "PACKAGE", "MODULE", "CLASS", "FUNCTION", "BLOCK", "CLOSURE"
};

int Init_PkgInfo(PkgInfo *pkg, char *pkgname, char *pkgfile, Options *opts)
{
  memset(pkg, 0, sizeof(PkgInfo));
  pkg->pkgfile = AtomString_New(pkgfile);
  pkg->pkgname = AtomString_New(pkgname);
  pkg->stbl = STable_New();
  pkg->opts = opts;
  return 0;
}

void Fini_PkgInfo(PkgInfo *pkg)
{
  STable_Free_Self(pkg->stbl);
}

void Show_PkgInfo(PkgInfo *pkg)
{
  puts("\n----------------------------------------");
  printf("scope-0(%s, %s) symbols:\n", scope_strings[0], pkg->pkgname.str);
  STable_Show(pkg->stbl);
  puts("----------------------------------------\n");
}

void load_extpkg()
{

}

static ParserUnit *new_parser_unit(ScopeKind scope)
{
  ParserUnit *u = mm_alloc(sizeof(ParserUnit));
	init_list_head(&u->link);
	u->stbl = NULL;
	u->sym = NULL;
	u->block = NULL;
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
  mm_free(u);
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
  FuncSymbol *funSym = arg;
  Vector_Append(&funSym->locvec, sym);
}

static void merge_parser_unit(ParserState *ps)
{
  ParserUnit *u = ps->u;

  switch (u->scope) {
  case SCOPE_MODULE: {
    if (u->block != NULL) {
      /* module has codes for __init__ */
      FuncSymbol *funSym = (FuncSymbol *)STable_Get(u->stbl, "__init__");
      if (funSym == NULL) {
        Log_Debug("create __init__ function");
        TypeDesc *proto = TypeDesc_Get_Proto(NULL, NULL);
        funSym = STable_Add_Func(u->stbl, "__init__", proto);
        assert(funSym != NULL);
        funSym->code = u->block;
      } else {
        //FIXME: merge codeblock
      }
      u->block = NULL;
    }
    u->stbl = NULL;
    break;
  }
  case SCOPE_FUNCTION: {
    FuncSymbol *funSym = (FuncSymbol *)u->sym;
    assert(funSym && funSym->kind == SYM_FUNC);
    CodeBlock *b = u->block;
    if (b != NULL) {
      /* if no return opcode, then add one */
      if (!b->ret) {
        Log_Debug("add 'return' to function '%s'", funSym->name);
        Inst_Append_NoArg(b, OP_RET);
      }
    }
    /* save codeblock to function symbol */
    funSym->code = u->block;
    u->block = NULL;
    /* free local symbol table */
    STable_Free_Self(u->stbl);
    u->stbl = NULL;
    /* simple clear symbol, it's stored in module's symbol table */
    u->sym = NULL;
    break;
  }
  case SCOPE_BLOCK: {
    /* FIXME: if-else, while and switch-case */
    ParserUnit *uu = up_parser_unit(ps);
    Log_Debug("merge code into up scope-%d(%s)",
              ps->depth - 1, scope_strings[uu->scope]);
    if (uu->block == NULL) {
      Log_Debug("up scope's codeblock is null");
      uu->block = u->block;
    } else {
      CodeBlock_Merge(u->block, uu->block);
      CodeBlock_Free(u->block);
    }
    u->block = NULL;
    STable_Free_Self(u->stbl);
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
  default:{
    assert(0);
    break;
  }
  }
}

static void show_parser_unit(ParserState *ps)
{
  ParserUnit *u = ps->u;
  const char *scope = scope_strings[u->scope];
  char *name = u->sym != NULL ? u->sym->name : NULL;
  name = (name == NULL) ? ps->filename : name;
  puts("\n----------------------------------------");
  printf("scope-%d(%s, %s) symbols:\n", ps->depth, scope, name);
  STable_Show(u->stbl);
  CodeBlock_Show(u->block);
  puts("----------------------------------------\n");
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
  Log_Debug("\x1b[35mExit scope-%d(%s)\x1b[0m",
            ps->depth, scope_strings[ps->u->scope]);

  show_parser_unit(ps);

  check_unused_symbols(ps->u);

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

ParserState *New_Parser(PkgInfo *pkg, char *filename)
{
  ParserState *ps = mm_alloc(sizeof(ParserState));
  ps->filename = strdup(filename);
  ps->pkg = pkg;

  Vector_Init(&ps->stmts);

  Init_Imports(ps);

  init_list_head(&ps->ustack);

  Vector_Init(&ps->errors);

  return ps;
}

void Destroy_Parser(ParserState *ps)
{
  assert(ps->u == NULL);
  assert(list_empty(&ps->ustack));

  Vector_Fini(&ps->stmts, Free_Stmt_Func, NULL);
  Fini_Imports(ps);

  mm_free(ps->filename);
  mm_free(ps);
}

static inline CodeBlock *__get_codeblock(ParserUnit *u)
{
  if (u->block == NULL)
    u->block = CodeBlock_New();
  return u->block;
}

static int expr_is_const(Expr *exp)
{
  ExprKind kind = exp->kind;
  if (kind != INT_KIND || kind != FLOAT_KIND ||
      kind != STRING_KIND || kind != CHAR_KIND)
    return 0;
  else
    return 1;
}

static char *expr_get_funcname(Expr *exp)
{
  CallExpr *callExp = (CallExpr *)exp;
  Expr *left = callExp->left;
  if (left->kind == ID_KIND) {
    return ((BaseExpr *)left)->id;
  } else if (left->kind == ATTRIBUTE_KIND) {
    return ((AttributeExpr *)left)->id.name;
  } else {
    return "<unknown>";
  }
}

static int type_is_compatible(TypeDesc *t1, TypeDesc *t2)
{
  /* FIXME: class inheritance */
  return 1;
}

static void parse_nil_expr(ParserState *ps, Expr *exp)
{
  UNUSED_PARAMETER(ps);
  UNUSED_PARAMETER(exp);
  assert(exp->ctx == EXPR_LOAD);
}

static void parse_int_expr(ParserState *ps, Expr *exp)
{
  ParserUnit *u = ps->u;
  BaseExpr *baseExp = (BaseExpr *)exp;
  assert(exp->ctx == EXPR_LOAD);
  exp->desc = TypeDesc_Get_Basic(BASIC_INT);
  TYPE_INCREF(exp->desc);

  Argument val = {.kind = ARG_INT, .ival = baseExp->ival};
  Inst_Append(__get_codeblock(u), OP_LOADK, &val);
}

static void parse_flt_expr(ParserState *ps, Expr *exp)
{
  ParserUnit *u = ps->u;
  BaseExpr *baseExp = (BaseExpr *)exp;
  assert(exp->ctx == EXPR_LOAD);
  exp->desc = TypeDesc_Get_Basic(BASIC_FLOAT);
  TYPE_INCREF(exp->desc);

  Argument val = {.kind = ARG_FLOAT, .fval = baseExp->fval};
  Inst_Append(__get_codeblock(u), OP_LOADK, &val);
}

static void parse_bool_expr(ParserState *ps, Expr *exp)
{
  ParserUnit *u = ps->u;
  BaseExpr *baseExp = (BaseExpr *)exp;
  assert(exp->ctx == EXPR_LOAD);
  exp->desc = TypeDesc_Get_Basic(BASIC_BOOL);
  TYPE_INCREF(exp->desc);

  Argument val = {.kind = ARG_BOOL, .fval = baseExp->bval};
  Inst_Append(__get_codeblock(u), OP_LOADK, &val);
}

static void parse_string_expr(ParserState *ps, Expr *exp)
{
  ParserUnit *u = ps->u;
  BaseExpr *baseExp = (BaseExpr *)exp;
  assert(exp->ctx == EXPR_LOAD);
  exp->desc = TypeDesc_Get_Basic(BASIC_STRING);
  TYPE_INCREF(exp->desc);

  Argument val = {.kind = ARG_STR, .str = baseExp->str};
  Inst_Append(__get_codeblock(u), OP_LOADK, &val);
}

static void parse_char_expr(ParserState *ps, Expr *exp)
{
  ParserUnit *u = ps->u;
  BaseExpr *baseExp = (BaseExpr *)exp;
  assert(exp->ctx == EXPR_LOAD);
  exp->desc = TypeDesc_Get_Basic(BASIC_CHAR);
  TYPE_INCREF(exp->desc);

  Argument val = {.kind = ARG_UCHAR, .uch = baseExp->ch};
  Inst_Append(__get_codeblock(u), OP_LOADK, &val);
}

static void parse_ident_expr(ParserState *ps, Expr *exp)
{
}

static void parse_self_expr(ParserState *ps, Expr *exp)
{
  assert(0);
}

static void parse_super_expr(ParserState *ps, Expr *exp)
{
  assert(0);
}

static void parse_unary_expr(ParserState *ps, Expr *exp)
{
  assert(0);
}

static void parse_binary_expr(ParserState *ps, Expr *exp)
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

static void parse_call_expr(ParserState *ps, Expr *exp)
{
  assert(0);
}

static void parse_slice_expr(ParserState *ps, Expr *exp)
{
  assert(0);
}

static void parse_list_expr(ParserState *ps, Expr *exp)
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

typedef void (*parse_expr_func)(ParserState *, Expr *);

static parse_expr_func parse_expr_funcs[] = {
  NULL,                 /* INVALID          */
  parse_nil_expr,       /* NIL_KIND         */
  parse_self_expr,      /* SELF_KIND        */
  parse_super_expr,     /* SUPER_KIND       */
  parse_int_expr,       /* INT_KIND         */
  parse_flt_expr,       /* FLOAT_KIND       */
  parse_bool_expr,      /* BOOL_KIND        */
  parse_string_expr,    /* STRING_KIND      */
  parse_char_expr,      /* CHAR_KIND        */
  parse_ident_expr,     /* ID_KIND          */
  parse_unary_expr,     /* UNARY_KIND       */
  parse_binary_expr,    /* BINARY_KIND      */
  parse_attribute_expr, /* ATTRIBUTE_KIND   */
  parse_subscript_expr, /* SUBSCRIPT_KIND   */
  parse_call_expr,      /* CALL_KIND        */
  parse_slice_expr,     /* SLICE_KIND       */
  parse_list_expr,      /* LIST_EXPR_KIND   */
  NULL,                 /* MAP_LIST_KIND    */
  NULL,                 /* MAP_ENTRY_KIND   */
  parse_array_expr,     /* ARRAY_KIND       */
  parse_map_expr,       /* MAP_KIND         */
  parse_set_expr,       /* SET_KIND         */
  parse_anonymous_expr, /* ANONY_FUNC_KIND  */
};

static void parser_visit_expr(ParserState *ps, Expr *exp)
{
  if (ps->errnum >= MAX_ERRORS)
    return;
  assert(exp->kind > 0 && exp->kind < nr_elts(parse_expr_funcs));
  parse_expr_func parse_func = parse_expr_funcs[exp->kind];
  assert(parse_func != NULL);
  parse_func(ps, exp);
}

static void parse_stmts(ParserState *ps, Vector *stmts);

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
    /* constant */
    if (konst == 1 && expr_is_const(rexp)) {
      Syntax_Error(ps, &rexp->pos, "not a valid const expression");
      return;
    }

    /* parse right expression of variable declaration */
    rexp->ctx = EXPR_LOAD;
    parser_visit_expr(ps, rexp);
    if (rexp->desc == NULL) {
      Syntax_Error(ps, &rexp->pos, "cannot resolve right expression's type");
      return;
    }

    /* multi-value's expression */
    if (rexp->desc->kind == TYPE_PROTO) {
      /*
         right expr's type is func proto, there are two exprs:
         1. function call
         2. anonymous function declaration
         check function call expr only
       */
      if (rexp->kind == CALL_KIND) {
        ProtoDesc *proto = (ProtoDesc *)rexp->desc;
        if (Vector_Size(proto->ret) != 1) {
          Syntax_Error(ps, &rexp->pos,
                       "multiple-value %s() in single-value context",
                       expr_get_funcname(rexp));
          return;
        }
      }
    }

    if (type->desc != NULL) {
      /* check type equals or not */
      if (!type_is_compatible(type->desc, rexp->desc)) {
        Syntax_Error(ps, &rexp->pos,
                     "right expression's type is not compatible");
        return;
      }
    } else {
      char buf[64];
      TypeDesc_ToString(rexp->desc, buf);
      Log_Debug("var '%s' type is none, set it as '%s'", id->name, buf);
      TYPE_INCREF(rexp->desc);
      type->desc = rexp->desc;
    }
  }

  /* update or add symbol */
  VarSymbol *varSym;
  ParserUnit *u = ps->u;
  if (u->scope == SCOPE_MODULE || u->scope == SCOPE_CLASS) {
    /*
     * variables in module or class canbe accessed by other packages,
     * during building AST, it is already saved in symbol table.
     */
    varSym = (VarSymbol *)STable_Get(u->stbl, id->name);
    assert(varSym != NULL);
    assert(varSym->kind == SYM_CONST || varSym->kind == SYM_VAR);
    if (varSym->desc == NULL) {
      char buf[64];
      TypeDesc_ToString(type->desc, buf);
      Log_Debug("update symbol '%s' type as '%s'", id->name, buf);
      varSym->desc = type->desc;
      TYPE_INCREF(varSym->desc);
    }
  } else if (u->scope == SCOPE_FUNCTION) {
    /* function scope has independent space for save variables. */
    Log_Debug("var '%s' declaration in function", id->name);
    assert(konst != 1);
    varSym = STable_Add_Var(u->stbl, id->name, type->desc);
    if (varSym == NULL) {
      Syntax_Error(ps, &id->pos, "var '%s' is duplicated", id->name);
      return;
    }
    FuncSymbol *funSym = (FuncSymbol *)u->sym;
    Vector_Append(&funSym->locvec, varSym);
    varSym->refcnt++;
  } else {
    assert(u->scope == SCOPE_BLOCK || u->scope == SCOPE_CLOSURE);
    /*
     * variables in these socpes must not be duplicated
     * within function or method scopes
     */
    Log_Debug("var '%s' declaration in block/closure", id->name);

    /* check there is no the same symbol in up unit until function scope. */
    int infunc = 0;
    Symbol *sym;
    ParserUnit *uu;
    int depth = ps->depth;
    struct list_head *pos;
    list_for_each(pos, &ps->ustack) {
      depth -= 1;
      uu = container_of(pos, ParserUnit, link);
      sym = STable_Get(uu->stbl, id->name);
      if (sym != NULL) {
        Syntax_Error(ps, &id->pos,
                     "var '%s' is already delcared in scope-%d(%s)",
                     id->name, depth, scope_strings[uu->scope]);
        return;
      }
      if (uu->scope == SCOPE_FUNCTION) {
        infunc = 1;
        break;
      }
    }

    /* BUG: block is not in function ? */
    assert(infunc);

    varSym = STable_Add_Var(u->stbl, id->name, type->desc);
    if (varSym == NULL) {
      Syntax_Error(ps, &id->pos, "var '%s' is duplicated", id->name);
      return;
    }

    /* local variables' index is function's index */
    varSym->index = uu->stbl->varindex++;
    Vector_Append(&((FuncSymbol *)uu->sym)->locvec, varSym);
    varSym->refcnt++;
  }

  /* generate code */
  if (rexp == NULL)
    return;

  if (u->scope == SCOPE_MODULE || u->scope == SCOPE_CLASS) {
    /* module or class variable, global variable */
    Inst_Append_NoArg(u->block, OP_LOAD0);
    Argument val = {.kind = ARG_STR, .str = varSym->name};
    Inst_Append(u->block, OP_SETFIELD, &val);
  } else {
    /* others are local variables */
    Argument val = {.kind = ARG_INT, .ival = varSym->index};
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
  assert(0);
}

static void parse_assign_stmt(ParserState *ps, Stmt *stmt)
{

}

static void parse_assignlist_stmt(ParserState *ps, Stmt *stmt)
{
  assert(0);
}

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
   * FIXME: hwo to order argument and return
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

  parse_stmts(ps, funStmt->body);

  Log_Debug("----end of function '%s'----", funStmt->id.name);

  Parser_Exit_Scope(ps);
}

static void parse_protodecl_stmt(ParserState *ps, Stmt *stmt)
{

}

static void parse_expr_stmt(ParserState *ps, Stmt *stmt)
{
  ExprStmt *expStmt = (ExprStmt *)stmt;
  parser_visit_expr(ps, expStmt->exp);
}

static void parse_return_stmt(ParserState *ps, Stmt *stmt)
{

}

static void parse_list_stmt(ParserState *ps, Stmt *stmt)
{
  ListStmt *listStmt = (ListStmt *)stmt;
  if (listStmt->block) {
    Parser_Enter_Scope(ps, SCOPE_BLOCK);
    ps->u->stbl = STable_New();
    parse_stmts(ps, listStmt->vec);
    Parser_Exit_Scope(ps);
  } else {
    parse_stmts(ps, listStmt->vec);
  }
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

  /* parse class' body */
  parse_stmts(ps, clsStmt->body);

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
  NULL,                     /* TYPEALIAS_KIND  */
  parse_class_stmt,         /* CLASS_KIND      */
};

static void parser_vist_stmt(ParserState *ps, Stmt *stmt)
{
  if (ps->errnum >= MAX_ERRORS)
    return;
  assert(stmt->kind > 0 && stmt->kind < nr_elts(parse_stmt_funcs));
  parse_stmt_func parse_func = parse_stmt_funcs[stmt->kind];
  assert(parse_func != NULL);
  parse_func(ps, stmt);
}

static void parse_stmts(ParserState *ps, Vector *stmts)
{
  if (stmts == NULL)
    return;

  Stmt *stmt;
  Vector_ForEach(stmt, stmts) {
    parser_vist_stmt(ps, stmt);
  }
}

int Build_AST(ParserState *ps, FILE *in)
{
  ps->state = STATE_BUILDING_AST;
  Log_Debug("\x1b[34m----STARTING BUILDING AST------\x1b[0m");
  yyscan_t scanner;
  yylex_init_extra(ps, &scanner);
  yyset_in(in, scanner);
  Parser_Enter_Scope(ps, SCOPE_MODULE);
  ps->u->stbl = ps->pkg->stbl;
  yyparse(ps, scanner);
  Parser_Exit_Scope(ps);
  yylex_destroy(scanner);
  Log_Debug("\x1b[34m----END OF BUILDING AST--------\x1b[0m");
  ps->state = STATE_NONE;
  return ps->errnum;
}

int Parse_AST(ParserState *ps)
{
  ps->state = STATE_PARSING_AST;
  Log_Debug("\x1b[32m----STARTING SEMANTIC ANALYSIS & CODE GEN----\x1b[0m");
  Parser_Enter_Scope(ps, SCOPE_MODULE);
  ps->u->stbl = ps->pkg->stbl;
  parse_stmts(ps, &ps->stmts);
  Parser_Exit_Scope(ps);
  Log_Debug("\x1b[32m----END OF SEMANTIC ANALYSIS & CODE GEN------\x1b[0m");
  ps->state = STATE_NONE;
  return ps->errnum;
}
