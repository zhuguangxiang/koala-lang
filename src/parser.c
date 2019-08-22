/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "parser.h"
#include "opcode.h"
#include "moduleobject.h"

/* lang module */
static Module _lang_;
/* ModuleObject */
static Symbol *modClsSym;
/* Module, path as key */
static HashMap modules;

void init_parser(void)
{
  _lang_.path = "lang";
  Object *ob = Module_Load(_lang_.path);
  if (ob == NULL)
    panic("cannot find 'lang' module");
  mod_from_mobject(&_lang_, ob);
  OB_DECREF(ob);
  modClsSym = find_from_builtins("Module");
  if (modClsSym == NULL)
    panic("cannot find type 'Module'");
}

void fini_parser(void)
{
  stable_free(_lang_.stbl);
}

Symbol *find_from_builtins(char *name)
{
  return stable_get(_lang_.stbl, name);
}

void mod_from_mobject(Module *mod, Object *ob)
{
  ModuleObject *mo = (ModuleObject *)ob;
  mod->name = mo->name;
  mod->stbl = stable_from_mobject(ob);
}

Symbol *mod_find_symbol(Module *mod, char *name)
{
  Symbol *sym = stable_get(mod->stbl, name);
  if (sym != NULL)
    return sym;
  return klass_find_member(modClsSym, name);
}

typedef struct inst {
  struct list_head link;
  int bytes;
  int argc;
  uint8_t op;
  ConstValue arg;
  /* break and continue statements */
  int upbytes;
} Inst;

#define JMP_BREAK    1
#define JMP_CONTINUE 2

typedef struct jmp_inst {
  int type;
  Inst *inst;
} JmpInst;

static inline CodeBlock *codeblock_new(void);

static inline void codeblock_add_inst(CodeBlock *b, Inst *i)
{
  list_add_tail(&i->link, &b->insts);
  b->bytes += i->bytes;
  i->upbytes = b->bytes;
}

static Inst *inst_add(ParserState *ps, uint8_t op, ConstValue *val)
{
  Inst *i = kmalloc(sizeof(Inst));
  init_list_head(&i->link);
  i->op = op;
  i->bytes = 1 + opcode_argc(op);
  if (val)
    i->arg = *val;

  ParserUnit *u = ps->u;
  if (u->block == NULL)
    u->block = codeblock_new();
  codeblock_add_inst(u->block, i);
  return i;
}

static inline Inst *inst_add_noarg(ParserState *ps, uint8_t op)
{
  return inst_add(ps, op, NULL);
}

static inline CodeBlock *codeblock_new(void)
{
  CodeBlock *b = kmalloc(sizeof(CodeBlock));
  init_list_head(&b->insts);
  return b;
}

void codeblock_free(CodeBlock *block)
{
  if (block == NULL)
    return;

  struct list_head *p, *n;
  list_for_each_safe(p, n, &block->insts) {
    list_del(p);
    kfree(p);
  }

  kfree(block);
}

static void codeblock_merge(CodeBlock *from, CodeBlock *to)
{
  Inst *i;
  struct list_head *p, *n;
  list_for_each_safe(p, n, &from->insts) {
    i = (Inst *)p;
    list_del(p);
    from->bytes -= i->bytes;
    codeblock_add_inst(to, i);
  }
  if (from->bytes != 0)
    panic("block has %d bytes left", from->bytes);

  CodeBlock *b = from->next;
  while (b) {
    list_for_each_safe(p, n, &b->insts) {
      i = (Inst *)p;
      list_del(p);
      b->bytes -= i->bytes;
      codeblock_add_inst(to, i);
    }
    if (b->bytes != 0)
      panic("block has %d bytes left", b->bytes);
    b = b->next;
  }
}

