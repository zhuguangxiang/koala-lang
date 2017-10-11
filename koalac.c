
#include <stdio.h>
#include "ast.h"

extern FILE *yyin;
extern int yyparse(void);

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

  yyin = fopen(argv[1], "r");
  yyparse();
  fclose(yyin);

  return 0;
}
