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

#include <stdio.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <pthread.h>
#include <ctype.h>
#include "version.h"
#include "parser.h"
#include "koala_yacc.h"
#include "koala_lex.h"
#include "moduleobject.h"
#include "tupleobject.h"
#include "fieldobject.h"
#include "methodobject.h"
#include "codeobject.h"
#include "eval.h"
#include "codeobject.h"
#include "image.h"
#include "enumobject.h"
#include "readline.h"
#include "textblock.h"

#define PROMPT      "> "
#define MORE_PROMPT "  "

static void show_banner(void)
{
  printf("Koala %s\r\n", KOALA_VERSION);

  struct utsname sysinfo;
  if (!uname(&sysinfo)) {
#if defined(__clang__)
  printf("[clang %d.%d.%d] on %s/%s %s\r\n",
         __clang_major__, __clang_minor__, __clang_patchlevel__,
         sysinfo.sysname, sysinfo.machine, sysinfo.release);
#elif defined(__GNUC__)
  printf("[gcc %d.%d.%d] on %s/%s %s\r\n",
         __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__,
         sysinfo.sysname, sysinfo.machine, sysinfo.release);
#elif defined(_MSC_VER)
#else
#endif
  }
}

static Symbol *cursym;
static Symbol *evalsym;
static Symbol *modSym;
static Module mod;
static ParserState ps;

static Object *mo;
static KoalaState kstate;
extern pthread_key_t kskey;

static void init_cmdline_env(void)
{
  mod.path = "__main__";
  mod.name = "__main__";
  mod.stbl = stable_new();
  vector_init(&mod.pss);

  modSym = symbol_new(mod.name, SYM_MOD);
  modSym->desc = desc_from_klass("lang", "Module");
  modSym->mod.ptr = &mod;

  TypeDesc *strdesc = desc_from_base(BASE_STR);
  TypeDesc *desc = desc_from_proto(NULL, strdesc);
  stable_add_func(mod.stbl, "__name__", desc);
  TYPE_DECREF(strdesc);
  TYPE_DECREF(desc);

  desc = desc_from_proto(NULL, NULL);
  evalsym = stable_add_func(mod.stbl, "__init__", desc);
  TYPE_DECREF(desc);
  mod.initsym = evalsym;

  ps.interactive = 1;
  ps.filename = "stdin";
  ps.module = &mod;
  ps.row = 1;
  ps.col = 1;
  vector_init(&ps.ustack);
  vector_init(&ps.upanonies);

  mo = module_new("__main__");

  init_stdin();
}

static void fini_cmdline_env(void)
{
  reset_stdin();

  stable_free(mod.stbl);
  mod.stbl = NULL;
  symbol_decref(modSym);
  modSym = NULL;
  OB_DECREF(mo);
  mo = NULL;
  expect(vector_size(&ps.ustack) == 0);
  vector_fini(&ps.ustack);
  expect(vector_size(&ps.upanonies) == 0);
  vector_fini(&ps.upanonies);
}

yyscan_t scanner;
int halt = 0;

void koala_readline(void)
{
  init_cmdline_env();
  show_banner();
  yylex_init_extra(&ps, &scanner);
  yyset_in(stdin, scanner);
  yyparse(&ps, scanner);
  yylex_destroy(scanner);
  putchar('\n');
  fini_cmdline_env();
}

static void _load_const_(void *val, int kind, int index, void *arg)
{
  Object *consts = arg;
  Object *ob;
  CodeInfo *ci;

  if (kind == CONST_LITERAL) {
    ob = new_literal(val);
  } else if (kind == CONST_TYPE) {
    ob = new_descob(val);
  } else {
    expect(kind == CONST_ANONY);
    ci = val;
    ob = code_new(ci->name, ci->desc, ci->codes, ci->size);
    code_set_module(ob, mo);
    code_set_consts(ob, consts);
    code_set_locvars(ob, ci->locvec);
    code_set_freevals(ob, ci->freevec);
    code_set_upvals(ob, ci->upvec);
    debug("'%s': %d locvars, %d freevals and %d upvals",
          ci->name, vector_size(ci->locvec),
          vector_size(ci->freevec), vector_size(ci->upvec));
  }

  tuple_set(consts, index, ob);
  OB_DECREF(ob);
}

