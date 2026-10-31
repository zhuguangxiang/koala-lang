# Koala `std/os` API Reference

## Contents

**Classes**

| Name | Summary |
|------|---------|
| [`Process`](#process) | Represents a spawned operating system process handle. |
| [`RawReader`](#rawreader) | A RawReader is the read half of an OS-level file descriptor or handle. |
| [`RawWriter`](#rawwriter) | A RawWriter is the write half of an OS-level file descriptor or handle. |

**Functions**

| Name | Summary |
|------|---------|
| [`getenv`](#getenv) | Get the value of an environment variable. Returns `nil` if the variable is not set. |
| [`setenv`](#setenv) | Set the value of an environment variable. If the variable already exists, its value is overwritten. |
| [`unsetenv`](#unsetenv) | Unset (remove) an environment variable. Does nothing if the variable is not set. |
| [`environ`](#environ) | Return a snapshot of the current process's environment variables. |
| [`getcwd`](#getcwd) | Return the absolute pathname of the current working directory. |
| [`chdir`](#chdir) | Change the current working directory to `path`. |
| [`pipe`](#pipe) | Create an anonymous pipe for inter-process or inter-task communication. |
| [`system`](#system) | Execute a system command line string in a subshell and return its raw wait status. |
| [`run`](#run) | Execute a program with specific arguments, capturing its execution state. |
| [`output`](#output) | Run a program like `run`, wait for it, and return its exit status with its stdout and stderr decoded as UTF-8. |
| [`errno`](#errno) | Return the last error code from the operating system. |
| [`strerror`](#strerror) | Return the system message for the error number `code`. |
| [`getpid`](#getpid) | Return the process ID of this process. |
| [`getppid`](#getppid) | Return the process ID of this process's parent. |
| [`getuid`](#getuid) | Return the real user ID of this process. |
| [`geteuid`](#geteuid) | Return the effective user ID of this process (the one used for permission checks; it differs from `getuid` in a set-user-ID program). |
| [`getgid`](#getgid) | Return the real group ID of this process. |
| [`expand_vars`](#expand_vars) | Return `s` with each `$NAME` and `${NAME}` replaced by the value of the environment variable NAME (letters, digits and `_`, not starting with a digit). As with Python's `os.path.expandvars`, a variable that is not set is left as it is, and so is a `$` not followed by a name. |
| [`nice`](#nice) | Add `inc` to this process's nice value and return the new value, or nil on failure. A higher value is a lower CPU priority; only a privileged process may lower it (a negative `inc`). |
| [`getloadavg`](#getloadavg) | Return the system load averages over the last 1, 5 and 15 minutes (the numbers `uptime` shows), or nil if they are not available. |
| [`getlogin`](#getlogin) | Return the name of the user logged in on this process's controlling terminal, else the user name of the real user ID (e.g. under cron or in a container); nil if neither is known. |
| [`isatty`](#isatty) | Return true if the file descriptor `fd` is a terminal, e.g. `os.isatty(1)` when stdout is not redirected to a file or a pipe. |
| [`terminal_size`](#terminal_size) | Return the `(columns, lines)` of the terminal on the file descriptor `fd` (stdout by default), or nil if `fd` is not a terminal or its size is unknown, e.g. when the output is piped. |
| [`umask`](#umask) | Set this process's file mode creation mask to `mask` (e.g. `0o022`) and return the previous one. Files and directories created afterwards do not get the permission bits set in the mask. |
| [`cpu_count`](#cpu_count) | Return the number of CPUs available to run on, or 1 if it is unknown. |
| [`platform`](#platform) | Return the operating system this runtime was built for: `"linux"`, `"darwin"`, `"freebsd"`, `"windows"` or `"unknown"`. |
| [`hostname`](#hostname) | Return the host name of this machine. Panic if it cannot be read. |
| [`uname`](#uname) | Return `(sysname, nodename, release, version, machine)` of the running kernel, e.g. `("Linux", "myhost", "6.8.0", "#1 SMP ...", "x86_64")`. |
| [`home_dir`](#home_dir) | Return the home directory of this process's user: `$HOME` when it is set and not empty, else the user database entry. Return nil if neither is known. |
| [`which`](#which) | Return the path of the program `name` as a shell would find it, like the `which` command: `name` itself if it contains a `/`, else the first `dir/name` in `PATH` that is an executable regular file (an empty `PATH` entry is the working directory). Return nil if there is none. |
| [`urandom`](#urandom) | Return `n` random bytes from the operating system, suitable for keys, tokens and seeds. Panic if `n` is negative or the source fails. |
| [`kill`](#kill) | Send the signal `sig` (e.g. `SIGTERM`) to the process `pid`; `sig` 0 only checks that the process exists and may be signalled. Return true on success, false on failure (no such process, not permitted); `os.errno()` tells why. |

## Classes

<a id="process"></a>

### `Process`

```kl
pub class Process {
    pub func stdin() RawWriter
    pub func stdout() RawReader
    pub func stderr() RawReader
    pub func pid() int
    pub func poll() int?
    pub func wait(timeout = -1.0) int?
    pub func communicate(input bytes? = nil, timeout = -1.0) (bytes, bytes)?
    pub func terminate()
    pub func kill()
}
```

Represents a spawned operating system process handle.

This class relies purely on the generic input/output byte stream contracts
defined in `std/io`, ensuring total architectural decoupling from `std/fs`.

---

<a id="process.stdin"></a>

**`stdin() RawWriter`** — *`@native`*

The standard input stream (stdin) of the child process.

Write raw bytes to this stream to feed input data into the running command.
Returns a `RawWriter` wrapping the parent-side write end of the pipe.

Example:
    let p = os.run("passwd", ["user"])!
    p.stdin().write("new_password\n".to_bytes())

<a id="process.stdout"></a>

**`stdout() RawReader`** — *`@native`*

The standard output stream (stdout) of the child process.

Read from this stream to capture the process execution text or binary output.
Returns a `RawReader` wrapping the parent-side read end of the pipe.

<a id="process.stderr"></a>

**`stderr() RawReader`** — *`@native`*

The standard error stream (stderr) of the child process.

Read from this stream to capture diagnostic or critical error outputs.
Returns a `RawReader` wrapping the parent-side read end of the pipe.

<a id="process.pid"></a>

**`pid() int`** — *`@native`*

Return the system process ID (PID) allocated to this child process.

<a id="process.poll"></a>

**`poll() int?`** — *`@native`*

Check whether the process has exited without blocking.

Returns the exit status when the process has exited, or `nil` while it
is still running. A process terminated by a signal returns the negative
signal number.

<a id="process.wait"></a>

**`wait(timeout = -1.0) int?`** — *`@native`*

Wait for the process to terminate and return its exit status code.
A process terminated by a signal returns the negative signal number.

`timeout` is in seconds; the default (any negative value) waits forever,
and then the result is never nil. With a `timeout` >= 0, return nil if
the process is still running when it ends.

This will synchronously block the current execution thread/fiber until
the child process exits or the timeout ends.

Example:
    let code = p.wait()!                  // no timeout: never nil
    if p.wait(5.0) == nil { p.kill() }    // still running after 5s

<a id="process.communicate"></a>

**`communicate(input bytes? = nil, timeout = -1.0) (bytes, bytes)?`** — *`@native`*

Send `input` to stdin and read stdout and stderr until EOF, then wait for the
process to exit.

Writing and reading run in one `poll` loop, so a large `input` cannot
deadlock against a child that blocks on writing its output (as writing all
of stdin first and then calling `stdout().read_all()` does once the data
exceeds the pipe capacity). stdin is closed after `input` is sent (at once
when `input` is nil or empty), the stdout and stderr ends are closed at
EOF, and the child is reaped, so nothing is leaked. A stream that is not
a pipe (see `os.run`) gives empty bytes.

Returns the `(stdout, stderr)` bytes. The exit status is then returned by
`wait()` or `poll()` without blocking.

`timeout` is in seconds; the default (any negative value) waits forever,
and then the result is never nil. With a `timeout` >= 0, return nil if
the child has not closed its output and exited when it ends: the child
keeps running and the pipes stay open, and the output read so far is
dropped (usually `kill()` and `wait()` it next).

Panic if `input` is non-empty but stdin is already closed, or if polling
or reading the pipes fails. If the child exits without reading all of
`input`, the rest is dropped.

Example:
    let p = os.run("sort")!
    let (out, err) = p.communicate("b\na\n".to_bytes())!
    print(out.to_str())   // "a\nb\n"
    print(p.wait()!)      // 0

<a id="process.terminate"></a>

**`terminate()`** — *`@native`*

Request termination of the child process (SIGTERM).

<a id="process.kill"></a>

**`kill()`** — *`@native`*

Forcefully terminate the child process immediately (SIGKILL).

<a id="rawreader"></a>

### `RawReader`

```kl
pub class RawReader : io.Reader & io.Closable {
    pub func __init__(fd int)
    pub func read(data bytes) int
    pub func read_all() bytes
    pub func close()
}
```
**Bases:** `io.Reader` & `io.Closable`


A RawReader is the read half of an OS-level file descriptor or handle.

It can wrap a pipe (from `os.pipe` or `os.run`), a regular file,
or a network socket. Reads block until data is available or the
writer closes the stream.

---

<a id="rawreader.__init__"></a>

**`__init__(fd int)`** — *`@native`*

Wrap the read half of the file descriptor `fd`, taking ownership of it.

Panic if `fd` is invalid or not open for reading.

<a id="rawreader.read"></a>

**`read(data bytes) int`** — *`@native`*

Read raw bytes from the underlying OS buffer into `data`, blocking until
data is available or EOF is reached.
Satisfies the `io.Reader` trait.

<a id="rawreader.read_all"></a>

**`read_all() bytes`** — *`@native`*

Read all remaining bytes from the stream until EOF, returning them
as a single `bytes` object.

Blocks until the writer side closes. Panic if the stream is already
closed or the underlying read fails.

<a id="rawreader.close"></a>

**`close()`** — *`@native`*

Close the read end of the stream, releasing the OS resource.

<a id="rawwriter"></a>

### `RawWriter`

```kl
pub class RawWriter : io.Writer & io.Closable {
    pub func __init__(fd int)
    pub func write(bs bytes) int
    pub func write_str(s str) int
    pub func flush()
    pub func close()
}
```
**Bases:** `io.Writer` & `io.Closable`


A RawWriter is the write half of an OS-level file descriptor or handle.

It can wrap a pipe, a regular file, or a network socket. It implements
the full `io.Writer` trait contract including string acceleration and
flushing control. Writes block when the underlying OS buffer has no
available capacity.

---

<a id="rawwriter.__init__"></a>

**`__init__(fd int)`** — *`@native`*

Wrap the write half of the file descriptor `fd`, taking ownership of it.

Panic if `fd` is invalid or not open for writing.

<a id="rawwriter.write"></a>

**`write(bs bytes) int`** — *`@native`*

Write raw bytes from `bs` into the underlying OS buffer.
Satisfies the `io.Writer` trait.

<a id="rawwriter.write_str"></a>

**`write_str(s str) int`** — *`@native`*

Encode `s` string as UTF-8 and write it directly to the underlying OS buffer.

The native implementation optimizes this to avoid intermediate heap allocation.

<a id="rawwriter.flush"></a>

**`flush()`** — *`@native`*

Flush pending data to the underlying sink.

For unbuffered kernel descriptors (like pipes or sockets), this is natively
implemented as a no-op, but must be provided to fulfill the `io.Writer` contract.

<a id="rawwriter.close"></a>

**`close()`** — *`@native`*

Close the write end of the stream and release its file descriptor.
Once all write ends are closed, readers observe EOF.

## Functions

<a id="getenv"></a>

### `getenv`

```kl
pub func getenv(key str) str?
```

*`@native`*

Get the value of an environment variable. Returns `nil` if the variable is not set.

<a id="setenv"></a>

### `setenv`

```kl
pub func setenv(key str, value str)
```

*`@native`*

Set the value of an environment variable.
If the variable already exists, its value is overwritten.

<a id="unsetenv"></a>

### `unsetenv`

```kl
pub func unsetenv(key str)
```

*`@native`*

Unset (remove) an environment variable. Does nothing if the variable is not set.

<a id="environ"></a>

### `environ`

```kl
pub func environ() dict[str, str]
```

*`@native`*

Return a snapshot of the current process's environment variables.

Returns a dictionary mapping variable names to their values.
Note: This is a one-time snapshot; changes to the environment after
this call will not be reflected in the returned dictionary.

Example:
    let env = os.environ()
    print(env["PATH"])
    if "HOME" in env {
        print("Home directory: ", env["HOME"])
    }

<a id="getcwd"></a>

### `getcwd`

```kl
pub func getcwd() str
```

*`@native`*

Return the absolute pathname of the current working directory.

Panic if it cannot be determined (e.g. the directory was removed).

Example:
    let cwd = os.getcwd()
    print("working in ", cwd)

<a id="chdir"></a>

### `chdir`

```kl
pub func chdir(path str) bool
```

*`@native`*

Change the current working directory to `path`.

Return true on success, false if the directory cannot be entered
(missing path or permission denied).

<a id="pipe"></a>

### `pipe`

```kl
pub func pipe() (RawReader, RawWriter)?
```

*`@native`*

Create an anonymous pipe for inter-process or inter-task communication.

Returns a tuple containing a `RawReader` (the read end) and a `RawWriter`
(the write end). Data written to the write end can be read from the read end.

If all write ends of the pipe are closed, subsequent reads from the read end
will return 0 (EOF).

Returns `nil` if the pipe cannot be created (e.g., the process has reached
the maximum number of open file descriptors).

Example:
    let (reader, writer) = os.pipe()?

    // Write data in one task/thread
    writer.write_str("hello from pipe")
    writer.close()

    // Read data in another task/thread
    let buf = bytes(1024)
    let n = reader.read(buf)
    print("read ", n, " bytes")
    reader.close()

<a id="system"></a>

### `system`

```kl
pub func system(cmd str) int
```

*`@native`*

Execute a system command line string in a subshell and return its raw wait status.

This is a synchronous, blocking operation. It passes `cmd` directly to the host OS shell
(`/bin/sh -c` on Unix). As with C `system()`, the result is the raw wait status, not
the exit code: the exit code is `(status >> 8) & 0xFF`, and `status & 0x7F` is the
signal number if the shell was killed by a signal. Return -1 if the shell cannot be
started.

The shell starts with the default SIGPIPE action, although this process ignores
SIGPIPE; the same holds for `run`.

<a id="run"></a>

### `run`

```kl
pub func run(executable str, args list[str]? = nil, cwd str? = nil, env dict[str, str]? = nil, stdin = 0, stdout = 0, stderr = 0) Process?
```

*`@native`*

Execute a program with specific arguments, capturing its execution state.

Unlike `system`, this bypasses the shell for safer argument passing.
`executable` is searched in `PATH` when it has no `/`.

`cwd` is the child's working directory (nil: this process's). `env` is the
child's whole environment (nil: a copy of this process's); to add one
variable, copy `environ()` and set it there.

`stdin`, `stdout` and `stderr` choose each standard stream of the child:
`PIPE` (the default), `INHERIT`, `DEVNULL`, or for `stderr` also `STDOUT`.
The `Process` end of a stream that is not a `PIPE` is closed: reading or
writing it panics, and `communicate` returns empty bytes for it.

Returns a `Process` handle on success, or `nil` if spawning fails (e.g.
the program or `cwd` does not exist). Panic on an invalid stream mode.

Example:
    let env = os.environ()
    env["LANG"] = "C"
    // the build prints to this terminal; nothing to read back
    let p = os.run("make", ["-j4"], cwd = "build", env = env,
                   stdin = os.DEVNULL, stdout = os.INHERIT, stderr = os.INHERIT)!
    print(p.wait()!)

See: `output`, `system`, `Process`.

<a id="output"></a>

### `output`

```kl
pub func output(executable str, args list[str]? = nil, cwd str? = nil, env dict[str, str]? = nil) (int, str, str)?
```

Run a program like `run`, wait for it, and return its exit status with
its stdout and stderr decoded as UTF-8.

The status is the one `Process.wait` returns (negative: killed by that
signal). Nothing is sent to its stdin. Return nil if it cannot be started.

Example:
    let (code, out, _) = os.output("git", ["rev-parse", "HEAD"])!
    if code == 0 { print(out.strip()) }

See: `run`, `Process.communicate`.

<a id="errno"></a>

### `errno`

```kl
pub func errno() int
```

*`@native`*

Return the last error code from the operating system.

This is typically used after a failed system call to determine the reason for the failure.

<a id="strerror"></a>

### `strerror`

```kl
pub func strerror(code int) str
```

*`@native`*

Return the system message for the error number `code`.

For example "No such file or directory" for ENOENT (2); an unknown
number gives "Unknown error N".

Example:
    if !fs.mkdir("/root/x") {
        print("mkdir failed: ", os.strerror(os.errno()))
    }

<a id="getpid"></a>

### `getpid`

```kl
pub func getpid() int
```

*`@native`*

Return the process ID of this process.

<a id="getppid"></a>

### `getppid`

```kl
pub func getppid() int
```

*`@native`*

Return the process ID of this process's parent.

<a id="getuid"></a>

### `getuid`

```kl
pub func getuid() int
```

*`@native`*

Return the real user ID of this process.

<a id="geteuid"></a>

### `geteuid`

```kl
pub func geteuid() int
```

*`@native`*

Return the effective user ID of this process (the one used for
permission checks; it differs from `getuid` in a set-user-ID program).

<a id="getgid"></a>

### `getgid`

```kl
pub func getgid() int
```

*`@native`*

Return the real group ID of this process.

<a id="expand_vars"></a>

### `expand_vars`

```kl
pub func expand_vars(s str) str
```

*`@native`*

Return `s` with each `$NAME` and `${NAME}` replaced by the value of the
environment variable NAME (letters, digits and `_`, not starting with a
digit). As with Python's `os.path.expandvars`, a variable that is not set
is left as it is, and so is a `$` not followed by a name.

Example:
    os.expand_vars("$HOME/.cache/${APP}")   // "/home/me/.cache/koala" if APP=koala

See: `getenv`, `environ`.

<a id="nice"></a>

### `nice`

```kl
pub func nice(inc int) int?
```

*`@native`*

Add `inc` to this process's nice value and return the new value, or nil
on failure. A higher value is a lower CPU priority; only a privileged
process may lower it (a negative `inc`).

<a id="getloadavg"></a>

### `getloadavg`

```kl
pub func getloadavg() (float64, float64, float64)?
```

*`@native`*

Return the system load averages over the last 1, 5 and 15 minutes (the
numbers `uptime` shows), or nil if they are not available.

<a id="getlogin"></a>

### `getlogin`

```kl
pub func getlogin() str?
```

*`@native`*

Return the name of the user logged in on this process's controlling
terminal, else the user name of the real user ID (e.g. under cron or in a
container); nil if neither is known.

See: `getuid`, `home_dir`.

<a id="isatty"></a>

### `isatty`

```kl
pub func isatty(fd int) bool
```

*`@native`*

Return true if the file descriptor `fd` is a terminal, e.g.
`os.isatty(1)` when stdout is not redirected to a file or a pipe.

See: `terminal_size`.

<a id="terminal_size"></a>

### `terminal_size`

```kl
pub func terminal_size(fd = 1) (int, int)?
```

*`@native`*

Return the `(columns, lines)` of the terminal on the file descriptor `fd`
(stdout by default), or nil if `fd` is not a terminal or its size is
unknown, e.g. when the output is piped.

Example:
    var width = 80
    if let size = os.terminal_size() {
        let (cols, _) = size
        width = cols
    }

See: `isatty`.

<a id="umask"></a>

### `umask`

```kl
pub func umask(mask int) int
```

*`@native`*

Set this process's file mode creation mask to `mask` (e.g. `0o022`) and
return the previous one. Files and directories created afterwards do not
get the permission bits set in the mask.

<a id="cpu_count"></a>

### `cpu_count`

```kl
pub func cpu_count() int
```

*`@native`*

Return the number of CPUs available to run on, or 1 if it is unknown.

Useful as the default number of parallel jobs.

<a id="platform"></a>

### `platform`

```kl
pub func platform() str
```

*`@native`*

Return the operating system this runtime was built for: `"linux"`,
`"darwin"`, `"freebsd"`, `"windows"` or `"unknown"`.

<a id="hostname"></a>

### `hostname`

```kl
pub func hostname() str
```

*`@native`*

Return the host name of this machine. Panic if it cannot be read.

<a id="uname"></a>

### `uname`

```kl
pub func uname() (str, str, str, str, str)
```

*`@native`*

Return `(sysname, nodename, release, version, machine)` of the running
kernel, e.g. `("Linux", "myhost", "6.8.0", "#1 SMP ...", "x86_64")`.

See: `platform`, `hostname`.

<a id="home_dir"></a>

### `home_dir`

```kl
pub func home_dir() str?
```

*`@native`*

Return the home directory of this process's user: `$HOME` when it is set
and not empty, else the user database entry. Return nil if neither is known.

See: `Path.expand_user` in `std/fs`.

<a id="which"></a>

### `which`

```kl
pub func which(name str) str?
```

*`@native`*

Return the path of the program `name` as a shell would find it, like the
`which` command: `name` itself if it contains a `/`, else the first
`dir/name` in `PATH` that is an executable regular file (an empty `PATH`
entry is the working directory). Return nil if there is none.

Example:
    if let git = os.which("git") { os.run(git, ["status"]) }

See: `run`.

<a id="urandom"></a>

### `urandom`

```kl
pub func urandom(n int) bytes
```

*`@native`*

Return `n` random bytes from the operating system, suitable for keys,
tokens and seeds. Panic if `n` is negative or the source fails.

<a id="kill"></a>

### `kill`

```kl
pub func kill(pid int, sig int) bool
```

*`@native`*

Send the signal `sig` (e.g. `SIGTERM`) to the process `pid`; `sig` 0 only
checks that the process exists and may be signalled. Return true on
success, false on failure (no such process, not permitted); `os.errno()`
tells why.

See: `Process.terminate`, `Process.kill`, `getpid`.
