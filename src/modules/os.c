/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <pwd.h>
#include <signal.h>
#include <spawn.h>
#include <sys/ioctl.h>
#include <sys/random.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>
#include "atom.h"
#include "bytesobj.h"
#include "dictobj.h"
#include "excobj.h"
#include "listobj.h"
#include "tupleobj.h"

#ifdef __cplusplus
extern "C" {
#endif

static TValue os_getenv(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *name = kl_arg_str(0);
    char *v = getenv(name);
    if (!v) return nil_value;
    return kl_val_str(atom(v));
}

static TValue os_setenv(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    char *name = kl_arg_str(0);
    char *value = kl_arg_str(1);
    setenv(name, value, 1);
    return nil_value;
}

static TValue os_unsetenv(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *name = kl_arg_str(0);
    unsetenv(name);
    return nil_value;
}

/*
 * pub func environ() dict[str, str]
 *
 * Return a one-time snapshot of the current process's environment variables.
 */
static TValue os_environ(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);

    /* External: the process environment, each entry has the form "KEY=VALUE" */
    extern char **environ;

    TValue dict = obj_value(kl_new_dict());

    for (char **entry = environ; entry && *entry; ++entry) {
        char *eq = strchr(*entry, '=');
        if (!eq) continue; /* skip malformed entries without '=' */

        TValue key = kl_val_nstr(*entry, eq - *entry);
        TValue val = kl_val_str(eq + 1);

        TValue ret = kl_call_set_item(&dict, &key, &val);
        if (is_error(&ret)) return ret;
    }

    return dict;
}

/*
 * Spawn attributes shared by os.run and os.system: give the child the default
 * SIGPIPE action back. This process ignores SIGPIPE (see os_native_lib_init),
 * and an ignored signal stays ignored across exec, so without this a child
 * like `yes` in `yes | head -1` would get EPIPE errors instead of exiting.
 */
static void init_spawn_attr(posix_spawnattr_t *attr)
{
    sigset_t sigdef;
    sigemptyset(&sigdef);
    sigaddset(&sigdef, SIGPIPE);

    posix_spawnattr_init(attr);
    posix_spawnattr_setsigdefault(attr, &sigdef);
    posix_spawnattr_setflags(attr, POSIX_SPAWN_SETSIGDEF);
}

/*
 * pub func system(cmd str) int
 *
 * Run `cmd` with `/bin/sh -c` and wait for it. Like system(3), return the raw
 * wait status (the exit code is `(status >> 8) & 0xFF`), or -1 if the shell
 * cannot be started. Spawned directly instead of via system(3), so the shell
 * gets the default SIGPIPE action (see init_spawn_attr).
 */
static TValue os_system(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *cmd = kl_arg_str(0);

    /* External: Environment variables (required by posix_spawn) */
    extern char **environ;

    char *argv[] = { "sh", "-c", cmd, NULL };

    posix_spawnattr_t attr;
    init_spawn_attr(&attr);

    pid_t pid;
    int spawn_status = posix_spawn(&pid, "/bin/sh", NULL, &attr, argv, environ);
    posix_spawnattr_destroy(&attr);

    if (spawn_status != 0) return int64_value(-1);

    int status;
    pid_t res;

    /* retry when interrupted by a signal */
    do {
        res = waitpid(pid, &status, 0);
    } while (res < 0 && errno == EINTR);

    if (res < 0) return int64_value(-1);
    return int64_value(status);
}

// --- Working Directory ---

/*
 * pub func getcwd() str
 *
 * Return the absolute pathname of the current working directory.
 * Panic if it cannot be determined (e.g. the directory was removed).
 */
static TValue os_getcwd(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);

    char buf[PATH_MAX];
    if (getcwd(buf, sizeof(buf)) == NULL) {
        raise_exc_fmt("getcwd failed: %s", strerror(errno));
        return error_value;
    }

    return kl_val_str(buf);
}

/*
 * pub func chdir(path str) bool
 *
 * Change the current working directory to `path`.
 * Return true on success, false if the directory cannot be entered.
 */
static TValue os_chdir(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *path = kl_arg_str(0);
    return bool_value(chdir(path) == 0);
}

// --- Raw Reader/Writer ---

typedef struct _RawObject {
    OBJECT_HEAD
    int fd;
    int closed;
} RawObject;

static TypeObject raw_reader_type;
static TypeObject raw_writer_type;

/*
 * Return 0 if `fd` is valid and open for `want` (O_RDONLY or O_WRONLY),
 * -1 otherwise. O_RDWR (sockets, "r+"/"w+" files) satisfies both directions.
 * F_GETFL fails with EBADF on invalid descriptors, so this covers the
 * plain validity check as well.
 */
static int fd_check_access(int fd, int want)
{
    int flags;
    do {
        flags = fcntl(fd, F_GETFL);
    } while (flags == -1 && errno == EINTR);

    if (flags == -1) return -1; /* EBADF: not an open descriptor */

    int mode = flags & O_ACCMODE;
    if (mode == O_RDWR) return 0;
    return mode == want ? 0 : -1;
}

/* Close the fd once; later calls are no-ops */
static void raw_close(RawObject *raw)
{
    if (!raw->closed) {
        close(raw->fd);
        raw->closed = 1;
    }
}

/* Finalizer of RawReader/RawWriter: close the fd if close() was never called */
static void raw_free(Object *self) { raw_close((RawObject *)self); }

/*
 * pub func __init__(fd int)
 *
 * Wrap the read half of `fd`, taking ownership of it.
 * Panic if `fd` is invalid or not open for reading.
 */
static TValue raw_reader_init(TValue *self, TValue *args, int nargs)
{
    RawObject *raw = SELF_AS(raw_reader_type);
    ASSERT(nargs == 1);
    int fd = (int)kl_arg_int64(0);

    if (fd_check_access(fd, O_RDONLY) != 0) {
        raise_exc_fmt("file descriptor %d is not open for reading", fd);
        return error_value;
    }

    raw->fd = fd;
    raw->closed = 0;
    return no_value;
}

