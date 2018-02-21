
#include "koala.h"

/* run koala as interpreter or virtual machine */
int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("error: no input files\n");
		return -1;
	}

	Koala_Initialize();
	Koala_Run(argv[1]);
	Koala_Finalize();

	puts("\nKoala Machine Exits");

	return 0;
}
