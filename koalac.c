
#include "parser.h"
#include "koala.h"

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

	FILE *in;
	KImage *image;

	in = fopen(argv[1], "r");
	Koala_Initialize();
	image = Compile(in);
	KImage_Write_File(image, outfile);
	Koala_Finalize();

	return 0;
}
