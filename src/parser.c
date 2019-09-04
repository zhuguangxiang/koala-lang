/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include "parser.h"
#include "opcode.h"
#include "moduleobject.h"

/* lang module */
static module _lang_;
/* ModuleObject */
static symbol *modClsSym;
/* Module, path as key */
static hashmap modules;

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

symbol *find_from_builtins(char *name)
{
  return stable_get(_lang_.stbl, name);
}

void mod_from_mobject(module *mod, Object *ob)
{
  ModuleObject *mo = (ModuleObject *)ob;
  mod->name = mo->name;
  mod->stbl = stable_from_mobject(ob);
}

symbol *mod_find_symbol(module *mod, char *name)
{
  symbol *sym = stable_get(mod->stbl, name);
  if (sym != NULL)
    return sym;
  return klass_find_member(modClsSym, name);
}

typedef struct inst {
  struct list_head link;
  int bytes;
  int argc;
  uint8_t op;
  literal arg;
  typedesc *desc;
  /* break and continue statements */
  int upbytes;
} Inst;

#define JMP_BREAK    1
#define JMP_CONTINUE 2

typedef struct jmp_inst {
  int type;
  Inst *inst;
} JmpInst;

static inline codeblock *codeblock_new(void);

static inline void codeblock_add_inst(codeblock *b, Inst *i)
{
  list_add_tail(&i->link, &b->insts);
  b->bytes += i->bytes;
  i->upbytes = b->bytes;
}

static Inst *inst_new(uint8_t op, literal *val, typedesc *desc)
{
  Inst *i = kmalloc(sizeof(Inst));
  init_list_head(&i->link);
  i->op = op;
  i->bytes = 1 + opcode_argc(op);
  if (val)
    i->arg = *val;
  i->desc = desc_incref(desc);
  return i;
}

static void inst_free(Inst *i)
{
  desc_decref(i->desc);
  kfree(i);
}

static Inst *inst_add(parserstate *ps, uint8_t op, literal *val)
{
  Inst *i = inst_new(op, val, NULL);
  parserunit *u = ps->u;
  if (u->block == NULL)
    u->block = codeblock_new();
  codeblock_add_inst(u->block, i);
  return i;
}

static inline Inst *inst_add_noarg(parserstate *ps, uint8_t op)
{
  return inst_add(ps, op, NULL);
}

static Inst *inst_add_type(parserstate *ps, uint8_t op, typedesc *desc)
{
  Inst *i = inst_new(op, NULL, desc);
  parserunit *u = ps->u;
  if (u->block == NULL)
    u->block = codeblock_new();
  codeblock_add_inst(u->block, i);
  return i;
}

static inline codeblock *codeblock_new(void)
{
  codeblock *b = kmalloc(sizeof(codeblock));
  init_list_head(&b->insts);
  return b;
}

void codeblock_free(codeblock *block)
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

static void codeblock_merge(codeblock *from, codeblock *to)
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

  codeblock *b = from->next;
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

void codeblock_show(codeblock *block)
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
    literal *val = &i->arg;
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
  case OP_NEW_ARRAY:
  case OP_NEW_MAP:
    bytebuffer_write_2bytes(buf, i->arg.ival);
    break;
  default:
    panic("invalid opcode %s", opcode_str(i->op));
    break;
  }
}

void code_gen(codeblock *block, Image *image, ByteBuffer *buf)
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
  literal v;                \
  v.kind = BASE_INT;        \
  v.ival = i;               \
  inst_add(ps, op, &v);     \
})

#define CODE_OP_S(op, s) ({ \
  literal v;                \
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

parserstate *new_parser(char *filename)
{
  parserstate *ps = kmalloc(sizeof(parserstate));
  ps->filename = filename;
  vector_init(&ps->ustack);
  return ps;
}

void free_parser(parserstate *ps)
{
  if (ps->u != NULL)
    panic("ps->u is not null");
  if (vector_size(&ps->ustack) > 0)
    panic("ps->ustack is not empty");
  kfree(ps);
}

static inline void unit_free(parserstate *ps)
{
  if (ps->u->block != NULL)
    panic("u->block is not null");
  if (vector_size(&ps->u->jmps) > 0)
    panic("u->jmps is not empty");
  vector_fini(&ps->u->jmps, NULL, NULL);
  kfree(ps->u);
  ps->u = NULL;
}

static void unit_show(parserstate *ps)
{
  parserunit *u = ps->u;
  const char *scope = scopes[u->scope];
  char *name = u->sym != NULL ? u->sym->name : NULL;
  name = (name == NULL) ? ps->filename : name;
  puts("---------------------------------------------");
  print("scope-%d(%s, %s) symbols:\n", ps->depth, scope, name);
  codeblock_show(u->block);
  puts("---------------------------------------------");
}