static Object *code_from_symbol(Symbol *sym)
{
  Object *ob;
  Object *consts;
  Image *image;
  uint8_t *code;
  int size;
  ByteBuffer buf;

  expect(sym->kind == SYM_FUNC);

  image = image_new("__main__");
  bytebuffer_init(&buf, 32);

  code_gen(sym->func.codeblock, image, &buf);

#if !defined(NLog)
  image_show(image);
#endif
  size = _size_(image, ITEM_CONST);
  if (size > 0)
    consts = tuple_new(size);
  else
    consts = NULL;
  image_load_consts(image, _load_const_, consts);
  size = bytebuffer_toarr(&buf, (char **)&code);

  ob = code_new(sym->name, sym->desc, code, size);
  code_set_consts(ob, consts);
  Vector *locvec = vector_new();
  fill_locvars(sym, locvec);
  code_set_locvars(ob, locvec);
  code_set_freevals(ob, &sym->func.freevec);
  vector_free(locvec);
  OB_DECREF(consts);
  kfree(code);

  bytebuffer_fini(&buf);
  image_free(image);
  return ob;
}

int cmd_add_const(ParserState *ps, Ident id)
{
  cursym = stable_add_const(mod.stbl, id.name, NULL);
  if (cursym != NULL) {
    return 0;
  } else {
    serror(id.row, id.col, "'%s' is redeclared", id.name);
    return -1;
  }
}

int cmd_add_var(ParserState *ps, Ident id)
{
  cursym = stable_add_var(mod.stbl, id.name, NULL);
  if (cursym != NULL) {
    return 0;
  } else {
    serror(id.row, id.col, "'%s' is redeclared", id.name);
    return -1;
  }
}

int cmd_add_func(ParserState *ps, Ident id)
{
  cursym = stable_add_func(mod.stbl, id.name, NULL);
  if (cursym == NULL) {
    serror(id.row, id.col, "'%s' is redeclared", id.name);
    return -1;
  }
  return 0;
}

static void cmd_visit_type(ParserState *ps, Symbol *sym, Vector *body)
{
  if (body == NULL)
    return;

  Symbol *sym2 = NULL;
  STable *stbl = sym->type.stbl;
  Ident *id;
  Stmt *s;
  vector_for_each(s, body) {
    if (s->kind == VAR_KIND) {
      id = &s->vardecl.id;
      sym2 = stable_add_var(stbl, id->name, NULL);
      if (sym2 == NULL) {
        serror(id->row, id->col, "'%s' is redeclared", id->name);
      }
    } else if (s->kind == FUNC_KIND) {
      id = &s->funcdecl.id;
      sym2 = stable_add_func(stbl, id->name, NULL);
      if (sym2 == NULL) {
        serror(id->row, id->col, "'%s' is redeclared", id->name);
      }
    } else if (s->kind == IFUNC_KIND) {
      /*
      Vector *idtypes = s->funcdecl.idtypes;
      Type *ret = &s->funcdecl.ret;
      TypeDesc *proto = parse_proto(idtypes, ret);
      */
      id = &s->funcdecl.id;
      sym2 = stable_add_ifunc(stbl, id->name, NULL);
      //TYPE_DECREF(proto);
      if (sym2 == NULL) {
        serror(id->row, id->col, "'%s' is redeclared", id->name);
      }
    } else {
      panic("invalid type's body statement: %d", s->kind);
    }
  }
}

static void cmd_visit_label(ParserState *ps, Symbol *sym, Vector *labels)
{
  STable *stbl = sym->type.stbl;
  Symbol *lblsym;
  EnumLabel *label;
  vector_for_each(label, labels) {
    lblsym = stable_add_label(stbl, label->id.name);
    if (lblsym == NULL) {
      serror(label->id.row, label->id.col,
            "enum label '%s' duplicated.", label->id.name);
    }
  }
}

int cmd_add_type(ParserState *ps, Stmt *stmt)
{
  Symbol *sym = NULL;
  Ident *id;
  Symbol *any;
  switch (stmt->kind) {
  case CLASS_KIND: {
    id = &stmt->class_stmt.id;
    sym = stable_add_class(mod.stbl, atom(ps->module->path), id->name);
    if (sym != NULL) {
      sym->desc->klass.path = atom(ps->module->name);
      any = find_from_builtins("Any");
      ++any->refcnt;
      vector_push_back(&sym->type.lro, any);
      cmd_visit_type(ps, sym, stmt->class_stmt.body);
    }
    break;
  }
  case TRAIT_KIND: {
    id = &stmt->class_stmt.id;
    sym = stable_add_trait(mod.stbl, atom(ps->module->path), id->name);
    if (sym != NULL) {
      any = find_from_builtins("Any");
      ++any->refcnt;
      vector_push_back(&sym->type.lro, any);
      cmd_visit_type(ps, sym, stmt->class_stmt.body);
    }
    break;
  }
  case ENUM_KIND: {
    id = &stmt->enum_stmt.id;
    sym = stable_add_enum(mod.stbl, atom(ps->module->path), id->name);
    if (sym != NULL) {
      any = find_from_builtins("Any");
      ++any->refcnt;
      vector_push_back(&sym->type.lro, any);
      cmd_visit_label(ps, sym, stmt->enum_stmt.mbrs.labels);
      cmd_visit_type(ps, sym, stmt->enum_stmt.mbrs.methods);
    }
    break;
  }
  default:
    panic("invalid stmt kind %d", stmt->kind);
    break;
  }

  if (sym == NULL) {
    serror(id->row, id->col, "'%s' is redeclared", id->name);
    cursym = NULL;
    return -1;
  }

  if (has_error(ps)) {
    cursym = NULL;
    return -1;
  }

  cursym = sym;
  return 0;
}

