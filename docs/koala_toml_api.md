# Koala `koala/toml` API Reference

## Contents

**Traits**

| Name | Summary |
|------|---------|
| [`Value`](#value) | The root trait of all TOML values. |

**Classes**

| Name | Summary |
|------|---------|
| [`Array`](#array) | A TOML array value. |
| [`Boolean`](#boolean) | A TOML boolean value. |
| [`Float`](#float) | A TOML float value. |
| [`Integer`](#integer) | A TOML integer value. |
| [`Parser`](#parser) | A line-oriented parser for a practical subset of TOML. |
| [`String`](#string) | A TOML string value. |
| [`Table`](#table) | A TOML table value: an ordered mapping from string keys to values. |

**Functions**

| Name | Summary |
|------|---------|
| [`parse_file`](#parse_file) | Parse the TOML file at `path` and return its root table. |
| [`parse_str`](#parse_str) | Parse the TOML text `s` and return its root table. |
| [`dump`](#dump) | Serialize `root` to TOML text. |
| [`dump_table`](#dump_table) | Write `tbl` into `buf`. `prefix` is the dotted path of the enclosing table, used to build nested `[a.b]` headers. |

## Traits

<a id="value"></a>

### `Value`

```kl
pub trait Value {
    func to_toml() str
}
```

The root trait of all TOML values.

Every TOML value conforms to `Value`: strings, integers, floats,
booleans, arrays and tables. It allows heterogeneous values to be
stored uniformly in [`Array`](#array) and [`Table`](#table).

---

<a id="value.to_toml"></a>

**`to_toml() str`**

## Classes

<a id="array"></a>

### `Array`

```kl
pub class Array : Value {
    pub let value = list[Value]()
    pub func __init__()
    pub func extend(values Iterable[Value])
    pub func to_toml() str
    pub func add(v Value)
    pub func len() int
    pub func get(_idx int) Value
    pub func get_str(_idx int) str
    pub func get_int(_idx int) int
    pub func get_float(_idx int) float
    pub func get_bool(_idx int) bool
    pub func get_table(_idx int) Table
    pub func get_array(_idx int) Array
    pub func fmt(f Formatter)
}
```
**Bases:** [`Value`](#value)


A TOML array value.

Elements are stored as `Value`, so strings, integers, floats and
booleans may be mixed freely.

---

**Fields**

<a id="array.value"></a>

**`pub let value = list[Value]()`**

The elements of the array.

<a id="array.__init__"></a>

**`__init__()`**

<a id="array.extend"></a>

**`extend(values Iterable[Value])`**

Append all values from `values` to the end of the array.

<a id="array.to_toml"></a>

**`to_toml() str`**

Serialize to `[elem, elem, ...]`, recursively.

Elements are joined with `", "`. Mixed types are allowed, matching
what the parser accepts.

<a id="array.add"></a>

**`add(v Value)`**

Append a value to the end of the array.

<a id="array.len"></a>

**`len() int`**

Return the number of elements.

<a id="array.get"></a>

**`get(_idx int) Value`**

Return the value at `_idx`. Panic if `_idx` is out of range.

<a id="array.get_str"></a>

**`get_str(_idx int) str`**

Return the string element at `_idx`.

Panic if `_idx` is out of range or if the element is not a String.

<a id="array.get_int"></a>

**`get_int(_idx int) int`**

Return the integer element at `_idx`.

Panic if `_idx` is out of range or if the element is not an Integer.

<a id="array.get_float"></a>

**`get_float(_idx int) float`**

Return the float element at `_idx`.

Panic if `_idx` is out of range or if the element is not a Float.

<a id="array.get_bool"></a>

**`get_bool(_idx int) bool`**

Return the boolean element at `_idx`.

Panic if `_idx` is out of range or if the element is not a Boolean.

<a id="array.get_table"></a>

**`get_table(_idx int) Table`**

Return the table at `_idx`.

Panic if `_idx` is out of range or if the element is not a Table.

<a id="array.get_array"></a>

**`get_array(_idx int) Array`**

Return the array at `_idx`.

Panic if `_idx` is out of range or if the element is not an Array.

<a id="array.fmt"></a>

**`fmt(f Formatter)`**

Format the elements for display.

<a id="boolean"></a>

### `Boolean`

```kl
pub class Boolean : Value {
    pub let value bool
    pub func __init__(_value bool)
    pub func fmt(f Formatter)
    pub func to_toml() str
}
```
**Bases:** [`Value`](#value)


A TOML boolean value.

---

**Fields**

<a id="boolean.value"></a>

**`pub let value bool`**

The boolean content.

<a id="boolean.__init__"></a>

**`__init__(_value bool)`**

Create a boolean value holding `_value`.

<a id="boolean.fmt"></a>

**`fmt(f Formatter)`**

Write `true` or `false` to `f`.

<a id="boolean.to_toml"></a>

**`to_toml() str`**

Serialize to `true` or `false`.

<a id="float"></a>

### `Float`

```kl
pub class Float : Value {
    pub let value float
    pub func __init__(_value float)
    pub func fmt(f Formatter)
    pub func to_toml() str
}
```
**Bases:** [`Value`](#value)


A TOML float value.

---

**Fields**

<a id="float.value"></a>

**`pub let value float`**

The float content.

<a id="float.__init__"></a>

**`__init__(_value float)`**

Create a float value holding `_value`.

<a id="float.fmt"></a>

**`fmt(f Formatter)`**

Write the decimal representation to `f`.

<a id="float.to_toml"></a>

**`to_toml() str`**

Serialize to a bare float, e.g. `3.14`.

<a id="integer"></a>

### `Integer`

```kl
pub class Integer : Value {
    pub let value int
    pub func __init__(_value int)
    pub func fmt(f Formatter)
    pub func to_toml() str
}
```
**Bases:** [`Value`](#value)


A TOML integer value.

---

**Fields**

<a id="integer.value"></a>

**`pub let value int`**

The integer content, with `_` separators removed.

<a id="integer.__init__"></a>

**`__init__(_value int)`**

Create an integer value holding `_value`.

<a id="integer.fmt"></a>

**`fmt(f Formatter)`**

Write the decimal representation to `f`.

<a id="integer.to_toml"></a>

**`to_toml() str`**

Serialize to a bare integer, e.g. `8080`.

<a id="parser"></a>

### `Parser`

```kl
pub class Parser {
    let src list[str]
    let root = Table()
    var current = root
    pub func __init__(_src list[str])
    pub func parse() Table
    func is_complete(pending Buffer) bool
    func strip_toml_comment(line str) str
    func parse_line(line str)
    func parse_table_header(line str)
    func parse_key_value(line str)
    func parse_value(value str) Value
    func parse_number(s str) Value
    func parse_array(value str) Array
}
```

A line-oriented parser for a practical subset of TOML.

Supported syntax:
- table headers: `[name]` (single level)
- key-value pairs: `key = value`
- values: double-quoted strings, integers, floats, booleans, arrays
- comments: `#` outside of strings
- integers and floats with `_` separators, e.g. `10_000`
- statements spanning multiple lines, e.g. arrays

A number is treated as a float when it contains `.` or an exponent
marker; underscore separators are removed before conversion.

---

**Fields**

<a id="parser.src"></a>

**`let src list[str]`**

<a id="parser.root"></a>

**`let root = Table()`**

<a id="parser.current"></a>

**`var current = root`**

<a id="parser.__init__"></a>

**`__init__(_src list[str])`**

Create a parser for the given source lines.

<a id="parser.parse"></a>

**`parse() Table`**

Parse all source lines and return the root table.

Lines are accumulated until the pending text is a complete
statement, so arrays may span multiple lines. Blank lines and
comment-only lines are ignored.

<a id="parser.is_complete"></a>

**`is_complete(pending Buffer) bool`**

Return true if `pending` is a complete statement, i.e. the bracket
depth is balanced and no string is left open.

<a id="parser.strip_toml_comment"></a>

**`strip_toml_comment(line str) str`**

Strip the trailing comment from `line`.

A '#' outside of a string starts a comment; everything before it is
returned. The line is returned unchanged when there is no comment.

<a id="parser.parse_line"></a>

**`parse_line(line str)`**

Dispatch one complete statement to the header or pair parser.

<a id="parser.parse_table_header"></a>

**`parse_table_header(line str)`**

Parse a `[name]` header and switch the active table to it.

<a id="parser.parse_key_value"></a>

**`parse_key_value(line str)`**

Parse a `key = value` pair into the active table.

<a id="parser.parse_value"></a>

**`parse_value(value str) Value`**

Parse a value: string, array, boolean, or number.

<a id="parser.parse_number"></a>

**`parse_number(s str) Value`**

Parse an integer or a float, removing `_` separators.

<a id="parser.parse_array"></a>

**`parse_array(value str) Array`**

Parse a `[a, b, c]` array whose elements are strings, booleans or
numbers.

<a id="string"></a>

### `String`

```kl
pub class String : Value {
    pub let value str
    pub func __init__(_value str)
    pub func fmt(f Formatter)
    pub func to_toml() str
}
```
**Bases:** [`Value`](#value)


A TOML string value.

---

**Fields**

<a id="string.value"></a>

**`pub let value str`**

The string content, without the surrounding quotes.

<a id="string.__init__"></a>

**`__init__(_value str)`**

Create a string value holding `_value`.

<a id="string.fmt"></a>

**`fmt(f Formatter)`**

Write the string content to `f`.

<a id="string.to_toml"></a>

**`to_toml() str`**

Serialize to a double-quoted string.

`"`, `\`, `\n`, `\t`, `\r` are escaped. All other bytes, including
multi-byte UTF-8 sequences, are passed through unchanged.

<a id="table"></a>

### `Table`

```kl
pub class Table : Value {
    pub let data = dict[str, Value]()
    pub func __init__()
    pub func add(_key str, _value Value)
    pub func get(_key str) Value
    pub func get_str(_key str) str
    pub func get_int(_key str) int
    pub func get_float(_key str) float
    pub func get_bool(_key str) bool
    pub func get_table(_key str) Table
    pub func get_array(_key str) Array
    pub func __contains__(_key str) bool
    pub func add_table(_key str) Table
    pub func update(_data dict[str, Value])
    pub func keys() list[str]
    pub func fmt(f Formatter)
}
```
**Bases:** [`Value`](#value)


A TOML table value: an ordered mapping from string keys to values.

---

**Fields**

<a id="table.data"></a>

**`pub let data = dict[str, Value]()`**

The key-value mappings, in insertion order.

<a id="table.__init__"></a>

**`__init__()`**

<a id="table.add"></a>

**`add(_key str, _value Value)`**

Associate `_value` with `_key`, replacing an existing mapping.

<a id="table.get"></a>

**`get(_key str) Value`**

Return the value mapped to `_key`. Panic if `_key` is not present.

<a id="table.get_str"></a>

**`get_str(_key str) str`**

Return the string value mapped to `_key`.

Panic if `_key` is not present or if the value is not a String.

<a id="table.get_int"></a>

**`get_int(_key str) int`**

Return the integer value mapped to `_key`.

Panic if `_key` is not present or if the value is not an Integer.

<a id="table.get_float"></a>

**`get_float(_key str) float`**

Return the float value mapped to `_key`.

Panic if `_key` is not present or if the value is not a Float.

<a id="table.get_bool"></a>

**`get_bool(_key str) bool`**

Return the boolean value mapped to `_key`.

Panic if `_key` is not present or if the value is not a Boolean.

<a id="table.get_table"></a>

**`get_table(_key str) Table`**

Return the table mapped to `_key`.

Panic if `_key` is not present or if the value is not a Table.

<a id="table.get_array"></a>

**`get_array(_key str) Array`**

Return the array mapped to `_key`.

Panic if `_key` is not present or if the value is not an Array.

<a id="table.__contains__"></a>

**`__contains__(_key str) bool`**

Return true if the table contains a mapping for `_key`.

<a id="table.add_table"></a>

**`add_table(_key str) Table`**

Create a subtable under `_key` and return it.

<a id="table.update"></a>

**`update(_data dict[str, Value])`**

Merge all mappings from `_data` into the table.

<a id="table.keys"></a>

**`keys() list[str]`**

Return the keys in insertion order.

<a id="table.fmt"></a>

**`fmt(f Formatter)`**

Format the mappings for display.

## Functions

<a id="parse_file"></a>

### `parse_file`

```kl
pub func parse_file(path str) Table
```

Parse the TOML file at `path` and return its root table.

See [`Parser`](#parser) for the supported TOML subset.

Errors

Panic if the file cannot be opened, or if its content is not valid
TOML within the supported subset.

<a id="parse_str"></a>

### `parse_str`

```kl
pub func parse_str(s str) Table
```

Parse the TOML text `s` and return its root table.

See [`Parser`](#parser) for the supported TOML subset.

<a id="dump"></a>

### `dump`

```kl
pub func dump(root Table) str
```

Serialize `root` to TOML text.

Scalar keys are emitted first, then nested tables as `[name]`
sections. Dotted headers are used for nested tables. Arrays of
tables (`[[name]]`) are not supported.

<a id="dump_table"></a>

### `dump_table`

```kl
func dump_table(buf Buffer, tbl Table, prefix str)
```

Write `tbl` into `buf`. `prefix` is the dotted path of the
enclosing table, used to build nested `[a.b]` headers.
