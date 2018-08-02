
#include <sys/utsname.h>
#include "koala.h"
#include "parser.h"
#include "codegen.h"

#define KOALA_VERSION "0.7.1"

typedef void (*help_fn)(void);
typedef int (*cmd_fn)(char *argv[], int size);

static void help_build(void);
static void help_run(void);
static void help_install(void);
static void help_help(void);
static void help_version(void);

static int cmd_build(char *argv[], int size);
static int cmd_run(char *argv[], int size);
static int cmd_install(char *argv[], int size);
static int cmd_help(char *argv[], int size);
static int cmd_version(char *argv[], int size);

struct command {
  char *name;
  char *desc;
  help_fn help;
  cmd_fn cmd;
} cmds[] = {
  {
    "build",
    "compile packages and dependencies",
    help_build,
    cmd_build
  },
  {
    "help",
    "show this information",
    help_help,
    cmd_help
  },
  {
    "install",
    "compile and install packages and dependencies",
    help_install,
    cmd_install
  },
  {
    "run",
    "compile and run Koala program",
    help_run,
    cmd_run
  },
  {
    "version",
    "print Koala version",
    help_version,
    cmd_version
  }
};

static cmd_fn get_cmd_func(char *name)
{
  struct command *item;
  for (int i = 0; i < nr_elts(cmds); i++) {
    item = cmds + i;
    if (!strcmp(item->name, name))
      return item->cmd;
  }
  return NULL;
}

static help_fn get_help_func(char *name)
{
  struct command *item;
  for (int i = 0; i < nr_elts(cmds); i++) {
    item = cmds + i;
    if (!strcmp(item->name, name))
      return item->help;
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/

static void help_build(void)
{
  puts("usage: koala build [-o output] [build flags] [packages]");
}

static void help_run(void)
{
  puts("usage: koala run [build flags] file(without .klc)");
}

static void help_install(void)
{

}

static void help_help(void)
{
  puts("usage: koala help [command]");
}

static void help_version(void)
{
  puts("usage: go version\n");
  puts("Version prints the Koala version.");
}

/*---------------------------------------------------------------------------*/

static int cmd_build(char *argv[], int size)
{
  UNUSED_PARAMETER(size);

  char *input = argv[0];

  FILE *in = fopen(input, "r");
  if (!in) {
    printf("%s: no such file or directory\n", input);
    return 0;
  }

  int len = strlen(input) - 2;
  char output[len + 4];
  memcpy(output, input, len);
  strcpy(output + len, "klc");

  ParserState *ps;
  Koala_Initialize();
  ps = new_parser(input);
  parser_module(ps, in);
  if (ps->errnum <= 0) {
    codegen_klc(ps, output);
  } else {
    fprintf(stderr, "There are %d errors.\n", ps->errnum);
  }
  destroy_parser(ps);
  Koala_Finalize();

  fclose(in);

  return 0;
}

#define KOALA_START "\
+------------------------+\
\n|  Koala Machine Starts  |\
\n+------------------------+\n\
"

#define KOALA_END "\
\n+------------------------+\
\n|  Koala Machine Exits   |\
\n+------------------------+\
"
static int cmd_run(char *argv[], int size)
{
  if (size <= 0) {
    puts("koala run: no koala code file listed");
    return 0;
  }

  puts(KOALA_START);

  Koala_Initialize();
  Koala_Run(argv[0], "Main");
  Koala_Finalize();

  puts(KOALA_END);

  return 0;
}

static int cmd_install(char *argv[], int size)
{
  UNUSED_PARAMETER(argv);
  UNUSED_PARAMETER(size);
  return 0;
}

static inline void show_cmd_helps(void)
{
  struct command *item;
  for (int i = 0; i < nr_elts(cmds); i++) {
    item = cmds + i;
    printf("\t%-12s%s\n", item->name, item->desc);
  }
}

static int cmd_help(char *argv[], int size)
{
  if (size <= 0) {
    puts("Koala is a tool for Koala-Language.\n");
    puts("Usage:\n");
    puts("\tkoala command [arguments]\n");
    puts("The commands are:\n");
    show_cmd_helps();
    puts("\nUse \"koala help [command]\" "
      "for more information about a command.\n");
    return 0;
  }

  if (size == 1) {
    help_fn fn = get_help_func(argv[0]);
    if (!fn) {
      printf("koala: unknown command \"%s\"\n", argv[0]);
      puts("Run 'koala help' for usage.");
    } else {
      fn();
    }
    return 0;
  }

  puts("usage: koala help command\n");
  puts("Too many arguemtns given.");
  return 0;
}

static int cmd_version(char *argv[], int size)
{
  UNUSED_PARAMETER(argv);
  UNUSED_PARAMETER(size);

  struct utsname sysinfo;
  if (!uname(&sysinfo)) {
    printf("koala version %s, %s/%s\n", KOALA_VERSION,
      sysinfo.sysname, sysinfo.machine);
  }
  return 0;
}

int main(int argc, char *argv[])
{
  if (argc == 1) {
    cmd_help(NULL, 0);
    return 0;
  } else {
    cmd_fn fn = get_cmd_func(argv[1]);
    if (!fn) {
      printf("koala: unknown command \"%s\"\n", argv[1]);
      puts("Run 'koala help' for usage.");
      return 0;
    }
    return fn(argv + 2, argc - 2);
  }
}
