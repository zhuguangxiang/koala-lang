# Koala database API: `koala/sqlite3`

A port of CPython's `sqlite3` module, the DB-API 2.0 interface to SQLite. Every
name, parameter order and default is Python's wherever the language allows it,
so most Python code ports by changing the import line and the error handling.

The engine is **SQLite 3.54.1**, the amalgamation vendored unmodified in
`libs/koala/sqlite3/native/sqlite3/` (its `VERSION` file records the source,
the hashes and how to upgrade), compiled into `libkoala_sqlite3.so` with
`SQLITE_THREADSAFE=1`, `SQLITE_OMIT_LOAD_EXTENSION` and `SQLITE_DQS=0`. No
system `libsqlite3` is needed.

| Package | Depends on | Native code |
|---------|-----------|-------------|
| `koala/sqlite3` | - | vendored SQLite 3.54.1 + `sqlite3_shim.c` |

## Contents

**module**

| Name | Summary |
|------|---------|
| [`connect`](#sqlite3-connect) | `connect(database, timeout=5.0, isolation_level="", cached_statements=128, uri=false)` -> `Connection?`. |
| [`Connection`](#sqlite3-connection) | A database connection: `cursor`, `execute`, `commit`, `rollback`, `close`, ... |
| [`Cursor`](#sqlite3-cursor) | Runs statements and reads rows: `execute`, `fetchone`, `fetchall`, `rowcount`, ... |
| [`Row`](#sqlite3-row) | One result row: `row[0]`, `row["name"]`, `keys()`, `len(row)`. |
| [`sqlite_version`](#sqlite3-version) | The SQLite version, `"3.54.1"`. |
| [`sqlite_version_info`](#sqlite3-version) | The same as numbers, `(3, 54, 1)`. |

**constants** - the 113 result codes under Python's names, primary and
extended: `SQLITE_OK`, `SQLITE_ERROR`, `SQLITE_BUSY`, `SQLITE_LOCKED`,
`SQLITE_CONSTRAINT`, ..., `SQLITE_ROW`, `SQLITE_DONE`, and the extended ones
such as `SQLITE_CONSTRAINT_UNIQUE`, `SQLITE_CONSTRAINT_PRIMARYKEY`,
`SQLITE_BUSY_TIMEOUT`, `SQLITE_IOERR_READ`. See [Errors](#sqlite3-errors).

## Differences from Python

The API is Python's wherever the language allows it. These deltas are
unavoidable, and all of them are deliberate:

| Python | Koala | Why |
|--------|-------|-----|
| raises `sqlite3.OperationalError`, `IntegrityError`, ... | the call returns nil (`connect`, `execute`, `executemany`, `executescript`, `fetchmany`, `fetchall`, `fetchone`) or an error code (`commit`, `rollback`, `set_isolation_level`); `conn.sqlite_errorcode()`, `sqlite_errorname()`, `errmsg()` tell what failed | Koala has no exceptions; a database error is an ordinary outcome, so it is a return value, C style. |
| raises `ProgrammingError`, `ValueError`, `IndexError` | panics with the same message text, prefixed `sqlite3: ` | These are bugs in the calling code, not outcomes to branch on. |
| `fetchone()` returns a tuple unless `row_factory = sqlite3.Row` | always a `Row` | A Koala tuple has a length fixed at compile time; a row's length is known only at run time. `Row` reads as a tuple does (`row[0]`) and adds `row["name"]`. |
| `execute(sql, {"id": 1})` | `execute(sql, named={"id": 1})` | One parameter cannot be a list or a dict without being `any`; `params` takes the list, `named` the dict. |
| `conn.in_transaction`, `conn.total_changes`, `conn.isolation_level = x` | `in_transaction()`, `total_changes()`, `isolation_level()`, `set_isolation_level(x)` | Koala has no computed attributes or setters. |
| a value read from a row is a typed Python object | an `any?`, read with `(row[0]! as int)!` | Koala is statically typed and a column's type is known only at run time. |
| `with conn:` commits or rolls back | call `commit()` / `rollback()` | Koala has no `with`. |
| a connection is closed when garbage collected | call `close()` | The runtime does not run finalizers yet (`Koala_TODO.md` §79). |
| `for row in cur.execute(...)` | `while let row = cur.fetchone() { ... }` | `Cursor` is not iterable yet (`Koala_TODO.md` §85). |

Not provided yet - type adapters and converters, `create_function` and the
other callbacks, `backup`, `iterdump`, `blobopen`, `interrupt`, the
`autocommit` attribute, `apilevel` / `paramstyle` / `threadsafety` - see
`Koala_TODO.md` §85.

## Values

A column value and a parameter are one of SQLite's storage classes:

| SQLite | Koala, read | Koala, bound |
|--------|-------------|--------------|
| NULL | `nil` | `nil` |
| INTEGER | `int` | `int`, or `bool` as 0 / 1 |
| REAL | `float` | `float` |
| TEXT | `str` | `str` |
| BLOB | `bytes` | `bytes` |

Binding any other type panics: `sqlite3: Error binding parameter 1: type
'list' is not supported`. A `bool` reads back as an `int`, as in Python.

Parameter lists are `list[any?]`, and a literal written in the call takes
that type, so `["ann", 30]` and `[["a", 1], ["b", nil]]` need no annotation.

## Reference

<a id="sqlite3-connect"></a>
### `connect(database, timeout = 5.0, isolation_level = "", cached_statements = 128, uri = false) -> Connection?`

Open the database file `database`, created when it does not exist;
`":memory:"` opens a private database in memory. Returns nil when the file
cannot be opened (a missing directory, no permission).

- `timeout` - seconds a statement waits for a lock held by another
  connection before it fails with `SQLITE_BUSY`.
- `isolation_level` - `""` (the default; SQLite's `DEFERRED`),
  `"DEFERRED"`, `"IMMEDIATE"`, `"EXCLUSIVE"`, in any case, or `nil` for
  autocommit. Any other string panics, as Python's `ValueError`. See
  [Transactions](#sqlite3-transactions).
- `cached_statements` - how many compiled statements the connection keeps
  for reuse; `0` turns the cache off.
- `uri` - read `database` as a URI, as `"file:app.db?mode=ro"`.

```kl
let conn = sqlite3.connect("app.db")!
let mem = sqlite3.connect(":memory:", isolation_level=nil)!
```

<a id="sqlite3-connection"></a>
### `Connection`

| Member | Python | Notes |
|--------|--------|-------|
| `cursor() -> Cursor` | `Connection.cursor` | |
| `execute(sql, params = nil, named = nil) -> Cursor?` | `Connection.execute` | `cursor().execute(...)`. |
| `executemany(sql, params = nil, named = nil) -> Cursor?` | `Connection.executemany` | `cursor().executemany(...)`. |
| `executescript(sql_script) -> Cursor?` | `Connection.executescript` | `cursor().executescript(...)`. |
| `commit() -> int` | `Connection.commit` | Commits the open transaction, if any. `SQLITE_OK`, or the error code. |
| `rollback() -> int` | `Connection.rollback` | Rolls it back, if any. `SQLITE_OK`, or the error code. |
| `close()` | `Connection.close` | Closes the cursors, then the connection. An open transaction is rolled back, not committed. Closing twice is safe. |
| `in_transaction() -> bool` | `in_transaction` | |
| `total_changes() -> int` | `total_changes` | Rows changed since the connection was opened. |
| `isolation_level() -> str?` | `isolation_level` | In upper case: `""`, `"DEFERRED"`, `"IMMEDIATE"`, `"EXCLUSIVE"`, or nil. |
| `set_isolation_level(level) -> int` | `isolation_level = level` | Setting nil commits the open transaction first. `SQLITE_OK`, or that commit's error code. |
| `sqlite_errorcode() -> int` | `Error.sqlite_errorcode` | The last error's code, `SQLITE_OK` after a call that succeeded. |
| `sqlite_errorname() -> str` | `Error.sqlite_errorname` | Its name, as `"SQLITE_CONSTRAINT_UNIQUE"`. |
| `errmsg() -> str` | `str(error)` | Its message, as `"UNIQUE constraint failed: t.id"`; `""` after a success. |

Using a closed connection panics: `sqlite3: Cannot operate on a closed
database.`

<a id="sqlite3-cursor"></a>
### `Cursor`

| Member | Python | Notes |
|--------|--------|-------|
| `execute(sql, params = nil, named = nil) -> Cursor?` | `Cursor.execute` | One statement. `params` binds the `?` placeholders in order; `named` binds `:name`, `@name` and `$name` by name. Returns this cursor. |
| `executemany(sql, params = nil, named = nil) -> Cursor?` | `Cursor.executemany` | The statement once per list of `params` (`list[list[any?]]`) or dict of `named` (`list[dict[str, any?]]`). Stops at the first error. |
| `executescript(sql_script) -> Cursor?` | `Cursor.executescript` | Every statement of the script, no parameters. Commits an open transaction first. |
| `fetchone() -> Row?` | `Cursor.fetchone` | The next row; nil at the end, or on an error. |
| `fetchmany(size = arraysize) -> list[Row]?` | `Cursor.fetchmany` | Up to `size` rows; nil on an error. |
| `fetchall() -> list[Row]?` | `Cursor.fetchall` | The remaining rows; nil on an error. |
| `close()` | `Cursor.close` | Using the cursor afterwards panics. Closing twice is safe. |
| `connection` | `Cursor.connection` | |
| `arraysize` | `Cursor.arraysize` | Default `1`. |
| `rowcount` | `Cursor.rowcount` | Rows changed by the last INSERT, UPDATE, DELETE or REPLACE (summed over `executemany`); `-1` after any other statement. |
| `lastrowid` | `Cursor.lastrowid` | The rowid of the last insert on the connection, nil before the first `execute`; `executemany` does not update it, as in Python. |
| `description` | `Cursor.description` | One 7-tuple per column: the name, then six nils. Nil after a statement that returns no rows. |

The panics follow Python's `ProgrammingError`s:

| Mistake | Panic |
|---------|-------|
| two statements in `execute` | `You can only execute one statement at a time.` |
| a wrong number of `?` values | `Incorrect number of bindings supplied. The current statement uses 2, and there are 1 supplied.` |
| a missing named value | `You did not supply a value for binding parameter :id.` |
| `named` for a `?` placeholder | `Binding 1 has no name, but you supplied a dictionary (which has only names).` |
| `executemany` of a query | `executemany() can only execute DML statements.` |
| both `params` and `named` | `params and named cannot both be given` |
| a closed cursor | `Cannot operate on a closed cursor.` |

Spaces and comments after the one statement are allowed: `"select 1; -- done"`.

<a id="sqlite3-row"></a>
### `Row`

| Member | Python | Notes |
|--------|--------|-------|
| `row[i] -> any?` | `row[i]` | Negative from the end. Out of range panics. |
| `row["name"] -> any?` | `row["name"]` | Matched case-insensitively, the first column of that name. Unknown names panic: `No item with that key`. |
| `keys() -> list[str]` | `Row.keys` | The column names, in order. |
| `len()` / `len(row)` | `len(row)` | The number of columns. |
| `for value in row` | iteration | The values, in order. |

```kl
let row = conn.execute("select id, name from people where id = ?", [1])!.fetchone()!
let id = (row[0]! as int)!
let name = (row["name"]! as str)!
```

<a id="sqlite3-version"></a>
### `sqlite_version` / `sqlite_version_info`

The version of the linked SQLite, `"3.54.1"` and `(3, 54, 1)`.

<a id="sqlite3-errors"></a>
## Errors

A failed call returns nil (or an error code), and the connection keeps the
error until its next call:

```kl
let cur = conn.execute("insert into people(id, name) values (?, ?)", [1, "ann"])
if cur == nil {
    if conn.sqlite_errorcode() == sqlite3.SQLITE_CONSTRAINT_PRIMARYKEY {
        print("duplicate id")
    }
    print(conn.sqlite_errorname(), conn.errmsg())
    // SQLITE_CONSTRAINT_PRIMARYKEY UNIQUE constraint failed: people.id
}
```

Codes are SQLite's extended result codes. The low 8 bits are the primary code,
so `code & 0xff == sqlite3.SQLITE_CONSTRAINT` matches any constraint failure
and `code & 0xff == sqlite3.SQLITE_BUSY` any lock timeout. Every call that can
fail resets the error first, so after `fetchall()` returns nil the error is the
one that stopped it.

`fetchone()` returns nil both at the end of the rows and on an error; check
`sqlite_errorcode()` when the difference matters.

<a id="sqlite3-transactions"></a>
## Transactions

The behaviour is Python's legacy transaction control (`autocommit` =
`LEGACY_TRANSACTION_CONTROL`, the default up to Python 3.14):

- Unless `isolation_level` is nil, an INSERT, UPDATE, DELETE or REPLACE opens
  a transaction with `BEGIN <isolation_level>` when none is open, so changes
  are kept only after `commit()`.
- Other statements - SELECT, CREATE, DROP, ... - open none.
- `close()` does not commit: uncommitted changes are rolled back.
- `executescript()` commits an open transaction, then runs the script as
  written; a `BEGIN` / `COMMIT` in the script is the script's own.
- With `isolation_level=nil` every statement commits on its own, and an
  explicit `BEGIN` opens a transaction as written.

```kl
let conn = sqlite3.connect("app.db")!
conn.execute("create table if not exists log(msg text)")
conn.executemany("insert into log values (?)", [["start"], ["stop"]])
print(conn.in_transaction())    // true
conn.commit()
conn.close()
```

## Statement cache

A connection keeps up to `cached_statements` compiled statements, keyed by
their SQL text, the least recently used going first. A statement in use by a
cursor is out of the cache, so two cursors running the same SQL do not share
one. Running the same text again with new parameters skips the compile:

```kl
for name in names {
    conn.execute("insert into people(name) values (?)", [name])
}
```

## Example

```kl
import "koala/sqlite3"

func main() {
    let conn = sqlite3.connect(":memory:")!
    conn.execute("create table people(id integer primary key, name text, age integer)")
    conn.executemany("insert into people(name, age) values (?, ?)",
                     [["ann", 30], ["bob", 25], ["cid", nil]])
    conn.commit()

    let cur = conn.execute("select name, age from people where age > :min order by age",
                           named={"min": 20})!
    while let row = cur.fetchone() {
        let name = (row["name"]! as str)!
        let age = (row["age"]! as int)!
        print(name, age)
    }
    // bob 25
    // ann 30

    conn.close()
}
```

## Verification

`libs/koala/sqlite3/test/` (59 tests, run with `wombat test sqlite3`):
`test_connection.kl` (opening, URI, error codes and messages, commit and
rollback, file databases), `test_cursor.kl` (every storage class, Unicode,
64-bit integers, fetching, `rowcount`, `lastrowid`, `description`),
`test_row.kl` (index, negative index, case-insensitive names, `keys`,
iteration), `test_transaction.kl` (implicit `BEGIN` under each isolation
level, autocommit, `close` without commit, `executescript`, a lock timeout
between two connections), `test_params.kl` (named parameters, `executemany`,
the statement cache and its eviction) and `test_version.kl`.
