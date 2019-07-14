/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_PARSER_H_
#define _KOALA_PARSER_H_

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/* position */
struct pos { int row; int col; };

struct module {
  /* module name is file name, directory name or __name__ */
  char *name;
  /* symbol table per module, not per source file */
  struct symbol_table *stbl;
};

/* per source file */
struct parser_state {
  /* file name */
  char *filename;
  /* its module */
  struct module *mod;

  /* is interactive ? */
  int interactive;
  /* is complete ? */
  int more;
  /* token */
  int token;
  /* token length */
  int len;
  /* token position */
  struct pos pos;

  /* error numbers */
  int errnum;
};

/* more than MAX_ERRORS, discard left errors shown */
#define MAX_ERRORS 8

/*
 * record and print syntax error.
 *
 * ps  - The struct parser_state.
 * pos - The position which the error happened.
 * fmt - The error message format.
 *
 * Returns nothing.
 */
#define syntax_error(ps, pos, fmt, ...)                        \
({                                                             \
  if (++ps->errnum > MAX_ERRORS) {                             \
    fprintf(stderr, "%s: " __ERR_COLOR__ "Too many errors.\n", \
            ps->filename);                                     \
  } else {                                                     \
    fprintf(stderr, "%s:%d:%d: " __ERR_COLOR__ fmt "\n",       \
            ps->filename, pos->row, pos->col, __VA_ARGS__);    \
  }                                                            \
})

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_PARSER_H_ */
