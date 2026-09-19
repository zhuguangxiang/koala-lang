# Koala `std/builtin` API Reference

## Contents

**Traits**

| Name | Summary |
|------|---------|
| [`any`](#any) | The `any` trait is the root of the Koala type hierarchy. Every class and trait implicitly inherits from `any`. It defines no methods and imposes no behavioral requirements. It exists solely as the universal supertype for all Koala values. |
| [`Equatable`](#equatable) | `Equatable` defines semantic equality between two values of the same type. The type parameter `T` is the type compared against, which is the conforming type itself. |
| [`Comparable`](#comparable) | `Comparable` defines a total ordering between values of the same type. The type parameter `T` is the type compared against, which is the conforming type itself. Types conforming to `Comparable` can be sorted, compared, and used in ordered data structures such as priority queues or tree-based collections. |
| [`Hashable`](#hashable) | `Hashable` defines the ability for a type to produce a stable hash value. Conforming to `Hashable` allows a type to be used as a key in dictionaries, sets, and other hash-based collections. |
| [`Printable`](#printable) | `Printable` defines a human-readable string representation for a type, used by [`print`](#print) and other display mechanisms. It is a display contract, never a content conversion (e.g. `bytes` displays its hexadecimal form, while decoding into a `str` uses [`bytes.to_str`](#bytes.to_str)). |
| [`Sequence`](#sequence) | A Sequence provides read-only indexed access to a collection of elements. |
| [`MutableSequence`](#mutablesequence) | A MutableSequence is an ordered collection that supports modification. |
| [`Map`](#map) | A Map is a collection of key-value mappings. |
| [`Set`](#set) | A Set is a collection that contains no duplicate elements. |
| [`Iterator`](#iterator) | An Iterator represents the state of an ongoing traversal of elements. |
| [`Iterable`](#iterable) | The Iterable trait is the basis for all objects that can be iterated over. |
| [`Arithmetic`](#arithmetic) | Trait for basic arithmetic operations. |
| [`Bitwise`](#bitwise) | Trait for bitwise operations. |

**Classes**

| Name | Summary |
|------|---------|
| [`bool`](#bool) | Represents a logical value that can be either true or false. Boolean types are typically used in conditional statements and logical operations. |
| [`ByteBuf`](#bytebuf) | ByteBuf is a mutable, growable byte buffer for general-purpose buffered I/O. |
| [`bytes`](#bytes) | A bytes is a fixed-size, readable and writable byte array. |
| [`BytesIter`](#bytesiter) | An iterator over the bytes of a byte array, from left to right. |
| [`BytesRevIter`](#bytesreviter) | An iterator over the bytes of a byte array, from right to left. |
| [`dict`](#dict) | A dict is a mutable, insertion-ordered hash map that maps unique keys to values. |
| [`Exception`](#exception) | An exception object represents a raised exception. Once raised, it cannot be caught and must be resolved. |
| [`list`](#list) | A list is a mutable sequence of elements. |
| [`NilType`](#niltype) | Represents a missing optional value in Koala. |
| [`int8`](#int8) | An 8-bit signed integer. |
| [`int16`](#int16) | A 16-bit signed integer. |
| [`int32`](#int32) | A 32-bit signed integer. |
| [`int64`](#int64) | A 64-bit signed integer. |
| [`uint8`](#uint8) | An 8-bit unsigned integer. |
| [`uint16`](#uint16) | A 16-bit unsigned integer. |
| [`uint32`](#uint32) | A 32-bit unsigned integer. |
| [`uint64`](#uint64) | A 64-bit unsigned integer. |
| [`float16`](#float16) | A 16-bit floating-point number (half precision). |
| [`float32`](#float32) | A 32-bit floating-point number (single precision). |
| [`float64`](#float64) | A 64-bit floating-point number (double precision). |
| [`range`](#range) | A range represents an immutable sequence of integers. |
| [`HashSet`](#hashset) | A HashSet is a mutable, unordered collection of unique elements, backed by a hash table. |
| [`TreeSet`](#treeset) | A TreeSet is a mutable, sorted collection of unique elements, backed by a balanced tree. |
| [`slice`](#slice) | A slice object represents a set of indices specified by `[start:end:step]`. |
| [`str`](#str) | A str is an immutable sequence of characters. |
| [`StrIter`](#striter) | An iterator over the characters of a string, from left to right. |
| [`StrRevIter`](#strreviter) | An iterator over the characters of a string, from right to left. |
| [`tuple`](#tuple) | A tuple is an immutable sequence of elements. |
| [`type`](#type) | The `type` class represents the runtime type of a value. |

**Functions**

| Name | Summary |
|------|---------|
| [`print`](#print) | Prints objects separated by `sep` and followed by `end`. |
| [`len`](#len) | Return the number of items in a collection or the length of an object. |
| [`hash`](#hash) | Return the hash value of an object. |
| [`panic`](#panic) | Immediately abort program execution with an error message. |
| [`format`](#format) | Format a string using positional arguments. |
| [`typeof`](#typeof) | Return the runtime type of a value. |
| [`abs`](#abs) | Computes the absolute value using a matching specialization. |
| [`abs_int64`](#abs_int64) | Specialized explicit implementation of `abs()` for 64-bit signed integers. |
| [`abs_float64`](#abs_float64) | Specialized explicit implementation of `abs()` for 64-bit floating-point numbers. |
| [`min`](#min) | Constrained generic minimum function, auto-specialized for primitives. |
| [`max`](#max) | Constrained generic maximum function, auto-specialized for primitives. |
| [`pow`](#pow) | Computes a power using a matching specialization. |
| [`pow_int64_int64`](#pow_int64_int64) | Specialized explicit implementation for 64-bit integer base and exponent. |
| [`pow_float64_int64`](#pow_float64_int64) | Specialized explicit implementation for float base and integer exponent. |
| [`pow_float64_float64`](#pow_float64_float64) | Specialized explicit implementation for float base and float exponent. |

## Traits

<a id="any"></a>

### `any`

```kl
pub trait any {}
```

The `any` trait is the root of the Koala type hierarchy.
Every class and trait implicitly inherits from `any`.
It defines no methods and imposes no behavioral requirements.
It exists solely as the universal supertype for all Koala values.

<a id="equatable"></a>

### `Equatable`

```kl
pub trait Equatable[T] {
    func __eq__(other T) bool
    func __ne__(other T) bool
}
```

`Equatable` defines semantic equality between two values of the same type.
The type parameter `T` is the type compared against, which is the
conforming type itself.

Types conforming to `Equatable` must provide both `__eq__` and `__ne__`,
ensuring consistent behavior for equality and inequality checks.

Conforming to `Equatable` allows a type to be used in conditional logic,
comparisons, and other equality-based operations.

---

<a id="equatable.__eq__"></a>

**`__eq__(other T) bool`**

Return true if `self` is equal to `other` (`self == other`).

<a id="equatable.__ne__"></a>

**`__ne__(other T) bool`**

Return true if `self` is not equal to `other` (`self != other`).

<a id="comparable"></a>

### `Comparable`

```kl
pub trait Comparable[T] : Equatable[T] {
    func __lt__(other T) bool
    func __le__(other T) bool
    func __gt__(other T) bool
    func __ge__(other T) bool
}
```
**Supertraits:** [`Equatable[T]`](#equatable)


`Comparable` defines a total ordering between values of the same type.
The type parameter `T` is the type compared against, which is the
conforming type itself.
Types conforming to `Comparable` can be sorted, compared, and used in
ordered data structures such as priority queues or tree-based collections.

`Comparable` inherits from [`Equatable`](#equatable), so conforming types must also
implement equality and inequality operations.

---

<a id="comparable.__lt__"></a>

**`__lt__(other T) bool`**

Return true if `self` is less than `other` (`self < other`).

<a id="comparable.__le__"></a>

**`__le__(other T) bool`**

Return true if `self` is less than or equal to `other` (`self <= other`).

<a id="comparable.__gt__"></a>

**`__gt__(other T) bool`**

Return true if `self` is greater than `other` (`self > other`).

<a id="comparable.__ge__"></a>

**`__ge__(other T) bool`**

Return true if `self` is greater than or equal to `other` (`self >= other`).

<a id="hashable"></a>

### `Hashable`

```kl
pub trait Hashable {
    func __hash__() int
}
```

`Hashable` defines the ability for a type to produce a stable hash value.
Conforming to `Hashable` allows a type to be used as a key in dictionaries,
sets, and other hash-based collections.

Equal values must always produce identical hash values. The hash should
be well-distributed to ensure good performance in hashed structures.

---

<a id="hashable.__hash__"></a>

**`__hash__() int`**

Return a stable hash value for `self`.

Equal values must always produce identical hash values.

<a id="printable"></a>

### `Printable`

```kl
pub trait Printable {
    func __str__() str
}
```

`Printable` defines a human-readable string representation for a type,
used by [`print`](#print) and other display mechanisms. It is a display contract,
never a content conversion (e.g. `bytes` displays its hexadecimal form,
while decoding into a `str` uses [`bytes.to_str`](#bytes.to_str)).

The returned string should be suitable for display to end users and
represent the value in a clear, readable form.

---

<a id="printable.__str__"></a>

**`__str__() str`**

Return a human-readable string representation of `self`.

<a id="sequence"></a>

### `Sequence`

```kl
pub trait Sequence[T] : Iterable[T] {
    func __len__() int
    func __contains__(item T) bool
    func __getitem__(index int) T
    func empty() bool
    func first() T
    func last() T
    func at(index int) T
    func index(value T, start = 0, end = -1) int
    func rindex(value T, start = 0, end = -1) int
    func count(value T, start = 0, end = -1) int
}
```
**Supertraits:** [`Iterable[T]`](#iterable)


A Sequence provides read-only indexed access to a collection of elements.

This trait is the standard internal type for variadic arguments (`...T`).
It provides random access and searching capabilities without modification.

---

<a id="sequence.__len__"></a>

**`__len__() int`**

Return the number of items in the sequence.

<a id="sequence.__contains__"></a>

**`__contains__(item T) bool`**

Return true if the sequence contains the specified value `item`.

<a id="sequence.__getitem__"></a>

**`__getitem__(index int) T`**

Return the item at the specified `index`.

Errors

Panic if `index` is negative or is not less than the sequence length.

<a id="sequence.empty"></a>

**`empty() bool`**

Returns whether the sequence contains no elements.

O(1) or O(n) depending on implementation.

Examples

Given a sequence `s`:

```kl
if s.empty() {
    print("The sequence is empty.")
}
```

<a id="sequence.first"></a>

**`first() T`**

Returns the first element. The same as `at(0)`.

Errors

Panic if the sequence is empty.

<a id="sequence.last"></a>

**`last() T`**

Returns the last element of the sequence.

Errors

Panic if the sequence is empty.

<a id="sequence.at"></a>

**`at(index int) T`**

Returns the element at the specified `index`.

Errors

Panic if `index` is negative or is not less than the sequence length.

<a id="sequence.index"></a>

**`index(value T, start = 0, end = -1) int`**

Return the first index of `value` in `[start, end)`.

If `end` is `-1`, the search extends to the end of the sequence.
Return `-1` if `value` is not present.

<a id="sequence.rindex"></a>

**`rindex(value T, start = 0, end = -1) int`**

Return the last index of `value` in `[start, end)`.

If `end` is `-1`, the search extends to the end of the sequence.
Return `-1` if `value` is not present.

<a id="sequence.count"></a>

**`count(value T, start = 0, end = -1) int`**

Return the number of occurrences of `value` in `[start, end)`.

If `end` is `-1`, the search extends to the end of the sequence.

<a id="mutablesequence"></a>

### `MutableSequence`

```kl
pub trait MutableSequence[T] : Sequence[T] {
    func __setitem__(index int, value T)
    func put(index int, value T)
    func push(value T)
    func pop(index = -1) T
    func extend(items Iterable[T])
    func insert(index int, value T)
    func remove(value T)
    func swap(index1 int, index2 int)
    func clear()
}
```
**Supertraits:** [`Sequence[T]`](#sequence)


A MutableSequence is an ordered collection that supports modification.

It extends the Sequence trait with operations for updating, inserting,
and removing elements.

---

<a id="mutablesequence.__setitem__"></a>

**`__setitem__(index int, value T)`**

Set the item at the specified `index` to `value`.

Errors

Panic if `index` is negative or is not less than the sequence length.

<a id="mutablesequence.put"></a>

**`put(index int, value T)`**

Sets the element at the specified `index` to `value`.

Errors

Panic if `index` is negative or is not less than the sequence length.

<a id="mutablesequence.push"></a>

**`push(value T)`**

Append `value` to the end of the sequence.

<a id="mutablesequence.pop"></a>

**`pop(index = -1) T`**

Remove and return the item at the specified `index`.

If `index` is omitted, remove and return the last item.

<a id="mutablesequence.extend"></a>

**`extend(items Iterable[T])`**

Append all elements from `items` to the end of the sequence.

<a id="mutablesequence.insert"></a>

**`insert(index int, value T)`**

Insert `value` at the specified `index`.

Elements at or after `index` are shifted to the right.

<a id="mutablesequence.remove"></a>

**`remove(value T)`**

Remove the first occurrence of `value` from the sequence.

Errors

Panic if `value` is not present.

<a id="mutablesequence.swap"></a>

**`swap(index1 int, index2 int)`**

Swap the elements at the specified `index1` and `index2`.

Errors

Panic if either index is out of bounds.

<a id="mutablesequence.clear"></a>

**`clear()`**

Remove all items from the sequence, leaving it empty.

<a id="map"></a>

### `Map`

```kl
pub trait Map[K, V] : Iterable[(K, V)] {
    func __len__() int
    func __contains__(key K) bool
    func __getitem__(key K) V
    func __setitem__(key K, value V)
    func keys() Sequence[K]
    func values() Sequence[V]
    func items() Sequence[(K, V)]
    func clear()
    func remove(key K) V
    func remove_or(key K, default_value V) V
    func get(key K) V?
    func get_or(key K, default_value V) V
}
```
**Supertraits:** [`Iterable[(K, V)]`](#iterable)


A Map is a collection of key-value mappings.

A map cannot contain duplicate keys; each key maps to at most one value.

---

<a id="map.__len__"></a>

**`__len__() int`**

Return the number of mappings in the map.

<a id="map.__contains__"></a>

**`__contains__(key K) bool`**

Return true if the map contains a mapping for `key`.

<a id="map.__getitem__"></a>

**`__getitem__(key K) V`**

Return the value associated with `key`.

Errors

Panic if `key` is not present.

<a id="map.__setitem__"></a>

**`__setitem__(key K, value V)`**

Associate `value` with `key`.

If `key` already exists, its associated value is replaced.

<a id="map.keys"></a>

**`keys() Sequence[K]`**

Return a sequence containing all keys in the map.

<a id="map.values"></a>

**`values() Sequence[V]`**

Return a sequence containing all values in the map.

<a id="map.items"></a>

**`items() Sequence[(K, V)]`**

Return a sequence containing all key-value pairs in the map.

<a id="map.clear"></a>

**`clear()`**

Remove all mappings from the map, leaving it empty.

<a id="map.remove"></a>

**`remove(key K) V`**

Remove the mapping for `key` and return its value.

Errors

Panic if `key` is not present.

<a id="map.remove_or"></a>

**`remove_or(key K, default_value V) V`**

Remove the mapping for `key` and return its value.

If `key` is not present, return `default_value`.

<a id="map.get"></a>

**`get(key K) V?`**

Return the value associated with `key`, or `nil` if `key` is not present.

Unlike indexing with `map[key]`, this method does not panic when
`key` is not present and is intended for use with `if let`.

<a id="map.get_or"></a>

**`get_or(key K, default_value V) V`**

Return the value associated with `key`, or `default_value` if `key` is
not present.

<a id="set"></a>

### `Set`

```kl
pub trait Set[T] : Iterable[T] {
    func __len__() int
    func __contains__(value T) bool
    func add(value T) bool
    func update(items Iterable[T])
    func remove(value T) bool
    func clear()
    func is_subset(other Iterable[T]) bool
    func is_superset(other Iterable[T]) bool
    func union(other Iterable[T]) Set[T]
    func intersect(other Iterable[T]) Set[T]
    func diff(other Iterable[T]) Set[T]
}
```
**Supertraits:** [`Iterable[T]`](#iterable)


A Set is a collection that contains no duplicate elements.

It models the mathematical set abstraction.

---

<a id="set.__len__"></a>

**`__len__() int`**

Return the number of elements in the set.

<a id="set.__contains__"></a>

**`__contains__(value T) bool`**

Return true if the set contains `value`.

<a id="set.add"></a>

**`add(value T) bool`**

Add `value` to the set.

Return true if `value` was not already present.

<a id="set.update"></a>

**`update(items Iterable[T])`**

Add all elements from `items` to the set.

<a id="set.remove"></a>

**`remove(value T) bool`**

Remove `value` from the set.

Return true if `value` was present.

<a id="set.clear"></a>

**`clear()`**

Remove all elements from the set.

<a id="set.is_subset"></a>

**`is_subset(other Iterable[T]) bool`**

Return true if every element in the set is contained in `other`.

<a id="set.is_superset"></a>

**`is_superset(other Iterable[T]) bool`**

Return true if every element in `other` is contained in the set.

<a id="set.union"></a>

**`union(other Iterable[T]) Set[T]`**

Return a new set containing the elements from the set and `other`.

<a id="set.intersect"></a>

**`intersect(other Iterable[T]) Set[T]`**

Return a new set containing the elements common to the set and `other`.

<a id="set.diff"></a>

**`diff(other Iterable[T]) Set[T]`**

Return a new set containing the elements in the set but not in `other`.

<a id="iterator"></a>

### `Iterator`

```kl
pub trait Iterator[T] {
    func has_next() bool
    func next_strict() T
    func next() T?
}
```

An Iterator represents the state of an ongoing traversal of elements.

It provides two complementary access patterns:
- Optional-result iteration via [`Iterator.next`](#iterator.next).
- Check-then-act iteration via [`Iterator.has_next`](#iterator.has_next)
  and [`Iterator.next_strict`](#iterator.next_strict).

The iterator never rewinds and never allocates; it is a forward-only traversal state machine.

---

<a id="iterator.has_next"></a>

**`has_next() bool`**

Returns whether another element is available.

Check this before calling
[`Iterator.next_strict`](#iterator.next_strict).

<a id="iterator.next_strict"></a>

**`next_strict() T`**

Returns the next element and advances the iterator.

Check `has_next()` before calling this method.
The returned value may be `nil` when `T` is nullable.

Errors

Panic if the iterator is exhausted.

Examples

Given an iterator `it`:

```kl
while it.has_next() {
    print(it.next_strict())
}
```

<a id="iterator.next"></a>

**`next() T?`**

Returns the next element and advances the iterator, or returns
`nil` if the iterator is exhausted.

Exhaustion is represented by `nil`, not a panic.
When `T` is nullable, an element may also be `nil`. Use
[`Iterator.has_next`](#iterator.has_next) together with
[`Iterator.next_strict`](#iterator.next_strict) to distinguish
such elements from exhaustion.

Examples

Given an iterator `it` whose element type is not nullable:

```kl
while let value = it.next() {
    print(value)
}
```

<a id="iterable"></a>

### `Iterable`

```kl
pub trait Iterable[T] {
    func iter(step = 1) Iterator[T]
}
```

The Iterable trait is the basis for all objects that can be iterated over.

It provides a way to create an iterator for traversing elements.

---

<a id="iterable.iter"></a>

**`iter(step = 1) Iterator[T]`**

Return a new iterator for traversing the elements.

`step` specifies the number of elements to advance on each iteration.

<a id="arithmetic"></a>

### `Arithmetic`

```kl
pub trait Arithmetic[T] {
    func __add__(rhs T) T
    func __sub__(rhs T) T
    func __mul__(rhs T) T
    func __div__(rhs T) T
    func __mod__(rhs T) T
    func __neg__() T
}
```

Trait for basic arithmetic operations.

Provides the fundamental binary arithmetic operators (`+`, `-`, `*`, `/`, `%`)
shared by all numeric types.

---

<a id="arithmetic.__add__"></a>

**`__add__(rhs T) T`**

Returns the sum of `self` and `rhs` (`self + rhs`).

<a id="arithmetic.__sub__"></a>

**`__sub__(rhs T) T`**

Returns the difference of `self` and `rhs` (`self - rhs`).

<a id="arithmetic.__mul__"></a>

**`__mul__(rhs T) T`**

Returns the product of `self` and `rhs` (`self * rhs`).

<a id="arithmetic.__div__"></a>

**`__div__(rhs T) T`**

Returns the quotient of `self` and `rhs` (`self / rhs`).

<a id="arithmetic.__mod__"></a>

**`__mod__(rhs T) T`**

Returns the remainder of `self` divided by `rhs` (`self % rhs`).

<a id="arithmetic.__neg__"></a>

**`__neg__() T`**

Returns the negation of `self` (`-self`).

<a id="bitwise"></a>

### `Bitwise`

```kl
pub trait Bitwise[T] {
    func __shl__(rhs T) T
    func __shr__(rhs T) T
    func __bitand__(rhs T) T
    func __bitor__(rhs T) T
    func __bitxor__(rhs T) T
    func __bitnot__() T
}
```

Trait for bitwise operations.

Provides shift and bitwise logical operators. Only integer types implement
this trait; floating-point types do not support bitwise operations.

---

<a id="bitwise.__shl__"></a>

**`__shl__(rhs T) T`**

Left shift (`self << rhs`).

<a id="bitwise.__shr__"></a>

**`__shr__(rhs T) T`**

Right shift (`self >> rhs`).

<a id="bitwise.__bitand__"></a>

**`__bitand__(rhs T) T`**

Bitwise AND (`self & rhs`).

<a id="bitwise.__bitor__"></a>

**`__bitor__(rhs T) T`**

Bitwise OR (`self | rhs`).

<a id="bitwise.__bitxor__"></a>

**`__bitxor__(rhs T) T`**

Bitwise XOR (`self ^ rhs`).

<a id="bitwise.__bitnot__"></a>

**`__bitnot__() T`**

Bitwise NOT (`~self`).

## Classes

<a id="bool"></a>

### `bool`

```kl
pub class bool {
    pub func __and__(other bool) bool
    pub func __or__(other bool) bool
    pub func __not__() bool
    pub func __eq__(other bool) bool
    pub func __ne__(other bool) bool
    pub func __hash__() int
    pub func __str__() str
}
```

Represents a logical value that can be either true or false.
Boolean types are typically used in conditional statements and logical operations.

---

<a id="bool.__and__"></a>

**`__and__(other bool) bool`** — *`@intrinsic`*

Return the logical AND of `self` and `other` (`self && other`).

<a id="bool.__or__"></a>

**`__or__(other bool) bool`** — *`@intrinsic`*

Return the logical OR of `self` and `other` (`self || other`).

<a id="bool.__not__"></a>

**`__not__() bool`** — *`@intrinsic`*

Return the logical negation of `self` (`!self`).

<a id="bool.__eq__"></a>

**`__eq__(other bool) bool`** — *`@intrinsic`*

Return true if `self` is equal to `other` (`self == other`).

<a id="bool.__ne__"></a>

**`__ne__(other bool) bool`** — *`@intrinsic`*

Return true if `self` is not equal to `other` (`self != other`).

<a id="bool.__hash__"></a>

**`__hash__() int`** — *`@native`*

Return a hash value for `self`.

<a id="bool.__str__"></a>

**`__str__() str`** — *`@native`*

Return the string representation of the boolean.
Returns "true" if true, and "false" if false.

<a id="bytebuf"></a>

### `ByteBuf`

```kl
pub class ByteBuf {
    pub func __init__(capacity = 256)
    pub func push(b uint8)
    pub func push_str(s str)
    pub func push_bytes(b bytes)
    pub func fill(value uint8, start = 0, end = -1)
    pub func zero(start = 0, end = -1)
    pub func to_str() str
    pub func to_bytes() bytes
    pub func __len__() int
    pub func empty() bool
    pub func clear()
}
```

ByteBuf is a mutable, growable byte buffer for general-purpose buffered I/O.

It provides efficient append operations and can be converted to `str` or `bytes`.
Suitable for building formatted output, protocol serialization (e.g., HTTP),
network message assembly, and other scenarios requiring incremental byte accumulation.

Bytes are appended at the end. [`ByteBuf.fill`](#bytebuf.fill) and
[`ByteBuf.zero`](#bytebuf.zero) overwrite existing ranges;
[`ByteBuf.clear`](#bytebuf.clear) resets the buffer's length to zero.
Individual-element indexing, insertion, and removal are not supported.

Examples

```kl
let buffer = ByteBuf()
buffer.push_str("Hello, ")
buffer.push_str("Koala!")
print(buffer.to_str())
```

---

<a id="bytebuf.__init__"></a>

**`__init__(capacity = 256)`** — *`@native`*

Create an empty buffer with an optional initial capacity.

The buffer starts with zero length. `capacity` specifies the initial
allocation size; subsequent writes will automatically grow the internal
storage when needed.

<a id="bytebuf.push"></a>

**`push(b uint8)`** — *`@native`*

Append a single byte to the end of the buffer.

If the internal storage is full, the buffer will automatically grow
(typically doubling its capacity).

<a id="bytebuf.push_str"></a>

**`push_str(s str)`** — *`@native`*

Append the UTF-8 encoded bytes of a string to the buffer.

The string is encoded as UTF-8 and its raw bytes are appended to
the end of the buffer.

<a id="bytebuf.push_bytes"></a>

**`push_bytes(b bytes)`** — *`@native`*

Append raw bytes to the end of the buffer.

The entire content of `b` is copied into the buffer.

<a id="bytebuf.fill"></a>

**`fill(value uint8, start = 0, end = -1)`** — *`@native`*

Fill the bytes in `[start, end)` with `value`.
If `end` is `-1`, the fill extends to the end of the buffer.

<a id="bytebuf.zero"></a>

**`zero(start = 0, end = -1)`** — *`@native`*

Set the bytes in `[start, end)` to zero.
If `end` is `-1`, the zeroing extends to the end of the buffer.

<a id="bytebuf.to_str"></a>

**`to_str() str`** — *`@native`*

Decode the buffered content as UTF-8 and return a new `str`.

Errors

Panic if the buffered bytes are not valid UTF-8.

<a id="bytebuf.to_bytes"></a>

**`to_bytes() bytes`** — *`@native`*

Return a fixed-size bytes snapshot of the buffered content.

Returns a view of the buffered data without copying.

<a id="bytebuf.__len__"></a>

**`__len__() int`** — *`@native`*

Return the current number of bytes in the buffer. O(1).

<a id="bytebuf.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the buffer contains no bytes. O(1).

<a id="bytebuf.clear"></a>

**`clear()`** — *`@native`*

Clear the buffer, resetting its length to zero.

The internal storage is retained for reuse; capacity is not reduced.

<a id="bytes"></a>

### `bytes`

```kl
pub class bytes : Sequence[uint8] {
    pub func __init__(size int)
    pub func iter(step = 1) BytesIter { return BytesIter(self, step) }
    pub func reversed(step = 1) BytesRevIter { return BytesRevIter(self, step) }
    pub func __len__() int
    pub func empty() bool
    pub func __getitem__(index int) uint8
    pub func __getslice__(r slice) bytes
    pub func __contains__(value uint8) bool
    pub func index(value uint8, start = 0, end = -1) int
    pub func rindex(value uint8, start = 0, end = -1) int
    pub func count(value uint8, start = 0, end = -1) int
    pub func __setitem__(index int, value uint8)
    pub func __setslice__(r slice, val bytes)
    pub func at(index int) uint8
    pub func put(index int, value uint8)
    pub func find_bytes(sub bytes, start = 0, end = -1) int
    pub func rfind_bytes(sub bytes, start = 0, end = -1) int
    pub func copy(src bytes, start = 0, end = -1) int
    pub func copy_str(s str, start = 0, end = -1) int
    pub func fill(value uint8, start = 0, end = -1)
    pub func zero(start = 0, end = -1)
    pub func view(start = 0, end = -1) bytes
    pub func startswith(prefix bytes) bool
    pub func endswith(suffix bytes) bool
    pub func replace(old bytes, new bytes) bytes
    pub func split(sep bytes, maxsplit = -1) list[bytes]
    pub func rsplit(sep bytes, maxsplit = -1) list[bytes]
    pub func splitlines() list[bytes]
    pub func strip() bytes
    pub func lstrip() bytes
    pub func rstrip() bytes
    pub func to_str() str
    pub func __str__() str
}
```
**Bases:** [`Sequence[uint8]`](#sequence)


A bytes is a fixed-size, readable and writable byte array.

It implements Sequence, providing random access, searching, slicing
and sub-sequence matching.
The size is determined at construction and never changes.

Examples

```kl
let data = bytes(3)
data[0] = uint8(65)
print(data[0])
```

---

<a id="bytes.__init__"></a>

**`__init__(size int)`** — *`@native`*

Create a byte array of the given `size`, initialized to zero.

<a id="bytes.iter"></a>

**`iter(step = 1) BytesIter { return BytesIter(self, step) }`**

Return an iterator over the bytes.

The iterator traverses the byte array from left to right, returning
one byte per step. The optional `step` argument controls
the stride of iteration:

- `step = 1` produces a normal byte-by-byte traversal.
- `step > 1` skips bytes, yielding a strided iteration.

The iterator is stateful and does not allocate; it reads directly
from the underlying byte array.

<a id="bytes.reversed"></a>

**`reversed(step = 1) BytesRevIter { return BytesRevIter(self, step) }`**

Return a reverse iterator over the bytes.

The iterator traverses the byte array from right to left, returning
one byte per step. The optional `step` argument controls
the stride of reverse iteration:

- `step = 1` produces a normal reverse traversal.
- `step > 1` skips bytes in reverse order.

The iterator is stateful and does not allocate; it reads directly
from the underlying byte array.

<a id="bytes.__len__"></a>

**`__len__() int`** — *`@native`*

Return the size of the byte array. O(1).

<a id="bytes.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the byte array is empty, false otherwise. O(1).

<a id="bytes.__getitem__"></a>

**`__getitem__(index int) uint8`** — *`@intrinsic`*

Return the byte at the specified index. O(1).

Errors

Panic if the index is out of range.

<a id="bytes.__getslice__"></a>

**`__getslice__(r slice) bytes`** — *`@intrinsic`*

Return a copy of the portion of the byte array defined by the slice.

<a id="bytes.__contains__"></a>

**`__contains__(value uint8) bool`** — *`@native`*

Return true if the byte array contains the byte `value`.

<a id="bytes.index"></a>

**`index(value uint8, start = 0, end = -1) int`** — *`@native`*

Return the first index of `value` in `[start, end)`. O(n).
If `end` is `-1`, the search extends to the end of the byte array.
Return `-1` if the value is not present.

<a id="bytes.rindex"></a>

**`rindex(value uint8, start = 0, end = -1) int`** — *`@native`*

Return the last index of `value` in `[start, end)`. O(n).
If `end` is `-1`, the search extends to the end of the byte array.
Return `-1` if the value is not present.

<a id="bytes.count"></a>

**`count(value uint8, start = 0, end = -1) int`** — *`@native`*

Return the number of occurrences of `value` in `[start, end)`. O(n).
If `end` is `-1`, the search extends to the end of the byte array.

<a id="bytes.__setitem__"></a>

**`__setitem__(index int, value uint8)`** — *`@intrinsic`*

Set the byte at the specified index to `value`. O(1).

Errors

Panic if the index is out of range.

<a id="bytes.__setslice__"></a>

**`__setslice__(r slice, val bytes)`** — *`@intrinsic`*

Replace a slice of the byte array with the bytes from `val`.
The length of the byte array never changes.

<a id="bytes.at"></a>

**`at(index int) uint8`** — *`@intrinsic`*

Return the byte at the specified `index`.

Errors

Panic if the index is out of range.

<a id="bytes.put"></a>

**`put(index int, value uint8)`** — *`@intrinsic`*

Set the byte at the specified `index` to `value`.

Errors

Panic if the index is out of range.

<a id="bytes.find_bytes"></a>

**`find_bytes(sub bytes, start = 0, end = -1) int`** — *`@native`*

Return the first index of the sub-sequence `sub` in `[start, end)`. O(n).
If `end` is `-1`, the search extends to the end of the byte array.
Return `-1` if `sub` is not present.

<a id="bytes.rfind_bytes"></a>

**`rfind_bytes(sub bytes, start = 0, end = -1) int`** — *`@native`*

Return the last index of the sub-sequence `sub` in `[start, end)`. O(n).
If `end` is `-1`, the search extends to the end of the byte array.
Return `-1` if `sub` is not present.

<a id="bytes.copy"></a>

**`copy(src bytes, start = 0, end = -1) int`** — *`@native`*

Copy the bytes of `src` in `[start, end)` into `self` from the beginning.

This is a raw memory copy operation, similar to C's memcpy.
The destination is always the beginning of `self`; if you need to copy
into a specific offset, use `self.view(offset).copy(src)` instead.

Arguments

- `src`: The source bytes to copy from.
- `start`: The start index in `src` (inclusive). Defaults to 0.
- `end`: The end index in `src` (exclusive). Defaults to -1 (end of src).

Returns

The number of bytes copied.
Returns `-1` if the slice length exceeds `self`'s capacity.

<a id="bytes.copy_str"></a>

**`copy_str(s str, start = 0, end = -1) int`** — *`@native`*

Copy the UTF-8 encoding of `s` in `[start, end)` into `self` from the beginning.

`start` and `end` select text in `s` before UTF-8 encoding.
The destination is always the beginning of `self`; if you need to copy
into a specific offset, use `self.view(offset).copy_str(s)` instead.

Arguments

- `s`: The source string to encode and copy from.
- `start`: The start rune index in `s` (inclusive). Defaults to 0.
- `end`: The end rune index in `s` (exclusive). Defaults to -1 (end of s).

Returns

The number of bytes copied.
Returns `-1` if the UTF-8 byte length exceeds `self`'s capacity.

<a id="bytes.fill"></a>

**`fill(value uint8, start = 0, end = -1)`** — *`@native`*

Fill the bytes in `[start, end)` with `value`.
If `end` is `-1`, the fill extends to the end of the byte array.

<a id="bytes.zero"></a>

**`zero(start = 0, end = -1)`** — *`@native`*

Set the bytes in `[start, end)` to zero.
If `end` is `-1`, the zeroing extends to the end of the byte array.

<a id="bytes.view"></a>

**`view(start = 0, end = -1) bytes`** — *`@native`*

Return a view of the bytes in `[start, end)`, sharing the same storage.
If `end` is `-1`, the view extends to the end of the byte array.

<a id="bytes.startswith"></a>

**`startswith(prefix bytes) bool`** — *`@native`*

Return true if the byte array starts with `prefix`.

<a id="bytes.endswith"></a>

**`endswith(suffix bytes) bool`** — *`@native`*

Return true if the byte array ends with `suffix`.

<a id="bytes.replace"></a>

**`replace(old bytes, new bytes) bytes`** — *`@native`*

Replace all occurrences of `old` with `new`, returning a new byte array.

<a id="bytes.split"></a>

**`split(sep bytes, maxsplit = -1) list[bytes]`** — *`@native`*

Split the bytes by the separator `sep` and return the parts as a list.
At most `maxsplit` splits are done; if `maxsplit` is -1, there is no limit.

<a id="bytes.rsplit"></a>

**`rsplit(sep bytes, maxsplit = -1) list[bytes]`** — *`@native`*

Split the bytes by the separator `sep` from the right and return
the parts as a list. At most `maxsplit` splits are done;
if `maxsplit` is -1, there is no limit.

<a id="bytes.splitlines"></a>

**`splitlines() list[bytes]`** — *`@native`*

Split the bytes at line boundaries and return the lines as a list.
The line break characters are not included.

<a id="bytes.strip"></a>

**`strip() bytes`** — *`@native`*

Remove leading and trailing whitespace bytes (0x20, \t, \n, \r).

<a id="bytes.lstrip"></a>

**`lstrip() bytes`** — *`@native`*

Remove leading whitespace bytes.

<a id="bytes.rstrip"></a>

**`rstrip() bytes`** — *`@native`*

Remove trailing whitespace bytes.

<a id="bytes.to_str"></a>

**`to_str() str`** — *`@native`*

Decode the bytes as UTF-8 and return a new `str`.

Errors

Panic if the bytes are not valid UTF-8.

<a id="bytes.__str__"></a>

**`__str__() str`** — *`@native`*

Return the hexadecimal string representation of the bytes.

<a id="bytesiter"></a>

### `BytesIter`

```kl
pub class BytesIter : Iterator[uint8] {
    let step int
    let source bytes
    let length int
    var cursor int
    pub func __init__(b bytes, _step = 1)
    pub func has_next() bool
    pub func next_strict() uint8
    pub func next() uint8?
    pub func __str__() str
}
```
**Bases:** [`Iterator[uint8]`](#iterator)


An iterator over the bytes of a byte array, from left to right.

`BytesIter` provides sequential access to each byte in the array.

The iterator is stateful: it maintains an internal cursor that
advances by `step` on each call to `next()`. A `step` greater than 1
produces a strided iteration.

Once the cursor reaches or exceeds the byte array length, `has_next()`
returns false. `next()` returns `nil`, while `next_strict()` triggers a panic.

Notes

- Bytes are returned as `uint8`.
- The iterator does not allocate; it reads directly from the source bytes.
- Iterators are not resettable; create a new iterator to iterate again.
- Strided iteration is useful for sampling, scanning, and simple parsing.

---

**Fields**

<a id="bytesiter.step"></a>

**`let step int`**

<a id="bytesiter.source"></a>

**`let source bytes`**

<a id="bytesiter.length"></a>

**`let length int`**

<a id="bytesiter.cursor"></a>

**`var cursor int`**

<a id="bytesiter.__init__"></a>

**`__init__(b bytes, _step = 1)`**

Initialize a forward iterator over `b` with an optional stride `_step`.

The cursor starts at index 0 and advances by `step` on each iteration.

<a id="bytesiter.has_next"></a>

**`has_next() bool`**

Return true if there are remaining bytes to iterate.

<a id="bytesiter.next_strict"></a>

**`next_strict() uint8`**

Return the next byte and advance the cursor by `step`.

Errors

Panic if the iterator is exhausted.

<a id="bytesiter.next"></a>

**`next() uint8?`**

Return the next byte, or `nil` if exhausted.

<a id="bytesiter.__str__"></a>

**`__str__() str`**

Return a human-readable description of the iterator state.

Useful for debugging and logging.

<a id="bytesreviter"></a>

### `BytesRevIter`

```kl
pub class BytesRevIter : Iterator[uint8] {
    let step int
    let source bytes
    let length int
    var cursor int
    pub func __init__(b bytes, _step = 1)
    pub func has_next() bool
    pub func next_strict() uint8
    pub func next() uint8?
    pub func __str__() str
}
```
**Bases:** [`Iterator[uint8]`](#iterator)


An iterator over the bytes of a byte array, from right to left.

`BytesRevIter` provides reverse sequential access to each byte in the array.

The iterator is stateful: it maintains an internal cursor that
moves backward by `step` on each call to `next()`. A `step` greater
than 1 produces a strided reverse iteration.

Once the cursor moves before the beginning of the byte array, `has_next()`
returns false. `next()` returns `nil`, while `next_strict()` triggers a panic.

Notes

- Bytes are returned as `uint8`.
- The iterator does not allocate; it reads directly from the source bytes.
- Iterators are not resettable; create a new iterator to iterate again.

---

**Fields**

<a id="bytesreviter.step"></a>

**`let step int`**

<a id="bytesreviter.source"></a>

**`let source bytes`**

<a id="bytesreviter.length"></a>

**`let length int`**

<a id="bytesreviter.cursor"></a>

**`var cursor int`**

<a id="bytesreviter.__init__"></a>

**`__init__(b bytes, _step = 1)`**

Initialize a reverse iterator over `b` with an optional stride `_step`.

The cursor starts at the last byte and moves backward by `step`.

<a id="bytesreviter.has_next"></a>

**`has_next() bool`**

Return true if there are remaining bytes to iterate.

<a id="bytesreviter.next_strict"></a>

**`next_strict() uint8`**

Return the next byte and move the cursor backward by `step`.

Errors

Panic if the iterator is exhausted.

<a id="bytesreviter.next"></a>

**`next() uint8?`**

Return the next byte in reverse order, or `nil` if exhausted.

<a id="bytesreviter.__str__"></a>

**`__str__() str`**

Return a human-readable description of the iterator state.

Useful for debugging and logging.

<a id="dict"></a>

### `dict`

```kl
pub class dict[K, V] : Map[K, V] {
    pub func __init__()
    pub static func fromkeys(keys Iterable[K], value V) dict[K, V]
    pub func __iter__() Iterator[(K, V)]
    pub func __len__() int
    pub func empty() bool
    pub func __contains__(key K) bool
    pub func __getitem__(key K) V
    pub func __setitem__(key K, value V)
    pub func keys() list[K]
    pub func values() list[V]
    pub func items() list[(K, V)]
    pub func clear()
    pub func remove(key K) V
    pub func remove_or(key K, default_value V) V
    pub func get(key K) V?
    pub func get_or(key K, default_value V) V
    pub func __str__() str
    pub func update(src Iterable[(K, V)])
    pub func setdefault(key K, default_value V) V
    pub func peek_first() (K, V)
    pub func peek_last() (K, V)
    pub func pop_first() (K, V)
    pub func pop_last() (K, V)
    pub func __or__(src Iterable[(K, V)]) dict[K, V]
    pub func __ior__(src Iterable[(K, V)]) dict[K, V]
    pub func reversed() Iterator[(K, V)]
}
```
**Bases:** [`Map[K, V]`](#map)


A dict is a mutable, insertion-ordered hash map that maps unique keys
to values.

It implements `Map`, providing key lookup, insertion, removal,
and iteration over key-value pairs. Iteration, `keys()`, `values()` and
`items()` all follow insertion order.

---

<a id="dict.__init__"></a>

**`__init__()`** — *`@native`*

Create an empty dict.

<a id="dict.fromkeys"></a>

**`static fromkeys(keys Iterable[K], value V) dict[K, V]`** — *`@native`*

Create a new dict with keys from `keys` and all values set to `value`.

<a id="dict.__iter__"></a>

**`__iter__() Iterator[(K, V)]`** — *`@native`*

Return an iterator over the (key, value) pairs in insertion order.

<a id="dict.__len__"></a>

**`__len__() int`** — *`@native`*

Return the number of key-value pairs. O(1).

<a id="dict.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the dict is empty, false otherwise. O(1).

<a id="dict.__contains__"></a>

**`__contains__(key K) bool`** — *`@native`*

Return true if the dict contains a mapping for `key`.

<a id="dict.__getitem__"></a>

**`__getitem__(key K) V`** — *`@native`*

Return the value mapped to `key`. O(1).

Errors

Panic if `key` is not present.

<a id="dict.__setitem__"></a>

**`__setitem__(key K, value V)`** — *`@native`*

Associate `value` with `key`, replacing any existing mapping.

<a id="dict.keys"></a>

**`keys() list[K]`** — *`@native`*

Return a list of all keys in the dict, in insertion order.

<a id="dict.values"></a>

**`values() list[V]`** — *`@native`*

Return a list of all values in the dict, in insertion order.

<a id="dict.items"></a>

**`items() list[(K, V)]`** — *`@native`*

Return a list of all (key, value) pairs in the dict, in insertion order.

<a id="dict.clear"></a>

**`clear()`** — *`@native`*

Remove all mappings from the dict, leaving it empty.

<a id="dict.remove"></a>

**`remove(key K) V`** — *`@native`*

Remove the mapping for `key` and return its value. O(1).

Errors

Panic if `key` is not present.

<a id="dict.remove_or"></a>

**`remove_or(key K, default_value V) V`** — *`@native`*

Remove the mapping for `key` and return its value. O(1).
If the key is not found, return `default_value`.

<a id="dict.get"></a>

**`get(key K) V?`** — *`@native`*

Return the value for `key` if present, else `nil`. O(1).

Missing keys return `nil` instead of panicking; use `if let`
to unwrap the result.

<a id="dict.get_or"></a>

**`get_or(key K, default_value V) V`** — *`@native`*

Return the value for `key` if present, else `default_value`. O(1).

<a id="dict.__str__"></a>

**`__str__() str`** — *`@native`*

Return a string representation of the dict.
Uses the form `{key1: value1, key2: value2}`.

<a id="dict.update"></a>

**`update(src Iterable[(K, V)])`** — *`@native`*

Merge all (key, value) pairs from `src` into the dict,
replacing the values of existing keys.

<a id="dict.setdefault"></a>

**`setdefault(key K, default_value V) V`** — *`@native`*

Return the value for `key` if it is in the dict.
Otherwise insert `key` with `default_value` and return `default_value`.

<a id="dict.peek_first"></a>

**`peek_first() (K, V)`** — *`@native`*

Return the first inserted (key, value) pair without removing it.

Errors

Panic if the dict is empty.

<a id="dict.peek_last"></a>

**`peek_last() (K, V)`** — *`@native`*

Return the last inserted (key, value) pair without removing it.

Errors

Panic if the dict is empty.

<a id="dict.pop_first"></a>

**`pop_first() (K, V)`** — *`@native`*

Remove and return the first inserted (key, value) pair.

Errors

Panic if the dict is empty.

<a id="dict.pop_last"></a>

**`pop_last() (K, V)`** — *`@native`*

Remove and return the last inserted (key, value) pair.

Errors

Panic if the dict is empty.

<a id="dict.__or__"></a>

**`__or__(src Iterable[(K, V)]) dict[K, V]`** — *`@native`*

Return a new dict with mappings of `self` followed by `src` (`self | src`).
Values from `src` replace those of duplicate keys.

<a id="dict.__ior__"></a>

**`__ior__(src Iterable[(K, V)]) dict[K, V]`** — *`@native`*

Merge all (key, value) pairs from `src` into the dict in place,
replacing the values of existing keys (`self |= src`).

<a id="dict.reversed"></a>

**`reversed() Iterator[(K, V)]`** — *`@native`*

Return a reverse iterator over the (key, value) pairs
in reverse insertion order.

<a id="exception"></a>

### `Exception`

```kl
pub class Exception {
    func __init__()
}
```

An exception object represents a raised exception.
Once raised, it cannot be caught and must be resolved.

Use [`panic`](#panic) to raise an exception; it is the only supported way.

---

<a id="exception.__init__"></a>

**`__init__()`**

Users should not directly instantiate this class.
Use [`panic`](#panic) to raise an exception.

<a id="list"></a>

### `list`

```kl
pub class list[T] : MutableSequence[T] {
    pub func __init__(args ...T)
    pub func __iter__() Iterator[T]
    pub func __len__() int
    pub func empty() bool
    pub func __getitem__(index int) T
    pub func __getslice__(r slice) list[T]
    pub func __contains__(v T) bool
    pub func index(value T, start = 0, end = -1) int
    pub func rindex(value T, start = 0, end = -1) int
    pub func count(value T, start = 0, end = -1) int
    pub func __setitem__(index int, value T)
    pub func __setslice__(r slice, val Iterable[T])
    pub func push(value T)
    pub func extend(items Iterable[T])
    pub func insert(index int, value T)
    pub func remove(value T)
    pub func pop(index = -1) T
    pub func clear()
    pub func reverse()
    pub func __str__() str
    pub func __add__(seq Sequence[T]) list[T]
    pub func __iadd__(seq Sequence[T]) list[T]
    pub func copy(start = 0, end = -1) list[T]
    pub func reversed() Iterator[T]
}
```
**Bases:** [`MutableSequence[T]`](#mutablesequence)


A list is a mutable sequence of elements.

It implements `MutableSequence`, providing dynamic array functionality:
random access, appending, insertion, deletion and slicing.

Examples

```kl
let values = list[int](1, 2, 3)
values.push(4)
print(values[0])
print(values.pop())
```

---

<a id="list.__init__"></a>

**`__init__(args ...T)`** — *`@native`*

Initialize a list with the given items.

<a id="list.__iter__"></a>

**`__iter__() Iterator[T]`** — *`@native`*

Return an iterator over the list.

<a id="list.__len__"></a>

**`__len__() int`** — *`@native`*

Return the number of items in the list. O(1).

<a id="list.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the list is empty, false otherwise. O(1).

<a id="list.__getitem__"></a>

**`__getitem__(index int) T`** — *`@native`*

Get the item at the specified index. O(1).

Errors

Panic if the index is out of range.

<a id="list.__getslice__"></a>

**`__getslice__(r slice) list[T]`** — *`@native`*

Return a new list that is a sub-slice of the current list.

<a id="list.__contains__"></a>

**`__contains__(v T) bool`** — *`@native`*

Return true if the list contains the specified value `v`. O(n).

<a id="list.index"></a>

**`index(value T, start = 0, end = -1) int`** — *`@native`*

Return the first index of `value` in `[start, end)`. O(n).
If `end` is `-1`, the search extends to the end of the list.
Return `-1` if the value is not present.

<a id="list.rindex"></a>

**`rindex(value T, start = 0, end = -1) int`** — *`@native`*

Return the last index of `value` in `[start, end)`. O(n).
If `end` is `-1`, the search extends to the end of the list.
Return `-1` if the value is not present.

<a id="list.count"></a>

**`count(value T, start = 0, end = -1) int`** — *`@native`*

Return the number of occurrences of `value` in `[start, end)`. O(n).
If `end` is `-1`, the search extends to the end of the list.

<a id="list.__setitem__"></a>

**`__setitem__(index int, value T)`** — *`@native`*

Set the item at the specified index to `value`. O(1).

Errors

Panic if the index is out of range.

<a id="list.__setslice__"></a>

**`__setslice__(r slice, val Iterable[T])`** — *`@native`*

Replace a slice of the list with elements from the iterable `val`.
This may change the length of the list.

<a id="list.push"></a>

**`push(value T)`** — *`@native`*

Push a new item with `value` to the end of the list.

<a id="list.extend"></a>

**`extend(items Iterable[T])`** — *`@native`*

Extend the list by appending all items from `items`.

<a id="list.insert"></a>

**`insert(index int, value T)`** — *`@native`*

Insert a new item with `value` at the given `index`.
Elements at or after the index are shifted to the right.

<a id="list.remove"></a>

**`remove(value T)`** — *`@native`*

Remove the first occurrence of `value` from the list.

Errors

Panic if `value` is not present.

<a id="list.pop"></a>

**`pop(index = -1) T`** — *`@native`*

Remove and return the item at the given `index`.
If no index is specified, removes and returns the last item (`index = -1`).

<a id="list.clear"></a>

**`clear()`** — *`@native`*

Remove all items from the list, leaving it empty.

<a id="list.reverse"></a>

**`reverse()`** — *`@native`*

Reverse the elements of the list in place.

<a id="list.__str__"></a>

**`__str__() str`** — *`@native`*

Return the string representation of the list.

<a id="list.__add__"></a>

**`__add__(seq Sequence[T]) list[T]`** — *`@native`*

Return a new list with items of `self` followed by `seq` (`self + seq`).

<a id="list.__iadd__"></a>

**`__iadd__(seq Sequence[T]) list[T]`** — *`@native`*

Extend the list in place by appending all items from `seq` (`self += seq`).

<a id="list.copy"></a>

**`copy(start = 0, end = -1) list[T]`** — *`@native`*

Return a copy of the list from `start` (inclusive) to `end` (exclusive).
If `end` is `-1`, the copy extends to the end of the list.

<a id="list.reversed"></a>

**`reversed() Iterator[T]`** — *`@native`*

Return a reverse iterator over the elements of the list.

<a id="niltype"></a>

### `NilType`

```kl
pub class NilType {
    func __init__()
    pub func __str__() str
}
```

Represents a missing optional value in Koala.

`NilType` is the type of the singleton value `nil`.
It is used exclusively to indicate that an optional value is absent.

`nil` is a unique value: there is only one instance of `NilType`.


---

<a id="niltype.__init__"></a>

**`__init__()`**

Users cannot instantiate `NilType` directly. Use `nil` instead.

<a id="niltype.__str__"></a>

**`__str__() str`** — *`@native`*

Return the string representation of `nil`.
Always returns `"nil"`.

<a id="int8"></a>

### `int8`

```kl
pub class int8 {
    pub func __init__(x int64 | uint64 | float64)
}
```

An 8-bit signed integer.

Used to truncate a wider integer value down to 8 bits. Arithmetic operations
are performed via implicit promotion to `int64`.

---

<a id="int8.__init__"></a>

**`__init__(x int64 | uint64 | float64)`** — *`@intrinsic`*

Creates an `int8` by truncating `x` to 8 bits.

<a id="int16"></a>

### `int16`

```kl
pub class int16 {
    pub func __init__(x int64 | uint64 | float64)
}
```

A 16-bit signed integer.

Used to truncate a wider integer value down to 16 bits. Arithmetic operations
are performed via implicit promotion to `int64`.

---

<a id="int16.__init__"></a>

**`__init__(x int64 | uint64 | float64)`** — *`@intrinsic`*

Creates an `int16` by truncating `x` to 16 bits.

<a id="int32"></a>

### `int32`

```kl
pub class int32 {
    pub func __init__(x int64 | uint64 | float64)
}
```

A 32-bit signed integer.

Used to truncate a wider integer value down to 32 bits. Arithmetic operations
are performed via implicit promotion to `int64`.

---

<a id="int32.__init__"></a>

**`__init__(x int64 | uint64 | float64)`** — *`@intrinsic`*

Creates an `int32` by truncating `x` to 32 bits.

<a id="int64"></a>

### `int64`

```kl
pub class int64 : Comparable & Arithmetic & Bitwise {
    pub func __init__(x int64 | uint64 | float64 | str, base = 10)
    pub func __add__(rhs int64) int64
    pub func __sub__(rhs int64) int64
    pub func __mul__(rhs int64) int64
    pub func __div__(rhs int64) int64
    pub func __mod__(rhs int64) int64
    pub func __neg__() int64
    pub func __shl__(rhs int64) int64
    pub func __shr__(rhs int64) int64
    pub func __bitand__(rhs int64) int64
    pub func __bitor__(rhs int64) int64
    pub func __bitxor__(rhs int64) int64
    pub func __bitnot__() int64
    pub func __gt__(rhs int64) bool
    pub func __ge__(rhs int64) bool
    pub func __lt__(rhs int64) bool
    pub func __le__(rhs int64) bool
    pub func __eq__(rhs int64) bool
    pub func __ne__(rhs int64) bool
    pub func __hash__() int
    pub func __str__() str
    pub func __iadd__(rhs int64) int64
    pub func __isub__(rhs int64) int64
    pub func __imul__(rhs int64) int64
    pub func __idiv__(rhs int64) int64
    pub func __imod__(rhs int64) int64
    pub func __ishl__(rhs int64) int64
    pub func __ishr__(rhs int64) int64
    pub func __ibitand__(rhs int64) int64
    pub func __ibitor__(rhs int64) int64
    pub func __ibitxor__(rhs int64) int64
    pub func pow(x int64) int64
    pub func abs() int64
    pub func to_float() float64
}
```
**Bases:** [`Comparable`](#comparable) & [`Arithmetic`](#arithmetic) & [`Bitwise`](#bitwise)


A 64-bit signed integer.

The primary signed integer type.

---

<a id="int64.__init__"></a>

**`__init__(x int64 | uint64 | float64 | str, base = 10)`** — *`@native`*

Creates an `int64`.

The argument `x` can be an integer, a floating-point value, or a string.
When `x` is a string, `base` specifies the numeric base used for parsing
(default `10`).

<a id="int64.__add__"></a>

**`__add__(rhs int64) int64`** — *`@intrinsic`*

Returns the sum of `self` and `rhs` (`self + rhs`).

<a id="int64.__sub__"></a>

**`__sub__(rhs int64) int64`** — *`@intrinsic`*

Returns the difference of `self` and `rhs` (`self - rhs`).

<a id="int64.__mul__"></a>

**`__mul__(rhs int64) int64`** — *`@intrinsic`*

Returns the product of `self` and `rhs` (`self * rhs`).

<a id="int64.__div__"></a>

**`__div__(rhs int64) int64`** — *`@intrinsic`*

Returns the quotient of `self` and `rhs` (`self / rhs`).

<a id="int64.__mod__"></a>

**`__mod__(rhs int64) int64`** — *`@intrinsic`*

Returns the remainder of `self` divided by `rhs` (`self % rhs`).

<a id="int64.__neg__"></a>

**`__neg__() int64`** — *`@intrinsic`*

Returns the negation of `self` (`-self`).

<a id="int64.__shl__"></a>

**`__shl__(rhs int64) int64`** — *`@intrinsic`*

Left shift (`self << rhs`).

<a id="int64.__shr__"></a>

**`__shr__(rhs int64) int64`** — *`@intrinsic`*

Right shift (`self >> rhs`).

<a id="int64.__bitand__"></a>

**`__bitand__(rhs int64) int64`** — *`@intrinsic`*

Bitwise AND (`self & rhs`).

<a id="int64.__bitor__"></a>

**`__bitor__(rhs int64) int64`** — *`@intrinsic`*

Bitwise OR (`self | rhs`).

<a id="int64.__bitxor__"></a>

**`__bitxor__(rhs int64) int64`** — *`@intrinsic`*

Bitwise XOR (`self ^ rhs`).

<a id="int64.__bitnot__"></a>

**`__bitnot__() int64`** — *`@intrinsic`*

Bitwise NOT (`~self`).

<a id="int64.__gt__"></a>

**`__gt__(rhs int64) bool`** — *`@intrinsic`*

Returns `true` if `self` is greater than `rhs` (`self > rhs`).

<a id="int64.__ge__"></a>

**`__ge__(rhs int64) bool`** — *`@intrinsic`*

Returns `true` if `self` is greater than or equal to `rhs` (`self >= rhs`).

<a id="int64.__lt__"></a>

**`__lt__(rhs int64) bool`** — *`@intrinsic`*

Returns `true` if `self` is less than `rhs` (`self < rhs`).

<a id="int64.__le__"></a>

**`__le__(rhs int64) bool`** — *`@intrinsic`*

Returns `true` if `self` is less than or equal to `rhs` (`self <= rhs`).

<a id="int64.__eq__"></a>

**`__eq__(rhs int64) bool`** — *`@intrinsic`*

Returns `true` if `self` is equal to `rhs` (`self == rhs`).

<a id="int64.__ne__"></a>

**`__ne__(rhs int64) bool`** — *`@intrinsic`*

Returns `true` if `self` is not equal to `rhs` (`self != rhs`).

<a id="int64.__hash__"></a>

**`__hash__() int`** — *`@native`*

Returns a hash value for `self`.

<a id="int64.__str__"></a>

**`__str__() str`** — *`@native`*

Returns the decimal string representation of `self`.

<a id="int64.__iadd__"></a>

**`__iadd__(rhs int64) int64`** — *`@intrinsic`*

Adds `rhs` to `self` in place (`self += rhs`).

<a id="int64.__isub__"></a>

**`__isub__(rhs int64) int64`** — *`@intrinsic`*

Subtracts `rhs` from `self` in place (`self -= rhs`).

<a id="int64.__imul__"></a>

**`__imul__(rhs int64) int64`** — *`@intrinsic`*

Multiplies `self` by `rhs` in place (`self *= rhs`).

<a id="int64.__idiv__"></a>

**`__idiv__(rhs int64) int64`** — *`@intrinsic`*

Divides `self` by `rhs` in place (`self /= rhs`).

<a id="int64.__imod__"></a>

**`__imod__(rhs int64) int64`** — *`@intrinsic`*

Assigns the remainder of `self` divided by `rhs` in place (`self %= rhs`).

<a id="int64.__ishl__"></a>

**`__ishl__(rhs int64) int64`** — *`@intrinsic`*

Left shifts `self` by `rhs` in place (`self <<= rhs`).

<a id="int64.__ishr__"></a>

**`__ishr__(rhs int64) int64`** — *`@intrinsic`*

Right shifts `self` by `rhs` in place (`self >>= rhs`).

<a id="int64.__ibitand__"></a>

**`__ibitand__(rhs int64) int64`** — *`@intrinsic`*

Bitwise ANDs `self` with `rhs` in place (`self &= rhs`).

<a id="int64.__ibitor__"></a>

**`__ibitor__(rhs int64) int64`** — *`@intrinsic`*

Bitwise ORs `self` with `rhs` in place (`self |= rhs`).

<a id="int64.__ibitxor__"></a>

**`__ibitxor__(rhs int64) int64`** — *`@intrinsic`*

Bitwise XORs `self` with `rhs` in place (`self ^= rhs`).

<a id="int64.pow"></a>

**`pow(x int64) int64`** — *`@native`*

Raises `self` to the power `x`.

<a id="int64.abs"></a>

**`abs() int64`** — *`@native`*

Returns the absolute value of `self`.

<a id="int64.to_float"></a>

**`to_float() float64`** — *`@native`*

Converts `self` to a `float64`.

<a id="uint8"></a>

### `uint8`

```kl
pub class uint8 {
    pub func __init__(x int64 | uint64 | float64)
}
```

An 8-bit unsigned integer.

Used to truncate a wider integer value down to 8 bits. Arithmetic operations
are performed via implicit promotion to `uint64`.

---

<a id="uint8.__init__"></a>

**`__init__(x int64 | uint64 | float64)`** — *`@intrinsic`*

Creates a `uint8` by truncating `x` to 8 bits.

<a id="uint16"></a>

### `uint16`

```kl
pub class uint16 {
    pub func __init__(x int64 | uint64 | float64)
}
```

A 16-bit unsigned integer.

Used to truncate a wider integer value down to 16 bits. Arithmetic operations
are performed via implicit promotion to `uint64`.

---

<a id="uint16.__init__"></a>

**`__init__(x int64 | uint64 | float64)`** — *`@intrinsic`*

Creates a `uint16` by truncating `x` to 16 bits.

<a id="uint32"></a>

### `uint32`

```kl
pub class uint32 {
    pub func __init__(x int64 | uint64 | float64)
}
```

A 32-bit unsigned integer.

Used to truncate a wider integer value down to 32 bits. Arithmetic operations
are performed via implicit promotion to `uint64`.

---

<a id="uint32.__init__"></a>

**`__init__(x int64 | uint64 | float64)`** — *`@intrinsic`*

Creates a `uint32` by truncating `x` to 32 bits.

<a id="uint64"></a>

### `uint64`

```kl
pub class uint64 : Comparable & Arithmetic & Bitwise {
    pub func __init__(x int64 | uint64 | float64 | str, base = 10)
    pub func __add__(rhs uint64) uint64
    pub func __sub__(rhs uint64) uint64
    pub func __mul__(rhs uint64) uint64
    pub func __div__(rhs uint64) uint64
    pub func __mod__(rhs uint64) uint64
    pub func __shl__(rhs uint64) uint64
    pub func __shr__(rhs uint64) uint64
    pub func __bitand__(rhs uint64) uint64
    pub func __bitor__(rhs uint64) uint64
    pub func __bitxor__(rhs uint64) uint64
    pub func __bitnot__() uint64
    pub func __gt__(rhs uint64) bool
    pub func __ge__(rhs uint64) bool
    pub func __lt__(rhs uint64) bool
    pub func __le__(rhs uint64) bool
    pub func __eq__(rhs uint64) bool
    pub func __ne__(rhs uint64) bool
    pub func __hash__() int
    pub func __str__() str
    pub func __iadd__(rhs uint64) uint64
    pub func __isub__(rhs uint64) uint64
    pub func __imul__(rhs uint64) uint64
    pub func __idiv__(rhs uint64) uint64
    pub func __imod__(rhs uint64) uint64
    pub func __ishl__(rhs uint64) uint64
    pub func __ishr__(rhs uint64) uint64
    pub func __ibitand__(rhs uint64) uint64
    pub func __ibitor__(rhs uint64) uint64
    pub func __ibitxor__(rhs uint64) uint64
    pub func pow(x uint64) uint64
    pub func to_float() float64
}
```
**Bases:** [`Comparable`](#comparable) & [`Arithmetic`](#arithmetic) & [`Bitwise`](#bitwise)


A 64-bit unsigned integer.

The primary unsigned integer type.

---

<a id="uint64.__init__"></a>

**`__init__(x int64 | uint64 | float64 | str, base = 10)`** — *`@native`*

Creates a `uint64`.

The argument `x` can be an integer, a floating-point value, or a string.
When `x` is a string, `base` specifies the numeric base used for parsing
(default `10`).

<a id="uint64.__add__"></a>

**`__add__(rhs uint64) uint64`** — *`@intrinsic`*

Returns the sum of `self` and `rhs` (`self + rhs`).

<a id="uint64.__sub__"></a>

**`__sub__(rhs uint64) uint64`** — *`@intrinsic`*

Returns the difference of `self` and `rhs` (`self - rhs`).

<a id="uint64.__mul__"></a>

**`__mul__(rhs uint64) uint64`** — *`@intrinsic`*

Returns the product of `self` and `rhs` (`self * rhs`).

<a id="uint64.__div__"></a>

**`__div__(rhs uint64) uint64`** — *`@intrinsic`*

Returns the quotient of `self` and `rhs` (`self / rhs`).

<a id="uint64.__mod__"></a>

**`__mod__(rhs uint64) uint64`** — *`@intrinsic`*

Returns the remainder of `self` divided by `rhs` (`self % rhs`).

<a id="uint64.__shl__"></a>

**`__shl__(rhs uint64) uint64`** — *`@intrinsic`*

Left shift (`self << rhs`).

<a id="uint64.__shr__"></a>

**`__shr__(rhs uint64) uint64`** — *`@intrinsic`*

Right shift (`self >> rhs`).

<a id="uint64.__bitand__"></a>

**`__bitand__(rhs uint64) uint64`** — *`@intrinsic`*

Bitwise AND (`self & rhs`).

<a id="uint64.__bitor__"></a>

**`__bitor__(rhs uint64) uint64`** — *`@intrinsic`*

Bitwise OR (`self | rhs`).

<a id="uint64.__bitxor__"></a>

**`__bitxor__(rhs uint64) uint64`** — *`@intrinsic`*

Bitwise XOR (`self ^ rhs`).

<a id="uint64.__bitnot__"></a>

**`__bitnot__() uint64`** — *`@intrinsic`*

Bitwise NOT (`~self`).

<a id="uint64.__gt__"></a>

**`__gt__(rhs uint64) bool`** — *`@intrinsic`*

Returns `true` if `self` is greater than `rhs` (`self > rhs`).

<a id="uint64.__ge__"></a>

**`__ge__(rhs uint64) bool`** — *`@intrinsic`*

Returns `true` if `self` is greater than or equal to `rhs` (`self >= rhs`).

<a id="uint64.__lt__"></a>

**`__lt__(rhs uint64) bool`** — *`@intrinsic`*

Returns `true` if `self` is less than `rhs` (`self < rhs`).

<a id="uint64.__le__"></a>

**`__le__(rhs uint64) bool`** — *`@intrinsic`*

Returns `true` if `self` is less than or equal to `rhs` (`self <= rhs`).

<a id="uint64.__eq__"></a>

**`__eq__(rhs uint64) bool`** — *`@intrinsic`*

Returns `true` if `self` is equal to `rhs` (`self == rhs`).

<a id="uint64.__ne__"></a>

**`__ne__(rhs uint64) bool`** — *`@intrinsic`*

Returns `true` if `self` is not equal to `rhs` (`self != rhs`).

<a id="uint64.__hash__"></a>

**`__hash__() int`** — *`@native`*

Returns a hash value for `self`.

<a id="uint64.__str__"></a>

**`__str__() str`** — *`@native`*

Returns the decimal string representation of `self`.

<a id="uint64.__iadd__"></a>

**`__iadd__(rhs uint64) uint64`** — *`@intrinsic`*

Adds `rhs` to `self` in place (`self += rhs`).

<a id="uint64.__isub__"></a>

**`__isub__(rhs uint64) uint64`** — *`@intrinsic`*

Subtracts `rhs` from `self` in place (`self -= rhs`).

<a id="uint64.__imul__"></a>

**`__imul__(rhs uint64) uint64`** — *`@intrinsic`*

Multiplies `self` by `rhs` in place (`self *= rhs`).

<a id="uint64.__idiv__"></a>

**`__idiv__(rhs uint64) uint64`** — *`@intrinsic`*

Divides `self` by `rhs` in place (`self /= rhs`).

<a id="uint64.__imod__"></a>

**`__imod__(rhs uint64) uint64`** — *`@intrinsic`*

Assigns the remainder of `self` divided by `rhs` in place (`self %= rhs`).

<a id="uint64.__ishl__"></a>

**`__ishl__(rhs uint64) uint64`** — *`@intrinsic`*

Left shifts `self` by `rhs` in place (`self <<= rhs`).

<a id="uint64.__ishr__"></a>

**`__ishr__(rhs uint64) uint64`** — *`@intrinsic`*

Right shifts `self` by `rhs` in place (`self >>= rhs`).

<a id="uint64.__ibitand__"></a>

**`__ibitand__(rhs uint64) uint64`** — *`@intrinsic`*

Bitwise ANDs `self` with `rhs` in place (`self &= rhs`).

<a id="uint64.__ibitor__"></a>

**`__ibitor__(rhs uint64) uint64`** — *`@intrinsic`*

Bitwise ORs `self` with `rhs` in place (`self |= rhs`).

<a id="uint64.__ibitxor__"></a>

**`__ibitxor__(rhs uint64) uint64`** — *`@intrinsic`*

Bitwise XORs `self` with `rhs` in place (`self ^= rhs`).

<a id="uint64.pow"></a>

**`pow(x uint64) uint64`** — *`@native`*

Raises `self` to the power `x`.

<a id="uint64.to_float"></a>

**`to_float() float64`** — *`@native`*

Converts `self` to a `float64`.

<a id="float16"></a>

### `float16`

```kl
pub class float16 {
    pub func __init__(x int64 | uint64 | float64)
}
```

A 16-bit floating-point number (half precision).

Used to truncate a `float64` value down to 16 bits. Arithmetic operations
are performed via implicit promotion to `float64`.

---

<a id="float16.__init__"></a>

**`__init__(x int64 | uint64 | float64)`** — *`@intrinsic`*

Creates a `float16` by truncating `x` to 16 bits.

<a id="float32"></a>

### `float32`

```kl
pub class float32 {
    pub func __init__(x int64 | uint64 | float64)
}
```

A 32-bit floating-point number (single precision).

Used to truncate a `float64` value down to 32 bits. Arithmetic operations
are performed via implicit promotion to `float64`.

---

<a id="float32.__init__"></a>

**`__init__(x int64 | uint64 | float64)`** — *`@intrinsic`*

Creates a `float32` by truncating `x` to 32 bits.

<a id="float64"></a>

### `float64`

```kl
pub class float64 : Comparable & Arithmetic {
    pub func __init__(x int64 | uint64 | float64 | str)
    pub func __add__(rhs float64) float64
    pub func __sub__(rhs float64) float64
    pub func __mul__(rhs float64) float64
    pub func __div__(rhs float64) float64
    pub func __mod__(rhs float64) float64
    pub func __neg__() float64
    pub func __gt__(rhs float64) bool
    pub func __ge__(rhs float64) bool
    pub func __lt__(rhs float64) bool
    pub func __le__(rhs float64) bool
    pub func __eq__(rhs float64) bool
    pub func __ne__(rhs float64) bool
    pub func __hash__() int
    pub func __str__() str
    pub func __iadd__(rhs float64) float64
    pub func __isub__(rhs float64) float64
    pub func __imul__(rhs float64) float64
    pub func __idiv__(rhs float64) float64
    pub func __imod__(rhs float64) float64
    pub func abs() float64
    pub func ceil() float64
    pub func floor() float64
    pub func round() float64
    pub func sqrt() float64
    pub func pow(x float64) float64
    pub func to_int() int64
    pub func is_nan() bool
    pub func is_inf() bool
}
```
**Bases:** [`Comparable`](#comparable) & [`Arithmetic`](#arithmetic)


A 64-bit floating-point number (double precision).

The primary floating-point type, conforming to IEEE 754.

---

<a id="float64.__init__"></a>

**`__init__(x int64 | uint64 | float64 | str)`** — *`@native`*

Creates a `float64` from an integer, another floating-point value, or a string.

<a id="float64.__add__"></a>

**`__add__(rhs float64) float64`** — *`@intrinsic`*

Returns the sum of `self` and `rhs` (`self + rhs`).

<a id="float64.__sub__"></a>

**`__sub__(rhs float64) float64`** — *`@intrinsic`*

Returns the difference of `self` and `rhs` (`self - rhs`).

<a id="float64.__mul__"></a>

**`__mul__(rhs float64) float64`** — *`@intrinsic`*

Returns the product of `self` and `rhs` (`self * rhs`).

<a id="float64.__div__"></a>

**`__div__(rhs float64) float64`** — *`@intrinsic`*

Returns the quotient of `self` and `rhs` (`self / rhs`).

<a id="float64.__mod__"></a>

**`__mod__(rhs float64) float64`** — *`@intrinsic`*

Returns the remainder of `self` divided by `rhs` (`self % rhs`).

<a id="float64.__neg__"></a>

**`__neg__() float64`** — *`@intrinsic`*

Returns the negation of `self` (`-self`).

<a id="float64.__gt__"></a>

**`__gt__(rhs float64) bool`** — *`@intrinsic`*

Returns `true` if `self` is greater than `rhs` (`self > rhs`).

<a id="float64.__ge__"></a>

**`__ge__(rhs float64) bool`** — *`@intrinsic`*

Returns `true` if `self` is greater than or equal to `rhs` (`self >= rhs`).

<a id="float64.__lt__"></a>

**`__lt__(rhs float64) bool`** — *`@intrinsic`*

Returns `true` if `self` is less than `rhs` (`self < rhs`).

<a id="float64.__le__"></a>

**`__le__(rhs float64) bool`** — *`@intrinsic`*

Returns `true` if `self` is less than or equal to `rhs` (`self <= rhs`).

<a id="float64.__eq__"></a>

**`__eq__(rhs float64) bool`** — *`@intrinsic`*

Returns `true` if `self` is equal to `rhs` (`self == rhs`).

<a id="float64.__ne__"></a>

**`__ne__(rhs float64) bool`** — *`@intrinsic`*

Returns `true` if `self` is not equal to `rhs` (`self != rhs`).

<a id="float64.__hash__"></a>

**`__hash__() int`** — *`@native`*

Returns a hash value for `self`.

<a id="float64.__str__"></a>

**`__str__() str`** — *`@native`*

Returns the string representation of `self`.

<a id="float64.__iadd__"></a>

**`__iadd__(rhs float64) float64`** — *`@intrinsic`*

Adds `rhs` to `self` in place (`self += rhs`).

<a id="float64.__isub__"></a>

**`__isub__(rhs float64) float64`** — *`@intrinsic`*

Subtracts `rhs` from `self` in place (`self -= rhs`).

<a id="float64.__imul__"></a>

**`__imul__(rhs float64) float64`** — *`@intrinsic`*

Multiplies `self` by `rhs` in place (`self *= rhs`).

<a id="float64.__idiv__"></a>

**`__idiv__(rhs float64) float64`** — *`@intrinsic`*

Divides `self` by `rhs` in place (`self /= rhs`).

<a id="float64.__imod__"></a>

**`__imod__(rhs float64) float64`** — *`@intrinsic`*

Assigns the remainder of `self` divided by `rhs` in place (`self %= rhs`).

<a id="float64.abs"></a>

**`abs() float64`** — *`@native`*

Returns the absolute value of `self`.

<a id="float64.ceil"></a>

**`ceil() float64`** — *`@native`*

Returns the smallest integer greater than or equal to `self`.

<a id="float64.floor"></a>

**`floor() float64`** — *`@native`*

Returns the largest integer less than or equal to `self`.

<a id="float64.round"></a>

**`round() float64`** — *`@native`*

Returns the nearest integer to `self`.

<a id="float64.sqrt"></a>

**`sqrt() float64`** — *`@native`*

Returns the square root of `self`.

<a id="float64.pow"></a>

**`pow(x float64) float64`** — *`@native`*

Raises `self` to the power `x`.

<a id="float64.to_int"></a>

**`to_int() int64`** — *`@native`*

Converts `self` to an `int64`, truncating toward zero.

<a id="float64.is_nan"></a>

**`is_nan() bool`** — *`@native`*

Returns `true` if `self` is NaN (not a number).

<a id="float64.is_inf"></a>

**`is_inf() bool`** — *`@native`*

Returns `true` if `self` is positive or negative infinity.

<a id="range"></a>

### `range`

```kl
pub class range : Sequence[int] {
    pub let start int
    pub let end int
    pub let step int
    pub func __init__(_start int, _end int, _step = 1)
    pub func __iter__() Iterator[int]
    pub func __len__() int
    pub func empty() bool
    pub func __getitem__(index int) int
    pub func __getslice__(r slice) range
    pub func __contains__(item int) bool
    pub func index(value int, _start = 0, _end = -1) int
    pub func rindex(value int, _start = 0, _end = -1) int
    pub func count(value int, _start = 0, _end = -1) int
    pub func __str__() str
    pub func reversed() range
}
```
**Bases:** [`Sequence[int]`](#sequence)


A range represents an immutable sequence of integers.

It is commonly used for looping a specific number of times in `for` loops.
Since it implements `Sequence`, it supports O(1) length and indexing.

Examples

```kl
for value in range(0, 3) {
    print(value)
}
```

---

**Fields**

<a id="range.start"></a>

**`pub let start int`**

The starting value of the range (inclusive).

<a id="range.end"></a>

**`pub let end int`**

The boundary value of the range (exclusive).

<a id="range.step"></a>

**`pub let step int`**

The interval between each number in the sequence.

<a id="range.__init__"></a>

**`__init__(_start int, _end int, _step = 1)`** — *`@native`*

Initialize a range from `_start` (inclusive) to `_end` (exclusive).

<a id="range.__iter__"></a>

**`__iter__() Iterator[int]`** — *`@native`*

Return an iterator for the range.

<a id="range.__len__"></a>

**`__len__() int`** — *`@native`*

Return the number of elements in the range. O(1).

<a id="range.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the range is empty. O(1).

<a id="range.__getitem__"></a>

**`__getitem__(index int) int`** — *`@native`*

Get the integer at the specified index. O(1).

<a id="range.__getslice__"></a>

**`__getslice__(r slice) range`** — *`@native`*

Return a new range that is a sub-slice of the current range. O(1).

<a id="range.__contains__"></a>

**`__contains__(item int) bool`** — *`@native`*

Return true if the specified item is part of the range. O(1).

<a id="range.index"></a>

**`index(value int, _start = 0, _end = -1) int`** — *`@native`*

Return the index of `value` in the range, restricted to `[_start, _end)`. O(1).
If `_end` is `-1`, the search extends to the end of the range.
Return `-1` if the value is not present.

<a id="range.rindex"></a>

**`rindex(value int, _start = 0, _end = -1) int`** — *`@native`*

Return the last index of `value` in the range, restricted to `[_start, _end)`.
Since all values in a range are unique, this is identical to
[`range.index`](#range.index).
If `_end` is `-1`, the search extends to the end of the range.
Return `-1` if the value is not present.

<a id="range.count"></a>

**`count(value int, _start = 0, _end = -1) int`** — *`@native`*

Return the number of occurrences of `value` in `[_start, _end)`. O(1).
Since all values in a range are unique, this returns 1 if the value
is present, otherwise 0.
If `_end` is `-1`, the search extends to the end of the range.

<a id="range.__str__"></a>

**`__str__() str`** — *`@native`*

Return the string representation of the range. O(1).
Uses the form `range(start, end, step)`.

<a id="range.reversed"></a>

**`reversed() range`** — *`@native`*

Return a new range with the elements in reverse order. O(1).

<a id="hashset"></a>

### `HashSet`

```kl
pub class HashSet[T] : Set[T] {
    pub func __init__()
    pub func __iter__() Iterator[T]
    pub func __len__() int
    pub func empty() bool
    pub func __contains__(v T) bool
    pub func add(v T) bool
    pub func update(items Iterable[T])
    pub func remove(v T) bool
    pub func clear()
    pub func is_subset(other Iterable[T]) bool
    pub func is_superset(other Iterable[T]) bool
    pub func union(other Iterable[T]) HashSet[T]
    pub func intersect(other Iterable[T]) HashSet[T]
    pub func diff(other Iterable[T]) HashSet[T]
    pub func __str__() str
    pub func sym_diff(other Iterable[T]) HashSet[T]
    pub func __or__(other Iterable[T]) HashSet[T]
    pub func __and__(other Iterable[T]) HashSet[T]
    pub func __sub__(other Iterable[T]) HashSet[T]
    pub func to_list() list[T]
}
```
**Bases:** [`Set[T]`](#set)


A HashSet is a mutable, unordered collection of unique elements,
backed by a hash table.

It implements `Set`, providing membership tests, insertion, removal,
and set algebra. Iteration order is unspecified.

---

<a id="hashset.__init__"></a>

**`__init__()`** — *`@native`*

Create an empty hash set.

<a id="hashset.__iter__"></a>

**`__iter__() Iterator[T]`** — *`@native`*

Return an iterator over the elements. Order is unspecified.

<a id="hashset.__len__"></a>

**`__len__() int`** — *`@native`*

Return the number of elements. O(1).

<a id="hashset.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the set is empty, false otherwise. O(1).

<a id="hashset.__contains__"></a>

**`__contains__(v T) bool`** — *`@native`*

Return true if the set contains `v`.

<a id="hashset.add"></a>

**`add(v T) bool`** — *`@native`*

Add an element to the set.
Return true if the element was not already present.

<a id="hashset.update"></a>

**`update(items Iterable[T])`** — *`@native`*

Add all elements from `items` to the set.
Unlike `add` which inserts a single element, `update` inserts
multiple elements from any iterable source in one call.

<a id="hashset.remove"></a>

**`remove(v T) bool`** — *`@native`*

Remove an element from the set.
Return true if the element was present.

<a id="hashset.clear"></a>

**`clear()`** — *`@native`*

Remove all elements from the set, leaving it empty.

<a id="hashset.is_subset"></a>

**`is_subset(other Iterable[T]) bool`** — *`@native`*

Return true if every element in the set is in `other`.

<a id="hashset.is_superset"></a>

**`is_superset(other Iterable[T]) bool`** — *`@native`*

Return true if every element in `other` is in the set.

<a id="hashset.union"></a>

**`union(other Iterable[T]) HashSet[T]`** — *`@native`*

Return a new set with elements from the set and `other`.

<a id="hashset.intersect"></a>

**`intersect(other Iterable[T]) HashSet[T]`** — *`@native`*

Return a new set with elements common to the set and `other`.

<a id="hashset.diff"></a>

**`diff(other Iterable[T]) HashSet[T]`** — *`@native`*

Return a new set with elements in the set that are not in `other`.

<a id="hashset.__str__"></a>

**`__str__() str`** — *`@native`*

Return a string representation of the set.
Uses the form `{element1, element2}`.

<a id="hashset.sym_diff"></a>

**`sym_diff(other Iterable[T]) HashSet[T]`** — *`@native`*

Return a new set with elements in the set or `other`, but not both.

<a id="hashset.__or__"></a>

**`__or__(other Iterable[T]) HashSet[T]`** — *`@native`*

Return a new set with elements from the set and `other` (`self | other`).

<a id="hashset.__and__"></a>

**`__and__(other Iterable[T]) HashSet[T]`** — *`@native`*

Return a new set with elements common to the set and `other` (`self & other`).

<a id="hashset.__sub__"></a>

**`__sub__(other Iterable[T]) HashSet[T]`** — *`@native`*

Return a new set with elements in the set that are not in `other` (`self - other`).

<a id="hashset.to_list"></a>

**`to_list() list[T]`** — *`@native`*

Return a list of all elements. Order is unspecified.

<a id="treeset"></a>

### `TreeSet`

```kl
pub class TreeSet[T : Comparable] : Set[T] {
    pub func __init__()
    pub func __iter__() Iterator[T]
    pub func __len__() int
    pub func empty() bool
    pub func __contains__(v T) bool
    pub func add(v T) bool
    pub func update(items Iterable[T])
    pub func remove(v T) bool
    pub func clear()
    pub func is_subset(other Iterable[T]) bool
    pub func is_superset(other Iterable[T]) bool
    pub func union(other Iterable[T]) TreeSet[T]
    pub func intersect(other Iterable[T]) TreeSet[T]
    pub func diff(other Iterable[T]) TreeSet[T]
    pub func __str__() str
    pub func sym_diff(other Iterable[T]) TreeSet[T]
    pub func __or__(other Iterable[T]) TreeSet[T]
    pub func __and__(other Iterable[T]) TreeSet[T]
    pub func __sub__(other Iterable[T]) TreeSet[T]
    pub func reversed() Iterator[T]
    pub func to_list() list[T]
}
```
**Bases:** [`Set[T]`](#set)


A TreeSet is a mutable, sorted collection of unique elements,
backed by a balanced tree.

It implements `Set`, providing membership tests, insertion, removal,
and set algebra. Elements are ordered by their `Comparable` ordering.
Iteration and `to_list()` follow that order; `reversed()` traverses it
in reverse.

---

<a id="treeset.__init__"></a>

**`__init__()`** — *`@native`*

Create an empty tree set.

<a id="treeset.__iter__"></a>

**`__iter__() Iterator[T]`** — *`@native`*

Return an iterator over the elements in sorted order.

<a id="treeset.__len__"></a>

**`__len__() int`** — *`@native`*

Return the number of elements. O(1).

<a id="treeset.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the set is empty, false otherwise. O(1).

<a id="treeset.__contains__"></a>

**`__contains__(v T) bool`** — *`@native`*

Return true if the set contains `v`.

<a id="treeset.add"></a>

**`add(v T) bool`** — *`@native`*

Add an element to the set.
Return true if the element was not already present.

<a id="treeset.update"></a>

**`update(items Iterable[T])`** — *`@native`*

Add all elements from `items` to the set.
Unlike `add` which inserts a single element, `update` inserts
multiple elements from any iterable source in one call.

<a id="treeset.remove"></a>

**`remove(v T) bool`** — *`@native`*

Remove an element from the set.
Return true if the element was present.

<a id="treeset.clear"></a>

**`clear()`** — *`@native`*

Remove all elements from the set, leaving it empty.

<a id="treeset.is_subset"></a>

**`is_subset(other Iterable[T]) bool`** — *`@native`*

Return true if every element in the set is in `other`.

<a id="treeset.is_superset"></a>

**`is_superset(other Iterable[T]) bool`** — *`@native`*

Return true if every element in `other` is in the set.

<a id="treeset.union"></a>

**`union(other Iterable[T]) TreeSet[T]`** — *`@native`*

Return a new sorted set with elements from the set and `other`.

<a id="treeset.intersect"></a>

**`intersect(other Iterable[T]) TreeSet[T]`** — *`@native`*

Return a new sorted set with elements common to the set and `other`.

<a id="treeset.diff"></a>

**`diff(other Iterable[T]) TreeSet[T]`** — *`@native`*

Return a new sorted set with elements in the set that are not in `other`.

<a id="treeset.__str__"></a>

**`__str__() str`** — *`@native`*

Return a string representation of the set.
Uses the form `{element1, element2}`, in sorted order.

<a id="treeset.sym_diff"></a>

**`sym_diff(other Iterable[T]) TreeSet[T]`** — *`@native`*

Return a new sorted set with elements in the set or `other`, but not both.

<a id="treeset.__or__"></a>

**`__or__(other Iterable[T]) TreeSet[T]`** — *`@native`*

Return a new sorted set with elements from the set and `other` (`self | other`).

<a id="treeset.__and__"></a>

**`__and__(other Iterable[T]) TreeSet[T]`** — *`@native`*

Return a new sorted set with elements common to the set and `other` (`self & other`).

<a id="treeset.__sub__"></a>

**`__sub__(other Iterable[T]) TreeSet[T]`** — *`@native`*

Return a new sorted set with elements in the set that are not in `other` (`self - other`).

<a id="treeset.reversed"></a>

**`reversed() Iterator[T]`** — *`@native`*

Return a reverse iterator over the elements in reverse sorted order.

<a id="treeset.to_list"></a>

**`to_list() list[T]`** — *`@native`*

Return a list of all elements in sorted order.

<a id="slice"></a>

### `slice`

```kl
pub class slice {
    pub let start int
    pub let end int
    pub let step int
    pub func __init__(_start int, _end int, _step = 1)
    pub func __str__() str
}
```

A slice object represents a set of indices specified by `[start:end:step]`.

It is internally created by the compiler when the `:` syntax is used
inside subscript brackets `[]`. Omitted and negative bounds are completed
by the compiler against the target sequence length, so all fields are
concrete integers. Follows Python's slicing semantics, including
negative steps for reverse slicing.

---

**Fields**

<a id="slice.start"></a>

**`pub let start int`**

The starting index of the slice (inclusive).

<a id="slice.end"></a>

**`pub let end int`**

The ending index of the slice (exclusive).
For reverse slices it may be `-1`, meaning the slice goes down to index 0.

<a id="slice.step"></a>

**`pub let step int`**

The step value of the slice. A negative step reverses the direction.
Must not be zero; a zero step triggers a panic.

<a id="slice.__init__"></a>

**`__init__(_start int, _end int, _step = 1)`**

Initialize a new slice object.

Typically constructed by the `build_intern` op when the compiler
translates the `[start:end:step]` syntax. Users may also construct
slice objects explicitly via `slice(start, end, step)`.

Pass `INT_MIN` for `_start` or `_end` to indicate that the bound is
omitted; the runtime will resolve it to the appropriate default based
on the sign of `_step` (positive step → start=0 / end=len,
negative step → start=len-1 / end=-1). This sentinel applies equally
whether the slice originates from compiler-generated syntax or
user-facing construction.

`_step` defaults to 1 and must not be zero; a zero step triggers a panic.

<a id="slice.__str__"></a>

**`__str__() str`** — *`@native`*

Return the string representation of the slice. O(1).
Uses the form `slice(start, end, step)`.

<a id="str"></a>

### `str`

```kl
pub class str : Sequence & Comparable {
    pub func __init__(s any)
    pub func iter(step = 1) StrIter { return StrIter(self, step) }
    pub func reversed(step = 1) StrRevIter { return StrRevIter(self, step) }
    pub func __len__() int
    pub func empty() bool
    pub func __getitem__(index int) str
    pub func __getslice__(r slice) str
    pub func __contains__(sub str) bool
    pub func at(index int) str
    pub func index(value str, start = 0, end = -1) int
    pub func rindex(value str, start = 0, end = -1) int
    pub func count(value str, start = 0, end = -1) int
    pub func __eq__(rhs str) bool
    pub func __ne__(rhs str) bool
    pub func __lt__(rhs str) bool
    pub func __le__(rhs str) bool
    pub func __gt__(rhs str) bool
    pub func __ge__(rhs str) bool
    pub func __hash__() int
    pub func __str__() str
    pub func substr(start = 0, end = -1) str
    pub func to_int() int
    pub func to_int_or(default int) int
    pub func to_float() float64
    pub func to_float_or(default float64) float64
    pub func format(args ...) str
    pub func __add__(rhs str) str
    pub func __mul__(n int) str
    pub func __iadd__(rhs str) str
    pub func join(seq Sequence[str]) str
    pub func to_bytes() bytes
    pub func split(sep = " ", maxsplit = -1) list[str]
    pub func rsplit(sep = " ", maxsplit = -1) list[str]
    pub func splitlines() list[str]
    pub func upper() str
    pub func lower() str
    pub func capitalize() str
    pub func strip() str
    pub func lstrip() str
    pub func rstrip() str
    pub func startswith(prefix str) bool
    pub func endswith(suffix str) bool
    pub func removeprefix(prefix str) str
    pub func removesuffix(suffix str) str
    pub func replace(old str, new str) str
    pub func zfill(width int) str
    pub func ljust(width int, fill = " ") str
    pub func rjust(width int, fill = " ") str
    pub func center(width int, fill = " ") str
    pub func isdigit() bool
    pub func isalpha() bool
    pub func isalnum() bool
    pub func isspace() bool
    pub func islower() bool
    pub func isupper() bool
}
```
**Bases:** [`Sequence`](#sequence) & [`Comparable`](#comparable)


A str is an immutable sequence of characters.

It implements Sequence, providing text processing capabilities:
searching, slicing, splitting, joining, case conversion and formatting.

Characters are encoded as UTF-8. str operates at the character level,
so code-point level APIs (like Python's `ord`/`chr`) are not provided;
use [`str.to_bytes`](#str.to_bytes) for byte-level access.

Examples

```kl
let text = "hello"
print(text.upper())
print(text.substr(1, 4))
```

---

<a id="str.__init__"></a>

**`__init__(s any)`** — *`@native`*

Initialize a string from any Koala object.

The constructor accepts `any` because all classes automatically
provide a `__str__()` method. This allows `str(s)` to serve as the
universal string-conversion mechanism.

Notes

Koala splits Java’s `Object` into three independent traits
(`Equatable`, `Hashable`, `Printable`). Classes auto‑conform to
these traits, while traits remain pure. This enables type‑safe,
generic equality (`Equatable[T]`) without Java’s `equals(Object)`
limitation.

<a id="str.iter"></a>

**`iter(step = 1) StrIter { return StrIter(self, step) }`**

Return an iterator over the characters of the string.

The iterator traverses the string from left to right, returning
one character per step. The optional `step` argument controls
the stride of iteration:

- `step = 1` produces a normal character-by-character traversal.
- `step > 1` skips characters, yielding a strided iteration.

The iterator is stateful and does not allocate; it reads directly
from the underlying string.

<a id="str.reversed"></a>

**`reversed(step = 1) StrRevIter { return StrRevIter(self, step) }`**

Return a reverse iterator over the characters of the string.

The iterator traverses the string from right to left, returning
one character per step. The optional `step` argument controls
the stride of reverse iteration:

- `step = 1` produces a normal reverse traversal.
- `step > 1` skips characters in reverse order.

The iterator is stateful and does not allocate; it reads directly
from the underlying string.

<a id="str.__len__"></a>

**`__len__() int`** — *`@native`*

Return the length of the string.

<a id="str.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the string is empty, false otherwise.

<a id="str.__getitem__"></a>

**`__getitem__(index int) str`** — *`@native`*

Return the character at the specified index.

Errors

Panic if the index is out of range.

<a id="str.__getslice__"></a>

**`__getslice__(r slice) str`** — *`@native`*

Return the substring defined by the slice.

<a id="str.__contains__"></a>

**`__contains__(sub str) bool`** — *`@native`*

Return true if the string contains the substring `sub`.

<a id="str.at"></a>

**`at(index int) str`** — *`@intrinsic`*

Returns a single-character string at the given index.

Errors

Panic if `index` is negative or is not less than the string length.

<a id="str.index"></a>

**`index(value str, start = 0, end = -1) int`** — *`@native`*

Return the first index of the substring `value` in `[start, end)`.
If `end` is `-1`, the search extends to the end of the string.
Return `-1` if the substring is not present.

<a id="str.rindex"></a>

**`rindex(value str, start = 0, end = -1) int`** — *`@native`*

Return the last index of the substring `value` in `[start, end)`.
If `end` is `-1`, the search extends to the end of the string.
Return `-1` if the substring is not present.

<a id="str.count"></a>

**`count(value str, start = 0, end = -1) int`** — *`@native`*

Return the number of occurrences of the substring `value` in `[start, end)`.
If `end` is `-1`, the search extends to the end of the string.

<a id="str.__eq__"></a>

**`__eq__(rhs str) bool`** — *`@native`*

Return true if `self` is equal to `rhs` (`self == rhs`).

<a id="str.__ne__"></a>

**`__ne__(rhs str) bool`** — *`@native`*

Return true if `self` is not equal to `rhs` (`self != rhs`).

<a id="str.__lt__"></a>

**`__lt__(rhs str) bool`** — *`@native`*

Return true if `self` is lexicographically less than `rhs` (`self < rhs`).

<a id="str.__le__"></a>

**`__le__(rhs str) bool`** — *`@native`*

Return true if `self` is lexicographically less than or equal to `rhs` (`self <= rhs`).

<a id="str.__gt__"></a>

**`__gt__(rhs str) bool`** — *`@native`*

Return true if `self` is lexicographically greater than `rhs` (`self > rhs`).

<a id="str.__ge__"></a>

**`__ge__(rhs str) bool`** — *`@native`*

Return true if `self` is lexicographically greater than or equal to `rhs` (`self >= rhs`).

<a id="str.__hash__"></a>

**`__hash__() int`** — *`@native`*

Return a stable hash value of the string.
Equal strings must always produce identical hash values.

<a id="str.__str__"></a>

**`__str__() str`** — *`@native`*

Return the string itself.

<a id="str.substr"></a>

**`substr(start = 0, end = -1) str`** — *`@native`*

Return the substring from `start` (inclusive) to `end` (exclusive).
If `end` is `-1`, the substring extends to the end of the string.

<a id="str.to_int"></a>

**`to_int() int`** — *`@native`*

Parse the string as a decimal integer.

Errors

Panic if the string is not a valid integer.

<a id="str.to_int_or"></a>

**`to_int_or(default int) int`** — *`@native`*

Parse the string as a decimal integer.
Return `default` if the string is not a valid integer.

<a id="str.to_float"></a>

**`to_float() float64`** — *`@native`*

Parse the string as a floating-point number.

Errors

Panic if the string is not a valid number.

<a id="str.to_float_or"></a>

**`to_float_or(default float64) float64`** — *`@native`*

Parse the string as a floating-point number.
Return `default` if the string is not a valid number.

<a id="str.format"></a>

**`format(args ...) str`** — *`@native`*

Return a formatted string using `self` as the format.
`{}` consumes one argument in order.

<a id="str.__add__"></a>

**`__add__(rhs str) str`** — *`@native`*

Return a new string with `self` followed by `rhs` (`self + rhs`).

<a id="str.__mul__"></a>

**`__mul__(n int) str`** — *`@native`*

Return a new string with `self` repeated `n` times (`self * n`).
Return an empty string if `n` is not positive.

<a id="str.__iadd__"></a>

**`__iadd__(rhs str) str`** — *`@native`*

Append `rhs` to `self` in place (`self += rhs`), returning the updated string.
This mutates the original string buffer when possible.

<a id="str.join"></a>

**`join(seq Sequence[str]) str`** — *`@native`*

Return a string made by joining the elements of `seq`,
with `self` as the separator.

<a id="str.to_bytes"></a>

**`to_bytes() bytes`** — *`@native`*

Return the UTF-8 encoded bytes of the string.

<a id="str.split"></a>

**`split(sep = " ", maxsplit = -1) list[str]`** — *`@native`*

Split the string by the separator `sep` and return the parts as a list.
At most `maxsplit` splits are done; if `maxsplit` is -1, there is no limit.

<a id="str.rsplit"></a>

**`rsplit(sep = " ", maxsplit = -1) list[str]`** — *`@native`*

Split the string by the separator `sep` from the right and return
the parts as a list. At most `maxsplit` splits are done;
if `maxsplit` is -1, there is no limit.

<a id="str.splitlines"></a>

**`splitlines() list[str]`** — *`@native`*

Split the string at line boundaries and return the lines as a list.
The line break characters are not included.

<a id="str.upper"></a>

**`upper() str`** — *`@native`*

Return a copy of the string with all characters converted to uppercase.

<a id="str.lower"></a>

**`lower() str`** — *`@native`*

Return a copy of the string with all characters converted to lowercase.

<a id="str.capitalize"></a>

**`capitalize() str`** — *`@native`*

Return a copy of the string with the first character capitalized
and the rest lowercased.

<a id="str.strip"></a>

**`strip() str`** — *`@native`*

Return a copy of the string with leading and trailing whitespace removed.

<a id="str.lstrip"></a>

**`lstrip() str`** — *`@native`*

Return a copy of the string with leading whitespace removed.

<a id="str.rstrip"></a>

**`rstrip() str`** — *`@native`*

Return a copy of the string with trailing whitespace removed.

<a id="str.startswith"></a>

**`startswith(prefix str) bool`** — *`@native`*

Return true if the string starts with `prefix`.

<a id="str.endswith"></a>

**`endswith(suffix str) bool`** — *`@native`*

Return true if the string ends with `suffix`.

<a id="str.removeprefix"></a>

**`removeprefix(prefix str) str`** — *`@native`*

Return a copy of the string with `prefix` removed
if the string starts with it, otherwise return the string unchanged.

<a id="str.removesuffix"></a>

**`removesuffix(suffix str) str`** — *`@native`*

Return a copy of the string with `suffix` removed
if the string ends with it, otherwise return the string unchanged.

<a id="str.replace"></a>

**`replace(old str, new str) str`** — *`@native`*

Return a copy of the string with all occurrences of `old` replaced by `new`.

<a id="str.zfill"></a>

**`zfill(width int) str`** — *`@native`*

Return a copy of the string left-padded with zeros to the given `width`.
If `width` is not greater than the string length, the string is returned unchanged.

<a id="str.ljust"></a>

**`ljust(width int, fill = " ") str`** — *`@native`*

Return the string left-justified in a field of the given `width`,
padded with `fill` (default space). If `width` is not greater than
the string length, the string is returned unchanged.

<a id="str.rjust"></a>

**`rjust(width int, fill = " ") str`** — *`@native`*

Return the string right-justified in a field of the given `width`,
padded with `fill` (default space). If `width` is not greater than
the string length, the string is returned unchanged.

<a id="str.center"></a>

**`center(width int, fill = " ") str`** — *`@native`*

Return the string centered in a field of the given `width`,
padded with `fill` (default space). If `width` is not greater than
the string length, the string is returned unchanged.

<a id="str.isdigit"></a>

**`isdigit() bool`** — *`@native`*

Return true if all characters in the string are digits.

<a id="str.isalpha"></a>

**`isalpha() bool`** — *`@native`*

Return true if all characters in the string are alphabetic.

<a id="str.isalnum"></a>

**`isalnum() bool`** — *`@native`*

Return true if all characters in the string are alphanumeric.

<a id="str.isspace"></a>

**`isspace() bool`** — *`@native`*

Return true if all characters in the string are whitespace.

<a id="str.islower"></a>

**`islower() bool`** — *`@native`*

Return true if all cased characters in the string are lowercase.

<a id="str.isupper"></a>

**`isupper() bool`** — *`@native`*

Return true if all cased characters in the string are uppercase.

<a id="striter"></a>

### `StrIter`

```kl
pub class StrIter : Iterator[str] {
    let step int
    let source str
    let length int
    var cursor int
    pub func __init__(s str, _step = 1)
    pub func has_next() bool
    pub func next_strict() str
    pub func next() str?
    pub func __str__() str
}
```
**Bases:** [`Iterator[str]`](#iterator)


An iterator over the characters of a string, from left to right.

`StrIter` provides sequential access to each character in UTF-8 order.

The iterator is stateful: it maintains an internal cursor that
advances by `step` on each call to `next()`. A `step` greater than 1
produces a strided iteration.

Once the cursor reaches or exceeds the string length, `has_next()`
returns false. `next()` returns `nil`, while `next_strict()` triggers a panic.

Notes

- Characters are returned as `str` (single-character strings).
- The iterator does not allocate; it reads directly from the source string.
- Iterators are not resettable; create a new iterator to iterate again.
- Strided iteration is useful for sampling, scanning, and simple parsing.

---

**Fields**

<a id="striter.step"></a>

**`let step int`**

<a id="striter.source"></a>

**`let source str`**

<a id="striter.length"></a>

**`let length int`**

<a id="striter.cursor"></a>

**`var cursor int`**

<a id="striter.__init__"></a>

**`__init__(s str, _step = 1)`**

Initialize a forward iterator over `s` with an optional stride `_step`.

The cursor starts at index 0 and advances by `step` on each iteration.

<a id="striter.has_next"></a>

**`has_next() bool`**

Return true if there are remaining characters to iterate.

<a id="striter.next_strict"></a>

**`next_strict() str`**

Return the next character and advance the cursor by `step`.

Errors

Panic if the iterator is exhausted.

<a id="striter.next"></a>

**`next() str?`**

Return the next character, or `nil` if exhausted.

<a id="striter.__str__"></a>

**`__str__() str`**

Return a human-readable description of the iterator state.

Useful for debugging and logging.

<a id="strreviter"></a>

### `StrRevIter`

```kl
pub class StrRevIter : Iterator[str] {
    let step int
    let source str
    let length int
    var cursor int
    pub func __init__(s str, _step = 1)
    pub func has_next() bool
    pub func next_strict() str
    pub func next() str?
    pub func __str__() str
}
```
**Bases:** [`Iterator[str]`](#iterator)


An iterator over the characters of a string, from right to left.

`StrRevIter` provides reverse sequential access to each character in UTF-8 order.

The iterator is stateful: it maintains an internal cursor that
moves backward by `step` on each call to `next()`. A `step` greater
than 1 produces a strided reverse iteration.

Once the cursor moves before the beginning of the string, `has_next()`
returns false. `next()` returns `nil`, while `next_strict()` triggers a panic.

Notes

- Characters are returned as `str` (single-character strings).
- The iterator does not allocate; it reads directly from the source string.
- Iterators are not resettable; create a new iterator to iterate again.

---

**Fields**

<a id="strreviter.step"></a>

**`let step int`**

<a id="strreviter.source"></a>

**`let source str`**

<a id="strreviter.length"></a>

**`let length int`**

<a id="strreviter.cursor"></a>

**`var cursor int`**

<a id="strreviter.__init__"></a>

**`__init__(s str, _step = 1)`**

Initialize a reverse iterator over `s` with an optional stride `_step`.

The cursor starts at the last character and moves backward by `step`.

<a id="strreviter.has_next"></a>

**`has_next() bool`**

Return true if there are remaining characters to iterate.

<a id="strreviter.next_strict"></a>

**`next_strict() str`**

Return the next character and move the cursor backward by `step`.

Errors

Panic if the iterator is exhausted.

<a id="strreviter.next"></a>

**`next() str?`**

Return the next character in reverse order, or `nil` if exhausted.

<a id="strreviter.__str__"></a>

**`__str__() str`**

Return a human-readable description of the iterator state.

<a id="tuple"></a>

### `tuple`

```kl
pub class tuple[infer T] : Sequence[T] {
    pub func __init__(args ...T)
    pub func __iter__() Iterator[T]
    pub func __len__() int
    pub func empty() bool
    pub func __getitem__[infer U](index int) U
    pub func __getslice__(r slice) tuple[T]
    pub func __contains__(v T) bool
    pub func index(value T, start = 0, end = -1) int
    pub func rindex(value T, start = 0, end = -1) int
    pub func count(value T, start = 0, end = -1) int
    pub func __str__() str
    pub func to_list() list[T]
    pub func __add__(other tuple[T]) tuple[T]
    pub func reversed() Iterator[T]
}
```
**Bases:** [`Sequence[T]`](#sequence)


A tuple is an immutable sequence of elements.

It implements `Sequence`, providing fixed-size, read-only access:
random access, searching and slicing.

---

<a id="tuple.__init__"></a>

**`__init__(args ...T)`** — *`@native`*

Initialize a tuple with the given elements.

<a id="tuple.__iter__"></a>

**`__iter__() Iterator[T]`** — *`@native`*

Return an iterator over the tuple.

<a id="tuple.__len__"></a>

**`__len__() int`** — *`@native`*

Return the number of elements in the tuple. O(1).

<a id="tuple.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the tuple is empty, false otherwise. O(1).

<a id="tuple.__getitem__"></a>

**`__getitem__[infer U](index int) U`** — *`@native`*

Get the element at the specified index.
The element type is inferred from the usage context.

Errors

Panic if the index is out of range.

<a id="tuple.__getslice__"></a>

**`__getslice__(r slice) tuple[T]`** — *`@native`*

Return the sub-tuple defined by the slice.

<a id="tuple.__contains__"></a>

**`__contains__(v T) bool`** — *`@native`*

Return true if the tuple contains the specified value `v`. O(n).

<a id="tuple.index"></a>

**`index(value T, start = 0, end = -1) int`** — *`@native`*

Return the first index of `value` in `[start, end)`. O(n).
If `end` is `-1`, the search extends to the end of the tuple.
Return `-1` if the value is not present.

<a id="tuple.rindex"></a>

**`rindex(value T, start = 0, end = -1) int`** — *`@native`*

Return the last index of `value` in `[start, end)`. O(n).
If `end` is `-1`, the search extends to the end of the tuple.
Return `-1` if the value is not present.

<a id="tuple.count"></a>

**`count(value T, start = 0, end = -1) int`** — *`@native`*

Return the number of occurrences of `value` in `[start, end)`. O(n).
If `end` is `-1`, the search extends to the end of the tuple.

<a id="tuple.__str__"></a>

**`__str__() str`** — *`@native`*

Return the string representation of the tuple.

<a id="tuple.to_list"></a>

**`to_list() list[T]`** — *`@native`*

Return a new list containing the elements of the tuple.

<a id="tuple.__add__"></a>

**`__add__(other tuple[T]) tuple[T]`** — *`@native`*

Return a new tuple with elements of `self` followed by `other` (`self + other`).

<a id="tuple.reversed"></a>

**`reversed() Iterator[T]`** — *`@native`*

Return a reverse iterator over the elements of the tuple.

<a id="type"></a>

### `type`

```kl
pub class type {
    func __init__()
    pub func __str__() str
    pub func name() str
    pub func methods() list[str]
    pub func lro() list[str]
}
```

The `type` class represents the runtime type of a value.

Type objects are produced only by the runtime via [`typeof`](#typeof)
and cannot be created directly.

---

<a id="type.__init__"></a>

**`__init__()`**

Type objects are created only by the runtime.

<a id="type.__str__"></a>

**`__str__() str`** — *`@native`*

Return the string representation of the type.
Uses the form `"<class 'name'>"`, for example `"<class 'str'>"`.

<a id="type.name"></a>

**`name() str`** — *`@native`*

Return the name of the type.
For types outside `std/builtin`, the name is prefixed with the module path.

<a id="type.methods"></a>

**`methods() list[str]`** — *`@native`*

Return the names of all methods of the type.

<a id="type.lro"></a>

**`lro() list[str]`** — *`@native`*

Return the names of the traits conformed to by the type,
in Linearized Resolution Order.

## Functions

<a id="print"></a>

### `print`

```kl
pub func print(objs ..., sep = " ", end = "\n")
```

*`@native`*

Prints objects separated by `sep` and followed by `end`.

Objects are converted to strings using their `__str__` method.
If no objects are given, `print()` writes only `end`.
The default separator is a space, and the default ending is a newline.

Examples

```kl
print("red", "green", "blue", sep = ", ")
```

<a id="len"></a>

### `len`

```kl
pub func len(obj any) int
```

*`@intrinsic`*

Return the number of items in a collection or the length of an object.

This function is an intrinsic and is resolved at compile time.
Semantically, `len(obj)` is equivalent to calling `obj.__len__()`.

Any object may define a `__len__()` method to customize its length behavior.
Built-in container types such as list, dict, tuple, and set provide their own
optimized implementations.

Notes

- As an intrinsic, `len()` does not have a user-level implementation.
- The compiler may lower `len(obj)` to specialized VM instructions
  (e.g., `OP_LEN`) when the object's type is known,
  enabling high-performance length queries.

<a id="hash"></a>

### `hash`

```kl
pub func hash(obj any) int
```

*`@intrinsic`*

Return the hash value of an object.

This function is an intrinsic and is resolved at compile time.
Semantically, `hash(obj)` is equivalent to calling `obj.__hash__()`.

Any class automatically provides a `__hash__()` method, and user-defined
types may override it to customize hashing behavior.

Notes

- As an intrinsic, `hash()` does not have a user-level implementation.
- The compiler may lower `hash(obj)` to a specialized VM instruction
  (e.g., `OP_HASH`) when the object's type is known, enabling efficient
  hashing for built-in and user-defined types.

<a id="panic"></a>

### `panic`

```kl
pub func panic(msg str)
```

*`@native`*

Immediately abort program execution with an error message.

`panic(msg)` raises a fatal runtime error and terminates the current
execution context. The function does not return.

This is a native builtin function implemented directly by the Koala VM.
It is used by the standard library and compiler-generated code to signal
unrecoverable conditions (such as failed assertions, type errors, or
internal invariants).

Notes

- `panic()` is an ordinary native function implemented in C.
- The VM may include additional diagnostic information such as
  stack traces or source locations when reporting the panic.
- User code should prefer returning `T?` or error values when
  appropriate; `panic()` is intended only for fatal errors.

<a id="format"></a>

### `format`

```kl
pub func format(fmt str, args ...) str
```

*`@native`*

Format a string using positional arguments.

`{}` consumes one argument in order. No named placeholders.
Arguments are converted using `__str__()`.

Examples

```kl
let text = format("Point({}, {})", 10, 20)
print(text) // Point(10, 20)
```

<a id="typeof"></a>

### `typeof`

```kl
pub func typeof(obj any) type
```

*`@native`*

Return the runtime type of a value.

Examples

```kl
let value_type = typeof(10)
print(value_type)
```

<a id="abs"></a>

### `abs`

```kl
pub func abs[T](a T) T
```

Computes the absolute value using a matching specialization.

Errors

Panic if execution reaches the unspecialized fallback.

<a id="abs_int64"></a>

### `abs_int64`

```kl
pub func abs_int64(a int) int
```

Specialized explicit implementation of `abs()` for 64-bit signed integers.

If user writes `abs(10)`, the compiler will route the call to `abs_int64(10)`.

<a id="abs_float64"></a>

### `abs_float64`

```kl
pub func abs_float64(a float) float
```

Specialized explicit implementation of `abs()` for 64-bit floating-point numbers.

If user writes `abs(10.0)`, the compiler will route the call to `abs_float64(10.0)`.

<a id="min"></a>

### `min`

```kl
pub func min[T : Comparable](x T, y T) T
```

*`@specialized(int, uint, float)`*

Constrained generic minimum function, auto-specialized for primitives.

<a id="max"></a>

### `max`

```kl
pub func max[T : Comparable](x T, y T) T
```

*`@specialized(int, uint, float)`*

Constrained generic maximum function, auto-specialized for primitives.

This will generate the specialized `max_int64`, `max_uint64`, and `max_float64`
functions. If the user writes `max(10, 20)`, the compiler will route the call to
`max_int64(10, 20)`.

<a id="pow"></a>

### `pow`

```kl
pub func pow[T, U](base T, exp U) any
```

Computes a power using a matching specialization.

To extend support for custom types, implement a standalone function named
`pow_<base_type>_<exp_type>`.

Errors

Panic if execution reaches the unspecialized fallback.

<a id="pow_int64_int64"></a>

### `pow_int64_int64`

```kl
pub func pow_int64_int64(base int, exp int)
```

*`@native`*

Specialized explicit implementation for 64-bit integer base and exponent.

Uses an optimized binary exponentiation (exponentiation by squaring) algorithm.
If user writes `pow(2, 10)`, the compiler will route the call to
`pow_int64_int64(2, 10)`.

<a id="pow_float64_int64"></a>

### `pow_float64_int64`

```kl
pub func pow_float64_int64(base float, exp int) float
```

*`@native`*

Specialized explicit implementation for float base and integer exponent.

Uses fast exponentiation by squaring, allowing optimal register usage in the VM.
If user writes `pow(2.0, 10)`, the compiler will route the call to
`pow_float64_int64(2.0, 10)`.

<a id="pow_float64_float64"></a>

### `pow_float64_float64`

```kl
pub func pow_float64_float64(base float, exp float) float
```

*`@native`*

Specialized explicit implementation for float base and float exponent.

This is a native fallback that routes directly to the VM's high-performance
C math library. If user writes `pow(2.0, 10.0)`, the compiler will route the call to
`pow_float64_float64(2.0, 10.0)`.
