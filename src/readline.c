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

#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include "common.h"

static struct termios orig;

void line_init(void)
{
    /* save old */
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig);

    /* modify the original mode */
    raw = orig;

    /*
      local modes - choing off, canonical off, no extended functions,
      no signal chars (^Z,^C)
     */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    /*
      control chars - set return condition: min number of bytes and timer.
      We want read to return every single byte, without timeout.
     */
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    /* put terminal in raw mode after flushing */
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void line_fini(void)
{
    /* reset term mode */
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
}

enum KEY_ACTION {
    KEY_NULL = 0,
    CTRL_A = 1,
    CTRL_B = 2,
    CTRL_C = 3,
    CTRL_D = 4,
    CTRL_E = 5,
    CTRL_F = 6,
    CTRL_H = 8,
    TAB = 9,
    NEWLINE = 10,
    CTRL_K = 11,
    CTRL_L = 12,
    RETURN = 13,
    CTRL_N = 14,
    CTRL_P = 16,
    CTRL_T = 20,
    CTRL_U = 21,
    CTRL_W = 23,
    CTRL_Z = 26,
    ESC = 27,
    BACKSPACE = 127,
};

/* line editor state */
typedef struct linestate {
    /* in fd */
    int in;
    /* out fd */
    int out;
    /* line editor buffer */
    char *buf;
    /* line editor buffer size */
    int bufsize;
    /* current cursor position */
    int pos;
    /* current line editor length */
    int len;
    /* prompt */
    char *prompt;
    /* prompt length */
    int plen;
    /* column */
    int col;
} LineState;

static void refresh(LineState *ls)
{
    char buf[ls->bufsize];

    // move cursor to ->pos+plen
    int len = sprintf(buf, "\r\x1b[%dC", ls->pos + ls->plen);

    // erase to right
    len += sprintf(buf + len, "\x1b[0K");

    // write current buffer content from ->pos
    strncat(buf + len, ls->buf + ls->pos, ls->len - ls->pos);
    len += ls->len - ls->pos;

    // move cursor again
    len += sprintf(buf + len, "\r\x1b[%dC", ls->pos + ls->plen);

    if (write(ls->out, buf, len) < 0) return;
}

static void refresh_cursor(LineState *ls)
{
    char buf[16];

    // move cursor to ->pos+plen
    int len = sprintf(buf, "\r\x1b[%dC", ls->pos + ls->plen);

    if (write(ls->out, buf, len) < 0) return;
}

static void line_insert(LineState *ls, char *s, int count)
{
    if (ls->len + count >= ls->bufsize) return;

    if (ls->pos == ls->len) {
        for (int i = 0; i < count; ++i) {
            ls->buf[ls->pos] = s[i];
            ++ls->pos;
            ++ls->len;
        }
        if (ls->plen + ls->len < ls->col) {
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
        // move cursor to 'count' right
        if (ls->pos != ls->len) {
            ls->pos += count;
            refresh_cursor(ls);
        }
    }
}

static inline void insert_tab(LineState *ls)
{
    line_insert(ls, "  ", 2);
}

static inline void do_newline(LineState *ls)
{
    ls->pos = ls->len;
    line_insert(ls, "\r\n\0", 3);
}

static inline void move_home(LineState *ls)
{
    if (ls->pos != 0) {
        ls->pos = 0;
        refresh(ls);
    }
}

static inline void move_end(LineState *ls)
{
    if (ls->pos != ls->len) {
        ls->pos = ls->len;
        refresh(ls);
    }
}

static void do_backspace(LineState *ls)
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

static inline void move_left(LineState *ls)
{
    if (ls->pos > 0) {
        ls->pos--;
        refresh(ls);
    }
}

static inline void move_right(LineState *ls)
{
    if (ls->pos != ls->len) {
        ls->pos++;
        refresh(ls);
    }
}

static void do_esc(LineState *ls)
{
    char seq[2];
    if (read(ls->in, seq, 2) < 0) return;

    if (seq[0] == '[') {
        // ESC [ sequences
        switch (seq[1]) {
            case 'A': // up
                break;
            case 'B': // down
                break;
            case 'C': // right
                move_right(ls);
                break;
            case 'D': // left
                move_left(ls);
                break;
            default:
                break;
        }
    } else {
        // ESC 0 sequences
    }
}

#define MAX_COLUMNS 80

static int get_term_colum(void)
{
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    if (!ws.ws_col)
        return MAX_COLUMNS;
    else
        return ws.ws_col;
}

static int line_edit(int in, int out, char *buf, int len, char *prompt)
{
    LineState ls = {
        .in = in,
        .out = out,
        .buf = buf,
        .bufsize = len,
        .prompt = prompt,
        .plen = strlen(prompt),
        .col = get_term_colum(),
    };

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
            case RETURN:
                printf("input is RETURN?\n");
                abort();
            case NEWLINE:
                do_newline(&ls);
                return ls.len;
            case BACKSPACE:
            case CTRL_H:
                do_backspace(&ls);
                break;
            case CTRL_D:
                if (write(ls.out, "\r\n", 2) < 0) return -1;
                return 0;
            case CTRL_C:
                if (write(ls.out, "^C", 2) < 0) return -1;
                raise(SIGINT);
                break;
            case ESC: // escape sequence
                do_esc(&ls);
                break;
            case CTRL_A: // go to start
                move_home(&ls);
                break;
            case CTRL_E: // go to end
                move_end(&ls);
                break;
            case CTRL_F: // forward
                move_right(&ls);
                break;
            case CTRL_B: // backward
                move_left(&ls);
                break;
            case CTRL_K:
                break;
            case CTRL_L:
                break;
            case CTRL_N:
                break;
            case CTRL_P:
                break;
            case CTRL_T:
                break;
            case CTRL_U:
                break;
            case CTRL_W:
                break;
            case CTRL_Z: // stop self
                line_fini();
                raise(SIGTSTP);
                line_init();
                break;
        }
    }
}

int readline(char *prompt, char *buf, int len)
{
    return line_edit(STDIN_FILENO, STDOUT_FILENO, buf, len, prompt);
}