/*
 * pub func read(data bytes) int
 *
 * Read raw bytes from the underlying OS buffer into `data`, blocking until
 * data is available or EOF is reached (0 is returned at EOF).
 * Satisfies the `io.Reader` trait.
 */
static TValue raw_reader_read(TValue *self, TValue *args, int nargs)
{
    RawObject *raw = SELF_AS(raw_reader_type);
    ASSERT(nargs == 1);
    BytesObject *buf = kl_arg_obj_as(0, bytes_type);

    if (raw->closed) {
        raise_exc_fmt("file descriptor %d is closed", raw->fd);
        return error_value;
    }

    /* retry when interrupted by a signal */
    ssize_t r;
    do {
        r = read(raw->fd, buf->data + buf->offset, buf->size);
    } while (r < 0 && errno == EINTR);

    return int64_value(r);
}

/*
 * pub func read_all() bytes
 *
 * Read from the read end until EOF (the writer side closes) and return all
 * bytes read as a single `bytes` object.
 * Panic if the fd is closed or the underlying read fails.
 */
static TValue raw_reader_read_all(TValue *self, TValue *args, int nargs)
{
    RawObject *raw = SELF_AS(raw_reader_type);
    ASSERT(nargs == 0);

    if (raw->closed) {
        raise_exc_fmt("file descriptor %d is closed", raw->fd);
        return error_value;
    }

    int cap = 8192;
    int len = 0;
    uint8_t *data = mm_alloc(cap);

    for (;;) {
        if (len == cap) {
            cap *= 2;
            data = mm_realloc(data, cap);
        }

        /* retry when interrupted by a signal */
        ssize_t r;
        do {
            r = read(raw->fd, data + len, cap - len);
        } while (r < 0 && errno == EINTR);

        if (r < 0) {
            raise_exc_fmt("read failed: %s", strerror(errno));
            mm_free(data);
            return error_value;
        }

        if (r == 0) break; /* EOF: the writer side closed */
        len += (int)r;
    }

    Object *bs = kl_bytes_from_data(data, (uint32_t)len);
    mm_free(data);
    return obj_value(bs);
}

/*
 * pub func close()
 *
 * Close the read end of the stream, releasing the OS resource.
 * Calling close() multiple times is a no-op. Satisfies `io.Closable`.
 */
static TValue raw_reader_close(TValue *self, TValue *args, int nargs)
{
    RawObject *raw = SELF_AS(raw_reader_type);
    ASSERT(nargs == 0);

    if (!raw->closed) {
        close(raw->fd);
        raw->closed = 1;
    }

    return no_value;
}

static MethodDef raw_reader_methods[] = {
    { "__init__", raw_reader_init },
    { "read", raw_reader_read },
    { "read_all", raw_reader_read_all },
    { "close", raw_reader_close },
    { NULL, NULL },
};

static TypeObject raw_reader_type = {
    ._type = &type_type,
    .name = "RawReader",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(RawObject),
    .methdefs = raw_reader_methods,
    ._free = raw_free,
};

/*
 * pub func __init__(fd int)
 *
 * Wrap the write half of `fd`, taking ownership of it.
 * Panic if `fd` is invalid or not open for writing.
 */
static TValue raw_writer_init(TValue *self, TValue *args, int nargs)
{
    RawObject *raw = SELF_AS(raw_writer_type);
    ASSERT(nargs == 1);
    int fd = (int)kl_arg_int64(0);

    if (fd_check_access(fd, O_WRONLY) != 0) {
        raise_exc_fmt("file descriptor %d is not open for writing", fd);
        return error_value;
    }

    raw->fd = fd;
    raw->closed = 0;
    return no_value;
}

/*
 * pub func write(bs bytes) int
 *
 * Write raw bytes from `bs` into the underlying OS buffer, blocking when
 * the OS buffer has no available capacity. Satisfies the `io.Writer` trait.
 */
static TValue raw_writer_write(TValue *self, TValue *args, int nargs)
{
    RawObject *raw = SELF_AS(raw_writer_type);
    ASSERT(nargs == 1);
    BytesObject *buf = kl_arg_obj_as(0, bytes_type);

    if (raw->closed) {
        raise_exc_fmt("file descriptor %d is closed", raw->fd);
        return error_value;
    }

    /* retry when interrupted by a signal */
    ssize_t r;
    do {
        r = write(raw->fd, buf->data + buf->offset, buf->size);
    } while (r < 0 && errno == EINTR);

    return int64_value(r);
}

/*
 * pub func write_str(s str) int
 *
 * Encode `s` string as UTF-8 and write it directly, avoiding an
 * intermediate bytes allocation.
 */
static TValue raw_writer_write_str(TValue *self, TValue *args, int nargs)
{
    RawObject *raw = SELF_AS(raw_writer_type);
    ASSERT(nargs == 1);
    StringObject *sobj = kl_arg_obj_as(0, str_type);

    if (raw->closed) {
        raise_exc_fmt("file descriptor %d is closed", raw->fd);
        return error_value;
    }

    /* retry when interrupted by a signal */
    ssize_t r;
    do {
        r = write(raw->fd, STR_BUF(sobj), STR_LEN(sobj));
    } while (r < 0 && errno == EINTR);

    return int64_value(r);
}

/*
 * pub func flush()
 *
 * Kernel descriptors are unbuffered, so this is a no-op, provided only to
 * fulfill the `io.Writer` contract.
 */
static TValue raw_writer_flush(TValue *self, TValue *args, int nargs)
{
    RawObject *raw = SELF_AS(raw_writer_type);
    ASSERT(nargs == 0);

    if (raw->closed) {
        raise_exc_fmt("file descriptor %d is closed", raw->fd);
        return error_value;
    }

    return no_value;
}

