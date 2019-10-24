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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "parser.h"
#include "koala_yacc.h"
#include "koala_lex.h"
#include "vector.h"
#include "log.h"

int file_input(ParserState *ps, char *buf, int size, FILE *in)
{
  errno = 0;
  int result = 0;
  while ((result = (int)fread(buf, 1, (size_t)size, in)) == 0 && ferror(in)) {
    if (errno != EINTR) {
      error("Input in scanner failed.");
      break;
    }
    errno = 0;
    clearerr(in);
  }
  return result;
}

int exist(char *path)
{
  struct stat sb;
  if (stat(path, &sb) < 0)
    return 0;
  return 1;
}

int isdotkl(char *filename)
{
  char *dot = strrchr(filename, '.');
  if (!dot || strlen(dot) != 3)
    return 0;
  if (dot[1] == 'k' && dot[2] == 'l')
    return 1;
  return 0;
}

int isdotklc(char *filename)
{
  char *dot = strrchr(filename, '.');
  if (!dot || strlen(dot) != 4)
    return 0;
  if (dot[1] == 'k' && dot[2] == 'l' && dot[3] == 'c')
    return 1;
  return 0;
}

int validate_srcfile(char *path)
{
  char *filename = strrchr(path, '/');
  if (!filename)
    filename = path;

  if (!isdotkl(filename)) {
    error("%s: Not a valid koala source file.", path);
    return 0;
  }

  struct stat sb;

  char *dir = str_ndup(path, strlen(path) - 3);
  if (!stat(dir, &sb)) {
    kfree(dir);
    error("%s: The same name file or directory exist.", path);
    return 0;
  } else {
    kfree(dir);
  }

  dir = strrchr(path, '/');
  if (!dir) {
    if (!stat("./__init__.kl", &sb)) {
      error("Not allowed '__init__.kl' exist, "
             "when single source file '%s' is compiled.", path);
      return 0;
    }
  } else {
    int extra = strlen("./__init__.kl");
    dir = str_ndup_ex(path, dir - path + 1, extra);
    strcat(dir, "./__init__.kl");
    if (!stat(dir, &sb)) {
      kfree(dir);
      error("Not allowed '__init__.kl' exist, "
             "when single source file '%s' is compiled.", path);
      return 0;
    }
    kfree(dir);
  }

  return 1;
}

int comp_add_const(ParserState *ps, Ident id, Type type)
{
  Module *mod = ps->module;
  Symbol *sym = stable_add_const(mod->stbl, id.name, type.desc);
  if (sym != NULL) {
    sym->k.typesym = get_desc_symbol(mod, type.desc);
    return 0;
  } else {
    return -1;
  }
}

int comp_add_var(ParserState *ps, Ident id, Type type)
{
  Module *mod = ps->module;
  Symbol *sym = stable_add_var(mod->stbl, id.name, type.desc);
  if (sym != NULL) {
    sym->var.typesym = get_desc_symbol(mod, type.desc);
    return 0;
  } else {
    return -1;
  }
}

int comp_add_func(ParserState *ps, char *name, Vector *idtypes, Type ret)
{
  Module *mod = ps->module;
  TypeDesc *proto = parse_proto(idtypes, &ret);
  Symbol *sym = stable_add_func(mod->stbl, name, proto);
  TYPE_DECREF(proto);
  return sym != NULL ? 0 : -1;
}

static void comp_visit_type(Symbol *sym, Vector *body)
{
  STable *stbl = sym->type.stbl;
  Stmt *s;
  vector_for_each(s, body) {
    if (s->kind == VAR_KIND) {
      stable_add_var(stbl, s->vardecl.id.name, s->vardecl.type.desc);
    } else if (s->kind == FUNC_KIND) {
      Vector *idtypes = s->funcdecl.idtypes;
      Type *ret = &s->funcdecl.ret;
      TypeDesc *proto = parse_proto(idtypes, ret);
      stable_add_func(stbl, s->funcdecl.id.name, proto);
      TYPE_DECREF(proto);
    } else if (s->kind == IFUNC_KIND) {
      Vector *idtypes = s->funcdecl.idtypes;
      Type *ret = &s->funcdecl.ret;
      TypeDesc *proto = parse_proto(idtypes, ret);
      stable_add_ifunc(stbl, s->funcdecl.id.name, proto);
      TYPE_DECREF(proto);
    } else {
      panic("invalid type's body statement: %d", s->kind);
    }
  }
}

static void comp_visit_label(ParserState *ps, Symbol *sym, Vector *labels)
{
  STable *stbl = sym->type.stbl;
  Symbol *lblsym;
  EnumLabel *label;
  vector_for_each(label, labels) {
    lblsym = stable_add_label(stbl, label->id.name);
    if (lblsym == NULL) {
      syntax_error(ps, label->id.row, label->id.col,
                   "enum label '%s' duplicated.", label->id.name);
    } else {
      lblsym->label.esym = sym;
      lblsym->desc = desc_from_label(sym->desc, label->types);
      lblsym->label.types = label->types;
      label->types = NULL;
    }
  }
}

