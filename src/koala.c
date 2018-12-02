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
#include "state.h"

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

static void usage(const char *prog)
{
  fprintf(stderr,
		"Usage: %s [<options>] <file>\n"
		"Options:\n"
		"\t-p <path:...>        Set pathes in where packages are searched.\n"
		"\t-h                   Print this message.\n",
		prog);
  exit(1);
}

static void parse_arguments(int argc, char *argv[], struct options *opts)
{
	extern char *optarg;
	extern int optind;
	int opt;
	while ((opt = getopt(argc, argv, "p:h")) != -1) {
		switch (opt) {
			case 'p':
				parse_pathes(optarg, opts);
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

static int run(struct options *opts)
{
	Koala_Initialize();

	Koala_Add_Property(KOALA_PATH, ".");
	char *path;
	Vector_ForEach(path, &opts->pathes) {
		Koala_Add_Property(KOALA_PATH, path);
	}

	Koala_Run(opts->filename);

	Koala_Show_Packages();

	Koala_Finalize();

	return 0;
}

int main(int argc, char *argv[])
{
	struct options opts;
	init_options(&opts);
	parse_arguments(argc, argv, &opts);
	char *s;
	Vector_ForEach(s, &opts.pathes) {
		puts(s);
	}
	puts(opts.filename);
	run(&opts);
	fini_options(&opts);
	return 0;
}
