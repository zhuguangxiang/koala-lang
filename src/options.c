/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "options.h"
#include "stringbuf.h"
#include "mem.h"
#include "stringex.h"
#include "log.h"

LOGGER(0)

struct namevalue *namevalue_new(char *name, char *value)
{
  struct namevalue *nv = Malloc(sizeof(struct namevalue));
  assert(nv != NULL);
  nv->name = name;
  nv->value = value;
  return nv;
}

void namevalue_free(struct namevalue *nv)
{
  Mfree(nv->name);
  Mfree(nv->value);
  Mfree(nv);
}

struct namevalue *parse_namevalue(char *opt, Options *opts, char *prog)
{
  char *name = opt;
  char *eq = strchr(opt, '=');
  if (eq == NULL || eq[1] == '\0') {
    fprintf(stderr, "Error: invalid <name=value>.\n\n");
    opts->usage(prog);
    exit(0);
  }
  return namevalue_new(strndup(name, eq - name), string_dup(eq + 1));
}

void parse_options(int argc, char *argv[], Options *opts)
{
  extern char *optarg;
  extern int optind;
  int opt;

  for (int i = 0; i < argc; i++) {
    Log_Printf("[%d]: %s\n", i, argv[i]);
  }

  while ((opt = getopt(argc, argv, "s:p:o:D:e:vh")) != -1) {
    switch (opt) {
    case 's':
      opts->srcpath = optarg;
    break;
    case 'p':
      Vector_Append(&opts->pathes, string_dup(optarg));
    break;
    case 'o':
      opts->outpath = string_dup(optarg);
    break;
    case 'D':
      Vector_Append(&opts->nvs, parse_namevalue(optarg, opts, argv[0]));
    break;
    case 'e':
      opts->encoding = string_dup(optarg);
    break;
    case 'v':
      opts->version();
      exit(0);
    break;
    case 'h':
      /* fall-through */
    case '?':
      opts->usage(argv[0]);
      exit(0);
    break;
    default:
      fprintf(stderr, "Error: invalid option '%c'.\n\n", opt);
      opts->usage(argv[0]);
      exit(0);
    break;
    }
  }

  while (optind < argc) {
    if (!strcmp(argv[optind], "?")) {
      opts->usage(argv[0]);
      exit(0);
    }
    Vector_Append(&opts->names, string_dup(argv[optind++]));
  }

  if (Vector_Size(&opts->names) == 0) {
    fprintf(stderr, "Error: please specify pacakge(s).\n\n");
    opts->usage(argv[0]);
    exit(0);
  }

  DeclareStringBuf(buf);
  char *s;
  Vector_ForEach(s, &opts->pathes) {
    /* last one, no colon */
    if (i + 1 == Vector_Size(&opts->pathes))
      StringBuf_Append_CStr(buf, s);
    else
      StringBuf_Format_CStr(buf, "#:", s);
  }
  if (buf.data != NULL)
    opts->pathstrings = string_dup(buf.data);
  FiniStringBuf(buf);
}

int init_options(Options *opts, void (*usage)(char *), void (*version)(void))
{
  memset(opts, 0, sizeof(Options));
  Vector_Init(&opts->pathes);
  Vector_Init(&opts->nvs);
  Vector_Init(&opts->names);
  opts->usage = usage;
  opts->version = version;
  return 0;
}

static void free_string_func(void *item, void *arg)
{
  Mfree(item);
}

static void free_namevalue_func(void *item, void *arg)
{
  namevalue_free(item);
}

void fini_options(Options *opts)
{
  Mfree(opts->srcpath);
  Mfree(opts->outpath);
  Mfree(opts->pathstrings);
  Vector_Fini(&opts->pathes, free_string_func, NULL);
  Vector_Fini(&opts->nvs, free_namevalue_func, NULL);
  Vector_Fini(&opts->names, free_string_func, NULL);
}

int options_number(Options *opts)
{
  int num = Vector_Size(&(opts)->pathes) + Vector_Size(&(opts)->nvs);
  if (opts->srcpath != NULL)
    num++;
  if (opts->outpath != NULL)
    num++;
  return 2 * num;
}

void options_toarray(Options *opts, char *array[], int ind)
{
  if (opts->srcpath != NULL) {
    array[ind++] = string_dup("-s");
    array[ind++] = string_dup(opts->srcpath);
  }

  char *path;
  Vector_ForEach(path, &opts->pathes) {
    array[ind++] = string_dup("-p");
    array[ind++] = string_dup(path);
  }

  if (opts->outpath != NULL) {
    array[ind++] = string_dup("-o");
    array[ind++] = string_dup(opts->outpath);
  }

  char *buf;
  struct namevalue *nv;
  Vector_ForEach(nv, &opts->nvs) {
    array[ind++] = string_dup("-D");
    buf = Malloc(strlen(nv->name) + strlen(nv->value) + 2);
    sprintf(buf, "%s=%s", nv->name, nv->value);
    array[ind++] = buf;
  }
}

void show_options(Options *opts)
{
  Log_Printf("srcput: %s\n", opts->srcpath);
  Log_Printf("output: %s\n", opts->outpath);

  char *s;

  Log_Puts("pathes:");
  Vector_ForEach(s, &opts->pathes) {
    Log_Printf("  [%d]: %s\n", i, s);
  }

  Log_Puts("name-values:");
  Vector_ForEach(s, &opts->nvs) {
    Log_Printf("  [%d]: %s\n", i, s);
  }

  Log_Puts("names:");
  Vector_ForEach(s, &opts->names) {
    Log_Printf(" [%d]: %s\n", i, s);
  }

  Log_Puts("\n");
}
