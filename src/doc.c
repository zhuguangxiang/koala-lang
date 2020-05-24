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

#include "compile.h"

#define DOC_MAX_LEN 1024

static char *doc_lines[DOC_MAX_LEN];
static int doc_lens[DOC_MAX_LEN];
static int doc_total;
static int doc_index;
int docflag;

void doc_add(char *txt)
{
  expect(doc_index < DOC_MAX_LEN);
  doc_lines[doc_index] = txt;
  int len = strlen(txt);
  doc_lens[doc_index] = len;
  doc_index++;
  doc_total += len + 1;
}

void doc_clear(void)
{
  char *txt;
  for (int i = 0; i < doc_index; ++i) {
    txt = doc_lines[i];
    if (txt != NULL) free(txt);
    doc_lines[i] = NULL;
    doc_lens[i] = 0;
  }
  doc_index = 0;
  doc_total = 0;
}

char *doc_get(void)
{
  if (doc_total <= 0) return NULL;
  char *txt = malloc(doc_total);
  char *ptr = txt;
  char *doc;
  int len;
  for (int i = 0; i < doc_index; ++i) {
    len = doc_lens[i];
    doc = doc_lines[i];
    memcpy(ptr, doc, len);
    ptr[len] = '\n';
    ptr += len + 1;
    free(doc);
    doc_lines[i] = NULL;
    doc_lens[i] = 0;
  }
  doc_index = 0;
  doc_total = 0;
  return txt;
}

/* koala --doc a/b/foo.kl [a/b/foo] */
void koala_doc(char *path)
{
  docflag = 1;
  Module mod = {0};
  build_module(path, &mod);
  if (mod.errors == 0) {
    HASHMAP_ITERATOR(iter, &mod.stbl->table);
    Symbol *s;
    iter_for_each(&iter, s) {
      if (s->doc != NULL) printf("DOC:\n%s\n", s->doc);
    }
  }
  release_module(&mod);
  docflag = 0;
}
