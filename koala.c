
#include "koala.h"
#include "options.h"

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

static int __run(struct options *options)
{
  puts(KOALA_START);

  Koala_Initialize();

  char *path;
  Vector_ForEach(path, &options->klcvec) {
    Koala_Env_Append("koala.path", path);
  }

  char *input = Vector_Get(&options->cmdvec, 0);
  Koala_Run(input, "main", &options->cmdvec);

  Koala_Finalize();

  puts(KOALA_END);

  return 0;
}

int main(int argc, char *argv[])
{
  struct options options;
  init_options(&options, OPTIONS_DELIMTER);
  int ret = parse_options(argc, argv, &options);
  if (ret) return -1;
  show_options(&options);
  __run(&options);
  return 0;
}