/*
 * pub func close()
 *
 * Close the write end of the stream and release its file descriptor.
 * Once all write ends are closed, readers observe EOF.
 * Calling close() multiple times is a no-op.
 */
static TValue raw_writer_close(TValue *self, TValue *args, int nargs)
{
    RawObject *raw = SELF_AS(raw_writer_type);
    ASSERT(nargs == 0);

    if (!raw->closed) {
        close(raw->fd);
        raw->closed = 1;
    }

    return no_value;
}

static MethodDef raw_writer_methods[] = {
    { "__init__", raw_writer_init },       { "write", raw_writer_write },
    { "write_str", raw_writer_write_str }, { "flush", raw_writer_flush },
    { "close", raw_writer_close },         { NULL, NULL },
};

static TypeObject raw_writer_type = {
    ._type = &type_type,
    .name = "RawWriter",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(RawObject),
    .methdefs = raw_writer_methods,
    ._free = raw_free,
};

/* Wrap a raw file descriptor, taking ownership of it */
static Object *kl_new_raw_object(TypeObject *tp, int fd)
{
    RawObject *raw = mm_alloc_obj(raw);
    INIT_OBJECT_HEAD(raw, tp, 0);
    raw->fd = fd;
    raw->closed = 0;
    return (Object *)raw;
}

/*
 * Create a pipe whose ends are close-on-exec, so they are never leaked into
 * child processes spawned later (e.g. via os.run).
 */
static int kl_pipe_cloexec(int fds[2])
{
    if (pipe(fds) < 0) return -1;

    fcntl(fds[0], F_SETFD, fcntl(fds[0], F_GETFD) | FD_CLOEXEC);
    fcntl(fds[1], F_SETFD, fcntl(fds[1], F_GETFD) | FD_CLOEXEC);
    return 0;
}

/*
 * pub func pipe() (RawReader, RawWriter)?
 *
 * Create an anonymous pipe. The tuple's first item is the read end
 * (RawReader), the second is the write end (RawWriter).
 * Both ends are close-on-exec: they will not leak into child processes
 * spawned later.
 */
static TValue os_pipe(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);

    int fds[2];
    if (kl_pipe_cloexec(fds) < 0) {
        /* nil if the pipe cannot be created (e.g., too many open descriptors) */
        return nil_value;
    }

    TValue items[2] = {
        obj_value(kl_new_raw_object(&raw_reader_type, fds[0])),
        obj_value(kl_new_raw_object(&raw_writer_type, fds[1])),
    };

    return obj_value(kl_new_tuple(items, 2));
}

// --- Process Management ---

typedef struct _Process {
    OBJECT_HEAD
    pid_t pid;
    Object *_stdin;
    Object *_stdout;
    Object *_stderr;
    int exit_status;
    int exited;
} Process;

static TypeObject process_type;

/* pub func stdin() RawWriter */
static TValue process_stdin(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    Process *proc = SELF_AS(process_type);
    return obj_value(proc->_stdin);
}

/* pub func stdout() RawReader */
static TValue process_stdout(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    Process *proc = SELF_AS(process_type);
    return obj_value(proc->_stdout);
}

/* pub func stderr() RawReader */
static TValue process_stderr(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    Process *proc = SELF_AS(process_type);
    return obj_value(proc->_stderr);
}

/* pub func pid() int */
static TValue process_pid(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    Process *proc = SELF_AS(process_type);
    return int64_value(proc->pid);
}

/* Non-blocking check of child process status, reused by poll() and wait() */
static void update_process_status(Process *proc)
{
    if (proc->exited) return;

    int status;
    pid_t res = waitpid(proc->pid, &status, WNOHANG);

    if (res == proc->pid) {
        proc->exited = 1;
        if (WIFEXITED(status)) {
            proc->exit_status = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            /* return negative signal number if terminated by signal (e.g., -9 for SIGKILL) */
            proc->exit_status = -WTERMSIG(status);
        } else {
            proc->exit_status = -1;
        }
    } else if (res < 0) {
        /* ECHILD: Child process no longer exists */
        proc->exited = 1;
        proc->exit_status = -1;
    }
}

/*
 * pub func poll() int?
 *
 * non-blocking check. Returns the exit code if the process has exited
 * (negative if terminated by a signal), or nil if it has not exited.
 */
static TValue process_poll(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    Process *proc = SELF_AS(process_type);
    update_process_status(proc);

    if (proc->exited) {
        return int64_value(proc->exit_status);
    }
    return nil_value;
}

/* Blocking wait for the child process to exit, shared by wait() and communicate() */
static void wait_process(Process *proc)
{
    if (!proc->exited) {
        int status;
        pid_t res;

        /* retry when interrupted by a signal */
        do {
            res = waitpid(proc->pid, &status, 0);
        } while (res < 0 && errno == EINTR);

        if (res == proc->pid) {
            proc->exited = 1;
            if (WIFEXITED(status)) {
                proc->exit_status = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                /* keep the same status with poll() */
                proc->exit_status = -WTERMSIG(status);
            } else {
                proc->exit_status = -1;
            }
        } else {
            proc->exited = 1;
            proc->exit_status = -1;
        }
    }
}

/* A growable byte buffer collecting one output stream in communicate() */
typedef struct _OutBuf {
    uint8_t *data;
    int len;
    int cap;
} OutBuf;

/*
 * Read what is available on `fd` into `buf`.
 * Return the read(2) result: > 0 bytes read, 0 at EOF, < 0 on error.
 */
static ssize_t outbuf_read(OutBuf *buf, int fd)
{
    if (buf->len == buf->cap) {
        buf->cap *= 2;
        buf->data = mm_realloc(buf->data, buf->cap);
    }

    ssize_t r = read(fd, buf->data + buf->len, buf->cap - buf->len);
    if (r > 0) buf->len += (int)r;
    return r;
}

