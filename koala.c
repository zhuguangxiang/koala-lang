
#include "koala.h"

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("error: no input files\n");
		return -1;
	}

	puts("\n+----------------------------+");
	puts("|   Koala Machine Starting   |");
	puts("+----------------------------+");

	Koala_Initialize();
	Koala_Run(argv[1]);
	Koala_Finalize();

	puts("\n+----------------------------+");
	puts("|   Koala Machine Exited     |");
	puts("+----------------------------+");

	return 0;
}
