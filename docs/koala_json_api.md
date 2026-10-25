# Koala `koala/json` API Reference

## Contents

**Traits**

| Name | Summary |
|------|---------|
| [`Value`](#value) | The root trait of all JSON values. |

**Classes**

| Name | Summary |
|------|---------|
| [`Array`](#array) | A JSON array value. |
| [`Boolean`](#boolean) | A JSON boolean value. |
| [`Float`](#float) | A JSON float value. |
| [`Integer`](#integer) | A JSON integer value. |
| [`Null`](#null) | A JSON null value. |
| [`Object`](#object) | A JSON object value: an ordered mapping from string keys to values. |
| [`Parser`](#parser) | A recursive descent parser for JSON. |
| [`String`](#string) | A JSON string value. |

**Functions**

| Name | Summary |
|------|---------|
| [`parse_file`](#parse_file) | Parse the JSON file at `path` and return its root value. |
| [`parse_str`](#parse_str) | Parse the JSON text `s` and return its root value. |
| [`dump`](#dump) | Serialize `v` to JSON text. |
| [`write_value`](#write_value) | Write the value `v` to `buf`. |
| [`write_object`](#write_object) | Write the object `obj` to `buf`. |
| [`write_array`](#write_array) | Write the array `arr` to `buf`. |
| [`write_break`](#write_break) | Write a line break followed by the indentation of the level `level`. |
| [`format_number`](#format_number) | Return the JSON text of the float `v`. |
| [`parse_array`](#parse_array) | Parse a `[...]` array at the current position. |
| [`parse_string`](#parse_string) | Parse a double-quoted string at the current position. |
| [`parse_escape`](#parse_escape) | Parse the escape sequence at the current position and write the decoded text to `buf`. |
| [`parse_unicode_escape`](#parse_unicode_escape) | Parse a `\uXXXX` escape sequence at the current position and write the decoded text to `buf`. |
| [`parse_hex4`](#parse_hex4) | Parse the four hexadecimal digits of a `\uXXXX` escape sequence at the current position. |
| [`hex_value`](#hex_value) | Return the numeric value of the hexadecimal digit `ch`, or `-1` if `ch` is not a hexadecimal digit. |
| [`write_codepoint`](#write_codepoint) | Write the UTF-8 encoding of the code point `cp` to `buf`. |
| [`parse_number`](#parse_number) | Parse a JSON number at the current position. |
| [`enter_nested`](#enter_nested) | Count one more level of object or array nesting. |
| [`skip_ws`](#skip_ws) | Skip the whitespace at the current position. |
| [`is_digit`](#is_digit) | Return true if `ch` is a decimal digit. |
| [`expect_word`](#expect_word) | Consume the literal `word` at the current position. |
| [`quote`](#quote) | Return `s` as a quoted JSON string, including the surrounding double quotes. |
| [`hex_byte`](#hex_byte) | Return the two-digit lowercase hexadecimal representation of the byte `b`. |

**Constants**

| Name | Summary |
|------|---------|
| [`MAX_DEPTH`](#max_depth) | The deepest nesting level the parser accepts, counting each object and each array. Real-world JSON stays far below this; the limit exists so that a hostile document raises instead of running the process out of stack. |

## Traits

<a id="value"></a>

### `Value`

```kl
pub trait Value {
    func to_json() str
}
```

The root trait of all JSON values.

Every JSON value conforms to `Value`: objects, arrays, strings,
numbers, booleans and null. It allows heterogeneous values to be
stored uniformly in [`Array`](#array) and [`Object`](#object).

---

<a id="value.to_json"></a>

**`to_json() str`**

Serialize the value to compact JSON text.

## Classes

<a id="array"></a>

### `Array`

```kl
pub class Array : Value {
    pub let value = list[Value]()
    pub func __init__()
    pub func add(v Value)
    pub func extend(values Iterable[Value])
    pub func len() int
    pub func get_str(_idx int) str
    pub func get_int(_idx int) int
    pub func get_float(_idx int) float
    pub func get_bool(_idx int) bool
    pub func get_object(_idx int) Object
    pub func get_array(_idx int) Array
    pub func fmt(f Formatter)
    pub func to_json() str
}
```
**Bases:** [`Value`](#value)


A JSON array value.

Elements are stored as `Value`, so strings, integers, floats,
booleans, nulls, objects and arrays may be mixed freely.

---

**Fields**

<a id="array.value"></a>

**`pub let value = list[Value]()`**

The elements of the array.

<a id="array.__init__"></a>

**`__init__()`**

<a id="array.add"></a>

**`add(v Value)`**

Append a value to the end of the array.

<a id="array.extend"></a>

**`extend(values Iterable[Value])`**

Append all values from `values` to the end of the array.

<a id="array.len"></a>

**`len() int`**

Return the number of elements.

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

<a id="array.get_object"></a>

**`get_object(_idx int) Object`**

Return the object element at `_idx`.

Panic if `_idx` is out of range or if the element is not an Object.

<a id="array.get_array"></a>

**`get_array(_idx int) Array`**

Return the array element at `_idx`.

Panic if `_idx` is out of range or if the element is not an Array.

<a id="array.fmt"></a>

**`fmt(f Formatter)`**

Format the elements for display.

<a id="array.to_json"></a>

**`to_json() str`**

Serialize to a JSON array, e.g. `[1,"two",true]`.

<a id="boolean"></a>

### `Boolean`

```kl
pub class Boolean : Value {
    pub let value bool
    pub func __init__(_value bool)
    pub func fmt(f Formatter)
    pub func to_json() str
}
```
**Bases:** [`Value`](#value)


A JSON boolean value.

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

<a id="boolean.to_json"></a>

**`to_json() str`**

Serialize to `true` or `false`.

<a id="float"></a>

### `Float`

```kl
pub class Float : Value {
    pub let value float
    pub func __init__(_value float)
    pub func fmt(f Formatter)
    pub func to_json() str
}
```
**Bases:** [`Value`](#value)


A JSON float value.

JSON numbers with a fraction or an exponent are stored as floats.

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

<a id="float.to_json"></a>

**`to_json() str`**

Serialize to a bare float, e.g. `3.14`.

<a id="integer"></a>

### `Integer`

```kl
pub class Integer : Value {
    pub let value int
    pub func __init__(_value int)
    pub func fmt(f Formatter)
    pub func to_json() str
}
```
**Bases:** [`Value`](#value)


A JSON integer value.

JSON numbers without a fraction or an exponent are stored as integers.

---

**Fields**

<a id="integer.value"></a>

**`pub let value int`**

The integer content.

<a id="integer.__init__"></a>

**`__init__(_value int)`**

Create an integer value holding `_value`.

<a id="integer.fmt"></a>

**`fmt(f Formatter)`**

Write the decimal representation to `f`.

<a id="integer.to_json"></a>

**`to_json() str`**

Serialize to a bare integer, e.g. `8080`.

<a id="null"></a>

### `Null`

```kl
pub class Null : Value {
    pub func __init__()
    pub func fmt(f Formatter)
    pub func to_json() str
}
```
**Bases:** [`Value`](#value)


A JSON null value.

---

<a id="null.__init__"></a>

**`__init__()`**

<a id="null.fmt"></a>

**`fmt(f Formatter)`**

Write `null` to `f`.

<a id="null.to_json"></a>

**`to_json() str`**

Serialize to `null`.

<a id="object"></a>

### `Object`

```kl
pub class Object : Value {
    pub let data = dict[str, Value]()
    pub func __init__()
    pub func add(_key str, _value Value)
    pub func get(_key str) Value
    pub func get_str(_key str) str
    pub func get_int(_key str) int
    pub func get_float(_key str) float
    pub func get_bool(_key str) bool
    pub func get_object(_key str) Object
    pub func get_array(_key str) Array
    pub func __contains__(_key str) bool
    pub func len() int
    pub func keys() list[str]
    pub func fmt(f Formatter)
    pub func to_json() str
}
```
**Bases:** [`Value`](#value)


A JSON object value: an ordered mapping from string keys to values.

---

**Fields**

<a id="object.data"></a>

**`pub let data = dict[str, Value]()`**

The key-value mappings, in insertion order.

<a id="object.__init__"></a>

**`__init__()`**

<a id="object.add"></a>

**`add(_key str, _value Value)`**

Associate `_value` with `_key`, replacing an existing mapping.

<a id="object.get"></a>

**`get(_key str) Value`**

Return the value mapped to `_key`. Panic if `_key` is not present.

<a id="object.get_str"></a>

**`get_str(_key str) str`**

Return the string value mapped to `_key`.

Panic if `_key` is not present or if the value is not a String.

<a id="object.get_int"></a>

**`get_int(_key str) int`**

Return the integer value mapped to `_key`.

Panic if `_key` is not present or if the value is not an Integer.

<a id="object.get_float"></a>

**`get_float(_key str) float`**

Return the float value mapped to `_key`.

Panic if `_key` is not present or if the value is not a Float.

<a id="object.get_bool"></a>

**`get_bool(_key str) bool`**

Return the boolean value mapped to `_key`.

Panic if `_key` is not present or if the value is not a Boolean.

<a id="object.get_object"></a>

**`get_object(_key str) Object`**

Return the object value mapped to `_key`.

Panic if `_key` is not present or if the value is not an Object.

<a id="object.get_array"></a>

**`get_array(_key str) Array`**

Return the array value mapped to `_key`.

Panic if `_key` is not present or if the value is not an Array.

<a id="object.__contains__"></a>

**`__contains__(_key str) bool`**

Return true if the object contains a mapping for `_key`.

<a id="object.len"></a>

**`len() int`**

Return the number of mappings.

<a id="object.keys"></a>

**`keys() list[str]`**

Return the keys in insertion order.

<a id="object.fmt"></a>

**`fmt(f Formatter)`**

Format the mappings for display.

<a id="object.to_json"></a>

**`to_json() str`**

Serialize to a JSON object, e.g. `{"a":1,"b":2}`.

<a id="parser"></a>

### `Parser`

```kl
pub class Parser {
    let src str
    var pos = 0
    var depth = 0
    pub func __init__(_src str)
    pub func parse() Value
    func parse_value() Value
}
```

A recursive descent parser for JSON.

The parser walks over the bytes of the source text, so the multi-byte
UTF-8 sequences of a string are copied unchanged.

Supported syntax:
- objects: `{"key":value,...}`
- arrays: `[value,...]`
- strings: double-quoted, with the `\uXXXX` escape and the surrogate
  pairs
- numbers: integers and floats, with an optional fraction and an
  optional exponent
- literals: `true`, `false` and `null`
- whitespace: space, tab, carriage return and line feed

---

**Fields**

<a id="parser.src"></a>

**`let src str`**

<a id="parser.pos"></a>

**`var pos = 0`**

<a id="parser.depth"></a>

**`var depth = 0`**

<a id="parser.__init__"></a>

**`__init__(_src str)`**

Create a parser for the source text `_src`.

<a id="parser.parse"></a>

**`parse() Value`**

Parse the whole source text and return the root value.

Errors

Panic if the source text is not valid JSON.

<a id="parser.parse_value"></a>

**`parse_value() Value`**

Parse one value at the current position.

<a id="string"></a>

### `String`

```kl
pub class String : Value {
    pub let value str
    pub func __init__(_value str)
    pub func fmt(f Formatter)
    pub func to_json() str
}
```
**Bases:** [`Value`](#value)


A JSON string value.

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

<a id="string.to_json"></a>

**`to_json() str`**

Serialize to a double-quoted JSON string.

`"`, `\`, `\b`, `\f`, `\n`, `\r` and `\t` are escaped. The other
control characters are written as `\u00XX`. All other bytes,
including multi-byte UTF-8 sequences, are passed through unchanged.

## Functions

<a id="parse_file"></a>

### `parse_file`

```kl
pub func parse_file(path str) Value
```

Parse the JSON file at `path` and return its root value.

See [`Parser`](#parser) for the supported JSON syntax.

Errors

Panic if the file cannot be opened, or if its content is not valid
JSON.

<a id="parse_str"></a>

### `parse_str`

```kl
pub func parse_str(s str) Value
```

Parse the JSON text `s` and return its root value.

See [`Parser`](#parser) for the supported JSON syntax.

Errors

Panic if `s` is not valid JSON.

<a id="dump"></a>

### `dump`

```kl
pub func dump(v Value, indent = 0) str
```

Serialize `v` to JSON text.

The output is compact by default. A positive `indent` produces a
pretty-printed text, where every nested level is indented by that
many spaces.

Example

```kl
print(json.dump(json.parse_str("{\"a\":[1,2]}")))
print(json.dump(json.parse_str("{\"a\":[1,2]}"), 2))
```

<a id="write_value"></a>

### `write_value`

```kl
func write_value(buf Buffer, v Value, indent int, level int)
```

Write the value `v` to `buf`.

<a id="write_object"></a>

### `write_object`

```kl
func write_object(buf Buffer, obj Object, indent int, level int)
```

Write the object `obj` to `buf`.

<a id="write_array"></a>

### `write_array`

```kl
func write_array(buf Buffer, arr Array, indent int, level int)
```

Write the array `arr` to `buf`.

<a id="write_break"></a>

### `write_break`

```kl
func write_break(buf Buffer, indent int, level int)
```

Write a line break followed by the indentation of the level `level`.

<a id="format_number"></a>

### `format_number`

```kl
func format_number(v float) str
```

Return the JSON text of the float `v`.

A float always keeps a fraction or an exponent so that it stays
distinguishable from an integer. Non-finite values have no JSON
representation and are rejected.

Errors

Panic if `v` is NaN or infinite.

<a id="parse_array"></a>

### `parse_array`

```kl
func parse_array() Array
```

Parse a `[...]` array at the current position.

<a id="parse_string"></a>

### `parse_string`

```kl
func parse_string() str
```

Parse a double-quoted string at the current position.

<a id="parse_escape"></a>

### `parse_escape`

```kl
func parse_escape(buf Buffer)
```

Parse the escape sequence at the current position and write the
decoded text to `buf`.

<a id="parse_unicode_escape"></a>

### `parse_unicode_escape`

```kl
func parse_unicode_escape(buf Buffer)
```

Parse a `\uXXXX` escape sequence at the current position and write
the decoded text to `buf`.

A high surrogate must be followed by a low surrogate, so that the
pair is decoded as a single code point above the basic plane.

<a id="parse_hex4"></a>

### `parse_hex4`

```kl
func parse_hex4() int
```

Parse the four hexadecimal digits of a `\uXXXX` escape sequence at
the current position.

<a id="hex_value"></a>

### `hex_value`

```kl
func hex_value(ch str) int
```

Return the numeric value of the hexadecimal digit `ch`, or `-1` if
`ch` is not a hexadecimal digit.

<a id="write_codepoint"></a>

### `write_codepoint`

```kl
func write_codepoint(buf Buffer, cp int)
```

Write the UTF-8 encoding of the code point `cp` to `buf`.

<a id="parse_number"></a>

### `parse_number`

```kl
func parse_number() Value
```

Parse a JSON number at the current position.

A number without a fraction and without an exponent is an integer,
otherwise it is a float.

<a id="enter_nested"></a>

### `enter_nested`

```kl
func enter_nested()
```

Count one more level of object or array nesting.

Errors

Panic if the nesting would go deeper than `MAX_DEPTH`.

<a id="skip_ws"></a>

### `skip_ws`

```kl
func skip_ws()
```

Skip the whitespace at the current position.

<a id="is_digit"></a>

### `is_digit`

```kl
func is_digit(ch str) bool
```

Return true if `ch` is a decimal digit.

<a id="expect_word"></a>

### `expect_word`

```kl
func expect_word(word str)
```

Consume the literal `word` at the current position.

<a id="quote"></a>

### `quote`

```kl
func quote(s str) str
```

Return `s` as a quoted JSON string, including the surrounding double
quotes.

The escape sequences follow the JSON grammar: `"`, `\`, `\b`, `\f`,
`\n`, `\r` and `\t` have a short form and the remaining control
characters are written as `\u00XX`. Every other byte, including
multi-byte UTF-8 sequences, is copied unchanged.

<a id="hex_byte"></a>

### `hex_byte`

```kl
func hex_byte(b uint8) str
```

Return the two-digit lowercase hexadecimal representation of the byte
`b`.

## Constants

<a id="max_depth"></a>

### `MAX_DEPTH`

```kl
const MAX_DEPTH = 64
```

The deepest nesting level the parser accepts, counting each object and
each array. Real-world JSON stays far below this; the limit exists so that
a hostile document raises instead of running the process out of stack.