/*
 * Drain `raw` once after poll() reported it ready; close it at EOF.
 * Return -1 on a read error, 0 otherwise.
 */
static int drain_once(RawObject *raw, OutBuf *buf)
{
    ssize_t r = outbuf_read(buf, raw->fd);
    if (r == 0) {
        raw_close(raw); /* EOF: the child closed its end */
    } else if (r < 0 && errno != EINTR && errno != EAGAIN) {
        return -1;
    }
    return 0;
}

/* The monotonic clock in milliseconds, for timeouts */
static int64_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* A deadline `seconds` from now on the now_ms() clock; a negative `seconds`
   is "no deadline" (-1) */
static int64_t deadline_after(double seconds)
{
    if (seconds < 0) return -1;
    return now_ms() + (int64_t)(seconds * 1000.0);
}

/* Milliseconds left until `deadline`, as poll() takes them: -1 for none */
static int ms_left(int64_t deadline)
{
    if (deadline < 0) return -1;
    int64_t left = deadline - now_ms();
    if (left <= 0) return 0;
    return left > INT32_MAX ? INT32_MAX : (int)left;
}

/* Wait for the child until `deadline` (-1: forever). Return 1 once it has
   exited, 0 if the deadline passed first. */
static int wait_until(Process *proc, int64_t deadline)
{
    if (deadline < 0) {
        wait_process(proc);
        return 1;
    }

    /* no blocking waitpid with a timeout: poll it, sleeping 1ms .. 50ms */
    long step_us = 1000;
    for (;;) {
        update_process_status(proc);
        if (proc->exited) return 1;

        int left = ms_left(deadline);
        if (left == 0) return 0;
        long us = (long)left * 1000 < step_us ? (long)left * 1000 : step_us;
        struct timespec ts = { us / 1000000, (us % 1000000) * 1000 };
        nanosleep(&ts, NULL);
        if (step_us < 50000) step_us *= 2;
    }
}

/*
 * The body of communicate(): send `input` (nil or bytes) to stdin, read
 * stdout and stderr until EOF, then wait for the child, all before `deadline`
 * (-1: none). Return the (stdout, stderr) tuple, nil
 * if the deadline passed first (the pipes stay open and the child running),
 * or an error.
 */
static TValue do_communicate(Process *proc, TValue *input, int64_t deadline)
{
    RawObject *in = (RawObject *)proc->_stdin;
    RawObject *out = (RawObject *)proc->_stdout;
    RawObject *err = (RawObject *)proc->_stderr;

    uint8_t *data = NULL;
    int size = 0;
    if (!is_nil(input)) {
        BytesObject *bs = (BytesObject *)to_obj(input);
        data = bs->data + bs->offset;
        size = bs->size;
    }

    if (size > 0 && in->closed) {
        raise_exc("communicate: stdin is closed");
        return error_value;
    }

    if (size == 0) {
        raw_close(in); /* nothing to send: let the child see EOF now */
    } else {
        /* never block on a full pipe: poll() decides when to write again */
        fcntl(in->fd, F_SETFL, fcntl(in->fd, F_GETFL) | O_NONBLOCK);
    }

    OutBuf obuf = { mm_alloc(8192), 0, 8192 };
    OutBuf ebuf = { mm_alloc(8192), 0, 8192 };
    int written = 0;

    for (;;) {
        struct pollfd fds[3];
        RawObject *raws[3];
        int n = 0;

        if (!in->closed) {
            fds[n] = (struct pollfd){ in->fd, POLLOUT, 0 };
            raws[n++] = in;
        }
        if (!out->closed) {
            fds[n] = (struct pollfd){ out->fd, POLLIN, 0 };
            raws[n++] = out;
        }
        if (!err->closed) {
            fds[n] = (struct pollfd){ err->fd, POLLIN, 0 };
            raws[n++] = err;
        }
        if (n == 0) break;

        int r = poll(fds, n, ms_left(deadline));
        if (r < 0) {
            if (errno == EINTR) continue;
            raise_exc_fmt("communicate: poll failed: %s", strerror(errno));
            goto fail;
        }
        if (r == 0) goto timeout;

        for (int i = 0; i < n; i++) {
            if (!fds[i].revents) continue;

            if (raws[i] == in) {
                ssize_t w = write(in->fd, data + written, size - written);
                if (w > 0) written += (int)w;
                /* all sent, or EPIPE: the child stopped reading its stdin */
                if (written == size || (w < 0 && errno != EINTR && errno != EAGAIN)) {
                    raw_close(in);
                }
            } else if (drain_once(raws[i], raws[i] == out ? &obuf : &ebuf) < 0) {
                raise_exc_fmt("communicate: read failed: %s", strerror(errno));
                goto fail;
            }
        }
    }

    if (!wait_until(proc, deadline)) goto timeout;

    TValue items[2] = {
        obj_value(kl_bytes_from_data(obuf.data, (uint32_t)obuf.len)),
        obj_value(kl_bytes_from_data(ebuf.data, (uint32_t)ebuf.len)),
    };
    mm_free(obuf.data);
    mm_free(ebuf.data);
    return obj_value(kl_new_tuple(items, 2));

timeout:
    mm_free(obuf.data);
    mm_free(ebuf.data);
    return nil_value;

fail:
    mm_free(obuf.data);
    mm_free(ebuf.data);
    return error_value;
}

/* The deadline for the `timeout` argument at `index` (seconds; negative: no
   timeout, -1) */
static int64_t arg_deadline(TValue *args, int nargs, int index)
{
    ASSERT(index < nargs);
    return deadline_after(to_float64(args + index));
}

/*
 * pub func wait(timeout = -1.0) int?
 *
 * Wait for the child process to exit and return its exit status (negative:
 * killed by that signal). With `timeout` >= 0, give up after that many
 * seconds and return nil if it is still running.
 */
