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

#include <sys/utsname.h>
#include "env.h"
#include "state.h"
#include "parser.h"
#include "codegen.h"
#include "koala_tokens.h"
#include "koala_lex.h"
#include "options.h"
#include "log.h"

/*
 * compile one package
 * compile all source files in the package and output into 'pkg-name'.klc
 * input: the package's dir
 */
static int compile(char *input, char *output, struct options *options)
{
	struct stat sb;
	if (lstat(input, &sb) == - 1) {
		fprintf(stderr, "%s: invalid filename\n", input);
		return -1;
	}

	if (!S_ISDIR(sb.st_mode)) {
		fprintf(stderr, "%s: is not a directory\n", input);
		return -1;
	}

	DIR *dir = opendir(input);
	if (!dir) {
		fprintf(stderr, "%s: no such file or directory\n", input);
		return -1;
	}

	Koala_Initialize();

	char *path;
	Vector_ForEach(path, &options->klcvec) {
		Koala_Add_Property(KOALA_PATH, path);
	}

	Vector vec;
	Vector_Init(&vec);

	PackageInfo *pkg = New_PackageInfo(output, options);
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
		destroy_parser(ps);
	}

	if (errnum <= 0) {
		parse_module_scope(pkg);
		codegen_klc(pkg);
	} else {
		fprintf(stderr, "There are %d errors.\n", errnum);
	}

	Vector_Fini(&vec, NULL, NULL);
	//free packageinfo
	Koala_Finalize();

	return 0;
}

int main(int argc, char *argv[])
{
	struct options options;
	init_options(&options, OPTIONS_DELIMTER);
	int ret = parse_options(argc, argv, &options);
	if (ret) return -1;
	show_options(&options);

	char *src = options.srcpath;
	char *str;
	Vector_ForEach(str, &options.cmdvec) {
		char input[strlen(src) + strlen(str) + 4];
		strcpy(input, src);
		strcat(input, str);
		Log_Debug("input:%s", input);

		char output[strlen(options.output) + strlen(str) + 8];
		strcpy(output, options.output);
		strcat(output, str);
		strcat(output, ".klc");
		Log_Debug("output:%s", output);

		compile(input, output, &options);
	}

	return 0;
}
