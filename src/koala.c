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

#include "koala.h"
#include "options.h"

#define KOALA_VERSION "0.8.5"

static void show_version(void)
{
  struct utsname sysinfo;
  if (!uname(&sysinfo)) {
    fprintf(stderr, "Koala version %s, %s/%s\n",
            KOALA_VERSION, sysinfo.sysname, sysinfo.machine);
  }
}

static void show_usage(char *prog)
{
  fprintf(stderr,
    "Usage: %s [<options>] <main package>\n"
    "Options:\n"
    "\t-p <path>          Specify where to find external packages.\n"
    "\t-D <name>=<value>  Set a property.\n"
    "\t-v                 Print virtual machine version.\n"
    "\t-h                 Print this message.\n",
    prog);
}

int main(int argc, char *argv[])
{
  /* parse arguments */
  Options options;
  init_options(&options, show_usage, show_version);
  parse_options(argc, argv, &options);
  show_options(&options);

  Koala_Initialize();

  /* set path for searching packages */
  Koala_SetPathes(&options.pathes);

  /* get executed package filename and arguments */
  int size = Vector_Size(&options.names);
  char *filename = Vector_Get(&options.names, 0);
  Object *args = NULL;
  if (size > 1) {
    int i = 1;
    char *str;
    args = Tuple_New(size - 1);
    while (i < size) {
      str = Vector_Get(&options.names, i);
      Tuple_Set(args, i - 1, String_New(str));
    }
  }

  /* call main */
  Koala_Main(filename, args);

  Koala_Finalize();

  fini_options(&options);
  return 0;
}
