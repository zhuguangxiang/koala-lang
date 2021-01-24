/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

// clang-format off
// Don't touch this!
#include <stdint.h>
#include "koala_yacc.h"
#include "koala_lex.h"

// clang-format on
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
