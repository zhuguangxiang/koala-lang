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
| [`system`](#system) | Execute a system command line string in a subshell and return its exit status code. |
| [`run`](#run) | Execute a program with specific arguments, capturing its execution state. |

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
    pub func wait() int
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

**`wait() int`** — *`@native`*

Wait for the process to terminate and return its exit status code.
A process terminated by a signal returns the negative signal number.

This will synchronously block the current execution thread/fiber until
the child process exits.

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

Execute a system command line string in a subshell and return its exit status code.

This is a synchronous, blocking operation. It passes `cmd` directly to the host OS shell
(e.g., `cmd.exe` on Windows or `/bin/sh` on Unix).

<a id="run"></a>

### `run`

```kl
pub func run(executable str, args list[str]? = nil) Process?
```

*`@native`*

Execute a program with specific arguments, capturing its execution state.

Unlike `system`, this bypasses the shell for safer argument passing.
Returns a `Process` handle on success, or `nil` if spawning fails.
