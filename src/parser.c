/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#include "parser.h"
#include "opcode.h"
#include "codeobject.h"
#include "moduleobject.h"
#include "methodobject.h"
#include "fieldobject.h"

/* lang module */
static Module _lang_;
/* ModuleObject */
static Symbol *modClsSym;
/* Module, path as key */
static HashMap modules;
/* null symbol */
static Symbol nullsym;
/* any symbol */
Symbol *anysym;

static int _mod_equal_(void *e1, void *e2)
{
  Module *m1 = e1;
  Module *m2 = e2;
  if (m1 == m2)
    return 1;
  if (m1->path == m2->path)
    return 1;
  return !strcmp(m1->path, m2->path);
}

static void _mod_free_(void *e, void *arg)
{
  Module *mod = e;
  debug("free module(parser) '%s'", mod->path);
  stable_free(mod->stbl);
  kfree(mod);
}

static void init_nullsym(void)
{
  nullsym.name = "NullType";
  nullsym.kind = SYM_CLASS;
  STable *stbl = stable_new();
  Vector *args = vector_new();
  vector_push_back(args, desc_from_any);
  TypeDesc *ret = desc_from_bool;
  TypeDesc *proto = desc_from_proto(args, ret);
  stable_add_func(stbl, "__eq__", proto);
  stable_add_func(stbl, "__neq__", proto);
  TYPE_DECREF(ret);
  TYPE_DECREF(proto);
  nullsym.type.stbl = stbl;
}

static void fini_nullsym(void)
{
  stable_free(nullsym.type.stbl);

}

void init_parser(void)
{
  _lang_.path = "lang";
  Object *ob = module_load(_lang_.path);
  expect(ob != NULL);
  mod_from_mobject(&_lang_, ob);
  OB_DECREF(ob);
  modClsSym = find_from_builtins("Module");
  expect(modClsSym != NULL);
  hashmap_init(&modules, _mod_equal_);
  init_nullsym();
}

void fini_parser(void)
{
  stable_free(_lang_.stbl);
  hashmap_fini(&modules, _mod_free_, NULL);
  fini_nullsym();
}

static int isbuiltin(char *path)
{
  static char *builtins[] = {
    "lang",
    NULL,
  };

  char **s = builtins;
  while (*s != NULL) {
    if (!strcmp(path, *s))
      return 1;
    ++s;
  }
  return 0;
}

Symbol *find_from_builtins(char *name)
{
  return stable_get(_lang_.stbl, name);
}

static STable *stable_from_mobject(Module *mod, Object *ob)
{
  ModuleObject *mo = (ModuleObject *)ob;
  STable *stbl = stable_new();
  if (mo->mtbl != NULL) {
    HASHMAP_ITERATOR(iter, mo->mtbl);
    struct mnode *node;
    Object *tmp;
    Symbol *sym;
    iter_for_each(&iter, node) {
      tmp = node->obj;
      if (type_check(tmp)) {
        sym = load_type(tmp);
      } else if (field_check(tmp)) {
        sym = load_field(tmp);
        sym->var.typesym = get_desc_symbol(mod, sym->desc);
      } else if (method_check(tmp)) {
        sym = load_method(tmp);
      } else {
        panic("object of '%s'?", OB_TYPE(tmp)->name);
      }
      stable_add_symbol(stbl, sym);
      symbol_decref(sym);
    }
  }

  TypeDesc *desc = desc_from_base('s');
  stable_add_var(stbl, "__name__", desc);
  TYPE_DECREF(desc);
  return stbl;
}

void mod_from_mobject(Module *mod, Object *ob)
{
  ModuleObject *mo = (ModuleObject *)ob;
  mod->name = mo->name;
  mod->stbl = stable_from_mobject(mod, ob);
}

Symbol *mod_find_symbol(Module *mod, char *name)
{
  if (mod == NULL)
    return NULL;

  Symbol *sym = stable_get(mod->stbl, name);
  if (sym != NULL)
    return sym;
  return type_find_mbr(modClsSym, name, NULL);
}

typedef struct inst {
  struct list_head link;
  int bytes;
  int argc;
  int hasob;
  uint8_t op;
  Literal arg;
  TypeDesc *desc;
  /* jmp(iter) */
  int offset;
  /* break&continue */
  int jmpdown;
  /* anonymous func */
  Symbol *anonysym;
} Inst;

#define JMP_BREAK    1
#define JMP_CONTINUE 2

typedef struct jmp_inst {
  int type;
  Inst *inst;
} JmpInst;

static inline CodeBlock *codeblock_new(void)
{
  CodeBlock *b = kmalloc(sizeof(CodeBlock));
  init_list_head(&b->insts);
  return b;
}

static inline int codeblock_bytes(CodeBlock *block)
{
  if (block == NULL)
    return 0;
  return block->bytes;
}

static inline void codeblock_add_inst(CodeBlock *b, Inst *i)
{
  list_add_tail(&i->link, &b->insts);
  b->bytes += i->bytes;
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
  symbol_decref(i->anonysym);
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
  if (from == NULL)
    return;

  Inst *i;
  struct list_head *p, *n;
  list_for_each_safe(p, n, &from->insts) {
    i = (Inst *)p;
    list_del(p);
    from->bytes -= i->bytes;
    codeblock_add_inst(to, i);
  }
  expect(from->bytes == 0);

  CodeBlock *b = from->next;
  while (b) {
    list_for_each_safe(p, n, &b->insts) {
      i = (Inst *)p;
      list_del(p);
      b->bytes -= i->bytes;
      codeblock_add_inst(to, i);
    }
    expect(b->bytes == 0);
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
      if (i->arg.kind != 0) {
        STRBUF(sbuf);
        literal_show(&i->arg, &sbuf);
        print("%.64s", strbuf_tostr(&sbuf));
        strbuf_fini(&sbuf);
      } else {
        if (i->argc != 0) {
          print("%d", i->argc);
        } else if (i->offset != 0) {
          print("%d", i->offset);
        }
      }
      print("\n");
    }
  }
}

static void code_gen_closure(Inst *i, Image *image, ByteBuffer *buf)
{
  Symbol *sym = i->anonysym;
  ByteBuffer tmpbuf;
  uint8_t *code;
  int size;
  int locals;
  int upvals;
  VECTOR(locvec);

  bytebuffer_init(&tmpbuf, 32);
  code_gen(sym->anony.codeblock, image, &tmpbuf);
  size = bytebuffer_toarr(&tmpbuf, (char **)&code);
  fill_locvars(sym, &locvec);
  CodeInfo ci = {
    sym->name, sym->desc, code, size,
    &locvec, &sym->anony.freevec, &sym->anony.upvec,
  };
  int index = image_add_anony(image, &ci);
  bytebuffer_write_byte(buf, i->op);
  bytebuffer_write_2bytes(buf, index);
  free_locvars(&locvec);
  vector_fini(&locvec);
  bytebuffer_fini(&tmpbuf);
}

static void inst_gen(Inst *i, Image *image, ByteBuffer *buf)
{
  int index = -1;
  if (i->op != OP_NEW_CLOSURE) {
    bytebuffer_write_byte(buf, i->op);
  }
  switch (i->op) {
  case OP_CONST_BYTE:
    bytebuffer_write_byte(buf, i->arg.ival);
    break;
  case OP_LOAD_CONST:
  case OP_LOAD_MODULE:
  case OP_GET_VALUE:
  case OP_SET_VALUE:
  case OP_GET_SUPER_VALUE:
  case OP_SET_SUPER_VALUE: {
    Literal *val = &i->arg;
    index = image_add_literal(image, val);
    bytebuffer_write_2bytes(buf, index);
    break;
  }
  case OP_GET_METHOD: {
    index = image_add_string(image, i->arg.str);
    bytebuffer_write_2bytes(buf, index);
    break;
  }
  case OP_CALL:
  case OP_SUPER_CALL: {
    index = image_add_string(image, i->arg.str);
    bytebuffer_write_2bytes(buf, index);
    bytebuffer_write_byte(buf, i->argc);
    break;
  }
  case OP_EVAL:
    bytebuffer_write_byte(buf, i->argc);
    break;
  case OP_NEW_EVAL:
    index = image_add_string(image, i->arg.str);
    bytebuffer_write_2bytes(buf, index);
    bytebuffer_write_byte(buf, i->argc);
    break;
  case OP_NEW:
    index = image_add_desc(image, i->desc);
    bytebuffer_write_2bytes(buf, index);
    bytebuffer_write_byte(buf, i->argc);
    break;
  case OP_TYPEOF:
  case OP_TYPECHECK:
    index = image_add_desc(image, i->desc);
    bytebuffer_write_2bytes(buf, index);
    break;
  case OP_POP_TOP:
  case OP_SWAP:
  case OP_CONST_NULL:
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
  case OP_NEW_ITER:
  case OP_UNPACK_TUPLE:
  case OP_LOAD_GLOBAL:
    break;
  case OP_MATCH:
    bytebuffer_write_byte(buf, i->argc);
    break;
  case OP_LOAD:
  case OP_STORE:
    bytebuffer_write_byte(buf, i->arg.ival);
    break;
  case OP_JMP:
  case OP_JMP_TRUE:
  case OP_JMP_FALSE:
    bytebuffer_write_2bytes(buf, i->offset);
    break;
  case OP_NEW_TUPLE:
    bytebuffer_write_2bytes(buf, i->arg.ival);
    break;
  case OP_NEW_MAP:
    index = image_add_desc(image, i->desc);
    bytebuffer_write_2bytes(buf, index);
    bytebuffer_write_byte(buf, i->arg.ival);
    break;
  case OP_NEW_ARRAY:
    index = image_add_desc(image, i->desc);
    bytebuffer_write_2bytes(buf, index);
    bytebuffer_write_2bytes(buf, i->arg.ival);
    break;
  case OP_NEW_RANGE:
    bytebuffer_write_byte(buf, i->arg.ival);
    break;
  case OP_NEW_CLOSURE:
    code_gen_closure(i, image, buf);
    break;
  case OP_FOR_ITER:
    bytebuffer_write_2bytes(buf, i->offset);
    break;
  case OP_UPVAL_LOAD:
    bytebuffer_write_byte(buf, i->arg.ival);
    break;
  case OP_DOT_INDEX:
  case OP_INIT_CALL:
  case OP_SUPER_INIT_CALL:
    bytebuffer_write_byte(buf, i->argc);
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

#define CODE_OP_ARGC(op, _argc) ({ \
  Inst *i = CODE_OP(op);           \
  i->argc = _argc;                 \
})

#define CODE_OP_TYPE(op, type) ({ \
  inst_add_type(ps, op, type);    \
})

#define CODE_OP_ANONY(sym) ({ \
  Inst *i = inst_add_noarg(ps, OP_NEW_CLOSURE); \
  i->anonysym = sym; \
})

#define CODE_LOAD(offset)       \
({                              \
  if (offset == 0) {            \
    CODE_OP(OP_LOAD_0);         \
  } else if (offset == 1) {     \
    CODE_OP(OP_LOAD_1);         \
  } else if (offset == 2) {     \
    CODE_OP(OP_LOAD_2);         \
  } else if (offset == 3) {     \
    CODE_OP(OP_LOAD_3);         \
  } else {                      \
    CODE_OP_I(OP_LOAD, offset); \
  }                             \
})

#define CODE_STORE(offset)        \
({                                \
  if (offset == 0) {              \
    CODE_OP(OP_STORE_0);          \
  } else if (offset == 1) {       \
    CODE_OP(OP_STORE_1);          \
  } else if (offset == 2) {       \
    CODE_OP(OP_STORE_2);          \
  } else if (offset == 3) {       \
    CODE_OP(OP_STORE_3);          \
  } else {                        \
    CODE_OP_I(OP_STORE, offset);  \
  }                               \
})

static const char *scopes[] = {
  NULL, "MODULE", "CLASS", "FUNCTION", "BLOCK", "ANONY"
};

static const char *blocks[] = {
  NULL, "BLOCK", "IF-BLOCK", "WHILE-BLOCK", "FOR-BLOCK", "MATCH-BLOCK",
  "MATCH-PATTERN", "MATCH-CLAUSE",
};

ParserState *new_parser(char *filename)
{
  ParserState *ps = kmalloc(sizeof(ParserState));
  ps->filename = atom(filename);
  ps->row = 1;
  vector_init(&ps->ustack);
  vector_init(&ps->stmts);
  return ps;
}

void free_parser(ParserState *ps)
{
  expect(ps->u == NULL);
  Stmt *stmt;
  vector_for_each(stmt, &ps->stmts) {
    stmt_free(stmt);
  }
  vector_fini(&ps->stmts);
  vector_fini(&ps->ustack);
  kfree(ps);
}

static inline ParserUnit *up_scope(ParserState *ps)
{
  return vector_top_back(&ps->ustack);
}

static inline void unit_free(ParserState *ps)
{
  ParserUnit *u = ps->u;
  expect(u->block == NULL);
  expect(vector_size(&u->jmps) == 0);
  vector_fini(&u->jmps);
  kfree(u);
  ps->u = NULL;
}

static void unit_show(ParserState *ps)
{
  ParserUnit *u = ps->u;
  const char *scope = scopes[u->scope];
  char *name = u->sym != NULL ? u->sym->name : NULL;
  name = (name == NULL) ? ps->filename : name;
  puts("---------------------------------------------");
  print("scope-%d(%s, %s) codes:\n", ps->depth, scope, name);
  codeblock_show(u->block);
  puts("---------------------------------------------");
}

static void parser_add_jmp(ParserState *ps, Inst *jmp)
{
  ParserUnit *u = ps->u;
  vector_push_back(&u->jmps, jmp);
}

static void parser_handle_jmps(ParserState *ps, int upoffset)
{
  ParserUnit *u = ps->u;
  int bytes = codeblock_bytes(u->block);
  Inst *jmp;
  vector_for_each(jmp, &u->jmps) {
    if (jmp->jmpdown) {
      jmp->offset = bytes - jmp->offset;
    } else {
      jmp->offset = upoffset - jmp->offset;
    }
  }
  vector_fini(&u->jmps);
}

static void merge_into_initfunc(ParserUnit *u)
{
  static char *name = "__init__";
  Symbol *sym = stable_get(u->stbl, name);
  if (sym == NULL) {
    debug("create '%s'", name);
    TypeDesc *proto = desc_from_proto(NULL, NULL);
    sym = stable_add_func(u->stbl, name, proto);
    TYPE_DECREF(proto);
    sym->func.codeblock = u->block;
  } else {
    debug("'%s' exist", name);
    if (!sym->func.codeblock) {
      sym->func.codeblock = u->block;
    } else {
      codeblock_merge(u->block, sym->func.codeblock);
      codeblock_free(u->block);
    }
  }

#if !defined(NLog)
  puts("---------------------------------------------");
  print("__init__ codes:\n");
  codeblock_show(sym->func.codeblock);
  puts("---------------------------------------------");
#endif

  u->block = NULL;
}

static void merge_jmps(ParserUnit *from, ParserUnit *to)
{
  int offset = codeblock_bytes(to->block);
  Inst *jmp;
  vector_for_each(jmp, &from->jmps) {
    jmp->offset += offset;
    vector_push_back(&to->jmps, jmp);
  }
  vector_fini(&from->jmps);
}

static void merge_up(ParserState *ps)
{
  ParserUnit *u = ps->u;
  ParserUnit *up = up_scope(ps);
  // merge jmps(break&continue)
  merge_jmps(u, up);
  // merge instructions
  if (up->block != NULL) {
    codeblock_merge(u->block, up->block);
    codeblock_free(u->block);
  } else {
    up->block = u->block;
  }
  u->block = NULL;
  u->sym = NULL;
}

