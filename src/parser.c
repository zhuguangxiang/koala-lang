
#include "parser.h"
#include "checker.h"
#include "koala_tokens.h"
#include "koala_lex.h"
#include "state.h"
#include "package.h"
#include "hashfunc.h"
#include "log.h"

int Lexer_DoYYInput(ParserState *ps, char *buf, int size, FILE *in)
{
  LineBuffer *lb = &ps->line;

  if (lb->lineleft <= 0) {
    if (!fgets(lb->line, LINE_MAX_LEN, in)) {
      if (ferror(in)) clearerr(in);
      return 0;
    } else {
      lb->lineleft = lb->linelen = strlen(lb->line);
      lb->row++;
      lb->len = 0;
      lb->col = 0;
      lb->print = 0;
    }
  }

  int sz = min(lb->lineleft, size);
  memcpy(buf, lb->line, sz);
  lb->lineleft -= sz;
  return sz;
}

void Lexer_DoUserAction(ParserState *ps, char *text)
{
  LineBuffer *lb = &ps->line;
  lb->col += lb->len;
  strncpy(lb->token, text, TOKEN_MAX_LEN);
  lb->len = strlen(text);
}

void Parser_SetLine(ParserState *ps, struct expr *exp)
{
  if (exp) {
    lineinfo_t *l = &exp->line;
    LineBuffer *lb = &ps->line;
    l->line = strdup(lb->line);
    l->row = lb->row;
    l->col = lb->col;
  }
}

void Parser_PrintError(ParserState *ps, lineinfo_t *l, char *fmt, ...)
{
  if (++ps->errnum >= MAX_ERRORS) {
    fprintf(stderr, COLOR_LIGHT_WHITE "Too many errors.\n" COLOR_NC);
    exit(-1);
  }

  fprintf(stderr, COLOR_LIGHT_WHITE "%s:%d:%d: " COLOR_NC,
          ps->filename, l->row, l->col);

  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  fprintf(stderr, COLOR_LIGHT_RED "\n%s", l->line);
  char ch;
  for (int i = 0; i < l->col - 1; i++) {
    ch = l->line[i];
    if (!isspace(ch)) putchar(' '); else putchar(ch);
  }
  puts("^" COLOR_NC);
}

/*----------------------------------------------------------------------------*/

void codegen_binary(CodeBlock *block, int op)
{
  switch (op) {
    case BINARY_ADD: {
      Log_Debug("add 'OP_ADD'");
      Inst_Append(block, OP_ADD, NULL);
      break;
    }
    case BINARY_SUB: {
      Log_Debug("add 'OP_SUB'");
      Inst_Append(block, OP_SUB, NULL);
      break;
    }
  case BINARY_MULT: {
    Log_Debug("add 'OP_MUL'");
    Inst_Append(block, OP_MUL, NULL);
    break;
  }
  case BINARY_DIV: {
    Log_Debug("add 'OP_DIV'");
    Inst_Append(block, OP_DIV, NULL);
    break;
  }
  case BINARY_MOD: {
    Log_Debug("add 'OP_MOD'");
    Inst_Append(block, OP_MOD, NULL);
    break;
  }
    case BINARY_GT: {
      Log_Debug("add 'OP_GT'");
      Inst_Append(block, OP_GT, NULL);
      break;
    }
    case BINARY_LT: {
      Log_Debug("add 'OP_LT'");
      Inst_Append(block, OP_LT, NULL);
      break;
    }
    case BINARY_EQ: {
      Log_Debug("add 'OP_EQ'");
      Inst_Append(block, OP_EQ, NULL);
      break;
    }
    default: {
      assert(0);
      break;
    }
  }
}

void codegen_unary(CodeBlock *block, int op)
{
  switch (op) {
    case UNARY_PLUS: {
      Log_Debug("omit 'OP_PLUS'");
      break;
    }
    case UNARY_MINUS: {
      Log_Debug("add 'OP_NEG'");
      Inst_Append(block, OP_NEG, NULL);
      break;
    }
    case UNARY_BIT_NOT: {
      Log_Debug("add 'OP_BNOT'");
      Inst_Append(block, OP_BNOT, NULL);
      break;
    }
    case UNARY_LNOT: {
      Log_Debug("add 'OP_LNOT'");
      Inst_Append(block, OP_LNOT, NULL);
      break;
    }
    default: {
      assert(0);
      break;
    }
  }
}

/*-------------------------------------------------------------------------*/
static void parser_visit_expr(ParserState *ps, struct expr *exp);
static ParserUnit *get_class_unit(ParserState *ps);
static void parser_vist_stmt(ParserState *ps, stmt_t *stmt);

/*-------------------------------------------------------------------------*/

static Symbol *find_external_package(ParserState *ps, char *path)
{
  Import key = {.path = path};
  HashNode *hnode = HashTable_Find(&ps->imports, &key);
  Import *import;
  if (!hnode) {
    Log_Error("cannot find '%s' package", path);
    return NULL;
  }
  import = container_of(hnode, Import, hnode);
  return import->sym;
}

static Symbol *find_userdef_symbol(ParserState *ps, TypeDesc *desc)
{
  Symbol *sym;
  if (desc->klass.path.str) {
    // find in external imported module
    sym = find_external_package(ps, desc->klass.path.str);
    if (!sym) {
      Log_Error("cannot find '%s' package", desc->klass.path.str);
      return NULL;
    }
    sym->refcnt++;
  } else {
    // find in current module
    sym = ps->sym;
  }

  assert(sym->kind == SYM_STABLE || sym->kind == SYM_MODULE);

  sym = STable_Get(sym->ptr, desc->klass.type.str);
  if (sym) {
    Log_Debug("find '%s.%s'", desc->klass.path.str, desc->klass.type.str);
    sym->refcnt++;
    return sym;
  } else {
    Log_Error("cannot find '%s.%s'",
              desc->klass.path.str, desc->klass.type.str);
    return NULL;
  }
}

static Symbol *find_symbol_linear_order(Symbol *clssym, char *name)
{
  Symbol *sym;
  Log_Debug(">>>>find_symbol_linear_order<<<<");
  //find in current class
  Log_Debug("finding '%s' in '%s'", name, clssym->name);
  sym = STable_Get(clssym->ptr, name);
  if (sym) {
    Log_Debug("'%s' is found in '%s'", name, clssym->name);
    return sym;
  }
  Log_Debug("'%s' is not in '%s'", name, clssym->name);

  //find in linear-order traits
  Symbol *s;
  Vector_ForEach_Reverse(s, &clssym->traits) {
    sym = find_symbol_linear_order(s, name);
    if (sym) {
      return sym;
    }
  }

  // //find in super class
  // if (clssym->super)
  // 	return find_symbol_linear_order(clssym->super, name);
  // else
    return NULL;
}

/*-------------------------------------------------------------------------*/
// External imported modules management

static Import *import_new(char *path)
{
  Import *import = malloc(sizeof(Import));
  import->path = strdup(path);
  Init_HashNode(&import->hnode, import);
  return import;
}

static void import_free(Import *import)
{
  free(import);
}

static uint32 import_hash(void *k)
{
  Import *import = k;
  return hash_string(import->path);
}

static int import_equal(void *k1, void *k2)
{
  Import *import1 = k1;
  Import *import2 = k2;
  return !strcmp(import1->path, import2->path);
}

static void init_imports(ParserState *ps)
{
  HashTable_Init(&ps->imports, import_hash, import_equal);
  ps->extstbl = STable_New();
  Symbol *sym = Parser_New_Import(ps, "lang", "lang");
  sym->refcnt = 1;
}

static void __import_free_fn(HashNode *hnode, void *arg)
{
  UNUSED_PARAMETER(arg);
  Import *import = container_of(hnode, Import, hnode);
  //free(import->path);
  import_free(import);
}

static void fini_imports(ParserState *ps)
{
  HashTable_Fini(&ps->imports, __import_free_fn, NULL);
  //STable_Free(ps->extstbl);
}

/*----------------------------------------------------------------------------*/

static uint32 extpkg_hash(void *k)
{
  struct extpkg *ext = k;
  return hash_string(ext->path);
}

static int extpkg_equal(void *k1, void *k2)
{
  struct extpkg *ext1 = k1;
  struct extpkg *ext2 = k2;
  return !strcmp(ext1->path, ext2->path);
}

void init_extpkg_hashtable(HashTable *table)
{
  HashTable_Init(table, extpkg_hash, extpkg_equal);
}

/*----------------------------------------------------------------------------*/

static ParserUnit *parent_scope(ParserState *ps)
{
  if (list_empty(&ps->ustack))
    return NULL;
  struct list_head *first = list_first(&ps->ustack);
  return container_of(first, ParserUnit, link);
}

/*--------------------------------------------------------------------------*/

struct path_stbl_struct {
  char *path;
  STable *stbl;
};

static void __to_stbl_fn(HashNode *hnode, void *arg)
{
  MemberDef *member = container_of(hnode, MemberDef, hnode);
  struct path_stbl_struct *path_struct = arg;
  STable *stbl = path_struct->stbl;
  char *path = path_struct->path;

  if (member->kind == MEMBER_CLASS || member->kind == MEMBER_TRAIT) {
    Symbol *s = STable_Add_Symbol(stbl, member->name, SYM_STABLE, 0);
    s->desc = TypeDesc_New_Klass(path, member->name);
    s->ptr = STable_New();
    struct path_stbl_struct tmp = {path, s->ptr};
    HashTable_Visit(member->klazz->table, __to_stbl_fn, &tmp);
  } else if (member->kind == MEMBER_VAR) {
    STable_Add_Var(stbl, member->name, member->desc, member->k);
  } else if (member->kind == MEMBER_CODE) {
    //FIXME
    STable_Add_Proto(stbl, member->name, member->desc);
  } else if (member->kind == MEMBER_PROTO) {
    //FIXME
    STable_Add_IProto(stbl, member->name, member->desc);
  } else {
    assert(0);
  }
}

/* for compiler only */
STable *Module_To_STable(Object *ob, char *path)
{
  Package *m = (Package *)ob;
  STable *stbl = STable_New();
  struct path_stbl_struct path_struct = {path, stbl};
  HashTable_Visit(m->table, __to_stbl_fn, &path_struct);
  return stbl;
}

// API used by yacc

static Symbol *add_import(STable *stbl, char *id, char *path)
{
  Symbol *sym = STable_Add_Symbol(stbl, id, SYM_STABLE, 0);
  if (!sym) return NULL;
  sym->path = strdup(path);
  return sym;
}

struct extpkg *get_extpkg(ParserState *ps, char *path)
{
  struct extpkg key = {.path = path};
  HashNode *hnode = HashTable_Find(&ps->pkg->expkgs, &key);
  return hnode ? container_of(hnode, struct extpkg, hnode) : NULL;
}

struct extpkg *put_extpkg(ParserState *ps, char *path, STable *stbl, char *id)
{
  struct extpkg *ext = malloc(sizeof(struct extpkg));
  Init_HashNode(&ext->hnode, ext);
  ext->path = strdup(path);
  ext->stbl = stbl;
  ext->id   = id;
  HashTable_Insert(&ps->pkg->expkgs, &ext->hnode);
  return ext;
}

struct extpkg *load_extpkg(ParserState *ps, char *path, int *exist)
{
  struct extpkg *extpkg = get_extpkg(ps, path);
  if (extpkg) {
    *exist = 1;
    return extpkg;
  }

  Object *ob = Koala_Get_Package(path);
  if (!ob) {
    Log_Warn("load package '%s' failed, try to compile it", path);
    pid_t pid = fork();
    if (pid == 0) {
      Log_Debug("child process %d", getpid());
      int argc = 3 + options_number(ps->pkg->options);
      char *argv[argc];
      argv[0] = "koalac";
      options_toarray(ps->pkg->options, argv, 1);
      argv[argc - 2] = path;
      argv[argc - 1] = NULL;
      execvp("koalac", argv);
      assert(0); /* never go here */
    }
    int status = 0;
    pid = wait(&status);
    Log_Debug("child process %d return status:%d", pid, status);
    if (WIFEXITED(status)) {
      int exitstatus = WEXITSTATUS(status);
      Log_Debug("child process %d: %s", pid, strerror(exitstatus));
      if (exitstatus) exit(-1);
    }
    ob = Koala_Load_Package(path);
    if (!ob) {
      Log_Error("load module '%s' failed", path);
      exit(-1);
    }
  }

  STable *extstbl = Module_To_STable(ob, path);
  if (!extstbl) {
    Log_Error("load module '%s' failed", path);
    exit(-1);
  }

  extpkg = put_extpkg(ps, path, extstbl, Package_Name(ob));
  if (!extpkg) {
    Log_Error("load module '%s' failed", path);
    STable_Free(extstbl);
    exit(-1);
  }

  return extpkg;
}

