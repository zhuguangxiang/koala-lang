
#include "koala.h"

/*
	koala --help
	Usage: koala [options] module...
	Options:
		-h, --help
		-v, --version
		--config FILE
		-o FILE
	Example:
		koala --config koala.properties test
		koala test(default: ~/koala.properties and ./koala.properties)
 */
int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("error: no input files\n");
		return -1;
	}

	puts("\n+----------------------------+");
	puts("|   Koala Machine Starting   |");
	puts("+----------------------------+\n");

	Koala_Initialize();
	Koala_Run(argv[1]);
	Koala_Finalize();

	puts("\n+----------------------------+");
	puts("|   Koala Machine Exited     |");
	puts("+----------------------------+");

	return 0;
}