static void unit_merge_free(ParserState *ps)
{
  ParserUnit *u = ps->u;

  switch (u->scope) {
  case SCOPE_MODULE: {
    // module has codes for __init__
    if (!has_error(ps) && u->block && u->block->bytes > 0) {
      merge_into_initfunc(u);
    } else {
      codeblock_free(u->block);
      u->block = NULL;
    }
    break;
  }
  case SCOPE_FUNC: {
    Symbol *sym = u->sym;
    expect(sym != NULL);
    expect(sym->func.codeblock == NULL);
    if (!has_error(ps)) {
      sym->func.codeblock = u->block;
    } else {
      codeblock_free(u->block);
    }
    u->block = NULL;
    u->sym = NULL;
    break;
  }
  case SCOPE_BLOCK: {
    // block has codes, merge it into up unit
    if (!has_error(ps) && u->block && u->block->bytes > 0) {
      merge_up(ps);
    } else {
      codeblock_free(u->block);
      u->block = NULL;
    }
    break;
  }
  case SCOPE_ANONY: {
    Symbol *sym = u->sym;
    expect(sym != NULL);
    expect(sym->anony.codeblock == NULL);
    if (!has_error(ps)) {
      sym->anony.codeblock = u->block;
    } else {
      codeblock_free(u->block);
    }
    u->block = NULL;
    u->sym = NULL;
    break;
  }
  case SCOPE_CLASS: {
    Symbol *sym = u->sym;
    expect(sym != NULL);
    expect(sym->type.stbl != NULL);
    if (!has_error(ps) && u->block && u->block->bytes > 0) {
      merge_into_initfunc(u);
    } else {
      codeblock_free(u->block);
    }
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

void parser_enter_scope(ParserState *ps, ScopeKind scope, int block)
{
#if !defined(NLog)
  const char *scopestr;
  if (scope != SCOPE_BLOCK)
    scopestr = scopes[scope];
  else
    scopestr = blocks[block];
  debug("Enter scope-%d(%s)", ps->depth + 1, scopestr);
#endif

  ParserUnit *u = kmalloc(sizeof(ParserUnit));
	u->scope = scope;
  vector_init(&u->jmps);
  u->blocktype = block;

  /* push old unit into stack */
  if (ps->u != NULL)
    vector_push_back(&ps->ustack, ps->u);
  ps->u = u;
  ps->depth++;
}

void parser_exit_scope(ParserState *ps)
{
#if !defined(NLog)
  const char *scopestr;
  if (ps->u->scope != SCOPE_BLOCK)
    scopestr = scopes[ps->u->scope];
  else
    scopestr = blocks[ps->u->blocktype];
  debug("Exit scope-%d(%s)", ps->depth, scopestr);
#endif

#if !defined(NLog)
  unit_show(ps);
#endif

  unit_merge_free(ps);
  ps->depth--;

  /* restore ps->u to top of ps->ustack */
  if (vector_size(&ps->ustack) > 0) {
    ps->u = vector_pop_back(&ps->ustack);
  }
}

Symbol *get_desc_symbol(Module *mod, TypeDesc *desc);

static Symbol *find_from_supers(Symbol *sym, char *name)
{
  if (sym == NULL)
    return NULL;

  if (sym->kind != SYM_CLASS && sym->kind != SYM_TRAIT)
    return NULL;

  Symbol *ret;
  Symbol *item;
  vector_for_each(item, &sym->type.lro) {
    ret = stable_get(item->type.stbl, name);
    if (ret != NULL) {
      return ret;
    }
  }
  return NULL;
}

static Symbol *find_id_symbol(ParserState *ps, Expr *exp)
{
  char *name = exp->id.name;
  Symbol *sym;
  ParserUnit *u = ps->u;

  /* find id from current scope */
  sym = stable_get(u->stbl, name);
  if (sym != NULL) {
    debug("find symbol '%s' in scope-%d(%s)",
          name, ps->depth, scopes[u->scope]);
    ++sym->used;
    exp->sym = sym;
    exp->desc = TYPE_INCREF(sym->desc);
    exp->id.where = CURRENT_SCOPE;
    exp->id.scope = u;
    return sym;
  }

  /* find ident from up scope */
  ParserUnit *up;
  int depth = ps->depth;
  vector_for_each_reverse(up, &ps->ustack) {
    depth -= 1;
    sym = stable_get(up->stbl, name);
    if (sym != NULL) {
      debug("find symbol '%s' in up scope-%d(%s)",
            name, depth, scopes[up->scope]);
      sym->used++;
      exp->sym = sym;
      exp->desc = TYPE_INCREF(sym->desc);
      exp->id.where = UP_SCOPE;
      exp->id.scope = up;
      return sym;
    }

    // try to find ident from class's supers
    sym = find_from_supers(up->sym, name);
    if (sym != NULL) {
      debug("find symbol '%s' in up scope-%d(%s)'s super class/traits",
            name, depth, scopes[up->scope]);
      sym->used++;
      exp->sym = sym;
      exp->desc = TYPE_INCREF(sym->desc);
      exp->id.where = UP_SCOPE;
      exp->id.scope = up;
      return sym;
    }
  }

  /* find ident from external scope (imported) */
  /* find ident from external scope (imported dot) */
  /* find ident from auto-imported */
  sym = find_from_builtins(name);
  if (sym != NULL) {
    debug("find symbol '%s' in auto-imported packages", name);
    sym->used++;
    exp->sym = sym;
    exp->desc = TYPE_INCREF(sym->desc);
    exp->id.where = AUTO_IMPORTED;
    exp->id.scope = NULL;
    return sym;
  }

  /* find from enum type */
  if (exp->id.etype != NULL) {
    TypeDesc *desc = exp->id.etype;
    expect(desc->kind == TYPE_KLASS);
    Symbol *esym = get_desc_symbol(ps->module, desc);
    expect(esym != NULL);
    expect(esym->kind == SYM_ENUM);
    sym = stable_get(esym->type.stbl, name);
    if (sym != NULL && sym->kind == SYM_LABEL) {
      debug("find enum label '%s' in enum(%s)", name, esym->name);
      ++sym->used;
      exp->sym = sym;
      exp->desc = TYPE_INCREF(sym->desc);
      exp->id.where = ID_IN_ENUM;
      exp->id.scope = NULL;
      return sym;
    }
  }

  return NULL;
}

static Symbol *find_symbol_byname(ParserState *ps, char *name)
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

  /* find ident from up scope */
  ParserUnit *up;
  int depth = ps->depth;
  vector_for_each_reverse(up, &ps->ustack) {
    /*
    if (up->scope == SCOPE_MODULE || up->scope == SCOPE_CLASS)
      break;
    */
    depth -= 1;
    sym = stable_get(up->stbl, name);
    if (sym != NULL) {
      debug("find symbol '%s' in up scope-%d(%s)",
        name, depth, scopes[up->scope]);
      sym->used++;
      return sym;
    }
  }

  return NULL;
}

static Symbol *get_klass_symbol(Module *mod, char *path, char *name)
{
  Symbol *sym;

  if (path == NULL) {
    /* find type from current module */
    sym = stable_get(mod->stbl, name);
    if (sym != NULL) {
      debug("find symbol '%s' in current module '%s'", name, mod->name);
      if (sym->kind == SYM_CLASS) {
        ++sym->used;
        return sym;
      } else if (sym->kind == SYM_TRAIT) {
        ++sym->used;
        return sym;
      } else if(sym->kind == SYM_ENUM) {
        ++sym->used;
        return sym;
      } else {
        error("symbol '%s' is not klass", name);
        return NULL;
      }
    }

    /* find type from auto-imported modules */
    sym = find_from_builtins(name);
    if (sym != NULL) {
      debug("find symbol '%s' in auto-imported modules", name);
      if (sym->kind == SYM_CLASS) {
        ++sym->used;
        return sym;
      } else if (sym->kind == SYM_TRAIT) {
        ++sym->used;
        return sym;
      } else if(sym->kind == SYM_ENUM) {
        ++sym->used;
        return sym;
      } else {
        error("symbol '%s' is not klass", name);
        return NULL;
      }
    }

    warn("cannot find symbol '%s'", name);
    return NULL;
  } else if (isbuiltin(path)) {
    /* find type from auto-imported modules */
    sym = find_from_builtins(name);
    if (sym != NULL) {
      debug("find symbol '%s' in auto-imported modules", name);
      if (sym->kind == SYM_CLASS) {
        ++sym->used;
        return sym;
      } else if (sym->kind == SYM_TRAIT) {
        ++sym->used;
        return sym;
      } else if(sym->kind == SYM_ENUM) {
        ++sym->used;
        return sym;
      } else {
        error("symbol '%s' is not klass", name);
        return NULL;
      }
    }
    warn("cannot find symbol '%s'", name);
    return NULL;
  } else {
    // path is not null and is not builtin path, search for external module
    sym = stable_get(mod->stbl, path);
    if (sym != NULL) {
      debug("find symbol '%s'", path);
      if (sym->kind == SYM_MOD) {
        Module *m = sym->mod.ptr;
        sym = stable_get(m->stbl, name);
        if (sym != NULL) {
          debug("find symbol '%s' in '%s'", name, sym->mod.path);
          ++sym->used;
          return sym;
        }
      } else {
        warn("symbol '%s' is not module", path);
        return NULL;
      }
    }
    warn("cannot find symbol '%s'", path);
    return NULL;
  }
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
    panic("invalid literal %c", kind);
    break;
  }
  return sym;
}

Symbol *get_desc_symbol(Module *mod, TypeDesc *desc)
{
  if (desc == NULL)
    return NULL;

  Symbol *sym;
  switch (desc->kind) {
  case TYPE_BASE:
    sym = get_literal_symbol(desc->base);
    break;
  case TYPE_KLASS:
    sym = get_klass_symbol(mod, desc->klass.path, desc->klass.type);
    // update auto-imported descriptor's path
    if (sym != NULL && desc->klass.path == NULL) {
      desc->klass.path = sym->desc->klass.path;
    }
    break;
  case TYPE_PROTO:
    sym = get_desc_symbol(mod, desc->proto.ret);
    break;
  case TYPE_LABEL:
    sym = get_desc_symbol(mod, desc->label.edesc);
    break;
  default:
    panic("invalid desc %d", desc->kind);
    break;
  }

  return sym;
}

static void parse_null(ParserState *ps, Expr *exp)
{
  expect(exp->ctx == EXPR_LOAD);
  exp->sym = &nullsym;
  exp->desc = desc_from_null;
  CODE_OP(OP_CONST_NULL);
}

static void parse_self(ParserState *ps, Expr *exp)
{
  ParserUnit *u = ps->u;
  expect(exp->ctx == EXPR_LOAD);
  if (u->scope == SCOPE_FUNC) {
    ParserUnit *up = up_scope(ps);
    if (up->scope != SCOPE_CLASS) {
      synerr(ps, exp->row, exp->col, "self must be used in method");
      return;
    }
    exp->sym = up->sym;
    exp->desc = TYPE_INCREF(up->sym->desc);
    CODE_LOAD(0);
  } else if (u->scope == SCOPE_BLOCK) {
    panic("SCOPE_BLOCK: not implemented");
  } else if (u->scope == SCOPE_ANONY) {
    panic("SCOPE_ANONY: not implemented");
  } else if (u->scope == SCOPE_MODULE) {
    exp->sym = u->sym;
    exp->desc = TYPE_INCREF(u->sym->desc);
    CODE_OP(OP_LOAD_GLOBAL);
  } else if (u->scope == SCOPE_CLASS) {
    panic("SCOPE_CLASS: not implemented");
  } else {
    panic("not implemented");
  }
}

static void parse_super(ParserState *ps, Expr *exp)
{
  ParserUnit *u = ps->u;
  if (u->scope == SCOPE_FUNC) {
    ParserUnit *up = up_scope(ps);
    if (up->scope != SCOPE_CLASS) {
      synerr(ps, exp->row, exp->col, "super must be used in method");
      return;
    }
    exp->super = 1;
    exp->sym = up->sym;
    exp->desc = TYPE_INCREF(up->sym->desc);
  } else {
    int infunc = 0;
    vector_for_each_reverse(u, &ps->ustack) {
      if (u->scope == SCOPE_FUNC) {
        infunc = 1;
        break;
      }
    }
    if (!infunc) {
      synerr(ps, exp->row, exp->col, "super must be used in method");
      return;
    }

    ParserUnit *up = NULL;
    vector_for_each_reverse(u, &ps->ustack) {
      if (u->scope == SCOPE_CLASS) {
        up = u;
        break;
      }
    }

    if (up == NULL) {
      synerr(ps, exp->row, exp->col, "super must be used in method");
      return;
    }

    exp->super = 1;
    exp->sym = up->sym;
    exp->desc = TYPE_INCREF(up->sym->desc);
  }

  if (exp->ctx == EXPR_CALL_FUNC) {
    CODE_LOAD(0);
    CODE_OP_ARGC(OP_SUPER_INIT_CALL, exp->argc);
  } else {
    expect(exp->ctx == EXPR_LOAD);
    CODE_LOAD(0);
  }
}

static void parse_literal(ParserState *ps, Expr *exp)
{
  if (exp->ctx != EXPR_LOAD)
    synerr(ps, exp->row, exp->col, "constant is not writable");

  exp->sym = get_literal_symbol(exp->k.value.kind);
  if (!has_error(ps)) {
    if (!exp->k.omit) {
      CODE_OP_V(OP_LOAD_CONST, exp->k.value);
    }
  }
}

static void parse_inplace_mapping(ParserState *ps, AssignOpKind op)
{
  static int opmapings[] = {
    -1, -1,
    OP_INPLACE_ADD, OP_INPLACE_SUB, OP_INPLACE_MUL,
    OP_INPLACE_DIV, OP_INPLACE_MOD, OP_INPLACE_POW,
    OP_INPLACE_AND, OP_INPLACE_OR, OP_INPLACE_XOR
  };

  expect(op >= OP_PLUS_ASSIGN && op <= OP_XOR_ASSIGN);
  CODE_OP(opmapings[op]);
}

static void code_inplace(ParserState *ps, char *name, Expr *exp)
{
  CODE_OP(OP_DUP);
  CODE_OP_S(OP_GET_VALUE, name);
  parser_visit_expr(ps, exp->inplace->assign.rexp);
  parse_inplace_mapping(ps, exp->inplace->assign.op);
  CODE_OP(OP_SWAP);
  CODE_OP_S(OP_SET_VALUE, name);
}

static void parse_local_inplace(ParserState *ps, Expr *exp)
{
  parser_visit_expr(ps, exp->inplace->assign.rexp);
  parse_inplace_mapping(ps, exp->inplace->assign.op);
}

static void ident_in_mod(ParserState *ps, Expr *exp)
{
  Symbol *sym = exp->sym;
  debug("ident '%s' in module", exp->id.name);
  if (sym->kind == SYM_MOD) {
    expect(exp->ctx == EXPR_LOAD);
    CODE_OP_S(OP_LOAD_MODULE, sym->mod.path);
    return;
  } else if (sym->kind == SYM_CLASS || sym->kind == SYM_TRAIT) {
    synerr(ps, exp->row, exp->col,
                 "Class/Trait '%s' cannot be accessed directly", exp->id.name);
    return;
  }

  if (exp->ctx == EXPR_LOAD) {
    CODE_OP(OP_LOAD_GLOBAL);
    CODE_OP_S(OP_GET_VALUE, exp->id.name);
  } else if (exp->ctx == EXPR_STORE) {
    if (sym->kind != SYM_CONST) {
      CODE_OP(OP_LOAD_GLOBAL);
      CODE_OP_S(OP_SET_VALUE, exp->id.name);
    }
  } else if (exp->ctx == EXPR_CALL_FUNC) {
    if (sym->kind == SYM_FUNC) {
      CODE_OP(OP_LOAD_GLOBAL);
      CODE_OP_S_ARGC(OP_CALL, exp->id.name, exp->argc);
    } else if (sym->kind == SYM_VAR) {
      // FIXME: closure or function, not allowed method?
      expect(sym->desc->kind == TYPE_PROTO);
      CODE_OP(OP_LOAD_GLOBAL);
      CODE_OP_S(OP_GET_VALUE, exp->id.name);
      CODE_OP_ARGC(OP_EVAL, exp->argc);
    } else {
      panic("symbol %d is callable?", sym->kind);
    }
  } else if (exp->ctx == EXPR_INPLACE) {
    CODE_OP(OP_LOAD_GLOBAL);
    code_inplace(ps, exp->id.name, exp);
  } else if (exp->ctx == EXPR_LOAD_FUNC) {
    CODE_OP(OP_LOAD_GLOBAL);
    CODE_OP_S(OP_GET_METHOD, exp->id.name);
  } else {
    panic("invalid expr's ctx %d", exp->ctx);
  }
}

static void ident_in_func(ParserState *ps, Expr *exp)
{
  Symbol *sym = exp->sym;
  if (exp->ctx == EXPR_LOAD) {
    CODE_LOAD(sym->var.index);
  } else if (exp->ctx == EXPR_STORE) {
    CODE_STORE(sym->var.index);
  } else if (exp->ctx == EXPR_INPLACE) {
    CODE_LOAD(sym->var.index);
    parse_local_inplace(ps, exp);
    CODE_STORE(sym->var.index);
  } else if (exp->ctx == EXPR_CALL_FUNC) {
    debug("call function:%s, argc:%d", sym->name, exp->argc);
    expect(sym->desc->kind == TYPE_PROTO);
    CODE_LOAD(sym->var.index);
    CODE_OP_ARGC(OP_EVAL, exp->argc);
  } else {
    panic("invalid expr's ctx %d", exp->ctx);
  }
}

static void ident_in_block(ParserState *ps, Expr *exp)
{
  Symbol *sym = exp->sym;
  if (exp->ctx == EXPR_LOAD) {
    CODE_LOAD(sym->var.index);
  } else if (exp->ctx == EXPR_STORE) {
    CODE_STORE(sym->var.index);
  } else if (exp->ctx == EXPR_INPLACE) {
    CODE_LOAD(sym->var.index);
    parse_local_inplace(ps, exp);
    CODE_STORE(sym->var.index);
  } else {
    panic("invalid expr's ctx %d", exp->ctx);
  }
}

static void ident_in_anony(ParserState *ps, Expr *exp)
{
  Symbol *sym = exp->sym;
  if (exp->ctx == EXPR_LOAD) {
    CODE_LOAD(sym->var.index);
  } else if (exp->ctx == EXPR_STORE) {
    CODE_STORE(sym->var.index);
  } else if (exp->ctx == EXPR_INPLACE) {
    CODE_LOAD(sym->var.index);
    parse_local_inplace(ps, exp);
    CODE_STORE(sym->var.index);
  } else {
    panic("invalid expr's ctx %d", exp->ctx);
  }
}

static void ident_in_class(ParserState *ps, Expr *exp)
{
  Symbol *sym = exp->sym;
  if (exp->ctx == EXPR_LOAD) {
    CODE_LOAD(0);
    if (sym->kind == SYM_LABEL) {
      debug("sym '%s' is enum label", sym->name);
      CODE_OP_S_ARGC(OP_NEW_EVAL, exp->id.name, exp->argc);
    } else {
      CODE_OP_S(OP_GET_VALUE, exp->id.name);
    }
  } else if (exp->ctx == EXPR_STORE) {
    CODE_LOAD(0);
    CODE_OP_S(OP_SET_VALUE, exp->id.name);
  } else if (exp->ctx == EXPR_CALL_FUNC) {
    CODE_LOAD(0);
    CODE_OP_S_ARGC(OP_CALL, exp->id.name, exp->argc);
  } else {
    panic("invalid expr's ctx %d", exp->ctx);
  }
}

static void ident_up_func(ParserState *ps, Expr *exp)
{
  Symbol *sym = exp->sym;
  ParserUnit *up = exp->id.scope;
  if (up->scope == SCOPE_MODULE) {
    Symbol *sym = exp->sym;
    if (sym->kind == SYM_MOD) {
      expect(exp->ctx == EXPR_LOAD);
      CODE_OP_S(OP_LOAD_MODULE, sym->mod.path);
      return;
    }
    if (exp->ctx == EXPR_LOAD) {
      CODE_OP(OP_LOAD_GLOBAL);
      CODE_OP_S(OP_GET_VALUE, exp->id.name);
    } else if (exp->ctx == EXPR_STORE) {
      if (sym->kind != SYM_CONST) {
        CODE_OP(OP_LOAD_GLOBAL);
        CODE_OP_S(OP_SET_VALUE, exp->id.name);
      }
    } else if (exp->ctx == EXPR_INPLACE) {
      CODE_OP(OP_LOAD_GLOBAL);
      code_inplace(ps, exp->id.name, exp);
    } else if (exp->ctx == EXPR_CALL_FUNC) {
      CODE_OP(OP_LOAD_GLOBAL);
      CODE_OP_S_ARGC(OP_CALL, exp->id.name, exp->argc);
    } else if (exp->ctx == EXPR_LOAD_FUNC) {
      CODE_OP(OP_LOAD_GLOBAL);
      CODE_OP_S(OP_GET_METHOD, exp->id.name);
    } else {
      panic("invalid expr's ctx %d", exp->ctx);
    }
  } else if (up->scope == SCOPE_CLASS) {
    if (exp->ctx == EXPR_LOAD) {
      CODE_LOAD(0);
      if (sym->kind == SYM_LABEL) {
        debug("sym '%s' is enum label", sym->name);
        CODE_OP_S_ARGC(OP_NEW_EVAL, exp->id.name, exp->argc);
      } else {
        CODE_OP_S(OP_GET_VALUE, exp->id.name);
      }
    } else if (exp->ctx == EXPR_STORE) {
      CODE_LOAD(0);
      CODE_OP_S(OP_SET_VALUE, exp->id.name);
    } else if (exp->ctx == EXPR_CALL_FUNC) {
      CODE_LOAD(0);
      CODE_OP_S_ARGC(OP_CALL, exp->id.name, exp->argc);
    } else {
      panic("invalid expr's ctx %d", exp->ctx);
    }
  } else {
    panic("invalid scope");
  }
}

static void ident_up_block(ParserState *ps, Expr *exp)
{
  ParserUnit *up = exp->id.scope;
  Symbol *sym = exp->sym;

  if (up->scope == SCOPE_MODULE) {
    ident_in_mod(ps, exp);
  } else if (up->scope == SCOPE_FUNC) {
    ident_in_block(ps, exp);
  } else if (up->scope == SCOPE_BLOCK) {
    ident_in_block(ps, exp);
  } else if (up->scope == SCOPE_CLASS) {
    ident_in_class(ps, exp);
  } else {
    panic("invalid scope");
  }
}

static void ident_up_anony(ParserState *ps, Expr *exp)
{
  ParserUnit *u = ps->u;
  ParserUnit *up = exp->id.scope;
  Symbol *sym = exp->sym;

  if (up->scope == SCOPE_MODULE) {
    ident_in_mod(ps, exp);
  } else if (up->scope == SCOPE_FUNC) {
    Symbol *funcsym = up->sym;
    Vector *freevec = &funcsym->func.freevec;
    int index = vector_append_int(freevec, sym->var.index);
    Symbol *anonysym = u->sym;
    Vector *upvec = &anonysym->anony.upvec;
    index = vector_append_int(upvec, index);
    CODE_LOAD(0);
    //FIXME: upval_load_0
    CODE_OP_I(OP_UPVAL_LOAD, index);
  }
  /*
  else if (up->scope == SCOPE_BLOCK) {
    ident_in_block(ps, exp);
  } else {
    panic("invalid scope");
  }
  */
}

static void ident_builtin_mod(ParserState *ps, Expr *exp)
{
  CODE_OP_S(OP_LOAD_MODULE, "lang");
  if (exp->ctx == EXPR_LOAD) {
    CODE_OP_S(OP_GET_VALUE, exp->id.name);
  } else if (exp->ctx == EXPR_STORE) {
    CODE_OP_S(OP_SET_VALUE, exp->id.name);
  } else if (exp->ctx == EXPR_CALL_FUNC) {
    CODE_OP_S_ARGC(OP_CALL, exp->id.name, exp->argc);
  } else if (exp->ctx == EXPR_LOAD_FUNC) {
    CODE_OP_S(OP_GET_METHOD, exp->id.name);
  } else {
    panic("invalid expr's ctx %d", exp->ctx);
  }
}

static void ident_in_enum_mod(ParserState *ps, Expr *exp)
{
  CODE_OP(OP_LOAD_GLOBAL);
  if (exp->ctx == EXPR_LOAD) {
    CODE_OP_S(OP_GET_VALUE, exp->id.etype->klass.type);
    CODE_OP_S_ARGC(OP_NEW_EVAL, exp->id.name, exp->argc);
  } else if (exp->ctx == EXPR_CALL_FUNC) {
    CODE_OP_S(OP_GET_VALUE, exp->id.etype->klass.type);
    CODE_OP_S_ARGC(OP_NEW_EVAL, exp->id.name, exp->argc);
  } else {
    panic("invalid expr's ctx %d", exp->ctx);
  }
}

typedef struct {
  ScopeKind scope;
  void (*code)(ParserState *, Expr *);
} IdCodeGen;

static IdCodeGen current_codes[] = {
  {SCOPE_MODULE,  ident_in_mod},
  {SCOPE_CLASS,   NULL},
  {SCOPE_FUNC,    ident_in_func},
  {SCOPE_BLOCK,   ident_in_block},
  {SCOPE_ANONY,   ident_in_anony},
  {0, NULL},
};

static IdCodeGen up_codes[] = {
  {SCOPE_MODULE,  NULL},
  {SCOPE_CLASS,   NULL},
  {SCOPE_FUNC,    ident_up_func},
  {SCOPE_BLOCK,   ident_up_block},
  {SCOPE_ANONY,   ident_up_anony},
  {0, NULL},
};

static IdCodeGen builtin_codes[] = {
  {SCOPE_MODULE,  ident_builtin_mod},
  {SCOPE_CLASS,   NULL},
  {SCOPE_FUNC,    NULL},
  {SCOPE_BLOCK,   NULL},
  {SCOPE_ANONY,   NULL},
  {0, NULL},
};

static IdCodeGen in_enum_codes[] = {
  {SCOPE_MODULE,  ident_in_enum_mod},
  {SCOPE_CLASS,   NULL},
  {SCOPE_FUNC,    NULL},
  {SCOPE_BLOCK,   NULL},
  {SCOPE_ANONY,   NULL},
  {0, NULL},
};

#define ident_codegen(codegens, ps, arg)  \
({                                        \
  ParserUnit *u = (ps)->u;                \
  IdCodeGen *gen = codegens;              \
  while (gen->scope != 0) {               \
    if (u->scope == gen->scope) {         \
      if (gen->code != NULL)              \
        gen->code(ps, arg);               \
      break;                              \
    }                                     \
    ++gen;                                \
  }                                       \
})

static void parse_ident(ParserState *ps, Expr *exp)
{
  Symbol *sym = find_id_symbol(ps, exp);
  if (sym == NULL) {
    synerr(ps, exp->row, exp->col, "'%s' is not defined", exp->id.name);
    return;
  }

  if (sym->kind == SYM_FUNC && exp->right == NULL) {
    exp->ctx = EXPR_LOAD_FUNC;
  }

  if (has_error(ps)) {
    return;
  }

  if (exp->id.where == CURRENT_SCOPE) {
    ident_codegen(current_codes, ps, exp);
  } else if (exp->id.where == UP_SCOPE) {
    ident_codegen(up_codes, ps, exp);
  } else if (exp->id.where == AUTO_IMPORTED) {
    ident_codegen(builtin_codes, ps, exp);
  } else if (exp->id.where == ID_IN_ENUM) {
    ident_codegen(in_enum_codes, ps, exp);
  } else {
    panic("invalid ident scope");
  }
}

static void parse_underscore(ParserState *ps, Expr *exp)
{
  printf("expr is underscore(_)\n");
  exp->desc = desc_from_any;
  exp->sym = NULL;
}

static void parse_unary(ParserState *ps, Expr *exp)
{
  Expr *e = exp->unary.exp;
  e->ctx = EXPR_LOAD;
  parser_visit_expr(ps, e);

  static char *funcnames[] = {
    NULL,         // INVALID
    NULL,         // UNARY_PLUS
    "__neg__",    // UNARY_NEG
    "__not__",    // UNARY_BIT_NOT
    "__lnot__",   // UNARY_LNOT
  };

  TypeDesc *desc = NULL;
  UnaryOpKind op = exp->unary.op;
  char *funcname = funcnames[op];
  if (funcname != NULL) {
    Symbol *sym = type_find_mbr(e->sym, funcname, NULL);
    if (sym == NULL) {
      synerr(ps, e->row, e->col, "unsupported '%s'", funcname);
      return;
    } else {
      desc = sym->desc;
      expect(desc != NULL && desc->kind == TYPE_PROTO);
      expect(desc->proto.args == NULL);
      desc = desc->proto.ret;
      expect(desc != NULL);
    }
  }

  exp->sym  = e->sym;
  exp->desc = TYPE_INCREF(desc);

  if (!has_error(ps)) {
    static int opcodes[] = {
      0, 0, OP_NEG, OP_BIT_NOT, OP_NOT
    };
    if (op >= UNARY_NEG && op <= UNARY_LNOT)
      CODE_OP(opcodes[op]);
  }
}

static int bop_isbool(BinaryOpKind op)
{
  if ((op >= BINARY_GT && op <= BINARY_NEQ) ||
      (op == BINARY_LAND) || (op == BINARY_LOR))
    return 1;
  else
    return 0;
}

static int check_inherit(TypeDesc *desc, Symbol *sym, TypeDesc *symdesc);

static void parse_binary(ParserState *ps, Expr *exp)
{
  static char *funcnames[] = {
    NULL,         // INVALID
    "__add__",    // BINARY_ADD
    "__sub__",    // BINARY_SUB
    "__mul__",    // BINARY_MULT
    "__div__",    // BINARY_DIV
    "__mod__",    // BINARY_MOD
    "__pow__",    // BINARY_POW

    "__gt__",     // BINARY_GT
    "__ge__",     // BINARY_GE
    "__lt__",     // BINARY_LT
    "__le__",     // BINARY_LE
    "__eq__",     // BINARY_EQ
    "__neq__",    // BINARY_NEQ

    "__and__",    // BINARY_BIT_AND
    "__xor__",    // BINARY_BIT_XOR
    "__or__",     // BINARY_BIT_OR

    "__land__",   // BINARY_LAND
    "__lor__",    // BINARY_LOR
  };

  Expr *rexp = exp->binary.rexp;
  Expr *lexp = exp->binary.lexp;
  TypeDesc *ldesc, *rdesc;

  rexp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, rexp);
  rdesc = rexp->desc;
  if (rdesc == NULL)
    return;

  lexp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, lexp);
  ldesc = lexp->desc;
  if (ldesc == NULL)
    return;

  int check = 1;
  BinaryOpKind op = exp->binary.op;
  if (op == BINARY_EQ || op == BINARY_NEQ) {
    if (desc_isnull(ldesc) || desc_isnull(rdesc)) {
      check = 0;
    }
  }

  if (op != BINARY_EQ && op != BINARY_NEQ) {
    if (desc_isnull(ldesc) || desc_isnull(rdesc)) {
      synerr(ps, lexp->row, lexp->col, "unsupported 'null' value operations");
      check = 0;
    }
  }

  if (check) {
    char *funcname = funcnames[op];
    expect(funcname != NULL);
    Symbol *sym = type_find_mbr(lexp->sym, funcname, NULL);
    if (sym == NULL) {
      synerr(ps, lexp->row, lexp->col, "unsupported '%s'", funcname);
      return;
    } else {
      TypeDesc *desc = sym->desc;
      expect(desc != NULL && desc->kind == TYPE_PROTO);
      desc = vector_get(desc->proto.args, 0);
      if (!desc_check(desc, rexp->desc)) {
        if (!check_inherit(desc, rexp->sym, NULL)) {
          synerr(ps, exp->binary.oprow, exp->binary.opcol,
                "types of '%s' are not matched", funcname);
          return;
        }
      }
    }
  }

  if (!has_error(ps)) {
    if (bop_isbool(op)) {
      exp->sym = find_from_builtins("Bool");
      expect(exp->sym != NULL);
      exp->desc = desc_from_bool;
    } else {
      exp->sym = get_desc_symbol(ps->module, lexp->desc);
      expect(exp->sym != NULL);
      exp->desc = TYPE_INCREF(lexp->desc);
    }

    static int opcodes[] = {
      0,
      OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_POW,
      OP_GT, OP_GE, OP_LT, OP_LE, OP_EQ, OP_NEQ,
      OP_BIT_AND, OP_BIT_XOR, OP_BIT_OR,
      OP_AND, OP_OR,
    };
    expect(op >= BINARY_ADD && op <= BINARY_LOR);
    CODE_OP(opcodes[op]);
  }
}