void codeblock_show(CodeBlock *block)
{
  if (block == NULL || block->bytes <= 0)
    return;

  puts("---------------------------------------------");
  if (!list_empty(&block->insts)) {
    Inst *i;
    STRBUF(sbuf);
    char *opname;
    int offset = 0;
    struct list_head *p;
    list_for_each(p, &block->insts) {
      i = (Inst *)p;
      opname = opcode_str(i->op);
      print("%6d", offset);
      print("%6d", i->bytes);
      offset += i->bytes;
      print("  %-16s", opname);
      constvalue_show(&i->arg, &sbuf);
      print("%.64s\n", strbuf_tostr(&sbuf));
      strbuf_fini(&sbuf);
    }
  }
}

static void inst_gen(Inst *i, Image *image, ByteBuffer *buf)
{
  int index = -1;
  bytebuffer_write_byte(buf, i->op);
  switch (i->op) {
  case OP_LOAD_CONST: {
    ConstValue *val = &i->arg;
    index = Image_Add_ConstValue(image, val);
    bytebuffer_write_2bytes(buf, index);
    break;
  }
  case OP_LOAD_MODULE:
    index = Image_Add_String(image, i->arg.str);
    bytebuffer_write_2bytes(buf, index);
    break;
  case OP_GET_VALUE:
    index = Image_Add_String(image, i->arg.str);
    bytebuffer_write_2bytes(buf, index);
    break;
  case OP_SET_VALUE:
    index = Image_Add_String(image, i->arg.str);
    bytebuffer_write_2bytes(buf, index);
    break;
  case OP_CALL:
  case OP_NEW_EVAL:
    index = Image_Add_String(image, i->arg.str);
    bytebuffer_write_2bytes(buf, index);
    bytebuffer_write_byte(buf, i->argc);
    break;
  case OP_NEW_OBJECT:
    index = Image_Add_String(image, i->arg.str);
    bytebuffer_write_2bytes(buf, index);
    break;
  case OP_PRINT:
  case OP_DUP:
  case OP_LOAD_0:
  case OP_LOAD_1:
  case OP_LOAD_2:
  case OP_LOAD_3:
  case OP_STORE_0:
  case OP_STORE_1:
  case OP_STORE_2:
  case OP_STORE_3:
  case OP_RETURN_VALUE:
  case OP_ADD:
  case OP_SUB:
  case OP_MUL:
  case OP_DIV:
  case OP_MOD:
  case OP_GT:
  case OP_GE:
  case OP_LT:
  case OP_LE:
  case OP_EQ:
  case OP_NEQ:
  case OP_NEG:
  case OP_SUBSCR_LOAD:
  case OP_SUBSCR_STORE:
    break;
  case OP_LOAD:
  case OP_STORE:
    bytebuffer_write_byte(buf, i->arg.ival);
    break;
  case OP_JMP:
  case OP_JMP_TRUE:
  case OP_JMP_FALSE:
  case OP_NEW_TUPLE:
  case OP_NEW_ARRAY:
  case OP_NEW_MAP:
    bytebuffer_write_2bytes(buf, i->arg.ival);
    break;
  default:
    panic("invalid opcode %s", opcode_str(i->op));
    break;
  }
}

void code_gen(CodeBlock *block, Image *image, ByteBuffer *buf)
{
  if (block == NULL)
    return;
  struct list_head *p;
  list_for_each(p, &block->insts) {
    inst_gen((Inst *)p, image, buf);
  }
}

#define CODE_OP(op) ({    \
  inst_add_noarg(ps, op); \
})

#define CODE_OP_V(op, v) ({ \
  inst_add(ps, op, &v);     \
})

#define CODE_OP_I(op, i) ({ \
  ConstValue v;             \
  v.kind = BASE_INT;        \
  v.ival = i;               \
  inst_add(ps, op, &v);     \
})

#define CODE_OP_S(op, s) ({ \
  ConstValue v;             \
  v.kind = BASE_STR;        \
  v.str = s;                \
  inst_add(ps, op, &v);     \
})

#define CODE_OP_S_ARGC(op, s, _argc) ({ \
  Inst *i = CODE_OP_S(op, s);           \
  i->argc = _argc;                      \
})

