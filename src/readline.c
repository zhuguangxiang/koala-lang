/*
 MIT License

 Copyright (c) 2018 James, https://github.com/zhuguangxiang

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>
#include "readline.h"

#define LINE_MAX_LINE 256
#define LINE_DEFAULT_COLUMNS  80
#define HISTORY_MAX 1000

static char histories[HISTORY_MAX][LINE_MAX_LINE];
static int save_pos;
static int cur_pos;

void add_history(char *line)
{
  if (line == NULL) return;
  if (line[0] == '\n') return;
  int len = strlen(line);
  if (len <= 0) return;
  if (line[len - 1] == '\n') --len;
  int pos = save_pos;
  --pos;
  if (pos < 0) pos = HISTORY_MAX - 1;
  char *saved = histories[pos];
  if (!strncmp(saved, line, len)) {
    cur_pos = save_pos;
    return;
  }
  strncpy(histories[save_pos], line, len);
  histories[save_pos][len] = '\0';
  ++save_pos;
  if (save_pos >= HISTORY_MAX) save_pos = 0;
  cur_pos = save_pos;
  histories[save_pos][0] = '\0';
}

enum KEY_ACTION {
  KEY_NULL = 0,	      /* NULL */
  CTRL_A = 1,         /* Ctrl+a */
  CTRL_B = 2,         /* Ctrl-b */
  CTRL_C = 3,         /* Ctrl-c */
  CTRL_D = 4,         /* Ctrl-d */
  CTRL_E = 5,         /* Ctrl-e */
  CTRL_F = 6,         /* Ctrl-f */
  CTRL_H = 8,         /* Ctrl-h */
  TAB = 9,            /* Tab */
  NEWLINE = 10,       /* NewLine */
  CTRL_K = 11,        /* Ctrl+k */
  CTRL_L = 12,        /* Ctrl+l */
  RETURN = 13,        /* RETURN */
  CTRL_N = 14,        /* Ctrl-n */
  CTRL_P = 16,        /* Ctrl-p */
  CTRL_T = 20,        /* Ctrl-t */
  CTRL_U = 21,        /* Ctrl+u */
  CTRL_W = 23,        /* Ctrl+w */
  CTRL_Z = 26,        /* Ctrl+z */
  ESC = 27,           /* Escape */
  BACKSPACE = 127,    /* Backspace */
};

static int terminal_colums(void)
{
  struct winsize ws;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  if (ws.ws_col == 0)
    return LINE_DEFAULT_COLUMNS;
  else
    return ws.ws_col;
}

/* The linestate structure represents the state during line editing */
struct linestate {
  int in;             /* stdin file descriptor            */
  int out;            /* stdout file descriptor           */
  char *buf;          /* edited line buffer               */
  size_t blen;        /* edited line buffer size          */
  const char *prompt; /* prompt to display                */
  size_t plen;        /* prompt length                    */
  size_t pos;         /* current cursor position          */
  size_t len;         /* current edited line length       */
  size_t cols;        /* number of columns in terminal    */
};

static void refresh(struct linestate *ls)
{
  char buf[LINE_MAX_LINE];
  // move cursor to ->pos+plen
  int len = sprintf(buf, "\r\x1b[%ldC", ls->pos + ls->plen);
  // erase to right
  len += sprintf(buf + len, "\x1b[0K");
  // write current buffer content from ->pos
  strncat(buf + len, ls->buf + ls->pos, ls->len - ls->pos);
  len += ls->len - ls->pos;
  // move cursor again
  len += sprintf(buf + len, "\r\x1b[%ldC", ls->pos + ls->plen);
  if (write(ls->out, buf, len) < 0) return;
}

static void refresh_cursor(struct linestate *ls)
{
  char buf[16];
  // move cursor to ->pos+plen
  int len = sprintf(buf, "\r\x1b[%ldC", ls->pos + ls->plen);
  if (write(ls->out, buf, len) < 0) return;
}

static void clear_line(struct linestate *ls)
{
  ls->len = 0;
  ls->pos = 0;
  char buf[16];
  // move cursor to ->pos+plen
  int len = sprintf(buf, "\r\x1b[%ldC", ls->plen);
  // erase to right
  len += sprintf(buf + len, "\x1b[0K");
  if (write(ls->out, buf, len) < 0) return;
}

static void line_insert(struct linestate *ls, char *s, int count)
{
  if (ls->len + count >= ls->blen)
    return;

  if (ls->pos == ls->len) {
    for (int i = 0; i < count; ++i) {
      ls->buf[ls->pos] = s[i];
      ++ls->pos;
      ++ls->len;
    }
    if (ls->plen + ls->len < ls->cols) {
      // not wrap line
      if (write(ls->out, s, count) < 0) return;
    } else {
      refresh(ls);
    }
  } else {
    char *from = ls->buf + ls->pos;
    char *to = from + count;
    int len = ls->len - ls->pos;
    memmove(to, from, len);
    int pos = ls->pos;
    for (int i = 0; i < count; ++i) {
      ls->buf[pos] = s[i];
      ++pos;
      ++ls->len;
    }
    refresh(ls);
    //move cursor to 'count' right
    if (ls->pos != ls->len) {
      ls->pos += count;
      refresh_cursor(ls);
    }
  }
}