TypeDesc *specialize_one(TypeDesc *para, TypeDesc *ref);

static int check_inherit(TypeDesc *desc, Symbol *sym, TypeDesc *symdesc)
{
  Symbol *typesym;
  if (sym->kind == SYM_VAR) {
    typesym = sym->var.typesym;
  } else {
    expect(sym->kind == SYM_CLASS);
    typesym = sym;
  }

  if (typesym->kind == SYM_PTYPE) {
    Symbol *item;
    vector_for_each_reverse(item, typesym->type.typesyms) {
      if (desc_isany(item->desc))
        continue;
      if (desc_check(desc, item->desc))
        return 1;
    }
    return 0;
  }

  Symbol *item;
  vector_for_each_reverse(item, &typesym->type.lro) {
    if (desc_isany(item->desc))
      continue;
    if (item->kind == SYM_TYPEREF) {
      TypeDesc *instanced = specialize_one(symdesc, item->desc);
      if (desc_check(desc, instanced)) {
        TYPE_DECREF(instanced);
        return 1;
      } else {
        TYPE_DECREF(instanced);
      }
    } else {
      if (desc_check(desc, item->desc)) {
        return 1;
      }
    }
  }
  return 0;
}

static void parse_ternary(ParserState *ps, Expr *exp)
{
  ParserUnit *u = ps->u;

  Expr *test = exp->ternary.test;
  test->ctx = EXPR_LOAD;
  parser_visit_expr(ps, test);
  //if not bool, error
  if (!desc_isbool(test->desc)) {
    synerr(ps, test->row, test->col, "if cond expr is not bool");
  }
  Inst *jmp = CODE_OP(OP_JMP_FALSE);
  int offset = codeblock_bytes(u->block);

  Expr *lexp = exp->ternary.lexp;
  lexp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, lexp);
  Inst *jmp2 = CODE_OP(OP_JMP);
  int offset2 = codeblock_bytes(u->block);

  jmp->offset = offset2 - offset;

  Expr *rexp = exp->ternary.rexp;
  rexp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, rexp);

  jmp2->offset = codeblock_bytes(u->block) - offset2;

  if (!desc_check(lexp->desc, rexp->desc) &&
      !check_inherit(lexp->desc, rexp->sym, NULL) &&
      !check_inherit(rexp->desc, lexp->sym, NULL)) {
    synerr(ps, rexp->row, rexp->col,
                 "type mismatch in conditional expression");
  } else {
    exp->sym = lexp->sym;
    exp->desc = TYPE_INCREF(exp->sym->desc);
  }
}

TypeDesc *specialize(TypeDesc *ref, Vector *tpvec)
{
  if (ref == NULL)
    return NULL;
  switch (ref->kind) {
  case TYPE_PROTO: {
    TypeDesc *rtype = ref->proto.ret;
    if (rtype != NULL) {
      rtype = specialize(rtype, tpvec);
    }

    Vector *args = vector_new();
    TypeDesc *ptype;
    vector_for_each(ptype, ref->proto.args) {
      ptype = specialize(ptype, tpvec);
      vector_push_back(args, ptype);
    }

    if (rtype != NULL || vector_size(args) != 0) {
      TypeDesc *proto = desc_from_proto(args, rtype);
      TYPE_DECREF(rtype);
      return proto;
    } else {
      TYPE_DECREF(rtype);
      free_descs(args);
      return TYPE_INCREF(ref);
    }
  }
  case TYPE_KLASS: {
    Vector *typeargs = vector_new();
    TypeDesc *item;
    vector_for_each(item, ref->klass.typeargs) {
      item = specialize(item, tpvec);
      vector_push_back(typeargs, item);
    }

    if (vector_size(typeargs) != 0) {
      TypeDesc *desc = desc_from_klass(ref->klass.path, ref->klass.type);
      desc->klass.typeargs = typeargs;
      return desc;
    } else {
      vector_free(typeargs);
      return TYPE_INCREF(ref);
    }
  }
  case TYPE_PARAREF: {
    if (vector_size(tpvec) <= 0) {
      return TYPE_INCREF(ref);
    }
    TypeDesc *desc = vector_get(tpvec, ref->pararef.index);
    return TYPE_INCREF(desc);
  }
  case TYPE_BASE:
  case TYPE_LABEL: {
    return TYPE_INCREF(ref);
  }
  default:
    panic("which type? generic type bug!");
    break;
  }
}

TypeDesc *specialize_one(TypeDesc *para, TypeDesc *ref)
{
  if (para == NULL || para->kind != TYPE_KLASS)
    return TYPE_INCREF(ref);

  return specialize(ref, para->klass.typeargs);
}

static inline
TypeDesc *specialize_types(TypeDesc *para1, TypeDesc *para2, TypeDesc *ref)
{
  TypeDesc *desc, *res;
  desc = specialize_one(para1, ref);
  res = specialize_one(para2, desc);
  TYPE_DECREF(desc);
  return res;
}

Symbol *get_type_symbol(ParserState *ps, TypeDesc *type);
void parse_subtype(ParserState *ps, Symbol *clssym, Vector *subtypes);

static void show_specialized_type(Symbol *sym, TypeDesc *desc)
{
  STRBUF(sbuf);
  desc_tostr(desc, &sbuf);
  debug("sym '%s' \x1b[1;35mspecialized-type: \x1b[0m%s\n",
        sym->name, strbuf_tostr(&sbuf));
  strbuf_fini(&sbuf);
}

