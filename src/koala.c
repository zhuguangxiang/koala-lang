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
#include "state.h"
#include "vm.h"
#include "task.h"

struct options {
	Vector pathes;
	char *filename;
};

static void init_options(struct options *opts)
{
	Vector_Init(&opts->pathes);
	opts->filename = NULL;
}

static void fini_options(struct options *opts)
{
	Vector_Fini(&opts->pathes, NULL, NULL);
	free(opts->filename);
}

static void parse_pathes(char *pathes, struct options *opts)
{
	char *comma;
	char *path;
	while (*pathes) {
		comma = strchr(pathes, ':');
		if (comma != NULL) {
			path = strndup(pathes, comma - pathes);
			Vector_Append(&opts->pathes, path);
			pathes = comma + 1;
		} else {
			path = strdup(pathes);
			Vector_Append(&opts->pathes, path);
			break;
		}
	}
}

#define KOALA_VERSION "0.8.5"

static void show_version(void)
{
	struct utsname sysinfo;
	if (!uname(&sysinfo)) {
		fprintf(stderr, "Koala version %s, %s/%s\n",
						KOALA_VERSION, sysinfo.sysname, sysinfo.machine);
	}
	exit(0);
}

static void usage(const char *prog)
{
  fprintf(stderr,
		"Usage: %s [<options>] <file>\n"
		"Options:\n"
		"\t-p <path:...>        Pathes in where packages are searched.\n"
		"\t-v                   Print product version.\n"
		"\t-h                   Print this message.\n",
		prog);
  exit(0);
}

static void parse_arguments(int argc, char *argv[], struct options *opts)
{
	extern char *optarg;
	extern int optind;
	int opt;
	while ((opt = getopt(argc, argv, "p:vh")) != -1) {
		switch (opt) {
			case 'p':
				parse_pathes(optarg, opts);
				break;
			case 'v':
				show_version();
				break;
			case 'h':
			/* fallthrough */
			case '?':
				usage(argv[0]);
				break;
			default:
				break;
		}
	}

	if (optind < argc)
		opts->filename = strdup(argv[optind]);
	else
		usage(argv[0]);
}

static void *task_entry_func(void *arg)
{
	struct options *opts = arg;
	KoalaState *ks = KoalaState_New();
	task_set_object(ks);
	Koala_Run_File(ks, opts->filename, NULL);
	Koala_Show_Packages();
	printf("task-%lu is finished.\n", current_task()->id);
	return NULL;
}

int main(int argc, char *argv[])
{
	/* parse arguments */
	struct options opts;
	init_options(&opts);
	parse_arguments(argc, argv, &opts);
	char *s;
	Vector_ForEach(s, &opts.pathes) {
		puts(s);
	}
	puts(opts.filename);

	/* initial koala machine */
	Koala_Initialize();

	/* set path for searching packages */
	Koala_Add_Property(KOALA_PATH, ".");
	char *path;
	Vector_ForEach(path, &opts.pathes) {
		Koala_Add_Property(KOALA_PATH, path);
	}

	/* display packages */
	Koala_Show_Packages();

	/* initial task's processors */
	init_processors(1);

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
