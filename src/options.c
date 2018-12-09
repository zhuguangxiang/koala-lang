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
#include "log.h"

struct namevalue *namevalue_new(char *name, char *value)
{
  struct namevalue *nv = malloc(sizeof(struct namevalue));
  assert(nv != NULL);
  nv->name = name;
  nv->value = value;
  return nv;
}

void namevalue_free(struct namevalue *nv)
{
  free(nv->name);
  free(nv->value);
  free(nv);
}

struct namevalue *parse_namevalue(char *opt, struct options *opts, char *prog)
{
  char *name = opt;
  char *eq = strchr(opt, '=');
  if (eq == NULL || eq[1] == '\0') {
    fprintf(stderr, "Error: invalid <name=value>.\n\n");
    opts->usage(prog);
    exit(0);
  }
  return namevalue_new(strndup(name, eq - name), strdup(eq + 1));
}

void parse_options(int argc, char *argv[], struct options *opts)
{
  extern char *optarg;
  extern int optind;
  int opt;

  for (int i = 0; i < argc; i++) {
    printf("[%d]: %s\n", i, argv[i]);
  }

  while ((opt = getopt(argc, argv, "s:p:o:D:vh")) != -1) {
    switch (opt) {
      case 's':
        opts->srcpath = optarg;
        break;
      case 'p':
        Vector_Append(&opts->pathes, optarg);
        break;
      case 'o':
        opts->outpath = strdup(optarg);
        break;
      case 'D':
        Vector_Append(&opts->nvs, parse_namevalue(optarg, opts, argv[0]));
        break;
      case 'v':
        opts->version();
        exit(0);
        break;
      case 'h':
        /* fall-through */
      case '?':
        opts->usage(argv[0]);
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
    Vector_Append(&opts->names, strdup(argv[optind++]));
  }

  if (Vector_Size(&opts->names) == 0) {
    fprintf(stderr, "Error: please specify pacakge(s).\n\n");
    opts->usage(argv[0]);
    exit(0);
  }
}

int init_options(struct options *opts)
{
  memset(opts, 0, sizeof(struct options));
  Vector_Init(&opts->pathes);
  Vector_Init(&opts->nvs);
  Vector_Init(&opts->names);
  return 0;
}

void fini_options(struct options *opts)
{
  if (opts->srcpath != NULL)
    free(opts->srcpath);

  char *path;
  Vector_ForEach(path, &opts->pathes)
    free(path);

  if (opts->outpath != NULL)
    free(opts->outpath);

  struct namevalue *nv;
  Vector_ForEach(nv, &opts->nvs)
    namevalue_free(nv);

  char *s;
  Vector_ForEach(s, &opts->names)
    free(s);
}

int options_number(struct options *opts)
{
  int num = Vector_Size(&(opts)->pathes) + Vector_Size(&(opts)->nvs);
  if (opts->srcpath != NULL)
    num++;
  if (opts->outpath != NULL)
    num++;
  return 2 * num;
}

void options_toarray(struct options *opts, char *array[], int ind)
{
  if (opts->srcpath != NULL) {
    array[ind++] = strdup("-s");
    array[ind++] = strdup(opts->srcpath);
  }

  char *path;
  Vector_ForEach(path, &opts->pathes) {
    array[ind++] = strdup("-p");
    array[ind++] = strdup(path);
  }

  if (opts->outpath != NULL) {
    array[ind++] = strdup("-o");
    array[ind++] = strdup(opts->outpath);
  }

  char *buf;
  struct namevalue *nv;
  Vector_ForEach(nv, &opts->nvs) {
    array[ind++] = strdup("-D");
    buf = malloc(strlen(nv->name) + strlen(nv->value) + 2);
    sprintf(buf, "%s=%s", nv->name, nv->value);
    array[ind++] = buf;
  }
}

void show_options(struct options *opts)
{
  printf("srcput: %s\n", opts->srcpath);
  printf("output: %s\n", opts->outpath);

  char *s;

  printf("pathes:\n");
  Vector_ForEach(s, &opts->pathes) {
    printf("  [%d]: %s\n", i, s);
  }

  printf("name-values:\n");
  Vector_ForEach(s, &opts->nvs) {
    printf("  [%d]: %s\n", i, s);
  }

  printf("names:\n");
  Vector_ForEach(s, &opts->names) {
    printf(" [%d]: %s\n", i, s);
  }

  puts("\n");
}
