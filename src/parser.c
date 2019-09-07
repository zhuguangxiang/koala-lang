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
  Literal arg;
  TypeDesc *desc;
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

static Inst *inst_new(uint8_t op, Literal *val, TypeDesc *desc)
{
  Inst *i = kmalloc(sizeof(Inst));
  init_list_head(&i->link);
  i->op = op;
  i->bytes = 1 + opcode_argc(op);
  if (val)
    i->arg = *val;
  i->desc = TYPE_INCREF(desc);
  return i;
}

static void inst_free(Inst *i)
{
  TYPE_DECREF(i->desc);
  kfree(i);
}

static Inst *inst_add(ParserState *ps, uint8_t op, Literal *val)
{
  Inst *i = inst_new(op, val, NULL);
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

static Inst *inst_add_type(ParserState *ps, uint8_t op, TypeDesc *desc)
{
  Inst *i = inst_new(op, NULL, desc);
  ParserUnit *u = ps->u;
  if (u->block == NULL)
    u->block = codeblock_new();
  codeblock_add_inst(u->block, i);
  return i;
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

  Inst *i;
  struct list_head *p, *n;
  list_for_each_safe(p, n, &block->insts) {
    list_del(p);
    inst_free((Inst *)p);
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
      literal_show(&i->arg, &sbuf);
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
  case OP_LOAD_CONST:
  case OP_LOAD_MODULE:
  case OP_GET_VALUE:
  case OP_SET_VALUE: {
    Literal *val = &i->arg;
    index = Image_Add_Literal(image, val);
    bytebuffer_write_2bytes(buf, index);
    break;
  }
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
  case OP_TYPEOF:
  case OP_TYPECHECK:
    index = Image_Add_Desc(image, i->desc);
    bytebuffer_write_2bytes(buf, index);
    break;
  case OP_POP_TOP:
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
  case OP_RETURN:
  case OP_ADD:
  case OP_SUB:
  case OP_MUL:
  case OP_DIV:
  case OP_POW:
  case OP_MOD:
  case OP_NEG:
  case OP_GT:
  case OP_GE:
  case OP_LT:
  case OP_LE:
  case OP_EQ:
  case OP_NEQ:
  case OP_BIT_AND:
  case OP_BIT_OR:
  case OP_BIT_XOR:
  case OP_BIT_NOT:
  case OP_AND:
  case OP_OR:
  case OP_NOT:
  case OP_INPLACE_ADD:
  case OP_INPLACE_SUB:
  case OP_INPLACE_MUL:
  case OP_INPLACE_DIV:
  case OP_INPLACE_POW:
  case OP_INPLACE_MOD:
  case OP_INPLACE_AND:
  case OP_INPLACE_OR:
  case OP_INPLACE_XOR:
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
  case OP_NEW_MAP:
    bytebuffer_write_2bytes(buf, i->arg.ival);
    break;
  case OP_NEW_ARRAY:
    index = Image_Add_Desc(image, i->desc);
    bytebuffer_write_2bytes(buf, index);
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
  Literal v;                \
  v.kind = BASE_INT;        \
  v.ival = i;               \
  inst_add(ps, op, &v);     \
})

#define CODE_OP_S(op, s) ({ \
  Literal v;                \
  v.kind = BASE_STR;        \
  v.str = s;                \
  inst_add(ps, op, &v);     \
})

#define CODE_OP_S_ARGC(op, s, _argc) ({ \
  Inst *i = CODE_OP_S(op, s);           \
  i->argc = _argc;                      \
})

#define CODE_OP_TYPE(op, type) ({ \
  inst_add_type(ps, op, type);    \
})

static const char *scopes[] = {
  NULL, "MODULE", "CLASS", "FUNCTION", "BLOCK", "CLOSURE"
};

ParserState *new_parser(char *filename)
{
  ParserState *ps = kmalloc(sizeof(ParserState));
  ps->filename = filename;
  vector_init(&ps->ustack);
  return ps;
}

void free_parser(ParserState *ps)
{
  if (ps->u != NULL)
    panic("ps->u is not null");
  if (vector_size(&ps->ustack) > 0)
    panic("ps->ustack is not empty");
  kfree(ps);
}