Symbol *Parser_New_Import(ParserState *ps, char *id, char *path)
{
  Import key = {.path = path};
  Import *import = (Import *)HashTable_Find(&ps->imports, &key);
  Symbol *sym;
  if (import) {
    Log_Error("package '%s' existed", path);
    PSError("package '%s' existed", path);
    return NULL;
  }

  int exist = 0;
  struct extpkg *extpkg = load_extpkg(ps, path, &exist);
  if (!extpkg) return NULL;

  if (!id) id = extpkg->id;
  sym = add_import(ps->extstbl, id, path);
  if (!sym) {
    Log_Debug("add import '%s <- %s' failed", id, path);
    return NULL;
  }
  sym->ptr = extpkg->stbl;
  import = import_new(path);
  import->sym = sym;
  lineinfo_t *l = &import->line;
  LineBuffer *lb = &ps->line;
  l->line = strdup(lb->line);
  l->row = lb->row;
  l->col = 1;
  sym->import = import;
  if (exist) {
    Log_Debug("package '%s <- %s' existed", id, path);
  } else {
    int res = HashTable_Insert(&ps->imports, &import->hnode);
    assert(!res);
    Log_Debug("add package '%s <- %s' successfully", id, path);
  }

  return sym;
}

char *Parser_Get_FullPath(ParserState *ps, char *id)
{
  Symbol *sym = STable_Get(ps->extstbl, id);
  if (!sym) {
    Log_Error("cannot find package: '%s'", id);
    return NULL;
  }
  assert(sym->kind == SYM_STABLE);
  sym->refcnt++;
  return sym->path;
}

/*---------------------------------------------------------------------------*/

static inline void __add_stmt(ParserState *ps, stmt_t *stmt)
{
  Vector_Append(&ps->stmts, stmt);
}

static void __new_var(ParserState *ps, char *id, TypeDesc *desc, int konst)
{
#define VAR_SHOW konst ? "const" : "var", id

  /* add variale's id to symbol table */
  Symbol *sym = STable_Add_Var(ps->u->stbl, id, desc, konst);
  if (sym) {
    Log_Debug("add %s '%s' successfully", VAR_SHOW);
    sym->up = ps->u->sym;
  } else {
    PSError("add %s '%s' failed", VAR_SHOW);
  }
}

void Parser_New_Vars(ParserState *ps, stmt_t *stmt)
{
  if (!stmt) return;

  if (stmt->kind == LIST_KIND) {
    stmt_t *s;
    Vector_ForEach(s, &stmt->list) {
      assert(s->kind == VAR_KIND);
      /* add statement for next parser */
      __add_stmt(ps, s);
      __new_var(ps, s->var.id, s->var.desc, s->var.konst);
    }
  } else {
    assert(stmt->kind == VARLIST_KIND);
    __add_stmt(ps, stmt);
    char *id;
    Vector_ForEach(id, stmt->vars.ids) {
      __new_var(ps, id, stmt->vars.desc, stmt->vars.konst);
    }
  }
}

static void parse_funcdecl(ParserState *ps, stmt_t *stmt)
{
  TypeDesc *proto;
  Symbol *sym;
  Vector *pdesc = NULL;
  stmt_t *s;
  if (stmt->func.args) {
    pdesc = Vector_New();
    Vector_ForEach(s, stmt->func.args) {
      Vector_Append(pdesc, TypeDesc_Dup(s->var.desc));
    }
  }
  proto = TypeDesc_New_Proto(pdesc, stmt->func.rets);
  sym = STable_Add_Proto(ps->u->stbl, stmt->func.id, proto);
  if (sym) {
    Log_Debug("add func '%s' successfully", stmt->func.id);
    sym->up = ps->u->sym;
  } else {
    Log_Debug("add func '%s' failed", stmt->func.id);
    PSError("%s redeclared in the package %s", stmt->func.id, ps->pkg->pkgname);
  }
}

void Parser_New_Func(ParserState *ps, stmt_t *stmt)
{
  if (!stmt) return;
  __add_stmt(ps, stmt);
  parse_funcdecl(ps, stmt);
}

static void parse_funcproto(ParserState *ps, stmt_t *stmt)
{
  TypeDesc *proto;
  Symbol *sym;
  proto = TypeDesc_New_Proto(stmt->proto.args, stmt->proto.rets);
  sym = STable_Add_IProto(ps->u->stbl, stmt->proto.id, proto);
  if (sym) {
    Log_Debug("add abstract func '%s' successful", stmt->proto.id);
    sym->up = ps->u->sym;
  } else {
    Log_Debug("add abstract func '%s' failed", stmt->proto.id);
  }
}

void Parser_New_ClassOrTrait(ParserState *ps, stmt_t *stmt)
{
  Symbol *sym = NULL;
  __add_stmt(ps, stmt);

  if (stmt->kind == CLASS_KIND) {
    sym = STable_Add_Class(ps->u->stbl, stmt->class_info.id);
  } else if (stmt->kind == TRAIT_KIND) {
    sym = STable_Add_Trait(ps->u->stbl, stmt->class_info.id);
  } else {
    assert(0);
  }

  sym->up = ps->u->sym;
  sym->ptr = STable_New();
  sym->desc = TypeDesc_New_Klass(NULL, stmt->class_info.id);
  Log_Debug(">>>>add class(trait) '%s' successfully", sym->name);

  parser_enter_scope(ps, sym->ptr, SCOPE_CLASS);
  ps->u->sym = sym;
  if (stmt->class_info.body) {
    stmt_t *s;
    Vector_ForEach(s, stmt->class_info.body) {
      if (s->kind == VAR_KIND) {
        __new_var(ps, s->var.id, s->var.desc, s->var.konst);
      } else if (s->kind == FUNC_KIND) {
        parse_funcdecl(ps, s);
      } else {
        assert(s->kind == PROTO_KIND);
        parse_funcproto(ps, s);
      }
    }
  }
  parser_exit_scope(ps);

  Log_Debug(">>>>end class(trait) '%s'", sym->name);
}

void Parser_New_TypeAlias(ParserState *ps, stmt_t *stmt)
{
  STable_Add_TypeAlias(ps->u->stbl, stmt->typealias.id, stmt->typealias.desc);
  Log_Debug("add typealias '%s' successful", stmt->typealias.id);
}

/*---------------------------------------------------------------------------*/

#if 0
static struct expr *optimize_binary_add(struct expr *l, struct expr *r)
{
  struct expr *e;
  if (l->kind == INT_KIND) {
    int64 val;
    if (r->kind == INT_KIND) {
      val = l->ival + r->ival;
    } else if (r->kind == FLOAT_KIND) {
      val = l->ival + r->fval;
    } else {
      assert(0);
    }
    e = expr_from_int(val);
  } else if (l->kind == FLOAT_KIND) {
    float64 val;
    if (r->kind == INT_KIND) {
      val = l->fval + r->ival;
    } else if (r->kind == FLOAT_KIND) {
      val = l->fval + r->fval;
    } else {
      assert(0);
    }
    e = expr_from_float(val);
  } else {
    kassert(0, "unsupported optimized type:%d", l->kind);
  }
  return e;
}

static struct expr *optimize_binary_sub(struct expr *l, struct expr *r)
{
  struct expr *e;
  if (l->kind == INT_KIND) {
    int64 val;
    if (r->kind == INT_KIND) {
      val = l->ival - r->ival;
    } else if (r->kind == FLOAT_KIND) {
      val = l->ival - r->fval;
    } else {
      assert(0);
    }
    e = expr_from_int(val);
  } else if (l->kind == FLOAT_KIND) {
    float64 val;
    if (r->kind == INT_KIND) {
      val = l->fval - r->ival;
    } else if (r->kind == FLOAT_KIND) {
      val = l->fval - r->fval;
    } else {
      assert(0);
    }
    e = expr_from_float(val);
  } else {
    kassert(0, "unsupported optimized type:%d", l->kind);
  }
  return e;
}

static int optimize_binary_expr(ParserState *ps, struct expr **exp)
{
  if (ps->olevel <= 0) return 0;

  int ret = 0;
  struct expr *origin = *exp;
  struct expr *left = origin->binary.left;
  struct expr *right = origin->binary.right;
  if (left->konst && right->konst) {
    ret = 1;
    struct expr *e = NULL;
    switch (origin->binary.op) {
      case BINARY_ADD: {
        Log_Debug("optimize add");
        e = optimize_binary_add(left, right);
        break;
      }
      case BINARY_SUB: {
        Log_Debug("optimize sub");
        e = optimize_binary_sub(left, right);
        break;
      }
      default: {
        assert(0);
      }
    }
    //free origin, left and right expression
    *exp = e;
  }

  return ret;
}

#endif

/*--------------------------------------------------------------------------*/

static void parser_show_scope(ParserState *ps)
{
#if 1
  Log_Debug("------scope show-------------");
  Log_Debug("scope-%d symbols:", ps->nestlevel);
  STable_Show(ps->u->stbl, 0);
  CodeBlock_Show(ps->u->block);
  Log_Debug("-----scope show end----------");
#else
  UNUSED_PARAMETER(ps);
#endif
}

static void parser_new_block(ParserUnit *u)
{
  if (u->block) return;
  CodeBlock *block = CodeBlock_New();
  u->block = block;
}

/*--------------------------------------------------------------------------*/

int check_npr_func(Symbol *sym)
{
  TypeDesc *desc = sym->desc;
  int rsz = Vector_Size(desc->proto.ret);
  int psz = Vector_Size(desc->proto.arg);
  return (rsz == 0 && psz == 0) ? 0 : -1;
}

static void parser_merge(ParserState *ps)
{
  ParserUnit *u = ps->u;
  // save code to symbol
  if (u->scope == SCOPE_FUNCTION || u->scope == SCOPE_METHOD) {
    if (u->scope == SCOPE_METHOD && !strcmp(u->sym->name, "__init__")) {
      Log_Debug("class __init__ function");
      Inst_Append_NoArg(u->block, OP_LOAD0);
    }
    if (!u->block->bret) {
      Log_Debug("add 'return' to function '%s'", u->sym->name);
      Inst_Append_NoArg(u->block, OP_RET);
    }
    u->sym->ptr = u->block;
    u->block = NULL;
    u->sym->locvars = u->stbl->varcnt;
    u->stbl = NULL;
    Log_Debug("save code to function '%s'", u->sym->name);
  } else if (u->scope == SCOPE_BLOCK) {
    if (u->loop) {
      // loop-statement check break or continue statement
      int offset;
      JmpInst *jmp;
      Vector_ForEach(jmp, &u->jmps) {
        if (jmp->type == JMP_BREAK) {
          offset = u->block->bytes - jmp->inst->upbytes;
        } else {
          assert(jmp->type == JMP_CONTINUE);
          offset = 0 - jmp->inst->upbytes;
        }
        jmp->inst->arg.kind = ARG_INT;
        jmp->inst->arg.ival = offset;
      }
      u->merge = 1;
    }

    ParserUnit *parent = parent_scope(ps);
    Log_Debug("merge code to parent's block(%d)", parent->scope);
    assert(parent->block);
    if (u->merge || parent->scope == SCOPE_FUNCTION ||
      u->scope == SCOPE_METHOD) {
      Log_Debug("merge code to up block");
      CodeBlock_Merge(u->block, parent->block);
    } else {
      Log_Debug("set block as up block's next");
      assert(parent->scope == SCOPE_BLOCK);
      parent->block->next = u->block;
      u->block = NULL;
    }
  } else if (u->scope == SCOPE_MODULE) {
    #if 0
    TypeDesc *proto = Type_New_Proto(NULL, NULL);
    Symbol *sym = STable_Add_Proto(u->stbl, "__init__", proto);
    assert(sym);
    Log_Debug("add 'return' to function '__init__'");
    Inst_Append(u->block, OP_RET, NULL);
    if (u->block && u->block->bytes > 0) {
      sym->ptr = u->block;
      //sym->stbl = u->stbl;
      sym->locvars = u->stbl->varcnt;
      //u->stbl = NULL;
    }
    u->sym = sym;
    u->block = NULL;
    Log_Debug("save code to module '__init__' function");
    #endif
  } else if (u->scope == SCOPE_CLASS) {

    if (u->block && u->block->bytes > 0) {
      Log_Debug("merge code into class or trait '%s' __init__ function",
        u->sym->name);
      Symbol *sym = STable_Get(u->sym->ptr, "__init__");
      if (!sym) {
        TypeDesc *proto = TypeDesc_New_Proto(NULL, NULL);
        sym = STable_Add_Proto(u->sym->ptr, "__init__", proto);
        assert(sym);
        Inst_Append_NoArg(u->block, OP_LOAD0);
        Log_Debug("add 'return' to function '__init__'");
        Inst_Append_NoArg(u->block, OP_RET);
      }

      if (u->sym->kind == SYM_TRAIT) {
        if(check_npr_func(sym)) {
          Log_Error("trait cannot have __init__ function");
          return;
        }
      } else {
        assert(u->sym->kind == SYM_CLASS);
      }

      CodeBlock_Merge(sym->ptr, u->block);
      CodeBlock *b = sym->ptr;
      assert(list_empty(&b->insts));
      assert(!b->next);
      CodeBlock_Free(b);
      sym->ptr = u->block;
      sym->locvars = u->stbl->varcnt;
      Log_Debug("save code to class '__init__'(defined) function");
    }	else {
      Symbol *sym = STable_Get(u->sym->ptr, "__init__");
      if (!sym) {
        TypeDesc *proto = TypeDesc_New_Proto(NULL, NULL);
        sym = STable_Add_Proto(u->sym->ptr, "__init__", proto);
        assert(sym);
        Inst_Append_NoArg(u->block, OP_LOAD0);
        Log_Debug("add 'return' to function '__init__'");
        Inst_Append_NoArg(u->block, OP_RET);
        sym->ptr = u->block;
        sym->locvars = 2; //FIXME
        Log_Debug("save code to class '__init__' function");
      }
    }

    u->block = NULL;
  } else {
    assert(0);
  }
}

