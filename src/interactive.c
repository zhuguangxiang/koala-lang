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
#include <pthread.h>
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
#include <readline/readline.h>
#include <readline/history.h>

#define PROMPT      "> "
#define MORE_PROMPT ". "

static void show_banner(void)
{
  printf("koala %s (%s)\n", KOALA_VERSION, __DATE__);

  struct utsname sysinfo;
  if (!uname(&sysinfo)) {
    printf("[GCC %d.%d.%d] on %s/%s %s\n",
           __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__,
           sysinfo.sysname, sysinfo.machine, sysinfo.release);
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

  ps.interactive = 1;
  ps.filename = "stdin";
  ps.module = &mod;
  vector_init(&ps.ustack);

  mo = Module_New("__main__");

  pthread_key_create(&kskey, NULL);
}

static void fini_cmdline_env(void)
{
  stable_free(mod.stbl);
  mod.stbl = NULL;
  symbol_decref(modSym);
  modSym = NULL;
  OB_DECREF(mo);
  mo = NULL;
  expect(vector_size(&ps.ustack) == 0);
  vector_fini(&ps.ustack);
}

yyscan_t scanner;

void Koala_ReadLine(void)
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

static void _get_const_(void *val, int kind, int index, void *arg)
{
  Object *tuple = arg;
  Object *ob;

  if (kind == CONST_LITERAL) {
    ob = new_literal(val);
  } else {
    expect(kind == CONST_TYPE);
    ob = new_descob(val);
  }

  Tuple_Set(tuple, index, ob);
  OB_DECREF(ob);
}

static Object *get_code_object(Symbol *sym)
{
  Object *ob;
  Object *consts;
  Image *image;
  uint8_t *code;
  int locals;
  int size;
  ByteBuffer buf;

  expect(sym->kind == SYM_FUNC);

  image = Image_New("__main__");
  bytebuffer_init(&buf, 32);

  code_gen(sym->func.codeblock, image, &buf);

#if !defined(NLog)
  Image_Show(image);
#endif

  locals = vector_size(&sym->func.locvec);

  size = Image_Const_Count(image);
  consts = Tuple_New(size);
  Image_Get_Consts(image, _get_const_, consts);
  size = bytebuffer_toarr(&buf, (char **)&code);

  ob = code_new(sym->name, sym->desc, locals, code, size);
  code_set_consts(ob, consts);
  OB_DECREF(consts);
  kfree(code);

  bytebuffer_fini(&buf);
  Image_Free(image);
  return ob;
}

int cmd_add_const(ParserState *ps, Ident id, Type type)
{
  cursym = stable_add_const(mod.stbl, id.name, type.desc);
  if (cursym != NULL) {
    cursym->k.typesym = get_desc_symbol(ps, type.desc);
    return 0;
  } else {
    return -1;
  }
}

int cmd_add_var(ParserState *ps, Ident id, Type type)
{
  cursym = stable_add_var(mod.stbl, id.name, type.desc);
  if (cursym != NULL) {
    cursym->var.typesym = get_desc_symbol(ps, type.desc);
    return 0;
  } else {
    return -1;
  }
}

int cmd_add_func(ParserState *ps, char *name, Vector *idtypes, Type ret)
{
  Vector *vec = NULL;
  if (vector_size(idtypes) > 0)
    vec = vector_new();
  IdType *item;
  vector_for_each(item, idtypes) {
    vector_push_back(vec, TYPE_INCREF(item->type.desc));
  }
  TypeDesc *proto = desc_from_proto(vec, ret.desc);
  cursym = stable_add_func(mod.stbl, name, proto);
  TYPE_DECREF(proto);
  return cursym != NULL ? 0 : -1;
}

static void add_symbol_to_module(Symbol *sym, Object *ob)
{
  switch (sym->kind) {
  case SYM_CONST: {
    break;
  }
  case SYM_VAR: {
    Object *field = field_new(sym->name, sym->desc);
    Field_SetFunc(field, field_default_setter, field_default_getter);
    Module_Add_Var(ob, field);
    OB_DECREF(field);
    break;
  }
  case SYM_FUNC: {
    Object *code = get_code_object(sym);
    Object *meth = Method_New(sym->name, code);
    Module_Add_Func(ob, meth);
    OB_DECREF(code);
    OB_DECREF(meth);
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
      Object *code = get_code_object(evalsym);
      Koala_EvalCode(code, mo, NULL);
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

int interactive(ParserState *ps, char *buf, int size)
{
  char *line;
  /* TAB as insert, not completion */
  rl_bind_key('\t', rl_insert);
  /* set TAB width as 2 spaces */
  rl_generic_bind(ISMACR, "\t", "  ", rl_get_keymap());

  if (ps->more) {
    line = readline(MORE_PROMPT);
  } else {
    /* clear error */
    ps->errnum = 0;
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

  strcpy(buf, line);
  int len = strlen(buf);
  /* apeend newline */
  buf[len++] = '\n';
  /* flex bug? leave last one char in buffer */
  buf[len++] = ' ';

  if (!empty(buf, len)) {
    ps->more++;
  }

  free(line);
  return len;
}
