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

/* lang module */
static Module _lang_;
/* ModuleObject */
static Symbol *modClsSym;
/* Module, path as key */
static HashMap modules;

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
}

void fini_parser(void)
{
  stable_free(_lang_.stbl);
  hashmap_fini(&modules, _mod_free_, NULL);
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
  case OP_SET_VALUE: {
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
  case OP_CALL: {
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
  case OP_INIT_CALL:
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
  NULL, "BLOCK", "IF-BLOCK", "WHILE-BLOCK", "FOR-BLOCK", "MATCH-BLOCK"
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
  expect(ps->u == NULL);
  expect(vector_size(&ps->ustack) == 0);
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
  print("scope-%d(%s, %s) symbols:\n", ps->depth, scope, name);
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

static void merge_into_func(ParserUnit *u, char *name)
{
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
      merge_into_func(u, "__init__");
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
      merge_into_func(u, "__init__");
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
    // find class(function) with generic type ?
    expect(exp->desc->paras == NULL);
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
      // find class(function) with generic type ?
      expect(exp->desc->paras == NULL);
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
    // find class(function) with generic type ?
    expect(exp->desc->paras == NULL);
    return sym;
  }
  return NULL;
}

static Symbol *find_local_id_symbol(ParserState *ps, char *name)
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
    if (up->scope == SCOPE_MODULE || up->scope == SCOPE_CLASS)
      break;
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

static Symbol *get_klass_symbol(ParserState *ps, char *path, char *name)
{
  Symbol *sym;
  Module *mod = ps->module;

  if (path == NULL) {
    /* find type from current module */
    sym = stable_get(mod->stbl, name);
    if (sym != NULL) {
      debug("find symbol '%s' in current module '%s'", name, mod->name);
      if (sym->kind == SYM_CLASS) {
        ++sym->used;
        return sym;
      } else {
        error("symbol '%s' is not Class", name);
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
      } else {
        error("symbol '%s' is not Class", name);
        return NULL;
      }
    }

    error("cannot find symbol '%s'", name);
    return NULL;
  } else if (isbuiltin(path)) {
    /* find type from auto-imported modules */
    sym = find_from_builtins(name);
    if (sym != NULL) {
      debug("find symbol '%s' in auto-imported modules", name);
      if (sym->kind == SYM_CLASS) {
        ++sym->used;
        return sym;
      } else {
        error("symbol '%s' is not Class", name);
        return NULL;
      }
    }
    error("cannot find symbol '%s'", name);
    return NULL;
  } else {
    panic("not implemented");
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

Symbol *get_desc_symbol(ParserState *ps, TypeDesc *desc)
{
  if (desc == NULL)
    return NULL;

  Symbol *sym;
  switch (desc->kind) {
  case TYPE_BASE:
    sym = get_literal_symbol(desc->base);
    break;
  case TYPE_KLASS:
    sym = get_klass_symbol(ps, desc->klass.path, desc->klass.type);
    // update auto-imported descriptor's path
    if (sym != NULL && desc->klass.path == NULL) {
      desc->klass.path = sym->desc->klass.path;
    }
    break;
  case TYPE_PROTO:
    sym = get_desc_symbol(ps, desc->proto.ret);
    break;
  default:
    panic("invalid desc %d", desc->kind);
    break;
  }

  return sym;
}

static void parse_self(ParserState *ps, Expr *exp)
{
  ParserUnit *u = ps->u;
  expect(exp->ctx == EXPR_LOAD);
  if (u->scope == SCOPE_FUNC) {
    ParserUnit *up = up_scope(ps);
    if (up->scope != SCOPE_CLASS) {
      syntax_error(ps, exp->row, exp->col, "self must be used in method");
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

static void parse_literal(ParserState *ps, Expr *exp)
{
  if (exp->ctx != EXPR_LOAD)
    syntax_error(ps, exp->row, exp->col, "constant is not writable");

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

static void parse_attr_inplace(ParserState *ps, char *name, Expr *exp)
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
  }

  if (exp->ctx == EXPR_LOAD) {
    CODE_OP(OP_LOAD_GLOBAL);
    CODE_OP_S(OP_GET_VALUE, exp->id.name);
  } else if (exp->ctx == EXPR_STORE) {
    CODE_OP(OP_LOAD_GLOBAL);
    CODE_OP_S(OP_SET_VALUE, exp->id.name);
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
    parse_attr_inplace(ps, exp->id.name, exp);
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

static void ident_up_func(ParserState *ps, Expr *exp)
{
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
      CODE_OP(OP_LOAD_GLOBAL);
      CODE_OP_S(OP_SET_VALUE, exp->id.name);
    } else if (exp->ctx == EXPR_INPLACE) {
      CODE_OP(OP_LOAD_GLOBAL);
      parse_attr_inplace(ps, exp->id.name, exp);
    } else {
      panic("invalid expr's ctx %d", exp->ctx);
    }
  } else if (up->scope == SCOPE_CLASS) {
    if (exp->ctx == EXPR_LOAD) {
      CODE_LOAD(0);
      CODE_OP_S(OP_GET_VALUE, exp->id.name);
    } else if (exp->ctx == EXPR_STORE) {
      CODE_LOAD(0);
      CODE_OP_S(OP_SET_VALUE, exp->id.name);
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
    syntax_error(ps, exp->row, exp->col,
      "cannot find symbol '%s'", exp->id.name);
    return;
  }

  if (sym->kind == SYM_FUNC && exp->right == NULL) {
    exp->ctx = EXPR_LOAD_FUNC;
  }

  if (exp->id.where == CURRENT_SCOPE) {
    ident_codegen(current_codes, ps, exp);
  } else if (exp->id.where == UP_SCOPE) {
    ident_codegen(up_codes, ps, exp);
  } else if (exp->id.where == AUTO_IMPORTED) {
    ident_codegen(builtin_codes, ps, exp);
  }
}

static void parse_unary(ParserState *ps, Expr *exp)
{
  Expr *e = exp->unary.exp;
  e->ctx = EXPR_LOAD;
  parser_visit_expr(ps, e);

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

  lexp->ctx = EXPR_LOAD;
  parser_visit_expr(ps, lexp);

  exp->sym = get_desc_symbol(ps, lexp->desc);
  if (exp->sym == NULL) {
    syntax_error(ps, exp->row, exp->col, "cannot find type");
    return;
  }

  if (exp->desc == NULL) {
    exp->desc = TYPE_INCREF(lexp->desc);
  }

  if (has_error(ps))
    return;

  if (exp->binary.op == BINARY_ADD) {
    Symbol *sym = klass_find_member(exp->sym, "__add__");
    if (sym == NULL) {
      syntax_error(ps, lexp->row, lexp->col, "unsupported +");
    } else {
      TypeDesc *desc = sym->desc;
      expect(desc != NULL && desc->kind == TYPE_PROTO);
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
    expect(op >= BINARY_ADD && op <= BINARY_LOR);
    CODE_OP(opcodes[op]);
  }
}

static void parse_ternary(ParserState *ps, Expr *exp)
{
  ParserUnit *u = ps->u;

  Expr *test = exp->ternary.test;
  test->ctx = EXPR_LOAD;
  parser_visit_expr(ps, test);
  //if not bool, error
  if (desc_isbool(test->desc)) {
    syntax_error(ps, test->row, test->col, "if cond expr is not bool");
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

  if (!desc_check(lexp->desc, rexp->desc)) {
    syntax_error(ps, rexp->row, rexp->col,
      "type mismatch in conditonal expression");
  }

  exp->sym = lexp->sym;
  exp->desc = TYPE_INCREF(exp->sym->desc);
}

static TypeDesc *type_maybe_instanced(TypeDesc *para, TypeDesc *ref)
{
  if (para->kind == TYPE_BASE)
    return TYPE_INCREF(ref);

  if (ref == NULL)
    return NULL;

  if (ref->kind == TYPE_BASE)
    return TYPE_INCREF(ref);

  switch (ref->kind) {
  case TYPE_PROTO: {
    expect(ref->paras == NULL);
    if (para->types == NULL)
      return TYPE_INCREF(ref);

    TypeDesc *rtype = ref->proto.ret;
    if (rtype != NULL && rtype->kind == TYPE_PARAREF) {
      rtype = vector_get(para->types, rtype->pararef.index);
    }

    Vector *args = vector_new();
    TypeDesc *ptype;
    vector_for_each(ptype, ref->proto.args) {
      if (ptype != NULL && ptype->kind == TYPE_PARAREF) {
        ptype = vector_get(para->types, ptype->pararef.index);
      }
      vector_push_back(args, TYPE_INCREF(ptype));
    }

    if (rtype != ref->proto.ret || vector_size(args) != 0) {
      return desc_from_proto(args, rtype);
    } else {
      free_descs(args);
      return TYPE_INCREF(ref);
    }
    break;
  }
  case TYPE_PARAREF: {
    expect(para->types != NULL);
    TypeDesc *desc = vector_get(para->types, ref->pararef.index);
    return TYPE_INCREF(desc);
    break;
  }
  case TYPE_KLASS: {
    Vector *types = vector_new();
    TypeDesc *item;
    TypeDesc *type;
    vector_for_each(item, ref->types) {
      if (item->kind == TYPE_PARAREF) {
        type = vector_get(para->types, item->pararef.index);
        expect(type != NULL);
        expect(type->kind != TYPE_PARAREF);
        expect(type->kind != TYPE_PARADEF);
        TYPE_INCREF(type);
        vector_push_back(types, type);
      } else {
        expect(item->kind != TYPE_PARADEF);
        TYPE_INCREF(item);
        vector_push_back(types, item);
      }
    }

    if (vector_size(types) > 0) {
      type = desc_from_klass(ref->klass.path, ref->klass.type);
      type->types = types;
      return type;
    } else {
      vector_free(types);
      return TYPE_INCREF(ref);
    }
    break;
  }
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
    sym = klass_find_member(lsym->var.typesym, id->name);
    break;
  case SYM_FUNC:
    debug("left sym '%s' is a func", lsym->name);
    desc = lsym->desc;
    if (vector_size(desc->proto.args)) {
      syntax_error(ps, lexp->row, lexp->col,
        "func with arguments cannot be accessed like field.");
    } else {
      sym = get_desc_symbol(ps, desc->proto.ret);
      if (sym != NULL) {
        expect(sym->kind == SYM_CLASS);
        sym = klass_find_member(sym, id->name);
      } else {
        syntax_error(ps, exp->row, exp->col, "cannot find type");
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
      "'%s' is not found in '%s'", id->name, lsym->name);
  } else {
    if (sym->kind == SYM_FUNC && exp->right == NULL) {
      syntax_error(ps, exp->row, exp->col,
        "call func '%s' or return itself?", id->name);
    } else {
      exp->sym = sym;
      exp->desc = type_maybe_instanced(ldesc, sym->desc);
    }
  }

  // generate codes
  if (!has_error(ps)) {
    switch (sym->kind) {
    case SYM_VAR: {
      if (exp->ctx == EXPR_LOAD)
        CODE_OP_S(OP_GET_VALUE, id->name);
      else if (exp->ctx == EXPR_STORE)
        CODE_OP_S(OP_SET_VALUE, id->name);
      else if (exp->ctx == EXPR_INPLACE)
        parse_attr_inplace(ps, id->name, exp);
      else
        panic("invalid attr expr's ctx %d", exp->ctx);
      break;
    }
    case SYM_FUNC: {
      if (exp->ctx == EXPR_LOAD)
        CODE_OP_S(OP_GET_VALUE, id->name);
      else if (exp->ctx == EXPR_CALL_FUNC) {
        CODE_OP_S_ARGC(OP_CALL, id->name, exp->argc);
      } else if (exp->ctx == EXPR_LOAD_FUNC)
        CODE_OP_S(OP_GET_METHOD, id->name);
      else if (exp->ctx == EXPR_STORE)
        CODE_OP_S(OP_SET_VALUE, id->name);
      else
        panic("invalid exp's ctx %d", exp->ctx);
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

  Symbol *sym = NULL;
  switch (lsym->kind) {
  case SYM_CONST:
    break;
  case SYM_VAR:
    debug("left sym '%s' is a var", lsym->name);
    sym = klass_find_member(lsym->var.typesym, funcname);
    break;
  case SYM_CLASS: {
    debug("left sym '%s' is a class", lsym->name);
    sym = klass_find_member(lsym, funcname);
    break;
  }
  default:
    panic("invalid left symbol %d", lsym->kind);
    break;
  }

  if (sym == NULL || sym->kind != SYM_FUNC) {
    syntax_error(ps, lexp->row, lexp->col,
      "'%s' is not supported subscript operation.", lsym->name);
    return;
  }

  Vector *args = sym->desc->proto.args;
  TypeDesc *desc = desc = sym->desc->proto.ret;

  if (exp->ctx == EXPR_LOAD) {
    if (vector_size(args) != 1) {
      syntax_error(ps, exp->row, exp->col,
        "Count of arguments of func %s is not only one", funcname);
    }
    if (desc == NULL) {
      syntax_error(ps, exp->row, exp->col,
        "Return value of func %s is void", funcname);
    }
  } else if (exp->ctx == EXPR_STORE) {
    if (vector_size(args) != 2) {
      syntax_error(ps, exp->row, exp->col,
        "Count of arguments of func %s is not two", funcname);
    }
    if (desc != NULL) {
      syntax_error(ps, exp->row, exp->col,
        "Return value of func %s is not void", funcname);
    }
  } else {
    panic("invalid expr's context");
  }

  if (sym != NULL) {
    desc = vector_get(args, 0);
    desc = type_maybe_instanced(lexp->desc, desc);
    if (!desc_check(iexp->desc, desc)) {
      syntax_error(ps, iexp->row, iexp->col, "subscript index type is error");
    }
    TYPE_DECREF(desc);

    TYPE_DECREF(exp->desc);
    if (exp->ctx == EXPR_LOAD) {
      desc = sym->desc->proto.ret;
    } else if (exp->ctx == EXPR_STORE) {
      desc = vector_get(args, 1);
    } else {
      desc = NULL;
    }
    desc = type_maybe_instanced(lexp->desc, desc);
    exp->desc = TYPE_INCREF(desc);
    exp->sym = get_desc_symbol(ps, desc);
    if (exp->sym == NULL) {
      syntax_error(ps, exp->row, exp->col, "cannot find type");
    }
    TYPE_DECREF(desc);
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

static int check_call_args(TypeDesc *proto, Vector *args)
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
    if (!desc_check(desc, arg->desc))
      return -1;
  }
  return 0;
}

static void parse_call(ParserState *ps, Expr *exp)
{
  Vector *args = exp->call.args;
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
  if (desc != NULL && desc->kind != TYPE_PROTO) {
    syntax_error(ps, lexp->row, lexp->col, "expr is not a func");
  }

  if (lexp->kind == CALL_KIND && desc->kind == TYPE_PROTO) {
    debug("left expr is func call and ret is func");
    CODE_OP_ARGC(OP_EVAL, lexp->argc);
  }

  // check call arguments
  if (!has_error(ps)) {
    if (check_call_args(desc, args)) {
      syntax_error(ps, arg->row, arg->col, "call args check failed");
    } else {
      exp->desc = TYPE_INCREF(desc->proto.ret);
      exp->sym = get_desc_symbol(ps, exp->desc);
    }
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
    syntax_error(ps, ps->row, ps->col, "length of tuple is larger than 16");
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
    syntax_error(ps, ps->row, ps->col, "length of array is larger than 16");
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

  TypeDesc *para = get_subarray_type(types);
  exp->desc = desc_from_array;
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
    syntax_error(ps, ps->row, ps->col, "length of dict is larger than 16");
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
          syntax_error(ps, exp->row, exp->col, "Key of Map is not the same");
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
        syntax_error(ps, id->row, id->col,
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

static Symbol *add_update_var(ParserState *ps, Ident *id, TypeDesc *desc)
{
  ParserUnit *u = ps->u;
  Symbol *sym;
  Symbol *funcsym;
  switch (u->scope) {
  case SCOPE_MODULE:
  case SCOPE_CLASS:
    sym = stable_get(u->stbl, id->name);
    expect(sym != NULL);
    break;
  case SCOPE_FUNC:
    // function scope has independent space for variables.
    debug("var '%s' declaration in func.", id->name);
    sym = stable_add_var(u->stbl, id->name, desc);
    if (sym == NULL) {
      syntax_error(ps, id->row, id->col, "parameter duplicated");
      return NULL;
    } else {
      funcsym = u->sym;
      vector_push_back(&funcsym->func.locvec, sym);
      ++sym->refcnt;
    }
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
      syntax_error(ps, id->row, id->col, "var '%s' is duplicated", id->name);
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
      syntax_error(ps, id->row, id->col, "parameter duplicated");
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

  if (sym->desc == NULL) {
    sym->desc = TYPE_INCREF(desc);
  }

  if (sym->var.typesym == NULL) {
    sym->var.typesym = get_desc_symbol(ps, sym->desc);
  }

  return sym;
}

static void parse_body(ParserState *ps, char *name, Vector *body, Type ret)
{
  int sz = vector_size(body);
  Stmt *s = NULL;
  vector_for_each(s, body) {
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
      syntax_error(ps, ret.row, ret.col, "func '%s' needs return value", name);
    } else {
      debug("add OP_RETURN");
      CODE_OP(OP_RETURN);
    }
    return;
  }

  // last one is expr-stmt, check it has value or not
  if (s->kind == EXPR_KIND) {
    if (s->hasvalue) {
      // expr and has value and check types ara matched.
      debug("last expr-stmt and has value, add OP_RETURN_VALUE");
      if (ret.desc == NULL) {
        syntax_error(ps, ret.row, ret.col, "func '%s' no return value", name);
      } else {
        if (!desc_check(ret.desc, s->desc)) {
          syntax_error(ps, ret.row, ret.col, "return values are not matched");
        } else {
          CODE_OP(OP_RETURN_VALUE);
        }
      }
    } else {
      // expr and no value
      if (ret.desc != NULL) {
        syntax_error(ps, ret.row, ret.col, "func '%s' no return value", name);
      } else {
        debug("last expr-stmt and no value, add OP_RETURN");
        CODE_OP(OP_RETURN);
      }
    }
    return;
  }

  if (s->hasvalue) {
    if (ret.desc == NULL) {
      syntax_error(ps, ret.row, ret.col, "func '%s' no return value", name);
    } else {
      if (!desc_check(ret.desc, s->desc)) {
          syntax_error(ps, ret.row, ret.col, "return values are not matched");
      }
    }
  } else {
    if (ret.desc != NULL) {
      syntax_error(ps, ret.row, ret.col, "func '%s' needs return value", name);
    }
  }

  if (!has_error(ps) && s->kind != RETURN_KIND) {
    // last one is return or other statement
    debug("last not expr-stmt and not ret-stmt, add OP_RETURN");
    CODE_OP(OP_RETURN);
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
    add_update_var(ps, &item->id, item->type.desc);
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
  e->ctx = EXPR_LOAD;
  parser_visit_expr(ps, e);
  TYPE_DECREF(exp->desc);
  exp->desc = desc_from_bool;
  exp->sym = get_desc_symbol(ps, exp->desc);
  if (exp->sym == NULL) {
    syntax_error(ps, exp->row, exp->col, "cannot find type");
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
  exp->sym = get_desc_symbol(ps, exp->desc);
  if (exp->sym == NULL) {
    syntax_error(ps, exp->row, exp->col, "cannot find type");
  }

  if (!has_error(ps)) {
    CODE_OP_TYPE(OP_TYPECHECK, exp->desc);
  }
}

static void check_new_args(ParserState *ps, Symbol *sym, Expr *exp)
{
  Ident *id = &exp->newobj.id;
  char *name = id->name;
  int row = id->row;
  int col = id->col;

  if (sym->kind != SYM_CLASS) {
    syntax_error(ps, row, col, "'%s' is not a class", name);
    return;
  }

  /*
  Vector *types = exp->newobj.types;
  if (!check_typepara(desc->paras, types)) {
    syntax_error(ps, id->row, id->col,
                 "'%s' generic type check faield", id->name);
  }
  */

  Vector *args = exp->newobj.args;
  int argc = vector_size(args);

  Symbol *initsym = stable_get(sym->type.stbl, "__init__");
  if (initsym == NULL && argc == 0)
    return;

  if (initsym == NULL && argc > 0) {
    syntax_error(ps, row, col, "'%s' has no __init__()", name);
    return;
  }

  TypeDesc *desc = initsym->desc;
  if (desc->kind != TYPE_PROTO) {
    syntax_error(ps, row, col, "'%s': __init__ is not a func", name);
    return;
  }

  if (desc->proto.ret != NULL) {
    syntax_error(ps, row, col, "'%s': __init__ must be no return", name);
    return;
  }

  if (check_call_args(desc, args))
    syntax_error(ps, row, col, "'%s' args are not mached", name);
}

static void parse_new(ParserState *ps, Expr *exp)
{
  char *path = exp->newobj.path;
  Ident *id = &exp->newobj.id;
  Symbol *sym = get_klass_symbol(ps, path, id->name);
  if (sym == NULL) {
    syntax_error(ps, id->row, id->col, "cannot find class '%s'", id->name);
    return;
  }

  check_new_args(ps, sym, exp);

  if (!has_error(ps)) {
    exp->sym = sym;
    exp->desc = TYPE_INCREF(sym->desc);

    /*
    Vector *types = exp->newobj.types;
    if (types != NULL) {
      TypeDesc *item;
      vector_for_each(item, types) {
        desc_add_paratype(exp->desc, item);
      }
    }
    */

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
      Inst *i = CODE_OP_TYPE(OP_NEW, desc);
      i->argc = argc;
    }

    if (desc->kind == TYPE_KLASS)  {
      CODE_OP_TYPE(OP_NEW, desc);
      if (argc > 0) {
        CODE_OP(OP_DUP);
        Expr *e;
        vector_for_each_reverse(e, args) {
          e->ctx = EXPR_LOAD;
          parser_visit_expr(ps, e);
        }
        CODE_OP_ARGC(OP_INIT_CALL, argc);
      } else {
        Symbol *initsym = stable_get(sym->type.stbl, "__init__");
        if (initsym != NULL) {
          CODE_OP_ARGC(OP_INIT_CALL, 0);
        }
      }
    }
  }
}

static void parse_range(ParserState *ps, Expr *exp)
{
  Expr *start = exp->range.start;
  Expr *end = exp->range.end;

  end->ctx = EXPR_LOAD;
  parser_visit_expr(ps, end);
  if (end->desc == NULL || !desc_isint(end->desc)) {
    syntax_error(ps, end->row, end->col,
      "range expects integer of end");
  }

  start->ctx = EXPR_LOAD;
  parser_visit_expr(ps, start);
  if (start->desc == NULL || !desc_isint(start->desc)) {
    syntax_error(ps, start->row, start->col,
      "range expects integer of start");
  }

  if (!has_error(ps)) {
    exp->sym = find_from_builtins("Range");
    exp->desc = TYPE_INCREF(exp->sym->desc);
    CODE_OP_I(OP_NEW_RANGE, exp->range.type);
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
    parse_literal,      /* LITERAL_KIND   */
    parse_ident,        /* ID_KIND        */
    parse_unary,        /* UNARY_KIND     */
    parse_binary,       /* BINARY_KIND    */
    parse_ternary,      /* TERNARY_KIND   */
    parse_atrr,         /* ATTRIBUTE_KIND */
    parse_subscr,       /* SUBSCRIPT_KIND */
    parse_call,         /* CALL_KIND      */
    parse_slice,        /* SLICE_KIND     */
    parse_tuple,        /* TUPLE_KIND     */
    parse_array,        /* ARRAY_KIND     */
    parse_map,          /* MAP_KIND       */
    parse_anony,        /* ANONY_KIND     */
    parse_is,           /* IS_KIND        */
    parse_as,           /* AS_KIND        */
    parse_new,          /* NEW_KIND       */
    parse_range,        /* RANGE_KIND     */
  };

  expect(exp->kind >= NIL_KIND && exp->kind <= RANGE_KIND);
  handlers[exp->kind](ps, exp);

  /* function's return maybe null */
  if (exp->kind != CALL_KIND && exp->desc == NULL) {
    syntax_error(ps, exp->row, exp->col, "cannot resolve expr's type");
  }
}

static Module *new_mod_from_mobject(char *path)
{
  Object *ob = module_load(path);
  expect(ob != NULL);
  debug("new module(parser) '%s' from memory", path);
  Module *mod = kmalloc(sizeof(Module));
  ModuleObject *mo = (ModuleObject *)ob;
  mod->path = path;
  mod->name = mo->name;
  mod->stbl = stable_from_mobject(ob);
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
    debug("import from '%s' as '%s'", path, name);
    Symbol *sym = symbol_new(name, SYM_MOD);
    sym->mod.path = path;
    sym->desc = desc_from_klass("lang", "Module");
    if (stable_add_symbol(u->stbl, sym) < 0) {
      syntax_error(ps, s->row, s->col, "symbol '%s' is duplicated", name);
      return;
    } else {
      symbol_decref(sym);
    }

    Module key = {.path = path};
    hashmap_entry_init(&key, strhash(path));
    Module *mod = hashmap_get(&modules, &key);
    if (mod == NULL) {
      // firstly load from memory
      mod = new_mod_from_mobject(path);

      // then load from disk
      //
      // compile it and try to load it again
      //

      // FIXME: delay-load?
      expect(mod != NULL);
      sym->mod.ptr = mod;
    } else {
      debug("'%s' already imported", path);
      sym->mod.ptr = mod;
    }
  }
}

static void parse_vardecl(ParserState *ps, Stmt *stmt)
{
  Expr *exp = stmt->vardecl.exp;
  Ident *id = &stmt->vardecl.id;
  Type *type = &stmt->vardecl.type;
  TypeDesc *desc = type->desc;

  if (exp != NULL) {
    exp->ctx = EXPR_LOAD;
    parser_visit_expr(ps, exp);

    if (desc == NULL) {
      desc = exp->desc;
    }

    if (desc == NULL) {
      return;
    }

    if (!desc_check(desc, exp->desc)) {
      syntax_error(ps, exp->row, exp->col, "types are not matched");
    }
  }

  /* add or update variable type symbol */
  Symbol *sym = add_update_var(ps, id, desc);
  if (sym == NULL)
    return;

  if (sym->var.typesym == NULL) {
    syntax_error(ps, type->row, type->col, "cannot find type");
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

static void parse_assign(ParserState *ps, Stmt *stmt)
{
  Expr *rexp = stmt->assign.rexp;
  Expr *lexp = stmt->assign.lexp;
  AssignOpKind op = stmt->assign.op;

  // assignment
  if (op == OP_ASSIGN) {
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

    if (lexp->desc == NULL) {
      syntax_error(ps, lexp->row, lexp->col,
        "cannot resolve left expr's type");
    }

    if (rexp->desc == NULL) {
      syntax_error(ps, rexp->row, rexp->col,
        "right expr's type is void");
    }

    // check type is compatible
    if (lexp->desc != NULL && rexp->desc != NULL) {
      if (!desc_check(lexp->desc, rexp->desc)) {
        syntax_error(ps, lexp->row, lexp->col,
          "types are not matched");
      }
    }
  } else {
    // inplace operations
    lexp->ctx = EXPR_INPLACE;
    lexp->inplace = stmt;
    switch (lexp->kind) {
    case ATTRIBUTE_KIND:
    case SUBSCRIPT_KIND:
    case ID_KIND:
      parser_visit_expr(ps, lexp);
      break;
    default:
      syntax_error(ps, lexp->row, lexp->col, "invalid inplace assignment");
      break;
    }
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

static void parse_funcdecl(ParserState *ps, Stmt *stmt)
{
  char *funcname = stmt->funcdecl.id.name;
  debug("parse function '%s'", funcname);
  parser_enter_scope(ps, SCOPE_FUNC, 0);
  ParserUnit *u = ps->u;
  u->stbl = stable_new();

  /* get func symbol */
  ParserUnit *up = up_scope(ps);
  expect(up != NULL);
  Symbol *sym = stable_get(up->stbl, funcname);
  expect(sym != NULL && sym->kind == SYM_FUNC);
  u->sym = sym;

  /* parse func arguments */
  Vector *idtypes = stmt->funcdecl.idtypes;
  IdType *item;
  vector_for_each(item, idtypes) {
    add_update_var(ps, &item->id, item->type.desc);
  }

  parse_body(ps, funcname, stmt->funcdecl.body, stmt->funcdecl.ret);

  stable_free(u->stbl);
  u->stbl = NULL;
  parser_exit_scope(ps);
  debug("end of function '%s'", funcname);
}

static int infunc(ParserState *ps)
{
  ParserUnit *u = ps->u;
  if (u->scope == SCOPE_FUNC || u->scope == SCOPE_ANONY)
    return 1;

  vector_for_each_reverse(u, &ps->ustack) {
    if (u->scope == SCOPE_FUNC || u->scope == SCOPE_ANONY)
      return 1;
  }

  return 0;
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
  if (!infunc(ps)) {
    syntax_error(ps, stmt->row, stmt->col, "'return' outside function");
  }

  Expr *exp = stmt->ret.exp;
  if (exp != NULL) {
    debug("return has value");
    exp->ctx = EXPR_LOAD;
    parser_visit_expr(ps, exp);
    if (exp->desc == NULL) {
      syntax_error(ps, exp->row, exp->col, "expr has no value");
    } else {
      stmt->hasvalue = 1;
      stmt->desc = TYPE_INCREF(exp->desc);
      CODE_OP(OP_RETURN_VALUE);
    }
  } else {
    debug("return has no value");
    CODE_OP(OP_RETURN);
  }
}

static void parse_break(ParserState *ps, Stmt *stmt)
{
  if (!inloop(ps)) {
    syntax_error(ps, stmt->row, stmt->col, "'break' outside loop");
  }

  Inst *jmp = CODE_OP(OP_JMP);
  jmp->offset = codeblock_bytes(ps->u->block);
  jmp->jmpdown = 1;
  parser_add_jmp(ps, jmp);
}

static void parse_continue(ParserState *ps, Stmt *stmt)
{
  if (!inloop(ps)) {
    syntax_error(ps, stmt->row, stmt->col, "'continue' outside loop");
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
    if (desc_isbool(test->desc)) {
      syntax_error(ps, test->row, test->col, "if cond expr is not bool");
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
    if (desc_isbool(test->desc)) {
      syntax_error(ps, test->row, test->col, "if cond expr is not bool");
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
  if (sym->kind == SYM_VAR) {
    sym = sym->var.typesym;
  } else if (sym->kind == SYM_CLASS) {

  } else {
    panic("which kind of symbol? %d", sym->kind);
  }

  sym = klass_find_member(sym, "__iter__");
  if (sym == NULL) {
    syntax_error(ps, iter->row, iter->col, "object is not iteratable.");
  } else {
    expect(desc_isproto(sym->desc));
    expect(vector_size(sym->desc->proto.ret->types) == 1);
    TypeDesc *subtype = vector_get(sym->desc->proto.ret->types, 0);
    desc = type_maybe_instanced(iter->sym->desc, subtype);
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
    sym = find_local_id_symbol(ps, vexp->id.name);
    if (sym != NULL) {
      if (!has_error(ps) && !desc_check(sym->desc, desc)) {
        syntax_error(ps, vexp->row, vexp->col, "types are not matched");
      }
    } else {
      // create local ident
      Ident id = {vexp->id.name, vexp->row, vexp->col};
      add_update_var(ps, &id, desc);
    }
  } else if (vexp->kind == TUPLE_KIND) {
    panic("not implemented");
  } else {
    // fallthrough
  }

  vexp->ctx = EXPR_STORE;
  parser_visit_expr(ps, vexp);
  if (!has_error(ps) && !desc_check(vexp->desc, desc)) {
    syntax_error(ps, vexp->row, vexp->col, "types are not matched");
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

  stable_free(u->stbl);
  u->stbl = NULL;
  parser_exit_scope(ps);
}

static void parse_ifunc(ParserState *ps, Stmt *stmt)
{
  Ident *id = &stmt->funcdecl.id;
  debug("add ifunc '%s'", id->name);
  Vector *idtypes = stmt->funcdecl.idtypes;
  Type *ret = &stmt->funcdecl.ret;
  TypeDesc *proto = parse_proto(idtypes, ret);
  stable_add_ifunc(ps->u->stbl, id->name, proto);
  TYPE_DECREF(proto);
}

static void parse_typedecl(ParserState *ps, Stmt *stmt)
{
  Ident *id = &stmt->class_stmt.id;
#if !defined(NLog)
  char *typename = stmt->kind == CLASS_KIND ? "class" : "trait";
#endif
  debug("parse %s '%s'", typename, id->name);
  Symbol *sym = stable_get(ps->u->stbl, id->name);
  expect(sym != NULL);

  //parse_class_extends(ps, stmt);

  parser_enter_scope(ps, SCOPE_CLASS, 0);
  ps->u->sym = sym;
  ps->u->stbl = sym->type.stbl;

  /* parse class body */
  Vector *body = stmt->class_stmt.body;
  int sz = vector_size(body);
  Stmt *s = NULL;
  vector_for_each(s, body) {
    parse_stmt(ps, s);
  }

  parser_exit_scope(ps);

  debug("end of %s '%s'", typename, id->name);
}

void parse_stmt(ParserState *ps, Stmt *stmt)
{
  static void (*handlers[])(ParserState *, Stmt *) = {
    NULL,               /* INVALID        */
    parse_import,       /* IMPORT_KIND    */
    NULL,               /* CONST_KIND     */
    parse_vardecl,      /* VAR_KIND       */
    parse_assign,       /* ASSIGN_KIND    */
    parse_funcdecl,     /* FUNC_KIND      */
    parse_return,       /* RETURN_KIND    */
    parse_expr,         /* EXPR_KIND      */
    parse_block,        /* BLOCK_KIND     */
    parse_ifunc,        /* IFUNC_KIND     */
    parse_typedecl,     /* CLASS_KIND     */
    parse_typedecl,     /* TRAIT_KIND     */
    NULL,               /* ENUM_KIND      */
    NULL,               /* EVAL_KIND      */
    parse_break,        /* BREAK_KIND     */
    parse_continue,     /* CONTINUE_KIND  */
    parse_if,           /* IF_KIND        */
    parse_while,        /* WHILE_KIND     */
    parse_for,          /* FOR_KIND       */
    NULL,               /* MATCH_KIND     */
  };

  expect(stmt->kind >= IMPORT_KIND && stmt->kind <= MATCH_KIND);
  handlers[stmt->kind](ps, stmt);
}
