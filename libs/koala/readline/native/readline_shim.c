/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * The terminal half of koala/readline: switching a terminal to raw mode
 * and back. Everything else - keys, editing, drawing, history and
 * completion - is written in Koala.
 *
 * Raw mode is the one linenoise uses: no echo, no line buffering, no
 * signal keys (Ctrl-C arrives as byte 3), no CR to NL translation on
 * input and no output processing, one byte per read. The original mode
 * is restored by tty_raw_off, and at exit if the program ends while the
 * terminal is raw.
 */

#include <signal.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include "object.h"

static struct termios g_orig_termios;
static int g_raw_fd = -1;
static int g_atexit_registered = 0;

static void restore_at_exit(void)
{
    if (g_raw_fd >= 0) {
        tcsetattr(g_raw_fd, TCSAFLUSH, &g_orig_termios);
        g_raw_fd = -1;
    }
}

// func tty_raw_on(fd int) bool
//
// Put the terminal on `fd` in raw mode. False when `fd` is not a
// terminal or its mode cannot be changed.
static TValue _tty_raw_on(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int fd = (int)kl_arg_int(0);

    if (g_raw_fd >= 0) return bool_value(1);
    if (!isatty(fd)) return bool_value(0);
    if (tcgetattr(fd, &g_orig_termios) == -1) return bool_value(0);

    if (!g_atexit_registered) {
        atexit(restore_at_exit);
        g_atexit_registered = 1;
    }

    struct termios raw = g_orig_termios;
    // input: no break, no CR to NL, no parity check, no strip, no flow control
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    // output: no post processing
    raw.c_oflag &= ~(OPOST);
    // control: 8 bit characters
    raw.c_cflag |= (CS8);
    // local: no echo, no canonical mode, no extended functions, no signal keys
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    // every read returns one byte, with no timeout
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSAFLUSH, &raw) < 0) return bool_value(0);
    g_raw_fd = fd;
    return bool_value(1);
}

// func tty_raw_off(fd int) bool
//
// Restore the mode the terminal had before tty_raw_on. True when it was
// not raw.
static TValue _tty_raw_off(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int fd = (int)kl_arg_int(0);

    if (g_raw_fd < 0) return bool_value(1);
    int ok = tcsetattr(fd, TCSAFLUSH, &g_orig_termios) == 0;
    g_raw_fd = -1;
    return bool_value(ok);
}

// The key the terminal had for `index` (VSUSP, VQUIT) before raw mode, -1
// when it had none: not raw, the key disabled, or signal keys off.
static int _signal_key(int index)
{
    if (g_raw_fd < 0) return -1;
    if (!(g_orig_termios.c_lflag & ISIG)) return -1;
    cc_t key = g_orig_termios.c_cc[index];
    if (key == _POSIX_VDISABLE) return -1;
    return (int)key;
}

// func tty_suspend_key() int
//
// The terminal's suspend key (Ctrl-Z unless stty changed it), -1 for none.
static TValue _tty_suspend_key(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    return int64_value(_signal_key(VSUSP));
}

// func tty_quit_key() int
//
// The terminal's quit key (Ctrl-\ unless stty changed it), -1 for none.
static TValue _tty_quit_key(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    return int64_value(_signal_key(VQUIT));
}

// func tty_suspend()
//
// Stop the process group, as the suspend key does with signal keys on. It
// returns once the group is continued, at once when SIGTSTP is ignored or
// the group is orphaned.
static TValue _tty_suspend(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    kill(0, SIGTSTP);
    return nil_value;
}

// func tty_quit()
//
// Send SIGQUIT to the process group, as the quit key does with signal keys
// on. It returns when SIGQUIT is ignored or handled.
static TValue _tty_quit(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    kill(0, SIGQUIT);
    return nil_value;
}

/// Entry point called by the runtime after `dlopen("libkoala_readline.so")`.
void readline_native_lib_init(NativeLib *lib)
{
    kl_reg_func(lib, "tty_raw_on", _tty_raw_on);
    kl_reg_func(lib, "tty_raw_off", _tty_raw_off);
    kl_reg_func(lib, "tty_suspend_key", _tty_suspend_key);
    kl_reg_func(lib, "tty_quit_key", _tty_quit_key);
    kl_reg_func(lib, "tty_suspend", _tty_suspend);
    kl_reg_func(lib, "tty_quit", _tty_quit);
}