static void _load_mbr_(char *name, int kind, void *data, void *arg)
{
  TypeObject *type = arg;
  if (kind == MBR_FIELD) {
    type_add_field_default(type, atom(name), data);
  } else if (kind == MBR_METHOD) {
    CodeInfo *ci = data;
    Object *code = code_new(name, ci->desc, ci->codes, ci->size);
    code_set_locvars(code, ci->locvec);
    code_set_freevals(code, ci->freevec);
    code_set_module(code, mo);
    code_set_type(code, type);
    code_set_consts(code, type->consts);
    Object *meth = method_new(name, code);
    type_add_method(type, meth);
    OB_DECREF(code);
    OB_DECREF(meth);
  } else if (kind == MBR_LABEL) {
    enum_add_label(type, name, data);
  } else if (kind == MBR_IFUNC) {
    Object *proto = proto_new(name, data);
    type_add_ifunc(type, proto);
    OB_DECREF(proto);
  } else {
    panic("_load_mbr__: not implemented");
  }
}

static void _load_base_(TypeDesc *desc, void *arg)
{
  TypeObject *type = arg;
  expect(desc->kind == TYPE_KLASS);

  Object *tp;
  if (desc->klass.path != NULL) {
    Object *m = module_load(desc->klass.path);
    tp = module_get(m, desc->klass.type);
    OB_DECREF(m);
  } else {
    tp = module_get(mo, desc->klass.type);
  }

  expect(tp != NULL);
  expect(type_check(tp));
  type_add_base(type, tp);
  OB_DECREF(tp);
}

static
void _load_type_(char *name, int baseidx, int mbridx, Image *image, void *arg)
{
  TypeObject *type = arg;
  expect(!strcmp(name, type->name));
  int size = _size_(image, ITEM_CONST);
  Object *consts = NULL;
  if (size > 0)
    consts = tuple_new(size);
  image_load_consts(image, _load_const_, consts);
  type->consts = consts;
  image_load_mbrs(image, mbridx, _load_mbr_, type);
  if (baseidx >= 0)
    image_load_bases(image, baseidx, _load_base_, type);
}

static TypeObject *class_from_symbol(Symbol *sym)
{
  TypeObject *tp = type_new(NULL, sym->name, TPFLAGS_CLASS);

  Image *image = image_new("__main__");
  type_write_image(sym, image);
  image_load_class(image, 0, _load_type_, tp);
  image_free(image);

  if (type_ready(tp) < 0)
    panic("Cannot initalize '%s' type.", sym->name);
  return tp;
}

static TypeObject *trait_from_symbol(Symbol *sym)
{
  TypeObject *tp = type_new(NULL, sym->name, TPFLAGS_TRAIT);

  Image *image = image_new("__main__");
  type_write_image(sym, image);
  image_load_trait(image, 0, _load_type_, tp);
  image_free(image);

  if (type_ready(tp) < 0)
    panic("Cannot initalize '%s' type.", sym->name);
  return tp;
}

static TypeObject *enum_from_symbol(Symbol *sym)
{
  TypeObject *tp = enum_type_new(NULL, sym->name);

  Image *image = image_new("__main__");
  type_write_image(sym, image);
  image_load_enum(image, 0, _load_type_, tp);
  image_free(image);

  if (type_ready(tp) < 0)
    panic("Cannot initalize '%s' type.", sym->name);

  // add __eq__ to enum symbol
  Vector *args = vector_new();
  vector_push_back(args, desc_from_any);
  TypeDesc *ret = desc_from_bool;
  TypeDesc *proto = desc_from_proto(args, ret);
  TYPE_DECREF(ret);
  stable_add_func(sym->type.stbl, "__eq__", proto);
  TYPE_DECREF(proto);

  return tp;
}

