
#include "options.h"
#include "log.h"

int ispkg(struct options *ops, char *arg)
{
  int len = strlen(arg);
  if (len < 4) return 0;
  return arg[0] == '-' && arg[1] == 'p' && arg[2] == 'k' && arg[3] == 'g';
}

int isargs(struct options *ops, char *arg)
{
  int len = strlen(arg);
  if (len < 5) return 0;
  return arg[0] == '-' && arg[1] == 'a' && arg[2] == 'r' && arg[3] == 'g'
    && arg[4] == 's';
}

int isout(struct options *ops, char *arg)
{
  int len = strlen(arg);
  if (len < 4) return 0;
  return arg[0] == '-' && arg[1] == 'o' && arg[2] == 'u' && arg[3] == 't';
}

int isklc(struct options *ops, char *arg)
{
  int len = strlen(arg);
  if (len < 4) return 0;
  return arg[0] == '-' && arg[1] == 'k' && arg[2] == 'l' && arg[3] == 'c';
}

int iskar(struct options *ops, char *arg)
{
  int len = strlen(arg);
  if (len < 4) return 0;
  return arg[0] == '-' && arg[1] == 'k' && arg[2] == 'a' && arg[3] == 'r';
}

void parse_klc_list(char *klc, struct options *ops)
{
  ops->klc = strdup(klc);

  char *p;
  while ((p = strsep(&klc, ops->delimiter))) {
    Vector_Append(&ops->klcvec, p);
  }
}

void parse_kar_list(char *kar, struct options *ops)
{
  ops->kar = strdup(kar);

  char *p;
  while ((p = strsep(&kar, ops->delimiter))) {
    Vector_Append(&ops->karvec, p);
  }
}

void parse_arg_list(char *args, struct options *ops)
{
  char *p;
  while ((p = strsep(&args, ops->delimiter))) {
    Vector_Append(&ops->args, p);
  }
}

void parse_output(char *out, struct options *ops)
{
  ops->outpkg = strdup(out);
}

void parse_package(char *pkg, struct options *ops)
{
  ops->srcpkg = strdup(pkg);
}

int parse_options(int argc, char *argv[], struct options *ops)
{
  int i = 1;
  while (i < argc) {
    if (isklc(ops, argv[i])) {
      if (++i < argc) {
        char *klc = argv[i];
        parse_klc_list(klc, ops);
      } else {
        error("invalid -klc option");
        return -1;
      }
    } else if (iskar(ops, argv[i])) {
      if (++i < argc) {
        char *kar = argv[i];
        parse_kar_list(kar, ops);
      } else {
        error("invalid -kar option");
        return -1;
      }
    } else if (isout(ops, argv[i])) {
      if (++i < argc) {
        char *out = argv[i];
        parse_output(out, ops);
      } else {
        error("invalid -o option");
        return -1;
      }
    } else if (ispkg(ops, argv[i])) {
      if (++i < argc) {
        char *pkg = argv[i];
        parse_package(pkg, ops);
      } else {
        error("invalid package");
        return -1;
      }
    } else if (isargs(ops, argv[i])) {
      if (++i < argc) {
        char *args = argv[i];
        parse_arg_list(args, ops);
      } else {
        error("invalid arguments");
      }
    } else {
      error("invalid commad");
      exit(-1);
      return -1;
    }
    ++i;
  }

  ops->cmd = strdup(argv[0]);

  return 0;
}

int init_options(struct options *ops, char *cmd, char delimiter)
{
  memset(ops, 0, sizeof(struct options));
  ops->__delims[0] = delimiter;
  ops->__delims[1] = 0;
  ops->delimiter = ops->__delims;
  ops->cmd = cmd;
  Vector_Init(&ops->klcvec);
  Vector_Init(&ops->karvec);
  Vector_Init(&ops->args);
  return 0;
}

void show_options(struct options *ops)
{
  printf("cmd: '%s'\n", ops->cmd);
  printf("src: '%s'\n", ops->srcpkg);
  printf("out: '%s'\n", ops->outpkg);
  printf("delimiter: '%c'\n", ops->__delims[0]);

  char *str;
  printf("klc:%s\n", ops->klc);
  Vector_ForEach(str, &ops->klcvec) {
    printf("  [%d]: '%s'\n", i, str);
  }

  printf("kar:%s\n", ops->kar);
  Vector_ForEach(str, &ops->karvec) {
    printf("  [%d]: '%s'\n", i, str);
  }

  printf("args\n");
  Vector_ForEach(str, &ops->args) {
    printf("  [%d]: '%s'\n", i, str);
  }
}
