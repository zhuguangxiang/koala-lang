/*===----------------------------------------------------------------------===*\
|*                               Koala                                        *|
|*                 The Multi-Paradigm Programming Language                    *|
|*                                                                            *|
|* MIT License                                                                *|
|* Copyright (c) ZhuGuangXiang https://github.com/zhuguangxiang               *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef _KOALA_PARSER_H_
#define _KOALA_PARSER_H_

#include "koala_yacc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_STR_BUF_LEN 4096

typedef struct ParserState {
    /* token id */
    int token;
    /* token length */
    int len;
    /* token line */
    int line;
    /* token column */
    int col;

    /* string buffer */
    char buf[MAX_STR_BUF_LEN];
    /* buf next avail */
    int next;
} ParserState;

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_PARSER_H_ */