static inline void unit_free(ParserState *ps)
{
  if (ps->u->block != NULL)
    panic("u->block is not null");
  if (vector_size(&ps->u->jmps) > 0)
    panic("u->jmps is not empty");
  vector_fini(&ps->u->jmps, NULL, NULL);
  kfree(ps->u);
  ps->u = NULL;
}

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

static void merge_into_func(ParserUnit *u, char *name, int create)
{
  Symbol *sym = stable_get(u->stbl, name);
  if (sym == NULL) {
    if (!create)
      panic("func '%s' is not exist", name);
    debug("create '%s'", name);
    TypeDesc *proto = desc_from_proto(NULL, NULL);
    sym = stable_add_func(u->stbl, name, proto);
    TYPE_DECREF(proto);
    sym->func.code = u->block;
  } else {
    debug("'%s' exist", name);
    if (!sym->func.code) {
      sym->func.code = u->block;
    } else {
      codeblock_merge(u->block, sym->func.code);
      codeblock_free(u->block);
    }
  }
  u->block = NULL;
}

static void unit_merge_free(ParserState *ps)
{
  ParserUnit *u = ps->u;

  switch (u->scope) {
  case SCOPE_MODULE: {
    /* module has codes for __init__ */
    if (u->block && u->block->bytes > 0) {
      merge_into_func(u, "__init__", 1);
    } else {
      codeblock_free(u->block);
      u->block = NULL;
    }
    break;
  }
  case SCOPE_FUNC: {
    Symbol *sym = u->sym;
    if (sym == NULL)
      panic("symbol is null");
    if (sym->func.code != NULL)
      panic("func '%s' already has codes", sym->name);
    sym->func.code = u->block;
    u->block = NULL;
    u->sym = NULL;
    break;
  }
  default:
    panic("invalid branch:%d", u->scope);
    break;
  }

  unit_free(ps);
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
  unit_merge_free(ps);
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

static Symbol *get_literal_symbol(char kind)
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

static Symbol *get_type_symbol(ParserState *ps, TypeDesc *desc)
{
  if (desc == NULL) {
    debug("no return");
    return NULL;
  }

  Symbol *sym;
  switch (desc->kind) {
  case TYPE_BASE:
    sym = get_literal_symbol(desc->base);
    break;
  case TYPE_KLASS:
    if (!strcmp(desc->klass.path, "lang"))
      sym = find_from_builtins(desc->klass.type);
    else
      panic("not implemented");
    break;
  case TYPE_PROTO:
    sym = get_type_symbol(ps, desc->proto.ret);
    break;
  default:
    panic("invalid desc kind %d", desc->kind);
    break;
  }
  return sym;
}

static void parse_self(ParserState *ps, Expr *exp)
{
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
}

static void parse_const(ParserState *ps, Expr *exp)
{
  if (exp->ctx != EXPR_LOAD)
    panic("exp ctx is not load");
  exp->sym = get_literal_symbol(exp->k.value.kind);
  if (!has_error(ps)) {
    if (!exp->k.omit) {
      CODE_OP_V(OP_LOAD_CONST, exp->k.value);
    }
  }
}

static void parse_ident(ParserState *ps, Expr *exp)
{
  Symbol *sym = find_id_symbol(ps, exp->id.name);
  if (sym == NULL) {
    syntax_error(ps, exp->row, exp->col,
                 "cannot find symbol '%s'", exp->id.name);
    return;
  }

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
  } else if (exp->ctx == EXPR_CALL_FUNC) {
    CODE_OP(OP_LOAD_0);
    CODE_OP_S_ARGC(OP_CALL, exp->id.name, exp->argc);
  } else {
    panic("invalid expr's ctx %d", exp->ctx);
  }
}

static void parse_unary(ParserState *ps, Expr *exp)
{
  Expr *e = exp->unary.exp;
  e->ctx = EXPR_LOAD;
  parser_visit_expr(ps, e);
  if (e->desc == NULL) {
    syntax_error(ps, e->row, e->col,
                 "cannot resolve expr's type");
  }
  exp->sym = e->sym;
  if (!has_error(ps)) {
    static int opcodes[] = {
      0, 0, OP_NEG, OP_BIT_NOT, OP_NOT
    };
    UnaryOpKind op = exp->unary.op;
    if (op >= UNARY_NEG && op <= UNARY_LNOT)
      CODE_OP(opcodes[op]);
  }
}