static TValue process_wait(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    Process *proc = SELF_AS(process_type);
    if (!wait_until(proc, arg_deadline(args, nargs, 0))) return nil_value;
    return int64_value(proc->exit_status);
}

/*
 * pub func communicate(input bytes? = nil, timeout = -1.0) (bytes, bytes)?
 *
 * Write `input` to stdin, then close it, while reading stdout and stderr
 * until EOF; then wait for the child to exit. One poll() loop serves all
 * three pipes, so a large `input` cannot deadlock against a child that
 * blocks on writing its output. All three parent-side ends are closed and
 * the child is reaped on return. A stream that is not a pipe (see os.run)
 * gives empty bytes.
 * Return the (stdout, stderr) bytes; the exit status is then available from
 * wait() or poll(). With `timeout` >= 0, give up after that many seconds and
 * return nil: the child keeps running, the pipes stay open, and the output
 * read so far is dropped.
 * Panic if `input` is non-empty but stdin is already closed, or if
 * poll()/read() fails.
 */
static TValue process_communicate(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    Process *proc = SELF_AS(process_type);
    return do_communicate(proc, &args[0], arg_deadline(args, nargs, 1));
}

/*
 * pub func terminate()
 *
 * send SIGTERM signal to request the subprocess to exit gracefully
 */
static TValue process_terminate(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    Process *proc = SELF_AS(process_type);

    if (proc->pid > 0) {
        kill(proc->pid, SIGTERM);
    }

    return no_value;
}

/* pub func kill()
 *
 * send SIGKILL signal to forcibly terminate the subprocess
 */
static TValue process_kill(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    Process *proc = SELF_AS(process_type);
    if (proc->pid > 0) {
        kill(proc->pid, SIGKILL);
    }
    return no_value;
}

/*
 * Finalizer of Process: reap the child if it has already exited, so it does
 * not stay a zombie. A running child is left alone: blocking here would stall
 * the runtime. The pipe ends are separate objects with their own finalizer.
 */
static void process_free(Object *self) { update_process_status((Process *)self); }

static MethodDef process_methods[] = {
    { "stdin", process_stdin },
    { "stdout", process_stdout },
    { "stderr", process_stderr },
    { "pid", process_pid },
    { "poll", process_poll },
    { "wait", process_wait },
    { "communicate", process_communicate },
    { "terminate", process_terminate },
    { "kill", process_kill },
    { NULL, NULL },
};

static TypeObject process_type = {
    ._type = &type_type,
    .name = "Process",
    .flags = TP_FLAGS_CLASS,
    .objsize = sizeof(Process),
    .methdefs = process_methods,
    ._free = process_free,
};

static Object *kl_new_process(pid_t pid, Object *stdin_f, Object *stdout_f, Object *stderr_f)
{
    Process *proc = mm_alloc_obj(proc);
    INIT_OBJECT_HEAD(proc, &process_type, 0);

    proc->pid = pid;
    proc->_stdin = stdin_f;
    proc->_stdout = stdout_f;
    proc->_stderr = stderr_f;
    proc->exit_status = 0;
    proc->exited = 0;

    return (Object *)proc;
}

/* Append "KEY=VALUE" for one mapping of the `env` dict of os.run */
static void env_entry_visit(TValue *key, TValue *value, void *arg)
{
    Vector *envs = arg;
    Object *k = to_obj(key);
    Object *v = to_obj(value);
    BUF(buf);
    buf_write_nstr(&buf, STR_BUF(k), STR_LEN(k));
    buf_write_char(&buf, '=');
    buf_write_nstr(&buf, STR_BUF(v), STR_LEN(v));
    char *entry = str_ndup(BUF_STR(buf), BUF_LEN(buf));
    FINI_BUF(buf);
    vector_push_back(envs, &entry);
}

/* Free the "KEY=VALUE" strings of `envs` (the NULL terminator included) */
static void free_env_entries(Vector *envs)
{
    char *entry;
    vector_foreach(entry, envs) {
        mm_free(entry);
    }
    vector_fini(envs);
}

/* Modes of a child's standard stream in os.run (PIPE ... in __os__.kl) */
#define STDIO_PIPE    0
#define STDIO_INHERIT 1
#define STDIO_DEVNULL 2
#define STDIO_STDOUT  3 /* stderr only: the same as stdout, 2>&1 */

/* Close the pipe ends of os.run that are open (-1: none) */
static void close_pipes(int fds[3][2])
{
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            if (fds[i][j] >= 0) close(fds[i][j]);
        }
    }
}

/* The parent-side end of a stream that is not a pipe: closed, so that using
   it panics ("file descriptor -1 is closed") */
static Object *kl_new_closed_raw(TypeObject *tp)
{
    RawObject *raw = (RawObject *)kl_new_raw_object(tp, -1);
    raw->closed = 1;
    return (Object *)raw;
}

/*
 * pub func run(executable str, args list[str]? = nil, cwd str? = nil,
 *              env dict[str, str]? = nil, stdin = PIPE, stdout = PIPE,
 *              stderr = PIPE) Process?
 *
 * `cwd` is the child's working directory (nil: this process's), `env` its
 * whole environment (nil: a copy of this process's). `stdin` / `stdout` /
 * `stderr` choose each stream: a pipe to this process, this process's own
 * stream, /dev/null, or (stderr only) the same as stdout.
 */
