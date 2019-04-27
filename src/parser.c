
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
    Syntax_Error(&id->pos,
                 "found different packages %s(%s) and %s(%s)",
                 m->pkgname, m->filename, ps->pkgname, ps->filename);
  }
}

static ParserUnit *new_unit(ScopeKind scope)
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

static void free_unit(ParserUnit *u)
{
  assert(list_unlinked(&u->link));
  assert(u->sym == NULL);
  assert(u->stbl == NULL);
  assert(u->block == NULL);
  assert(Vector_Size(&u->jmps) == 0);
  Vector_Fini_Self(&u->jmps);
  Mfree(u);
}

ParserUnit *Parser_Get_UpScope(ParserState *ps)
{
  struct list_head *pos = list_first(&ps->ustack);
  return (pos != NULL) ? container_of(pos, ParserUnit, link) : NULL;
}

static void merge_unit(ParserState *ps)
{
  ParserUnit *u = ps->u;

  switch (u->scope) {
  case SCOPE_MODULE: {
    /* module has codes for __init__ */
    FuncSymbol *funcSym = (FuncSymbol *)STable_Get(u->stbl, "__init__");
    if (funcSym == NULL) {
      Log_Puts("__init__ no exist");
      if (u->block->bytes > 0) {
        Log_Puts("create __init__ function");
        TypeDesc *proto = TypeDesc_New_Proto(NULL, NULL);
        funcSym = STable_Add_Func(u->stbl, "__init__", proto);
        TYPE_DECREF(proto);
        assert(funcSym != NULL);
        funcSym->code = u->block;
      } else {
        Log_Puts("no codes for __init__");
        /* no codes free codeblock self */
        CodeBlock_Free(u->block);
      }
    } else {
      /* __init__ exist */
      Log_Puts("__init__ exist");
      /* FIXME: merge codeblock */
      CodeBlock_Free(u->block);
    }
    u->block = NULL;
    u->stbl = NULL;
    break;
  }
  case SCOPE_CLASS: {
    SymKind kind = u->sym->kind;
    if (kind == SYM_CLASS || kind == SYM_TRAIT) {
      ClassSymbol *clsSym = (ClassSymbol *)u->sym;
      assert(u->stbl == clsSym->stbl);
    } else {
      assert(kind == SYM_ENUM);
      EnumSymbol *enmSym = (EnumSymbol *)u->sym;
      assert(u->stbl == enmSym->stbl);
    }

    if (u->block->bytes > 0) {
      Log_Debug("merge codes to '%s' __init__ func", u->sym->name);
    } else {
      Log_Debug("'%s' has no init codes", u->sym->name);
    }
    CodeBlock_Free(u->block);
    u->block = NULL;
    u->sym = NULL;
    u->stbl = NULL;
    break;
  }
  case SCOPE_FUNCTION: {
    FuncSymbol *funcSym = (FuncSymbol *)u->sym;
    assert(funcSym && funcSym->kind == SYM_FUNC);
    //CodeBlock *b = u->block;
    /* if no return opcode, then add one */
    // if (!b->ret) {
    //   Log_Debug("add 'return' to function '%s'", funcSym->name);
    //   Inst_Append_NoArg(b, RETURN);
    // }
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
    ParserUnit *uu = Parser_Get_UpScope(ps);
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

static void show_unit(ParserState *ps)
{
  ParserUnit *u = ps->u;
  const char *scope = scope_name(u);
  char *name = u->sym != NULL ? u->sym->name : NULL;
  name = (name == NULL) ? ps->filename : name;
  Log_Puts("\n---------------------------------------------");
  Log_Printf("scope-%d(%s, %s) symbols:\n", ps->depth, scope, name);
  if (ps->depth == 1) {
    show_module_symbol(&ps->symbols);
  } else {
    STable_Show(u->stbl);
  }
  CodeBlock_Show(u->block);
  Log_Puts("---------------------------------------------\n");
}

static void check_unused_symbols(ParserState *ps)
{
  /* FIXME */
}

void Parser_Enter_Scope(ParserState *ps, ScopeKind scope)
{
  ParserUnit *u = new_unit(scope);
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
  const char *name = scope_name(ps->u);
  int depth = ps->depth;

  show_unit(ps);
  check_unused_symbols(ps);

  merge_unit(ps);

  /* free current parserunit */
  free_unit(ps->u);
  ps->u = NULL;

  /* restore ps->u to top of ps->ustack */
  struct list_head *head = list_first(&ps->ustack);
  if (head != NULL) {
    list_del(head);
    ps->u = container_of(head, ParserUnit, link);
  }
  ps->depth--;

  Log_Debug("\x1b[35mExit scope-%d(%s)\x1b[0m", depth, name);
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
  return 1;
  //return TypeDesc_Equal(t1, t2);
}

static void code_const_expr(ParserState *ps, Expr *exp)
{
  assert(exp->ctx == EXPR_LOAD);
  ParserUnit *u = ps->u;
  //no need generate load_const, if is constant value(stored in VarSymbol)
  if (!exp->omit)
    CODE_LOAD_CONST(u->block, ((ConstExpr *)exp)->value);
}

static void parse_self_expr(ParserState *ps, Expr *exp)
{
  assert(exp->ctx == EXPR_LOAD);
  Symbol *sym;
  ParserUnit *uu;
  int depth = ps->depth;
  list_for_each_entry(uu, &ps->ustack, link) {
    depth -= 1;
    sym = uu->sym;
    if (sym->kind == SYM_CLASS || sym->kind == SYM_TRAIT) {
      sym->used++;
      exp->sym = sym;
      exp->desc = sym->desc;
      TYPE_INCREF(exp->desc);
      Log_Debug("find self '%s' in up scope-%d(%s)",
                sym->name, depth, scope_name(uu));
      break;
    }
  }

  if (exp->sym == NULL) {
    Syntax_Error(&exp->pos, "cannot find self");
    return;
  }

  CODE_LOAD(ps->u->block, 0);
}

static void parse_super_expr(ParserState *ps, Expr *exp)
{
  assert(0);
}

static void parse_attribute_expr(ParserState *ps, Expr *exp)
{
  ParserUnit *u = ps->u;
  AttributeExpr *attrExp = (AttributeExpr *)exp;
  Expr *lexp = attrExp->left;
  TypeDesc *desc;
  Symbol *sym;

  /* parse left expression */
  lexp->ctx = EXPR_LOAD;
  lexp->right = exp;
  Parse_Expression(ps, lexp);
  if (lexp->sym == NULL) {
    assert(0);
    return;
  }

  Symbol *lsym = lexp->sym;
  switch (lsym->kind) {
  case SYM_VAR: {
    Log_Debug("'%s' is variable", lsym->name);
    assert(lsym->desc != NULL);
    String s = TypeDesc_ToString(lexp->desc);
    Log_Debug("left expr's type: %s", s.str);
    VarSymbol *varSym = (VarSymbol *)lsym;
    sym = STable_Get(varSym->stbl, attrExp->id.name);
    if (sym == NULL) {
      Syntax_Error(&attrExp->id.pos,
                   "'%s' is not found in '%s'", attrExp->id.name, lsym->name);
      return;
    }
    break;
  }
  case SYM_CLASS:
  case SYM_TRAIT: {
    Log_Debug("'%s' is class/trait", lsym->name);
    assert(lsym->desc != NULL);
    String s = TypeDesc_ToString(lexp->desc);
    Log_Debug("left expr's type: %s", s.str);
    ClassSymbol *clsSym = (ClassSymbol *)lsym;
    sym = STable_Get(clsSym->stbl, attrExp->id.name);
    if (sym == NULL) {
      Syntax_Error(&attrExp->id.pos,
                   "'%s' is not found in '%s'", attrExp->id.name, lsym->name);
      return;
    }
    break;
  }
  case SYM_ENUM: {
    Log_Debug("'%s is enum", lsym->name);
    assert(lsym->desc != NULL);
    String s = TypeDesc_ToString(lexp->desc);
    Log_Debug("left expr's type: %s", s.str);
    EnumSymbol *enumSym = (EnumSymbol *)lsym;
    sym = STable_Get(enumSym->stbl, attrExp->id.name);
    if (sym == NULL) {
      Syntax_Error(&attrExp->id.pos,
                   "'%s' is not found in '%s'", attrExp->id.name, lsym->name);
      return;
    }
    break;
  }
  case SYM_FUNC:
  case SYM_NFUNC: {
    if (lexp->kind == CALL_KIND) {
      ProtoDesc *proto = (ProtoDesc *)lsym->desc;
      if (Vector_Size((Vector *)proto->ret) != 1) {
        Syntax_Error(&lexp->pos,
                     "'%s' is multi-returns' function", lsym->name);
        return;
      }
    } else {
    }
    break;
  }
  case SYM_PKG: {
    Log_Debug("'%s' is external package", lsym->name);
    Package *pkg = ((PkgSymbol *)lsym)->pkg;
    sym = STable_Get(pkg->stbl, attrExp->id.name);
    if (sym == NULL) {
      Syntax_Error(&attrExp->id.pos,
                   "'%s' is not found in '%s'", attrExp->id.name, lsym->name);
      return;
    }
    break;
  }
  default:
    assert(0);
    break;
  }

  /* set expressions's symbol */
  String s = TypeDesc_ToString(sym->desc);
  Log_Debug("'%s' type: %s", sym->name, s.str);
  exp->sym = sym;
  exp->desc = sym->desc;
  TYPE_INCREF(exp->desc);
}

static void code_attribute_expr(ParserState *ps, Expr *exp)
{
  ParserUnit *u = ps->u;
  AttributeExpr *attrExp = (AttributeExpr *)exp;
  Expr *lexp = attrExp->left;

  /* code left expression */
  Code_Expression(ps, lexp);

  Symbol *sym = exp->sym;
  if (sym == NULL)
    return;

  switch (sym->kind) {
  case SYM_VAR: {
    break;
  }
  case SYM_AFUNC:
  case SYM_TRAIT: {
    break;
  }
  case SYM_EVAL: {
    assert(exp->ctx == EXPR_LOAD);
    Expr *right = exp->right;
    int argc = 0;
    if (right != NULL && right->kind == CALL_KIND)
      argc = Vector_Size(((CallExpr *)right)->args);
    CODE_NEW_ENUM(u->block, sym->name, argc);
    break;
  }
  case SYM_FUNC:
  case SYM_NFUNC: {
    assert(exp->ctx == EXPR_LOAD);
    Expr *right = exp->right;
    if (right->kind == CALL_KIND) {
      CallExpr *callExp = (CallExpr *)right;
      /* function call */
      int argc = Vector_Size(callExp->args);
      CODE_CALL(u->block, attrExp->id.name, argc);
    } else {

    }
    break;
  }
  case SYM_IFUNC: {
    break;
  }
  default:
    assert(0);
    break;
  }
}

static void parse_subscript_expr(ParserState *ps, Expr *exp)
{
  assert(0);
}

static int __check_call_arguments(TypeDesc *desc, Vector *args)
{
  assert(desc->kind == TYPE_PROTO);
  ProtoDesc *proto = (ProtoDesc *)desc;
  Vector *para = proto->arg;

  int ndesc = Vector_Size(para);
  int nexp = Vector_Size(args);
  if (nexp < ndesc)
    return 0;

  TypeDesc *type;
  int varg = 0;
  Expr *e;
  Vector_ForEach(e, args) {
    if (!varg) {
      type = Vector_Get(para, i);
      /* variably arguments */
      if (type->kind == TYPE_VARG) {
        assert(i + 1 == Vector_Size(para));
        varg = 1;
        type = ((VargDesc *)type)->base;
      }
    }
    //FIXME
    //if (!TypeDesc_Equal(type, e->desc))
    //  return 0;
  }

  return 1;
}

static int check_new_enum(ParserState *ps, EnumValSymbol *evSym, Vector *args)
{
  Vector *vec = evSym->types;
  if (Vector_Size(vec) != Vector_Size(args))
    return 0;
  TypeDesc *type;
  Expr *exp;
  Vector_ForEach(exp, args) {
    type = Vector_Get(vec, i);
    if (!TypeDesc_Equal(type, exp->desc))
      return 0;
  }
  return 1;
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
    ParserUnit *uu = Parser_Get_UpScope(ps);
    if (u->scope != SCOPE_FUNCTION || uu->scope != SCOPE_CLASS ||
        strcmp(u->sym->name, "__init__") || !list_empty(&u->block->insts)) {
      Syntax_Error(&lexp->pos,
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

  /* set call expression's symbol as its left expr's symbol */
  exp->sym = lexp->sym;

  /* check call's arguments */
  SymKind kind = exp->sym->kind;
  TypeDesc *proto;
  if (kind == SYM_CLASS) {
    ClassSymbol *clsSym = (ClassSymbol *)exp->sym;
    Log_Debug("call class '%s' __init__ function", clsSym->name);
    Symbol *__init__ = STable_Get(clsSym->stbl, "__init__");
    int argc = Vector_Size(callExp->args);
    if (__init__ == NULL && argc > 0) {
      Syntax_Error(&lexp->pos, "__init__ function needs no arguments");
      return;
    }

    if (__init__ == NULL) {
      proto = TypeDesc_New_Proto(NULL, clsSym->desc);
    } else {
      proto = __init__->desc;
    }

    /* set call expression's descriptor */
    assert(proto->kind == TYPE_PROTO);
    TYPE_INCREF(proto);
    exp->desc = proto;
    if (!__check_call_arguments(proto, callExp->args)) {
      Syntax_Error(&lexp->pos,
                  "argument of function '%s' are not matched", exp->sym->name);
    }

    //FIXME: new object() is not a function call
  } else if (kind == SYM_EVAL) {
    // check parameters whether they are matched with defined types
    EnumValSymbol *evSym = (EnumValSymbol *)exp->sym;
    exp->desc = evSym->esym->desc;
    TYPE_INCREF(exp->desc);
    if (!check_new_enum(ps, evSym, callExp->args)) {
      Syntax_Error(&exp->pos,
                  "argument of enum '%s' are not matched", exp->sym->name);
    }
  } else if (kind == SYM_ENUM) {
    Syntax_Error(&lexp->pos,
                 "enum '%s' cannot be instanced", exp->sym->name);
  } else {
    /* var(func type), func, ifunc and nfunc */
    Log_Debug("call var(proto)/interface/native function '%s'", exp->sym->name);
    /* set proto as its left expr's descriptor */
    proto = lexp->desc;
    /* set call expression's descriptor */
    assert(proto->kind == TYPE_PROTO);
    TYPE_INCREF(proto);
    exp->desc = proto;
    if (!__check_call_arguments(proto, callExp->args)) {
      Syntax_Error(&lexp->pos,
                  "argument of function '%s' are not matched", exp->sym->name);
    }
  }
}

static void code_call_expr(ParserState *ps, Expr *exp)
{
  ParserUnit *u = ps->u;
  CallExpr *callExp = (CallExpr *)exp;

  /* code argument-list */
  Expr *e;
  Vector_ForEach_Reverse(e, callExp->args) {
    Code_Expression(ps, e);
  }

  /* code left expression */
  Expr *lexp = callExp->left;
  Code_Expression(ps, lexp);
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
  { NULL,                 code_const_expr  },           /* LITERAL_KIND    */
  { Parse_Ident_Expr,     Code_Ident_Expr  },           /* ID_KIND         */
  { Parse_Unary_Expr,     Code_Unary_Expr  },           /* UNARY_KIND      */
  { Parse_Binary_Expr,    Code_Binary_Expr },           /* BINARY_KIND     */
  { parse_attribute_expr, code_attribute_expr },        /* ATTRIBUTE_KIND  */
  { parse_subscript_expr, NULL },                       /* SUBSCRIPT_KIND  */
  { parse_call_expr,      code_call_expr },             /* CALL_KIND       */
  { parse_slice_expr,     NULL },                       /* SLICE_KIND      */
  { parse_list_expr,      NULL },                       /* ARRAY_LIST_KIND */
  { parse_list_expr,      NULL },                       /* MAP_LIST_KIND   */
  { parse_mapentry_expr,  NULL },                       /* MAP_ENTRY_KIND  */
  { parse_array_expr,     NULL },                       /* ARRAY_KIND      */
  { parse_map_expr,       NULL },                       /* MAP_KIND        */
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
  /* if there are errors, stop generating codes */
  if (ps->errnum > 0)
    return;
  assert(exp->kind > 0 && exp->kind < nr_elts(expr_funcs));
  expr_func code = (expr_funcs + exp->kind)->code;
  if (code != NULL)
    code(ps, exp);
}

/*
 * visiting expr has two stage:
 * 1. parse expression
 * 2. generate code
 */
static inline void parser_visit_expr(ParserState *ps, Expr *exp)
{
  assert(exp->kind > 0 && exp->kind < nr_elts(expr_funcs));
  struct expr_func_s *func = expr_funcs + exp->kind;
  if (ps->errnum <= MAX_ERRORS && func->parse != NULL)
    func->parse(ps, exp);
  /* if there are errors, stop generating codes */
  if (ps->errnum <= 0 && func->code != NULL)
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
      Syntax_Error(&id->pos,
                   "var '%s' is already delcared in scope-%d(%s)",
                   id->name, depth, scope_name(uu));
      return NULL;
    }
    if (uu->scope == SCOPE_FUNCTION || uu->scope == SCOPE_CLOSURE)
      return uu;
  }
  return NULL;
}

/*
 * get type's symbol table
 * every type has its stbl, even if it's base type(e.g. int, string etc)
 */
static STable *get_type_stbl(ParserState *ps, TypeDesc *desc)
{
  STable *stbl = NULL;
  switch (desc->kind) {
  case TYPE_BASE: {
    break;
  }
  case TYPE_KLASS: {
    KlassDesc *klass = (KlassDesc *)desc;
    char *path = klass->path.str;
    char *type = klass->type.str;
    Log_Debug("try to get %s.%s stbl", path, type);
    if (path != NULL) {
      Package *pkg = Find_Package(path);
      assert(pkg != NULL);
      stbl = pkg->stbl;
    } else {
      Symbol *sym;
      Vector_ForEach(sym, &ps->symbols) {
        if (!strcmp(sym->name, type)) {
          if (sym->kind == SYM_CLASS || sym->kind == SYM_TRAIT) {
            ClassSymbol *clsSym = (ClassSymbol *)sym;
            stbl = clsSym->stbl;
          } else {
            assert(sym->kind == SYM_ENUM);
            EnumSymbol *eSym = (EnumSymbol *)sym;
            stbl = eSym->stbl;
          }
          break;
        }
      }
    }
    break;
  }
  case TYPE_VARG: {
    break;
  }
  default:
    assert(0);
    break;
  }
  return stbl;
}

static VarSymbol *add_update_var(ParserState *ps, Ident *id, TypeDesc *desc)
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
      String s = TypeDesc_ToString(desc);
      Log_Debug("update symbol '%s' type as '%s'", id->name, s.str);
      varSym->desc = desc;
      TYPE_INCREF(varSym->desc);
    }
    varSym->stbl = get_type_stbl(ps, desc);
    break;
  case SCOPE_FUNCTION:
  case SCOPE_CLOSURE: {
    /* function scope has independent space for save variables. */
    Log_Debug("var '%s' declaration in function", id->name);
    varSym = STable_Add_Var(u->stbl, id->name, desc);
    if (varSym == NULL) {
      Syntax_Error(&id->pos, "var '%s' is duplicated", id->name);
      return NULL;
    }
    varSym->stbl = get_type_stbl(ps, desc);
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
      Syntax_Error(&id->pos, "var '%s' is duplicated", id->name);
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
    varSym->stbl = get_type_stbl(ps, desc);
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

static void save_const_to_symbol(VarSymbol *varSym, Expr *exp)
{
  if (exp->kind == LITERAL_KIND) {
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
    if (konst)
      rexp->omit = 1;
    parser_visit_expr(ps, rexp);
    if (rexp->desc == NULL) {
      Syntax_Error(&rexp->pos, "cannot resolve right expression's type");
      return;
    }

    /* constant */
    if (konst && !Expr_Is_Const(rexp)) {
      Syntax_Error(&rexp->pos, "not a valid constant expression");
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
      if (rdesc->kind == TYPE_PROTO) {
        ProtoDesc *proto = (ProtoDesc *)rdesc;
        if (proto->ret == NULL) {
          Syntax_Error(&rexp->pos, "function has no return");
          return;
        }
        rdesc = proto->ret;
      } else {
        assert(rdesc->kind == TYPE_KLASS);
      }

      if (type->desc == NULL) {
        String s = TypeDesc_ToString(rdesc);
        Log_Debug("var '%s' type is none, set it as '%s'", id->name, s.str);
        TYPE_INCREF(rdesc);
        type->desc = rdesc;
      }
    }

    if (type->desc != NULL) {
      /* check type equals or not */
      if (!__type_is_compatible(type->desc, rdesc)) {
        Syntax_Error(&rexp->pos,
                     "right expression's type is not compatible");
        return;
      }
    } else {
      String s = TypeDesc_ToString(rdesc);
      Log_Debug("var '%s' type is none, set it as '%s'", id->name, s.str);
      TYPE_INCREF(rdesc);
      type->desc = rdesc;
    }
  }

  /* update or add symbol */
  VarSymbol *varSym = add_update_var(ps, id, type->desc);
  if (varSym == NULL)
    return;

  /* if it is constant, save the value to its symbol */
  if (konst)
    save_const_to_symbol(varSym, rexp);

  /* generate code, variable, but not constant */
  if (rexp != NULL && !konst) {
    ParserUnit *u = ps->u;
    if (u->scope == SCOPE_MODULE || u->scope == SCOPE_CLASS) {
      /* module or class variable, global variable */
      CODE_LOAD(u->block, 0);
      CODE_SET_ATTR(u->block, varSym->name);
    } else {
      /* others are local variables */
      CODE_STORE(u->block, varSym->index);
    }
  }
}

static void parse_const_decl(ParserState *ps, Stmt *stmt)
{
  VarDeclStmt *varStmt = (VarDeclStmt *)stmt;
  parse_variable(ps, &varStmt->id, &varStmt->type, varStmt->exp, 1);
}

static void parse_var_decl(ParserState *ps, Stmt *stmt)
{
  VarDeclStmt *varStmt = (VarDeclStmt *)stmt;
  parse_variable(ps, &varStmt->id, &varStmt->type, varStmt->exp, 0);
}

static void parse_statements(ParserState *ps, Vector *stmts);

static void parse_func_decl(ParserState *ps, Stmt *stmt)
{
  FuncDeclStmt *funStmt = (FuncDeclStmt *)stmt;

  Parser_Enter_Scope(ps, SCOPE_FUNCTION);
  ps->u->stbl = STable_New();

  ParserUnit *uu = Parser_Get_UpScope(ps);
  assert(uu != NULL);
  assert(uu->scope == SCOPE_MODULE || uu->scope == SCOPE_CLASS);
  Symbol *sym = STable_Get(uu->stbl, funStmt->id.name);
  assert(sym != NULL);
  ps->u->sym = sym;

  Log_Debug("----parse function '%s'----", funStmt->id.name);

  /* parameter-list */
  IdType *idtype;
  Vector_ForEach(idtype, funStmt->args) {
    parse_variable(ps, &idtype->id, &idtype->type, NULL, 0);
  }

  parse_statements(ps, funStmt->body);

  Log_Debug("----end of function '%s'----", funStmt->id.name);

  Parser_Exit_Scope(ps);
}

static void parse_proto_decl(ParserState *ps, Stmt *stmt)
{
  FuncDeclStmt *funStmt = (FuncDeclStmt *)stmt;
  if (funStmt->native)
    Log_Debug("'%s' is native func", funStmt->id.name);
  else
    Log_Debug("'%s' is proto", funStmt->id.name);
}

static void parse_class_supers(ParserState *ps, Vector *supers)
{

}

static void parse_class_decl(ParserState *ps, Stmt *stmt)
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

static void parse_trait_supers(ParserState *ps, Vector *supers)
{
}

static void parse_trait_decl(ParserState *ps, Stmt *stmt)
{
  KlassStmt *clsStmt = (KlassStmt *)stmt;
  Log_Debug("----parse trait '%s'----", clsStmt->id.name);

  Symbol *sym = STable_Get(ps->u->stbl, clsStmt->id.name);
  assert(sym);

  parse_trait_supers(ps, clsStmt->super);

  Parser_Enter_Scope(ps, SCOPE_CLASS);
  ps->u->sym = sym;
  ps->u->stbl = ((ClassSymbol *)sym)->stbl;

  /* parse class's body statements */
  parse_statements(ps, clsStmt->body);

  Parser_Exit_Scope(ps);

  Log_Debug("----end of trait '%s'----", clsStmt->id.name);
}

static void parse_enum_decl(ParserState *ps, Stmt *stmt)
{
  EnumStmt *eStmt = (EnumStmt *)stmt;
  Log_Debug("----parse enum '%s'----", eStmt->id.name);

  Symbol *sym = STable_Get(ps->u->stbl, eStmt->id.name);
  assert(sym != NULL);

  Parser_Enter_Scope(ps, SCOPE_CLASS);
  ps->u->sym = sym;
  ps->u->stbl = ((EnumSymbol *)sym)->stbl;

  /* parse class's body statements */
  parse_statements(ps, eStmt->body);

  Parser_Exit_Scope(ps);

  Log_Debug("----end of enum '%s'----", eStmt->id.name);
}

static void parse_enumval_decl(ParserState *ps, Stmt *stmt)
{
  EnumValStmt *evStmt = (EnumValStmt *)stmt;

  Log_Debug("----parse enumvalue '%s'----", evStmt->id.name);

  Log_Debug("----end of enumvalue '%s'----", evStmt->id.name);
}

static void parse_expr_stmt(ParserState *ps, Stmt *stmt)
{
  ExprStmt *expStmt = (ExprStmt *)stmt;
  Expr *exp = expStmt->exp;
  exp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, exp);
}

/*
  a = 100
  a.b = 200
  a.b.c = 300
  a.b[1].c = 400
  a.b(1,2).c = 500
  leftmost identifier is variable or imported external package name
 */
static void parse_assignment(ParserState *ps, Stmt *stmt)
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
    Syntax_Error(&rexp->pos, "cannot resolve right expression's type");
    return;
  }
  if (rdesc->kind == TYPE_PROTO) {
    #if 0
    ProtoDesc *proto = (ProtoDesc *)rdesc;
    int n = Vector_Size((Vector *)proto->ret);
    if (n != 1) {
      Syntax_Error(&rexp->pos, "multiple-value in single-value context");
      return;
    }
    rdesc = Vector_Get(proto->ret, 0);
    #endif
  }

  TypeDesc *ldesc = lexp->desc;
  if (!__type_is_compatible(ldesc, rdesc)) {
    Syntax_Error(&rexp->pos, "right expression's type is not compatible");
    return;
  }
}