static void parse_binary(ParserState *ps, Expr *exp)
{
  Expr *rexp = exp->binary.rexp;
  Expr *lexp = exp->binary.lexp;

  rexp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, rexp);
  if (rexp->desc == NULL) {
    syntax_error(ps, rexp->row, rexp->col,
                 "cannot resolve expr's type");
  }

  lexp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, lexp);
  if (lexp->desc == NULL) {
    syntax_error(ps, lexp->row, lexp->col,
                 "cannot resolve expr's type");
  }

  exp->sym = get_type_symbol(ps, lexp->desc);
  if (exp->sym == NULL) {
    return;
  }

  if (exp->desc == NULL) {
    exp->desc = TYPE_INCREF(lexp->desc);
  }

  if (exp->binary.op == BINARY_ADD) {
    Symbol *sym = klass_find_member(exp->sym, "__add__");
    if (sym == NULL) {
      syntax_error(ps, lexp->row, lexp->col, "unsupported +");
    } else {
      TypeDesc *desc = sym->desc;
      if (desc == NULL || desc->kind != TYPE_PROTO)
        panic("__add__'s type descriptor is invalid");
      desc = vector_get(desc->proto.args, 0);
      if (!desc_check(desc, rexp->desc)) {
        syntax_error(ps, lexp->row, lexp->col,
                    "left and right + is not matched");
      }
    }
  }

  if (!has_error(ps)) {
    static int opcodes[] = {
      0,
      OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_POW,
      OP_GT, OP_GE, OP_LT, OP_LE, OP_EQ, OP_NEQ,
      OP_BIT_AND, OP_BIT_XOR, OP_BIT_OR,
      OP_AND, OP_OR,
    };
    BinaryOpKind op = exp->binary.op;
    if (op < BINARY_ADD || op > BINARY_LOR)
      panic("invalid BinaryOpKind %d", op);
    CODE_OP(opcodes[op]);
  }
}

static TypeDesc *type_maybe_instanced(TypeDesc *para, TypeDesc *ref)
{
  if (para->kind == TYPE_BASE)
    return TYPE_INCREF(ref);

  switch (ref->kind) {
  case TYPE_PROTO: {
    if (ref->typeparas != NULL)
      panic("Not Implemented of function");
    if (para->kind != TYPE_KLASS)
      panic("generic type bug!");
    if (para->klass.types == NULL)
      return TYPE_INCREF(ref);

    TypeDesc *rtype = ref->proto.ret;
    if (rtype != NULL && rtype->kind == TYPE_PARAREF) {
      rtype = vector_get(para->klass.types, rtype->pararef.index);
    }

    Vector *args = vector_new();
    TypeDesc *ptype;
    vector_for_each(ptype, ref->proto.args) {
      if (ptype != NULL && ptype->kind == TYPE_PARAREF) {
        ptype = vector_get(para->klass.types, ptype->pararef.index);
      }
      vector_push_back(args, ptype);
    }

    if (rtype != ref->proto.ret || vector_size(args) != 0) {
      return desc_from_proto(args, rtype);
    } else {
      vector_free(args, NULL, NULL);
      return TYPE_INCREF(ref);
    }
    break;
  }
  case TYPE_PARAREF: {
    if (para->kind != TYPE_KLASS)
      panic("generic type bug!");
    TypeDesc *desc = vector_get(para->klass.types, ref->pararef.index);
    return TYPE_INCREF(desc);
    break;
  }
  case TYPE_KLASS:
    if (ref->typeparas != NULL)
      panic("generic type bug!");
    break;
  default:
    panic("which type? generic type bug!");
    break;
  }
}

