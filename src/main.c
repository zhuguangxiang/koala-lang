/*
MIT License

Copyright (c) 2018 James, https://github.com/zhuguangxiang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

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
#include <getopt.h>
#include <sys/utsname.h>
#include "version.h"
#include "interactive.h"
#include "compile.h"
#include "run.h"
#include "debug.h"

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

struct command {
  short cflag;
  short has;
  char module[MAX_PATH_LEN];
};

static void copy_path(char *arg, struct command *cmd)
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

  if (cmd->has) {
    errmsg("Only one module is allowed.");
    usage();
    exit(0);
  }

  memcpy(cmd->module, path, len);
  cmd->has = 1;
}

void parse_command(int argc, char *argv[], struct command *cmd)
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
      cmd->cflag = 1;
      copy_path(optarg, cmd);
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
    copy_path(argv[optind++], cmd);
  }

  if (optind < argc) {
    errmsg("Only one module is allowed.");
    usage();
    exit(0);
  }
}

static struct command cmd;

int main(int argc, char *argv[])
{
  parse_command(argc, argv, &cmd);

  //koala_initialize();

  if (!cmd.cflag) {
    if (!cmd.has) {
      koala_active();
    } else {
      koala_run(cmd.module);
    }
  } else {
    koala_compile(cmd.module);
  }

  //koala_destory();

  return 0;
}
