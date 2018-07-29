
#include "stdio.h"
#include "koala.h"
#include "parser.h"
#include "codegen.h"

void build(void)
{
  char *input = "test/expression.kl";

	FILE *in = fopen(input, "r");
	if (!in) {
		printf("%s: no such file or directory\n", input);
		return;
	}

	int len = strlen(input) - 2;
	char output[len + 4];
	memcpy(output, input, len);
	strcpy(output + len, "klc");

	ParserState *ps;
	ps = new_parser(input);
	parser_module(ps, in);
	if (ps->errnum <= 0) {
		codegen_klc(ps, output);
	} else {
		fprintf(stderr, "There are %d errors.\n", ps->errnum);
	}
	destroy_parser(ps);

	fclose(in);
}

int main(int argc, char *argv[])
{
  Koala_Initialize();

  build();

  Object *res = Koala_Run("test/expression", "Add", "ii", 100, 200);
  Koala_Finalize();

  TValue v = Tuple_Get(res, 0);
  printf("call koala 'Add' func: 100 + 200 = %lld\n", VALUE_INT(&v));
  return 0;
}