static void parse_atrr(ParserState *ps, Expr *exp)
{
  Expr *lexp = exp->attr.lexp;
  lexp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, lexp);
  if (lexp->desc == NULL) {
    syntax_error(ps, lexp->row, lexp->col,
                 "cannot resolve expr's type");
  }

  Symbol *lsym = lexp->sym;
  if (lsym == NULL)
    return;

  Ident *id = &exp->attr.id;
  TypeDesc *desc;
  TypeDesc *ldesc = lsym->desc;
  Symbol *sym;
  switch (lsym->kind) {
  case SYM_CONST:
    break;
  case SYM_VAR:
    debug("left sym '%s' is a var", lsym->name);
    sym = klass_find_member(lsym->sym, id->name);
    break;
  case SYM_FUNC:
    debug("left sym '%s' is a func", lsym->name);
    desc = lsym->desc;
    if (vector_size(desc->proto.args)) {
      syntax_error(ps, lexp->row, lexp->col,
                   "func with arguments cannot be accessed like field.");
    } else {
      sym = get_type_symbol(ps, desc->proto.ret);
      if (sym != NULL) {
        if (sym->kind != SYM_CLASS)
          panic("func's retval is not a class");
        sym = klass_find_member(sym, id->name);
      }
      ldesc = desc->proto.ret;
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
    panic("invalid left symbol %d", lsym->kind);
    break;
  }

  if (sym == NULL) {
    syntax_error(ps, id->row, id->col,
                 "'%s' is not found in '%s'",
                 id->name, lsym->name);
  } else {
    exp->sym = sym;
    exp->desc = type_maybe_instanced(ldesc, sym->desc);
  }

  // generate codes
  if (!has_error(ps)) {
    switch (sym->kind) {
    case SYM_VAR:
      if (exp->ctx == EXPR_LOAD)
        CODE_OP_S(OP_GET_VALUE, id->name);
      else if (exp->ctx == EXPR_STORE)
        CODE_OP_S(OP_SET_VALUE, id->name);
      else
        panic("invalid attr expr's ctx %d", exp->ctx);
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
      panic("invalid symbol kind %d", sym->kind);
      break;
    }
  }
}

static void parse_subscr(ParserState *ps, Expr *exp)
{
  Expr *idx = exp->subscr.index;
  idx->ctx = EXPR_LOAD;
  parser_visit_expr(ps, idx);
  if (idx->desc == NULL) {
    syntax_error(ps, idx->row, idx->col,
                 "cannot resolve expr's type");
  }

  Expr *lexp = exp->subscr.lexp;
  lexp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, lexp);
  if (lexp->desc == NULL) {
    syntax_error(ps, lexp->row, lexp->col,
                 "cannot resolve expr's type");
  }

  Symbol *lsym = lexp->sym;
  if (lsym == NULL) {
    return;
  }

  Symbol *sym = NULL;
  switch (lsym->kind) {
  case SYM_CONST:
    break;
  case SYM_VAR:
    debug("left sym '%s' is a var", lsym->name);
    sym = klass_find_member(lsym->sym, "__getitem__");
    break;
  case SYM_CLASS: {
    debug("left sym '%s' is a class", lsym->name);
    sym = klass_find_member(lsym, "__getitem__");
    break;
  }
  default:
    panic("invalid left symbol %d", lsym->kind);
    break;
  }

  if (sym == NULL || sym->kind != SYM_FUNC) {
    syntax_error(ps, lexp->row, lexp->col,
                 "'%s' is not supported subscript operation.",
                 lsym->name);
  }

  if (sym != NULL) {
    TypeDesc *desc = sym->desc;
    Vector *args = desc->proto.args;
    desc = vector_get(args, 0);
    if (!desc_check(idx->desc, desc)) {
      syntax_error(ps, idx->row, idx->col,
                  "subscript index type is error");
    }
  }

  if (!has_error(ps)) {
    TYPE_DECREF(exp->desc);
    Vector *types = lexp->desc->klass.types;
    TypeDesc *desc = vector_get(types, 0);
    exp->desc = TYPE_INCREF(desc);
    exp->sym = get_type_symbol(ps, exp->desc);
    if (exp->ctx == EXPR_LOAD) {
      CODE_OP(OP_SUBSCR_LOAD);
    } else {
      CODE_OP(OP_SUBSCR_STORE);
    }
  }
}

