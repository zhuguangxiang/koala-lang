
#include "options.h"
#include "log.h"

int isoutput(char *arg)
{
	int len = strlen(arg);
	if (len < 2) return 0;
	return arg[0] == '-' && arg[1] == 'd';
}

int isklcpath(char *arg)
{
	int len = strlen(arg);
	if (len < 3) return 0;
	return arg[0] == '-' && arg[1] == 'c' && arg[2] == 'p';
}

int issrcpath(char *arg)
{
	int len = strlen(arg);
	if (len < 3) return 0;
	return arg[0] == '-' && arg[1] == 's' && arg[2] == 'p';
}

int iskar(char *arg)
{
	int len = strlen(arg);
	if (len < 4) return 0;
	return arg[0] == '-'  && arg[1] == 'k' && arg[2] == 'a' && arg[3] == 'r';
}

void parse_output(char *arg, struct options *ops)
{
	ops->output = strdup(arg);
}

void parse_klcpath(char *arg, struct options *ops)
{
	ops->klcpath = strdup(arg);

	char *p;
	while ((p = strsep(&arg, ops->delimiter))) {
		Vector_Append(&ops->klcvec, p);
	}
}

void parse_srcpath(char *arg, struct options *ops)
{
	ops->srcpath = strdup(arg);
}

void parse_kar(char *arg, struct options *ops)
{
	ops->kar = strdup(arg);
}

int parse_options(int argc, char *argv[], struct options *ops)
{
	int i = 1;
	while (i < argc) {
		if (isklcpath(argv[i])) {
			if (++i < argc) {
				char *klcpath = argv[i];
				parse_klcpath(klcpath, ops);
			} else {
				Log_Error("invalid -cp option");
				exit(-1);
			}
		} else if (issrcpath(argv[i])) {
			if (++i < argc) {
				char *srcpath = argv[i];
				parse_srcpath(srcpath, ops);
			} else {
				Log_Error("invalid -sp option");
				exit(-1);
			}
		} else if (isoutput(argv[i])) {
			if (++i < argc) {
				char *output = argv[i];
				parse_output(output, ops);
			} else {
				Log_Error("invalid -d option");
				exit(-1);
			}
		} else if (iskar(argv[i])) {
			if (++i < argc) {
				char *kar = argv[i];
				parse_kar(kar, ops);
			} else {
				Log_Error("invalid -kar option");
				exit(-1);
			}
		} else {
			Vector_Append(&ops->cmdvec, argv[i]);
		}
		++i;
	}
	return 0;
}

int init_options(struct options *ops, char delimiter)
{
	memset(ops, 0, sizeof(struct options));
	ops->__delims[0] = delimiter;
	ops->__delims[1] = 0;
	ops->delimiter = ops->__delims;
	Vector_Init(&ops->klcvec);
	Vector_Init(&ops->cmdvec);
	return 0;
}

void show_options(struct options *ops)
{
	printf("delimiter: '%c'\n", ops->__delims[0]);
	printf("-d: '%s'\n", ops->output);

	printf("-sp:%s\n", ops->srcpath);

	char *str;
	printf("-cp:\n");
	Vector_ForEach(str, &ops->klcvec) {
		printf("  [%d]: '%s'\n", i, str);
	}

	printf("cmd:\n");
	Vector_ForEach(str, &ops->cmdvec) {
		printf(" '%s'", str);
	}
	puts("\n");
}