static void add_symbol_to_module(Symbol *sym, Object *ob)
{
  switch (sym->kind) {
  case SYM_CONST:
  case SYM_VAR: {
    Object *field = field_new(sym->name, sym->desc);
    Field_SetFunc(field, field_default_setter, field_default_getter);
    module_add_var(ob, field, NULL);
    OB_DECREF(field);
    break;
  }
  case SYM_FUNC: {
    Object *code = code_from_symbol(sym);
    Object *meth = method_new(sym->name, code);
    module_add_func(ob, meth);
    code_set_module(code, ob);
    OB_DECREF(code);
    OB_DECREF(meth);
    break;
  }
  case SYM_CLASS: {
    TypeObject *tp = class_from_symbol(sym);
    module_add_type(ob, tp);
    OB_DECREF(tp);
    break;
  }
  case SYM_TRAIT: {
    TypeObject *tp = trait_from_symbol(sym);
    module_add_type(ob, tp);
    OB_DECREF(tp);
    break;
  }
  case SYM_ENUM: {
    TypeObject *tp = enum_from_symbol(sym);
    module_add_type(ob, tp);
    OB_DECREF(tp);
    break;
  }
  default: {
    panic("invalid branch:%d", sym->kind);
    break;
  }
  }
}

void cmd_eval_stmt(ParserState *ps, Stmt *stmt)
{
  if (stmt == NULL)
    return;

  parser_enter_scope(ps, SCOPE_MODULE, 0);
  ps->u->stbl = mod.stbl;
  ps->u->sym = modSym;
  parse_stmt(ps, stmt);
  parser_exit_scope(ps);

  if (has_error(ps)) {
    if (cursym != NULL) {
      stable_remove(mod.stbl, cursym->name);
      symbol_decref(cursym);
      cursym = NULL;
    }
  } else {
    if (cursym != NULL) {
      add_symbol_to_module(cursym, mo);
      cursym = NULL;
    }
    if (evalsym->func.codeblock != NULL) {
      KoalaState *ks = pthread_getspecific(kskey);
      if (ks == NULL) {
        kstate.top = -1;
        pthread_setspecific(kskey, &kstate);
      }
      Object *code = code_from_symbol(evalsym);
      code_set_module(code, mo);
      struct timeval past, future, cost;
      gettimeofday(&past, NULL);
      koala_evalcode(code, NULL, NULL, NULL);
      gettimeofday(&future, NULL);
      timersub(&future, &past, &cost);
      //printf("cost: %ld.%06lds\n", cost.tv_sec, cost.tv_usec);
      expect(kstate.top == -1);
      OB_DECREF(code);
    }
  }
  codeblock_free(evalsym->func.codeblock);
  evalsym->func.codeblock = NULL;
}

static int empty(char *buf, int size)
{
  int n;
  char ch;
  for (n = 0; n < size; ++n) {
    ch = buf[n];
    if (ch != '\n' && ch != ' ' &&
        ch != '\t' && ch != '\r')
      return 0;
  }
  return 1;
}

static volatile int lexwrap = -1;
/*
When the scanner receives an end-of-file indication from YY_INPUT, it then
checks the yywrap() function.

If yywrap() returns false (zero), then it is assumed that the function has gone
ahead and set up yyin to point to another input file, and scanning continues.

If it returns true (non-zero), then the scanner terminates.
*/
int yywrap(yyscan_t yyscanner)
{
  int ret = lexwrap;
  lexwrap = -1;
  return ret;
}

int interactive(ParserState *ps, char *buf, int size)
{
  if (halt) {
    return 0;
  }

  char *line;

  if (ps->more) {
    line = readline(MORE_PROMPT);
  } else {
    /* clear error */
    ps->errors = 0;
    line = readline(PROMPT);
  }

  if (!line) {
    if (ferror(stdin))
      clearerr(stdin);
    //clear_history();
    ps->quit = 1;
    return 0;
  }

  /* add history of readline */
  //add_history(line);

  int res = textblock_input(line, buf);
  if (res) {
    free(line);
    int state = textblock_state();
    if (state == TB_CONT) {
      ps->more++;
    }

    int len = strlen(buf);
    if (len == 0) {
      lexwrap = 0;
      return 0;
    }

    // flex bug? leave last one char in buffer
    buf[len++] = '\r';
    return len;
  } else {
    strcpy(buf, line);
    free(line);
    int len = strlen(buf);

    // flex bug? leave last one char in buffer
    buf[len++] = '\r';

    if (!empty(buf, len)) {
      ps->more++;
    }

    return len;
  }
}
