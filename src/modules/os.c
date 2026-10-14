/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <spawn.h>
#include <sys/wait.h>
#include "atom.h"
#include "listobj.h"

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

static TValue os_system(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    char *cmd = kl_arg_str(0);
    int ret = system(cmd);
    return int64_value(ret);
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

/* pub func stdin() io.Writer */
static TValue process_stdin(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    Process *proc = SELF_AS(process_type);
    return obj_value(proc->_stdin);
}

/* pub func stdout() io.Reader */
static TValue process_stdout(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 0);
    Process *proc = SELF_AS(process_type);
    return obj_value(proc->_stdout);
}

/* pub func stderr() io.Reader */
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
        if (waitpid(proc->pid, &status, 0) == proc->pid) {
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
    .priv_size = sizeof(Process),
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

    // 2. Create three sets of pipes (Parent <-> Child)
    int stdin_pipe[2], stdout_pipe[2], stderr_pipe[2];

    if (pipe(stdin_pipe) < 0 || pipe(stdout_pipe) < 0 || pipe(stderr_pipe) < 0) {
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

    // 10. Wrap pipe ends with unified File objects
    extern Object *kl_new_file(TValue path, TValue mode, int fd);

    TValue path = nil_value;
    TValue read_mode = obj_value(kl_new_str("r"));
    TValue write_mode = obj_value(kl_new_str("w"));

    Object *_stdin = kl_new_file(path, write_mode, stdin_pipe[1]);
    Object *_stdout = kl_new_file(path, read_mode, stdout_pipe[0]);
    Object *_stderr = kl_new_file(path, read_mode, stderr_pipe[0]);

    // 11. Construct and return the Process object
    Object *proc = kl_new_process(pid, _stdin, _stdout, _stderr);
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
    kl_reg_func(lib, "system", os_system);
    kl_reg_func(lib, "run", os_run);
    kl_reg_type(lib, &process_type);
}

#ifdef __cplusplus
}
#endif