static TValue os_run(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 7);
    char *executable = kl_arg_str(0);
    char *cwd = is_nil(&args[2]) ? NULL : kl_arg_str(2);

    int modes[3] = { (int)kl_arg_int64(4), (int)kl_arg_int64(5), (int)kl_arg_int64(6) };
    static const char *names[3] = { "stdin", "stdout", "stderr" };
    for (int i = 0; i < 3; i++) {
        int max = i == 2 ? STDIO_STDOUT : STDIO_DEVNULL;
        if (modes[i] < STDIO_PIPE || modes[i] > max) {
            raise_exc_fmt("run: invalid mode %d for %s", modes[i], names[i]);
            return error_value;
        }
    }

    /* the child's environment: "KEY=VALUE" strings, NULL-terminated */
    Vector envs;
    vector_init_ptr(&envs);
    if (!is_nil(&args[3])) {
        kl_dict_foreach(kl_arg_obj(3), env_entry_visit, &envs);
        char *end = NULL;
        vector_push_back(&envs, &end);
    }
    int arg_count;

    Object *arg_list = NULL;

    if (is_nil(&args[1])) {
        arg_count = 0;
    } else {
        arg_list = kl_arg_obj(1);
        arg_count = kl_list_len(arg_list);
    }

    // 1. prepare arguments

    Vector vec;
    vector_init_ptr(&vec);

    vector_push_back(&vec, &executable);

    for (int i = 0; i < arg_count; i++) {
        TValue *item = kl_list_get(arg_list, i);
        Object *ob = to_obj(item);
        ASSERT(IS_STR(ob));
        vector_push_back(&vec, &STR_BUF(ob));
    }

    char *empty = NULL;
    vector_push_back(&vec, &empty);

    char **argv = VECTOR_ITEMS(&vec, char *);

    // 2. A close-on-exec pipe for each PIPE stream; [0] is the read end
    int fds[3][2] = { { -1, -1 }, { -1, -1 }, { -1, -1 } };
    for (int i = 0; i < 3; i++) {
        if (modes[i] == STDIO_PIPE && kl_pipe_cloexec(fds[i]) < 0) {
            close_pipes(fds);
            goto error;
        }
    }

    // 3. In the subprocess: set up fd 0, 1 and 2 for each mode. The pipe
    //    ends are close-on-exec, so only the dup2'ed copies stay open.
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    for (int i = 0; i < 3; i++) {
        if (modes[i] == STDIO_PIPE) {
            /* the child's end: read for stdin, write for stdout/stderr */
            posix_spawn_file_actions_adddup2(&actions, fds[i][i == 0 ? 0 : 1], i);
        } else if (modes[i] == STDIO_DEVNULL) {
            int flags = i == 0 ? O_RDONLY : O_WRONLY;
            posix_spawn_file_actions_addopen(&actions, i, "/dev/null", flags, 0);
        } else if (modes[i] == STDIO_STDOUT) {
            /* stderr after stdout is set up: 2>&1 */
            posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO, STDERR_FILENO);
        }
        /* STDIO_INHERIT: the child keeps this process's stream */
    }

    /* In the subprocess: enter `cwd`; a missing directory fails the spawn */
    if (cwd) posix_spawn_file_actions_addchdir_np(&actions, cwd);

    // 4. launch subprocess
    /* External: Environment variables (required by posix_spawn) */
    extern char **environ;

    /* the child gets the default SIGPIPE action back */
    posix_spawnattr_t attr;
    init_spawn_attr(&attr);

    /* an `env` dict replaces the environment; nil inherits this process's */
    char **envp = vector_empty(&envs) ? environ : VECTOR_ITEMS(&envs, char *);

    pid_t pid;
    int spawn_status = posix_spawnp(&pid,       /* Output: Child process PID */
                                    executable, /* Executable (searched in PATH) */
                                    &actions,   /* File actions */
                                    &attr,      /* Attributes */
                                    argv,       /* Argument list */
                                    envp        /* Environment variables */
    );

    // 5. Clean up temporary resources
    posix_spawn_file_actions_destroy(&actions);
    posix_spawnattr_destroy(&attr);

    if (spawn_status != 0) {
        /* posix_spawn failed */
        close_pipes(fds);
        goto error;
    }

    // 6. Parent: close the child's pipe ends, wrap the parent's ones; a
    //    stream that is not a pipe gets a closed end, so using it panics
    for (int i = 0; i < 3; i++) {
        if (modes[i] == STDIO_PIPE) close(fds[i][i == 0 ? 0 : 1]);
    }

    Object *_stdin = modes[0] == STDIO_PIPE ? kl_new_raw_object(&raw_writer_type, fds[0][1])
                                            : kl_new_closed_raw(&raw_writer_type);
    Object *_stdout = modes[1] == STDIO_PIPE ? kl_new_raw_object(&raw_reader_type, fds[1][0])
                                             : kl_new_closed_raw(&raw_reader_type);
    Object *_stderr = modes[2] == STDIO_PIPE ? kl_new_raw_object(&raw_reader_type, fds[2][0])
                                             : kl_new_closed_raw(&raw_reader_type);

    // 11. Construct and return the Process object
    Object *proc = kl_new_process(pid, _stdin, _stdout, _stderr);

    vector_fini(&vec);
    free_env_entries(&envs);
    return obj_value(proc);

error:
    vector_fini(&vec);
    free_env_entries(&envs);
    return nil_value;
}

static TValue os_errno(TValue *self, TValue *args, int nargs) { return int64_value(errno); }

/*
 * pub func strerror(code int) str
 *
 * Return the system message for the error number `code`, e.g. "No such file
 * or directory" for ENOENT; an unknown number gives "Unknown error N".
 */
static TValue os_strerror(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int code = (int)kl_arg_int64(0);
    return kl_val_str(strerror(code));
}

/*
 * pub func expand_vars(s str) str
 *
 * Return `s` with each `$NAME` and `${NAME}` replaced by the value of the
 * environment variable NAME (letters, digits and '_', not starting with a
 * digit). A variable that is not set, a '$' not followed by a name, and an
 * unclosed `${` are left as they are.
 */