static void parse_attr(ParserState *ps, Expr *exp)
{
  Expr *lexp = exp->attr.lexp;
  lexp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, lexp);

  Symbol *lsym = lexp->sym;
  if (lsym == NULL)
    return;

  Ident *id = &exp->attr.id;
  TypeDesc *desc;
  TypeDesc *ldesc = lexp->desc;
  Symbol *sym;
  TypeDesc *pdesc = NULL;
  switch (lsym->kind) {
  case SYM_CONST:
    debug("left sym '%s' is a const var", lsym->name);
    sym = type_find_mbr(lsym->var.typesym, id->name, &pdesc);
    break;
  case SYM_VAR:
    debug("left sym '%s' is a var", lsym->name);
    Symbol *ltypesym = lsym->var.typesym;
    if (ltypesym->kind == SYM_PTYPE && ldesc->kind != TYPE_PARAREF) {
      debug("get left sym's instanced type");
      ltypesym = get_type_symbol(ps, ldesc);
    }
    sym = type_find_mbr(ltypesym, id->name, &pdesc);
    break;
  case SYM_FUNC:
    debug("left sym '%s' is a func", lsym->name);
    desc = lsym->desc;
    if (vector_size(desc->proto.args)) {
      synerr(ps, lexp->row, lexp->col,
        "func with arguments cannot be accessed like field.");
    } else {
      sym = get_type_symbol(ps, desc->proto.ret);
      if (sym != NULL) {
        expect(sym->kind == SYM_CLASS);
        sym = type_find_mbr(sym, id->name, &pdesc);
      } else {
        synerr(ps, exp->row, exp->col, "cannot find type");
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
    if (!lexp->super) {
      sym = type_find_mbr(lsym, id->name, &pdesc);
    } else {
      sym = type_find_super_mbr(lsym, id->name);
    }
    break;
  }
  case SYM_ENUM: {
    debug("left sym '%s' is an enum", lsym->name);
    sym = type_find_mbr(lsym, id->name, &pdesc);
    if (lexp->kind == ID_KIND && sym != NULL && sym->kind != SYM_LABEL) {
      // only allowed SYM_LABEL
      sym = NULL;
    }
    break;
  }
  case SYM_LABEL: {
    debug("left sym '%s' is an enum label", lsym->name);
    sym = type_find_mbr(lsym->label.esym, id->name, &pdesc);
    break;
  }
  case SYM_TRAIT: {
    debug("left sym '%s' is a trait", lsym->name);
    if (!lexp->super) {
      sym = type_find_mbr(lsym, id->name, &pdesc);
    } else {
      sym = type_find_super_mbr(lsym, id->name);
    }
    break;
  }
  default:
    panic("invalid left symbol %d", lsym->kind);
    break;
  }

  if (sym == NULL) {
    if (!lexp->super) {
      synerr(ps, id->row, id->col,
                   "'%s' is not found in '%s'", id->name, lsym->name);
    } else {
      synerr(ps, id->row, id->col,
                   "'%s' is not found in super of '%s'",
                   id->name, lsym->name);
    }
  } else {
    if (sym->kind == SYM_FUNC && exp->right == NULL) {
      synerr(ps, exp->row, exp->col,
        "call func '%s' or return itself?", id->name);
    } else {
      exp->sym = sym;
      exp->desc = specialize_types(pdesc, ldesc, sym->desc);
      TYPE_DECREF(pdesc);
      show_specialized_type(sym, exp->desc);
    }
  }

  // generate codes
  if (!has_error(ps)) {
    switch (sym->kind) {
    case SYM_VAR: {
      if (exp->ctx == EXPR_LOAD) {
        if (!lexp->super)
          CODE_OP_S(OP_GET_VALUE, id->name);
        else
          CODE_OP_S(OP_GET_SUPER_VALUE, id->name);
      } else if (exp->ctx == EXPR_STORE) {
        if (!lexp->super)
          CODE_OP_S(OP_SET_VALUE, id->name);
        else
          CODE_OP_S(OP_SET_SUPER_VALUE, id->name);
      } else if (exp->ctx == EXPR_INPLACE) {
        code_inplace(ps, id->name, exp);
      } else {
        panic("invalid attr expr's ctx %d", exp->ctx);
      }
      break;
    }
    case SYM_FUNC: {
      if (exp->ctx == EXPR_LOAD)
        CODE_OP_S(OP_GET_VALUE, id->name);
      else if (exp->ctx == EXPR_CALL_FUNC) {
        if (!lexp->super)
          CODE_OP_S_ARGC(OP_CALL, id->name, exp->argc);
        else
          CODE_OP_S_ARGC(OP_SUPER_CALL, id->name, exp->argc);
      } else if (exp->ctx == EXPR_LOAD_FUNC)
        CODE_OP_S(OP_GET_METHOD, id->name);
      else if (exp->ctx == EXPR_STORE) {
        CODE_OP_S(OP_SET_VALUE, id->name);
      } else {
        panic("invalid exp's ctx %d", exp->ctx);
      }
      break;
    }
    case SYM_LABEL: {
      if (exp->ctx == EXPR_LOAD) {
        if (sym->label.types != NULL) {
          synerr(ps, id->row, id->col,
                       "enum '%s' needs values", id->name);
        } else{
          CODE_OP_S_ARGC(OP_NEW_EVAL, id->name, exp->argc);
        }
      } else if (exp->ctx == EXPR_CALL_FUNC) {
        if (sym->label.types == NULL) {
          synerr(ps, id->row, id->col, "enum '%s' no values", id->name);
        } else {
          CODE_OP_S_ARGC(OP_NEW_EVAL, id->name, exp->argc);
        }
      } else {
        synerr(ps, id->row, id->col, "enum '%s' is readonly", id->name);
      }
      break;
    }
    case SYM_IFUNC: {
      if (exp->ctx == EXPR_CALL_FUNC) {
        if (!lexp->super)
          CODE_OP_S_ARGC(OP_CALL, id->name, exp->argc);
        else
          CODE_OP_S_ARGC(OP_SUPER_CALL, id->name, exp->argc);
      } else {
        panic("invalid exp's ctx %d", exp->ctx);
      }
      break;
    }
    default:
      panic("invalid symbol kind %d", sym->kind);
      break;
    }
  }
}

static void parse_subscr(ParserState *ps, Expr *exp)
{
  Expr *iexp = exp->subscr.index;
  iexp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, iexp);

  Expr *lexp = exp->subscr.lexp;
  lexp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, lexp);

  Symbol *lsym = lexp->sym;
  if (lsym == NULL) {
    return;
  }

  char *funcname = "__getitem__";
  if (exp->ctx == EXPR_STORE)
    funcname = "__setitem__";

  TypeDesc *pdesc = NULL;
  Symbol *sym = NULL;
  switch (lsym->kind) {
  case SYM_CONST:
    break;
  case SYM_VAR:
    debug("left sym '%s' is a var", lsym->name);
    sym = type_find_mbr(lsym->var.typesym, funcname, &pdesc);
    break;
  case SYM_CLASS: {
    debug("left sym '%s' is a class", lsym->name);
    sym = type_find_mbr(lsym, funcname, &pdesc);
    break;
  }
  default:
    panic("invalid left symbol %d", lsym->kind);
    break;
  }

  if (sym == NULL || sym->kind != SYM_FUNC) {
    synerr(ps, lexp->row, lexp->col,
      "'%s' is not supported subscript operation.", lsym->name);
    return;
  }

  Vector *args = sym->desc->proto.args;
  TypeDesc *desc = sym->desc->proto.ret;

  if (exp->ctx == EXPR_LOAD) {
    if (vector_size(args) != 1) {
      synerr(ps, exp->row, exp->col,
        "Count of arguments of func %s is not only one", funcname);
    }
    if (desc == NULL) {
      synerr(ps, exp->row, exp->col,
        "Return value of func %s is void", funcname);
    }
  } else if (exp->ctx == EXPR_STORE) {
    if (vector_size(args) != 2) {
      synerr(ps, exp->row, exp->col,
        "Count of arguments of func %s is not two", funcname);
    }
    if (desc != NULL) {
      synerr(ps, exp->row, exp->col,
        "Return value of func %s is not void", funcname);
    }
  } else {
    panic("invalid expr's context");
  }

  // check index in 'map[index]'
  TypeDesc *tmp = vector_get(args, 0);
  desc = specialize_types(pdesc, lexp->desc, tmp);
  show_specialized_type(sym, desc);
  if (!desc_check(iexp->desc, desc)) {
    synerr(ps, iexp->row, iexp->col, "subscript index type is error");
  }
  TYPE_DECREF(desc);

  // check value in 'map[index] = value'
  expect(exp->desc == NULL);
  if (exp->ctx == EXPR_LOAD) {
    tmp = sym->desc->proto.ret;
  } else if (exp->ctx == EXPR_STORE) {
    tmp = vector_get(args, 1);
  } else {
    tmp = NULL;
  }
  desc = specialize_types(pdesc, lexp->desc, tmp);
  show_specialized_type(sym, desc);
  TYPE_DECREF(pdesc);

  exp->desc = desc;
  exp->sym = get_type_symbol(ps, desc);
  if (exp->sym == NULL) {
    synerr(ps, exp->row, exp->col, "cannot find type");
  }

  if (!has_error(ps)) {
    if (exp->ctx == EXPR_LOAD) {
      CODE_OP(OP_SUBSCR_LOAD);
    } else if (exp->ctx == EXPR_STORE) {
      CODE_OP(OP_SUBSCR_STORE);
    } else if (exp->ctx == EXPR_INPLACE) {
      panic("not implemented");
    } else {
      panic("invalid subscribe expr's ctx");
    }
  }
}

static int check_call_args(ParserState *ps, TypeDesc *proto, Vector *args)
{
  Vector *descs = proto->proto.args;
  int sz = vector_size(descs);
  int argc = vector_size(args);
  if (sz != argc)
    return -1;

  TypeDesc *desc;
  Expr *arg;
  for (int i = 0; i < sz; ++i) {
    desc = vector_get(descs, i);
    arg = vector_get(args, i);
    if (!desc_isnull(arg->desc) && !desc_check(desc, arg->desc)) {
      if (!check_inherit(desc, arg->sym, NULL)) {
        STRBUF(sbuf1);
        STRBUF(sbuf2);
        desc_tostr(desc, &sbuf1);
        desc_tostr(arg->desc, &sbuf2);
        synerr(ps, arg->row, arg->col, "expected '%s', but found '%s'",
                    strbuf_tostr(&sbuf1), strbuf_tostr(&sbuf2));
        strbuf_fini(&sbuf1);
        strbuf_fini(&sbuf2);
        return -1;
      }
    }
  }
  return 0;
}

static int check_label_args(TypeDesc *label, Vector *args)
{
  Vector *descs = label->label.types;
  int sz = vector_size(descs);
  int argc = vector_size(args);
  if (sz != argc)
    return -1;

  TypeDesc *desc;
  Expr *arg;
  for (int i = 0; i < sz; ++i) {
    desc = vector_get(descs, i);
    arg = vector_get(args, i);
    if (!desc_check(desc, arg->desc))
      return -1;
  }
  return 0;
}

static TypeDesc *check_inherit_only(TypeDesc *desc, Symbol *sym)
{
  Symbol *item;
  vector_for_each_reverse(item, &sym->type.lro) {
    if (desc_isany(item->desc))
      continue;
    if (check_klassdesc(desc, item->desc))
      return item->desc;
  }
  return NULL;
}

void parse_typepara_value(TypeDesc *para, TypeDesc *arg, Symbol *argsym,
                          ParserState *ps, Vector *values)
{
  STRBUF(sbuf1);
  STRBUF(sbuf2);
  desc_tostr(para, &sbuf1);
  desc_tostr(arg, &sbuf2);

  if (para->kind == TYPE_PARAREF) {
    debug("'%s' <- %s\n", strbuf_tostr(&sbuf1), strbuf_tostr(&sbuf2));
    vector_set(values, para->pararef.index, arg);
  } else if (para->kind == TYPE_KLASS) {
    if (arg->kind == TYPE_KLASS) {
      if (!check_klassdesc(para, arg)) {
        TypeDesc *arg2 = check_inherit_only(para, argsym);
        if (arg2 == NULL) {
          synerr(ps, 0, 0, "'%s' and '%s' are not matched-1\n",
                  strbuf_tostr(&sbuf1), strbuf_tostr(&sbuf2));
        } else {
          TypeDesc *arg2inst = specialize_one(arg, arg2);
          TypeDesc *tmp;
          TypeDesc *tmp2;
          Symbol *tmp2sym;
          vector_for_each(tmp, para->klass.typeargs) {
            tmp2 = vector_get(arg2inst->klass.typeargs, idx);
            tmp2sym = get_type_symbol(ps, tmp2);
            parse_typepara_value(tmp, tmp2, tmp2sym, ps, values);
          }
          TYPE_DECREF(arg2inst);
        }
      } else {
        TypeDesc *tmp;
        TypeDesc *tmp2;
        Symbol *tmp2sym;
        vector_for_each(tmp, para->klass.typeargs) {
          tmp2 = vector_get(arg->klass.typeargs, idx);
          tmp2sym = get_type_symbol(ps, tmp2);
          parse_typepara_value(tmp, tmp2, tmp2sym, ps, values);
        }
      }
    } else {
      synerr(ps, 0, 0, "'%s' and '%s' are not matched-2\n",
            strbuf_tostr(&sbuf1), strbuf_tostr(&sbuf2));
    }
  } else {
    debug("%s vs %s\n", strbuf_tostr(&sbuf1), strbuf_tostr(&sbuf2));
  }

  strbuf_fini(&sbuf1);
  strbuf_fini(&sbuf2);
}

static void parse_call(ParserState *ps, Expr *exp)
{
  Vector *args = exp->call.args;
  int argc = vector_size(args);
  Expr *arg;
  vector_for_each_reverse(arg, args) {
    arg->ctx = EXPR_LOAD;
    parser_visit_expr(ps, arg);
  }

  Expr *lexp = exp->call.lexp;
  lexp->argc = vector_size(args);
  lexp->ctx = EXPR_CALL_FUNC;
  parser_visit_expr(ps, lexp);
  TypeDesc *desc = lexp->desc;
  if (desc == NULL) {
    ++ps->errors;
    return;
  }

  if (desc->kind == TYPE_PROTO) {
    Symbol *fnsym = lexp->sym;
    if (vector_size(fnsym->func.typesyms) > 0) {
      // type parameter <- type argument
      VECTOR(tpvec);
      Expr *arg;
      TypeDesc *para;
      vector_for_each(para, desc->proto.args) {
        arg = vector_get(args, idx);
        parse_typepara_value(para, arg->desc, arg->sym, ps, &tpvec);
      }
      if (!has_error(ps)) {
        TypeDesc *ret = desc->proto.ret;
        TypeDesc *retinst = specialize(ret, &tpvec);
        exp->desc = TYPE_INCREF(retinst);
        exp->sym = get_type_symbol(ps, exp->desc);
        TYPE_DECREF(retinst);
      }
      vector_fini(&tpvec);
    } else {
      if (check_call_args(ps, desc, args)) {
        return;
      } else {
        exp->desc = TYPE_INCREF(desc->proto.ret);
        exp->sym = get_type_symbol(ps, exp->desc);
      }
    }
  } else if (desc->kind == TYPE_LABEL) {
    if (check_label_args(desc, args)) {
      synerr(ps, exp->row, exp->col, "enum args check failed");
    } else {
      exp->desc = TYPE_INCREF(desc->label.edesc);
      exp->sym = get_type_symbol(ps, exp->desc);
    }
  } else if (desc->kind == TYPE_KLASS) {
    expect(lexp->super);
    if (!exp->first) {
      synerr(ps, lexp->row, lexp->col,
                   "call to super must be first statement");
    }

    if (exp->funcname != NULL && strcmp(exp->funcname, "__init__")) {
      synerr(ps, lexp->row, lexp->col,
                   "call to super must be in __init__");
    }

    Symbol *init = stable_get(lexp->sym->type.stbl, "__init__");
    expect(init != NULL);
    init->super = 1;

    Symbol *base = lexp->sym->type.base;
    if (base == NULL) {
      if (argc != 0) {
        synerr(ps, lexp->row, lexp->col, "super requires no arguments");
      } else {
        synerr(ps, lexp->row, lexp->col, "no super exist");
      }
    } else if (base != NULL) {
      if (base->kind == SYM_TYPEREF)
        base = base->typeref.sym;
      init = stable_get(base->type.stbl, "__init__");
      if (init == NULL) {
        if (argc != 0) {
          synerr(ps, lexp->row, lexp->col, "super requires no arguments");
        } else {
          synerr(ps, lexp->row, lexp->col, "super no __init__");
        }
      } else {
        TypeDesc *initdesc;
        initdesc = specialize_one(lexp->sym->type.base->desc, init->desc);
        if (check_call_args(ps, initdesc, args)) {
          TYPE_DECREF(initdesc);
          return;
        } else {
          TYPE_DECREF(initdesc);
        }
      }
    }
  } else {
    synerr(ps, lexp->row, lexp->col, "expr is not a func");
  }

  if (lexp->kind == CALL_KIND && desc->kind == TYPE_PROTO) {
    debug("left expr is func call and ret is func");
    CODE_OP_ARGC(OP_EVAL, lexp->argc);
  }
}

static void parse_slice(ParserState *ps, Expr *exp)
{
  Expr *e = exp->slice.end;
  e->ctx = EXPR_LOAD;
  parser_visit_expr(ps, e);

  e = exp->slice.start;
  e->ctx = EXPR_LOAD;
  parser_visit_expr(ps, e);

  e = exp->slice.lexp;
  e->ctx = EXPR_LOAD;
  parser_visit_expr(ps, e);

  if (!has_error(ps)) {
    expect(exp->ctx == EXPR_LOAD);
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
    synerr(ps, ps->row, ps->col, "length of tuple is larger than 16");
  }

  exp->desc = desc_from_tuple;

  Expr *e;
  vector_for_each_reverse(e, exp->tuple) {
    if (exp->ctx == EXPR_STORE)
      e->ctx = EXPR_STORE;
    else
      e->ctx = EXPR_LOAD;
    parser_visit_expr(ps, e);
    if (e->desc != NULL) {
      desc_add_paratype(exp->desc, e->desc);
    }
  }

  if (!has_error(ps)) {
    if (exp->ctx == EXPR_LOAD)
      CODE_OP_I(OP_NEW_TUPLE, size);
  }
}

static TypeDesc *get_subarray_type(Vector *vec)
{
  if (vec == NULL)
    return desc_from_any;

  TypeDesc *desc = NULL;
  TypeDesc *tmp;
  vector_for_each(tmp, vec) {
    if (desc == NULL) {
      desc = tmp;
    } else {
      if (!desc_check(desc, tmp)) {
        return desc_from_any;
      }
    }
  }
  return TYPE_INCREF(desc);
}

static void parse_array(ParserState *ps, Expr *exp)
{
  Vector *vec = exp->array;
  int size = vector_size(vec);
  if (size > 16) {
    synerr(ps, ps->row, ps->col, "length of array is larger than 16");
  }

  Vector *types = NULL;
  if (vector_size(exp->array) > 0) {
    types = vector_new();
  }

  Expr *e;
  vector_for_each_reverse(e, vec) {
    e->ctx = EXPR_LOAD;
    parser_visit_expr(ps, e);
    if (e->desc != NULL) {
      vector_push_back(types, TYPE_INCREF(e->desc));
    }
  }

  exp->desc = desc_from_array;
  TypeDesc *para = get_subarray_type(types);
  if (para != NULL)
    desc_add_paratype(exp->desc, para);

  if (!has_error(ps)) {
    Inst *inst = CODE_OP_I(OP_NEW_ARRAY, size);
    inst->desc = TYPE_INCREF(exp->desc);
  }

  TYPE_DECREF(para);
  free_descs(types);
}

static void parse_map(ParserState *ps, Expr *exp)
{
  int size = vector_size(exp->map);
  if (size > 16) {
    synerr(ps, ps->row, ps->col, "length of dict is larger than 16");
  }

  MapEntry *entry;
  Vector *kvec = vector_new();
  Vector *vvec = vector_new();
  TypeDesc *desc = NULL;
  Expr *key;
  Expr *val;
  vector_for_each(entry, exp->map) {
    val = entry->val;
    val->ctx = EXPR_LOAD;
    parser_visit_expr(ps, val);
    if (val->desc != NULL) {
      vector_push_back(vvec, TYPE_INCREF(val->desc));
    }
    key = entry->key;
    key->ctx = EXPR_LOAD;
    parser_visit_expr(ps, key);
    if (key->desc != NULL) {
      if (desc == NULL) {
        desc = key->desc;
      } else {
        if (!desc_check(desc, key->desc)) {
          synerr(ps, exp->row, exp->col, "Key of Map is not the same");
        }
      }
      vector_push_back(kvec, TYPE_INCREF(key->desc));
    }
  }

  if (!has_error(ps)) {
    TypeDesc *kdesc = get_subarray_type(kvec);
    TypeDesc *vdesc = get_subarray_type(vvec);
    exp->desc = desc_from_map;
    desc_add_paratype(exp->desc, kdesc);
    desc_add_paratype(exp->desc, vdesc);
    Inst *inst = CODE_OP_I(OP_NEW_MAP, size);
    inst->desc = TYPE_INCREF(exp->desc);
    TYPE_DECREF(kdesc);
    TYPE_DECREF(vdesc);
  }

  free_descs(kvec);
  free_descs(vvec);
}