static const char *scopes[] = {
  NULL, "MODULE", "CLASS", "FUNCTION", "BLOCK", "CLOSURE"
};

ParserState *New_Parser(Module *mod, char *filename)
{
  ParserState *ps = kmalloc(sizeof(ParserState));
  ps->filename = filename;
  ps->module = mod;
  vector_init(&ps->ustack);
  return ps;
}

void Free_Parser(ParserState *ps)
{
  if (ps->u != NULL)
    panic("ps->u is not null");
  if (vector_size(&ps->ustack) > 0)
    panic("ps->ustack is not empty");
  kfree(ps);
}

static inline void unit_check(ParserUnit *u)
{
  if (u->block != NULL)
    panic("u->block is not null");
  if (vector_size(&u->jmps) > 0)
    panic("u->jmps is not empty");
}

#define unit_free(ps) \
({ \
  unit_check(ps->u); \
  vector_fini(&ps->u->jmps, NULL, NULL); \
  kfree(ps->u); \
  ps->u = NULL; \
})

static void unit_show(ParserState *ps)
{
  ParserUnit *u = ps->u;
  const char *scope = scopes[u->scope];
  char *name = u->sym != NULL ? u->sym->name : NULL;
  name = (name == NULL) ? ps->filename : name;
  puts("---------------------------------------------");
  print("scope-%d(%s, %s) symbols:\n", ps->depth, scope, name);
  codeblock_show(u->block);
  puts("---------------------------------------------");
}

static void merge_into_init(ParserUnit *u)
{
  Symbol *sym = stable_get(u->stbl, "__init__");
  if (sym == NULL) {
    debug("create __init__");
    TypeDesc *proto = desc_from_proto(NULL, NULL);
    sym = stable_add_func(u->stbl, "__init__", proto);
    TYPE_DECREF(proto);
    sym->func.code = u->block;
    u->block = NULL;
  } else {
    debug("merge into __init__");
    if (!sym->func.code) {
      sym->func.code = u->block;
    } else {
      codeblock_merge(u->block, sym->func.code);
      codeblock_free(u->block);
    }
    u->block = NULL;
  }
}

static void merge(ParserState *ps)
{
  ParserUnit *u = ps->u;

  switch (u->scope) {
  case SCOPE_MODULE: {
    /* module has codes for __init__ */
    if (u->block->bytes > 0) {
      merge_into_init(u);
    } else {
      codeblock_free(u->block);
      u->block = NULL;
    }
    break;
  }
  default:
    panic("invalid branch:%d", u->scope);
    break;
  }
}

void parser_enter_scope(ParserState *ps, ScopeKind scope)
{
  debug("Enter scope-%d(%s)", ps->depth + 1, scopes[scope]);
  ParserUnit *u = kmalloc(sizeof(ParserUnit));
	u->scope = scope;
	vector_init(&u->jmps);

  /* push old unit into stack */
  if (ps->u != NULL)
    vector_push_back(&ps->ustack, ps->u);
  ps->u = u;
  ps->depth++;
}

void parser_exit_scope(ParserState *ps)
{
  debug("Exit scope-%d(%s)", ps->depth, scopes[ps->u->scope]);
#if !defined(NDEBUG)
  unit_show(ps);
#endif
  merge(ps);
  unit_free(ps);
  ps->depth--;

  /* restore ps->u to top of ps->ustack */
  if (vector_size(&ps->ustack) > 0) {
    ps->u = vector_pop_back(&ps->ustack);
  }
}

static Symbol *find_id_symbol(ParserState *ps, char *name)
{
  Symbol *sym;
  ParserUnit *u = ps->u;

  /* find id from current scope */
  sym = stable_get(u->stbl, name);
  if (sym != NULL) {
    debug("find symbol '%s' in scope-%d(%s)",
          name, ps->depth, scopes[u->scope]);
    ++sym->used;
    return sym;
  }
  return NULL;
}

