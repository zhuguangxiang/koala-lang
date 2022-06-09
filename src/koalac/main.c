/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
 */

/* clang-format off */
#include "parser.h"
#include "koala_yacc.h"
#include "koala_lex.h"
/* clang-format on */

static void build_ast(char *path)
{
    FILE *in = fopen(path, "r");
    if (in == NULL) {
        printf("%s: No such file or directory.", path);
        return;
    }

    ParserState ps = { 0 };

    yyscan_t scanner;
    yylex_init_extra(&ps, &scanner);
    yyset_in(in, scanner);
    yyparse(&ps, scanner);
    yylex_destroy(scanner);

    fclose(in);
}

int main(int argc, char *argv[])
{
    UNUSED(argc);
    init_atom();
    build_ast(argv[1]);
    return 0;
}
