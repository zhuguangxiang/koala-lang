/*===----------------------------------------------------------------------===*\
|*                                                                            *|
|* This file is part of the koala-lang project, under the MIT License.        *|
|*                                                                            *|
|* Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#include "parser/parser.h"

#ifdef __cplusplus
extern "C" {
#endif

int _file_input_(ParserState *ps, char *buf, int size, FILE *in)
{
    return 0;
}

void koala_compile(char *path)
{
    // file_input = _file_input_;
}

#ifdef __cplusplus
}
#endif
