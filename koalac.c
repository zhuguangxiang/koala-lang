
#include "ast.h"

extern FILE *yyin;
extern int yyparse(void);
extern void koala_main(char *arg);

int yyerror(const char *str)
{
  fprintf(stderr, "%s\n", str);
  return 0;
}

void yyecho(char *str)
{
  fprintf(stderr, "%s\n", str);
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(stderr, "no parameter\n");
    return 0;
  }

  koala_main("/opt/libs/koala_lib:/opt/libs/third_part_klib");

  yyin = fopen(argv[1], "r");
  yyparse();
  fclose(yyin);

  return 0;
}