static ParserUnit *parse_block_vardecl(ParserState *ps, Ident *id)
{
  ParserUnit *u;
  Symbol *sym;
  int depth = ps->depth;
  vector_for_each_reverse(u, &ps->ustack) {
    depth -= 1;
    if (u->scope != SCOPE_MODULE && u->scope != SCOPE_CLASS) {
      sym = stable_get(u->stbl, id->name);
      if (sym != NULL) {
        synerr(ps, id->row, id->col,
          "symbol '%s' is already declared in scope-%d(%s)",
          id->name, depth, scopes[u->scope]);
        return NULL;
      }
    }

    if (u->scope == SCOPE_MODULE ||
        u->scope == SCOPE_FUNC ||
        u->scope == SCOPE_ANONY)
      return u;
  }
  return NULL;
}

Symbol *get_type_symbol(ParserState *ps, TypeDesc *type);

static Symbol *new_update_var(ParserState *ps, Ident *id, TypeDesc *desc)
{
  ParserUnit *u = ps->u;
  Symbol *sym;
  Symbol *funcsym;
  switch (u->scope) {
  case SCOPE_MODULE:
    sym = stable_get(u->stbl, id->name);
    expect(sym != NULL);
    break;
  case SCOPE_CLASS:
    sym = stable_get(u->stbl, id->name);
    expect(sym != NULL);
    break;
  case SCOPE_FUNC:
    // function scope has independent space for variables.
    debug("var '%s' declaration in func.", id->name);
    sym = stable_add_var(u->stbl, id->name, desc);
    if (sym == NULL) {
      synerr(ps, id->row, id->col, "'%s' is redeclared", id->name);
      return NULL;
    } else {
      funcsym = u->sym;
      vector_push_back(&funcsym->func.locvec, sym);
      ++sym->refcnt;
    }
    expect(sym != NULL);
    break;
  case SCOPE_BLOCK:
    // variables in block socpe must not be duplicated within function scope
    debug("var '%s' declaration in block.", id->name);
    // get up scope which has independent space for variables
    ParserUnit *up = parse_block_vardecl(ps, id);
    if (up == NULL)
      return NULL;
    sym = stable_add_var(u->stbl, id->name, desc);
    if (sym == NULL) {
      synerr(ps, id->row, id->col, "'%s' is redeclared", id->name);
      return NULL;
    }

    if (up->scope == SCOPE_MODULE) {
      funcsym = ps->module->initsym;
      vector_push_back(&funcsym->func.locvec, sym);
      sym->var.index = vector_size(&funcsym->func.locvec);
    } else {
      // set local var's index as its up's (func, closusre, etc) index
      sym->var.index = ++up->stbl->varindex;
      funcsym = up->sym;
      vector_push_back(&funcsym->func.locvec, sym);
    }
    debug("var '%s' index %d", id->name, sym->var.index);
    ++sym->refcnt;
    break;
  case SCOPE_ANONY:
    // anonymous scope has independent space for variables.
    debug("var '%s' declaration in anony func.", id->name);
    sym = stable_add_var(u->stbl, id->name, desc);
    if (sym == NULL) {
      synerr(ps, id->row, id->col, "'%s' is redeclared", id->name);
      return NULL;
    } else {
      vector_push_back(&u->sym->anony.locvec, sym);
      ++sym->refcnt;
    }
    break;
  default:
    panic("not implemented");
    break;
  }

  expect(sym->kind == SYM_VAR);

  // update type's descriptor
  if (sym->desc == NULL) {
    if (desc->kind == TYPE_LABEL) {
      sym->desc = TYPE_INCREF(desc->label.edesc);
    } else {
      sym->desc = TYPE_INCREF(desc);
    }
  }

  // update var's type's symbol
  if (sym->var.typesym == NULL) {
    if (desc->kind == TYPE_PARAREF)
      sym->var.typesym = find_symbol_byname(ps, desc->pararef.name);
    else
      sym->var.typesym = get_type_symbol(ps, sym->desc);
  }

  return sym;
}

static void parse_body(ParserState *ps, char *name, Vector *body, Type ret)
{
  int sz = vector_size(body);
  Stmt *s = NULL;
  vector_for_each(s, body) {
    if ((idx == 0) && (s->kind == EXPR_KIND)) {
      s->expr.exp->first = 1;
      s->expr.exp->funcname = name;
    }
    if (idx == sz - 1)
      s->last = 1;
    parse_stmt(ps, s);
  }

  if (has_error(ps))
    return;

  if (s == NULL) {
    // body is empty
    debug("func body is empty");
    if (ret.desc != NULL) {
      synerr(ps, ret.row, ret.col,
                   "'%s' incompatible return type", name);
    } else {
      if (strcmp(name, "__init__")) {
        debug("add OP_RETURN");
        CODE_OP(OP_RETURN);
      } else {
        debug("__init__ func no need add OP_RETURN");
      }
    }
    return;
  }

  // last one is expr-stmt, check it has value or not
  if (s->kind == EXPR_KIND) {
    Expr *exp = s->expr.exp;
    if (ret.desc == NULL && exp->desc == NULL) {
      debug("last expr-stmt and no value, add OP_RETURN");
      CODE_OP(OP_RETURN);
    } else if (ret.desc != NULL && exp->desc != NULL) {
      if (!desc_check(ret.desc, exp->desc) &&
        !check_inherit(ret.desc, exp->sym, NULL)) {
        synerr(ps, exp->row, exp->col,
                      "'%s' incompatible return type", name);
      }
      if (!has_error(ps)) {
        debug("last expr-stmt and has value, add OP_RETURN_VALUE");
        CODE_OP(OP_RETURN_VALUE);
      }
    } else if (ret.desc == NULL && exp->desc != NULL ) {
      synerr(ps, exp->row, exp->col,
                   "'%s' incompatible return type", name);
    } else {
      synerr(ps, exp->row, exp->col,
                   "'%s' incompatible return type", name);
    }
    return;
  }

  /*
  if (s->hasvalue) {
    if (ret.desc == NULL) {
      synerr(ps, ret.row, ret.col, "func '%s' no return value", name);
    } else {
      if (!desc_check(ret.desc, s->desc)) {
          synerr(ps, ret.row, ret.col, "return values are not matched");
      }
    }
  } else {
    if (ret.desc != NULL) {
      synerr(ps, ret.row, ret.col, "func '%s' needs return value", name);
    }
  }
  */

  if (!has_error(ps) && s->kind != RETURN_KIND) {
    // last one is return or other statement
    if (strcmp(name, "__init__")) {
      debug("last not expr-stmt and not ret-stmt, add OP_RETURN");
      CODE_OP(OP_RETURN);
    } else {
      debug("__init__ func no need add OP_RETURN");
    }
  }
}

static Symbol *new_anony_symbol(Expr *exp)
{
  static int id = 1;
#define ANONY_PREFIX "anony_%d"
  char name[64];
  snprintf(name, 63, ANONY_PREFIX, id++);
  debug("new anonymous func '%s'", name);

  // parse anonymous's proto
  Vector *idtypes = exp->anony.idtypes;
  Type *ret = &exp->anony.ret;
  TypeDesc *proto = parse_proto(idtypes, ret);

  //new anonymous symbol
  Symbol *sym = symbol_new(atom(name), SYM_ANONY);
  sym->desc = proto;
  return sym;
}

static void parse_anony(ParserState *ps, Expr *exp)
{
  // new anonymous symbol
  Symbol *sym = new_anony_symbol(exp);
  exp->sym = sym;
  exp->desc = TYPE_INCREF(sym->desc);

  // parse anonymous func
  parser_enter_scope(ps, SCOPE_ANONY, 0);
  ParserUnit *u = ps->u;
  u->stbl = stable_new();
  u->sym = sym;

  /* parse anony func arguments */
  Vector *idtypes = exp->anony.idtypes;
  IdType *item;
  vector_for_each(item, idtypes) {
    new_update_var(ps, &item->id, item->type.desc);
  }

  parse_body(ps, sym->name, exp->anony.body, exp->anony.ret);

  stable_free(u->stbl);
  u->stbl = NULL;
  parser_exit_scope(ps);

  if (!has_error(ps)) {
    CODE_OP_ANONY(sym);
    if (exp->ctx == EXPR_CALL_FUNC) {
      CODE_OP_ARGC(OP_EVAL, exp->argc);
    }
  }
}

static void parse_is(ParserState *ps, Expr *exp)
{
  Expr *e = exp->isas.exp;
  if (e != NULL) {
    e->ctx = EXPR_LOAD;
    parser_visit_expr(ps, e);
  }
  TYPE_DECREF(exp->desc);
  exp->desc = desc_from_bool;
  exp->sym = get_desc_symbol(ps->module, exp->desc);
  if (exp->sym == NULL) {
    synerr(ps, exp->row, exp->col, "cannot find type");
  }

  if (!has_error(ps)) {
    CODE_OP_TYPE(OP_TYPEOF, exp->isas.type.desc);
  }
}

static void parse_as(ParserState *ps, Expr *exp)
{
  Expr *e = exp->isas.exp;
  e->ctx = EXPR_LOAD;
  parser_visit_expr(ps, e);

  TYPE_DECREF(exp->desc);
  exp->desc = TYPE_INCREF(exp->isas.type.desc);
  exp->sym = get_desc_symbol(ps->module, exp->desc);
  if (exp->sym == NULL) {
    synerr(ps, exp->row, exp->col, "cannot find type");
  }

  if (!has_error(ps)) {
    CODE_OP_TYPE(OP_TYPECHECK, exp->desc);
  }
}

static
void check_new_args(ParserState *ps, Symbol *sym, TypeDesc *para, Expr *exp)
{
  Ident *id = &exp->newobj.id;
  char *name = id->name;
  int row = id->row;
  int col = id->col;

  if (sym->kind != SYM_CLASS) {
    synerr(ps, row, col, "'%s' is not a class", name);
    return;
  }

  Vector *args = exp->newobj.args;
  int argc = vector_size(args);

  Symbol *initsym = stable_get(sym->type.stbl, "__init__");
  if (initsym == NULL && argc == 0)
    return;

  if (initsym == NULL && argc > 0) {
    synerr(ps, row, col, "'%s' has no __init__()", name);
    return;
  }

  TypeDesc *desc = initsym->desc;

  if (desc->kind != TYPE_PROTO) {
    synerr(ps, row, col, "'%s': __init__ is not a func", name);
    return;
  }

  if (desc->proto.ret != NULL) {
    synerr(ps, row, col, "'%s': __init__ must be no return", name);
    return;
  }

  int npara = vector_size(desc->proto.args);
  if (argc != npara) {
    synerr(ps, row, col, "expected %d args, but %d args", npara, argc);
    return;
  }

  TypeDesc *instanced = specialize_one(para, desc);
  show_specialized_type(initsym, instanced);
  if (check_call_args(ps, instanced, args) < 0) {
    synerr(ps, row, col, "instanced-arguments checked failed.");
  }
  TYPE_DECREF(instanced);
}

static void parse_new(ParserState *ps, Expr *exp)
{
  char *path = exp->newobj.path;
  Ident *id = &exp->newobj.id;
  Symbol *sym = get_klass_symbol(ps->module, path, id->name);
  if (sym == NULL) {
    synerr(ps, id->row, id->col, "'%s' is not defined", id->name);
    return;
  }

  if (sym->kind != SYM_CLASS) {
    synerr(ps, id->row, id->col, "'%s' is not Class", id->name);
    return;
  }

  exp->sym = sym;
  Vector *types = exp->newobj.types;
  if (vector_size(sym->type.typesyms) != vector_size(types)) {
    synerr(ps, id->row, id->col,
          "'%s' type arguments is not macthed.", id->name);
    return;
  }

  if (types != NULL) {
    exp->desc = desc_dup(sym->desc);
    TypeDesc *item;
    vector_for_each(item, types) {
      Symbol *sym2 = get_type_symbol(ps, item);
      if (sym2 == NULL) {
        synerr(ps, id->row, id->col, "'%s' is not defined", id->name);
        return;
      }
      if (sym2->kind == SYM_CLASS || sym2->kind == SYM_TRAIT) {
        parse_subtype(ps, sym2, item->klass.typeargs);
        desc_add_paratype(exp->desc, item);
      } else if (sym2->kind == SYM_PTYPE) {
        item = desc_from_pararef(sym2->name, sym2->paratype.index);
        desc_add_paratype(exp->desc, item);
        TYPE_DECREF(item);
      }
    }
  } else {
    exp->desc = TYPE_INCREF(sym->desc);
  }

  // generate codes
  Vector *args = exp->newobj.args;
  int argc = vector_size(args);
  TypeDesc *desc = exp->desc;
  if (desc->kind == TYPE_BASE) {
    Expr *e;
    vector_for_each_reverse(e, args) {
      e->ctx = EXPR_LOAD;
      parser_visit_expr(ps, e);
    }
    check_new_args(ps, sym, desc, exp);
    Inst *i = CODE_OP_TYPE(OP_NEW, desc);
    i->argc = argc;
  } else if (desc->kind == TYPE_KLASS)  {
    CODE_OP_TYPE(OP_NEW, desc);
    if (argc > 0) {
      CODE_OP(OP_DUP);
      Expr *e;
      vector_for_each_reverse(e, args) {
        e->ctx = EXPR_LOAD;
        parser_visit_expr(ps, e);
      }
      check_new_args(ps, sym, desc, exp);
      CODE_OP_ARGC(OP_INIT_CALL, argc);
    } else {
      Symbol *initsym = stable_get(sym->type.stbl, "__init__");
      if (initsym != NULL) {
        check_new_args(ps, sym, desc, exp);
        CODE_OP(OP_DUP);
        CODE_OP_ARGC(OP_INIT_CALL, 0);
      }
    }
  } else {
    panic("invalid desc kind : %d", desc->kind);
  }
}

static void parse_range(ParserState *ps, Expr *exp)
{
  Expr *start = exp->range.start;
  Expr *end = exp->range.end;

  end->ctx = EXPR_LOAD;
  parser_visit_expr(ps, end);
  if (end->desc == NULL || !desc_isint(end->desc)) {
    synerr(ps, end->row, end->col,
      "range expects integer of end");
  }

  start->ctx = EXPR_LOAD;
  parser_visit_expr(ps, start);
  if (start->desc == NULL || !desc_isint(start->desc)) {
    synerr(ps, start->row, start->col,
      "range expects integer of start");
  }

  if (!has_error(ps)) {
    exp->sym = find_from_builtins("Range");
    exp->desc = TYPE_INCREF(exp->sym->desc);
    CODE_OP_I(OP_NEW_RANGE, exp->range.type);
  }
}

static void parse_enum_pattern_args(ParserState *ps, Vector *types, Expr *exp)
{
  Ident *id = &exp->enum_pattern.id;
  int row = id->row;
  int col = id->col;
  Vector *exps = exp->enum_pattern.exps;

  int size = vector_size(types);
  int argc = vector_size(exps);
  if (size != argc) {
    synerr(ps, row, col, "expected %d args, but %d", size, argc);
    return;
  }

  Expr *e;
  vector_for_each(e, exps) {
    switch (e->kind) {
    case UNDER_KIND:
      /* place holder, do nothing */
      debug("pattern is place holder(_), do nothing");
      break;
    case ID_KIND:
      debug("pattern is ident, new variable");
      Ident id = {e->id.name, e->row, e->col};
      Symbol *sym = new_update_var(ps, &id, vector_get(types, idx));
      if (sym != NULL) {
        sym->var.dotindex = idx;
      }
      break;
    case LITERAL_KIND:
      debug("pattern is literal");
      e->ctx = EXPR_LOAD;
      parse_literal(ps, e);
      if (!desc_check(vector_get(types, idx), e->desc)) {
        synerr(ps, e->row, e->col, "pattern type checked failed");
      }
      CODE_OP_I(OP_LOAD_CONST, idx);
      CODE_OP_I(OP_NEW_TUPLE, 2);
      ++exp->enum_pattern.argc;
      break;
    default:
      panic("invalid enum pattern kind %d", e->kind);
      break;
    }
  }
}

static void parse_enum_pattern(ParserState *ps, Expr *exp)
{
  Ident *id = &exp->enum_pattern.id;
  Ident *ename = &exp->enum_pattern.ename;
  Ident *mod = &exp->enum_pattern.mname;

  Symbol *msym = NULL;
  if (mod->name != NULL) {
    msym = stable_get(ps->module->stbl, mod->name);
    if (msym == NULL) {
      synerr(ps, mod->row, mod->col,
                  "cannot find symbol '%s'", mod->name);
    }

    if (msym->kind != SYM_MOD) {
      synerr(ps, id->row, id->col,
                  "'%s' is not module", id->name);
      msym = NULL;
    }
  }

  Symbol *esym = NULL;
  if (ename->name != NULL) {
    Module *m = (msym != NULL) ? msym->mod.ptr : ps->module;
    esym = stable_get(m->stbl, ename->name);
    if (esym == NULL) {
      synerr(ps, ename->row, ename->col,
                  "cannot find symbol '%s'", ename->name);
    }

    if (esym->kind != SYM_ENUM) {
      synerr(ps, ename->row, ename->col,
                  "'%s' is not enum", ename->name);
      esym = NULL;
    }
  }

  STable *stbl = NULL;
  if (esym != NULL) {
    stbl = esym->type.stbl;
  } else {
    // auto detect from match expr
    Symbol *sym = exp->enum_pattern.sym;
    if (sym == NULL || sym->kind != SYM_ENUM) {
      synerr(ps, id->row, id->col,
                  "cannot resolve '%s' symbol", id->name);
    }
  }

  if (stbl == NULL)
    return;

  Symbol *lsym = stable_get(stbl, id->name);
  if (lsym == NULL) {
    synerr(ps, id->row, id->col,
                "cannot resolve '%s' symbol", id->name);
    return;
  }

  if (lsym->kind != SYM_LABEL) {
    synerr(ps, id->row, id->col,
                "'%s' is not enum value", id->name);
    return;
  }

  parse_enum_pattern_args(ps, lsym->label.types, exp);

  if (!has_error(ps)) {
    exp->sym = lsym;
    exp->desc = TYPE_INCREF(lsym->desc->label.edesc);
  }
}

