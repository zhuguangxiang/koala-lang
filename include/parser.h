/*
 * This file is part of the koala-lang project, under the MIT License.
 * Copyright (c) 2020-2021 James <zhuguangxiang@gmail.com>
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

#include "ast.h"
#include "atom.h"
#include "buffer.h"
#include "symtbl.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Module {
    /* saved in global parser modules */
    HashMapEntry entry;
    /* module path */
    char *path;
    /* module name is file, dir or __name__ */
    char *name;
    /* symbol table per module(share between files) */
    SymTblRef stbl;
    /* ParserState per source file */
    Vector pss;
} Module, *ModuleRef;

typedef struct _ParserState {
    FILE *in;
    void *lexer;
    ModuleRef mod;
    /* statements */
    Vector stmts;

    /* file name */
    char *filename;
    /* source line buffer */
    // SBuf linebuf;
    /* token string index */
    // int token_index;

    /* count of errors */
    int errors;

    /* is interactive ? */
    int cmd;
    /* is complete ? */
    int more;

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
    Buffer buf;
    /* long strings/text */
    Vector svec;

    /* document */
    Buffer doc_buf;
    int doc_line;

    /* type parameters */
    int in_angle;
} ParserState, *ParserStateRef;

/* more than MAX_ERRORS, discard left errors shown */
#define MAX_ERRORS 8

void parse_stmt(ParserStateRef ps, StmtRef stmt);

void do_klass_typeparams(ParserStateRef ps, char *name);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_PARSER_H_ */