static void parser_init_unit(ParserUnit *u, STable *stbl, int scope)
{
  init_list_head(&u->link);
  u->stbl = stbl;
  u->sym = NULL;
  u->block = NULL;
  u->scope = scope;
  Vector_Init(&u->jmps);
}

static void vec_jmpinst_free_fn(void *item, void *arg)
{
  UNUSED_PARAMETER(arg);
  JmpInst_Free(item);
}

static void parser_fini_unit(ParserUnit *u)
{
  list_del(&u->link);
  CodeBlock *b = u->block;
  //FIXME:
  if (b) {
    assert(list_empty(&b->insts));
  }

  Vector_Fini(&u->jmps, vec_jmpinst_free_fn, NULL);
  //FIXME
  //assert(!u->stbl);
  //STable_Free(u->stbl);
}

static ParserUnit *parser_new_unit(STable *stbl, int scope)
{
  ParserUnit *u = calloc(1, sizeof(ParserUnit));
  parser_init_unit(u, stbl, scope);
  return u;
}

static void parser_free_unit(ParserUnit *u)
{
  parser_fini_unit(u);
  free(u);
}

void parser_enter_scope(ParserState *ps, STable *stbl, int scope)
{
  //AtomTable *atbl = NULL;
//	if (ps->u) atbl = ps->u->stbl->atbl;
  if (!stbl) stbl = STable_New();
  ParserUnit *u = parser_new_unit(stbl, scope);

  /* Push the old ParserUnit on the stack. */
  if (ps->u) {
    list_add(&ps->u->link, &ps->ustack);
  }
  ps->u = u;
  ps->nestlevel++;
  parser_new_block(ps->u);
}

void parser_exit_scope(ParserState *ps)
{
  parser_show_scope(ps);

  check_unused_symbols(ps);

  parser_merge(ps);

  ParserUnit *u = ps->u;

  if (u->scope == SCOPE_MODULE) {
    //save module's symbol to ps
    //assert(ps->sym->ptr == u->stbl);
    u->stbl = NULL;
  } else if (u->scope == SCOPE_CLASS) {
    assert(!list_empty(&ps->ustack));
    ParserUnit *parent = NULL;
    struct list_head *first = list_first(&ps->ustack);
    parent = container_of(first, ParserUnit, link);
    assert(parent->scope == SCOPE_MODULE);
    // class symbol table is stored in class symbol
    u->stbl = NULL;
  } else {
    Log_Debug(">>>> exit scope:%d", u->scope);
  }

  assert(ps->u != &ps->pkg->top);
  parser_free_unit(u);
  ps->nestlevel--;

  /* Restore c->u to the parent unit. */
  struct list_head *first = list_first(&ps->ustack);
  if (first) {
    list_del(first);
    ps->u = container_of(first, ParserUnit, link);
  } else {
    ps->u = NULL;
  }
}

/*--------------------------------------------------------------------------*/

static void parser_expr_desc(ParserState *ps, struct expr *exp, Symbol *sym)
{
  exp->sym = sym;
  if (!exp->desc) {
    if (sym->kind == SYM_MODULE) {
      Log_Debug("ident '%s' is as Module", ps->filename);
    } else {
      char buf[64];
      TypeDesc_ToString(sym->desc, buf);
      Log_Debug("ident '%s' is as '%s'", exp->id, buf);
      exp->desc = sym->desc;
    }
  }
}

static void parser_curscope_ident(ParserState *ps, Symbol *sym,
  struct expr *exp)
{
  ParserUnit *u = ps->u;
  switch (u->scope) {
    case SCOPE_MODULE:
    case SCOPE_CLASS: {
      assert(exp->ctx == EXPR_LOAD);
      if (sym->kind == SYM_VAR) {
        Log_Debug("symbol '%s' is variable", sym->name);
        Inst_Append_NoArg(u->block, OP_LOAD0);
        Argument val = {.kind = ARG_STR, .str = sym->name};
        Inst_Append(u->block, OP_GETFIELD, &val);
      } else if (sym->kind == SYM_PROTO) {
        Log_Debug("symbol '%s' is function", sym->name);
        if (exp->right && exp->right->kind == CALL_KIND) {
          Inst_Append_NoArg(u->block, OP_LOAD0);
          Argument val = {.kind = ARG_STR, .str = sym->name};
          Inst *i = Inst_Append(u->block, OP_CALL, &val);
          i->argc = exp->argc;
        } else {
          Log_Debug("load '%s' func to reg", sym->name);
          Inst_Append_NoArg(u->block, OP_LOAD0);
          Argument val = {.kind = ARG_STR, .str = sym->name};
          Inst_Append(u->block, OP_GETFIELD, &val);
          exp->ctx = EXPR_LOAD_FUNC;
        }
      } else {
        assert(0);
      }
      break;
    }
    case SCOPE_FUNCTION:
    case SCOPE_METHOD:
    case SCOPE_BLOCK: {
      if (sym->kind == SYM_VAR) {
        Log_Debug("symbol '%s' is variable", sym->name);
        if (sym->desc->kind == TYPE_PROTO &&
          exp->right && exp->right->kind == CALL_KIND) {
          Inst_Append_NoArg(u->block, OP_LOAD0);
        }
        int opcode;
        Log_Debug("local's variable");
        opcode = (exp->ctx == EXPR_LOAD) ? OP_LOAD : OP_STORE;
        Argument val = {.kind = ARG_INT, .ival = sym->index};
        Inst_Append(u->block, opcode, &val);
        if (sym->desc->kind == TYPE_PROTO &&
          exp->right && exp->right->kind == CALL_KIND) {
          Inst *i = Inst_Append(u->block, OP_CALL0, &val);
          i->argc = exp->argc;
        }
      } else {
        assert(0);
      }
      break;
    }
    default: {
      assert(0);
      break;
    }
  }
}

static void parser_upscope_ident(ParserState *ps, Symbol *sym,
  struct expr *exp)
{
  ParserUnit *u = ps->u;
  switch (u->scope) {
    case SCOPE_CLASS: {
      assert(sym->up->kind == SYM_MODULE);
      Log_Debug("symbol '%s' in module '%s'", sym->name, ps->filename);
      if (sym->kind == SYM_VAR) {
        Log_Debug("symbol '%s' is variable", sym->name);
        Inst_Append_NoArg(u->block, OP_LOAD0);
        Inst_Append_NoArg(u->block, OP_GETM);
        Argument val = {.kind = ARG_STR, .str = sym->name};
        Inst_Append(u->block, OP_GETFIELD, &val);
      } else if (sym->kind == SYM_PROTO) {
        Log_Debug("symbol '%s' is function", sym->name);
        Inst_Append_NoArg(u->block, OP_LOAD0);
        Inst_Append_NoArg(u->block, OP_GETM);
        Argument val = {.kind = ARG_STR, .str = sym->name};
        Inst *i = Inst_Append(u->block, OP_CALL, &val);
        i->argc = exp->argc;
      } else {
        assert(0);
      }
      break;
    }
    case SCOPE_FUNCTION: {
      assert(sym->up->kind == SYM_MODULE);
      Log_Debug("symbol '%s' in module '%s'", sym->name, ps->filename);
      if (sym->kind == SYM_VAR) {
        Log_Debug("symbol '%s' is variable", sym->name);
        int opcode;
        if (sym->desc->kind == TYPE_PROTO &&
          exp->right && exp->right->kind == CALL_KIND) {
          Inst_Append_NoArg(u->block, OP_LOAD0);
        }
        Inst_Append_NoArg(u->block, OP_LOAD0);
        opcode = (exp->ctx == EXPR_LOAD) ? OP_GETFIELD : OP_SETFIELD;
        Argument val = {.kind = ARG_STR, .str = sym->name};
        Inst_Append(u->block, opcode, &val);
        if (sym->desc->kind == TYPE_PROTO &&
          exp->right && exp->right->kind == CALL_KIND) {
          Inst *i = Inst_Append(u->block, OP_CALL0, &val);
          i->argc = exp->argc;
        }
      } else if (sym->kind == SYM_PROTO) {
        Log_Debug("symbol '%s' is function", sym->name);
        if (exp->right && exp->right->kind == CALL_KIND) {
          Inst_Append_NoArg(u->block, OP_LOAD0);
          Argument val = {.kind = ARG_STR, .str = sym->name};
          Inst *i = Inst_Append(u->block, OP_CALL, &val);
          i->argc = exp->argc;
        } else {
          Log_Debug("load '%s' func to reg", sym->name);
          Inst_Append_NoArg(u->block, OP_LOAD0);
          Argument val = {.kind = ARG_STR, .str = sym->name};
          Inst_Append(u->block, OP_GETFIELD, &val);
          exp->ctx = EXPR_LOAD_FUNC;
        }
      } else if (sym->kind == SYM_CLASS) {
        Log_Debug("symbol '%s' is class", sym->name);
        if (exp->right && exp->right->kind == CALL_KIND) {
          Log_Debug("new object :%s", exp->sym->name);
          Inst_Append_NoArg(u->block, OP_LOAD0);
          Inst_Append_NoArg(u->block, OP_GETM);
          Argument val = {.kind = ARG_STR, .str = sym->name};
          Inst *i = Inst_Append(u->block, OP_NEW, &val);
          i->argc = exp->argc;
          val.str = "__init__";
          i = Inst_Append(u->block, OP_CALL, &val);
          i->argc = exp->argc;
        } else {
          if (!exp->right) {
            Log_Debug("'%s' is a class", sym->name);
            Inst_Append_NoArg(u->block, OP_LOAD0);
            Inst_Append_NoArg(u->block, OP_GETM);
          } else {
            assert(0);
          }
        }
      } else {
        assert(0);
      }
      break;
    }
    case SCOPE_METHOD: {
      if (sym->up->kind == SYM_CLASS || sym->up->kind == SYM_TRAIT) {
        Log_Debug("symbol '%s' in class '%s'", sym->name, sym->up->name);
        Log_Debug("symbol '%s' is inherited ? %s", sym->name,
          sym->inherited ? "true" : "false");
        if (sym->kind == SYM_VAR) {
          Log_Debug("symbol '%s' is variable", sym->name);
          Inst_Append_NoArg(u->block, OP_LOAD0);
          int opcode = (exp->ctx == EXPR_LOAD) ? OP_GETFIELD : OP_SETFIELD;
          Argument val = {.kind = ARG_STR, .str = sym->name};
          Inst_Append(u->block, opcode, &val);
        } else if (sym->kind == SYM_PROTO) {
          Log_Debug("symbol '%s' is function", sym->name);
          Inst_Append_NoArg(u->block, OP_LOAD0);
          Argument val = {.kind = ARG_STR, .str = sym->name};
          Inst *i = Inst_Append(u->block, OP_CALL, &val);
          i->argc = exp->argc;
        } else if (sym->kind == SYM_IPROTO) {
          Log_Debug("symbol '%s' is abstract function", sym->name);
          Inst_Append_NoArg(u->block, OP_LOAD0);
          Argument val = {.kind = ARG_STR, .str = sym->name};
          Inst *i = Inst_Append(u->block, OP_CALL, &val);
          i->argc = exp->argc;
        } else {
          assert(0);
        }
      } else {
        assert(sym->up->kind == SYM_MODULE);
        Log_Debug("symbol '%s' in module '%s'", sym->name, ps->filename);
        if (sym->kind == SYM_VAR) {
          Log_Debug("symbol '%s' is variable", sym->name);
          Inst_Append_NoArg(u->block, OP_LOAD0);
          Inst_Append_NoArg(u->block, OP_GETM);
          int opcode = (exp->ctx == EXPR_LOAD) ? OP_GETFIELD : OP_SETFIELD;
          Argument val = {.kind = ARG_STR, .str = sym->name};
          Inst_Append(u->block, opcode, &val);
        } else if (sym->kind == SYM_PROTO) {
          Log_Debug("symbol '%s' is function", sym->name);
          Inst_Append_NoArg(u->block, OP_LOAD0);
          Inst_Append_NoArg(u->block, OP_GETM);
          Argument val = {.kind = ARG_STR, .str = sym->name};
          Inst *i = Inst_Append(u->block, OP_CALL, &val);
          i->argc = exp->argc;
        } else if (sym->kind == SYM_CLASS) {
          Log_Debug("symbol '%s' is class", sym->name);
          Inst_Append_NoArg(u->block, OP_LOAD0);
          Inst_Append_NoArg(u->block, OP_GETM);
          Argument val = {.kind = ARG_STR, .str = sym->name};
          Inst *i = Inst_Append(u->block, OP_NEW, &val);
          i->argc = exp->argc;
          val.str = "__init__";
          i = Inst_Append(u->block, OP_CALL, &val);
          i->argc = exp->argc;
        } else {
          assert(0);
        }
      }
      break;
    }
    case SCOPE_BLOCK: {
      Symbol *up = sym->up;
      if (!up || up->kind == SYM_PROTO) {
        Log_Debug("symbol '%s' is local variable", sym->name);
        // local's variable
        int opcode = (exp->ctx == EXPR_LOAD) ? OP_LOAD : OP_STORE;
        Argument val = {.kind = ARG_INT, .ival = sym->index};
        Inst_Append(u->block, opcode, &val);
      } else if (up->kind == SYM_CLASS) {
        Log_Debug("symbol '%s' is class variable", sym->name);
        Log_Debug("symbol '%s' is inherited ? %s", sym->name,
          sym->inherited ? "true" : "false");
        Inst_Append_NoArg(u->block, OP_LOAD0);
        int opcode = (exp->ctx == EXPR_LOAD) ? OP_GETFIELD : OP_SETFIELD;
        Argument val = {.kind = ARG_STR, .str = sym->name};
        Inst_Append(u->block, opcode, &val);
      } else if (up->kind == SYM_MODULE) {
        Log_Debug("symbol '%s' is module variable", sym->name);
        Inst_Append_NoArg(u->block, OP_LOAD0);
        Inst_Append_NoArg(u->block, OP_GETM);
        int opcode = (exp->ctx == EXPR_LOAD) ? OP_GETFIELD : OP_SETFIELD;
        Argument val = {.kind = ARG_STR, .str = sym->name};
        Inst_Append(u->block, opcode, &val);
      } else {
        assert(0);
      }
      break;
    }
    default: {
      assert(0);
      break;
    }
  }
}

