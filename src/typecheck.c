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

Symbol *get_desc_symbol_ex(ParserState *ps, TypeDesc *type);
void typecheck(ParserState *ps, TypeDesc *type);

void typecheckarg(ParserState *ps, Symbol *sym, TypeDesc *base)
{
  debug("check symbol '%s'", sym->name);

  if (base->kind == TYPE_PARAREF) {
    return;
  }

  expect(base->kind == TYPE_KLASS || desc_isany(base));

  TypeDesc *desc = sym->desc;
  if (desc->kind == TYPE_PARADEF) {
    debug("'%s' is type parameter", sym->name);

    return;
  }

  if (desc_check(desc, base))
    return;

  // check bases class and traits
  Symbol *item;
  vector_for_each_reverse(item, &sym->type.lro) {
    if (desc_isany(item->desc))
      continue;
    if (desc_check(item->desc, base))
      return;
  }

  error("not matched");
}

void typecheckargs(ParserState *ps, Vector *tptypes, Vector *args)
{
  Symbol *sym;
  TypeDesc *arg;
  TypeDesc *item;
  vector_for_each(item, tptypes) {
    expect(item->kind == TYPE_PARADEF);
    debug("check type paramter '%s'", item->paradef.name);
    arg = vector_get(args, idx);
    sym = get_desc_symbol_ex(ps, arg);
    if (sym == NULL) {
      error("'%s' is not defined", arg->klass.type);
    } else {
      TypeDesc *bnd;
      vector_for_each(bnd, item->paradef.types) {
        typecheckarg(ps, sym, bnd);
      }
    }
    typecheck(ps, arg);
  }
}

// check : List<Pet<Dog<Cat<T>>>, Box<an.Pet<Cat<T>>>>
// find type and check its type args ara matched by its type parameters
// recursively
void typecheck(ParserState *ps, TypeDesc *type)
{
  if (type->kind != TYPE_KLASS) {
    debug("[typecheck] not class/triat");
    return;
  }

  Symbol *sym = get_desc_symbol_ex(ps, type);
  if (sym == NULL) {
    error("cannot find symbol '%s'", type->klass.type);
    return;
  }

  int argsize = vector_size(type->types);
  TypeDesc *stype = sym->desc;
  if (stype->kind == TYPE_PARADEF) {
    debug("[typecheck] typeparadef");
    if (argsize > 0) {
      error("typepara needs no args");
      return;
    }
    //*real = desc_from_pararef(stype->paradef.name, stype->paradef.index);
    return;
  }

  int parasize = vector_size(stype->paras);
  if (argsize != parasize) {
    error("count of args and paras are not equal");
    return;
  }

  typecheckargs(ps, stype->paras, type->types);
}
