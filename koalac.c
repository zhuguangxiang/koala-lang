
#include <sys/utsname.h>
#include "koala.h"
#include "parser.h"
#include "codegen.h"
#include "koala_yacc.h"
#include "koala_lex.h"
#include "options.h"

static int __compile(char *input, char *output, struct options *options)
{
  struct stat sb;
  if (lstat(input, &sb) == - 1) {
    printf("%s: invalid filename\n", input);
    exit(EBADF);
    return -1;
  }

  if (!S_ISDIR(sb.st_mode)) {
    printf("%s: is not a directory\n", input);
    return 0;
  }

  DIR *dir = opendir(input);
  if (!dir) {
    printf("%s: no such file or directory\n", input);
    return 0;
  }

  Koala_Initialize();

  char *path;
  Vector_ForEach(path, &options->klcvec) {
    Koala_Env_Append("koala.path", path);
  }

  Vector vec;
  Vector_Init(&vec);

  PackageInfo *pkg = New_PackageInfo(output, options);
  ParserState *ps;
  int errnum = 0;

  struct dirent *dent;
  char name[512];
  FILE *in;
  while ((dent = readdir(dir))) {
    sprintf(name, "%s/%s", input, dent->d_name);
    if (lstat(name, &sb) != 0) continue;
    if (S_ISDIR(sb.st_mode)) continue;
    in = fopen(name, "r");
    if (!in) {
      printf("%s: no such file or directory\n", name);
      continue;
    }
    ps = new_parser(pkg, dent->d_name);
    yyset_in(in, ps->scanner);
    yyparse(ps, ps->scanner);
    fclose(in);
    Vector_Append(&vec, ps);
    errnum += ps->errnum;
  }
  closedir(dir);

  Vector_ForEach(ps, &vec) {
    if (ps->errnum <= 0)
      parser_body(ps, &ps->stmts);
    destroy_parser(ps);
  }

  if (errnum <= 0) {
    parse_module_scope(pkg);
    codegen_klc(pkg);
  } else {
    fprintf(stderr, "There are %d errors.\n", errnum);
  }

  Vector_Fini(&vec, NULL, NULL);
  //free packageinfo
  Koala_Finalize();

  return 0;
}

int main(int argc, char *argv[])
{
  struct options options;
  init_options(&options, OPTIONS_DELIMTER);
  int ret = parse_options(argc, argv, &options);
  if (ret) return -1;
  show_options(&options);

  char *src = options.srcpath;
  char *str;
  Vector_ForEach(str, &options.cmdvec) {
    char input[strlen(src) + strlen(str) + 4];
    strcpy(input, src);
    strcat(input, str);
    debug("input:%s", input);

    char output[strlen(options.output) + strlen(str) + 8];
    strcpy(output, options.output);
    strcat(output, str);
    strcat(output, ".klc");
    debug("output:%s", output);

    __compile(input, output, &options);
  }

  return 0;
}
