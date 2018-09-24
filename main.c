
#include <sys/utsname.h>
#include "koala.h"
#include "parser.h"
#include "codegen.h"
#include "koala_yacc.h"
#include "koala_lex.h"

#define KOALA_VERSION "0.7.1"

typedef void (*help_fn)(void);
typedef int (*cmd_fn)(char *argv[], int size);

static void help_build(void);
static void help_run(void);
static void help_install(void);
static void help_help(void);
static void help_version(void);
static void help_archive(void)
{
}

static int cmd_build(char *argv[], int size);
static int cmd_run(char *argv[], int size);
static int cmd_install(char *argv[], int size);
static int cmd_help(char *argv[], int size);
static int cmd_version(char *argv[], int size);
static int cmd_archive(char *argv[], int size)
{
  return 0;
}

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
    "archive",
    "archive pacakges into .kar",
    help_archive,
    cmd_archive
  },
  {
    "install",
    "build, ar and install Koala archive",
    help_install,
    cmd_install
  },
  {
    "run",
    "run Koala program",
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

  int len = strlen(input);
  char output[16 + len + 4];
  strcpy(output, "../pkg/");
  strncpy(output + 7, input, len);
  strcpy(output + len + 7, ".klc");
  printf("path:%s\n", output);

  Koala_Initialize();
  Koala_Env_Append("koala.path", "../pkg/");

  Vector vec;
  Vector_Init(&vec);

  PackageInfo *pkg = New_PackageInfo(output);
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
    //parser_enter_scope(ps, ps->sym->ptr, SCOPE_MODULE);
    //ps->u->sym = ps->sym;
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
    //parser_exit_scope(ps);
    destroy_parser(ps);
  }

  if (errnum <= 0) {
    codegen_klc(pkg);
  } else {
    fprintf(stderr, "There are %d errors.\n", errnum);
  }

  Vector_Fini(&vec, NULL, NULL);
  Koala_Finalize();

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
  Koala_Run(argv[0], "main");
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
