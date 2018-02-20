
#include "koala.h"

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("error: no input files\n");
		return -1;
	}

	Koala_Init();
	Koala_Run(argv[1]);
	Koala_Fini();

	puts("\nKoala Machine Exits");

	return 0;
}
