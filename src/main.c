/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/utsname.h>
#include "koala.h"

static void version(void)
{
  struct utsname sysinfo;
  if (!uname(&sysinfo)) {
    printf("koala %s %s/%s\n", KOALA_VERSION,
           sysinfo.sysname, sysinfo.machine);
  }
}

static void usage(void)
{
  printf("Usage: koala [<options>] [<module-name>]\n"
         "Options:\n"
         "  -c            Compile the module.\n"
         "  -v, --verion  Print Koala version.\n"
         "  -h, --help    Print this message.\n"
         "\n");
  printf("Environment variables:\n"
         "KOALA_HOME - koala installed directory.\n"
         "             The default (third-part) module search path.\n"
         "KOALA_PATH - ':' seperated list of directories preferred to\n"
         "             the default module search path (sys.path).\n"
         "\n");
}

#define MAX_PATH_LEN 1024
int cflag;
char module[MAX_PATH_LEN];

static void save_path(char *arg)
{
  char *path = arg;
  char *slash = path + strlen(path);
  /* remove trailing slashes */
  while (*--slash == '/');
  int len = slash - path + 1;

  if (len >= MAX_PATH_LEN) {
    errmsg("Too long module's path.");
    usage();
    exit(0);
  }

  memcpy(module, path, len);
}

void parse_command(int argc, char *argv[])
{
  extern char *optarg;
  extern int optind;
  struct option options[] = {
    {"version", no_argument, NULL, 'v'},
    {"help",    no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}
  };
  int opt;
  char *slash;
  char *arg;
  int len;

  while ((opt = getopt_long(argc, argv, "c:vh?", options, NULL)) != -1) {
    switch (opt) {
    case 'c': {
      cflag = 1;
      save_path(optarg);
      break;
    }
    case 'v':
      version();
      exit(0);
      break;
    case 'h':
      /* fall-through */
    case '?':
      usage();
      exit(0);
      break;
    default:
      errmsg("invalid option '%c'.", opt);
      usage();
      exit(0);
      break;
    }
  }

  if (optind < argc) {
    if (!strcmp(argv[optind], "?")) {
      usage();
      exit(0);
    }
    if (cflag) {
      errmsg("Only one module is allowed.");
      usage();
      exit(0);
    }
    cflag = 2;
    save_path(argv[optind++]);
  }

  if (optind < argc) {
    errmsg("Only one module is allowed.");
    usage();
    exit(0);
  }
}

void koala_initialize(void);
void koala_destroy(void);

int main(int argc, char *argv[])
{
  parse_command(argc, argv);

  koala_initialize();

  if (cflag == 1) {
    koala_compile(module);
  } else if (cflag == 2) {
    koala_run(module);
  } else {
    koala_active();
  }

  koala_destroy();

  return 0;
}
