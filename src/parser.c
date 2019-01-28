
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

void load_extpkg()
{

}

static CodeBlock *codeblock(ParserUnit *u)
{
  if (u->block == NULL)
    u->block = CodeBlock_New();
  return u->block;
}

static void init_parser_unit(ParserUnit *u, ScopeKind scope)
{
	init_list_head(&u->link);
	u->stbl = NULL;
	u->sym = NULL;
	u->block = NULL;
	u->scope = scope;
	Vector_Init(&u->jmps);
}

static void fini_parser_unit(ParserUnit *u)
{
  assert(list_unlinked(&u->link));

  //FIXME
  assert(u->sym == NULL);
  assert(u->stbl == NULL);

  CodeBlock *block = u->block;
  if (block != NULL) {
    assert(list_empty(&block->insts));
    assert(block->next == NULL);
  }

  //FIXME
  assert(Vector_Size(&u->jmps) == 0);
  Vector_Fini(&u->jmps, NULL, NULL);
}

static ParserUnit *new_parser_unit(ScopeKind scope)
{
  ParserUnit *u = mm_alloc(sizeof(ParserUnit));
  init_parser_unit(u, scope);
  return u;
}

static void free_parser_unit(ParserUnit *u)
{
  fini_parser_unit(u);
  mm_free(u);
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
      /* save codeblock to function symbol */
      funSym->code = u->block;
      u->block = NULL;
      /* save local variables' symbol to FuncSymbol->locvec */
      STable_Free(u->stbl, save_locvar_func, funSym);
      u->stbl = NULL;
    }
    break;
  }
  case SCOPE_BLOCK: {

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

static const char *scope_strings[] = {
  NULL, "PACKAGE", "FUNCTION", "BLOCK", "CLOSURE", "CLASS"
};

#if 1

static void show_parser_unit(ParserState *ps)
{
  ParserUnit *u = ps->u;
  const char *scope = scope_strings[u->scope];
  char *name = u->sym != NULL ? u->sym->name: ps->pkgname;
  puts("\n----------------------------------------");
  printf("scope-%d(%s, %s) symbols:\n", ps->depth, scope, name);
  STable_Show(u->stbl);
  CodeBlock_Show(u->block);
  puts("----------------------------------------\n");
}

void Show_PkgInfo(PkgInfo *pkg)
{
  puts("\n----------------------------------------");
  printf("scope-0(%s, %s) symbols:\n", scope_strings[1], pkg->pkgname.str);
  STable_Show(pkg->stbl);
  puts("----------------------------------------\n");
}

#else
#define show_parser_unit(ps) ((void *)0)
#endif

static void check_unused_symbols(ParserUnit *u)
{

}

void Parser_Enter_Scope(ParserState *ps, ScopeKind scope)
{
  ParserUnit *u = new_parser_unit(scope);
  /* push old unit into stack */
  if (ps->u != NULL)
    list_add(&ps->u->link, &ps->ustack);
  ps->u = u;
  ps->depth++;

  Log_Debug("Enter scope-%d(%s)", ps->depth, scope_strings[scope]);
}

void Parser_Exit_Scope(ParserState *ps)
{
  ParserUnit *u = ps->u;

  if (ps->state == STATE_PARSING_AST)
    show_parser_unit(ps);

  check_unused_symbols(u);

  Log_Debug("Exit scope-%d(%s)", ps->depth, scope_strings[u->scope]);

  merge_parser_unit(ps);

  /* remove current unit */
  assert(u != &ps->top);
  free_parser_unit(u);

  /* restore ps->u to top of ps->ustack */
  struct list_head *head = list_first(&ps->ustack);
  assert(head != NULL);
  list_del(head);
  ps->u = container_of(head, ParserUnit, link);
  ps->depth--;
}

ParserState *New_Parser(PkgInfo *pkg, char *filename)
{
  ParserState *ps = mm_alloc(sizeof(ParserState));
  ps->filename = strdup(filename);
  ps->pkg = pkg;

  Vector_Init(&ps->stmts);

  Init_Imports(ps);

  init_parser_unit(&ps->top, SCOPE_MODULE);
  ps->u = &ps->top;
  ps->u->stbl = pkg->stbl;
  init_list_head(&ps->ustack);

  Vector_Init(&ps->errors);

  return ps;
}

int Build_AST(ParserState *ps, FILE *in)
{
  ps->state = STATE_BUILDING_AST;
  Log_Debug("\x1b[34m----BUILDING AST-----------\x1b[0m");
  yyscan_t scanner;
  yylex_init_extra(ps, &scanner);
  yyset_in(in, scanner);
  yyparse(ps, scanner);
  yylex_destroy(scanner);
  Log_Debug("\x1b[34m----END OF BUILDING AST----\x1b[0m");
  ps->state = STATE_NONE;
  return ps->errnum;
}


void Destroy_Parser(ParserState *ps)
{
  ParserUnit *u = ps->u;

  check_unused_symbols(u);

  assert(u->scope == SCOPE_MODULE);
  assert(u == &ps->top);
  assert(list_empty(&ps->ustack));
  assert(u->stbl == ps->pkg->stbl);
  assert(u->sym == NULL);

  /* FIXME: merge __init__ to pkginfo->stbl */
  /* simple clear */
  assert(u->block == NULL);
  u->stbl = NULL;

  Fini_Imports(ps);
  fini_parser_unit(u);
  Vector_Fini(&ps->stmts, Free_Stmt_Func, NULL);
  mm_free(ps->filename);
  mm_free(ps);
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
  Argument val = {.kind = ARG_INT, .ival = baseExp->ival};
  Inst_Append(codeblock(u), OP_LOADK, &val);
}

static void parse_flt_expr(ParserState *ps, Expr *exp)
{
  ParserUnit *u = ps->u;
  BaseExpr *baseExp = (BaseExpr *)exp;
  assert(exp->ctx == EXPR_LOAD);
  Argument val = {.kind = ARG_FLOAT, .fval = baseExp->fval};
  Inst_Append(codeblock(u), OP_LOADK, &val);
}

static void parse_bool_expr(ParserState *ps, Expr *exp)
{
  ParserUnit *u = ps->u;
  BaseExpr *baseExp = (BaseExpr *)exp;
  assert(exp->ctx == EXPR_LOAD);
  Argument val = {.kind = ARG_BOOL, .fval = baseExp->bval};
  Inst_Append(codeblock(u), OP_LOADK, &val);
}

static void parse_string_expr(ParserState *ps, Expr *exp)
{
  ParserUnit *u = ps->u;
  BaseExpr *baseExp = (BaseExpr *)exp;
  assert(exp->ctx == EXPR_LOAD);
  Argument val = {.kind = ARG_STR, .str = baseExp->str};
  Inst_Append(codeblock(u), OP_LOADK, &val);
}

static void parse_char_expr(ParserState *ps, Expr *exp)
{
  assert(0);
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

static void parse_vardecl_stmt(ParserState *ps, Stmt *stmt)
{
  VarDeclStmt *varStmt = (VarDeclStmt *)stmt;
  Log_Debug("parse variable '%s' declaration", varStmt->id);

  Expr *rexp = varStmt->exp;
  if (rexp != NULL) {
    /* parse right expression of variable declaration */
    parser_visit_expr(ps, rexp);
    if (rexp->desc == NULL) {
      return;
    }
  }
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
  parse_stmts(ps, listStmt->vec);
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
  NULL,                     /* INVALID            */
  parse_vardecl_stmt,       /* VAR_KIND           */
  parse_varlistdecl_stmt,   /* VARLIST_EXPR_KIND  */
  parse_assign_stmt,        /* ASSIGN_KIND        */
  parse_assignlist_stmt,    /* ASSIGNLIST_KIND    */
  parse_funcdecl_stmt,      /* FUNC_KIND          */
  parse_protodecl_stmt,     /* PROTO_KIND         */
  parse_expr_stmt,          /* EXPR_KIND          */
  parse_return_stmt,        /* RETURN_KIND        */
  parse_list_stmt,          /* LIST_KIND          */
  NULL,                     /* TYPEALIAS_KIND     */
  parse_class_stmt,         /* CLASS_KIND         */
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

void Parse(ParserState *ps)
{
  ps->state = STATE_PARSING_AST;
  Log_Debug("\x1b[32m----SEMANTIC ANALYSIS & CODE GEN-----------\x1b[0m");
  parse_stmts(ps, &ps->stmts);
  Log_Debug("\x1b[32m----END OF SEMANTIC ANALYSIS & CODE GEN----\x1b[0m");
  ps->state = STATE_NONE;
}
