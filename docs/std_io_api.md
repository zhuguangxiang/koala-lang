# Koala `std/io` API Reference

## Contents

**Traits**

| Name | Summary |
|------|---------|
| [`Closable`](#closable) | Closable represents a resource that can be explicitly closed to release underlying system resources (file descriptors, network sockets, etc.). |
| [`Reader`](#reader) | Reader is the core interface for reading raw bytes from any byte source. |
| [`Seeker`](#seeker) | An object with a movable cursor over its content. |
| [`Writer`](#writer) | Writer is the core interface for writing raw bytes or strings to any byte sink. |

**Classes**

| Name | Summary |
|------|---------|
| [`BufReader`](#bufreader) | BufReader is a buffered wrapper around a Reader that reduces the number of system calls by reading data in large chunks and serving smaller reads from an internal buffer. |
| [`BufWriter`](#bufwriter) | BufWriter is a buffered wrapper around a Writer that reduces the number of system calls by accumulating writes in an internal buffer and flushing them to the underlying writer in large chunks. |
| [`BytesIO`](#bytesio) | BytesIO is an in-memory byte stream that implements Reader, Writer, Seeker and Closable. It provides a file-like API for reading and writing binary data entirely in memory, without disk I/O. |
| [`Splitter`](#splitter) | Splitter splits a BufReader into chunks delimited by a single byte. |
| [`SplitterWith`](#splitterwith) | SplitterWith splits a BufReader into chunks delimited by a multi-byte separator sequence (e.g., "\r\n" or "::"). |
| [`Lines`](#lines) | Lines iterates over lines of text from a BufReader. |
| [`Words`](#words) | Words iterates over whitespace-separated tokens from a BufReader. |
| [`StringIO`](#stringio) | StringIO is an in-memory text stream that implements Reader, Seeker and Closable. It wraps a UTF-8 string and provides file-like reading operations without touching the filesystem. |

**Functions**

| Name | Summary |
|------|---------|
| [`write_all`](#write_all) | Write all bytes of `bs` to `dst`, retrying on partial writes until the whole buffer is written or `dst` fails. |
| [`read_all`](#read_all) | Read all remaining bytes from `src` until EOF and return them as `bytes`. |
| [`read_exact`](#read_exact) | Read exactly `bs.len()` bytes from `src` into `bs`, retrying on short reads. |
| [`copy`](#copy) | Copy bytes from `src` to `dst` until the source reaches EOF. |

**Constants**

| Name | Summary |
|------|---------|
| [`SEEK_SET`](#seek_set) | Seek whence: absolute position from the start of content. |
| [`SEEK_CUR`](#seek_cur) | Seek whence: relative to the current position. |
| [`SEEK_END`](#seek_end) | Seek whence: relative to the end of content. |

## Traits

<a id="closable"></a>

### `Closable`

```kl
pub trait Closable {
    func close()
}
```

Closable represents a resource that can be explicitly closed to release
underlying system resources (file descriptors, network sockets, etc.).

Implementing this trait signals that the resource holds external state
that should be cleaned up when no longer needed. Users are expected to
call close() when done, or use a context manager / defer pattern if
available.

Typical implementations: File, Socket, Pipe, and buffered wrappers
like BufReader / BufWriter that need to propagate close to their
underlying reader or writer.

Example:
    let f = fs.open("data.txt", "r")!
    f.close()
    // ... use f

---

<a id="closable.close"></a>

**`close()`**

Close the underlying resource and release associated system handles.

After calling close(), any subsequent read/write operations on this
resource should panic with an "I/O operation on closed file" error.

Calling close() multiple times is safe — subsequent calls are no-ops.

<a id="reader"></a>

### `Reader`

```kl
pub trait Reader {
    func read(bs bytes) int
}
```

Reader is the core interface for reading raw bytes from any byte source.

Implementations can wrap files, network sockets, stdin, or in-memory buffers.
The trait deliberately operates on raw bytes rather than strings to keep
IO encoding-agnostic — callers are responsible for decoding bytes into
strings if needed.

Implementations should follow these conventions:
- Return the number of bytes actually read (0 ≤ n ≤ bs.len()).
- Return 0 when EOF is reached (no more data available).
- Never return a negative value.
- A return value less than bs.len() does NOT necessarily mean EOF;
  it may simply indicate that fewer bytes are currently available.

Example:
    let f = fs.open("data.bin", "r")!
    let buf = bytes(1024)
    let n = f.read(buf)
    if n > 0 {
        process(buf[:n])
    }
    f.close()

---

<a id="reader.read"></a>

**`read(bs bytes) int`**

Read up to bs.len() bytes from the underlying source into the provided buffer.

# Arguments
- `bs`: The destination buffer to fill with data.

# Returns
The number of bytes actually read, or 0 if EOF has been reached.

# Notes
- The buffer `bs` is mutated in-place with the read data.
- A short read (n < bs.len()) is not an error and does not imply EOF.
- If the resource has been closed, this method should panic.

<a id="seeker"></a>

### `Seeker`

```kl
pub trait Seeker {
    func seek(offset int, whence int) int
    func tell() int
}
```

An object with a movable cursor over its content.

Implementers: `fs.File`, `io.BytesIO`, `io.StrIO`.

---

<a id="seeker.seek"></a>

**`seek(offset int, whence int) int`**

Move the cursor by `offset` relative to `whence` and return
the new absolute position, or -1 on error.

`whence`: 0 = start of content, 1 = current position,
2 = end of content (POSIX values).

See: `SEEK_SET`, `SEEK_CUR`, `SEEK_END`.

<a id="seeker.tell"></a>

**`tell() int`**

Return the current cursor position, or -1 on error.

Equivalent to `seek(0, 1)`, provided for readability.

See: `seek`.

<a id="writer"></a>

### `Writer`

```kl
pub trait Writer {
    func write(bs bytes) int
    func write_str(s str) int
    func flush()
}
```

Writer is the core interface for writing raw bytes or strings to any byte sink.

Implementations can wrap files, network sockets, stdout, stderr, or
in-memory buffers. It provides both byte-level and string-level write
operations to accommodate different caller needs while remaining
encoding-agnostic at the trait level.

Implementations should follow these conventions:
- Return the number of bytes actually written.
- A short write (n < bs.len()) indicates a partial write, which may
  signal an error condition (e.g., disk full, broken pipe).
- Never return a negative value.
- The flush() method should be called to ensure buffered data is
  pushed to the underlying sink.

Example:
    let f = fs.open("output.bin", "w")!
    let n = f.write(b"hello")
    let m = f.write_str("world")
    if n + m == 10 {
        f.flush()
    }
    f.close()

---

<a id="writer.write"></a>

**`write(bs bytes) int`**

Write the contents of the provided byte slice to the underlying sink.

Arguments
- `bs`: The source bytes to write.

Returns
The number of bytes actually written. A value less than bs.len()
indicates a partial write, which typically signals an error condition.

# Notes
- The buffer `bs` is not mutated.
- For buffered writers, data may be held in an internal buffer until
  flush() is called or the buffer is full.
- If the resource has been closed, this method should panic.

<a id="writer.write_str"></a>

**`write_str(s str) int`**

Write the given string to the underlying sink.

This is a convenience method for callers who have string data and
want to avoid manually encoding to bytes. The implementation is
responsible for converting the string to its UTF-8 byte representation
before writing.

Arguments
- `s`: The source string to write.

Returns
The number of bytes actually written (not the number of characters).
A value less than the UTF-8 byte length of `s` indicates a partial write.

Notes
- The string `s` is not mutated.
- This method is semantically equivalent to `write(s.to_bytes())` but
  may be more efficient in implementations that can avoid an intermediate
  allocation.
- If the resource has been closed, this method should panic.

<a id="writer.flush"></a>

**`flush()`**

Flush any internally buffered data to the underlying sink.

For unbuffered writers (e.g., raw File), this is typically a no-op.
For buffered writers (e.g., BufWriter), this forces all pending data
to be written out immediately.

Notes
- Should be called after a series of write() / write_str() calls when
  data must be guaranteed to reach the underlying sink (e.g., before
  close(), or when writing log entries that must not be lost).
- If the resource has been closed, this method should panic.

## Classes

<a id="bufreader"></a>

### `BufReader`

```kl
pub class BufReader : Reader {
    let inner Reader
    let buf bytes
    var start int
    var end int
    pub func __init__(_inner Reader, size = 8192)
    pub func read(out bytes) int
    pub func avail() int
    pub func peek(n int) bytes
    pub func discard(n int) int
    pub func fill() int
    pub func read_until(out Buffer, delim uint8) int
    pub func read_line(_buf Buffer) int
    pub func read_byte() int
    pub func split(delim uint8, size = 256) Splitter
    pub func split_with(sep bytes, size = 256) SplitterWith
    pub func lines(size = 256) Lines
    pub func words(size = 256) Words
}
```
**Bases:** [`Reader`](#reader)


BufReader is a buffered wrapper around a Reader that reduces the number
of system calls by reading data in large chunks and serving smaller
reads from an internal buffer.

It is especially useful when the underlying reader performs expensive
operations per call (e.g., file I/O, network sockets). Instead of
calling read() for every small request, BufReader fetches data in
configurable block sizes (default 8192 bytes) and serves subsequent
reads from memory.

BufReader also provides peek() and discard() methods for lookahead
operations, and read_until() for delimited reading (used by Splitter,
Lines, and Words iterators). The convenience factories split(),
split_with(), lines(), and words() create those iterators directly on
this reader.

Example:
    let f = fs.open("data.txt", "r")!
    let br = io.BufReader(f)
    for line in br.lines() {
        print(line)
    }
    f.close()

---

**Fields**

<a id="bufreader.inner"></a>

**`let inner Reader`**

The underlying reader that provides the raw byte stream.

<a id="bufreader.buf"></a>

**`let buf bytes`**

The internal buffer that holds prefetched data from the underlying reader.

<a id="bufreader.start"></a>

**`var start int`**

The start index of unconsumed data in the buffer.
Data in buf[start..end] is valid and has not yet been returned to the caller.

<a id="bufreader.end"></a>

**`var end int`**

The end index of unconsumed data in the buffer.
Data in buf[start..end] is valid and has not yet been returned to the caller.

<a id="bufreader.__init__"></a>

**`__init__(_inner Reader, size = 8192)`**

Create a new BufReader wrapping the given reader with the specified buffer size.

Arguments
- `_inner`: The underlying reader to wrap.
- `size`: The internal buffer size in bytes. Defaults to 8192.

The buffer is allocated immediately. Larger sizes reduce system call frequency
but increase memory usage.

<a id="bufreader.read"></a>

**`read(out bytes) int`**

Read up to `out.len()` bytes from the buffered stream into the provided buffer.

Data is served from the internal buffer first. If the buffer is empty,
it is refilled from the underlying reader before serving the read.

Arguments
- `out`: The destination buffer to fill with data.

Returns
The number of bytes actually read, or 0 if EOF has been reached.

<a id="bufreader.avail"></a>

**`avail() int`**

Return the number of bytes currently available in the internal buffer
without triggering a refill from the underlying reader.

This is useful for checking whether data is ready to be consumed
without blocking or performing I/O.

Returns
The number of unconsumed bytes in the buffer (end - start).

<a id="bufreader.peek"></a>

**`peek(n int) bytes`**

Peek at the next n bytes without consuming them.

Panics if n exceeds the buffer capacity, or if the stream holds fewer
than n bytes after one fill attempt. The returned view stays valid
only until the next read, discard, or peek.

<a id="bufreader.discard"></a>

**`discard(n int) int`**

Skip (discard) the next n bytes in the stream without reading them
into a caller-provided buffer.

This is more efficient than reading into a temporary buffer when
the data is not needed, as it avoids copying.

Arguments
- `n`: The number of bytes to skip.

Returns
The number of bytes actually skipped. May be less than n if EOF
is reached before skipping the requested amount.

<a id="bufreader.fill"></a>

**`fill() int`**

Compact unconsumed bytes to the front, then read once into the free
tail space. Returns the available byte count after filling; 0 means
no data (EOF).

Public so lookahead callers (e.g. io.Words) can probe for EOF without
the strict requirement of peek().

<a id="bufreader.read_until"></a>

**`read_until(out Buffer, delim uint8) int`**

Read bytes from the stream into the provided buffer until a delimiter
byte is encountered or EOF is reached.

The delimiter byte is consumed from the stream but NOT included in
the output buffer. This is the core method used by Splitter, Lines,
and Words iterators.

Arguments
- `out`: The destination buffer to append read bytes to.
- `delim`: The delimiter byte to stop at (not included in output).

Returns
The number of bytes appended to the output buffer (excluding the
delimiter) when the delimiter is found; this may be 0 for leading or
consecutive delimiters. Returns -1 if EOF is reached before the
delimiter is found; any trailing bytes read before EOF are still
appended to `out`.

<a id="bufreader.read_line"></a>

**`read_line(_buf Buffer) int`**

Read one line up to '\n', appending it to `buf`.

The '\n' is consumed but not appended, and a trailing '\r' is
NOT stripped (use Lines for that). Returns the number of bytes appended
(0 for an empty line), or -1 at EOF; bytes read before EOF are still appended to `buf`.

<a id="bufreader.read_byte"></a>

**`read_byte() int`**

Read a single byte and return its value (0-255), or -1 at EOF.

Exactly one byte is consumed, so NUL bytes are returned as 0 and no
data is ever read ahead or lost.

<a id="bufreader.split"></a>

**`split(delim uint8, size = 256) Splitter`**

Return a Splitter that yields chunks delimited by the given byte.

Convenience factory equivalent to Splitter(self, delim). The iterator
shares this reader, so interleaving direct reads with iteration
consumes the same underlying stream.

Arguments
- `delim`: The single byte used as the delimiter (e.g., b',' or b'\n').
  The delimiter is consumed but not included in the returned chunks.
- `size` optionally sets the iterator's initial internal buffer capacity (default 256).
  It still grows automatically when needed.

Returns
A Splitter over the remaining bytes of this reader.

Example:
    for chunk in br.split(',') { print(chunk.to_str()) }

<a id="bufreader.split_with"></a>

**`split_with(sep bytes, size = 256) SplitterWith`**

Return a SplitterWith that yields chunks delimited by a multi-byte
separator sequence.

Semantics mirror split(): the separator is consumed but never
included, leading and consecutive separators produce empty chunks, a
trailing separator adds no final empty chunk, and a non-empty
remainder is returned at EOF. The separator may straddle reader
refills and may be longer than the reader buffer capacity. An empty
separator panics at construction time. For single-byte delimiters,
prefer split(), which is faster.

`size` optionally sets the iterator's initial internal buffer capacity (default 256).
It still grows automatically when needed.

Example:
    for chunk in br.split_with("::".to_bytes()) { print(chunk.to_str()) }

<a id="bufreader.lines"></a>

**`lines(size = 256) Lines`**

Return a Lines iterator over the remaining lines of the stream.

Convenience factory equivalent to Lines(self). Lines end at '\n'; a
trailing '\r' of a CRLF line ending is stripped. The iterator shares
this reader with any other consumer.

Returns
A Lines iterator over the remaining bytes of this reader.

`size` optionally sets the iterator's initial internal buffer capacity (default 256).
It still grows automatically when needed.

Example:
    for line in br.lines() { print(line) }

<a id="bufreader.words"></a>

**`words(size = 256) Words`**

Return a Words iterator over whitespace-separated tokens.

Convenience factory equivalent to Words(self). Tokens are separated
by spaces, tabs, newlines, and carriage returns; one trailing
delimiter is consumed together with each token. The iterator shares
this reader with any other consumer.

Returns
A Words iterator over the remaining bytes of this reader.

`size` optionally sets the iterator's initial internal buffer capacity (default 256).
It still grows automatically when needed.

Example:
    for word in br.words() { print(word) }

<a id="bufwriter"></a>

### `BufWriter`

```kl
pub class BufWriter : Writer {
    let inner Writer
    let buf bytes
    var pos int
    pub func __init__(_inner Writer, buf_size = 8192)
    func _flush()
    pub func write(data bytes) int
    pub func write_str(s str) int
    pub func flush()
}
```
**Bases:** [`Writer`](#writer)


BufWriter is a buffered wrapper around a Writer that reduces the number
of system calls by accumulating writes in an internal buffer and flushing
them to the underlying writer in large chunks.

It is especially useful when the underlying writer performs expensive
operations per call (e.g., file I/O, network sockets). Instead of
writing every small piece of data immediately, BufWriter batches writes
and sends them in configurable block sizes (default 8192 bytes).

BufWriter automatically flushes its buffer when:
- The buffer is full.
- A write larger than the buffer size is performed (bypasses buffer).
- flush() is called explicitly.

Example:
    let f = fs.open("output.txt", "w")!
    let bw = io.BufWriter(f)
    bw.write_str("Hello, World!")
    bw.flush()
    f.close()

---

**Fields**

<a id="bufwriter.inner"></a>

**`let inner Writer`**

The underlying writer that receives the flushed data.

<a id="bufwriter.buf"></a>

**`let buf bytes`**

The internal buffer that accumulates data before flushing.

<a id="bufwriter.pos"></a>

**`var pos int`**

The current write position in the internal buffer.
Data in buf[0..pos] is pending and has not yet been flushed.

<a id="bufwriter.__init__"></a>

**`__init__(_inner Writer, buf_size = 8192)`**

Create a new BufWriter wrapping the given writer with the specified buffer size.

Arguments
- `_inner`: The underlying writer to wrap.
- `buf_size`: The internal buffer size in bytes. Defaults to 8192.

The buffer is allocated immediately. Larger sizes reduce system call
frequency but increase memory usage.

<a id="bufwriter._flush"></a>

**`_flush()`**

Flush the internal buffer to the underlying writer.

This is an internal method called automatically when the buffer is
full or when a large write bypasses the buffer. Users should call
the public flush() method instead, which also flushes the underlying writer.

<a id="bufwriter.write"></a>

**`write(data bytes) int`**

Write the provided byte slice to the buffered stream.

Data is accumulated in the internal buffer. If the data exceeds the
buffer capacity, the buffer is flushed first, and then:
- If the data is larger than the entire buffer, it is written directly
  to the underlying writer (bypassing the buffer).
- Otherwise, the data is copied into the now-empty buffer.

Arguments
- `data`: The source bytes to write.

Returns
The number of bytes actually written (buffered or flushed).
A value less than data.len() indicates a partial write.

Notes
- This method satisfies the Writer trait contract.
- Data may not reach the underlying writer until flush() is called
  or the buffer fills up.

<a id="bufwriter.write_str"></a>

**`write_str(s str) int`**

Write the UTF-8 encoding of the string to the buffered stream.

This is a convenience method equivalent to write(s.to_bytes()).
Data is accumulated in the internal buffer. If the UTF-8 encoded data
exceeds the buffer capacity, the buffer is flushed first, and then:
- If the encoded data is larger than the entire buffer, it is written
  directly to the underlying writer (bypassing the buffer).
- Otherwise, the data is copied into the now-empty buffer.

Arguments
- `s`: The source string to encode and write.

Returns
The number of bytes actually written (buffered or flushed).
A value less than the UTF-8 byte length of `s` indicates a partial write.

<a id="bufwriter.flush"></a>

**`flush()`**

Flush all buffered data to the underlying writer and then flush
the underlying writer itself.

This ensures that all data written so far has been pushed through
to the final destination (e.g., disk, network).

Notes
- Should be called when data must be guaranteed to reach the
  underlying sink (e.g., before close(), or when writing log entries).
- After calling flush(), the internal buffer is empty and ready
  for new writes.

<a id="bytesio"></a>

### `BytesIO`

```kl
pub class BytesIO : Reader & Writer & Seeker & Closable {
    let bs bytes
    var end int = 0
    var pos int = 0
    var closed bool = false
    pub func __init__(size = 8192)
    pub static func from_bytes(data bytes) BytesIO
    pub static func from_str(s str) BytesIO
    pub func read(_bs bytes) int
    pub func write(_bs bytes) int
    pub func write_str(s str) int
    pub func flush()
    pub func seek(offset int, whence int) int
    pub func tell() int
    pub func close()
    pub func is_closed() bool
    pub func size() int
    pub func capacity() int
    pub func getvalue() bytes
}
```
**Bases:** [`Reader`](#reader) & [`Writer`](#writer) & [`Seeker`](#seeker) & [`Closable`](#closable)


BytesIO is an in-memory byte stream that implements Reader, Writer,
Seeker and Closable. It provides a file-like API for reading and
writing binary data entirely in memory, without disk I/O.

Unlike Python's BytesIO (which uses a dynamically resizing buffer),
Koala's BytesIO wraps a fixed-capacity bytes buffer. The capacity is
determined at construction time and never changes. The stream keeps
track of the logical end of the data written so far, which never
exceeds the capacity:
- Reading serves bytes from the current position up to the logical end,
  and returns 0 once the position reaches the end (EOF).
- Writing at or beyond the logical end appends data and moves the end
  forward; writing before the end overwrites existing bytes without
  changing the end.

BytesIO is useful for:
- Building binary data incrementally before sending over a network.
- Testing code that expects a Reader / Writer without touching the filesystem.
- Temporary storage of binary data (e.g., image processing, serialization).

Example:
    let bio = io.BytesIO(64)
    bio.write_str("Hello, ")
    bio.write_str("BytesIO!")
    bio.seek(0, 0)
    let buf = bytes(64)
    let n = bio.read(buf)          // n == 15
    print(buf.view(0, n).to_str()) // "Hello, BytesIO!"

---

**Fields**

<a id="bytesio.bs"></a>

**`let bs bytes`**

The underlying fixed-capacity byte buffer.

<a id="bytesio.end"></a>

**`var end int = 0`**

The logical end of the data written so far, 0 <= end <= bs.len().

<a id="bytesio.pos"></a>

**`var pos int = 0`**

The current read/write position within the buffer, 0 <= pos <= end.

<a id="bytesio.closed"></a>

**`var closed bool = false`**

Whether close() has been called.

<a id="bytesio.__init__"></a>

**`__init__(size = 8192)`**

Create an empty BytesIO with the given buffer capacity.

Arguments
- `size`: The fixed buffer capacity in bytes. Defaults to 8192.

The buffer is allocated and zero-filled. The logical end and the
read/write position both start at 0.

Panic if `size` is negative.

<a id="bytesio.from_bytes"></a>

**`static from_bytes(data bytes) BytesIO`**

Create a BytesIO initialized with a copy of the given byte data.

The capacity equals data.len(), the logical end is data.len(), and
the position is 0, so the content can be read back immediately.

Arguments
- `data`: The initial byte data. It is not mutated.

Returns
A new BytesIO containing a copy of `data`.

<a id="bytesio.from_str"></a>

**`static from_str(s str) BytesIO`**

Create a BytesIO initialized with the UTF-8 encoding of the given string.

This is a convenience method equivalent to BytesIO.from_bytes(s.to_bytes()).

Arguments
- `s`: The initial string data.

Returns
A new BytesIO containing the UTF-8 bytes of `s`, positioned at 0.

<a id="bytesio.read"></a>

**`read(_bs bytes) int`**

Read up to _bs.len() bytes from the current position into `_bs`.

Arguments
- `_bs`: The destination buffer to fill with data.

Returns
The number of bytes actually read, or 0 if the position has
reached the logical end (EOF).

Notes
- A short read (n < _bs.len()) means EOF was reached, since all
  the data is already in memory.
- Panic if the stream has been closed.

<a id="bytesio.write"></a>

**`write(_bs bytes) int`**

Write `_bs` at the current position.

Writing at or beyond the logical end appends data and moves the
end forward; writing before the end overwrites existing bytes.
After the call the position is just past the written data.

Arguments
- `_bs`: The source bytes to write. It is not mutated.

Returns
The number of bytes actually written, always _bs.len().

Notes
- Panic if the stream has been closed.
- Panic if the write would exceed the fixed capacity.

<a id="bytesio.write_str"></a>

**`write_str(s str) int`**

Write the UTF-8 encoding of the given string at the current position.

This is a convenience method equivalent to write(s.to_bytes()).

Arguments
- `s`: The string to write.

Returns
The number of bytes actually written (not the number of characters).

Notes
- Panic if the stream has been closed.
- Panic if the write would exceed the fixed capacity.

<a id="bytesio.flush"></a>

**`flush()`**

Flush buffered data to the underlying sink.

BytesIO keeps everything in memory, so this is a no-op. It exists
to satisfy the Writer trait and to panic on a closed stream like
the other I/O operations.

Panic if the stream has been closed.

<a id="bytesio.seek"></a>

**`seek(offset int, whence int) int`**

Move the read/write position to `offset` relative to `whence`.

The resulting position is clamped into [0, end]. Seeking past the
logical end does not create gaps: the next write appends at the
end and extends it.

Arguments
- `offset`: The byte offset relative to `whence`. May be negative.
- `whence`: The reference point for the offset:
  - 0: Start of the data (SEEK_SET).
  - 1: Current position (SEEK_CUR).
  - 2: Logical end of the data (SEEK_END).

Returns
The new absolute position, or -1 if `whence` is invalid or the
stream has been closed.

<a id="bytesio.tell"></a>

**`tell() int`**

Return the current read/write position.

Returns
The current byte offset from the start of the buffer,
or -1 if the stream has been closed.

<a id="bytesio.close"></a>

**`close()`**

Close this BytesIO.

Since BytesIO operates entirely in memory, no system resources are
released; the stream is only marked as closed. Subsequent read,
write, write_str and flush operations panic; seek returns -1 and
tell returns -1. Calling close() multiple times is safe.

<a id="bytesio.is_closed"></a>

**`is_closed() bool`**

Return true if close() has been called.

<a id="bytesio.size"></a>

**`size() int`**

Return the logical end of the data written so far.

Returns
The number of valid bytes from the start of the buffer,
or -1 if the stream has been closed.

<a id="bytesio.capacity"></a>

**`capacity() int`**

Return the fixed capacity of the underlying buffer.

<a id="bytesio.getvalue"></a>

**`getvalue() bytes`**

Return a copy of the bytes written so far, from 0 to the logical end.

The position is not changed. The result is an independent copy:
later writes to the stream do not affect it.

Panic if the stream has been closed.

<a id="splitter"></a>

### `Splitter`

```kl
pub class Splitter : Iterator[bytes] {
    let br BufReader
    let buf Buffer
    let delim uint8
    var ready bool = false
    var done bool = false
    pub func __init__(_br BufReader, _delim uint8, size = 256)
    pub func has_next() bool
    pub func next_strict() bytes
    pub func next() bytes?
}
```
**Bases:** `Iterator[bytes]`


Splitter splits a BufReader into chunks delimited by a single byte.

It reads from the underlying reader on each iteration, accumulating bytes
into an internal buffer until the delimiter is encountered or EOF is reached.
The delimiter itself is not included in the returned chunk.

Leading and consecutive delimiters produce empty chunks. The empty
remainder after a trailing delimiter is dropped at EOF, and a non-empty
trailing chunk without a final delimiter is still returned. has_next()
caches its result, so it may be called repeatedly without consuming chunks.

Typical use case: splitting a stream by a custom separator (e.g., CSV fields
separated by ',' or log entries separated by '\x00').

Example:
    let br = io.BufReader(io.StdinReader())
    for chunk in io.Splitter(br, ',') {
        print(chunk)
    }

---

**Fields**

<a id="splitter.br"></a>

**`let br BufReader`**

The underlying buffered reader that provides the byte stream.

<a id="splitter.buf"></a>

**`let buf Buffer`**

Internal buffer that accumulates bytes until the delimiter is found.
Reused across iterations to avoid repeated allocations.

<a id="splitter.delim"></a>

**`let delim uint8`**

The delimiter byte used to split the stream.
This byte is consumed from the reader but not included in returned chunks.

<a id="splitter.ready"></a>

**`var ready bool = false`**

Whether the buffer holds a chunk that has not yet been returned.

<a id="splitter.done"></a>

**`var done bool = false`**

Whether the underlying reader has reached EOF.

<a id="splitter.__init__"></a>

**`__init__(_br BufReader, _delim uint8, size = 256)`**

Create a new Splitter that splits the given reader by the specified delimiter.

Arguments
- `_br`: The buffered reader to read from.
- `_delim`: The single byte used as the delimiter (e.g., b',' or b'\n').
- `size`: The initial capacity of the internal buffer in bytes
  (default 256). The buffer still grows automatically if a chunk
  exceeds this size. Must be positive.

<a id="splitter.has_next"></a>

**`has_next() bool`**

Return true if another chunk is available, buffering it if necessary.

Repeated calls do not advance past the pending chunk. A chunk is empty
when the delimiter appears at the start of the remaining input or when
two delimiters are consecutive. Iteration stops at EOF; a non-empty
trailing chunk without a final delimiter is still returned.

<a id="splitter.next_strict"></a>

**`next_strict() bytes`**

Return the next chunk and advance the iterator. Panic if exhausted.

May be called without a preceding has_next(). The returned bytes do
not include the delimiter and are independent of the internal buffer,
so they remain valid after advancing.

<a id="splitter.next"></a>

**`next() bytes?`**

Return the next chunk, or nil if EOF has been reached.

Convenience method that combines has_next() and next_strict().

<a id="splitterwith"></a>

### `SplitterWith`

```kl
pub class SplitterWith : Iterator[bytes] {
    let br BufReader
    let buf Buffer
    let pending_buf Buffer
    let sep bytes
    let tmp bytes
    var ready bool = false
    var done bool = false
    pub func __init__(_br BufReader, _sep bytes, size = 256)
    pub func has_next() bool
    pub func next_strict() bytes
    pub func next() bytes?
}
```
**Bases:** `Iterator[bytes]`


SplitterWith splits a BufReader into chunks delimited by a multi-byte
separator sequence (e.g., "\r\n" or "::").

Semantics mirror Splitter: the separator is consumed but never included,
leading and consecutive separators produce empty chunks, the empty
remainder after a trailing separator is dropped at EOF, and a non-empty
trailing chunk without a final separator is still returned. Separators
may straddle reader refills and may be longer than the reader buffer
capacity; an incomplete separator prefix at EOF is plain data. An empty
separator panics at construction time. For single-byte delimiters prefer
Splitter, which avoids the extra scanning cost. has_next() caches its
result, so it may be called repeatedly without consuming chunks.

Because chunks accumulate in the iterator's own buffer, it may read ahead
of the last returned chunk; interleave direct reads on the underlying
reader with care.

Example:
    for chunk in io.SplitterWith(br, "::".to_bytes()) { print(chunk) }

---

**Fields**

<a id="splitterwith.br"></a>

**`let br BufReader`**

The underlying buffered reader that provides the byte stream.

<a id="splitterwith.buf"></a>

**`let buf Buffer`**

Accumulates bytes until the separator is found; reused across calls.

<a id="splitterwith.pending_buf"></a>

**`let pending_buf Buffer`**

Holds the chunk waiting to be returned, so `buf` can already keep
the remainder that follows the separator.

<a id="splitterwith.sep"></a>

**`let sep bytes`**

The separator sequence; consumed but not included in chunks.

<a id="splitterwith.tmp"></a>

**`let tmp bytes`**

Scratch buffer reused for each read from the underlying reader.

<a id="splitterwith.ready"></a>

**`var ready bool = false`**

Whether a chunk is waiting to be returned.

<a id="splitterwith.done"></a>

**`var done bool = false`**

Whether the underlying reader has reached EOF.

<a id="splitterwith.__init__"></a>

**`__init__(_br BufReader, _sep bytes, size = 256)`**

Create a SplitterWith splitting `_br` on the separator `_sep`.

`_sep` must not be empty (panics otherwise). `size` sets the initial
capacity of the internal buffers and the per-read scratch size
(default 256); buffers still grow automatically for larger chunks.
Must be positive.

<a id="splitterwith.has_next"></a>

**`has_next() bool`**

Return true if another chunk is available, buffering it if necessary.

Repeated calls do not advance past the pending chunk. A chunk is
empty when the separator appears at the start of the remaining input
or when two separators are consecutive. At EOF a non-empty trailing
chunk is still returned; an incomplete separator prefix is data.

<a id="splitterwith.next_strict"></a>

**`next_strict() bytes`**

Return the next chunk and advance the iterator. Panic if exhausted.

The returned bytes exclude the separator and are independent of the
internal buffers, so they remain valid after advancing.

<a id="splitterwith.next"></a>

**`next() bytes?`**

Return the next chunk, or nil if EOF has been reached.

<a id="lines"></a>

### `Lines`

```kl
pub class Lines : Iterator[str] {
    let br BufReader
    let buf Buffer
    var ready bool = false
    var done bool = false
    var cr bool = false
    pub func __init__(_br BufReader, size = 256)
    pub func has_next() bool
    pub func next_strict() str
    pub func next() str?
}
```
**Bases:** `Iterator[str]`


Lines iterates over lines of text from a BufReader.

Each iteration reads until a newline character ('\n') is encountered.
The newline character and any trailing carriage return ('\r') are stripped
from the returned string, so each yielded line is clean text without line endings.

This is the most common way to read a text file or stream line by line
without loading the entire content into memory.

Example:
    let f = fs.open("data.txt")!
    let br = io.BufReader(f)
    for line in io.Lines(br) {
        print(line)
    }
    f.close()

---

**Fields**

<a id="lines.br"></a>

**`let br BufReader`**

The underlying buffered reader that provides the byte stream.

<a id="lines.buf"></a>

**`let buf Buffer`**

Internal buffer that accumulates bytes until a newline is found.
Reused across iterations to avoid repeated allocations.

<a id="lines.ready"></a>

**`var ready bool = false`**

Whether the buffer holds a line that has not yet been returned.

<a id="lines.done"></a>

**`var done bool = false`**

Whether the underlying reader has reached EOF.

<a id="lines.cr"></a>

**`var cr bool = false`**

Whether the pending line was terminated by '\n' (so a trailing
'\r' of a CRLF ending must be stripped).

<a id="lines.__init__"></a>

**`__init__(_br BufReader, size = 256)`**

Create a new Lines iterator that reads lines from the given buffered reader.

Arguments
- `_br`: The buffered reader to read from.

The internal buffer is initialized with a capacity of 256 bytes and will
grow automatically if a line exceeds this size.

Note: Lines handles both Unix ('\n') and Windows ('\r\n') line endings.
The returned strings have both '\n' and trailing '\r' stripped.

<a id="lines.has_next"></a>

**`has_next() bool`**

Return true if another line is available, buffering it if necessary.

Repeated calls do not advance past the pending line. An empty line
(a bare '\n' or consecutive newlines) is returned as an empty string.
Iteration stops at EOF; a non-empty final line without a trailing
newline is still returned.

<a id="lines.next_strict"></a>

**`next_strict() str`**

Return the next line and advance the iterator. Panic if exhausted.

May be called without a preceding has_next(). The returned string has
the '\n' line ending and any trailing '\r' of a CRLF ending removed,
and is independent of the internal buffer.

<a id="lines.next"></a>

**`next() str?`**

Return the next line, or nil if EOF has been reached.

Convenience method that combines has_next() and next_strict().

<a id="words"></a>

### `Words`

```kl
pub class Words : Iterator[str] {
    let br BufReader
    let buf Buffer
    var ready bool = false
    var done bool = false
    pub func __init__(_br BufReader, size = 256)
    pub func has_next() bool
    pub func next_strict() str
    pub func next() str?
}
```
**Bases:** `Iterator[str]`


Words iterates over whitespace-separated tokens from a BufReader.

It splits on ASCII space, tab, newline, and carriage return, skipping
leading and repeated separators. Other bytes, including UTF-8 text, are
preserved. Empty input or trailing whitespace produces no extra iterations.
Tokens may span any number of buffered reads.

This is useful for tokenizing input where fields are separated by
arbitrary amounts of whitespace, such as parsing command output or
simple data formats.

Example:
    let br = io.BufReader(io.StdinReader())
    for word in io.Words(br) {
        print(word)
    }

---

**Fields**

<a id="words.br"></a>

**`let br BufReader`**

The underlying buffered reader that provides the byte stream.

<a id="words.buf"></a>

**`let buf Buffer`**

Internal buffer that accumulates non-whitespace bytes for the current token.
Reused across iterations to avoid repeated allocations.

<a id="words.ready"></a>

**`var ready bool = false`**

Whether the buffer holds a token that has not yet been returned.

<a id="words.done"></a>

**`var done bool = false`**

Whether the underlying reader has reached EOF.

<a id="words.__init__"></a>

**`__init__(_br BufReader, size = 256)`**

Create a new Words iterator that tokenizes the given buffered reader
by whitespace (spaces, tabs, newlines, carriage returns).

Arguments
- `_br`: The buffered reader to read from.
- `size`: The initial capacity of the internal buffer in bytes
  (default 256). The buffer still grows automatically if a token
  exceeds this size. Must be positive.

<a id="words.has_next"></a>

**`has_next() bool`**

Return true if another word is available, buffering it if necessary.

Repeated calls do not advance past the pending word. Leading whitespace
is skipped, and word fragments are copied before the reader is refilled.

<a id="words.next_strict"></a>

**`next_strict() str`**

Return the next word and advance the iterator. Panic if exhausted.

May be called without a preceding has_next(). The returned string is
independent of the internal buffer and remains valid after advancing.

<a id="words.next"></a>

**`next() str?`**

Return the next token, or nil if EOF has been reached.

Convenience method that combines has_next() and next_strict().

<a id="stringio"></a>

### `StringIO`

```kl
pub class StringIO : Reader & Seeker & Closable {
    let source str
    let bs bytes
    var pos int = 0
    var closed bool = false
    pub func __init__(s str)
    pub func read(_bs bytes) int
    pub func seek(offset int, whence int) int
    pub func tell() int
    pub func close()
    pub func is_closed() bool
    pub func eof() bool
    pub func size() int
    pub func getvalue() str
}
```
**Bases:** [`Reader`](#reader) & [`Seeker`](#seeker) & [`Closable`](#closable)


StringIO is an in-memory text stream that implements Reader, Seeker
and Closable. It wraps a UTF-8 string and provides file-like reading
operations without touching the filesystem.

StringIO is read-only: it implements no Writer methods, and the
underlying string never changes. Reading operates at the byte level
as required by the Reader trait — positions and counts are UTF-8
byte offsets, not character counts.

StringIO is useful for:
- Testing code that expects a Reader without touching the filesystem.
- Feeding in-memory text into byte-oriented consumers such as
  BufReader, Lines, Words and Splitter.

Example:
    let sio = io.StringIO("Hello, StringIO!")
    let buf = bytes(64)
    let n = sio.read(buf)          // n == 16
    print(buf.view(0, n).to_str()) // "Hello, StringIO!"

---

**Fields**

<a id="stringio.source"></a>

**`let source str`**

The underlying string. It is never mutated.

<a id="stringio.bs"></a>

**`let bs bytes`**

The UTF-8 bytes of `source`, computed once at construction.
All reading is served from this byte view.

<a id="stringio.pos"></a>

**`var pos int = 0`**

The current read position in bytes, 0 <= pos <= bs.len().

<a id="stringio.closed"></a>

**`var closed bool = false`**

Whether close() has been called.

<a id="stringio.__init__"></a>

**`__init__(s str)`**

Create a StringIO reading from the given string.

Arguments
- `s`: The source string. It is not mutated.

The read position starts at 0.

<a id="stringio.read"></a>

**`read(_bs bytes) int`**

Read up to _bs.len() bytes from the current position into `_bs`.

Arguments
- `_bs`: The destination buffer to fill with data.

Returns
The number of bytes actually read, or 0 if the position has
reached the end of the string (EOF).

Notes
- A short read (n < _bs.len()) means EOF was reached, since all
  the data is already in memory.
- Reading is byte-oriented: a read may end in the middle of a
  multi-byte UTF-8 character.
- Panic if the stream has been closed.

<a id="stringio.seek"></a>

**`seek(offset int, whence int) int`**

Move the read position to `offset` relative to `whence`.

The resulting position is clamped into [0, bs.len()], so it can
never point outside the source string.

Arguments
- `offset`: The byte offset relative to `whence`. May be negative.
- `whence`: The reference point for the offset:
  - 0: Start of the string (SEEK_SET).
  - 1: Current position (SEEK_CUR).
  - 2: End of the string (SEEK_END).

Returns
The new absolute position, or -1 if `whence` is invalid or the
stream has been closed.

<a id="stringio.tell"></a>

**`tell() int`**

Return the current read position.

Returns
The current byte offset from the start of the string,
or -1 if the stream has been closed.

<a id="stringio.close"></a>

**`close()`**

Close this StringIO.

Since StringIO operates entirely in memory, no system resources are
released; the stream is only marked as closed. Subsequent read and
getvalue operations panic; seek and tell return -1. Calling close()
multiple times is safe.

<a id="stringio.is_closed"></a>

**`is_closed() bool`**

Return true if close() has been called.

<a id="stringio.eof"></a>

**`eof() bool`**

Return true if the read position has reached the end of the string.

Unlike Java's ready() which checks if the next read will block,
StringIO is always "ready" since it's in-memory. eof() tells you
whether there is any data left to read.

<a id="stringio.size"></a>

**`size() int`**

Return the length of the source string in UTF-8 bytes.

Returns
The total number of readable bytes,
or -1 if the stream has been closed.

<a id="stringio.getvalue"></a>

**`getvalue() str`**

Return the whole source string.

The read position is not changed. The string is immutable, so the
result always reflects the full content regardless of the position.

Panic if the stream has been closed.

## Functions

<a id="write_all"></a>

### `write_all`

```kl
pub func write_all(dst Writer, bs bytes) int
```

Write all bytes of `bs` to `dst`, retrying on partial writes until the
whole buffer is written or `dst` fails.

# Arguments
- `dst`: The destination writer.
- `bs`: The bytes to write.

# Returns
The number of bytes actually written. If `dst.write` returns 0 or a
negative value, the loop stops and the partial count is returned; callers
can compare the result with `bs.len()` to detect an incomplete write.

# Notes
- `dst` is not flushed; buffered writers must be flushed/closed by the caller.

<a id="read_all"></a>

### `read_all`

```kl
pub func read_all(src Reader) bytes
```

Read all remaining bytes from `src` until EOF and return them as `bytes`.

Useful when the total size is unknown upfront (e.g., reading an entire
file or process output). Chunks are accumulated into a growable buffer,
so the result is assembled incrementally.

# Arguments
- `src`: The source reader.

# Returns
A `bytes` holding everything read. The loop stops at the first read that
returns 0 (EOF) or a negative value (I/O error), so the result contains
only the bytes read before that point.

<a id="read_exact"></a>

### `read_exact`

```kl
pub func read_exact(src Reader, bs bytes) int
```

Read exactly `bs.len()` bytes from `src` into `bs`, retrying on short reads.

# Arguments
- `src`: The source reader.
- `bs`: The destination buffer to fill.

# Returns
The number of bytes actually read. The loop stops early only at EOF (0) or
a read error (negative); callers can compare the result with `bs.len()` to
detect a short fill.

# Notes
- `bs` is mutated in-place with the read data.

<a id="copy"></a>

### `copy`

```kl
pub func copy(dst Writer, src Reader) int
```

Copy bytes from `src` to `dst` until the source reaches EOF.

Data is pumped through an internal 8192-byte buffer and written to `dst`
chunk by chunk, so the whole stream is never held in memory. This makes
copy suitable for files as well as interactive streams (e.g., process pipes).

# Arguments
- `dst`: The destination writer.
- `src`: The source reader.

# Returns
The total number of bytes actually written to `dst`. The loop stops at the
first read that returns 0 (EOF) or a negative value (I/O error), and also
if a write fails partway, so the result counts only the bytes transferred
before that point.

# Notes
- `dst` is not flushed; buffered writers must be flushed/closed by the caller.
- Each chunk is written via `write_all`, so a partial write by `dst` is
  completed before moving on; if `dst` fails, copy stops and returns the
  bytes written so far.

Example:
    let src = fs.open("in.bin", "r")!
    let dst = fs.open("out.bin", "w")!
    let n = copy(dst, src)
    src.close()
    dst.close()

## Constants

<a id="seek_set"></a>

### `SEEK_SET`

```kl
pub const SEEK_SET = 0
```

Seek whence: absolute position from the start of content.

<a id="seek_cur"></a>

### `SEEK_CUR`

```kl
pub const SEEK_CUR = 1
```

Seek whence: relative to the current position.

<a id="seek_end"></a>

### `SEEK_END`

```kl
pub const SEEK_END = 2
```

Seek whence: relative to the end of content.