static Symbol *get_const_symbol(char kind)
{
  Symbol *sym;
  switch (kind) {
  case BASE_INT:
    sym = stable_get(_lang_.stbl, "Integer");
    break;
  case BASE_STR:
    sym = stable_get(_lang_.stbl, "String");
    break;
  case BASE_ANY:
    sym = stable_get(_lang_.stbl, "Any");
    break;
  case BASE_BOOL:
    sym = stable_get(_lang_.stbl, "Bool");
    break;
  case BASE_FLOAT:
    sym = stable_get(_lang_.stbl, "Float");
    break;
  case BASE_CHAR:
    sym = stable_get(_lang_.stbl, "Char");
    break;
  case BASE_BYTE:
    sym = stable_get(_lang_.stbl, "Byte");
    break;
  default:
    panic("invalid const %c", kind);
    break;
  }
  return sym;
}

static Symbol *get_desc_symbol(ParserState *ps, TypeDesc *desc)
{
  if (desc == NULL) {
    debug("no return");
    return NULL;
  }

  Symbol *sym;
  switch (desc->kind) {
  case TYPE_BASE:
    sym = get_const_symbol(desc->base.type);
    break;
  case TYPE_KLASS:
    if (!strcmp(desc->klass.path, "lang"))
      sym = find_from_builtins(desc->klass.type);
    else
      panic("not implemented");
    break;
  default:
    panic("invalid desc kind %d", desc->kind);
    break;
  }
  return sym;
}

static void parse_attr_expr(ParserState *ps, Expr *exp)
{
  Expr *lexp = exp->attr.lexp;
  Symbol *lsym = lexp->sym;
  Symbol *sym = NULL;
  STable *stbl = NULL;
  Ident *id = &exp->attr.id;
  TypeDesc *desc;

  switch (lsym->kind) {
  case SYM_CONST:
    break;
  case SYM_VAR:
    debug("left sym '%s' is a var", lsym->name);
    desc = lsym->desc;
    sym = get_desc_symbol(ps, desc);
    if (sym == NULL || sym->kind != SYM_CLASS) {
      syntax_error(ps, exp->row, exp->col,
                   "cannot resolve symbol '%s' type", lsym->name);
    } else {
      sym = klass_find_member(sym, id->name);
    }
    break;
  case SYM_FUNC:
    debug("left sym '%s' is a func", lsym->name);
    desc = lsym->desc;
    sym = get_desc_symbol(ps, desc->proto.ret);
    if (sym != NULL) {
      if (sym->kind != SYM_CLASS)
        panic("func's retval is not a class");
      sym = klass_find_member(sym, id->name);
    }
    break;
  case SYM_MOD: {
    debug("left sym '%s' is a module", lsym->name);
    Module *mod = lsym->mod.ptr;
    sym = mod_find_symbol(mod, id->name);
    break;
  }
  case SYM_CLASS: {
    debug("left sym '%s' is a class", lsym->name);
    sym = klass_find_member(lsym, id->name);
    break;
  }
  default:
    panic("invalid left sym %d", lsym->kind);
    break;
  }

  if (sym == NULL) {
    syntax_error(ps, id->row, id->col, "'%s' is not found in '%s'",
                 id->name, lsym->name);
    return;
  }

  exp->sym = sym;

  if (!exp->desc) {
    exp->desc = TYPE_INCREF(sym->desc);
  } else {
    warn("exp's desc is not null");
  }
}

static void code_attr_expr(ParserState *ps, Expr *exp)
{
  if (has_error(ps))
    return;

  Symbol *sym = exp->sym;
  Ident *id = &exp->attr.id;

  switch (sym->kind) {
  case SYM_VAR:
    CODE_OP_S(OP_GET_VALUE, id->name);
    break;
  case SYM_FUNC:
    if (exp->ctx == EXPR_LOAD)
      CODE_OP_S(OP_GET_VALUE, id->name);
    else if (exp->ctx == EXPR_CALL_FUNC)
      CODE_OP_S_ARGC(OP_CALL, id->name, exp->argc);
    else if (exp->ctx == EXPR_LOAD_FUNC)
      CODE_OP_S(OP_GET_OBJECT, id->name);
    else if (exp->ctx == EXPR_STORE)
      CODE_OP_S(OP_SET_VALUE, id->name);
    else
      panic("invalid exp's ctx %d", exp->ctx);
    break;
  default:
    panic("invalid sym kind %d", sym->kind);
    break;
  }
}