#if 0
static void parser_superclass_ident(ParserState *ps, Symbol *sym,
  struct expr *exp)
{
  ParserUnit *u = ps->u;
  switch (u->scope) {
    case SCOPE_METHOD:
    case SCOPE_BLOCK: {
      ParserUnit *pu = get_class_unit(ps);
      assert(ps);
      break;
    }
    default: {
      kassert(0, "invalid scope:%d", u->scope);
      break;
    }
  }
}
#endif

static void gencode_id_in_super(ParserState *ps, struct expr *exp, Symbol *sym)
{
  ParserUnit *u = ps->u;
  char *id = exp->id;

  sym->refcnt++;
  parser_expr_desc(ps, exp, sym);

  Argument val = {.kind = ARG_STR, .str = id};

  if (sym->kind == SYM_VAR) {
    if (exp->ctx == EXPR_LOAD) {
      Log_Debug("load:%s", val.str);
      Inst_Append_NoArg(u->block, OP_LOAD0);
      Inst_Append(ps->u->block, OP_GETFIELD, &val);
    } else {
      assert(exp->ctx == EXPR_STORE);
      Log_Debug("store:%s", val.str);
      Inst_Append_NoArg(u->block, OP_LOAD0);
      Inst_Append(ps->u->block, OP_SETFIELD, &val);
    }
  } else if (sym->kind == SYM_PROTO) {
    Inst_Append_NoArg(u->block, OP_LOAD0);
    Inst *i = Inst_Append(u->block, OP_CALL, &val);
    i->argc = exp->argc;
  } else {
    assert(0);
  }
}

static void parser_ident(ParserState *ps, struct expr *exp)
{
  ParserUnit *u = ps->u;
  char *id = exp->id;

  // find ident from local scope
  Symbol *sym = STable_Get(u->stbl, id);
  if (sym) {
    Log_Debug("symbol '%s' is found in local scope", id);
    sym->refcnt++;
    parser_expr_desc(ps, exp, sym);
    parser_curscope_ident(ps, sym, exp);
    return;
  }

  // find ident from parent scope
  ParserUnit *up;
  struct list_head *pos;
  list_for_each(pos, &ps->ustack) {
    up = container_of(pos, ParserUnit, link);
    sym = STable_Get(up->stbl, id);
    if (sym) {
      Log_Debug("symbol '%s' is found in up scope", id);
      sym->refcnt++;
      parser_expr_desc(ps, exp, sym);
      parser_upscope_ident(ps, sym, exp);
      return;
    }
  }

  // ident is in traits ?
  ParserUnit *cu = get_class_unit(ps);
  if (cu && Vector_Size(&cu->sym->traits) > 0) {
    Symbol *trait;
    Vector_ForEach(trait, &cu->sym->traits) {
      sym = STable_Get(trait->ptr, id);
      if (sym) {
        Log_Debug("symbol '%s' is found in trait '%s'", id, trait->name);
        gencode_id_in_super(ps, exp, sym);
        return;
      }
    }
  }

  // ident is in super ?
  Symbol *super = NULL;
  if (cu && cu->sym->super) {
    super = cu->sym->super;
    sym = STable_Get(super->ptr, id);
    if (sym) {
      Log_Debug("symbol '%s' is found in super '%s'", id, super->name);
      gencode_id_in_super(ps, exp, sym);
      return;
    }
  }

  // ident is current module's name
  #if 0
  FIXME
  if (ps->package && !strcmp(id, ps->package)) {
    sym = ps->sym;
    assert(sym);
    Log_Debug("symbol '%s' is current module", id);
    sym->refcnt++;
    parser_expr_desc(ps, exp, sym);
    assert(exp->ctx == EXPR_LOAD);
    Inst_Append_NoArg(u->block, OP_LOAD0);
    Inst_Append_NoArg(u->block, OP_GETM);
    return;
  }
  #endif

  // find ident from external module
  sym = STable_Get(ps->extstbl, id);
  if (sym) {
    Log_Debug("symbol '%s' is found in external scope", id);
    assert(sym->kind == SYM_STABLE);
    sym->refcnt++;
    parser_expr_desc(ps, exp, sym);
    assert(exp->ctx == EXPR_LOAD);
    Log_Debug("symbol '%s' is module", sym->name);
    Argument val = {.kind = ARG_STR, .str = sym->path};
    Inst_Append(u->block, OP_LOADM, &val);
    return;
  }

  Parser_PrintError(ps, &exp->line, "cannot find '%s'", id);
}

static void parser_attribute(ParserState *ps, struct expr *exp)
{
  struct expr *left = exp->attribute.left;
  left->right = exp;
  left->ctx = EXPR_LOAD;
  parser_visit_expr(ps, left);

  if (exp->supername)
    Log_Debug(">>>(%s).%s", exp->supername, exp->attribute.id);
  else
    Log_Debug(">>>.%s", exp->attribute.id);

  Symbol *leftsym = left->sym;
  if (!leftsym) {
    return;
  }

  Symbol *sym = NULL;
  if (leftsym->kind == SYM_STABLE) {
    Log_Debug(">>>>symbol '%s' is a module", leftsym->name);
    sym = STable_Get(leftsym->ptr, exp->attribute.id);
    if (!sym) {
      Parser_PrintError(ps, &exp->line, "cannot find '%s' in '%s'",
        exp->attribute.id, left->str);
      exp->sym = NULL;
      return;
    }
    if (sym->kind == SYM_STABLE) {
      if (!sym->desc) {
        Log_Warn("symbol '%s' desc is null", sym->name);
        sym->desc = TypeDesc_New_Klass(leftsym->path, sym->name);
      }
    }
  } else if (leftsym->kind == SYM_VAR) {
    Log_Debug("symbol '%s' is a variable", leftsym->name);
    assert(leftsym->desc);
    sym = find_userdef_symbol(ps, leftsym->desc);
    if (!sym) {
      char buf[64];
      TypeDesc_ToString(leftsym->desc, buf);
      Log_Error("cannot find '%s' in '%s'", exp->attribute.id, buf);
      return;
    }
    assert(sym->kind == SYM_STABLE ||
      sym->kind == SYM_CLASS || sym->kind == SYM_TRAIT);
    char *typename = sym->name;
    sym = find_symbol_linear_order(sym, exp->attribute.id);
    if (!sym) {
      Log_Error("cannot find '%s' in '%s'", exp->attribute.id, typename);
      return;
    }
  } else if (leftsym->kind == SYM_CLASS) {
    Log_Debug("symbol '%s' is a class", leftsym->name);
    sym = find_symbol_linear_order(leftsym, exp->attribute.id);
    //sym = STable_Get(leftsym->ptr, exp->attribute.id);
    if (!sym) {
      Log_Error("cannot find '%s' in '%s'", exp->attribute.id, leftsym->name);
      return;
      // if (!leftsym->super) {
      // 	Log_Error("cannot find '%s' in '%s' class",
      // 		exp->attribute.id, leftsym->name);
      // 	return;
      // }

      // sym = STable_Get(leftsym->super->ptr, exp->attribute.id);
      // if (!sym) {
      // 	Log_Error("cannot find '%s' in super '%s' class",
      // 		exp->attribute.id, leftsym->super->name);
      // 	return;
      // }

      // Log_Debug("find '%s' in super '%s' class",
      // 	exp->attribute.id, leftsym->super->name);
    } else {
      Log_Debug("find '%s' in '%s' class", exp->attribute.id, leftsym->name);
    }
  } else if (leftsym->kind == SYM_MODULE) {
    Log_Debug("symbol '%s' is a module", leftsym->name);
    char *typename = leftsym->name;
    sym = STable_Get(leftsym->ptr, exp->attribute.id);
    if (!sym) {
      Log_Error("cannot find '%s' in '%s'", exp->attribute.id, typename);
      return;
    }
  } else if (leftsym->kind == SYM_TRAIT) {
    Log_Debug("symbol '%s' is a trait", leftsym->name);
    char *typename = leftsym->name;
    sym = STable_Get(leftsym->ptr, exp->attribute.id);
    if (!sym) {
      Log_Error("cannot find '%s' in '%s'", exp->attribute.id, typename);
      return;
    }
  } else {
    assert(0);
  }

  // set expression's symbol
  exp->sym = sym;
  exp->desc = sym->desc;

  // generate code
  ParserUnit *u = ps->u;
  Argument val = {.kind = ARG_STR, .str = NULL};

  if (sym->kind == SYM_VAR) {
    if (exp->supername) {
      char *namebuf = malloc(strlen(exp->supername) + 1 +
        strlen(exp->attribute.id) + 1);
      assert(namebuf);
      sprintf(namebuf, "%s.%s", exp->supername, exp->attribute.id);
      val.str = namebuf;
    } else {
      val.str = exp->attribute.id;
    }

    if (exp->ctx == EXPR_LOAD) {
      Log_Debug("load:%s", val.str);
      Inst_Append(ps->u->block, OP_GETFIELD, &val);
    } else {
      assert(exp->ctx == EXPR_STORE);
      Log_Debug("store:%s", val.str);
      Inst_Append(ps->u->block, OP_SETFIELD, &val);
    }
  } else if (sym->kind == SYM_PROTO) {
    if (exp->supername) {
      char *namebuf = malloc(strlen(exp->supername) + 1 +
        strlen(exp->attribute.id) + 1);
      assert(namebuf);
      sprintf(namebuf, "%s.%s", exp->supername, exp->attribute.id);
      val.str = namebuf;
    } else {
      val.str = exp->attribute.id;
    }
    Inst *i = Inst_Append(u->block, OP_CALL, &val);
    i->argc = exp->argc;
  } else if (sym->kind == SYM_STABLE) {
    // new object
    val.str = sym->name;
    Inst *i = Inst_Append(u->block, OP_NEW, &val);
    i->argc = exp->argc;
    val.str = "__init__";
    i = Inst_Append(u->block, OP_CALL, &val);
    i->argc = exp->argc;
  } else if (sym->kind == SYM_IPROTO) {
    if (left->kind == SUPER_KIND) {
      //for super.xyz(), super is abstract method and not impl in traits
      char *namebuf = malloc(strlen("super") + 1 +
        strlen(exp->attribute.id) + 1);
      assert(namebuf);
      sprintf(namebuf, "super.%s", exp->attribute.id);
      val.str = namebuf;
      Inst *i = Inst_Append(u->block, OP_CALL, &val);
      i->argc = exp->argc;
    } else {
      val.str = sym->name;
      Inst *i = Inst_Append(u->block, OP_CALL, &val);
      i->argc = exp->argc;
    }
  } else {
    assert(0);
  }
}