static TValue os_expand_vars(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    Object *so = kl_arg_obj_as(0, str_type);
    char *str = STR_BUF(so);
    size_t len = STR_LEN(so);

    BUF(buf);
    size_t i = 0;
    while (i < len) {
        if (str[i] != '$') {
            buf_write_char(&buf, str[i++]);
            continue;
        }

        /* the name: after "${" up to '}', or the longest run after '$' */
        int braced = i + 1 < len && str[i + 1] == '{';
        size_t start = i + (braced ? 2 : 1);
        size_t end = start;
        if (start < len && (isalpha((unsigned char)str[start]) || str[start] == '_')) {
            while (end < len && (isalnum((unsigned char)str[end]) || str[end] == '_')) end++;
        }
        size_t next = end;
        if (braced) {
            if (end == start || end >= len || str[end] != '}') end = start; /* not ${NAME} */
            else next = end + 1;
        }
        if (end == start) {
            buf_write_char(&buf, str[i++]); /* a plain '$' */
            continue;
        }

        char name[256];
        size_t nlen = end - start;
        char *value = NULL;
        if (nlen < sizeof(name)) {
            memcpy(name, str + start, nlen);
            name[nlen] = '\0';
            value = getenv(name);
        }
        if (value) {
            buf_write_str(&buf, value);
        } else {
            buf_write_nstr(&buf, str + i, (int)(next - i)); /* not set: kept */
        }
        i = next;
    }

    TValue res = kl_val_nstr(BUF_STR(buf), BUF_LEN(buf));
    FINI_BUF(buf);
    return res;
}

/*
 * pub func nice(inc int) int?
 *
 * Add `inc` to this process's nice value (a higher value is a lower CPU
 * priority; only a privileged process may lower it) and return the new value,
 * or nil on failure.
 */
static TValue os_nice(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int inc = (int)kl_arg_int64(0);
    errno = 0;
    int r = nice(inc);
    if (r == -1 && errno != 0) return nil_value; /* -1 is also a valid value */
    return int64_value(r);
}

/*
 * pub func getloadavg() (float64, float64, float64)?
 *
 * Return the system load averages over the last 1, 5 and 15 minutes, or nil
 * if they are not available.
 */
static TValue os_getloadavg(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    double la[3];
    if (getloadavg(la, 3) != 3) return nil_value;
    TValue items[3] = { float64_value(la[0]), float64_value(la[1]), float64_value(la[2]) };
    return obj_value(kl_new_tuple(items, 3));
}

/* pub func getppid() int */
static TValue os_getppid(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    return int64_value(getppid());
}

/* pub func getuid() int */
static TValue os_getuid(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    return int64_value(getuid());
}

/* pub func geteuid() int */
static TValue os_geteuid(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    return int64_value(geteuid());
}

/* pub func getgid() int */
static TValue os_getgid(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    return int64_value(getgid());
}

/*
 * pub func getlogin() str?
 *
 * Return the name of the user logged in on this process's controlling
 * terminal (getlogin(3)), else the user name of the real user ID; nil if
 * neither is known.
 */
static TValue os_getlogin(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    char *name = getlogin();
    if (name && name[0]) return kl_val_str(name);

    struct passwd *pw = getpwuid(getuid());
    if (pw && pw->pw_name && pw->pw_name[0]) return kl_val_str(pw->pw_name);
    return nil_value;
}

/* pub func isatty(fd int) bool */
static TValue os_isatty(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return bool_value(isatty((int)kl_arg_int64(0)) == 1);
}

/*
 * pub func terminal_size(fd = 1) (int, int)?
 *
 * Return the (columns, lines) of the terminal on `fd`, or nil if `fd` is not
 * a terminal or its size is unknown.
 */
static TValue os_terminal_size(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int fd = (int)kl_arg_int64(0);

    struct winsize ws;
    if (ioctl(fd, TIOCGWINSZ, &ws) != 0 || ws.ws_col == 0) return nil_value;
    TValue items[2] = { int64_value(ws.ws_col), int64_value(ws.ws_row) };
    return obj_value(kl_new_tuple(items, 2));
}

/*
 * pub func umask(mask int) int
 *
 * Set this process's file mode creation mask to `mask` and return the
 * previous one.
 */
static TValue os_umask(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    mode_t old = umask((mode_t)kl_arg_int64(0) & 0777);
    return int64_value(old);
}

/* pub func getpid() int */
static TValue os_getpid(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    return int64_value(getpid());
}

/*
 * pub func cpu_count() int
 *
 * Return the number of online CPUs, or 1 if it cannot be determined.
 */
static TValue os_cpu_count(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return int64_value(n > 0 ? n : 1);
}

/*
 * pub func platform() str
 *
 * Return the operating system this runtime was built for.
 */
static TValue os_platform(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
#if defined(__linux__)
    return kl_val_str("linux");
#elif defined(__APPLE__)
    return kl_val_str("darwin");
#elif defined(__FreeBSD__)
    return kl_val_str("freebsd");
#elif defined(_WIN32)
    return kl_val_str("windows");
#else
    return kl_val_str("unknown");
#endif
}

/* Return 1 if `path` is a regular file this process may execute */
static int is_executable_file(char *path)
{
    struct stat sb;
    return stat(path, &sb) == 0 && S_ISREG(sb.st_mode) && access(path, X_OK) == 0;
}

/*
 * pub func which(name str) str?
 *
 * Return the path of the program `name` as a shell would find it: `name`
 * itself if it has a '/', else the first `dir/name` in PATH that is an
 * executable regular file (an empty PATH entry is the working directory).
 * Return nil if there is none.
 */
