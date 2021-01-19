/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "koala_yacc_lex.h"
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