static void parse_call(ParserState *ps, Expr *exp)
{
  Vector *args = exp->call.args;
  VECTOR_REVERSE_ITERATOR(iter, args);
  Expr *arg;
  iter_for_each(&iter, arg) {
    arg->ctx = EXPR_LOAD;
    parser_visit_expr(ps, arg);
    if (arg->desc == NULL) {
      syntax_error(ps, arg->row, arg->col,
                   "cannot resolve expr's type");
    }
  }

  Expr *lexp = exp->call.lexp;
  lexp->argc = vector_size(args);
  lexp->ctx = EXPR_CALL_FUNC;
  parser_visit_expr(ps, lexp);
  TypeDesc *desc = lexp->desc;
  if (desc == NULL) {
    syntax_error(ps, lexp->row, lexp->col,
                 "cannot resolve expr's type");
  } else {
    if (desc->kind != TYPE_PROTO) {
      syntax_error(ps, lexp->row, lexp->col, "expr is not a func");
    }
  }

  // check call arguments
  if (!has_error(ps)) {
    Vector *params = desc->proto.args;
    int psz = vector_size(params);
    int argc = vector_size(args);
    if (psz != argc) {
      syntax_error(ps, exp->row, exp->col,
                  "count of arguments are not matched");
    }
    TypeDesc *pdesc, *indesc;
    for (int i = 0; i < psz; ++i) {
      pdesc = vector_get(params, i);
      arg = vector_get(args, i);
      if (!desc_check(pdesc, arg->desc)) {
        syntax_error(ps, exp->row, exp->col,
                    "types are not compatible");
      }
    }
    exp->desc = TYPE_INCREF(desc->proto.ret);
    exp->sym = get_type_symbol(ps, exp->desc);
  }
}

static void parse_slice(ParserState *ps, Expr *exp)
{
  Expr *e = exp->slice.end;
  e->ctx = EXPR_LOAD;
  parser_visit_expr(ps, e);
  if (e->desc == NULL) {
    syntax_error(ps, e->row, e->col,
                 "cannot resolve expr's type");
  }

  e = exp->slice.start;
  e->ctx = EXPR_LOAD;
  parser_visit_expr(ps, e);
  if (e->desc == NULL) {
    syntax_error(ps, e->row, e->col,
                 "cannot resolve expr's type");
  }

  e = exp->slice.lexp;
  e->ctx = EXPR_LOAD;
  parser_visit_expr(ps, e);
  if (e->desc == NULL) {
    syntax_error(ps, e->row, e->col,
                 "cannot resolve expr's type");
  }

  if (!has_error(ps)) {
    if (exp->ctx != EXPR_LOAD)
      panic("slice expr ctx is not load");
    if (!exp->leftside)
      CODE_OP(OP_SLICE_LOAD);
    else
      CODE_OP(OP_SLICE_STORE);
  }
}

static void parse_tuple(ParserState *ps, Expr *exp)
{
  int size = vector_size(exp->tuple);
  if (size > 16) {
    syntax_error(ps, ps->row, ps->col,
                 "length of tuple is larger than 16");
  }

  VECTOR_REVERSE_ITERATOR(iter, exp->tuple);
  Expr *e;
  iter_for_each(&iter, e) {
    e->ctx = EXPR_LOAD;
    parser_visit_expr(ps, e);
    if (e->desc == NULL) {
      syntax_error(ps, e->row, e->col,
                   "cannot resolve expr's type");
    }
  }

  if (!has_error(ps)) {
    CODE_OP_I(OP_NEW_TUPLE, size);
  }
}

static void parse_array(ParserState *ps, Expr *exp)
{
  Vector *vec = exp->array.elems;
  int size = vector_size(vec);
  if (size > 16) {
    syntax_error(ps, ps->row, ps->col,
                 "length of array is larger than 16");
  }

  Expr *e;
  for (int i = size - 1; i >= 0; --i) {
    e = vector_get(vec, i);
    e->ctx = EXPR_LOAD;
    parser_visit_expr(ps, e);
    if (e->desc == NULL) {
      syntax_error(ps, e->row, e->col,
                   "cannot resolve expr's type");
    }
  }

  if (!has_error(ps)) {
    Inst *inst = CODE_OP_I(OP_NEW_ARRAY, size);
    TypeDesc *desc = exp->desc;
    inst->desc = vector_get(desc->klass.types, 0);
  }
}