int comp_add_type(ParserState *ps, Stmt *stmt)
{
  Module *mod = ps->module;
  Symbol *sym = NULL;
  Symbol *any;
  switch (stmt->kind) {
  case CLASS_KIND: {
    sym = stable_add_class(mod->stbl, stmt->class_stmt.id.name);
    any = find_from_builtins("Any");
    ++any->refcnt;
    vector_push_back(&sym->type.bases, any);
    comp_visit_type(sym, stmt->class_stmt.body);
    break;
  }
  case TRAIT_KIND: {
    sym = stable_add_trait(mod->stbl, stmt->class_stmt.id.name);
    any = find_from_builtins("Any");
    ++any->refcnt;
    vector_push_back(&sym->type.bases, any);
    comp_visit_type(sym, stmt->class_stmt.body);
    break;
  }
  case ENUM_KIND: {
    sym = stable_add_enum(mod->stbl, stmt->enum_stmt.id.name);
    any = find_from_builtins("Any");
    ++any->refcnt;
    vector_push_back(&sym->type.bases, any);
    comp_visit_label(ps, sym, stmt->enum_stmt.mbrs.labels);
    comp_visit_type(sym, stmt->enum_stmt.mbrs.methods);
    break;
  }
  default:
    panic("invalid stmt kind %d", stmt->kind);
    break;
  }
  return sym != NULL ? 0 : -1;
}

void comp_add_stmt(ParserState *ps, Stmt *s)
{
  if (s != NULL)
    vector_push_back(&ps->stmts, s);
  else
    warn("stmt is null");
}

static void build_ast(char *path, Module *mod)
{
  FILE *in = fopen(path, "r");
  if (in == NULL) {
    error("%s: No such file or directory.", path);
    return;
  }

  ParserState *ps = new_parser(path);
  ps->interactive = 0;
  ps->module = mod;
  vector_push_back(&mod->pss, ps);

  debug("\x1b[34m----STARTING BUILDING AST------\x1b[0m");
  yyscan_t scanner;
  yylex_init_extra(ps, &scanner);
  yyset_in(in, scanner);
  yyparse(ps, scanner);
  yylex_destroy(scanner);
  debug("\x1b[34m----END OF BUILDING AST--------\x1b[0m");

  mod->errors += ps->errors;
  fclose(in);
}

static void parse_ast(ParserState *ps)
{
  debug("\x1b[32m----STARTING SEMANTIC ANALYSIS & CODE GEN----\x1b[0m");
  parser_enter_scope(ps, SCOPE_MODULE, 0);
  ps->u->stbl = ps->module->stbl;
  ps->u->sym = ps->module->sym;
  Stmt *stmt;
  vector_for_each(stmt, &ps->stmts) {
    parse_stmt(ps, stmt);
    if (ps->errors >= MAX_ERRORS) {
      break;
    }
  }
  parser_exit_scope(ps);
  debug("\x1b[32m----END OF SEMANTIC ANALYSIS & CODE GEN------\x1b[0m");
}

static void write_image(Module *mod)
{
  debug("\x1b[32m----STARTING GENERATING IMAGE----\x1b[0m");
  Image *image = image_new(mod->name);
  stable_write_image(mod->stbl, image);
  image_finish(image);
#if !defined(NLog)
  image_show(image);
#endif
  STRBUF(klcfile);
  if (isdotkl(mod->name)) {
    strbuf_append(&klcfile, mod->name);
    strbuf_append_char(&klcfile, 'c');
  } else {
    strbuf_append(&klcfile, mod->name);
    strbuf_append(&klcfile, ".klc");
  }
  char *path = strbuf_tostr(&klcfile);
  debug("write image to '%s'", path);
  image_write_file(image, path);
  image_free(image);
  strbuf_fini(&klcfile);
  debug("\x1b[32m----END OF GENERATING IMAGE------\x1b[0m");
}

/* koala -c a/b/foo.kl [a/b/foo] */
void koala_compile(char *path)
{
  Module mod = {0};
  Symbol *modSym;

  mod.name = path;
  mod.stbl = stable_new();
  vector_init(&mod.pss);

  modSym = symbol_new(mod.name, SYM_MOD);
  modSym->desc = desc_from_klass("lang", "Module");
  modSym->mod.ptr = &mod;

  TypeDesc *desc = desc_from_proto(NULL, NULL);
  Symbol *initsym = stable_add_func(mod.stbl, "__init__", desc);
  TYPE_DECREF(desc);
  mod.initsym = initsym;
  mod.sym = modSym;

  if (isdotkl(path)) {
    // single source file, build one ast
    build_ast(path, &mod);
  } else {
    /* module directory */
  }

  // parse all source files in the same directory as one module
  ParserState *ps;
  vector_for_each(ps, &mod.pss) {
    if (!has_error(ps)) {
      parse_ast(ps);
      mod.errors += ps->errors;
    }
    free_parser(ps);
  }

  // write image file
  if (mod.errors == 0) {
    write_image(&mod);
  }

  vector_fini(&mod.pss);
  stable_free(mod.stbl);
  symbol_decref(modSym);
}
