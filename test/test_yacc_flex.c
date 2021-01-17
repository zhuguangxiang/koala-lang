/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangxiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

// NOTES: do not change the included sequence

#include <stdint.h>
#include "koala_yacc.h"
#include "koala_lex.h"
#include "readline.h"

static yyscan_t scanner;

int main(int argc, char *argv[])
{
    init_readline();

    yylex_init_extra(NULL, &scanner);
    // yyset_in(stdin, scanner);
    yyparse(NULL, scanner);
    yylex_destroy(scanner);

    fini_readline();
    return 0;
}