static void parser_array(ParserState *ps, expr_t *exp)
{
  expr_t *e;
  int count = 0;
  Vector_ForEach_Reverse(e, exp->list) {
    count++;
    parser_visit_expr(ps, e);
  }

  //FIXME: count > STACK_SIZE ?
  Argument arg = {.kind = ARG_INT, .ival = count};
  Inst_Append(ps->u->block, OP_NEWARRAY, &arg);
}

static void parser_subscript(ParserState *ps, expr_t *exp)
{
  expr_t *left = exp->subscript.left;
  left->right = exp;
  left->ctx = EXPR_LOAD;
  parser_visit_expr(ps, left);

  expr_t *index = exp->subscript.index;
  index->ctx = EXPR_LOAD;
  parser_visit_expr(ps, index);

  exp->desc = left->desc->array.base;

  if (exp->ctx == EXPR_LOAD) {
    Inst_Append_NoArg(ps->u->block, OP_LOAD_SUBSCR);
  } else {
    assert(exp->ctx == EXPR_STORE);
    Inst_Append_NoArg(ps->u->block, OP_STORE_SUBSCR);
  }
}

static void parser_call(ParserState *ps, struct expr *exp)
{
  int argc = 0;
  //check super()
  if (exp->call.left->kind == SUPER_KIND) {
    assert(ps->u->scope == SCOPE_METHOD);
    assert(!strcmp(ps->u->sym->name, "__init__"));
    assert(list_empty(&ps->u->block->insts));
  }

  if (exp->call.args) {
    struct expr *e;
    Vector_ForEach_Reverse(e, exp->call.args) {
      e->ctx = EXPR_LOAD;
      parser_visit_expr(ps, e);
    }
    argc = Vector_Size(exp->call.args);
  }

  struct expr *left = exp->call.left;
  left->right = exp;
  left->ctx = EXPR_LOAD;
  left->argc = argc;
  parser_visit_expr(ps, left);
  if (!left->sym) {
    return;
  }

  Symbol *sym = left->sym;
  assert(sym);
  if (sym->kind == SYM_PROTO) {
    Log_Debug("call %s()", sym->name);
    /* function type */
    exp->sym = sym;
    exp->desc = sym->desc;
    /* check arguments */
    // if (!check_call_args(exp->desc, exp->call.args)) {
    // 	Log_Error("arguments are not matched.");
    // 	return;
    // }
  } else if (sym->kind == SYM_CLASS || sym->kind == SYM_STABLE) {
    Log_Debug("class '%s'", sym->name);
    /* class type */
    exp->sym = sym;
    exp->desc = sym->desc;
    /* get __init__ function */
    Symbol *__init__ = STable_Get(sym->ptr, "__init__");
    if (__init__) {
      TypeDesc *proto = __init__->desc;
      /* check arguments and returns(no returns) */
      if (!check_call_args(proto, exp->call.args)) {
        Log_Error("Arguments of __init__ are not matched.");
        return;
      }
      /* check no returns */
      if (Vector_Size(proto->proto.ret) > 0) {
        Log_Error("Returns of __init__ must be 0");
        return;
      }
    } else {
      Log_Debug("'%s' class is not defined __init__", sym->name);
      if (argc) {
        Log_Error("There is no __init__, but pass %d arguments", argc);
        return;
      }
    }
  } else if (sym->kind == SYM_IPROTO) {
    Log_Debug("call abstract method %s()", sym->name);
    /* function type */
    exp->sym = sym;
    exp->desc = sym->desc;
    /* check arguments */
    if (!check_call_args(exp->desc, exp->call.args)) {
      Log_Error("abstract method arguments are not matched.");
      return;
    }
  } else if (sym->kind == SYM_VAR) {
    if (sym->desc->kind == TYPE_PROTO) {
      /* function type */
      exp->sym = sym;
      exp->desc = sym->desc;
      /* check arguments */
      if (!check_call_args(exp->desc, exp->call.args)) {
        Log_Error("arguments are not matched.");
        return;
      }
    } else {
      Parser_PrintError(ps, &exp->line, "'%s' is not a func", sym->name);
      return;
    }
  } else {
    assert(0);
  }
}

static ParserUnit *get_class_unit(ParserState *ps)
{
  if (ps->u->scope != SCOPE_METHOD) return NULL;
  ParserUnit *u;
  struct list_head *pos;
  list_for_each(pos, &ps->ustack) {
    u = container_of(pos, ParserUnit, link);
    if (u->scope == SCOPE_CLASS) return u;
  }
  return NULL;
}

static void handle_traits(ParserState *ps, Symbol *sym)
{
  ParserUnit *u = ps->u;
  Symbol *trait;
  Argument val = {.kind = ARG_STR};

  Vector_ForEach(trait, &sym->traits) {
    char *namebuf = malloc(strlen(trait->name) + 1 + strlen("__init__") + 1);
    assert(namebuf);
    sprintf(namebuf, "%s.__init__", trait->name);
    Log_Debug("call trait '%s' __init__ function", trait->name);
    Inst_Append_NoArg(u->block, OP_LOAD0);
    val.str = namebuf;
    Inst *i = Inst_Append(u->block, OP_CALL, &val);
    i->argc = 0;
  }
}

static void parser_super(ParserState *ps, struct expr *exp)
{
  ParserUnit *u = ps->u;
  ParserUnit *cu = get_class_unit(ps);

  //NOTES: no need find traits, find in super class only
  Symbol *super;
  struct expr *r = exp->right;
  if (r->kind == CALL_KIND) {
    assert(u->scope == SCOPE_METHOD);
    assert(!strcmp(u->sym->name, "__init__"));
    assert(cu->sym->kind == SYM_CLASS);
    assert(cu->sym->super);
    exp->sym = cu->sym->super;
    super = cu->sym->super;
    char *namebuf = malloc(strlen(super->name) + 1 + strlen(u->sym->name) + 1);
    assert(namebuf);
    sprintf(namebuf, "%s.%s", super->name, u->sym->name);
    Inst_Append_NoArg(u->block, OP_LOAD0);
    Argument val = {.kind = ARG_STR, .str = namebuf};
    Inst *i = Inst_Append(u->block, OP_CALL, &val);
    i->argc = exp->argc;
    handle_traits(ps, cu->sym);
  } else if (r->kind == ATTRIBUTE_KIND) {
    Inst_Append_NoArg(u->block, OP_LOAD0);
    //find in linear-order traits
    Symbol *s;
    Symbol *sym = NULL;
    Vector_ForEach_Reverse(s, &cu->sym->traits) {
      sym = STable_Get(s->ptr, exp->right->attribute.id);
      if (sym) break;
    }
    if (sym) {
      exp->sym = s;
      super = s;
    } else {
      assert(cu->sym->super);
      exp->sym = cu->sym->super;
      super = cu->sym->super;
    }
  } else {
    assert(0);
  }

  r->supername = super->name;
}

/*
static void parser_binary(ParserState *ps, struct expr *exp)
{
  Log_Debug("binary_op:%d", exp->binary.op);
  exp->binary.right->ctx = EXPR_LOAD;
  parser_visit_expr(ps, exp->binary.right);
  exp->binary.left->ctx = EXPR_LOAD;
  parser_visit_expr(ps, exp->binary.left);
  exp->desc = exp->binary.left->desc;

  // generate code
  if (exp->binary.op == BINARY_ADD) {
    Log_Debug("add 'OP_ADD'");
    Inst_Append(ps->u->block, OP_ADD, NULL);
  } else if (exp->binary.op == BINARY_SUB) {
    Log_Debug("add 'OP_SUB'");
    Inst_Append(ps->u->block, OP_SUB, NULL);
  }
  // if (optimize_binary_expr(ps, &exp)) {
  //   exp->gencode = 1;
  //   parser_visit_expr(ps, exp);
  // } else {
  //   exp->binary.left->gencode = 1;
  //   exp->binary.right->gencode = 1;
  //   parser_visit_expr(ps, exp->binary.right);
  //   parser_visit_expr(ps, exp->binary.left);
  //   // generate code
  //   Log_Debug("add 'OP_ADD' inst");
  //   Inst_Append(ps->u->block, OP_ADD, NULL);
  // }
}
*/

static void parser_visit_expr(ParserState *ps, struct expr *exp)
{
  switch (exp->kind) {
    case ID_KIND: {
      parser_ident(ps, exp);
      break;
    }
    case INT_KIND: {
      assert(exp->ctx == EXPR_LOAD);
      Argument val = {.kind = ARG_INT, .ival = exp->ival};
      Inst_Append(ps->u->block, OP_LOADK, &val);
      break;
    }
    case FLOAT_KIND: {
      assert(exp->ctx == EXPR_LOAD);
      Argument val = {.kind = ARG_FLOAT, .fval = exp->fval};
      Inst_Append(ps->u->block, OP_LOADK, &val);
      break;
    }
    case BOOL_KIND: {
      assert(exp->ctx == EXPR_LOAD);
      Argument val = {.kind = ARG_BOOL, .fval = exp->bval};
      Inst_Append(ps->u->block, OP_LOADK, &val);
      break;
    }
    case STRING_KIND: {
      assert(exp->ctx == EXPR_LOAD);
      Argument val = {.kind = ARG_STR, .str = exp->str};
      Inst_Append(ps->u->block, OP_LOADK, &val);
      break;
    }
    case SELF_KIND: {
      assert(exp->ctx == EXPR_LOAD);
      Log_Debug("load self");
      ParserUnit *cu = get_class_unit(ps);
      exp->sym = cu->sym;
      Inst_Append_NoArg(ps->u->block, OP_LOAD0);
      break;
    }
    case SUPER_KIND: {
      assert(exp->ctx == EXPR_LOAD);
      Log_Debug("load super");
      parser_super(ps, exp);
      break;
    }
    case TYPEOF_KIND: {
      assert(exp->ctx == EXPR_LOAD);
      Log_Debug("typeof");
      assert(exp->right->kind == CALL_KIND);
      Symbol *sym = find_external_package(ps, "lang");
      assert(sym);
      sym = STable_Get(sym->ptr, "TypeOf");
      assert(sym);
      exp->sym = sym;

      Argument val = {.kind = ARG_STR, .str = "lang"};
      Inst_Append(ps->u->block, OP_LOADM, &val);
      val.str = "TypeOf";
      Inst *i = Inst_Append(ps->u->block, OP_CALL, &val);
      i->argc = exp->argc;
      break;
    }
    case ARRAY_KIND: {
      parser_array(ps, exp);
      break;
    }
    case ATTRIBUTE_KIND: {
      parser_attribute(ps, exp);
      break;
    }
    case SUBSCRIPT_KIND: {
      parser_subscript(ps, exp);
      break;
    }
    case CALL_KIND: {
      parser_call(ps, exp);
      break;
    }
    case BINARY_KIND: {
      Log_Debug("binary_op:%d", exp->binary.op);
      exp->binary.right->ctx = EXPR_LOAD;
      parser_visit_expr(ps, exp->binary.right);
      exp->binary.left->ctx = EXPR_LOAD;
      parser_visit_expr(ps, exp->binary.left);
      if (binop_relation(exp->binary.op)) {
        exp->desc = &Bool_Type;
      } else {
        exp->desc = exp->binary.left->desc;
      }
      codegen_binary(ps->u->block, exp->binary.op);
      break;
    }
    case UNARY_KIND: {
      Log_Debug("unary_op:%d", exp->unary.op);
      exp->unary.operand->ctx = EXPR_LOAD;
      parser_visit_expr(ps, exp->unary.operand);
      exp->desc = TypeDesc_Dup(exp->unary.operand->desc);
      codegen_unary(ps->u->block, exp->unary.op);
      break;
    }
    default: {
      assert(0);
      break;
    }
  }
}

/*--------------------------------------------------------------------------*/

static ParserUnit *get_function_unit(ParserState *ps)
{
  ParserUnit *u;
  struct list_head *pos;
  list_for_each(pos, &ps->ustack) {
    u = container_of(pos, ParserUnit, link);
    if (u->scope == SCOPE_FUNCTION || u->scope == SCOPE_METHOD) return u;
  }
  return NULL;
}

#if 0
static int __intf_cls_check(Symbol *intf, Symbol *cls)
{
  int result = -1;
  Symbol *method;
  Symbol *base = cls;
  while (base) {
    method = STable_Get(base->ptr, intf->name);
    if (!method) {
      if (!base->super) {
        Log_Error("Is '%s' in '%s'? no", intf->name, base->name);
        result = -1;
        break;
      } else {
        Log_Debug("go super class to check '%s' is in '%s'",
          intf->name, base->name);
      }
    } else {
      Log_Debug("Is '%s' in '%s'? yes", intf->name, base->name);
      if (!Proto_IsEqual(method->desc->proto, intf->desc->proto)) {
        Log_Error("abstract func check failed");
        result = -1;
      } else {
        Log_Debug("abstract func check ok");
        result = 0;
      }
      break;
    }
    base = base->super;
  }
  return result;
}