void parser_visit_expr(ParserState *ps, Expr *exp)
{
  /* if errors is greater than MAX_ERRORS, stop parsing */
  if (ps->errors >= MAX_ERRORS)
    return;

  /* default expr has value */
  exp->hasvalue = 1;

  static void (*handlers[])(ParserState *, Expr *) = {
    NULL,                 /* INVALID            */
    parse_null,           /* NULL_KIND          */
    parse_self,           /* SELF_KIND          */
    parse_super,          /* SUPER_KIND         */
    parse_literal,        /* LITERAL_KIND       */
    parse_ident,          /* ID_KIND            */
    parse_underscore,     /* UNDER_KIND         */
    parse_unary,          /* UNARY_KIND         */
    parse_binary,         /* BINARY_KIND        */
    parse_ternary,        /* TERNARY_KIND       */
    parse_attr,           /* ATTRIBUTE_KIND     */
    parse_subscr,         /* SUBSCRIPT_KIND     */
    parse_call,           /* CALL_KIND          */
    parse_slice,          /* SLICE_KIND         */
    parse_tuple,          /* TUPLE_KIND         */
    parse_array,          /* ARRAY_KIND         */
    parse_map,            /* MAP_KIND           */
    parse_anony,          /* ANONY_KIND         */
    parse_is,             /* IS_KIND            */
    parse_as,             /* AS_KIND            */
    parse_new,            /* NEW_KIND           */
    parse_range,          /* RANGE_KIND         */
    parse_enum_pattern,   /* ENUM_PATTERN_KIND  */
  };

  expect(exp->kind >= NULL_KIND && exp->kind <= ENUM_PATTERN_KIND);
  handlers[exp->kind](ps, exp);

  /* function's return maybe null */
  if (exp->kind != CALL_KIND && exp->desc == NULL) {
    ++ps->errors;
  }
}

static Module *new_mod_from_mobject(Module *_mod, char *path)
{
  Object *ob = module_load(path);
  if (ob == NULL) {
    warn("cannot load module '%s'", path);
    return NULL;
  }
  debug("new module(parser) '%s' from memory", path);
  Module *mod = kmalloc(sizeof(Module));
  ModuleObject *mo = (ModuleObject *)ob;
  mod->path = path;
  mod->name = mo->name;
  mod->stbl = stable_from_mobject(_mod, ob);
  hashmap_entry_init(mod, strhash(path));
  hashmap_add(&modules, mod);
  OB_DECREF(ob);
  return mod;
}

static void parse_import(ParserState *ps, Stmt *s)
{
  ParserUnit *u = ps->u;
  int type = s->import.type;

  if (type == IMPORT_ALL) {

  } else if (type == IMPORT_PARTIAL) {

  } else {
    expect(type == 0);
    Ident *id = &s->import.id;
    char *name = id->name;
    char *path = s->import.path;
    path = atom(path);
    if (name == NULL) {
      name = strrchr(path, '/');
      name = (name == NULL) ? path : atom(name + 1);
    }

    Module key = {.path = path};
    hashmap_entry_init(&key, strhash(path));
    Module *mod = hashmap_get(&modules, &key);
    if (mod == NULL) {
      // firstly load from memory
      mod = new_mod_from_mobject(ps->module, path);
      if (mod == NULL) {
        // NOTE: do not compile it from source, if its source exist.
        synerr(ps, s->import.pathrow, s->import.pathcol,
                    "no such module '%s'", path);
      }
    } else {
      debug("'%s' already imported", path);
    }

    if (!has_error(ps)) {
      debug("import from '%s' as '%s'", path, name);
      Symbol *sym = symbol_new(name, SYM_MOD);
      sym->mod.path = path;
      sym->desc = desc_from_klass("lang", "Module");
      if (stable_add_symbol(u->stbl, sym) < 0) {
        warn("symbol '%s' is duplicated", name);
      } else {
        sym->mod.ptr = mod;
        symbol_decref(sym);
      }
    }
  }
}

static void parse_constdecl(ParserState *ps, Stmt *stmt)
{
  Expr *exp = stmt->vardecl.exp;
  Ident *id = &stmt->vardecl.id;
  Type *type = &stmt->vardecl.type;
  TypeDesc *desc = type->desc;

  exp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, exp);
  if (exp->desc == NULL)
    return;

  if (desc == NULL)
    desc = exp->desc;

  if (desc == NULL)
    return;

  if (!desc_check(desc, exp->desc)) {
    if (!check_inherit(desc, exp->sym, NULL)) {
      STRBUF(sbuf1);
      STRBUF(sbuf2);
      desc_tostr(desc, &sbuf1);
      desc_tostr(exp->desc, &sbuf2);
      synerr(ps, exp->row, exp->col, "expected '%s', but found '%s'",
                   strbuf_tostr(&sbuf1), strbuf_tostr(&sbuf2));
      strbuf_fini(&sbuf1);
      strbuf_fini(&sbuf2);
    }
  }

  ParserUnit *u = ps->u;

  /* get const variable type symbol */
  Symbol *sym = stable_get(u->stbl, id->name);
  expect(sym != NULL);
  expect(sym->kind == SYM_CONST);

  if (sym->desc == NULL) {
    sym->desc = TYPE_INCREF(desc);
  }

  if (sym->var.typesym == NULL) {
    sym->var.typesym = get_desc_symbol(ps->module, sym->desc);
    if (type != NULL && sym->var.typesym == NULL) {
      STRBUF(sbuf);
      desc_tostr(sym->desc, &sbuf);
      synerr(ps, type->row, type->col,
                   "'%s' is not defined", strbuf_tostr(&sbuf));
      strbuf_fini(&sbuf);
    }
  }

  /* generate codes */
  if (exp != NULL && !has_error(ps)) {
    ScopeKind scope = u->scope;
    if (scope == SCOPE_MODULE) {
      CODE_OP(OP_LOAD_GLOBAL);
      CODE_OP_S(OP_SET_VALUE, id->name);
    } else {
      panic("bug? const can be only defined in module.");
    }
  }
}

void parse_subtype(ParserState *ps, Symbol *clssym, Vector *subtypes);

static void parse_vardecl(ParserState *ps, Stmt *stmt)
{
  Expr *exp = stmt->vardecl.exp;
  Ident *id = &stmt->vardecl.id;
  Type *type = &stmt->vardecl.type;
  TypeDesc *desc = type->desc;
  int dec = 0;

  // maybe update variable's type
  if (desc != NULL) {
    Symbol *sym = get_type_symbol(ps, desc);
    if (sym != NULL) {
      if (sym->kind == SYM_CLASS || sym->kind == SYM_TRAIT) {
        parse_subtype(ps, sym, desc->klass.typeargs);
      } else if (sym->kind == SYM_PTYPE) {
        desc = desc_from_pararef(sym->name, sym->paratype.index);
        dec = 1;
      }
    }
  }

  TypeDesc *rdesc = NULL;
  if (exp != NULL) {
    exp->ctx = EXPR_LOAD;
    parser_visit_expr(ps, exp);
    rdesc = exp->desc;
    if (rdesc == NULL) {
      return;
    }
  }

  if (desc != NULL) {
    if (rdesc && rdesc->kind == TYPE_LABEL) {
      rdesc = rdesc->label.edesc;
      debug("update expr's type as its enum '%s'", rdesc->klass.type);
    }

    if (rdesc && !desc_isnull(rdesc) && !desc_check(desc, rdesc)) {
      if (!check_inherit(desc, exp->sym, rdesc)) {
        STRBUF(sbuf1);
        STRBUF(sbuf2);
        desc_tostr(desc, &sbuf1);
        desc_tostr(exp->desc, &sbuf2);
        synerr(ps, exp->row, exp->col, "expected '%s', but found '%s'",
                    strbuf_tostr(&sbuf1), strbuf_tostr(&sbuf2));
        strbuf_fini(&sbuf1);
        strbuf_fini(&sbuf2);
      }
      // } else {
      //   //check subtype
      //   if (!check_subdesc(desc, rdesc)) {
      //     STRBUF(sbuf1);
      //     STRBUF(sbuf2);
      //     desc_tostr(desc, &sbuf1);
      //     desc_tostr(exp->desc, &sbuf2);
      //     synerr(ps, exp->row, exp->col, "expected '%s', but found '%s'",
      //                 strbuf_tostr(&sbuf1), strbuf_tostr(&sbuf2));
      //     strbuf_fini(&sbuf1);
      //     strbuf_fini(&sbuf2);
      //   }
      // }
    }
  } else {
    if (rdesc && desc_isnull(rdesc)) {
      synerr(ps, id->row, id->col, "unknown type of var '%s'", id->name);
      return;
    } else {
      desc = exp->desc;
    }
  }

  /* add or update variable type symbol */
  Symbol *sym = new_update_var(ps, id, desc);
  if (dec) TYPE_DECREF(desc);
  if (sym == NULL) return;

  if (sym->var.typesym == NULL) {
    STRBUF(sbuf);
    desc_tostr(type->desc, &sbuf);
    synerr(ps, type->row, type->col,
                 "'%s' is not defined", strbuf_tostr(&sbuf));
    strbuf_fini(&sbuf);
  }

  /* generate codes */
  if (exp != NULL && !has_error(ps)) {
    ScopeKind scope = ps->u->scope;
    if (scope == SCOPE_MODULE) {
      CODE_OP(OP_LOAD_GLOBAL);
      CODE_OP_S(OP_SET_VALUE, id->name);
    } else if (scope == SCOPE_CLASS) {
      CODE_LOAD(0);
      CODE_OP_S(OP_SET_VALUE, id->name);
    } else {
      /* others are local variables */
      CODE_STORE(sym->var.index);
    }
  }
}

static void parse_simple_assign(ParserState *ps, Stmt *stmt)
{
  Expr *rexp = stmt->assign.rexp;
  Expr *lexp = stmt->assign.lexp;

  // enum auto detect, how?

  rexp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, rexp);

  if (lexp->kind == TUPLE_KIND) {
    if (!has_error(ps) && desc_istuple(rexp->desc)) {
      // unpack tuple
      CODE_OP(OP_UNPACK_TUPLE);
    }
  }

  lexp->ctx = EXPR_STORE;
  parser_visit_expr(ps, lexp);

  Symbol *sym = lexp->sym;
  if (sym == NULL)
    return;
  if (sym->kind == SYM_CONST) {
    synerr(ps, rexp->row, rexp->col, "cannot assign to '%s'", sym->name);
    return;
  }

  if (has_error(ps))
    return;

  if (lexp->desc == NULL) {
    synerr(ps, lexp->row, lexp->col,
      "cannot resolve left expr's type");
  }

  if (rexp->desc == NULL) {
    synerr(ps, rexp->row, rexp->col,
      "right expr's type is void");
  }

  // check type is compatible
  TypeDesc *ldesc = lexp->desc;
  if (ldesc == NULL) {
    synerr(ps, lexp->row, lexp->col, "cannot resolve left expr");
    return;
  }

  TypeDesc *rdesc = rexp->desc;
  if (rdesc == NULL) {
    synerr(ps, rexp->row, rexp->col, "cannot resolve right expr");
    return;
  }

  if (ldesc->kind == TYPE_LABEL) {
    synerr(ps, lexp->row, lexp->col, "left expr is not settable");
    return;
  }

  if (rdesc->kind == TYPE_LABEL) {
    rdesc = rdesc->label.edesc;
    debug("right expr is enum '%s' label '%s'",
          rdesc->klass.type, rexp->sym->name);
  }

  if (desc_isnull(rdesc)) {
    debug("rigth expr is null");
    return;
  }

  if (!desc_check(ldesc, rdesc)) {
    if (!check_inherit(ldesc, rexp->sym, NULL)) {
      STRBUF(sbuf1);
      STRBUF(sbuf2);
      desc_tostr(ldesc, &sbuf1);
      desc_tostr(rdesc, &sbuf2);
      synerr(ps, rexp->row, rexp->col, "expected '%s', but found '%s'",
                   strbuf_tostr(&sbuf1), strbuf_tostr(&sbuf2));
      strbuf_fini(&sbuf1);
      strbuf_fini(&sbuf2);
    }
  }
}

static void parser_inplace_assign(ParserState *ps, Stmt *stmt)
{
  Expr *rexp = stmt->assign.rexp;
  Expr *lexp = stmt->assign.lexp;

  lexp->ctx = EXPR_INPLACE;
  lexp->inplace = stmt;
  switch (lexp->kind) {
  case ATTRIBUTE_KIND:
  case SUBSCRIPT_KIND:
  case ID_KIND:
    parser_visit_expr(ps, lexp);
    break;
  default:
    synerr(ps, lexp->row, lexp->col, "invalid inplace assignment");
    break;
  }
}

static void parse_assign(ParserState *ps, Stmt *stmt)
{
  AssignOpKind op = stmt->assign.op;
  if (op == OP_ASSIGN) {
    // simple assignment
    parse_simple_assign(ps, stmt);
  } else {
    // inplace assignment
    parser_inplace_assign(ps, stmt);
  }
}

TypeDesc *parse_proto(Vector *idtypes, Type *ret)
{
  Vector *vec = NULL;
  if (vector_size(idtypes) > 0)
    vec = vector_new();
  IdType *item;
  vector_for_each(item, idtypes) {
    vector_push_back(vec, TYPE_INCREF(item->type.desc));
  }
  return desc_from_proto(vec, ret->desc);
}

static int lro_exist(Vector *vec, Symbol *sym)
{
  Symbol *item;
  vector_for_each(item, vec) {
    if (desc_isany(item->desc))
      continue;
    if (desc_check(item->desc, sym->desc))
      return 1;
  }
  return 0;
}

static void lro_add(Symbol *base, Vector *lro, char *symname)
{
  Vector *lrovec = &base->type.lro;
  if (base->kind == SYM_TYPEREF) {
    lrovec = &base->typeref.sym->type.lro;
  }

  Symbol *sym;
  vector_for_each(sym, lrovec) {
    if (desc_isany(sym->desc))
      continue;
    if (!lro_exist(lro, sym)) {
      debug("add sym '%s' into '%s' lro", sym->name, symname);
      vector_push_back(lro, sym);
      ++sym->refcnt;
    }
  }

  if (!desc_isany(base->desc) && !lro_exist(lro, base)) {
    debug("add sym '%s' into '%s' lro", base->name, symname);
    vector_push_back(lro, base);
    ++base->refcnt;
  }
}

static void lro_show(Symbol *sym)
{
  debug("------lro of '%s'------", sym->name);
  Symbol *s;
  vector_for_each(s, &sym->type.lro) {
    STRBUF(sbuf);
    desc_tostr(s->desc, &sbuf);
    debug("%s", strbuf_tostr(&sbuf));
    strbuf_fini(&sbuf);
  }
  debug("------------------------");
}

TypeDesc *parse_func_proto(ParserState *ps, Vector *idtypes, Type *ret)
{
  TypeDesc *proto;
  TypeDesc *desc;
  Symbol *sym;
  Vector *vec = NULL;
  if (vector_size(idtypes) > 0)
    vec = vector_new();

  IdType *item;
  vector_for_each(item, idtypes) {
    desc = item->type.desc;
    sym = get_type_symbol(ps, desc);
    if (sym == NULL) {
      STRBUF(sbuf);
      desc_tostr(desc, &sbuf);
      synerr(ps, item->type.row, item->type.col,
            "'%s' is not defined", strbuf_tostr(&sbuf));
      strbuf_fini(&sbuf);
      goto error_exit;
    } else {
      if (sym->kind == SYM_CLASS || sym->kind == SYM_TRAIT) {
        parse_subtype(ps, sym, desc->klass.typeargs);
        TYPE_INCREF(desc);
      } else if (sym->kind == SYM_PTYPE) {
        desc = desc_from_pararef(sym->name, sym->paratype.index);
      }
      vector_push_back(vec, desc);
    }
  }

  desc = ret->desc;
  int dec = 0;
  if (desc != NULL) {
    sym = get_type_symbol(ps, desc);
    if (sym == NULL) {
      STRBUF(sbuf);
      desc_tostr(desc, &sbuf);
      synerr(ps, ret->row, ret->col,
            "'%s' is not defined", strbuf_tostr(&sbuf));
      strbuf_fini(&sbuf);
      goto error_exit;
    } else {
      if (sym->kind == SYM_CLASS || sym->kind == SYM_TRAIT) {
        parse_subtype(ps, sym, desc->klass.typeargs);
      } else if (sym->kind == SYM_PTYPE) {
        desc = desc_from_pararef(sym->name, sym->paratype.index);
        dec = 1;
      }
    }
  }

  proto = desc_from_proto(vec, desc);
  if (dec) TYPE_DECREF(desc);
  return proto;

error_exit:
  vector_for_each(desc, vec) {
    TYPE_DECREF(desc);
  }
  vector_free(vec);
  return NULL;
}

void parse_typepara_decl(ParserState *ps, Vector *typeparas);

static void parse_funcdecl(ParserState *ps, Stmt *stmt)
{
  char *funcname = stmt->funcdecl.id.name;
  debug("parse function '%s'", funcname);

  /* get func symbol */
  Symbol *sym = stable_get(ps->u->stbl, funcname);
  expect(sym != NULL && sym->kind == SYM_FUNC);

  parser_enter_scope(ps, SCOPE_FUNC, 0);
  ParserUnit *u = ps->u;
  u->stbl = stable_new();
  u->sym = sym;

  parse_typepara_decl(ps, stmt->funcdecl.typeparas);

  /* parse func's proto */
  Vector *idtypes = stmt->funcdecl.idtypes;
  Type *ret = &stmt->funcdecl.ret;
  sym->desc = parse_func_proto(ps, idtypes, ret);
  if (sym->desc == NULL)
    goto exit_label;

  /* parse func arguments */
  Vector *argtypes = sym->desc->proto.args;
  IdType *item;
  vector_for_each(item, idtypes) {
    Symbol *sym2 = new_update_var(ps, &item->id, vector_get(argtypes, idx));
    if (sym2 != NULL && sym2->var.typesym == NULL) {
      STRBUF(sbuf);
      desc_tostr(item->type.desc, &sbuf);
      synerr(ps, item->type.row, item->type.col,
                  "'%s' is not defined", strbuf_tostr(&sbuf));
      strbuf_fini(&sbuf);
    }
  }

  // check return type
  TypeDesc *rettype = sym->desc->proto.ret;
  if (rettype != NULL) {
    if (!strcmp(funcname, "__init__")) {
      synerr(ps, ret->row, ret->col, "__init__ needs no value");
    } else {
      if (rettype->kind == TYPE_PARADEF) {
        Symbol *parasym = stable_get(u->stbl, rettype->paradef.name);
        expect(parasym != NULL && parasym->kind == SYM_PTYPE);
        debug("ret type '%s' is generic", parasym->name);
      } else {
        sym = get_type_symbol(ps, rettype);
        if (sym == NULL) {
          STRBUF(sbuf);
          desc_tostr(rettype, &sbuf);
          synerr(ps, ret->row, ret->col,
                      "'%s' is not defined", strbuf_tostr(&sbuf));
          strbuf_fini(&sbuf);
        }
      }
    }
  }

  Type ret2 = {rettype, ret->row, ret->col};
  parse_body(ps, funcname, stmt->funcdecl.body, ret2);

exit_label:
  stable_free(u->stbl);
  u->stbl = NULL;
  parser_exit_scope(ps);
  debug("end of function '%s'", funcname);
}