static void merge_into_func(parserunit *u, char *name, int create)
{
  symbol *sym = stable_get(u->stbl, name);
  if (sym == NULL) {
    if (!create)
      panic("func '%s' is not exist", name);
    debug("create '%s'", name);
    typedesc *proto = desc_from_proto(NULL, NULL);
    sym = stable_add_func(u->stbl, name, proto);
    desc_decref(proto);
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

static void unit_merge_free(parserstate *ps)
{
  parserunit *u = ps->u;

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
    symbol *sym = u->sym;
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

void parser_enter_scope(parserstate *ps, scopekind scope)
{
  debug("Enter scope-%d(%s)", ps->depth + 1, scopes[scope]);
  parserunit *u = kmalloc(sizeof(parserunit));
	u->scope = scope;
	vector_init(&u->jmps);

  /* push old unit into stack */
  if (ps->u != NULL)
    vector_push_back(&ps->ustack, ps->u);
  ps->u = u;
  ps->depth++;
}

void parser_exit_scope(parserstate *ps)
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

static symbol *find_id_symbol(parserstate *ps, char *name)
{
  symbol *sym;
  parserunit *u = ps->u;

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

static symbol *get_literal_symbol(char kind)
{
  symbol *sym;
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

static symbol *get_type_symbol(parserstate *ps, typedesc *desc)
{
  if (desc == NULL) {
    debug("no return");
    return NULL;
  }

  symbol *sym;
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

static void parse_self(parserstate *ps, expr *exp)
{
  parserunit *u = ps->u;
  if (exp->ctx != EXPR_LOAD)
    panic("exp ctx is not load");
  if (u->scope == SCOPE_MODULE) {
    exp->sym = u->sym;
    exp->desc = desc_incref(u->sym->desc);
    CODE_OP(OP_LOAD_0);
  } else if (u->scope == SCOPE_CLASS) {
    panic("not implemented");
  } else {
    panic("not implemented");
  }
}

static void parse_const(parserstate *ps, expr *exp)
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

static void parse_ident(parserstate *ps, expr *exp)
{
  symbol *sym = find_id_symbol(ps, exp->id.name);
  if (sym == NULL) {
    syntax_error(ps, exp->row, exp->col,
                 "cannot find symbol '%s'", exp->id.name);
    return;
  }

  exp->sym = sym;
  exp->desc = desc_incref(sym->desc);
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

static void parse_unary(parserstate *ps, expr *exp)
{
  expr *e = exp->unary.exp;
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
    unaryopkind op = exp->unary.op;
    if (op >= UNARY_NEG && op <= UNARY_LNOT)
      CODE_OP(opcodes[op]);
  }
}

static void parse_binary(parserstate *ps, expr *exp)
{
  expr *rexp = exp->binary.rexp;
  expr *lexp = exp->binary.lexp;

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
    exp->desc = desc_incref(lexp->desc);
  }

  if (exp->binary.op == BINARY_ADD) {
    symbol *sym = klass_find_member(exp->sym, "__add__");
    if (sym == NULL) {
      syntax_error(ps, lexp->row, lexp->col, "unsupported +");
    } else {
      typedesc *desc = sym->desc;
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
    binaryopkind op = exp->binary.op;
    if (op < BINARY_ADD || op > BINARY_LOR)
      panic("invalid binaryopkind %d", op);
    CODE_OP(opcodes[op]);
  }
}

static void parse_atrr(parserstate *ps, expr *exp)
{
  expr *lexp = exp->attr.lexp;
  lexp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, lexp);
  if (lexp->desc == NULL) {
    syntax_error(ps, lexp->row, lexp->col,
                 "cannot resolve expr's type");
  }

  symbol *lsym = lexp->sym;
  ident *id = &exp->attr.id;
  typedesc *desc;
  symbol *sym;
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
    }
    break;
  case SYM_MOD: {
    debug("left sym '%s' is a module", lsym->name);
    module *mod = lsym->mod.ptr;
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
    exp->desc = desc_incref(sym->desc);
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

static void parse_subscr(parserstate *ps, expr *exp)
{
  expr *idx = exp->subscr.index;
  idx->ctx = EXPR_LOAD;
  parser_visit_expr(ps, idx);
  if (idx->desc == NULL) {
    syntax_error(ps, idx->row, idx->col,
                 "cannot resolve expr's type");
  }

  expr *lexp = exp->subscr.lexp;
  lexp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, lexp);
  if (lexp->desc == NULL) {
    syntax_error(ps, lexp->row, lexp->col,
                 "cannot resolve expr's type");
  }

  symbol *lsym = lexp->sym;
  if (lsym == NULL) {
    return;
  }

  symbol *sym = NULL;
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
    typedesc *desc = sym->desc;
    vector *args = desc->proto.args;
    desc = vector_get(args, 0);
    if (!desc_check(idx->desc, desc)) {
      syntax_error(ps, idx->row, idx->col,
                  "subscript index type is error");
    }
  }

  if (!has_error(ps)) {
    desc_decref(exp->desc);
    vector *types = lexp->desc->klass.types;
    typedesc *desc = vector_get(types, 0);
    exp->desc = desc_incref(desc);
    exp->sym = get_type_symbol(ps, exp->desc);
    if (exp->ctx == EXPR_LOAD) {
      CODE_OP(OP_SUBSCR_LOAD);
    } else {
      CODE_OP(OP_SUBSCR_STORE);
    }
  }
}

static void parse_call(parserstate *ps, expr *exp)
{
  vector *args = exp->call.args;
  vector_reverse_iterator(iter, args);
  expr *arg;
  iter_for_each(&iter, arg) {
    arg->ctx = EXPR_LOAD;
    parser_visit_expr(ps, arg);
    if (arg->desc == NULL) {
      syntax_error(ps, arg->row, arg->col,
                   "cannot resolve expr's type");
    }
  }

  expr *lexp = exp->call.lexp;
  lexp->argc = vector_size(args);
  lexp->ctx = EXPR_CALL_FUNC;
  parser_visit_expr(ps, lexp);
  typedesc *desc = lexp->desc;
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
    vector *params = desc->proto.args;
    int psz = vector_size(params);
    int argc = vector_size(args);
    if (psz != argc) {
      syntax_error(ps, exp->row, exp->col,
                  "count of arguments are not matched");
    }
    typedesc *pdesc, *indesc;
    for (int i = 0; i < psz; ++i) {
      pdesc = vector_get(params, i);
      arg = vector_get(args, i);
      if (!desc_check(pdesc, arg->desc)) {
        syntax_error(ps, exp->row, exp->col,
                    "types are not compatible");
      }
    }
    exp->desc = desc_incref(desc->proto.ret);
    exp->sym = get_type_symbol(ps, exp->desc);
  }
}

static void parse_slice(parserstate *ps, expr *exp)
{
  expr *e = exp->slice.end;
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

static void parse_tuple(parserstate *ps, expr *exp)
{
  int size = vector_size(exp->tuple);
  if (size > 16) {
    syntax_error(ps, ps->row, ps->col,
                 "length of tuple is larger than 16");
  }

  vector_reverse_iterator(iter, exp->tuple);
  expr *e;
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

static void parse_array(parserstate *ps, expr *exp)
{
  vector *vec = exp->array.elems;
  int size = vector_size(vec);
  if (size > 16) {
    syntax_error(ps, ps->row, ps->col,
                 "length of array is larger than 16");
  }

  expr *e;
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
    CODE_OP_I(OP_NEW_ARRAY, size);
  }
}

static void parse_mapentry(parserstate *ps, expr *exp)
{
  expr *e = exp->mapentry.val;
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

static void parse_map(parserstate *ps, expr *exp)
{
  int size = vector_size(exp->map);
  if (size > 16) {
    syntax_error(ps, ps->row, ps->col,
                 "length of dict is larger than 16");
  }
  vector_reverse_iterator(iter, exp->map);
  expr *e;
  iter_for_each(&iter, e) {
    e->ctx = EXPR_LOAD;
    parser_visit_expr(ps, e);
  }

  if (!has_error(ps)) {
    CODE_OP_I(OP_NEW_MAP, size);
  }
}

static void parse_is(parserstate *ps, expr *exp)
{
  expr *e = exp->isas.exp;
  e->ctx = EXPR_LOAD;
  parser_visit_expr(ps, e);
  if (!e->desc) {
    syntax_error(ps, e->row, e->col,
                 "cannot resolve expr's type");
  }
  desc_decref(exp->desc);
  exp->desc = desc_from_bool;
  exp->sym = get_type_symbol(ps, exp->desc);

  if (!has_error(ps)) {
    CODE_OP_TYPE(OP_TYPEOF, exp->isas.type.desc);
  }
}

static void parse_as(parserstate *ps, expr *exp)
{
  expr *e = exp->isas.exp;
  e->ctx = EXPR_LOAD;
  parser_visit_expr(ps, e);
  if (!e->desc) {
    syntax_error(ps, e->row, e->col,
                 "cannot resolve expr's type");
  }
  desc_decref(exp->desc);
  exp->desc = desc_incref(exp->isas.type.desc);
  exp->sym = get_type_symbol(ps, exp->desc);

  if (!has_error(ps)) {
    CODE_OP_TYPE(OP_TYPECHECK, exp->desc);
  }
}

void parser_visit_expr(parserstate *ps, expr *exp)
{
  /* if errors is greater than MAX_ERRORS, stop parsing */
  if (ps->errnum > MAX_ERRORS)
    return;

  /* default expr has value */
  exp->hasvalue = 1;

  static void (*handlers[])(parserstate *, expr *) = {
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

static void parse_func_body(parserstate *ps, stmt *s)
{
  vector *vec = s->funcdecl.body;
  int sz = vector_size(vec);
  stmt *tmp = NULL;
  for (int i = 0; i < sz; ++i) {
    tmp = vector_get(vec, i);
    if (i == sz - 1)
      tmp->last = 1;
    parse_stmt(ps, tmp);
  }

  if (tmp != NULL) {
    /* check last statement has value or not */
    if (tmp->kind == EXPR_KIND) {
      if (tmp->hasvalue) {
        debug("last expr-stmt and has value, add OP_RETURN_VALUE");
        CODE_OP(OP_RETURN_VALUE);
      } else {
        debug("last expr-stmt and no value, add OP_RETURN");
        CODE_OP(OP_RETURN);
      }
    } else {
      if (tmp->kind != RETURN_KIND) {
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

static void add_update_variable(parserstate *ps, ident *id, typedesc *desc)
{
  parserunit *u = ps->u;
  symbol *sym;
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
    sym->desc = desc_incref(desc);
    sym->sym = get_type_symbol(ps, desc);
  }
}

static void parse_vardecl(parserstate *ps, stmt *s)
{
  expr *exp = s->vardecl.exp;
  if (exp == NULL) {
    return;
  }

  ident *id = &s->vardecl.id;
  type *type = &s->vardecl.type;
  typedesc *desc = type->desc;

  exp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, exp);
  if (!exp->desc) {
    syntax_error(ps, exp->row, exp->col,
                  "cannot resolve right expr's type");
  }

  if (desc == NULL) {
    desc = exp->desc;
  }

  if (!s->vardecl.freevar && !desc_check(desc, exp->desc)) {
    syntax_error(ps, exp->row, exp->col,
                 "types are not compatible");
  }

  /* add or update variable symbol */
  add_update_variable(ps, id, desc);

  /* generate codes */
  if (!has_error(ps)) {
    scopekind scope = ps->u->scope;
    if (scope == SCOPE_MODULE) {
      CODE_OP(OP_LOAD_0);
      CODE_OP_S(OP_SET_VALUE, id->name);
    }
  }
}

static void parse_assignment(parserstate *ps, stmt *s)
{
  assignopkind op = s->assign.op;
  expr *rexp = s->assign.rexp;
  expr *lexp = s->assign.lexp;
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

  symbol *sym = lexp->sym;
  if (sym->kind == SYM_VAR && sym->var.freevar) {
    if (sym->desc == NULL)
      panic("symbol '%s' unknown type", sym->name);
    desc_decref(sym->desc);
    sym->desc = desc_incref(rexp->desc);
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
        panic("invalid assignopkind %d", op);
      CODE_OP(opmapings[op]);
    }
  }
}

static void parse_funcdecl(parserstate *ps, stmt *s)
{
  parserunit *u = ps->u;
  char *funcname = s->funcdecl.id.name;
  parserunit *up;
  symbol *sym;
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
  parse_func_body(ps, s);

  stable_free(u->stbl);
  u->stbl = NULL;
  parser_exit_scope(ps);
  debug("end of function '%s'", funcname);
}

static void parse_return(parserstate *ps, stmt *s)
{
  expr *exp = s->ret.exp;
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

static void parse_expr(parserstate *ps, stmt *s)
{
  expr *exp = s->expr.exp;
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
    s->hasvalue = exp->hasvalue;
    if (!has_error(ps)) {
      if (!s->last && s->hasvalue) {
        /* not last statement, pop its value */
        CODE_OP(OP_POP_TOP);
      }
    }
  }
}

static void parse_block(parserstate *ps, stmt *s)
{
  parserunit *u = ps->u;
  vector *vec = s->block.vec;
  parser_enter_scope(ps, SCOPE_BLOCK);
  u->stbl = stable_new();

  int sz = vector_size(vec);
  stmt *tmp = NULL;
  for (int i = 0; i < sz; ++i) {
    tmp = vector_get(vec, i);
    parse_stmt(ps, tmp);
  }

  stable_free(u->stbl);
  u->stbl = NULL;
  parser_exit_scope(ps);
}

void parse_stmt(parserstate *ps, stmt *s)
{
  static void (*handlers[])(parserstate *, stmt *) = {
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

  if (s->kind < IMPORT_KIND || s->kind > BLOCK_KIND)
    panic("invalid statement:%d", s->kind);
  handlers[s->kind](ps, s);
}