struct intf_inherit_struct {
  Symbol *sym;
  int result;
};

static void __intf_func_fn(Symbol *sym, void *arg)
{
  struct intf_inherit_struct *temp = arg;
  if (temp->result) return;

  Symbol *sub = temp->sym;
  assert(sym->kind == SYM_IPROTO);
  assert(sub->kind == SYM_CLASS);
  temp->result = __intf_cls_check(sym, sub);
}

static int check_intf_cls_inherit(Symbol *base, Symbol *sub)
{
  struct intf_inherit_struct temp = {sub, 0};
  STable_Traverse(base->ptr, __intf_func_fn, &temp);
  return temp.result;
}

static int check_intf_intf_inherit(Symbol *base, Symbol *sub)
{
  if (!sub->table) return -1;
  BaseSymbol basesym = {.sym = base};
  BaseSymbol *temp = HashTable_Find(sub->table, &basesym);
  return temp ? 0 : -1;
}

static int check_symbol_inherit(Symbol *base, Symbol *sub)
{
  Symbol *up = sub->super;
  while (up) {
    if (up == base) return 0;
    up = up->super;
  }
  return -1;
}
#endif

#if 0
int check_variable_type(ParserState *ps, stmt_t *stmt)
{
  char *id = stmt->var.id;
  TypeDesc *desc = stmt->var.desc;
  expr_t *rexp = stmt->var.exp;

  if (desc && desc->kind == TYPE_ARRAY) {
    /* check array's base type */
    desc = desc->array.base;
  }

  if (desc && desc->kind == TYPE_USRDEF) {
    STable *stbl = ps->sym->ptr;
    if (desc->usrdef.path) {
      Import key = {.path = desc->usrdef.path};
      Import *import = HashTable_Find(&ps->imports, &key);
      assert(import);
      stbl = import->sym->ptr;
    }
    Symbol *sym = STable_Get(stbl, desc->usrdef.type);
    if (!sym) {
      PSError("undefined '%s'", desc->usrdef.type);
      return -1;
    }
    /*
    if (sym->kind == SYM_TYPEALIAS) {
      Log_Debug("symbol '%s' is typealias, set its real type", sym->name);
      var->desc = sym->desc;
      var->typealias = 1;
    }
    */
  }

  if (rexp) {
    rexp->ctx = EXPR_LOAD;
    parser_visit_expr(ps, rexp);
    if (!rexp->desc) {
      Log_Error("cannot resolve var '%s' type", id);
      return -1;
    }

    if (!desc) {
      desc = Type_Dup(rexp->desc);
      stmt->var.desc = desc;
    }

    if (!Type_Equal(desc, rexp->desc)) {
      PSError("typecheck failed");
      return -1;
    }
#if 1
    if (exp->desc->kind == TYPE_PROTO) {
      TypeDesc *proto = exp->desc;
      if (exp->ctx == EXPR_LOAD) {
        int rsize = Vector_Size(proto->proto.ret);
        if (rsize != 1) {
          //FIXME
          Log_Error("function's returns is not 1:%d", rsize);
          return;
        }

        if (!var->desc) {
          var->desc = Vector_Get(proto->proto.ret, 0);
        } else {
          if (!Type_Equal(var->desc, Vector_Get(proto->proto.ret, 0))) {
            Log_Error("proto typecheck failed");
            return;
          }
        }
      } else if (exp->ctx == EXPR_LOAD_FUNC) {
        if (!var->desc) {
          var->desc = proto;
        } else {
          if (!Type_Equal(var->desc, proto)) {
            Log_Error("proto typecheck failed");
            return;
          }
        }
      } else {
        kassert(0, "invalid expr context:%d", exp->ctx);
        return;
      }
    } else {
      if (!var->desc) {
        var->desc = exp->desc;
      }

      if (!Type_Equal(var->desc, exp->desc)) {
        //literal check failed
        if (var->desc->kind == TYPE_USRDEF &&
          exp->desc->kind == TYPE_USRDEF) {
          //Symbol *s1 = find_userdef_symbol(ps, var->desc);
          //Symbol *s2 = find_userdef_symbol(ps, exp->desc);
          // if (s1->kind == SYM_INTF) {
          // 	if (s2->kind == SYM_CLASS) {
          // 		if (check_intf_cls_inherit(s1, s2)) {
          // 			Log_Error("check intf <- class failed");
          // 			return;
          // 		}
          // 	} else if (s2->kind == SYM_INTF) {
          // 		if (check_intf_intf_inherit(s1, s2)) {
          // 			Log_Error("check intf <- intf failed");
          // 			return;
          // 		}
          // 	} else {
          // 		kassert(0, "invalid:%d", s2->kind);
          // 	}
          // } else {
          // 	if (check_symbol_inherit(s1, s2)) {
          // 		Log_Error("check_symbol_inherit(class <- class) failed");
          // 		return;
          // 	}
          // }
        } else {
          Log_Error("typecheck failed");
          return;
        }
      }
    }
#endif
  }

  return 0;
}

#endif

static void parse_variable(ParserState *ps, stmt_t *stmt)
{
  ParserUnit *u = ps->u;
  char *id = stmt->var.id;
  TypeDesc *desc = stmt->var.desc;
  expr_t *rexp = stmt->var.exp;
  int konst = stmt->var.konst;
  TypeDesc *rrdesc;

  Log_Debug("parse variable '%s'", id);

  // check variable's type is valid
  if (desc) {
    // check array's base type
    TypeDesc *d = desc;
    while (d->kind == TYPE_ARRAY) d = d->array.base;
    if (d->kind == TYPE_KLASS) {
      if (!find_userdef_symbol(ps, d)) {
        PSError("undefined '%s'", d->klass.type.str);
        return;
      }
    }
  }

  // visit its right expr if necessary
  if (rexp) {
    rexp->ctx = EXPR_LOAD;
    parser_visit_expr(ps, rexp);
    expr_t *rrexp = expr_get_rrexp(rexp);
    if (!rrexp->desc) {
      PSError("cannot resolve expr's type");
      return;
    }

    rrdesc = rrexp->desc;
    if (rrdesc->kind == TYPE_PROTO) {
      int nret = Vector_Size(rrdesc->proto.ret);
      if ( nret != 1) {
        PSError("assignment mismatch: 1 variables but %d values", nret);
        return;
      }
      rrdesc = Vector_Get(rrdesc->proto.ret, 0);
    }

    if (!desc) {
      desc = TypeDesc_Dup(rrdesc);
      stmt->var.desc = desc;
    } else {
      if (!TypeDesc_Equal(desc, rrdesc)) {
        PSError("typecheck failed");
        return;
      }
    }
  }

  // here variable's type must be valid
  assert(desc);

  Symbol *sym;
  if (u->scope == SCOPE_MODULE || u->scope == SCOPE_CLASS) {
    // module's or class's variable
    Log_Debug("variable '%s' in module or class", id);
    sym = STable_Get(u->stbl, id);
    assert(sym);
    if (sym->kind == SYM_VAR && !sym->desc) {
      STable_Update_Symbol(u->stbl, sym, desc);
#if LOG_DEBUG
      char buf[64];
      Type_ToString(desc, buf);
      Log_Debug("update symbol '%s' type -> '%s'", id, buf);
#endif
    }
    // generate code
    if (rexp) {
      Inst_Append_NoArg(ps->u->block, OP_LOAD0);
      Argument val = {.kind = ARG_STR, .str = id};
      Inst_Append(ps->u->block, OP_SETFIELD, &val);
    }
  } else if (u->scope == SCOPE_FUNCTION || u->scope == SCOPE_METHOD) {
    /* local variable */
    Log_Debug("variable '%s' in function", id);
    sym = STable_Add_Var(u->stbl, id, desc, konst);
    sym->up = u->sym;
    Vector_Append(&u->sym->locvec, sym);
    /* generate code */
    if (rexp) {
      Argument val = {.kind = ARG_INT, .ival = sym->index};
      Inst_Append(ps->u->block, OP_STORE, &val);
    }
  } else if (u->scope == SCOPE_BLOCK) {
    /* local's variable in block */
    Log_Debug("variable '%s' in block", id);
    //FIXME: up -> up -> up
    ParserUnit *pu = get_function_unit(ps);
    assert(pu);
    sym = STable_Get(pu->stbl, id);
    if (sym) {
      Log_Error("variable '%s' is already defined", id);
      return;
    }
    //FIXME: need add to function's symbol table? how to handle different block has the same variable
    sym = STable_Add_Var(u->stbl, id, desc, konst);
    assert(sym);
    sym->index = pu->stbl->varcnt++;  //FIXME: why index?
    sym->up = NULL;
    Vector_Append(&pu->sym->locvec, sym);
    /* generate code */
    if (rexp) {
      Argument val = {.kind = ARG_INT, .ival = sym->index};
      Inst_Append(ps->u->block, OP_STORE, &val);
    }
  } else {
    assert(0);
  }

#if 0
  if (var->typealias) {
    Log_Debug("update typealias symbol '%s' desc", sym->name);
    sym->desc = var->desc;
  }
#endif
}

static void parse_varlist(ParserState *ps, stmt_t *stmt)
{
  TypeDesc *desc = stmt->vars.desc;

  // check variable's type is valid
  if (desc) {
    // check array's base type
    if (desc->kind == TYPE_ARRAY) desc = desc->array.base;
    if (desc->kind == TYPE_KLASS) {
      if (!find_userdef_symbol(ps, desc)) {
        PSError("undefined '%s'", desc->klass.type.str);
        return;
      }
    }
  }

  // visit its right expr
  expr_t *rexp = stmt->vars.exp;
  rexp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, rexp);
  expr_t *rrexp = expr_get_rrexp(rexp);
  if (!rrexp->desc) {
    PSError("cannot resolve expr's type");
    return;
  }

  TypeDesc *rrdesc = rrexp->desc;
  if (rrdesc->kind != TYPE_PROTO) {
    PSError("missing expression in var declaration");
    return;
  }

  int nid = Vector_Size(stmt->vars.ids);
  int nret = Vector_Size(rrdesc->proto.ret);
  if (nid != nret) {
    PSError("assignment mismatch: %d variables but %d values", nid, nret);
    return;
  }

  if (desc) {
    TypeDesc *item;
    Vector_ForEach(item, rrdesc->proto.ret) {
      if (!TypeDesc_Equal(desc, item)) {
        PSError("typecheck failed");
        return;
      }
    }
  }

  Symbol *sym;
  ParserUnit *u = ps->u;
  int konst = stmt->vars.konst;
  char *id;
  if (u->scope == SCOPE_MODULE || u->scope == SCOPE_CLASS) {
    // module's or class's variable
    Vector_ForEach(id, stmt->vars.ids) {
      Log_Debug("varlist '%s' in module or class", id);
    }

    sym = STable_Get(u->stbl, id);
    assert(sym);
    if (sym->kind == SYM_VAR && !sym->desc) {
      STable_Update_Symbol(u->stbl, sym, desc);
#if LOG_DEBUG
      char buf[64];
      Type_ToString(desc, buf);
      Log_Debug("update symbol '%s' type -> '%s'", id, buf);
#endif
    }
    // generate code
    if (rexp) {
      Vector_ForEach_Reverse(id, stmt->vars.ids) {
        Inst_Append_NoArg(ps->u->block, OP_LOAD0);
        Argument val = {.kind = ARG_STR, .str = id};
        Inst_Append(ps->u->block, OP_SETFIELD, &val);
      }
    }
  } else if (u->scope == SCOPE_FUNCTION || u->scope == SCOPE_METHOD) {
    /* local variable */
    Vector_ForEach_Reverse(id, stmt->vars.ids) {
      Log_Debug("variable '%s' in function", id);
      if (!desc) desc = Vector_Get(rrdesc->proto.ret, i);
      sym = STable_Add_Var(u->stbl, id, desc, konst);
      sym->up = u->sym;
      Vector_Append(&u->sym->locvec, sym);
      /* generate code */
      if (rexp) {
        Argument val = {.kind = ARG_INT, .ival = sym->index};
        Inst_Append(ps->u->block, OP_STORE, &val);
      }
    }
  } else if (u->scope == SCOPE_BLOCK) {
    /* local's variable in block */
    Vector_ForEach(id, stmt->vars.ids) {
      Log_Debug("variable '%s' in block", id);
      //FIXME: up -> up -> up
      ParserUnit *pu = get_function_unit(ps);
      assert(pu);
      sym = STable_Get(pu->stbl, id);
      if (sym) {
        Log_Error("variable '%s' is already defined", id);
        return;
      }
      //FIXME: need add to function's symbol table? how to handle different block has the same variable
      sym = STable_Add_Var(u->stbl, id, desc, konst);
      assert(sym);
      sym->index = pu->stbl->varcnt++;  //FIXME: why index?
      sym->up = NULL;
      Vector_Append(&pu->sym->locvec, sym);
      /* generate code */
      if (rexp) {
        Argument val = {.kind = ARG_INT, .ival = sym->index};
        Inst_Append(ps->u->block, OP_STORE, &val);
      }
    }
  } else {
    assert(0);
  }
}