static void parser_visit_expr(ParserState *ps, Expr *exp)
{
  /* if errors is greater than MAX_ERRORS, stop parsing */
  if (ps->errnum > MAX_ERRORS)
    return;

  switch (exp->kind) {
  case SELF_KIND: {
    ParserUnit *u = ps->u;
    if (exp->ctx != EXPR_LOAD)
      panic("exp ctx is not load");
    if (u->scope == SCOPE_MODULE) {
      exp->sym = u->sym;
      exp->desc = TYPE_INCREF(u->sym->desc);
      CODE_OP(OP_LOAD_0);
    } else if (u->scope == SCOPE_CLASS) {
      panic("not implemented");
    } else {
      panic("not implemented");
    }
    break;
  }
  case SUPER_KIND: {
    /* code */
    break;
  }
  case LITERAL_KIND: {
    if (exp->ctx != EXPR_LOAD)
      panic("exp ctx is not load");
    exp->sym = get_const_symbol(exp->k.value.kind);
    if (has_error(ps))
      return;
    if (!exp->k.omit)
      CODE_OP_V(OP_LOAD_CONST, exp->k.value);
    break;
  }
  case ID_KIND: {
    Symbol *sym = find_id_symbol(ps, exp->id.name);
    if (sym == NULL) {
      syntax_error(ps, exp->row, exp->col,
                   "cannot find symbol '%s'", exp->id.name);
    } else {
      exp->sym = sym;
      exp->desc = TYPE_INCREF(sym->desc);
      exp->id.where = CURRENT_SCOPE;
      exp->id.scope = ps->u;

      if (exp->ctx == EXPR_LOAD) {
        CODE_OP(OP_LOAD_0);
        CODE_OP_S(OP_GET_VALUE, exp->id.name);
      } else if (exp->ctx == EXPR_STORE) {
        CODE_OP(OP_LOAD_0);
        CODE_OP_S(OP_SET_VALUE, exp->id.name);
      } else if (exp->ctx == EXPR_ASSIGN) {
        CODE_OP(OP_LOAD_0);
        CODE_OP_S(OP_GET_VALUE, exp->id.name);
      } else {
        panic("invalid expr's ctx %d", exp->ctx);
      }
    }
    break;
  }
  case UNARY_KIND: {
    /* code */
    break;
  }
  case BINARY_KIND: {
    /* code */
    break;
  }
  case ATTRIBUTE_KIND: {
    Expr *lexp = exp->attr.lexp;
    lexp->ctx = EXPR_LOAD;
    parser_visit_expr(ps, lexp);
    if (lexp->sym == NULL) {
      return;
    }
    parse_attr_expr(ps, exp);
    code_attr_expr(ps, exp);
    break;
  }
  case SUBSCRIPT_KIND: {
    Expr *e;
    e = exp->subscr.index;
    e->ctx = EXPR_LOAD;
    parser_visit_expr(ps, e);
    e = exp->subscr.lexp;
    e->ctx = EXPR_LOAD;
    parser_visit_expr(ps, e);
    if (!has_error(ps)) {
      if (exp->ctx != EXPR_LOAD)
        panic("subscript expr ctx is not load");
      if (!exp->leftside)
        CODE_OP(OP_SUBSCR_LOAD);
      else
        CODE_OP(OP_SUBSCR_STORE);
    }
    break;
  }
  case CALL_KIND: {
    VECTOR_REVERSE_ITERATOR(iter, exp->call.args);
    Expr *arg;
    iter_for_each(&iter, arg) {
      arg->ctx = EXPR_LOAD;
      parser_visit_expr(ps, arg);
    }

    Expr *lexp = exp->call.lexp;
    lexp->argc = vector_size(exp->call.args);
    lexp->ctx = EXPR_CALL_FUNC;
    parser_visit_expr(ps, lexp);
    if (lexp->sym == NULL)
      return;

    if (lexp->desc == NULL || lexp->desc->kind != TYPE_PROTO)
      panic("call proto error");
    TypeDesc *desc = lexp->desc;
    //check_call_args(ps, exp, desc->proto.args);
    exp->desc = TYPE_INCREF(desc->proto.ret);
    /* set call exp's sym as func's retval symbol */
    exp->sym = get_desc_symbol(ps, desc->proto.ret);
    break;
  }
  case SLICE_KIND: {
    Expr *e;
    e = exp->slice.end;
    e->ctx = EXPR_LOAD;
    parser_visit_expr(ps, e);
    e = exp->slice.start;
    e->ctx = EXPR_LOAD;
    parser_visit_expr(ps, e);
    e = exp->slice.lexp;
    e->ctx = EXPR_LOAD;
    parser_visit_expr(ps, e);
    if (!has_error(ps)) {
      if (exp->ctx != EXPR_LOAD)
        panic("slice expr ctx is not load");
      if (!exp->leftside)
        CODE_OP(OP_SLICE_LOAD);
      else
        CODE_OP(OP_SLICE_STORE);
    }
    break;
  }
  case TUPLE_KIND: {
    int size = vector_size(exp->tuple);
    if (size > 16) {
      syntax_error(ps, ps->row, ps->col, "length of tuple is larger than 16");
      return;
    }
    VECTOR_REVERSE_ITERATOR(iter, exp->tuple);
    Expr *elem;
    iter_for_each(&iter, elem) {
      elem->ctx = EXPR_LOAD;
      parser_visit_expr(ps, elem);
    }
    exp->sym = find_from_builtins("Tuple");
    exp->desc = TYPE_INCREF(exp->sym->desc);
    if (!has_error(ps)) {
      CODE_OP_I(OP_NEW_TUPLE, size);
    }
    break;
  }
  case ARRAY_KIND: {
    int size = vector_size(exp->array.elems);
    if (size > 16) {
      syntax_error(ps, ps->row, ps->col, "length of array is larger than 16");
      return;
    }
    VECTOR_REVERSE_ITERATOR(iter, exp->array.elems);
    Expr *elem;
    iter_for_each(&iter, elem) {
      elem->ctx = EXPR_LOAD;
      parser_visit_expr(ps, elem);
    }
    exp->sym = find_from_builtins("Array");
    exp->desc = TYPE_INCREF(exp->sym->desc);
    if (!has_error(ps)) {
      CODE_OP_I(OP_NEW_ARRAY, size);
    }
    break;
  }
  case MAP_ENTRY_KIND: {
    Expr *e;
    e = exp->mapentry.val;
    e->ctx = EXPR_LOAD;
    parser_visit_expr(ps, e);
    e = exp->mapentry.key;
    e->ctx = EXPR_LOAD;
    parser_visit_expr(ps, e);
    if (!has_error(ps)) {
      if (exp->ctx != EXPR_LOAD)
        panic("mapentry expr ctx is not load");
      if (exp->leftside)
        panic("mapentry is not at left side");
      CODE_OP_I(OP_NEW_TUPLE, 2);
    }
    break;
  }
  case MAP_KIND: {
    int size = vector_size(exp->map);
    if (size > 16) {
      syntax_error(ps, ps->row, ps->col, "length of dict is larger than 16");
      return;
    }
    VECTOR_REVERSE_ITERATOR(iter, exp->map);
    Expr *elem;
    iter_for_each(&iter, elem) {
      elem->ctx = EXPR_LOAD;
      parser_visit_expr(ps, elem);
    }
    exp->sym = find_from_builtins("Dict");
    exp->desc = TYPE_INCREF(exp->sym->desc);
    if (!has_error(ps)) {
      CODE_OP_I(OP_NEW_MAP, size);
    }
    break;
  }
  default:
    panic("invalid exp:%d", exp->kind);
    break;
  }
}