static ParserUnit *get_func_scope(ParserState *ps)
{
  ParserUnit *u = ps->u;
  if (u->scope == SCOPE_FUNC || u->scope == SCOPE_ANONY)
    return u;

  vector_for_each_reverse(u, &ps->ustack) {
    if (u->scope == SCOPE_FUNC || u->scope == SCOPE_ANONY)
      return u;
  }

  return NULL;
}

static int inloop(ParserState *ps)
{
  ParserUnit *u = ps->u;
  if (u->scope == SCOPE_BLOCK &&
      (u->blocktype == WHILE_BLOCK || u->blocktype == FOR_BLOCK))
    return 1;

  vector_for_each_reverse(u, &ps->ustack) {
    if (u->scope == SCOPE_BLOCK &&
        (u->blocktype == WHILE_BLOCK || u->blocktype == FOR_BLOCK))
      return 1;
  }

  return 0;
}

static void parse_return(ParserState *ps, Stmt *stmt)
{
  ParserUnit *fu = get_func_scope(ps);
  if (fu == NULL) {
    synerr(ps, stmt->row, stmt->col, "'return' outside function");
    return;
  }

  Expr *exp = stmt->ret.exp;
  if (exp != NULL) {
    debug("return has value");
    exp->ctx = EXPR_LOAD;
    parser_visit_expr(ps, exp);

    if (exp->desc == NULL) {
      synerr(ps, exp->row, exp->col, "expr has no value");
      return;
    }

    TypeDesc *desc = fu->sym->desc;
    expect(desc != NULL);
    TypeDesc *ret = desc->proto.ret;
    if (!desc_isnull(exp->desc) && !desc_check(ret, exp->desc) &&
        !check_inherit(ret, exp->sym, NULL)) {
      synerr(ps, exp->row, exp->col, "incompatible return types");
    }

    if (!has_error(ps)) {
      stmt->hasvalue = 1;
      stmt->desc = TYPE_INCREF(exp->desc);
      CODE_OP(OP_RETURN_VALUE);
    }

    return;
  }

  debug("return has no value");
  CODE_OP(OP_RETURN);
}

static void parse_break(ParserState *ps, Stmt *stmt)
{
  if (!inloop(ps)) {
    synerr(ps, stmt->row, stmt->col, "'break' outside loop");
  }

  Inst *jmp = CODE_OP(OP_JMP);
  jmp->offset = codeblock_bytes(ps->u->block);
  jmp->jmpdown = 1;
  parser_add_jmp(ps, jmp);
}

static void parse_continue(ParserState *ps, Stmt *stmt)
{
  if (!inloop(ps)) {
    synerr(ps, stmt->row, stmt->col, "'continue' outside loop");
  }

  Inst *jmp = CODE_OP(OP_JMP);
  jmp->offset = codeblock_bytes(ps->u->block);
  jmp->jmpdown = 0;
  parser_add_jmp(ps, jmp);
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
    stmt->desc = TYPE_INCREF(exp->desc);
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
  parser_enter_scope(ps, SCOPE_BLOCK, ONLY_BLOCK);
  ParserUnit *u = ps->u;
  u->stbl = stable_new();

  Stmt *s;
  Vector *vec = stmt->block.vec;
  vector_for_each(s, vec) {
    parse_stmt(ps, s);
  }

  stable_free(u->stbl);
  u->stbl = NULL;
  parser_exit_scope(ps);
}

static void parse_if(ParserState *ps, Stmt *stmt)
{
  parser_enter_scope(ps, SCOPE_BLOCK, IF_BLOCK);
  ParserUnit *u = ps->u;
  u->stbl = stable_new();
  Expr *test = stmt->if_stmt.test;
  Vector *block = stmt->if_stmt.block;
  Stmt *orelse = stmt->if_stmt.orelse;
  Inst *jmp = NULL;
  Inst *jmp2 = NULL;
  int offset = 0;
  int offset2 = 0;

  if (test != NULL) {
    test->ctx = EXPR_LOAD;
    // optimize binary compare operator
    parser_visit_expr(ps, test);
    if (!desc_isbool(test->desc)) {
      synerr(ps, test->row, test->col, "expected bool type");
    }
    jmp = CODE_OP(OP_JMP_FALSE);
    offset = codeblock_bytes(u->block);
  }

  if (block != NULL) {
    Stmt *s;
    vector_for_each(s, block) {
      parse_stmt(ps, s);
    }
    if (orelse != NULL) {
      jmp2 = CODE_OP(OP_JMP);
      offset2 = codeblock_bytes(u->block);
    }
  }

  if (jmp != NULL) {
    offset = codeblock_bytes(u->block) - offset;
    jmp->offset = offset;
  }

  if (orelse != NULL) {
    parse_stmt(ps, orelse);
  }

  if (jmp2 != NULL) {
    offset2 = codeblock_bytes(u->block) - offset2;
    jmp2->offset = offset2;
  }

  stable_free(u->stbl);
  u->stbl = NULL;
  parser_exit_scope(ps);
}

static void parse_while(ParserState *ps, Stmt *stmt)
{
  parser_enter_scope(ps, SCOPE_BLOCK, WHILE_BLOCK);
  ParserUnit *u = ps->u;
  Expr *test = stmt->while_stmt.test;
  Vector *block = stmt->while_stmt.block;
  Inst *jmp = NULL;
  int offset = 0;

  if (test != NULL) {
    test->ctx = EXPR_LOAD;
    // optimize binary compare operator
    parser_visit_expr(ps, test);
    if (!desc_isbool(test->desc)) {
      synerr(ps, test->row, test->col, "expected bool type");
    }
    jmp = CODE_OP(OP_JMP_FALSE);
    offset = codeblock_bytes(u->block);
  }

  if (block != NULL) {
    Stmt *s;
    vector_for_each(s, block) {
      parse_stmt(ps, s);
    }
  }

  Inst *jmp2 = CODE_OP(OP_JMP);
  jmp2->offset = 0 - codeblock_bytes(u->block);

  if (jmp != NULL) {
    offset = codeblock_bytes(u->block) - offset;
    jmp->offset = offset;
  }

  parser_handle_jmps(ps, 0);

  parser_exit_scope(ps);
}

static void parse_for(ParserState *ps, Stmt *stmt)
{
  parser_enter_scope(ps, SCOPE_BLOCK, FOR_BLOCK);
  ParserUnit *u = ps->u;
  u->stbl = stable_new();
  Expr *vexp = stmt->for_stmt.vexp;
  Expr *iter = stmt->for_stmt.iter;
  Expr *step = stmt->for_stmt.step;
  Vector *block = stmt->for_stmt.block;
  Symbol *sym;
  TypeDesc *desc = NULL;
  Inst *jmp = NULL;
  int offset = 0;

  iter->ctx = EXPR_LOAD;
  parser_visit_expr(ps, iter);
  sym = iter->sym;
  if (sym == NULL) {
    goto exit_label;
  }

  if (sym->kind == SYM_VAR) {
    sym = sym->var.typesym;
  } else if (sym->kind == SYM_CLASS) {
    expect(!strcmp(sym->name, "Range"));
  } else {
    panic("which kind of symbol? %d", sym->kind);
  }

  TypeDesc *pdesc = NULL;
  sym = type_find_mbr(sym, "__iter__", &pdesc);
  if (sym == NULL) {
    synerr(ps, iter->row, iter->col, "object is not iteratable.");
  } else {
    TypeDesc *tmp = sym->desc;
    expect(desc_isproto(tmp));
    desc = specialize_types(pdesc, iter->desc, tmp);
    TYPE_DECREF(pdesc);
    show_specialized_type(sym, desc);

    expect(desc_isproto(desc));
    desc = desc->proto.ret;
    expect(desc->kind == TYPE_KLASS);
    desc = vector_get(desc->klass.typeargs, 0);
    expect(desc != NULL);

    CODE_OP(OP_NEW_ITER);
    offset = codeblock_bytes(u->block);

    if (step != NULL) {
      step->ctx = EXPR_LOAD;
      parser_visit_expr(ps, step);
    } else {
      CODE_OP_I(OP_LOAD_CONST, 1);
    }
    jmp = CODE_OP(OP_FOR_ITER);
    jmp->offset = codeblock_bytes(u->block);
  }

  // if ident is not declared, declare it automatically.
  if (vexp->kind == ID_KIND) {
    sym = find_symbol_byname(ps, vexp->id.name);
    if (sym != NULL) {
      if (!has_error(ps) && !desc_check(sym->desc, desc)) {
        synerr(ps, vexp->row, vexp->col, "types are not matched");
      }
    } else {
      // create local variable
      Ident id = {vexp->id.name, vexp->row, vexp->col};
      new_update_var(ps, &id, desc);
    }
  } else if (vexp->kind == TUPLE_KIND) {
    panic("not implemented");
  } else {
    // fallthrough
    panic("why ? not implemented");
  }

  vexp->ctx = EXPR_STORE;
  parser_visit_expr(ps, vexp);
  if (!has_error(ps) && !desc_check(vexp->desc, desc)) {
    synerr(ps, vexp->row, vexp->col, "types are not matched");
  }

  TYPE_DECREF(desc);

  if (block != NULL) {
    Stmt *s;
    vector_for_each(s, block) {
      parse_stmt(ps, s);
    }
  }

  Inst *jmp2 = CODE_OP(OP_JMP);
  jmp2->offset = offset - codeblock_bytes(u->block);

  parser_handle_jmps(ps, offset);

  if (jmp != NULL) {
    jmp->offset = codeblock_bytes(u->block) - jmp->offset;
  }

  //pop iterator
  CODE_OP(OP_POP_TOP);

exit_label:
  stable_free(u->stbl);
  u->stbl = NULL;
  parser_exit_scope(ps);
}

static void parse_ifunc(ParserState *ps, Stmt *stmt)
{
  Ident *id = &stmt->funcdecl.id;
  debug("parse ifunc '%s'", id->name);
  Symbol *sym = stable_get(ps->u->stbl, id->name);
  expect(sym != NULL && sym->kind == SYM_IFUNC);
}

static void parse_class_extends(ParserState *ps, Symbol *clssym, Stmt *stmt)
{
  ExtendsDef *ext = stmt->class_stmt.extends;
  if (ext == NULL)
    return;

  // parse base class
  Symbol *sym;
  Type *base = &ext->type;
  sym = get_desc_symbol(ps->module, base->desc);
  if (sym == NULL) {
    STRBUF(sbuf);
    desc_tostr(base->desc, &sbuf);
    synerr(ps, base->row, base->col,
                 "'%s' is not defined", strbuf_tostr(&sbuf));
    strbuf_fini(&sbuf);
  } else if (sym->kind != SYM_CLASS && sym->kind != SYM_TRAIT) {
    synerr(ps, base->row, base->col,
                 "'%s' is not class/trait", sym->name);
  } else {
    TypeDesc *desc = base->desc;
    parse_subtype(ps, sym, desc->klass.typeargs);
    if (!has_error(ps)) {
      if (vector_size(desc->klass.typeargs) > 0) {
        Symbol *refsym = new_typeref_symbol(desc, sym);
        if (sym->kind == SYM_CLASS) {
          debug("add typeref symbol '%s'", refsym->name);
          clssym->type.base = refsym;
        } else {
          vector_push_back(&clssym->type.traits, refsym);
        }
      } else {
        ++sym->refcnt;
        if (sym->kind == SYM_CLASS) {
          clssym->type.base = sym;
        } else {
          vector_push_back(&clssym->type.traits, sym);
        }
      }
    }
  }

  // parse traits
  Type *trait;
  vector_for_each(trait, ext->withes) {
    sym = get_desc_symbol(ps->module, trait->desc);
    if (sym == NULL) {
      STRBUF(sbuf);
      desc_tostr(trait->desc, &sbuf);
      synerr(ps, trait->row, trait->col,
                   "'%s' is not defined", strbuf_tostr(&sbuf));
      strbuf_fini(&sbuf);
    } else if (sym->kind != SYM_TRAIT) {
      synerr(ps, trait->row, trait->col, "'%s' is not trait", sym->name);
    } else {
      TypeDesc *desc = trait->desc;
      parse_subtype(ps, sym, desc->klass.typeargs);
      if (!has_error(ps)) {
        if (vector_size(desc->klass.typeargs) > 0) {
          Symbol *refsym = new_typeref_symbol(desc, sym);
          debug("add typeref symbol '%s'", refsym->name);
          vector_push_back(&clssym->type.traits, refsym);
        } else {
          ++sym->refcnt;
          vector_push_back(&clssym->type.traits, sym);
        }
      }
    }
  }

  // build lro
  sym = clssym->type.base;
  Vector *lro = &clssym->type.lro;
  char *symname = clssym->name;
  if (sym != NULL) {
    lro_add(sym, lro, symname);
  }

  vector_for_each(sym, &clssym->type.traits) {
    lro_add(sym, lro, symname);
  }

  // show lro
  lro_show(clssym);
}

static void addcode_supercall_noargs(CodeBlock **old)
{
  CodeBlock *block = codeblock_new();
  Inst *inst = inst_new(OP_LOAD_0, NULL, NULL);
  codeblock_add_inst(block, inst);
  inst = inst_new(OP_SUPER_INIT_CALL, NULL, NULL);
  inst->argc = 0;
  codeblock_add_inst(block, inst);
  codeblock_merge(*old, block);
  codeblock_free(*old);
  *old = block;
}

static
void check_one_proto_impl(ParserState *ps, Symbol *s, int idx, Symbol *cls)
{
  Symbol *ret;
  Symbol *sub;
  int size = vector_size(&cls->type.lro);
  for (int i = idx; i < size; ++i) {
    sub = vector_get(&cls->type.lro, i);
    ret = stable_get(sub->type.stbl, s->name);
    if (ret != NULL && ret->kind == SYM_FUNC) {
      return;
    }
  }

  sub = cls;
  ret = stable_get(sub->type.stbl, s->name);
  if (ret != NULL && ret->kind == SYM_FUNC) {
    return;
  }

  synerr(ps, 0, 0, "'%s' is not implemented", s->name);
}

static void check_trait_ifunc_impl(int idx, Symbol *item,
                                  Symbol *clssym, ParserState *ps)
{
  HASHMAP_ITERATOR(iter, &item->type.stbl->table);
  Symbol *s;
  iter_for_each(&iter, s) {
    if (s->kind == SYM_IFUNC)
      check_one_proto_impl(ps, s, idx, clssym);
    else
      expect(s->kind == SYM_FUNC);
  }
}

static void check_proto_impl(ParserState *ps)
{
  Symbol *sym = ps->u->sym;
  expect(sym->kind == SYM_CLASS);

  Symbol *item;
  vector_for_each(item, &sym->type.lro) {
    if (idx == 0) {
      expect(!strcmp(item->name, "Any"));
      continue;
    }

    if (item->kind == SYM_CLASS) {
      //no check
    } else if (item->kind == SYM_TRAIT) {
      check_trait_ifunc_impl(idx, item, sym, ps);
    } else {
      expect(item->kind == SYM_TYPEREF);
      item = item->typeref.sym;
      if (item->kind == SYM_TRAIT) {
        check_trait_ifunc_impl(idx, item, sym, ps);
      }
    }
  }
}

Symbol *get_type_symbol(ParserState *ps, TypeDesc *type)
{
  if (type == NULL) {
    return NULL;
  }

  if (type->kind == TYPE_PARAREF) {
    return find_symbol_byname(ps, type->pararef.name);
  }

  ParserUnit *u = ps->u;
  if (type->kind != TYPE_KLASS) {
    return get_desc_symbol(ps->module, type);
  }

  char *path = type->klass.path;
  char *name = type->klass.type;
  Symbol *sym = NULL;

  if (path == NULL) {
    // check it is a type parameter or not
    sym = find_symbol_byname(ps, name);
    if (sym != NULL) {
      if (sym->kind == SYM_PTYPE) {
        debug("'%s' is type parameter", name);
      } else {
        expect(sym->kind == SYM_CLASS || sym->kind == SYM_TRAIT);
        debug("'%s' is a klass", name);
      }
      return sym;
    }
  }

  // not a type parameter, check it is a class/trait or not
  sym = get_desc_symbol(ps->module, type);
  if (sym == NULL) {
    synerr(ps, 0, 0, "'%s' is not defined", name);
    return NULL;
  }

  if (sym->kind != SYM_CLASS && sym->kind != SYM_TRAIT) {
    synerr(ps, 0, 0, "'%s' is not a class/trait", name);
    return NULL;
  }

  debug("'%s' is klass", name);
  return sym;
}

int match_subtype(Symbol *subtype, Symbol *type)
{
  // self is self's subtype
  if (subtype == type)
    return 1;

  // check bases class and traits
  Symbol *item;
  vector_for_each(item, &subtype->type.lro) {
    if (item == type)
      return 1;
  }

  // type is not subtype's parent
  return 0;
}

void tp_match_typepara(ParserState *ps, Symbol *tp1, Symbol *tp2)
{
  Symbol *item1;
  Symbol *item2;
  vector_for_each(item2, tp2->paratype.typesyms) {
    while (item2->kind == SYM_PTYPE) {
      item2 = get_type_symbol(ps, item2->desc);
    }

    item1 = vector_get(tp1->paratype.typesyms, idx);
    while (item1->kind == SYM_PTYPE) {
      item1 = get_type_symbol(ps, item1->desc);
    }

    debug("match '%s' is subtype of '%s'?", item1->name, item2->name);
    if (!match_subtype(item1, item2)) {
      synerr(ps, 0, 0, "'%s' is not subtype of '%s'",
                   item1->name, item2->name);
    }
  }
}

void kls_match_typepara(ParserState *ps, Symbol *kls, Symbol *tp)
{
  Symbol *item2;
  vector_for_each(item2, tp->paratype.typesyms) {
    while (item2->kind == SYM_PTYPE) {
      item2 = get_type_symbol(ps, item2->desc);
    }

    debug("match '%s' is subtype of '%s'?", kls->name, item2->name);
    if (!match_subtype(kls, item2)) {
      synerr(ps, 0, 0, "'%s' is not subtype of '%s'",
                   kls->name, item2->name);
    }
  }
}