static void parse_mapentry(ParserState *ps, Expr *exp)
{
  Expr *e = exp->mapentry.val;
  e->ctx = EXPR_LOAD;
  parser_visit_expr(ps, e);
  if (e->desc == NULL) {
    syntax_error(ps, e->row, e->col,
                  "cannot resolve expr's type");
  }

  e = exp->mapentry.key;
  e->ctx = EXPR_LOAD;
  parser_visit_expr(ps, e);
  if (e->desc == NULL) {
    syntax_error(ps, e->row, e->col,
                  "cannot resolve expr's type");
  }

  if (!has_error(ps)) {
    if (exp->ctx != EXPR_LOAD)
      panic("mapentry expr ctx is not load");
    if (exp->leftside)
      panic("mapentry is not at left side");
    CODE_OP_I(OP_NEW_TUPLE, 2);
  }
}

static void parse_map(ParserState *ps, Expr *exp)
{
  int size = vector_size(exp->map);
  if (size > 16) {
    syntax_error(ps, ps->row, ps->col,
                 "length of dict is larger than 16");
  }
  VECTOR_REVERSE_ITERATOR(iter, exp->map);
  Expr *e;
  iter_for_each(&iter, e) {
    e->ctx = EXPR_LOAD;
    parser_visit_expr(ps, e);
  }

  if (!has_error(ps)) {
    CODE_OP_I(OP_NEW_MAP, size);
  }
}

static void parse_is(ParserState *ps, Expr *exp)
{
  Expr *e = exp->isas.exp;
  e->ctx = EXPR_LOAD;
  parser_visit_expr(ps, e);
  if (!e->desc) {
    syntax_error(ps, e->row, e->col,
                 "cannot resolve expr's type");
  }
  TYPE_DECREF(exp->desc);
  exp->desc = desc_from_bool;
  exp->sym = get_type_symbol(ps, exp->desc);

  if (!has_error(ps)) {
    CODE_OP_TYPE(OP_TYPEOF, exp->isas.type.desc);
  }
}

static void parse_as(ParserState *ps, Expr *exp)
{
  Expr *e = exp->isas.exp;
  e->ctx = EXPR_LOAD;
  parser_visit_expr(ps, e);
  if (!e->desc) {
    syntax_error(ps, e->row, e->col,
                 "cannot resolve expr's type");
  }
  TYPE_DECREF(exp->desc);
  exp->desc = TYPE_INCREF(exp->isas.type.desc);
  exp->sym = get_type_symbol(ps, exp->desc);

  if (!has_error(ps)) {
    CODE_OP_TYPE(OP_TYPECHECK, exp->desc);
  }
}

void parser_visit_expr(ParserState *ps, Expr *exp)
{
  /* if errors is greater than MAX_ERRORS, stop parsing */
  if (ps->errnum > MAX_ERRORS)
    return;

  /* default expr has value */
  exp->hasvalue = 1;

  static void (*handlers[])(ParserState *, Expr *) = {
    NULL,               /* INVALID        */
    NULL,               /* NIL_KIND       */
    parse_self,         /* SELF_KIND      */
    NULL,               /* SUPER_KIND     */
    parse_const,        /* LITERAL_KIND   */
    parse_ident,        /* ID_KIND        */
    parse_unary,        /* UNARY_KIND     */
    parse_binary,       /* BINARY_KIND    */
    NULL,               /* TERNARY_KIND   */
    parse_atrr,         /* ATTRIBUTE_KIND */
    parse_subscr,       /* SUBSCRIPT_KIND */
    parse_call,         /* CALL_KIND      */
    parse_slice,        /* SLICE_KIND     */
    parse_tuple,        /* TUPLE_KIND     */
    parse_array,        /* ARRAY_KIND     */
    parse_mapentry,     /* MAP_ENTRY_KIND */
    parse_map,          /* MAP_KIND       */
    NULL,               /* ANONY_KIND     */
    parse_is,           /* IS_KIND        */
    parse_as,           /* AS_KIND        */
  };

  if (exp->kind < NIL_KIND || exp->kind > AS_KIND)
    panic("invalid expression:%d", exp->kind);
  handlers[exp->kind](ps, exp);
}