static void parser_init_function(ParserState *ps, Vector *stmts)
{
  Log_Debug("------parser_init_function-------");
  if (stmts) {
    stmt_t *s;
    Vector_ForEach(s, stmts) {
      #if 0
    FIXME:
      if (i == 0) {
        struct expr *exp = s->exp;
        if (exp->kind != SUPER_KIND) {
          ParserUnit *cu = get_class_unit(ps);
          if (cu->sym->kind == SYM_CLASS) {
            Log_Debug("class hanlde traits init");
            handle_traits(ps, cu->sym);
          } else {
            assert(cu->sym->kind == SYM_TRAIT);
            Log_Debug("trait needs not handle traits init");
          }
        }
      }
      #endif
      parser_vist_stmt(ps, s);
    }
  }
  Log_Debug("------parser_init_function end---");
}

static void parser_function(ParserState *ps, stmt_t *stmt, int scope)
{
  parser_enter_scope(ps, NULL, scope);

  ParserUnit *parent = parent_scope(ps);
  Symbol *sym = STable_Get(parent->stbl, stmt->func.id);
  assert(sym);
  ps->u->sym = sym;

  if (parent->scope == SCOPE_MODULE || parent->scope == SCOPE_CLASS) {
    Log_Debug("------parse function '%s'--------", stmt->func.id);
    if (stmt->func.args) {
      stmt_t *s;
      Vector_ForEach(s, stmt->func.args) {
        parse_variable(ps, s);
      }
    }
    if (!strcmp(stmt->func.id, "__init__"))
      parser_init_function(ps, stmt->func.body);
    else
      parser_body(ps, stmt->func.body);
    Log_Debug("------parse function '%s' end----", stmt->func.id);
  } else {
    assert(0);
  }

  parser_exit_scope(ps);
}

void sym_inherit_fn(Symbol *sym, void *arg)
{
  Symbol *subsym = arg;
  Symbol *supersym = subsym->super;
  STable *stbl = subsym->ptr;
  Symbol *s;

  char namebuf[strlen(supersym->name) + 1 + strlen(sym->name) + 1];
  sprintf(namebuf, "%s.%s", supersym->name, sym->name);

  if (STable_Get(stbl, sym->name)) {
    Log_Debug("symbol '%s' exist in subclass '%s'", sym->name, subsym->name);
  }

  if (sym->kind == SYM_VAR) {
    Log_Debug("inherit variable '%s' from super class", namebuf);
    s = STable_Add_Var(stbl, namebuf, sym->desc, sym->konst);
    if (s) {
      s->up = subsym;
      s->inherited = 1;
      s->super = sym;
    }
  } else if (sym->kind == SYM_PROTO) {
    Log_Debug("inherit function '%s' from super class", namebuf);
    s = STable_Add_Proto(stbl, namebuf, sym->desc);
    if (s) {
      s->up = subsym;
      s->inherited = 1;
      s->super = sym;
    }
  } else {
    assert(0);
  }
}

static int trait_in_super(Symbol *trait, Symbol *sym)
{
  if (!sym) return 0;

  Symbol *t;
  Vector_ForEach_Reverse(t, &sym->traits) {
    if (t == trait) return 1;
  }

  return trait_in_super(trait, sym->super);
}

static void line_trait(Symbol *trait, Symbol *to)
{
  assert(trait->kind == SYM_TRAIT);
  Symbol *s;
  Vector_ForEach(s, &to->traits) {
    if (s == trait) {
      Log_Debug("trait '%s' is already in liner order", trait->name);
      return;
    }
  }

  if (trait_in_super(trait, to->super)) {
    Log_Debug("trait '%s' is already in super class '%s'",
      trait->name, to->super->name);
    return;
  }

  Log_Debug("add trait '%s' to liner order", trait->name);
  Vector_Append(&to->traits, trait);
}

static void parser_order(Vector *vec, Symbol *to)
{
  Log_Debug("------parse order -------");
  Symbol *s;
  Vector_ForEach(s, vec) {
    parser_order(&s->traits, to);
    line_trait(s, to);
  }

  Log_Debug("------parse order end-------");
}

struct check_trait_s {
  int index;
  Symbol *sym;
};

static void __check_trait_fn(Symbol *sym, void *arg)
{
  if (sym->kind != SYM_IPROTO) return;

  struct check_trait_s *temp = arg;
  Symbol *s;
  if (temp->sym->super) {
    s = STable_Get(temp->sym->super->ptr, sym->name);
    if (s) {
      Log_Debug("'%s' has impl in super class '%s'",
        sym->name, temp->sym->super->name);
      return;
    }
  }

  int size = Vector_Size(&temp->sym->traits);
  Symbol *trait;
  for (int i = temp->index; i < size; i++) {
    trait = Vector_Get(&temp->sym->traits, i);
    s = STable_Get(trait->ptr, sym->name);
    if (!s) {
      Log_Debug("not found '%s' in trait '%s'", sym->name, trait->name);
      continue;
    }
    if (s->kind != SYM_PROTO) {
      Log_Debug("'%s' in trait '%s' is not a func", sym->name, trait->name);
      continue;
    }
    //check function's proto
    Log_Debug("found '%s' in trait '%s'", sym->name, trait->name);
    return;
  }

  s = STable_Get(temp->sym->ptr, sym->name);
  if (!s) {
    Log_Error("'%s' has not impl in line-order in class '%s'",
      sym->name, temp->sym->name);
    return;
  }
  if (s->kind != SYM_PROTO) {
    Log_Error("'%s' in class '%s' is not a func", sym->name, temp->sym->name);
    return;
  }
  //check function's proto
  Log_Debug("'%s' has impl in line-order in class '%s'",
    sym->name,temp->sym->name);
}

// flat extends' traits
static int parser_traits(ParserState *ps, Vector *traits, Symbol *sym)
{
  Log_Debug("------parse traits -------");

  Vector syms;
  Vector_Init(&syms);
  TypeDesc *desc;
  Symbol *s;
  char buf[64];
  Vector_ForEach(desc, traits) {
    TypeDesc_ToString(desc, buf);
    Log_Debug("trait '%s'", buf);
    s = find_userdef_symbol(ps, desc);
    if (!s) {
      Log_Error("cannot find trait '%s'", buf);
      return -1;
    }
    if (s->kind != SYM_TRAIT) {
      Log_Error("symbol '%s' is a trait", buf);
      return -1;
    }
    assert(s->ptr);
    Vector_Append(&syms, s);
  }

  parser_order(&syms, sym);

  Vector_Fini(&syms, NULL, NULL);

  //check traits abstract funcs whether have impl in class
  if (sym->kind == SYM_CLASS) {
    struct check_trait_s temp = {.sym = sym};
    Vector_ForEach(s, &sym->traits) {
      if (s->kind == SYM_TRAIT) {
        Log_Debug(">>>>check abstract functions in trait '%s'<<<<", s->name);
        temp.index = i + 1;
        STable_Traverse(s->ptr, __check_trait_fn, &temp);
      }
    }
  }

  Log_Debug("------parse traits end -------");

  return 0;
}

static void parser_class(ParserState *ps, stmt_t *stmt)
{
  Log_Debug("------parse class(trait) -------");
  Log_Debug("%s:%s", stmt->kind == CLASS_KIND ? "class" : "trait",
    stmt->class_info.id);

  ParserUnit *u = ps->u;
  Symbol *sym = STable_Get(u->stbl, stmt->class_info.id);
  assert(sym && sym->ptr);

  TypeDesc *desc = stmt->class_info.super;
  if (desc) {
    char buf[64];
    TypeDesc_ToString(desc, buf);
    Log_Debug("class's super '%s'", buf);
    Symbol *super = find_userdef_symbol(ps, desc);
    if (!super) {
      Log_Error("cannot find super '%s'", buf);
      return;
    }
    if (super->kind != SYM_CLASS && super->kind != SYM_STABLE) {
      Log_Error("super '%s' is not a class", buf);
      return;
    }
    assert(super->ptr);
    sym->super = super;
    // Vector_Append(&sym->extends, super);
  }

  if (stmt->class_info.traits) {
    if (parser_traits(ps, stmt->class_info.traits, sym))
      return;
  }

  parser_enter_scope(ps, sym->ptr, SCOPE_CLASS);

  ps->u->sym = sym;

  if (stmt->class_info.body) {
    stmt_t *s;
    Vector_ForEach(s, stmt->class_info.body) {
      if (s->kind == VAR_KIND) {
        //parse_variable(ps, s->vardecl.var, s->vardecl.exp);
      } else if (s->kind == FUNC_KIND) {
        parser_function(ps, s, SCOPE_METHOD);
      } else if (s->kind == PROTO_KIND) {
        Log_Debug("func %s is abstract", s->proto.id);
      } else {
        assert(0);
      }
    }
  }

  parser_exit_scope(ps);

  printf("++++++++++linear order++++++++++\n");
  printf("%s ", sym->name);
  Symbol *sb;
  Vector_ForEach_Reverse(sb, &sym->traits) {
    printf("-> %s ", sb->name);
  }
  if (sym->super) printf("-> %s", sym->super->name);
  printf("\n++++++++++++++++++++++++++++++\n");
  Log_Debug("------parse class(trait) end---");
}

static void parser_typealias(ParserState *ps, stmt_t *stmt)
{
  Log_Debug("------parse typealias -------");
  ParserUnit *u = ps->u;
  Symbol *sym = STable_Get(u->stbl, stmt->typealias.id);
  assert(sym);
  Log_Debug("------parse typealias end----");
}

// static void __inherit_imethod_fn(Symbol *sym, void *arg)
// {
// 	struct intf_inherit_struct *temp = arg;
// 	if (temp->result) return;
// 	Symbol *intf = temp->sym;
// 	assert(sym->kind == SYM_IPROTO);
// 	assert(intf->kind == SYM_INTF);
// 	Symbol *imeth = STable_Add_IProto(intf->ptr, sym->name, sym->desc->proto);
// 	if (!imeth)
// 		Log_Debug("The same imethod '%s' in '%s.'", sym->name, intf->name);
// 	else
// 		Log_Debug("Inherit imethod '%s' to '%s' successfully.", sym->name, intf->name);
// 	temp->result = 0;
// }

// static int inherit_imethods(Symbol *sym, Symbol *basesym)
// {
// 	struct intf_inherit_struct temp = {sym, 0};
// 	STable_Traverse(basesym->ptr, __inherit_imethod_fn, &temp);
// 	return temp.result;
// }

// static void __inherit_intf_fn(HashNode *hnode, void *arg)
// {
// 	Symbol *sym = arg;
// 	BaseSymbol *basesym = container_of(hnode, BaseSymbol, hnode);
// 	if (Symbol_Add_Base(sym, basesym->sym)) {
// 		Log_Debug("The same intf '%s' in '%s'.", basesym->sym->name, sym->name);
// 	} else {
// 		Log_Debug("Inherit intf '%s' to '%s' successfully.",
// 			basesym->sym->name, sym->name);
// 	}
// }

// static int inherit_interfaces(Symbol *sym, Symbol *basesym)
// {
// 	assert(sym->kind == SYM_INTF);
// 	assert(basesym->kind == SYM_INTF);
// 	Symbol_Add_Base(sym, basesym);
// 	HashTable_Traverse(basesym->table, __inherit_intf_fn, sym);
// 	return 0;
// }

// static void parser_interface(ParserState *ps, stmt_t *stmt)
// {
// 	Log_Debug("------parse interface-------");
// 	Log_Debug("interface:%s", stmt->intf_type.id);

// 	ParserUnit *u = ps->u;
// 	Symbol *sym = STable_Get(u->stbl, stmt->intf_type.id);
// 	assert(sym && sym->ptr);

// 	if (!stmt->intf_type.bases) {
// 		Log_Debug("------parse interface end---");
// 		return;
// 	}

// 	TypeDesc *desc;
// 	Symbol *basesym;
// 	char *typestr;
// 	Vector_ForEach(desc, stmt->intf_type.bases) {
// 		typestr = TypeDesc_ToString(desc);
// 		Log_Debug("interface with base '%s'", typestr);
// 		basesym = find_userdef_symbol(ps, desc);
// 		if (!basesym) {
// 			Log_Error("cannot find base interface '%s'", typestr);
// 			free(typestr);
// 			return;
// 		}

// 		if (basesym->kind != SYM_INTF) {
// 			Log_Error("base '%s' is not an interface", typestr);
// 			free(typestr);
// 			return;
// 		}

// 		free(typestr);

// 		inherit_imethods(sym, basesym);
// 		inherit_interfaces(sym, basesym);
// 	}

// 	Log_Debug("------parse interface end---");
// }