static inline void insert_tab(struct linestate *ls)
{
  line_insert(ls, "  ", 2);
}

static inline void do_newline(struct linestate *ls)
{
  ls->pos = ls->len;
  line_insert(ls, "\n", 1);
}

static inline void move_home(struct linestate *ls)
{
  if (ls->pos != 0) {
    ls->pos = 0;
    refresh(ls);
  }
}

static inline void move_end(struct linestate *ls)
{
  if (ls->pos != ls->len) {
    ls->pos = ls->len;
    refresh(ls);
  }
}

static void do_backspace(struct linestate *ls)
{
  if (ls->pos > 0 && ls->len > 0) {
    char *from = ls->buf + ls->pos;
    char *to = from - 1;
    int len = ls->len - ls->pos;
    memmove(to, from, len);
    --ls->pos;
    --ls->len;
    refresh(ls);
  }
}

static inline void move_up(struct linestate *ls)
{
  int pos = cur_pos;
  --pos;
  if (pos < 0) pos = HISTORY_MAX - 1;
  if (pos == save_pos) return;
  char *line = histories[pos];
  int len = strlen(line);
  if (len <= 0) return;
  cur_pos = pos;
  clear_line(ls);
  line_insert(ls, line, len);
}

static inline void move_down(struct linestate *ls)
{
  if (cur_pos == save_pos) return;
  ++cur_pos;
  if (cur_pos >= HISTORY_MAX) cur_pos = 0;
  char *line = histories[cur_pos];
  int len = strlen(line);
  clear_line(ls);
  line_insert(ls, line, len);
}

static inline void move_left(struct linestate *ls)
{
  if (ls->pos > 0) {
    ls->pos--;
    refresh(ls);
  }
}

static inline void move_right(struct linestate *ls)
{
  if (ls->pos != ls->len) {
    ls->pos++;
    refresh(ls);
  }
}

static void do_esc(struct linestate *ls)
{
  char seq[2];
  if (read(ls->in, seq, 2) < 0) return;

  if (seq[0] == '[') {
    // ESC [ sequences
    switch (seq[1]) {
    case 'A': //up
      move_up(ls);
      break;
    case 'B': //down
      move_down(ls);
      break;
    case 'C': //right
      move_right(ls);
      break;
    case 'D': //left
      move_left(ls);
      break;
    default:
      break;
    }
  } else {
    // ESC 0 sequences
  }
}

static int line_edit(int in, int out, char *buf, size_t len, const char *prompt)
{
  struct linestate ls;
  ls.in = in;
  ls.out = out;
  ls.buf = buf;
  ls.blen = len;
  ls.prompt = prompt;
  ls.plen = strlen(prompt);
  ls.cols = terminal_colums();
  ls.pos = 0;
  ls.len = 0;

  if (write(ls.out, ls.prompt, ls.plen) < 0) return -1;

  while (1) {
    char ch;
    if (read(ls.in, &ch, 1) <= 0) return ls.len;
    switch (ch) {
    default:
      line_insert(&ls, &ch, 1);
      break;
    case TAB:
      insert_tab(&ls);
      break;
    case NEWLINE:
      do_newline(&ls);
      return ls.len;
    case BACKSPACE:
    case CTRL_H:
      do_backspace(&ls);
      break;
    case CTRL_D:
      return 0;
    case CTRL_C:
      if (write(ls.out, "^C", 2) < 0) return -1;
      //raise(SIGINT);
      ls.len = ls.pos = 0;
      do_newline(&ls);
      return ls.len;
    case ESC: // escape sequence
      do_esc(&ls);
      break;
    case CTRL_A: // go to the start of the line
      move_home(&ls);
      break;
    case CTRL_E: // go to the end of the line
      move_end(&ls);
      break;
    case CTRL_Z: // stop self
      raise(SIGTSTP);
      reset_stdin();
      init_stdin();
      break;
    case CTRL_P:
      move_up(&ls);
      break;
    case CTRL_N:
      move_down(&ls);
      break;
    case CTRL_B:
      break;
    case CTRL_F:
      break;
    }
  }
}

static struct termios orig;

void init_stdin(void)
{
  // set terminal's parameters
  struct termios raw;
  tcgetattr(STDIN_FILENO, &orig);
  raw = orig;
  //raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void reset_stdin(void)
{
  // reset terminal's parameters
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
}

char *readline(const char *prompt)
{
  char buf[LINE_MAX_LINE];
  int count;

  // edit current line
  count = line_edit(STDIN_FILENO, STDOUT_FILENO, buf, sizeof(buf), prompt);

  if (count <= 0) return NULL;
  return strndup(buf, count);
}
