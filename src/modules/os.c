/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <fcntl.h>
#include <limits.h>
#include <spawn.h>
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

static TValue os_system(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *cmd = kl_arg_str(0);
    int ret = system(cmd);
    return int64_value(ret);
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

/*
 * pub func wait() int
 *
 * Blocking wait for the child process to exit.
 */
static TValue process_wait(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    Process *proc = SELF_AS(process_type);

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
    return int64_value(proc->exit_status);
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

static MethodDef process_methods[] = {
    { "stdin", process_stdin },
    { "stdout", process_stdout },
    { "stderr", process_stderr },
    { "pid", process_pid },
    { "poll", process_poll },
    { "wait", process_wait },
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

// pub func run(executable str, args list[str]? = nil) Process? {}
static TValue os_run(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    char *executable = kl_arg_str(0);
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

    // 2. Create three close-on-exec pipes (Parent <-> Child)
    int stdin_pipe[2], stdout_pipe[2], stderr_pipe[2];

    if (kl_pipe_cloexec(stdin_pipe) < 0) {
        goto error;
    }

    if (kl_pipe_cloexec(stdout_pipe) < 0) {
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        goto error;
    }

    if (kl_pipe_cloexec(stderr_pipe) < 0) {
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        goto error;
    }

    // 3. Configure posix_spawn file actions
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);

    // 4. In the subprocess: redirect the pipe ends to standard input/output/error
    posix_spawn_file_actions_adddup2(&actions, stdin_pipe[0], STDIN_FILENO);
    posix_spawn_file_actions_adddup2(&actions, stdout_pipe[1], STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&actions, stderr_pipe[1], STDERR_FILENO);

    /* 5. In the subprocess: close the original pipe ends (already duplicated to 0/1/2) */
    posix_spawn_file_actions_addclose(&actions, stdin_pipe[0]);
    posix_spawn_file_actions_addclose(&actions, stdout_pipe[1]);
    posix_spawn_file_actions_addclose(&actions, stderr_pipe[1]);

    /* 6. In the subprocess: close the pipe ends used by the parent process */
    posix_spawn_file_actions_addclose(&actions, stdin_pipe[1]);
    posix_spawn_file_actions_addclose(&actions, stdout_pipe[0]);
    posix_spawn_file_actions_addclose(&actions, stderr_pipe[0]);

    // 7. launch subprocess
    /* External: Environment variables (required by posix_spawn) */
    extern char **environ;

    pid_t pid;
    int spawn_status = posix_spawnp(&pid,       /* Output: Child process PID */
                                    executable, /* Executable (searched in PATH) */
                                    &actions,   /* File actions */
                                    NULL,       /* Default attributes */
                                    argv,       /* Argument list */
                                    environ     /* Environment variables */
    );

    // 8. Clean up temporary resources
    posix_spawn_file_actions_destroy(&actions);

    if (spawn_status != 0) {
        /* posix_spawn failed */
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
        goto error;
    }

    // 9. Parent: Close unneeded pipe ends
    close(stdin_pipe[0]);  /* Close stdin read end (used by child) */
    close(stdout_pipe[1]); /* Close stdout write end (used by child) */
    close(stderr_pipe[1]); /* Close stderr write end (used by child) */

    // 10. Wrap the parent-side pipe ends as raw stream objects

    Object *_stdin = kl_new_raw_object(&raw_writer_type, stdin_pipe[1]);
    Object *_stdout = kl_new_raw_object(&raw_reader_type, stdout_pipe[0]);
    Object *_stderr = kl_new_raw_object(&raw_reader_type, stderr_pipe[0]);

    // 11. Construct and return the Process object
    Object *proc = kl_new_process(pid, _stdin, _stdout, _stderr);

    vector_fini(&vec);
    return obj_value(proc);

error:
    vector_fini(&vec);
    return nil_value;
}

void os_native_lib_init(NativeLib *lib)
{
    kl_reg_func(lib, "getenv", os_getenv);
    kl_reg_func(lib, "setenv", os_setenv);
    kl_reg_func(lib, "unsetenv", os_unsetenv);
    kl_reg_func(lib, "environ", os_environ);
    kl_reg_func(lib, "getcwd", os_getcwd);
    kl_reg_func(lib, "chdir", os_chdir);
    kl_reg_func(lib, "pipe", os_pipe);
    kl_reg_func(lib, "system", os_system);
    kl_reg_func(lib, "run", os_run);
    kl_reg_type(lib, &raw_reader_type);
    kl_reg_type(lib, &raw_writer_type);
    kl_reg_type(lib, &process_type);
}

#ifdef __cplusplus
}
#endif