static int check_returns(TypeDesc *desc, Expr *exp)
{
  assert(desc->kind == TYPE_PROTO);
  ProtoDesc *proto = (ProtoDesc *)desc;
  return 1; //TypeDesc_Equal(proto->ret, exp->desc);
}

static void parse_return(ParserState *ps, Stmt *stmt)
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

  Expr *e = retStmt->exp;
  if (e != NULL) {
    e->ctx = EXPR_LOAD;
    parser_visit_expr(ps, e);
  }

  if (!check_returns(funcSym->desc, e)) {
    Syntax_Error(&retStmt->pos, "func %s: returns are not matched.",
                 funcSym->name);
    return;
  }

  /* FIXME: all control flow branches to check
     need include returns' count in OP_RET n?
   */
  Inst_Append_NoArg(u->block, RETURN);
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

typedef void (*parse_stmt_func)(ParserState *, Stmt *);

static parse_stmt_func parse_stmt_funcs[] = {
  NULL,                     /* INVALID         */
  parse_const_decl,         /* CONST_KIND      */
  parse_var_decl,           /* VAR_KIND        */
  NULL,                     /* TUPLE_KIND      */
  parse_func_decl,          /* FUNC_KIND       */
  parse_proto_decl,         /* PROTO_KIND      */
  parse_class_decl,         /* CLASS_KIND      */
  parse_trait_decl,         /* TRAIT_KIND      */
  parse_enum_decl,          /* ENUM_KIND       */
  parse_enumval_decl,       /* ENUM_VALUE_KIND */
  parse_expr_stmt,          /* EXPR_KIND       */
  parse_assignment,         /* ASSIGN_KIND     */
  parse_return,             /* RETURN_KIND     */
  NULL,                     /* BREAK_KIND      */
  NULL,                     /* CONTINUE_KIND   */
  parse_list_stmt,          /* LIST_KIND       */
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