static TValue os_which(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *name = kl_arg_str(0);
    if (name[0] == '\0') return nil_value;

    if (strchr(name, '/')) {
        return is_executable_file(name) ? kl_val_str(name) : nil_value;
    }

    char *path = getenv("PATH");
    if (!path) path = "/usr/local/bin:/usr/bin:/bin";

    for (char *p = path;; ++p) {
        char *end = strchr(p, ':');
        size_t len = end ? (size_t)(end - p) : strlen(p);

        BUF(buf);
        if (len == 0) {
            buf_write_str(&buf, name);
        } else {
            buf_write_nstr(&buf, p, (int)len);
            if (p[len - 1] != '/') buf_write_char(&buf, '/');
            buf_write_str(&buf, name);
        }
        int found = is_executable_file(BUF_STR(buf));
        TValue res = found ? kl_val_nstr(BUF_STR(buf), BUF_LEN(buf)) : nil_value;
        FINI_BUF(buf);
        if (found) return res;

        if (!end) break;
        p = end;
    }
    return nil_value;
}

/*
 * pub func home_dir() str?
 *
 * Return the home directory of this process's user: $HOME when set and not
 * empty, else the user database entry. Return nil if neither is known.
 */
static TValue os_home_dir(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    char *home = getenv("HOME");
    if (home && home[0]) return kl_val_str(home);

    struct passwd *pw = getpwuid(getuid());
    if (pw && pw->pw_dir && pw->pw_dir[0]) return kl_val_str(pw->pw_dir);
    return nil_value;
}

/*
 * pub func hostname() str
 *
 * Return the host name of this machine. Panic if it cannot be read.
 */
static TValue os_hostname(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    char buf[256];
    if (gethostname(buf, sizeof(buf)) != 0) {
        raise_exc_fmt("gethostname failed: %s", strerror(errno));
        return error_value;
    }
    buf[sizeof(buf) - 1] = '\0';
    return kl_val_str(buf);
}

/*
 * pub func uname() (str, str, str, str, str)
 *
 * Return (sysname, nodename, release, version, machine) of the running
 * kernel, e.g. ("Linux", "myhost", "6.8.0", "#1 SMP ...", "x86_64").
 */
static TValue os_uname(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    struct utsname u;
    if (uname(&u) != 0) {
        raise_exc_fmt("uname failed: %s", strerror(errno));
        return error_value;
    }
    TValue items[5] = {
        kl_val_str(u.sysname), kl_val_str(u.nodename), kl_val_str(u.release),
        kl_val_str(u.version), kl_val_str(u.machine),
    };
    return obj_value(kl_new_tuple(items, 5));
}

/*
 * pub func urandom(n int) bytes
 *
 * Return `n` random bytes from the OS (getrandom(2)), suitable for keys and
 * tokens. Panic if `n` is negative or the OS source fails.
 */
static TValue os_urandom(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int64_t n = kl_arg_int64(0);
    if (n < 0 || n > INT32_MAX) {
        raise_exc_fmt("urandom: invalid size %" PRId64, n);
        return error_value;
    }

    uint8_t *data = mm_alloc(n > 0 ? (int)n : 1);
    int64_t got = 0;
    while (got < n) {
        ssize_t r = getrandom(data + got, (size_t)(n - got), 0);
        if (r < 0) {
            if (errno == EINTR) continue;
            raise_exc_fmt("urandom failed: %s", strerror(errno));
            mm_free(data);
            return error_value;
        }
        got += r;
    }

    Object *bs = kl_bytes_from_data(data, (uint32_t)n);
    mm_free(data);
    return obj_value(bs);
}

/*
 * pub func kill(pid int, sig int) bool
 *
 * Send the signal `sig` to the process `pid` (kill(2)); `sig` 0 only checks
 * that the process exists and may be signalled. Return true on success.
 */
static TValue os_kill(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    pid_t pid = (pid_t)kl_arg_int64(0);
    int sig = (int)kl_arg_int64(1);
    return bool_value(kill(pid, sig) == 0);
}

void os_native_lib_init(NativeLib *lib)
{
    /* a write to a pipe whose reader exited fails with EPIPE instead of killing us */
    signal(SIGPIPE, SIG_IGN);

    kl_reg_func(lib, "getenv", os_getenv);
    kl_reg_func(lib, "setenv", os_setenv);
    kl_reg_func(lib, "unsetenv", os_unsetenv);
    kl_reg_func(lib, "environ", os_environ);
    kl_reg_func(lib, "getcwd", os_getcwd);
    kl_reg_func(lib, "chdir", os_chdir);
    kl_reg_func(lib, "pipe", os_pipe);
    kl_reg_func(lib, "system", os_system);
    kl_reg_func(lib, "run", os_run);
    kl_reg_func(lib, "errno", os_errno);
    kl_reg_func(lib, "strerror", os_strerror);
    kl_reg_func(lib, "getpid", os_getpid);
    kl_reg_func(lib, "getppid", os_getppid);
    kl_reg_func(lib, "expand_vars", os_expand_vars);
    kl_reg_func(lib, "nice", os_nice);
    kl_reg_func(lib, "getloadavg", os_getloadavg);
    kl_reg_func(lib, "getuid", os_getuid);
    kl_reg_func(lib, "geteuid", os_geteuid);
    kl_reg_func(lib, "getgid", os_getgid);
    kl_reg_func(lib, "getlogin", os_getlogin);
    kl_reg_func(lib, "isatty", os_isatty);
    kl_reg_func(lib, "terminal_size", os_terminal_size);
    kl_reg_func(lib, "umask", os_umask);
    kl_reg_func(lib, "cpu_count", os_cpu_count);
    kl_reg_func(lib, "platform", os_platform);
    kl_reg_func(lib, "which", os_which);
    kl_reg_func(lib, "home_dir", os_home_dir);
    kl_reg_func(lib, "hostname", os_hostname);
    kl_reg_func(lib, "uname", os_uname);
    kl_reg_func(lib, "urandom", os_urandom);
    kl_reg_func(lib, "kill", os_kill);
    kl_reg_type(lib, &raw_reader_type);
    kl_reg_type(lib, &raw_writer_type);
    kl_reg_type(lib, &process_type);
}

#ifdef __cplusplus
}
#endif
