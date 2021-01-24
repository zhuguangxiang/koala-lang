/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "parser.h"

#ifdef __cplusplus
extern "C" {
#endif

static int need_semicolon(ParserState *ps)
{
    static int tokens[] = { INT_LITERAL, STRING_LITERAL, ID, SELF, SUPER, TRUE,
        FALSE, ')', ']', '}', '>', '_' };

    for (int i = 0; i < COUNT_OF(tokens); ++i) {
        if (tokens[i] == ps->token) return 1;
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
