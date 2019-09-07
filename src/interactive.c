/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
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

static Symbol *sym;
static Symbol *funcsym;
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
  modSym->mod.ptr = &mod;

  TypeDesc *strdesc = desc_from_base(BASE_STR);
  TypeDesc *desc = desc_from_proto(NULL, strdesc);
  stable_add_func(mod.stbl, "__name__", desc);
  TYPE_DECREF(strdesc);
  TYPE_DECREF(desc);

  desc = desc_from_proto(NULL, NULL);
  funcsym = stable_add_func(mod.stbl, "__init__", desc);
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
  if (kind == CONST_LITERAL)
    ob = New_Literal(val);
  else
    ob = New_Desc(val);
  Tuple_Set(tuple, index, ob);
  OB_DECREF(ob);
}

static Object *getcode(CodeBlock *block)
{
  Object *ob;
  CodeObject *co;
  Object *consts;
  Image *image;
  uint8_t *code;
  int size;
  ByteBuffer buf;

  image = Image_New("__main__");
  bytebuffer_init(&buf, 32);

  code_gen(block, image, &buf);
#if !defined(NDEBUG)
  Image_Show(image);
#endif
  size = Image_Const_Count(image);
  consts = Tuple_New(size);
  Image_Get_Consts(image, _get_const_, consts);
  TypeDesc *proto = desc_from_proto(NULL, NULL);
  size = bytebuffer_toarr(&buf, (char **)&code);
  ob = Code_New("__code__", proto, 0, code, size);
  TYPE_DECREF(proto);
  kfree(code);
  co = (CodeObject *)ob;
  co->consts = OB_INCREF(consts);
  OB_DECREF(consts);

  bytebuffer_fini(&buf);
  Image_Free(image);
  return ob;
}

void Cmd_Add_Const(Ident id, Type type)
{
  sym = stable_add_const(mod.stbl, id.name, type.desc);
}

void Cmd_Add_Var(Ident id, Type type, int freevar)
{
  sym = stable_add_var(mod.stbl, id.name, type.desc);
  sym->var.freevar = freevar;
}

void Cmd_Add_Func(char *name, TypeDesc *desc)
{
  if (desc == NULL)
    desc = desc_from_proto(NULL, NULL);
  sym = stable_add_func(mod.stbl, name, desc);
}

static void add_symbol_to_mobject(Symbol *sym, Object *ob)
{
  if (sym == NULL)
    return;

  switch (sym->kind) {
  case SYM_CONST: {
    break;
  }
  case SYM_VAR: {
    Object *field = Field_New(sym->name, sym->desc);
    Field_SetFunc(field, Field_Default_Set, Field_Default_Get);
    Module_Add_Var(ob, field);
    OB_DECREF(field);
    break;
  }
  case SYM_FUNC: {
    Object *code = getcode(sym->func.code);
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

void Cmd_EvalStmt(ParserState *ps, Stmt *stmt)
{
  if (stmt == NULL)
    return;

  parser_enter_scope(ps, SCOPE_MODULE);
  ps->u->stbl = mod.stbl;
  ps->u->sym = modSym;
  parse_stmt(ps, stmt);
  parser_exit_scope(ps);

  if (has_error(ps)) {
    codeblock_free(funcsym->func.code);
    funcsym->func.code = NULL;
  } else {
    add_symbol_to_mobject(sym, mo);
    sym = NULL;
    if (funcsym->func.code != NULL) {
      KoalaState *ks = pthread_getspecific(kskey);
      if (ks == NULL) {
        kstate.top = -1;
        pthread_setspecific(kskey, &kstate);
      }
      Object *code = getcode(funcsym->func.code);
      Koala_EvalCode(code, mo, NULL);
      OB_DECREF(code);
      codeblock_free(funcsym->func.code);
      funcsym->func.code = NULL;
    }
  }
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

  if (empty(buf, len)) {
    ps->more = 0;
  } else {
    ps->more++;
  }

  free(line);
  return len;
}