static void parse_assign(ParserState *ps, stmt_t *stmt)
{
  struct expr *r = stmt->assign.right;
  struct expr *l = stmt->assign.left;
  r->ctx = EXPR_LOAD;
  parser_visit_expr(ps, r);
  l->ctx = EXPR_STORE;
  parser_visit_expr(ps, l);

  expr_t *rrexp = expr_get_rrexp(r);
  TypeDesc *rrdesc = rrexp->desc;
  if (rrdesc->kind == TYPE_PROTO) {
    int nret = Vector_Size(rrdesc->proto.ret);
    if (nret != 1) {
      PSError("assignment mismatch: 1 variables but %d values", nret);
      return;
    }
    rrdesc = Vector_Get(rrdesc->proto.ret, 0);
  }
  if (!TypeDesc_Equal(l->desc, rrdesc)) {
    PSError("typecheck failed");
  }
}

static void parse_assignlist(ParserState *ps, stmt_t *stmt)
{
  // check count of vars and func return values
  int nid = Vector_Size(stmt->assigns.left);
  expr_t *exp = stmt->assigns.right;
  exp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, exp);
  if (!exp->desc) {
    PSError("cannot parse expr's type");
    return;
  }

  expr_t *rrexp = expr_get_rrexp(exp);
  TypeDesc *rrdesc = rrexp->desc;
  if (rrdesc->kind != TYPE_PROTO) {
    PSError("missing expression in var declaration");
    return;
  }

  int nret = Vector_Size(rrdesc->proto.ret);
  if (nid != nret) {
    PSError("assignment mismatch: %d variables but %d values", nid, nret);
    return;
  }

  expr_t *e;
  Vector_ForEach_Reverse(e, stmt->assigns.left) {
    e->ctx = EXPR_STORE;
    parser_visit_expr(ps, e);
  }

  Vector_ForEach(e, stmt->assigns.left) {
    rrexp = expr_get_rrexp(e);
    if (!TypeDesc_Equal(rrexp->desc, Vector_Get(rrdesc->proto.ret, i))) {
      PSError("typecheck failed");
    }
  }
}

static void parser_return(ParserState *ps, stmt_t *stmt)
{
  ParserUnit *u = ps->u;
  Symbol *sym = NULL;
  if (u->scope == SCOPE_FUNCTION || u->scope == SCOPE_METHOD) {
    Log_Debug("return in function");
    sym = u->sym;
  } else if (u->scope == SCOPE_BLOCK) {
    Log_Debug("return in block");
    ParserUnit *parent = get_function_unit(ps);
    assert(parent);
    sym = parent->sym;
  } else {
    assert(0);
  }

  struct expr *e;
  Vector_ForEach(e, stmt->returns) {
    e->ctx = EXPR_LOAD;
    parser_visit_expr(ps, e);
  }
  check_return_types(sym, stmt->returns);

  Inst_Append(u->block, OP_RET, NULL);
  u->block->bret = 1;
}

static void parser_if(ParserState *ps, stmt_t *stmt)
{
  parser_enter_scope(ps, NULL, SCOPE_BLOCK);
  int testsize = 0;
  struct expr *test = stmt->if_stmt.test;
  if (test) {
    // ELSE branch has not 'test'
    parser_visit_expr(ps, test);
    assert(test->desc);
    //FXIME
    /*
    if (test->desc != &Bool_Type) {
      Log_Error("if-stmt condition is not bool");
      return;
    }
    */
    testsize = ps->u->block->bytes;
  }

  CodeBlock *b = ps->u->block;
  Inst *jumpfalse = NULL;
  if (test) {
    jumpfalse = Inst_Append(b, OP_JUMP_FALSE, NULL);
  }

  parser_body(ps, stmt->if_stmt.body);

  if (test) {
    // ELSE branch has not 'test'
    int offset = b->bytes;
    Log_Debug("offset:%d", offset);
    if (stmt->if_stmt.orelse) {
      offset -= testsize;
      assert(offset >= 0);
      Log_Debug("offset2:%d", offset);
    } else {
      offset -= testsize + 1 + OpCode_ArgCount(OP_JUMP_FALSE);
      Log_Debug("offset3:%d", offset);
      assert(offset >= 0);
    }
    jumpfalse->arg.kind = ARG_INT;
    jumpfalse->arg.ival = offset;
  }

  if (stmt->if_stmt.orelse) {
    parser_if(ps, stmt->if_stmt.orelse);
    assert(b->next);
    CodeBlock *nb = b->next;
    int offset = 0;
    while (nb) {
      offset += nb->bytes;
      nb = nb->next;
    }
    Argument val = {.kind = ARG_INT, .ival = offset};
    Inst_Append(b, OP_JUMP, &val);
  }

  if (stmt->if_stmt.belse)
    ps->u->merge = 0;
  else
    ps->u->merge = 1;

  parser_exit_scope(ps);
}

static void parser_while(ParserState *ps, stmt_t *stmt)
{
  parser_enter_scope(ps, NULL, SCOPE_BLOCK);
  ParserUnit *u = ps->u;
  u->loop = 1;
  CodeBlock *b = u->block;
  Inst *jmp;
  int jmpsize = 0;
  int bodysize = 0;

  if (stmt->while_stmt.btest) {
    jmp = Inst_Append(u->block, OP_JUMP, NULL);
    jmpsize = 1 + OpCode_ArgCount(OP_JUMP);
  }

  parser_body(ps, stmt->while_stmt.body);
  bodysize = u->block->bytes - jmpsize;

  struct expr *test = stmt->while_stmt.test;
  parser_visit_expr(ps, test);
  assert(test->desc);
  //FIXME
  /*
  if (test->desc != &Bool_Type) {
    Log_Error("while-stmt condition is not bool");
  }
  */
  int offset = 0 - (u->block->bytes - jmpsize +
    1 + OpCode_ArgCount(OP_JUMP_TRUE));
  Argument val = {.kind = ARG_INT, .ival = offset};
  Inst_Append(b, OP_JUMP_TRUE, &val);

  if (stmt->while_stmt.btest) {
    jmp->arg.kind = ARG_INT;
    jmp->arg.ival = bodysize;
  }

  parser_exit_scope(ps);
}

static void parser_break(ParserState *ps, stmt_t *stmt)
{
  UNUSED_PARAMETER(stmt);

  ParserUnit *u = ps->u;
  if (u->scope != SCOPE_BLOCK) {
    Log_Error("break statement not within a loop or switch");
    return;
  }

  CodeBlock *b = u->block;
  // current block is a loop
  if (u->loop) {
    Inst *i = Inst_Append(b, OP_JUMP, NULL);
    JmpInst *jmp = JmpInst_New(i, JMP_BREAK);
    Vector_Append(&u->jmps, jmp);
  } else {
    ParserUnit *parent;
    struct list_head *pos;
    list_for_each(pos, &ps->ustack) {
      parent = container_of(pos, ParserUnit, link);
      if (parent->scope != SCOPE_BLOCK) {
        Log_Error("break statement is not within a block");
        break;
      }

      if (parent->loop) {
        Inst *i = Inst_Append(b, OP_JUMP, NULL);
        //FIXME:
        JmpInst *jmp = JmpInst_New(i, JMP_BREAK);
        Vector_Append(&parent->jmps, jmp);
        return;
      }
    }

    Log_Error("break statement not within a loop or switch");
  }
}

static void parser_continue(ParserState *ps, stmt_t *stmt)
{
  UNUSED_PARAMETER(stmt);

  ParserUnit *u = ps->u;
  if (u->scope != SCOPE_BLOCK) {
    Log_Error("continue statement not within a loop");
    return;
  }

  CodeBlock *b = u->block;
  // current block is a loop
  if (u->loop) {
    Inst *i = Inst_Append(b, OP_JUMP, NULL);
    JmpInst *jmp = JmpInst_New(i, JMP_CONTINUE);
    Vector_Append(&u->jmps, jmp);
  } else {
    ParserUnit *parent;
    struct list_head *pos;
    list_for_each(pos, &ps->ustack) {
      parent = container_of(pos, ParserUnit, link);
      if (parent->scope != SCOPE_BLOCK) {
        Log_Error("break statement is not within a block");
        break;
      }

      if (parent->loop) {
        Inst *i = Inst_Append(b, OP_JUMP, NULL);
        //FIXME:
        JmpInst *jmp = JmpInst_New(i, JMP_CONTINUE);
        Vector_Append(&parent->jmps, jmp);
        return;
      }
    }

    Log_Error("continue statement not within a loop");
  }
}

static void parser_list(ParserState *ps, stmt_t *stmt)
{
  stmt_t *s;
  Vector_ForEach(s, &stmt->list) {
    parser_vist_stmt(ps, s);
  }
}

static void parser_vist_stmt(ParserState *ps, stmt_t *stmt)
{
  switch (stmt->kind) {
    case VAR_KIND: {
      parse_variable(ps, stmt);
      break;
    }
    case VARLIST_KIND: {
      parse_varlist(ps, stmt);
      break;
    }
    case FUNC_KIND: {
      parser_function(ps, stmt, SCOPE_FUNCTION);
      break;
    }
    case ASSIGN_KIND: {
      parse_assign(ps, stmt);
      break;
    }
    case ASSIGNS_KIND: {
      parse_assignlist(ps, stmt);
      break;
    }
    case TYPEALIAS_KIND: {
      parser_typealias(ps, stmt);
      break;
    }
    case CLASS_KIND:
    case TRAIT_KIND: {
      parser_class(ps, stmt);
      break;
    }
    case EXPR_KIND: {
      parser_visit_expr(ps, stmt->exp);
      break;
    }
    case RETURN_KIND: {
      parser_return(ps, stmt);
      break;
    }
    case IF_KIND: {
      parser_if(ps, stmt);
      break;
    }
    case WHILE_KIND: {
      parser_while(ps, stmt);
      break;
    }
    case BREAK_KIND: {
      parser_break(ps, stmt);
      break;
    }
    case CONTINUE_KIND: {
      parser_continue(ps, stmt);
      break;
    }
    case LIST_KIND: {
      parser_list(ps, stmt);
      break;
    }
    default: {
      assert(0);
      break;
    }
  }
}

void parser_body(ParserState *ps, Vector *stmts)
{
  if (!stmts) return;

  stmt_t *s;
  Vector_ForEach(s, stmts) {
    parser_vist_stmt(ps, s);
  }
}

/*----------------------------------------------------------------------------*/

PackageInfo *New_PackageInfo(char *pkgfile, struct options *ops)
{
  PackageInfo *pkg = calloc(1, sizeof(PackageInfo));
  pkg->pkgfile = pkgfile;
  pkg->sym = Symbol_New(SYM_MODULE);
  pkg->sym->ptr = STable_New();
  parser_init_unit(&pkg->top, pkg->sym->ptr, SCOPE_MODULE);
  init_extpkg_hashtable(&pkg->expkgs);
  pkg->options = ops;
  return pkg;
}

void Parser_Set_Package(ParserState *ps, char *pkgname)
{
  PackageInfo *pkg = ps->pkg;
  assert(pkg);
  if (!pkg->pkgname) {
    pkg->pkgname = pkgname;
  } else {
    if (strcmp(pkg->pkgname, pkgname)) {
      PSError("found existed packages %s and %s in %s", pkg->pkgname, pkgname,
      ps->filename);
    }
  }
}

ParserState *new_parser(PackageInfo *pkg, char *filename)
{
  ParserState *ps = calloc(1, sizeof(ParserState));
  yylex_init_extra(ps, &ps->scanner);
  ps->filename = filename;
  ps->pkg = pkg;
  ps->sym = pkg->sym;
  Vector_Init(&ps->stmts);
  init_list_head(&ps->ustack);
  Vector_Init(&ps->errors);
  ps->u = &pkg->top;
  ps->nestlevel++;
  parser_new_block(ps->u);
  ps->u->sym = ps->sym;
  init_imports(ps);
  return ps;
}

void parse_module_scope(PackageInfo *pkg)
{
  ParserUnit *u = &pkg->top;
  assert(u->scope == SCOPE_MODULE);
  Symbol *sym = STable_Get(u->stbl, "__init__");
  if (sym == NULL) {
    TypeDesc *proto = TypeDesc_New_Proto(NULL, NULL);
    sym = STable_Add_Proto(u->stbl, "__init__", proto);
  }
  assert(sym);
  Log_Debug("add 'return' to function '__init__'");
  Inst_Append(u->block, OP_RET, NULL);
  if (u->block && u->block->bytes > 0) {
    sym->ptr = u->block;
    //sym->stbl = u->stbl;
    sym->locvars = u->stbl->varcnt;
    //u->stbl = NULL;
  }
  u->sym = sym;
  u->block = NULL;
  Log_Debug("save code to module '__init__' function");
}

void destroy_parser(ParserState *ps)
{
  parser_show_scope(ps);
  check_unused_symbols(ps);
  check_unused_imports(ps);
  assert(ps->u == &ps->pkg->top);
  vec_stmt_fini(&ps->stmts);
  fini_imports(ps);
  yylex_destroy(ps->scanner);
  free(ps);
}