static void code_assign_stmt(ParserState *ps, AssignOpKind op)
{
  switch (op) {
  case OP_ASSIGN:
    /* no need generate codes */
    break;
  case OP_PLUS_ASSIGN:

    break;
  case OP_MINUS_ASSIGN:
    break;
  case OP_MULT_ASSIGN:
    break;
  case OP_DIV_ASSIGN:
    break;
  case OP_POW_ASSIGN:
    break;
  case OP_MOD_ASSIGN:
    break;
  case OP_AND_ASSIGN:
    break;
  case OP_OR_ASSIGN:
    break;
  case OP_XOR_ASSIGN:
    break;
  default:
    panic("invalid AssignOpKind %d", op);
    break;
  }
}

void parse_stmt(ParserState *ps, Stmt *stmt)
{
  switch (stmt->kind) {
  case IMPORT_KIND: {
    /* code */
    break;
  }
  case CONST_KIND: {
    break;
  }
  case VAR_KIND: {
    Ident *id = &stmt->vardecl.id;
    Type *type = &stmt->vardecl.type;
    Expr *exp = stmt->vardecl.exp;
    if (exp != NULL) {
      exp->ctx = EXPR_LOAD;
      parser_visit_expr(ps, exp);
    }

    if (exp->desc == NULL) {
      syntax_error(ps, exp->row, exp->col,
                   "cannot resolve right expr's type");
    }

    if (type->desc) {
      //check_var_type();
    } else {
      // update var type
      Symbol *sym = find_id_symbol(ps, id->name);
      if (sym == NULL)
        panic("cannot find variable '%s'", id->name);
      sym->desc = TYPE_INCREF(exp->desc);
    }

    if (!has_error(ps)) {
      ScopeKind scope = ps->u->scope;
      if (scope == SCOPE_MODULE) {
        CODE_OP(OP_LOAD_0);
        CODE_OP_S(OP_SET_VALUE, id->name);
      }
    }
    break;
  }
  case ASSIGN_KIND: {
    AssignOpKind op = stmt->assign.op;
    Expr *rexp = stmt->assign.rexp;
    Expr *lexp = stmt->assign.lexp;
    rexp->ctx = EXPR_LOAD;
    if (op == OP_ASSIGN)
      lexp->ctx = EXPR_STORE;
    else
      lexp->ctx = EXPR_ASSIGN;
    parser_visit_expr(ps, rexp);
    parser_visit_expr(ps, lexp);
    TypeDesc *rdesc = rexp->desc;
    TypeDesc *ldesc = lexp->desc;
    if (!rdesc || !ldesc) {
      int row = stmt->assign.lexp->row;
      int col = stmt->assign.lexp->col;
      syntax_error(ps, row, col, "cannot resolve right or left exprs' type");
    } else {
      //check type is compatible
      code_assign_stmt(ps, op);
    }
    break;
  }
  case FUNC_KIND: {
    break;
  }
  case RETURN_KIND: {
    break;
  }
  case EXPR_KIND: {
    Expr *exp = stmt->expr.exp;
    if (exp == NULL)
      return;
    exp->ctx = EXPR_LOAD;
    if (ps->interactive && ps->depth <= 1) {
      parser_visit_expr(ps, exp);
      if (ps->errnum > MAX_ERRORS)
        return;
      CODE_OP(OP_PRINT);
    } else if (exp->kind != LITERAL_KIND) {
      parser_visit_expr(ps, exp);
      if (ps->errnum > MAX_ERRORS)
        return;
      CODE_OP(OP_POP_TOP);
    } else {
      /* empty branch */
      panic("empty branch");
    }
    break;
  }
  default:
    panic("invalid stmt:%d", stmt->kind);
    break;
  }
}
