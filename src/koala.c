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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/utsname.h>
#include "options.h"
#include "state.h"
#include "eval.h"
#include "task.h"

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
    "\t-p <path>         Specify where to find external packages.\n"
    "\t-Dname=value      Set a property.\n"
    "\t-v                Print virtual machine version.\n"
    "\t-h                Print this message.\n",
    prog);
}

static void *task_entry_func(void *arg)
{
  struct options *opts = arg;
  KoalaState *ks = KoalaState_New();
  task_set_object(ks);
  char *file = Vector_Get(&opts->names, 0);
  Koala_Run_File(ks, file, NULL);
  Koala_Show_Packages();
  printf("task-%lu is finished.\n", current_task()->id);
  return NULL;
}

int main(int argc, char *argv[])
{
  /* parse arguments */
  struct options opts;
  init_options(&opts);
  set_options_usage(&opts, show_usage);
  set_options_version(&opts, show_version);
  parse_options(argc, argv, &opts);
  show_options(&opts);

  /* initial koala machine */
  Koala_Initialize();

  /*
   * set path for searching packages
   * 1. current source code directory
   * 2. -p directory
   */
  Koala_Add_Property(KOALA_PATH, ".");
  char *path;
  Vector_ForEach(path, &opts.pathes) {
    Koala_Add_Property(KOALA_PATH, path);
  }

  /* display packages */
  Koala_Show_Packages();

  /* initial task's processors */
  task_init_procs(1);

  /* new a task and wait for it's finished */
  task_attr_t attr = {.stacksize = 1 * 512 * 1024};
  task_t *task = task_create(&attr, task_entry_func, &opts);
  task_join(task, NULL);

  /* finialize koala machine */
  Koala_Finalize();

  /* finialize options */
  fini_options(&opts);

  printf("task-%lu is finished.\n", current_task()->id);
  return 0;
}
