/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2018-2021 James <zhuguangxiang@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _KOALA_PARSER_H_
#define _KOALA_PARSER_H_

#include "atom.h"
#include "sbuf.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LINE_MAX_LEN  256
#define TOKEN_MAX_LEN 80

typedef struct _ParserState {
    /* file name */
    char *filename;
    /* line source */
    char linebuf[LINE_MAX_LEN];
    /* token string */
    char tokenstr[TOKEN_MAX_LEN];
    /* count of errors */
    int errors;

    /* is interactive ? */
    int interactive;
    /* is complete ? */
    int more;
    /* interactive quit */
    int quit;

    /* token ident */
    int token;
    /* token length */
    int len;
    /* token line */
    int line;
    /* token column */
    int col;
    /* in multi-line string */
    int in_str;

    /* string buffer */
    SBuf sbuf;
    /* long strings/text */
    Vector svec;

    /* document */
    SBuf doc_buf;
    int doc_line;

    /* type parameters */
    int in_angle;
} ParserState, *ParserStateRef;

void do_klass_typeparams(ParserState *ps, char *name);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_PARSER_H_ */
