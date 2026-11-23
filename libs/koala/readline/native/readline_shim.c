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
 *
 * The program's terminal is restored however it ends:
 * - exit() and a normal return from main run restore_at_exit, so the
 *   original mode comes back after a panic too;
 * - a fatal signal (Ctrl-C and friends, abort, a crash) runs
 *   fatal_signal first: the original mode, and the escape sequence
 *   koala/tui writes when it leaves the screen (reset the scroll region,
 *   leave the alternate screen, bracketed paste off), then the signal is
 *   raised again with its old disposition. On a terminal not in the
 *   alternate screen the sequence is a no-op; on a pipe it is not
 *   written.
 */

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "object.h"

static struct termios g_orig_termios;
static int g_raw_fd = -1;
static int g_atexit_registered = 0;

// Reset the scroll region, leave the alternate screen, bracketed paste
// off: the last words koala/tui writes when it leaves the screen.
#define RESTORE_SEQ "\x1b[r\x1b[?1049l\x1b[?2004l"

static void restore_termios(void)
{
    if (g_raw_fd >= 0) {
        tcsetattr(g_raw_fd, TCSAFLUSH, &g_orig_termios);
        g_raw_fd = -1;
    }
}

static void write_restore_seq(void)
{
    if (isatty(1)) {
        (void)!write(1, RESTORE_SEQ, sizeof(RESTORE_SEQ) - 1);
    }
}

static void restore_at_exit(void)
{
    restore_termios();
    write_restore_seq();
}

// The signals that end a program while its terminal is raw, with their
// dispositions from before raw mode: the deliberate ends (Ctrl-C and the
// like), abort() — the C-level panic path — and the crash faults, so the
// terminal comes back for those too. Only syscalls run in the handler.
#define NFATAL 9
static const int g_fatal_sigs[NFATAL] = {
    SIGINT, SIGTERM, SIGHUP, SIGQUIT,
    SIGABRT, SIGSEGV, SIGBUS, SIGFPE, SIGILL,
};
static struct sigaction g_fatal_prev[NFATAL];
static int g_handlers_installed = 0;

static void fatal_signal(int sig)
{
    restore_termios();
    write_restore_seq();
    for (int i = 0; i < NFATAL; i++) {
        if (g_fatal_sigs[i] == sig) {
            sigaction(sig, &g_fatal_prev[i], NULL);
            break;
        }
    }
    raise(sig);
}

// Ctrl-C while a program watches for it: only a flag, never an exit, so a
// streaming request or a long tool run can be stopped where it is. Set from
// the handler, so it is sig_atomic_t and nothing else runs there.
static volatile sig_atomic_t g_int_flag = 0;
static int g_int_watch = 0;

static void interrupt_signal(int sig)
{
    (void)sig;
    g_int_flag = 1;
}

static void install_fatal_handlers(void)
{
    if (g_handlers_installed) return;
    g_handlers_installed = 1;
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = fatal_signal;
    sigemptyset(&sa.sa_mask);
    for (int i = 0; i < NFATAL; i++) {
        sigaction(g_fatal_sigs[i], &sa, &g_fatal_prev[i]);
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
    install_fatal_handlers();

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

// func tty_interrupt_watch(on bool)
//
// While watching, Ctrl-C (SIGINT) only raises a flag instead of ending the
// program, so a long run can stop where it is; the flag is read with
// tty_interrupt_taken. Off puts the fatal handler back: Ctrl-C restores the
// terminal and leaves the program, as before.
static TValue _tty_interrupt_watch(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int on = kl_arg_bool(0);

    // SIGINT is one of the fatal signals, so its slot exists once the
    // fatal handlers are installed; watchers need that state too.
    install_fatal_handlers();
    if (on) {
        if (g_int_watch) return nil_value;
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = interrupt_signal;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGINT, &sa, NULL);
        g_int_watch = 1;
    } else if (g_int_watch) {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = fatal_signal;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGINT, &sa, NULL);
        g_int_watch = 0;
    }
    return nil_value;
}

// func tty_interrupt_taken() bool
//
// True when Ctrl-C arrived since the last call; reading it clears it, so
// one press wakes one wait loop.
static TValue _tty_interrupt_taken(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    if (!g_int_flag) return bool_value(0);
    g_int_flag = 0;
    return bool_value(1);
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
    kl_reg_func(lib, "tty_interrupt_watch", _tty_interrupt_watch);
    kl_reg_func(lib, "tty_interrupt_taken", _tty_interrupt_taken);
}