static void parse_func_body(ParserState *ps, Stmt *stmt)
{
  Vector *vec = stmt->funcdecl.body;
  int sz = vector_size(vec);
  Stmt *s = NULL;
  for (int i = 0; i < sz; ++i) {
    s = vector_get(vec, i);
    if (i == sz - 1)
      s->last = 1;
    parse_stmt(ps, s);
  }

  if (s != NULL) {
    /* check last statement has value or not */
    if (s->kind == EXPR_KIND) {
      if (s->hasvalue) {
        debug("last expr-stmt and has value, add OP_RETURN_VALUE");
        CODE_OP(OP_RETURN_VALUE);
      } else {
        debug("last expr-stmt and no value, add OP_RETURN");
        CODE_OP(OP_RETURN);
      }
    } else {
      if (s->kind != RETURN_KIND) {
        debug("last not expr-stmt and not ret-stmt, add OP_RETURN");
        CODE_OP(OP_RETURN);
      }
    }
  } else {
    debug("func body is empty");
    debug("add OP_RETURN");
    CODE_OP(OP_RETURN);
  }
}

static void add_update_variable(ParserState *ps, Ident *id, TypeDesc *desc)
{
  ParserUnit *u = ps->u;
  Symbol *sym;
  switch (u->scope) {
  case SCOPE_MODULE:
  case SCOPE_CLASS:
    sym = stable_get(u->stbl, id->name);
    if (sym == NULL)
      panic("cannot find var symbol '%s'", id->name);
    break;
  default:
    panic("not implemented");
    break;
  }

  if (sym->kind != SYM_VAR)
    panic("symbol '%s' is not variable", id->name);
  if (sym->desc == NULL) {
    sym->desc = TYPE_INCREF(desc);
    sym->sym = get_type_symbol(ps, desc);
  }
}

static void parse_vardecl(ParserState *ps, Stmt *stmt)
{
  Expr *exp = stmt->vardecl.exp;
  if (exp == NULL) {
    return;
  }

  Ident *id = &stmt->vardecl.id;
  Type *type = &stmt->vardecl.type;
  TypeDesc *desc = type->desc;

  exp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, exp);
  if (!exp->desc) {
    syntax_error(ps, exp->row, exp->col,
                  "cannot resolve right expr's type");
  }

  if (desc == NULL) {
    desc = exp->desc;
  }

  if (!stmt->vardecl.freevar && !desc_check(desc, exp->desc)) {
    syntax_error(ps, exp->row, exp->col,
                 "types are not compatible");
  }

  /* add or update variable symbol */
  add_update_variable(ps, id, desc);

  /* generate codes */
  if (!has_error(ps)) {
    ScopeKind scope = ps->u->scope;
    if (scope == SCOPE_MODULE) {
      CODE_OP(OP_LOAD_0);
      CODE_OP_S(OP_SET_VALUE, id->name);
    }
  }
}

static void parse_assignment(ParserState *ps, Stmt *stmt)
{
  AssignOpKind op = stmt->assign.op;
  Expr *rexp = stmt->assign.rexp;
  Expr *lexp = stmt->assign.lexp;
  rexp->ctx = EXPR_LOAD;
  if (op == OP_ASSIGN) {
    lexp->ctx = EXPR_STORE;
  } else {
    lexp->ctx = EXPR_LOAD;
  }

  parser_visit_expr(ps, rexp);
  if (!rexp->desc) {
    syntax_error(ps, rexp->row, rexp->col,
                 "cannot resolve right expr's type");
  }

  parser_visit_expr(ps, lexp);
  if (!lexp->desc) {
    syntax_error(ps, lexp->row, lexp->col,
                 "cannot resolve left expr's type");
  }

  Symbol *sym = lexp->sym;
  if (sym->kind == SYM_VAR && sym->var.freevar) {
    if (sym->desc == NULL)
      panic("symbol '%s' unknown type", sym->name);
    TYPE_DECREF(sym->desc);
    sym->desc = TYPE_INCREF(rexp->desc);
  } else {
    // check type is compatible
    if (!has_error(ps)) {
      if (!desc_check(lexp->desc, rexp->desc)) {
        syntax_error(ps, lexp->row, lexp->col,
                      "types are not compatible");
      }
    }
  }

  if (!has_error(ps)) {
    static int opmapings[] = {
      -1,
      -1,
      OP_INPLACE_ADD, OP_INPLACE_SUB, OP_INPLACE_MUL,
      OP_INPLACE_DIV, OP_INPLACE_POW, OP_INPLACE_MOD,
      OP_INPLACE_AND, OP_INPLACE_OR, OP_INPLACE_XOR
    };
    if (op != OP_ASSIGN) {
      if (op < OP_ASSIGN || op > OP_XOR_ASSIGN)
        panic("invalid AssignOpKind %d", op);
      CODE_OP(opmapings[op]);
    }
  }
}