void check_subtype(ParserState *ps, Symbol *tpsym, Symbol *sym)
{
  expect(tpsym->kind == SYM_PTYPE);

  debug("paratype '%s' <- typearg '%s'", tpsym->name, sym->name);

  // any type is ok for no bounds
  if (tpsym->paratype.typesyms == NULL) {
    debug("no bounds of paratype '%s' (it matches any)", tpsym->name);
    return;
  }

  if (sym->kind == SYM_PTYPE) {
    debug("'%s' is typepara", sym->name);
    tp_match_typepara(ps, sym, tpsym);
  } else if (sym->kind == SYM_CLASS || sym->kind == SYM_TRAIT) {
    debug("'%s' is klass", sym->name);
    kls_match_typepara(ps, sym, tpsym);
  } else {
    panic("invalid type %d in subtype", sym->kind);
  }
}

void parse_subtype(ParserState *ps, Symbol *clssym, Vector *subtypes)
{
  Symbol *subsym;
  Symbol *tpsym;
  TypeDesc *item;
  vector_for_each(item, subtypes) {
    subsym = get_type_symbol(ps, item);
    if (subsym == NULL)
      continue;

    tpsym = vector_get(clssym->type.typesyms, idx);
    if (tpsym == NULL) {
      synerr(ps, 0, 0, "symbol '%s' no typeparas", clssym->name);
      continue;
    }

    check_subtype(ps, tpsym, subsym);

    if (subsym->kind == SYM_CLASS || subsym->kind == SYM_TRAIT) {
      parse_subtype(ps, subsym, item->klass.typeargs);
    } else if (subsym->kind == SYM_PTYPE) {
      // update subtype's descriptor as type para ref
      TYPE_DECREF(item);
      item = desc_from_pararef(subsym->name, subsym->paratype.index);
      vector_set(subtypes, idx, item);
    }
  }
}

void parse_bound(ParserState *ps, Symbol *sym, Vector *bounds)
{
  Symbol *bndsym;
  TypeDesc *item;
  vector_for_each(item, bounds) {
    bndsym = get_type_symbol(ps, item);
    if (bndsym == NULL)
     return;

    if (idx > 0 && bndsym->kind == SYM_CLASS) {
      synerr(ps, 0, 0, " expect '%s' is a trait", item->klass.type);
      return;
    }

    if (bndsym->kind == SYM_PTYPE) {
      debug("bound '%s' is type parameter", bndsym->name);
      if (item->klass.typeargs != NULL) {
        synerr(ps, 0, 0, "'%s' no need type parameter", bndsym->name);
      }
      continue;
    }

    if (bndsym->type.typesyms == NULL) {
      debug("'%s' no typepara", bndsym->name);
      typepara_add_bound(sym, bndsym);
      if (item->klass.typeargs != NULL) {
        synerr(ps, 0, 0, "'%s' no need type parameter", bndsym->name);
      }
    } else {
      debug("new '%s' typeref", bndsym->name);
      Symbol *refsym = symbol_new(bndsym->name, SYM_TYPEREF);
      refsym->desc = TYPE_INCREF(item);
      refsym->typeref.sym = sym;
      typepara_add_bound(sym, refsym);
      parse_subtype(ps, bndsym, item->klass.typeargs);
    }
  }
}

void parse_typepara_decl(ParserState *ps, Vector *typeparas)
{
  ParserUnit *u = ps->u;
  char *name;
  Symbol *sym;
  TypeParaDef *item;
  vector_for_each(item, typeparas) {
    name = item->type.name;
    debug("parse typepara '%s'", name);
    sym = stable_add_typepara(u->stbl, name, idx);
    if (sym == NULL) {
      synerr(ps, 0, 0, "'%s' is redeclared", name);
      continue;
    }
    ++sym->refcnt;
    sym_add_typepara(u->sym, sym);
    parse_bound(ps, sym, item->bounds);
    debug("end of typepara '%s'", name);
  }
}

static void parse_class(ParserState *ps, Stmt *stmt)
{
  Ident *id = &stmt->class_stmt.id;
  debug("parse class '%s'", id->name);
  Symbol *sym = stable_get(ps->u->stbl, id->name);
  expect(sym != NULL);

  parser_enter_scope(ps, SCOPE_CLASS, 0);
  ps->u->sym = sym;
  ps->u->stbl = sym->type.stbl;

  parse_typepara_decl(ps, stmt->class_stmt.typeparas);

  parse_class_extends(ps, sym, stmt);

  /* parse class body */
  Vector *body = stmt->class_stmt.body;
  int sz = vector_size(body);
  Stmt *s = NULL;
  vector_for_each(s, body) {
    parse_stmt(ps, s);
  }

  Symbol *initsym = stable_get(ps->u->stbl, "__init__");
  Symbol *basesym = sym->type.base;
  Symbol *baseinitsym = NULL;
  int baseinitargc = 0;
  if (basesym != NULL) {
    baseinitsym = stable_get(basesym->type.stbl, "__init__");
  }
  if (baseinitsym != NULL) {
    expect(baseinitsym->desc->kind == TYPE_PROTO);
    baseinitargc = vector_size(baseinitsym->desc->proto.args);
  }

  if (initsym == NULL) {
    if (baseinitargc != 0) {
      synerr(ps, stmt->row, stmt->col, "require call super");
    } else {
      if (baseinitsym != NULL) {
        debug("create '__init__' and add code to call super()");
        TypeDesc *proto = desc_from_proto(NULL, NULL);
        initsym = stable_add_func(ps->u->stbl, "__init__", proto);
        TYPE_DECREF(proto);
        CodeBlock *code = NULL;
        addcode_supercall_noargs(&code);
        initsym->func.codeblock = code;
      }
    }
  } else {
    if (baseinitargc != 0) {
      if (!initsym->super) {
        synerr(ps, stmt->row, stmt->col, "require call super");
      }
    } else {
      if (baseinitsym != NULL && !initsym->super) {
        // auto call super() with no arguments
        debug("exist '__init__' and add code to call super()");
        CodeBlock *code = initsym->func.codeblock;
        addcode_supercall_noargs(&code);
        initsym->func.codeblock = code;
      }
    }
  }

  // check protos are all implemented.
  check_proto_impl(ps);

  parser_exit_scope(ps);

  debug("end of class '%s'", id->name);
}

static void parse_trait_extends(ParserState *ps, Symbol *traitsym, Stmt *stmt)
{
  ExtendsDef *ext = stmt->class_stmt.extends;
  if (ext == NULL)
    return;

  // parse base trait
  Symbol *sym;
  Type *base = &ext->type;
  sym = get_desc_symbol(ps->module, base->desc);
  if (sym == NULL) {
    STRBUF(sbuf);
    desc_tostr(base->desc, &sbuf);
    synerr(ps, base->row, base->col,
                 "'%s' is not defined", strbuf_tostr(&sbuf));
    strbuf_fini(&sbuf);
  } else if (sym->kind != SYM_TRAIT) {
    synerr(ps, base->row, base->col, "'%s' is not trait", sym->name);
  } else {
    vector_push_back(&traitsym->type.traits, sym);

    TypeDesc *desc = base->desc;
    parse_subtype(ps, sym, desc->klass.typeargs);
    if (!has_error(ps)) {
      if (vector_size(desc->klass.typeargs) > 0) {
        Symbol *refsym = new_typeref_symbol(desc, sym);
        debug("add typeref symbol '%s'", refsym->name);
        vector_push_back(&traitsym->type.traits, refsym);
      } else {
        ++sym->refcnt;
        vector_push_back(&traitsym->type.traits, sym);
      }
    }
  }

  // parse with traits
  Type *trait;
  vector_for_each(trait, ext->withes) {
    sym = get_desc_symbol(ps->module, trait->desc);
    if (sym == NULL) {
      STRBUF(sbuf);
      desc_tostr(trait->desc, &sbuf);
      synerr(ps, trait->row, trait->col,
                   "'%s' is not defined", strbuf_tostr(&sbuf));
      strbuf_fini(&sbuf);
    } else if (sym->kind != SYM_TRAIT) {
      synerr(ps, trait->row, trait->col, "'%s' is not trait", sym->name);
    } else {
      TypeDesc *desc = trait->desc;
      parse_subtype(ps, sym, desc->klass.typeargs);
      if (!has_error(ps)) {
        if (vector_size(desc->klass.typeargs) > 0) {
          Symbol *refsym = new_typeref_symbol(desc, sym);
          debug("add typeref symbol '%s'", refsym->name);
          vector_push_back(&traitsym->type.traits, refsym);
        } else {
          ++sym->refcnt;
          vector_push_back(&traitsym->type.traits, sym);
        }
      }
    }
  }

  // build lro
  Vector *lro = &traitsym->type.lro;
  char *symname = traitsym->name;
  vector_for_each(sym, &traitsym->type.traits) {
    lro_add(sym, lro, symname);
  }

  // show lro
  lro_show(traitsym);
}

static void parse_trait(ParserState *ps, Stmt *stmt)
{
  Ident *id = &stmt->class_stmt.id;
  debug("parse trait '%s'", id->name);
  Symbol *sym = stable_get(ps->u->stbl, id->name);
  expect(sym != NULL);

  parser_enter_scope(ps, SCOPE_CLASS, 0);
  ps->u->sym = sym;
  ps->u->stbl = sym->type.stbl;

  parse_typepara_decl(ps, stmt->class_stmt.typeparas);
  parse_trait_extends(ps, sym, stmt);

  /* parse class body */
  Vector *body = stmt->class_stmt.body;
  int sz = vector_size(body);
  Stmt *s = NULL;
  vector_for_each(s, body) {
    parse_stmt(ps, s);
  }

  Symbol *initsym = stable_get(ps->u->stbl, "__init__");
  if (initsym != NULL) {
    synerr(ps, stmt->row, stmt->col,
                 "'__init__' is not allowed in trait");
  }

  parser_exit_scope(ps);

  debug("end of trait '%s'", id->name);
}

static void parse_enum(ParserState *ps, Stmt *stmt)
{
  Ident *id = &stmt->enum_stmt.id;
  debug("parse enum '%s'", id->name);

  Symbol *sym = stable_get(ps->u->stbl, id->name);
  expect(sym != NULL);

  parser_enter_scope(ps, SCOPE_CLASS, 0);
  ps->u->sym = sym;
  ps->u->stbl = sym->type.stbl;

  /* parse enum methods */
  Vector *methods = stmt->enum_stmt.mbrs.methods;
  Stmt *s = NULL;
  vector_for_each(s, methods) {
    parse_funcdecl(ps, s);
  }

  parser_exit_scope(ps);

  debug("end of enum '%s'", id->name);
}

static void parse_match_clause(ParserState *ps, MatchClause *clause)
{
  parser_enter_scope(ps, SCOPE_BLOCK, MATCH_CLAUSE);
  ParserUnit *u = ps->u;
  if (clause->stbl != NULL) {
    u->stbl = clause->stbl;
    clause->stbl = NULL;
  } else {
    u->stbl = stable_new();
  }

  HASHMAP_ITERATOR(iter, &u->stbl->table);
  Symbol *sym;
  iter_for_each(&iter, sym) {
    expect(sym->kind == SYM_VAR);
    CODE_OP(OP_DUP);
    CODE_OP_ARGC(OP_DOT_INDEX, sym->var.dotindex);
    CODE_STORE(sym->var.index);
  }

  Stmt *s = clause->block;
  if (s->kind == EXPR_KIND) {
    parse_expr(ps, s);
  } else {
    expect(s->kind == BLOCK_KIND);
    Stmt *item;
    Vector *vec = s->block.vec;
    vector_for_each(item, vec) {
      parse_stmt(ps, item);
    }
  }

  stable_free(u->stbl);
  u->stbl = NULL;
  parser_exit_scope(ps);
}

static void parse_match(ParserState *ps, Stmt *stmt)
{
  debug("parse match");
  Expr *exp = stmt->match_stmt.exp;
  exp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, exp);
  /*
  Symbol *sym = type_find_mbr(exp->sym, "__eq__");
  if (sym == NULL) {
    synerr(ps, exp->row, exp->col, "unsupported match operation");
  }
  */

  parser_enter_scope(ps, SCOPE_BLOCK, MATCH_BLOCK);
  ParserUnit *u = ps->u;

  Vector *clauses = stmt->match_stmt.clauses;
  int count = vector_size(clauses);
  MatchClause *match;
  MatchClause *underscore = NULL;
  int islast = 0;
  Inst *matchjmps[count];
  Inst *blockjmps[count];
  Inst *underjmp = NULL;
  int matchoffset[count];
  int blockoffset[count];
  int blockjmpoffset[count];
  int matchindex = 0;
  int blockindex = 0;
  int underoffset = 0;
  int blockjmpindex = 0;

  // parse conditons
  vector_for_each(match, clauses) {
    Expr *pattern = match->pattern;
    TypeDesc *patterndesc = NULL;
    if (pattern->kind == UNDER_KIND) {
      if (underscore != NULL) {
        synerr(ps, pattern->row, pattern->col,
                    "duplicated underscore(_)");
      }
      underscore = match;
      if (idx == count - 1) {
        islast = 1;
      }
      continue;
    }

    if (pattern->kind == ENUM_PATTERN_KIND) {
      parser_enter_scope(ps, SCOPE_BLOCK, MATCH_PATTERN);
      ps->u->stbl = stable_new();
    }

    pattern->ctx = EXPR_LOAD;
    parser_visit_expr(ps, pattern);

    if (pattern->kind == ENUM_PATTERN_KIND) {
      if (stable_size(ps->u->stbl) > 0) {
        // save symbol table for parsing match clauses.
        match->stbl = ps->u->stbl;
      } else {
        match->stbl = NULL;
        stable_free(ps->u->stbl);
      }
      ps->u->stbl = NULL;
      parser_exit_scope(ps);
    }

    if (pattern->kind == ARRAY_KIND) {
      debug("match pattern is array");
      Expr *subexp = vector_get(pattern->array, 0);
      if (subexp == NULL) {
        synerr(ps, pattern->row, pattern->col,
                    "cannot resolve array's subtype");
      } else {
        patterndesc = subexp->desc;
      }
      pattern->enum_pattern.argc = 1;
    } else if (pattern->kind == RANGE_KIND) {
      debug("match pattern is range");
      patterndesc = pattern->range.start->desc;
      pattern->enum_pattern.argc = 1;
    } else if (pattern->kind == IS_KIND) {
      debug("match pattern is IS TYPE");
      patterndesc = pattern->isas.type.desc;
      pattern->enum_pattern.argc = 1;
    } else if (pattern->kind == ENUM_PATTERN_KIND) {
      debug("match pattern is enum");
      patterndesc = pattern->desc;
      //for enum label name
      ++pattern->enum_pattern.argc;
      CODE_OP_S(OP_LOAD_CONST, pattern->enum_pattern.id.name);
    } else {
      // single literal
      expect(pattern->kind == LITERAL_KIND);
      patterndesc = pattern->desc;
      pattern->enum_pattern.argc = 1;
    }

    if (patterndesc == NULL) {
      synerr(ps, pattern->row, pattern->col, "cannot resolve test type");
    } else if (!desc_check(patterndesc, exp->desc)) {
      synerr(ps, pattern->row, pattern->col, "types are not matched");
    } else {
      CODE_OP_ARGC(OP_MATCH, pattern->enum_pattern.argc);
      Inst *jmp = CODE_OP(OP_JMP_TRUE); //where to jmp?
      matchjmps[matchindex] = jmp;
      matchoffset[matchindex] = codeblock_bytes(u->block);
      ++matchindex;
    }
  }

  // parse underscore(_) pattern
  if (underscore != NULL) {
    int understart = codeblock_bytes(u->block);
    parse_match_clause(ps, underscore);
    if (vector_size(clauses) > 1) {
      Inst *jmp = CODE_OP(OP_JMP); //where to jmp?
      underjmp = jmp;
      underoffset = codeblock_bytes(u->block);
    }
  }

  // parse each body
  vector_for_each(match, clauses) {
    int blockstart = codeblock_bytes(u->block);
    Inst *jmp = NULL;
    if (match == underscore) {
      continue;
    }

    parse_match_clause(ps, match);

    if (islast) {
      // last one is underscore clause
      if (idx < count - 2) {
        // last second clause not need jmp
        jmp = CODE_OP(OP_JMP); //where to jmp?
      }
    } else {
      // last one is not underscore clause
      if (idx < count - 1) {
        // last one clause not need jmp
        jmp = CODE_OP(OP_JMP); //where to jmp?
      }
    }

    if (jmp != NULL) {
      blockjmps[blockjmpindex] = jmp;
      blockjmpoffset[blockjmpindex] = codeblock_bytes(u->block);
      ++blockjmpindex;
    }
    blockoffset[blockindex] = blockstart;
    ++blockindex;
  }

  // handle jumps' offset
  if (!has_error(ps)) {
    expect(matchindex == blockindex);
    int totalsize = codeblock_bytes(u->block);
    int matchsize = matchoffset[matchindex - 1];

    for (int i = 0; i < matchindex; ++i) {
      matchjmps[i]->offset = blockoffset[i] - matchoffset[i];
    }

    if (underjmp != NULL) {
      underjmp->offset = totalsize - underoffset;
    }

    for (int i = 0; i < blockjmpindex; ++i) {
      blockjmps[i]->offset = totalsize - blockjmpoffset[i];
    }
  }

  parser_exit_scope(ps);

  // pop stmt->match_stmt.exp
  CODE_OP(OP_POP_TOP);

  debug("end of match");
}

void parse_stmt(ParserState *ps, Stmt *stmt)
{
  /* if errors is greater than MAX_ERRORS, stop parsing */
  if (ps->errors >= MAX_ERRORS)
    return;

  static void (*handlers[])(ParserState *, Stmt *) = {
    NULL,               /* INVALID        */
    parse_import,       /* IMPORT_KIND    */
    parse_constdecl,    /* CONST_KIND     */
    parse_vardecl,      /* VAR_KIND       */
    parse_assign,       /* ASSIGN_KIND    */
    parse_funcdecl,     /* FUNC_KIND      */
    parse_return,       /* RETURN_KIND    */
    parse_expr,         /* EXPR_KIND      */
    parse_block,        /* BLOCK_KIND     */
    parse_ifunc,        /* IFUNC_KIND     */
    parse_class,        /* CLASS_KIND     */
    parse_trait,        /* TRAIT_KIND     */
    parse_enum,         /* ENUM_KIND      */
    parse_break,        /* BREAK_KIND     */
    parse_continue,     /* CONTINUE_KIND  */
    parse_if,           /* IF_KIND        */
    parse_while,        /* WHILE_KIND     */
    parse_for,          /* FOR_KIND       */
    parse_match,        /* MATCH_KIND     */
  };

  expect(stmt->kind >= IMPORT_KIND && stmt->kind <= MATCH_KIND);
  handlers[stmt->kind](ps, stmt);
}
