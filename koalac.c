
#include "parser.h"
#include "koala.h"
#include "codegen.h"

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("error: no input files\n");
		return -1;
	}

	char *srcfile = argv[1];
	int len = strlen(srcfile);
	char *outfile = malloc(len + 4 + 1);
	char *tmp = strrchr(srcfile, '.');
	if (tmp) {
		memcpy(outfile, srcfile, tmp - srcfile);
		outfile[tmp - srcfile] = 0;
	} else {
		strcpy(outfile, srcfile);
	}
	strcat(outfile, ".klc");

	FILE *in = fopen(argv[1], "r");

	ParserState ps;
	Koala_Initialize();
	init_parser(&ps);
	parser_module(&ps, in);
	codegen_klc(&ps, outfile);
	fini_parser(&ps);
	Koala_Finalize();
	return 0;
}