static void parse_funcdecl(ParserState *ps, Stmt *stmt)
{
  ParserUnit *u = ps->u;
  char *funcname = stmt->funcdecl.id.name;
  ParserUnit *up;
  Symbol *sym;
  parser_enter_scope(ps, SCOPE_FUNC);
  debug("parse function '%s'", funcname);
  u->stbl = stable_new();

  up = vector_top_back(&ps->ustack);
  if (up == NULL)
    panic("func '%s' up-unit is null", funcname);
  sym = stable_get(up->stbl, funcname);
  if (sym == NULL || sym->kind != SYM_FUNC)
    panic("symbol '%s' is not a func", funcname);
  u->sym = sym;
  parse_func_body(ps, stmt);

  stable_free(u->stbl);
  u->stbl = NULL;
  parser_exit_scope(ps);
  debug("end of function '%s'", funcname);
}

static void parse_return(ParserState *ps, Stmt *stmt)
{
  Expr *exp = stmt->ret.exp;
  if (exp != NULL) {
    debug("return has value");
    exp->ctx = EXPR_LOAD;
    parser_visit_expr(ps, exp);
    CODE_OP(OP_RETURN_VALUE);
  } else {
    debug("return has no value");
    CODE_OP(OP_RETURN);
  }
}

static void parse_expr(ParserState *ps, Stmt *stmt)
{
  Expr *exp = stmt->expr.exp;
  if (exp == NULL)
    return;
  exp->ctx = EXPR_LOAD;
  if (ps->interactive && ps->depth <= 1) {
    parser_visit_expr(ps, exp);
    if (!has_error(ps)) {
      CODE_OP(OP_PRINT);
    }
  } else  {
    parser_visit_expr(ps, exp);
    stmt->hasvalue = exp->hasvalue;
    if (!has_error(ps)) {
      if (!stmt->last && stmt->hasvalue) {
        /* not last statement, pop its value */
        CODE_OP(OP_POP_TOP);
      }
    }
  }
}

static void parse_block(ParserState *ps, Stmt *stmt)
{
  ParserUnit *u = ps->u;
  Vector *vec = stmt->block.vec;
  parser_enter_scope(ps, SCOPE_BLOCK);
  u->stbl = stable_new();

  int sz = vector_size(vec);
  Stmt *s = NULL;
  for (int i = 0; i < sz; ++i) {
    s = vector_get(vec, i);
    parse_stmt(ps, s);
  }

  stable_free(u->stbl);
  u->stbl = NULL;
  parser_exit_scope(ps);
}

void parse_stmt(ParserState *ps, Stmt *stmt)
{
  static void (*handlers[])(ParserState *, Stmt *) = {
    NULL,               /* INVALID       */
    NULL,               /* IMPORT_KIND   */
    NULL,               /* CONST_KIND    */
    parse_vardecl,      /* VAR_KIND      */
    parse_assignment,   /* ASSIGN_KIND   */
    parse_funcdecl,     /* FUNC_KIND     */
    parse_return,       /* RETURN_KIND   */
    parse_expr,         /* EXPR_KIND     */
    parse_block,        /* BLOCK_KIND    */
  };

  if (stmt->kind < IMPORT_KIND || stmt->kind > BLOCK_KIND)
    panic("invalid statement:%d", stmt->kind);
  handlers[stmt->kind](ps, stmt);
}
