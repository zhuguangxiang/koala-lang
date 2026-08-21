# Koala `std/builtin` API Reference

## Contents

**Traits**

| Name | Summary |
|------|---------|
| [`any`](#any) | The `any` trait is the root of the Koala type hierarchy. |
| [`Equatable`](#equatable) | Equatable defines semantic equality between two values of the same type. |
| [`Comparable`](#comparable) | Comparable defines a total ordering between values of the same type. |
| [`Hashable`](#hashable) | Hashable defines the ability for a type to produce a stable hash value. |
| [`Printable`](#printable) | Printable defines a human-readable string representation for a type, used by `print()` and other display mechanisms. |
| [`Collection`](#collection) | Base trait for all collections. |
| [`Sequence`](#sequence) | A Sequence is a read-only, indexable collection of elements. |
| [`MutableSequence`](#mutablesequence) | A MutableSequence is an ordered collection that supports modification. |
| [`Map`](#map) | A Map is an object that maps keys to values. |
| [`Set`](#set) | A collection that contains no duplicate elements. |
| [`Iterator`](#iterator) | An Iterator represents a position in a sequence of elements. |
| [`Iterable`](#iterable) | The Iterable trait is the basis for all objects that can be looped over. |
| [`Arithmetic`](#arithmetic) | Trait for basic arithmetic operations. |
| [`BitwiseOperators`](#bitwiseoperators) | Trait for bitwise operations. |

**Classes**

| Name | Summary |
|------|---------|
| [`bool`](#bool) | Represents a logical value that can be either true or false. |
| [`ByteBuf`](#bytebuf) | A ByteBuf is a variable-length, readable and writable byte buffer. |
| [`bytes`](#bytes) | A bytes is a fixed-size, readable and writable byte array. |
| [`dict`](#dict) | A dict is a mutable, insertion-ordered hash map that maps unique keys to values. |
| [`list`](#list) | A list is a mutable sequence of elements. |
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
| [`slice`](#slice) | A slice object represents a set of indices specified by [start:end:step]. |
| [`str`](#str) | A str is an immutable sequence of characters. |
| [`tuple`](#tuple) | A tuple is an immutable sequence of elements. |
| [`type`](#type) | The type class represents the runtime type of a value. |

**Functions**

| Name | Summary |
|------|---------|
| [`print`](#print) | Print objects to the text stream file, separated by `sep` and followed by `end`. |
| [`len`](#len) | Return the number of items in a collection or the length of an object. |
| [`panic`](#panic) | Immediately abort program execution with an error message. |
| [`format`](#format) | Format a string using positional arguments. |
| [`typeof`](#typeof) | Return the runtime type of a value. |

## Traits

### `any`

```kl
pub trait any {}
```

The [`any`](#any) trait is the root of the Koala type hierarchy. Every class and trait implicitly inherits from [`any`](#any). It defines no methods and imposes no behavioral requirements. It exists solely as the universal supertype for all Koala values.

### `Equatable`

```kl
pub trait Equatable[T] {
    func __eq__(other T) bool
    func __ne__(other T) bool
}
```

Equatable defines semantic equality between two values of the same type. The type parameter T is the type compared against, which is the conforming type itself.

Types conforming to Equatable must provide both `__eq__` and `__ne__`, ensuring consistent behavior for equality and inequality checks.

Conforming to Equatable allows a type to be used in conditional logic, comparisons, and other equality-based operations.

---

<a id="equatable.__eq__"></a>

**`__eq__(other T) bool`**

Return true if `self` is equal to `other` (`self == other`).

<a id="equatable.__ne__"></a>

**`__ne__(other T) bool`**

Return true if `self` is not equal to `other` (`self != other`).

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


Comparable defines a total ordering between values of the same type. The type parameter T is the type compared against, which is the conforming type itself. Types conforming to Comparable can be sorted, compared, and used in ordered data structures such as priority queues or tree-based collections.

Comparable inherits from Equatable, so conforming types must also implement equality and inequality operations.

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

### `Hashable`

```kl
pub trait Hashable {
    func __hash__() int
}
```

Hashable defines the ability for a type to produce a stable hash value. Conforming to Hashable allows a type to be used as a key in dictionaries, sets, and other hash-based collections.

Equal values must always produce identical hash values. The hash should be well-distributed to ensure good performance in hashed structures.

---

<a id="hashable.__hash__"></a>

**`__hash__() int`**

Return a stable hash value for `self`.

Equal values must always produce identical hash values.

### `Printable`

```kl
pub trait Printable {
    func __str__() str
}
```

Printable defines a human-readable string representation for a type, used by [`print()`](#print) and other display mechanisms. It is a display contract, never a content conversion (e.g. [`bytes`](#bytes) displays its hexadecimal form, while decoding into a [`str`](#str) is a separate `to_str()` method).

The returned string should be suitable for display to end users and represent the value in a clear, readable form.

---

<a id="printable.__str__"></a>

**`__str__() str`**

Return a human-readable string representation of `self`.

### `Collection`

```kl
pub trait Collection[T] : Iterable[T] {
    func __len__() int
    func empty() bool
}
```
**Supertraits:** [`Iterable[T]`](#iterable)


Base trait for all collections. Represents a group of objects, known as its elements.

---

<a id="collection.__len__"></a>

**`__len__() int`**

Return the number of items in the collection.

<a id="collection.empty"></a>

**`empty() bool`**

Return true if the collection is empty, false otherwise.

### `Sequence`

```kl
pub trait Sequence[T] : Collection[T] {
    func __getitem__(index int) T
    func __getslice__(r slice) Sequence[T]
    func __contains__(v T) bool
    func index(value T, start = 0, end = -1) int
    func rindex(value T, start = 0, end = -1) int
    func count(value T, start = 0, end = -1) int
}
```
**Supertraits:** [`Collection[T]`](#collection)


A Sequence is a read-only, indexable collection of elements.

This trait is the standard internal type for variadic arguments (...T). It provides random access and searching capabilities without modification.

---

<a id="sequence.__getitem__"></a>

**`__getitem__(index int) T`**

Get the item at the specified index.

**Precondition:** 0 <= index < __len__().

Panic if the index is out of range.

<a id="sequence.__getslice__"></a>

**`__getslice__(r slice) Sequence[T]`**

Return a shallow copy of a portion of the sequence defined by the slice.

<a id="sequence.__contains__"></a>

**`__contains__(v T) bool`**

Return true if the sequence contains the specified value `v`. This is typically an O(n) operation.

<a id="sequence.index"></a>

**`index(value T, start = 0, end = -1) int`**

Return the first index of `value` in [start, end). If `end` is -1, the search extends to the end of the sequence. Return -1 if the value is not present.

<a id="sequence.rindex"></a>

**`rindex(value T, start = 0, end = -1) int`**

Return the last index of `value` in [start, end). If `end` is -1, the search extends to the end of the sequence. Return -1 if the value is not present.

<a id="sequence.count"></a>

**`count(value T, start = 0, end = -1) int`**

Return the number of occurrences of `value` in [start, end). If `end` is -1, the search extends to the end of the sequence.

### `MutableSequence`

```kl
pub trait MutableSequence[T] : Sequence[T] {
    func __setitem__(index int, value T)
    func __setslice__(r slice, val Iterable[T])
    func append(value T)
    func extend(items Iterable[T])
    func insert(index int, value T)
    func remove(value T)
    func pop(index = -1) T
    func clear()
    func reverse()
}
```
**Supertraits:** [`Sequence[T]`](#sequence)


A MutableSequence is an ordered collection that supports modification.

It extends the Sequence trait by adding methods to update, insert, and remove elements, such as those found in a dynamic list.

---

<a id="mutablesequence.__setitem__"></a>

**`__setitem__(index int, value T)`**

Set the item at the specified index to `value`.

**Precondition:** 0 <= index < __len__().

<a id="mutablesequence.__setslice__"></a>

**`__setslice__(r slice, val Iterable[T])`**

Replace a slice of the sequence with elements from the iterable `val`. This may change the length of the sequence.

<a id="mutablesequence.append"></a>

**`append(value T)`**

Append a new item with `value` to the end of the sequence.

<a id="mutablesequence.extend"></a>

**`extend(items Iterable[T])`**

Extend the sequence by appending all items from the iterable `items`.

<a id="mutablesequence.insert"></a>

**`insert(index int, value T)`**

Insert a new item with `value` at the given `index`. Elements at or after the index are shifted to the right.

<a id="mutablesequence.remove"></a>

**`remove(value T)`**

Remove the first occurrence of `value` from the sequence. Panic if the value is not present.

<a id="mutablesequence.pop"></a>

**`pop(index = -1) T`**

Remove and return the item at the given `index`. If no index is specified, removes and returns the last item (-1).

<a id="mutablesequence.clear"></a>

**`clear()`**

Remove all items from the sequence, leaving it empty.

<a id="mutablesequence.reverse"></a>

**`reverse()`**

Reverse the elements of the sequence in place.

### `Map`

```kl
pub trait Map[K, V] : Collection[(K, V)] {
    func __contains__(key K) bool
    func __getsub__(key K) V
    func __setsub__(key K, value V)
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
**Supertraits:** [`Collection[(K, V)]`](#collection)


A Map is an object that maps keys to values. A map cannot contain duplicate keys; each key can map to at most one value.

---

<a id="map.__contains__"></a>

**`__contains__(key K) bool`**

Return true if the map contains a mapping for the specified key.

<a id="map.__getsub__"></a>

**`__getsub__(key K) V`**

Return the value to which the specified key is mapped. Panic if the key is not present.

<a id="map.__setsub__"></a>

**`__setsub__(key K, value V)`**

Associate the specified value with the specified key in this map.

<a id="map.keys"></a>

**`keys() Sequence[K]`**

Return a list of all keys in the map.

<a id="map.values"></a>

**`values() Sequence[V]`**

Return a list of all values in the map.

<a id="map.items"></a>

**`items() Sequence[(K, V)]`**

Return a list of all key-value pairs (tuples) in the map.

<a id="map.clear"></a>

**`clear()`**

Remove all of the mappings from the map, leaving it empty.

<a id="map.remove"></a>

**`remove(key K) V`**

Remove the mapping for a key from this map if it is present. Panic if the key is not present.

<a id="map.remove_or"></a>

**`remove_or(key K, default_value V) V`**

Remove the key and return its value. If key is not found, return default_value.

<a id="map.get"></a>

**`get(key K) V?`**

Return the value for key if key is in the map, else null. Unlike `__getsub__` (`map[key]`), this never panics and is meant to be used with `if let` unwrapping.

<a id="map.get_or"></a>

**`get_or(key K, default_value V) V`**

Return the value for key if key is in the map, else return default_value.

### `Set`

```kl
pub trait Set[T] : Collection[T] {
    func __contains__(v T) bool
    func add(v T) bool
    func extend(items Iterable[T])
    func remove(v T) bool
    func clear()
    func is_subset(other Iterable[T]) bool
    func is_superset(other Iterable[T]) bool
    func union(other Iterable[T]) Set[T]
    func intersect(other Iterable[T]) Set[T]
    func diff(other Iterable[T]) Set[T]
}
```
**Supertraits:** [`Collection[T]`](#collection)


A collection that contains no duplicate elements. Models the mathematical set abstraction.

---

<a id="set.__contains__"></a>

**`__contains__(v T) bool`**

Return true if the set contains the specified element.

<a id="set.add"></a>

**`add(v T) bool`**

Add an element to the set. Return true if the element was not already present.

<a id="set.extend"></a>

**`extend(items Iterable[T])`**

Add all elements from the iterable to the set.

<a id="set.remove"></a>

**`remove(v T) bool`**

Remove an element from the set. Return true if the element was present.

<a id="set.clear"></a>

**`clear()`**

Remove all elements from the set.

<a id="set.is_subset"></a>

**`is_subset(other Iterable[T]) bool`**

Return true if every element in the set is in `other`.

<a id="set.is_superset"></a>

**`is_superset(other Iterable[T]) bool`**

Return true if every element in `other` is in the set.

<a id="set.union"></a>

**`union(other Iterable[T]) Set[T]`**

Return a new set with elements from the set and the other collection.

<a id="set.intersect"></a>

**`intersect(other Iterable[T]) Set[T]`**

Return a new set with elements common to the set and the other collection.

<a id="set.diff"></a>

**`diff(other Iterable[T]) Set[T]`**

Return a new set with elements in the set that are not in the other collection.

### `Iterator`

```kl
pub trait Iterator[T] {
    func __has_next__() bool
    func __next__() T
}
```

An Iterator represents a position in a sequence of elements. It provides a safe way to traverse collections through a check-then-act pattern.

---

<a id="iterator.__has_next__"></a>

**`__has_next__() bool`**

Return true if the iteration has more elements, false otherwise. This method must be checked before every call to `__next__()`.

<a id="iterator.__next__"></a>

**`__next__() T`**

Return the next item from the iterator.

PRECONDITION: `__has_next__()` must return true before calling this method. If called when `__has_next__()` is false, it will trigger a panic.

### `Iterable`

```kl
pub trait Iterable[T] {
    func __iter__() Iterator[T]
}
```

The Iterable trait is the basis for all objects that can be looped over. It provides lazy functional operators for efficient data processing.

---

<a id="iterable.__iter__"></a>

**`__iter__() Iterator[T]`**

Return a new iterator object to begin a traversal of the elements.

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

Provides the fundamental binary arithmetic operators (`+`, `-`, `*`, `/`, `%`) shared by all numeric types.

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

### `BitwiseOperators`

```kl
pub trait BitwiseOperators[T] {
    func __lsh__(rhs T) T
    func __rsh__(rhs T) T
    func __bitand__(rhs T) T
    func __bitor__(rhs T) T
    func __bitxor__(rhs T) T
    func __bitnot__() T
}
```

Trait for bitwise operations.

Provides shift and bitwise logical operators. Only integer types implement this trait; floating-point types do not support bitwise operations.

---

<a id="bitwiseoperators.__lsh__"></a>

**`__lsh__(rhs T) T`**

Left shift (`self << rhs`).

<a id="bitwiseoperators.__rsh__"></a>

**`__rsh__(rhs T) T`**

Right shift (`self >> rhs`).

<a id="bitwiseoperators.__bitand__"></a>

**`__bitand__(rhs T) T`**

Bitwise AND (`self & rhs`).

<a id="bitwiseoperators.__bitor__"></a>

**`__bitor__(rhs T) T`**

Bitwise OR (`self | rhs`).

<a id="bitwiseoperators.__bitxor__"></a>

**`__bitxor__(rhs T) T`**

Bitwise XOR (`self ^ rhs`).

<a id="bitwiseoperators.__bitnot__"></a>

**`__bitnot__() T`**

Bitwise NOT (`~self`).

## Classes

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

Represents a logical value that can be either true or false. Boolean types are typically used in conditional statements and logical operations.

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

Return the string representation of the boolean. Returns "true" if true, and "false" if false.

### `ByteBuf`

```kl
pub class ByteBuf : MutableSequence[uint8] {
    pub func __init__(size = 0)
    pub func __iter__() Iterator[uint8]
    pub func __len__() int
    pub func empty() bool
    pub func __getitem__(index int) uint8
    pub func __getslice__(r slice) bytes
    pub func __contains__(value uint8) bool
    pub func index(value uint8, start = 0, end = -1) int
    pub func rindex(value uint8, start = 0, end = -1) int
    pub func count(value uint8, start = 0, end = -1) int
    pub func __setitem__(index int, value uint8)
    pub func __setslice__(r slice, val Iterable[uint8])
    pub func append(value uint8)
    pub func extend(items Iterable[uint8])
    pub func insert(index int, value uint8)
    pub func remove(value uint8)
    pub func pop(index = -1) uint8
    pub func clear()
    pub func reverse()
    pub func __str__() str
    pub func append_bytes(value bytes, start = 0, end = -1)
    pub func append_str(value str, start = 0, end = -1)
    pub func fill(value uint8, start = 0, end = -1)
    pub func zero(start = 0, end = -1)
    pub func find(sub bytes, start = 0, end = -1) int
    pub func rfind(sub bytes, start = 0, end = -1) int
    pub func starts_with(prefix bytes) bool
    pub func ends_with(suffix bytes) bool
    pub func copy(src bytes, start = 0, end = -1) int
    pub func to_bytes() bytes
    pub func to_str() str
    pub func reversed() Iterator[uint8]
}
```
**Bases:** [`MutableSequence[uint8]`](#mutablesequence)


A ByteBuf is a variable-length, readable and writable byte buffer.

---

<a id="bytebuf.__init__"></a>

**`__init__(size = 0)`** — *`@native`*

Create a byte buffer of the initial `size`, filled with zeros.

<a id="bytebuf.__iter__"></a>

**`__iter__() Iterator[uint8]`** — *`@native`*

Return an iterator over the bytes.

<a id="bytebuf.__len__"></a>

**`__len__() int`** — *`@native`*

Return the length of the buffer. O(1).

<a id="bytebuf.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the buffer is empty, false otherwise. O(1).

<a id="bytebuf.__getitem__"></a>

**`__getitem__(index int) uint8`** — *`@intrinsic`*

Return the byte at the specified index. O(1). Panic if the index is out of range.

<a id="bytebuf.__getslice__"></a>

**`__getslice__(r slice) bytes`** — *`@native`*

Return a copy of the portion of the buffer defined by the slice, as a fixed-size bytes snapshot.

<a id="bytebuf.__contains__"></a>

**`__contains__(value uint8) bool`** — *`@native`*

Return true if the buffer contains the byte `value`. O(n).

<a id="bytebuf.index"></a>

**`index(value uint8, start = 0, end = -1) int`** — *`@native`*

Return the first index of `value` in [start, end). O(n). If `end` is -1, the search extends to the end of the buffer. Return -1 if the value is not present.

<a id="bytebuf.rindex"></a>

**`rindex(value uint8, start = 0, end = -1) int`** — *`@native`*

Return the last index of `value` in [start, end). O(n). If `end` is -1, the search extends to the end of the buffer. Return -1 if the value is not present.

<a id="bytebuf.count"></a>

**`count(value uint8, start = 0, end = -1) int`** — *`@native`*

Return the number of occurrences of `value` in [start, end). O(n). If `end` is -1, the search extends to the end of the buffer.

<a id="bytebuf.__setitem__"></a>

**`__setitem__(index int, value uint8)`** — *`@intrinsic`*

Set the byte at the specified index to `value`. O(1). Panic if the index is out of range.

<a id="bytebuf.__setslice__"></a>

**`__setslice__(r slice, val Iterable[uint8])`** — *`@native`*

Replace a slice of the buffer with the elements from `val`. This may change the length of the buffer.

<a id="bytebuf.append"></a>

**`append(value uint8)`** — *`@native`*

Append a byte `value` to the end of the buffer.

<a id="bytebuf.extend"></a>

**`extend(items Iterable[uint8])`** — *`@native`*

Extend the buffer by appending all bytes from `items`.

<a id="bytebuf.insert"></a>

**`insert(index int, value uint8)`** — *`@native`*

Insert a byte `value` at the given `index`. Bytes at or after the index are shifted to the right.

<a id="bytebuf.remove"></a>

**`remove(value uint8)`** — *`@native`*

Remove the first occurrence of `value` from the buffer. Panic if the value is not present.

<a id="bytebuf.pop"></a>

**`pop(index = -1) uint8`** — *`@native`*

Remove and return the byte at the given `index`. If no index is specified, removes and returns the last byte (-1).

<a id="bytebuf.clear"></a>

**`clear()`** — *`@native`*

Remove all bytes from the buffer, leaving it empty.

<a id="bytebuf.reverse"></a>

**`reverse()`** — *`@native`*

Reverse the bytes in place.

<a id="bytebuf.__str__"></a>

**`__str__() str`** — *`@native`*

Return the hexadecimal string representation of the buffer.

<a id="bytebuf.append_bytes"></a>

**`append_bytes(value bytes, start = 0, end = -1)`** — *`@native`*

Append the bytes of `value` in [start, end) to the end of the buffer. If `end` is -1, the append extends to the end of `value`.

<a id="bytebuf.append_str"></a>

**`append_str(value str, start = 0, end = -1)`** — *`@native`*

Append the UTF-8 encoded bytes of `value` in [start, end) to the end of the buffer. If `end` is -1, the append extends to the end of `value`.

<a id="bytebuf.fill"></a>

**`fill(value uint8, start = 0, end = -1)`** — *`@native`*

Fill the bytes in [start, end) with `value`. If `end` is -1, the fill extends to the end of the buffer.

<a id="bytebuf.zero"></a>

**`zero(start = 0, end = -1)`** — *`@native`*

Set the bytes in [start, end) to zero. If `end` is -1, the zeroing extends to the end of the buffer.

<a id="bytebuf.find"></a>

**`find(sub bytes, start = 0, end = -1) int`** — *`@native`*

Return the first index of the sub-sequence `sub` in [start, end). O(n). If `end` is -1, the search extends to the end of the buffer. Return -1 if `sub` is not present.

<a id="bytebuf.rfind"></a>

**`rfind(sub bytes, start = 0, end = -1) int`** — *`@native`*

Return the last index of the sub-sequence `sub` in [start, end). O(n). If `end` is -1, the search extends to the end of the buffer. Return -1 if `sub` is not present.

<a id="bytebuf.starts_with"></a>

**`starts_with(prefix bytes) bool`** — *`@native`*

Return true if the buffer starts with `prefix`.

<a id="bytebuf.ends_with"></a>

**`ends_with(suffix bytes) bool`** — *`@native`*

Return true if the buffer ends with `suffix`.

<a id="bytebuf.copy"></a>

**`copy(src bytes, start = 0, end = -1) int`** — *`@native`*

Copy the bytes of `src` in [start, end) into `self` from the beginning, overwriting existing content. The length of the buffer never changes. Return the number of bytes copied. Return -1 if `end - start` exceeds the length of `self`.

<a id="bytebuf.to_bytes"></a>

**`to_bytes() bytes`** — *`@native`*

Return a fixed-size bytes snapshot of the buffer.

<a id="bytebuf.to_str"></a>

**`to_str() str`** — *`@native`*

Decode the buffer as UTF-8 and return a new str. Panic if the bytes are not valid UTF-8.

<a id="bytebuf.reversed"></a>

**`reversed() Iterator[uint8]`** — *`@native`*

Return a reverse iterator over the bytes.

### `bytes`

```kl
pub class bytes : Sequence[uint8] {
    pub func __init__(size int)
    pub func __iter__() Iterator[uint8]
    pub func __len__() int
    pub func empty() bool
    pub func __getitem__(index int) uint8
    pub func __getslice__(r slice) bytes
    pub func __contains__(value uint8) bool
    pub func index(value uint8, start = 0, end = -1) int
    pub func rindex(value uint8, start = 0, end = -1) int
    pub func count(value uint8, start = 0, end = -1) int
    pub func __str__() str
    pub func __setitem__(index int, value uint8)
    pub func __setslice__(r slice, val bytes)
    pub func find(sub bytes, start = 0, end = -1) int
    pub func rfind(sub bytes, start = 0, end = -1) int
    pub func copy(src bytes, start = 0, end = -1) int
    pub func fill(value uint8, start = 0, end = -1)
    pub func zero(start = 0, end = -1)
    pub func view(start = 0, end = -1) bytes
    pub func starts_with(prefix bytes) bool
    pub func ends_with(suffix bytes) bool
    pub func reverse()
    pub func to_str() str
    pub func reversed() Iterator[uint8]
}
```
**Bases:** [`Sequence[uint8]`](#sequence)


A bytes is a fixed-size, readable and writable byte array.

It implements Sequence, providing random access, searching, slicing and sub-sequence matching. The size is determined at construction and never changes.

---

<a id="bytes.__init__"></a>

**`__init__(size int)`** — *`@native`*

Create a byte array of the given `size`, initialized to zero.

<a id="bytes.__iter__"></a>

**`__iter__() Iterator[uint8]`** — *`@native`*

Return an iterator over the bytes.

<a id="bytes.__len__"></a>

**`__len__() int`** — *`@native`*

Return the size of the byte array. O(1).

<a id="bytes.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the byte array is empty, false otherwise. O(1).

<a id="bytes.__getitem__"></a>

**`__getitem__(index int) uint8`** — *`@intrinsic`*

Return the byte at the specified index. O(1). Panic if the index is out of range.

<a id="bytes.__getslice__"></a>

**`__getslice__(r slice) bytes`** — *`@intrinsic`*

Return a copy of the portion of the byte array defined by the slice.

<a id="bytes.__contains__"></a>

**`__contains__(value uint8) bool`** — *`@native`*

Return true if the byte array contains the byte `value`. O(n).

<a id="bytes.index"></a>

**`index(value uint8, start = 0, end = -1) int`** — *`@native`*

Return the first index of `value` in [start, end). O(n). If `end` is -1, the search extends to the end of the byte array. Return -1 if the value is not present.

<a id="bytes.rindex"></a>

**`rindex(value uint8, start = 0, end = -1) int`** — *`@native`*

Return the last index of `value` in [start, end). O(n). If `end` is -1, the search extends to the end of the byte array. Return -1 if the value is not present.

<a id="bytes.count"></a>

**`count(value uint8, start = 0, end = -1) int`** — *`@native`*

Return the number of occurrences of `value` in [start, end). O(n). If `end` is -1, the search extends to the end of the byte array.

<a id="bytes.__str__"></a>

**`__str__() str`** — *`@native`*

Return the hexadecimal string representation of the bytes.

<a id="bytes.__setitem__"></a>

**`__setitem__(index int, value uint8)`** — *`@intrinsic`*

Set the byte at the specified index to `value`. O(1). Panic if the index is out of range.

<a id="bytes.__setslice__"></a>

**`__setslice__(r slice, val bytes)`** — *`@intrinsic`*

Replace a slice of the byte array with the bytes from `val`. The length of the byte array never changes.

<a id="bytes.find"></a>

**`find(sub bytes, start = 0, end = -1) int`** — *`@native`*

Return the first index of the sub-sequence `sub` in [start, end). O(n). If `end` is -1, the search extends to the end of the byte array. Return -1 if `sub` is not present.

<a id="bytes.rfind"></a>

**`rfind(sub bytes, start = 0, end = -1) int`** — *`@native`*

Return the last index of the sub-sequence `sub` in [start, end). O(n). If `end` is -1, the search extends to the end of the byte array. Return -1 if `sub` is not present.

<a id="bytes.copy"></a>

**`copy(src bytes, start = 0, end = -1) int`** — *`@native`*

Copy the bytes of `src` in [start, end) into `self` from the beginning. If `end` is -1, the copy extends to the end of `src`. Return the number of bytes copied. Return -1 if `end - start` exceeds the size of `self`.

<a id="bytes.fill"></a>

**`fill(value uint8, start = 0, end = -1)`** — *`@native`*

Fill the bytes in [start, end) with `value`. If `end` is -1, the fill extends to the end of the byte array.

<a id="bytes.zero"></a>

**`zero(start = 0, end = -1)`** — *`@native`*

Set the bytes in [start, end) to zero. If `end` is -1, the zeroing extends to the end of the byte array.

<a id="bytes.view"></a>

**`view(start = 0, end = -1) bytes`** — *`@native`*

Return a view of the bytes in [start, end), sharing the same storage. If `end` is -1, the view extends to the end of the byte array.

<a id="bytes.starts_with"></a>

**`starts_with(prefix bytes) bool`** — *`@native`*

Return true if the byte array starts with `prefix`.

<a id="bytes.ends_with"></a>

**`ends_with(suffix bytes) bool`** — *`@native`*

Return true if the byte array ends with `suffix`.

<a id="bytes.reverse"></a>

**`reverse()`** — *`@native`*

Reverse the bytes in place.

<a id="bytes.to_str"></a>

**`to_str() str`** — *`@native`*

Decode the bytes as UTF-8 and return a new str. Panic if the bytes are not valid UTF-8.

<a id="bytes.reversed"></a>

**`reversed() Iterator[uint8]`** — *`@native`*

Return a reverse iterator over the bytes.

### `dict`

```kl
pub class dict[K, V] : Map[K, V] {
    pub func __init__()
    pub func __iter__() Iterator[(K, V)]
    pub func __len__() int
    pub func empty() bool
    pub func __contains__(key K) bool
    pub func __getsub__(key K) V
    pub func __setsub__(key K, value V)
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


A dict is a mutable, insertion-ordered hash map that maps unique keys to values.

It implements Map, providing key lookup, insertion, removal, and iteration over key-value pairs. Iteration, keys(), values() and items() all follow insertion order.

---

<a id="dict.__init__"></a>

**`__init__()`** — *`@native`*

Create an empty dict.

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

<a id="dict.__getsub__"></a>

**`__getsub__(key K) V`** — *`@native`*

Return the value mapped to `key`. O(1). Panic if the key is not present.

<a id="dict.__setsub__"></a>

**`__setsub__(key K, value V)`** — *`@native`*

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

Remove the mapping for `key` and return its value. O(1). Panic if the key is not present.

<a id="dict.remove_or"></a>

**`remove_or(key K, default_value V) V`** — *`@native`*

Remove the mapping for `key` and return its value. O(1). If the key is not found, return `default_value`.

<a id="dict.get"></a>

**`get(key K) V?`** — *`@native`*

Return the value for `key` if present, else null. O(1). Unlike `__getsub__`, this never panics; use with `if let` unwrapping.

<a id="dict.get_or"></a>

**`get_or(key K, default_value V) V`** — *`@native`*

Return the value for `key` if present, else `default_value`. O(1).

<a id="dict.__str__"></a>

**`__str__() str`** — *`@native`*

Return a string representation of the dict. Format: "{key1: value1, key2: value2}".

<a id="dict.update"></a>

**`update(src Iterable[(K, V)])`** — *`@native`*

Merge all (key, value) pairs from `src` into the dict, replacing the values of existing keys.

<a id="dict.setdefault"></a>

**`setdefault(key K, default_value V) V`** — *`@native`*

Return the value for `key` if it is in the dict. Otherwise insert `key` with `default_value` and return `default_value`.

<a id="dict.peek_first"></a>

**`peek_first() (K, V)`** — *`@native`*

Return the first inserted (key, value) pair without removing it. Panic if the dict is empty.

<a id="dict.peek_last"></a>

**`peek_last() (K, V)`** — *`@native`*

Return the last inserted (key, value) pair without removing it. Panic if the dict is empty.

<a id="dict.pop_first"></a>

**`pop_first() (K, V)`** — *`@native`*

Remove and return the first inserted (key, value) pair. Panic if the dict is empty.

<a id="dict.pop_last"></a>

**`pop_last() (K, V)`** — *`@native`*

Remove and return the last inserted (key, value) pair. Panic if the dict is empty.

<a id="dict.__or__"></a>

**`__or__(src Iterable[(K, V)]) dict[K, V]`** — *`@native`*

Return a new dict with mappings of `self` followed by `src` (`self | src`). Values from `src` replace those of duplicate keys.

<a id="dict.__ior__"></a>

**`__ior__(src Iterable[(K, V)]) dict[K, V]`** — *`@native`*

Merge all (key, value) pairs from `src` into the dict in place, replacing the values of existing keys (`self |= src`).

<a id="dict.reversed"></a>

**`reversed() Iterator[(K, V)]`** — *`@native`*

Return a reverse iterator over the (key, value) pairs in reverse insertion order.

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
    pub func append(value T)
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

It implements MutableSequence, providing dynamic array functionality: random access, appending, insertion, deletion and slicing.

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

Get the item at the specified index. O(1). Panic if the index is out of range.

<a id="list.__getslice__"></a>

**`__getslice__(r slice) list[T]`** — *`@native`*

Return a new list that is a sub-slice of the current list.

<a id="list.__contains__"></a>

**`__contains__(v T) bool`** — *`@native`*

Return true if the list contains the specified value `v`. O(n).

<a id="list.index"></a>

**`index(value T, start = 0, end = -1) int`** — *`@native`*

Return the first index of `value` in [start, end). O(n). If `end` is -1, the search extends to the end of the list. Return -1 if the value is not present.

<a id="list.rindex"></a>

**`rindex(value T, start = 0, end = -1) int`** — *`@native`*

Return the last index of `value` in [start, end). O(n). If `end` is -1, the search extends to the end of the list. Return -1 if the value is not present.

<a id="list.count"></a>

**`count(value T, start = 0, end = -1) int`** — *`@native`*

Return the number of occurrences of `value` in [start, end). O(n). If `end` is -1, the search extends to the end of the list.

<a id="list.__setitem__"></a>

**`__setitem__(index int, value T)`** — *`@native`*

Set the item at the specified index to `value`. O(1). Panic if the index is out of range.

<a id="list.__setslice__"></a>

**`__setslice__(r slice, val Iterable[T])`** — *`@native`*

Replace a slice of the list with elements from the iterable `val`. This may change the length of the list.

<a id="list.append"></a>

**`append(value T)`** — *`@native`*

Append a new item with `value` to the end of the list.

<a id="list.extend"></a>

**`extend(items Iterable[T])`** — *`@native`*

Extend the list by appending all items from `items`.

<a id="list.insert"></a>

**`insert(index int, value T)`** — *`@native`*

Insert a new item with `value` at the given `index`. Elements at or after the index are shifted to the right.

<a id="list.remove"></a>

**`remove(value T)`** — *`@native`*

Remove the first occurrence of `value` from the list. Panic if the value is not present.

<a id="list.pop"></a>

**`pop(index = -1) T`** — *`@native`*

Remove and return the item at the given `index`. If no index is specified, removes and returns the last item (-1).

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

Return a copy of the list from `start` (inclusive) to `end` (exclusive). If `end` is -1, the copy extends to the end of the list.

<a id="list.reversed"></a>

**`reversed() Iterator[T]`** — *`@native`*

Return a reverse iterator over the elements of the list.

### `int8`

```kl
pub class int8 {
    pub func __init__(x int64 | uint64)
}
```

An 8-bit signed integer.

Used to truncate a wider integer value down to 8 bits. Arithmetic operations are performed via implicit promotion to [`int64`](#int64).

---

<a id="int8.__init__"></a>

**`__init__(x int64 | uint64)`** — *`@intrinsic`*

Creates an [`int8`](#int8) by truncating `x` to 8 bits.

### `int16`

```kl
pub class int16 {
    pub func __init__(x int64 | uint64)
}
```

A 16-bit signed integer.

Used to truncate a wider integer value down to 16 bits. Arithmetic operations are performed via implicit promotion to [`int64`](#int64).

---

<a id="int16.__init__"></a>

**`__init__(x int64 | uint64)`** — *`@intrinsic`*

Creates an [`int16`](#int16) by truncating `x` to 16 bits.

### `int32`

```kl
pub class int32 {
    pub func __init__(x int64 | uint64)
}
```

A 32-bit signed integer.

Used to truncate a wider integer value down to 32 bits. Arithmetic operations are performed via implicit promotion to [`int64`](#int64).

---

<a id="int32.__init__"></a>

**`__init__(x int64 | uint64)`** — *`@intrinsic`*

Creates an [`int32`](#int32) by truncating `x` to 32 bits.

### `int64`

```kl
pub class int64 : Comparable & Arithmetic & BitwiseOperators {
    pub func __init__(x int64 | uint64 | str | float64, base = 10)
    pub func __add__(rhs int64) int64
    pub func __sub__(rhs int64) int64
    pub func __mul__(rhs int64) int64
    pub func __div__(rhs int64) int64
    pub func __mod__(rhs int64) int64
    pub func __neg__() int64
    pub func __lsh__(rhs int64) int64
    pub func __rsh__(rhs int64) int64
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
    pub func __ilsh__(rhs int64) int64
    pub func __irsh__(rhs int64) int64
    pub func __ibitand__(rhs int64) int64
    pub func __ibitor__(rhs int64) int64
    pub func __ibitxor__(rhs int64) int64
    pub func pow(x int64) int64
    pub func abs() int64
    pub func to_float() float64
    pub func min(other int64) int64
    pub func max(other int64) int64
}
```
**Bases:** [`Comparable`](#comparable) & [`Arithmetic`](#arithmetic) & [`BitwiseOperators`](#bitwiseoperators)


A 64-bit signed integer.

The primary signed integer type.

---

<a id="int64.__init__"></a>

**`__init__(x int64 | uint64 | str | float64, base = 10)`** — *`@native`*

Creates an [`int64`](#int64).

The argument `x` can be an integer, a floating-point value, or a string. When `x` is a string, `base` specifies the numeric base used for parsing (default `10`).

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

<a id="int64.__lsh__"></a>

**`__lsh__(rhs int64) int64`** — *`@intrinsic`*

Left shift (`self << rhs`).

<a id="int64.__rsh__"></a>

**`__rsh__(rhs int64) int64`** — *`@intrinsic`*

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

<a id="int64.__ilsh__"></a>

**`__ilsh__(rhs int64) int64`** — *`@intrinsic`*

Left shifts `self` by `rhs` in place (`self <<= rhs`).

<a id="int64.__irsh__"></a>

**`__irsh__(rhs int64) int64`** — *`@intrinsic`*

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

Raises `self` to the power `x` (`self ** x`).

<a id="int64.abs"></a>

**`abs() int64`** — *`@native`*

Returns the absolute value of `self`.

<a id="int64.to_float"></a>

**`to_float() float64`** — *`@native`*

Converts `self` to a [`float64`](#float64).

<a id="int64.min"></a>

**`min(other int64) int64`** — *`@native`*

Returns the smaller of `self` and `other`.

<a id="int64.max"></a>

**`max(other int64) int64`** — *`@native`*

Returns the larger of `self` and `other`.

### `uint8`

```kl
pub class uint8 {
    pub func __init__(x int64 | uint64)
}
```

An 8-bit unsigned integer.

Used to truncate a wider integer value down to 8 bits. Arithmetic operations are performed via implicit promotion to [`uint64`](#uint64).

---

<a id="uint8.__init__"></a>

**`__init__(x int64 | uint64)`** — *`@intrinsic`*

Creates a [`uint8`](#uint8) by truncating `x` to 8 bits.

### `uint16`

```kl
pub class uint16 {
    pub func __init__(x int64 | uint64)
}
```

A 16-bit unsigned integer.

Used to truncate a wider integer value down to 16 bits. Arithmetic operations are performed via implicit promotion to [`uint64`](#uint64).

---

<a id="uint16.__init__"></a>

**`__init__(x int64 | uint64)`** — *`@intrinsic`*

Creates a [`uint16`](#uint16) by truncating `x` to 16 bits.

### `uint32`

```kl
pub class uint32 {
    pub func __init__(x int64 | uint64)
}
```

A 32-bit unsigned integer.

Used to truncate a wider integer value down to 32 bits. Arithmetic operations are performed via implicit promotion to [`uint64`](#uint64).

---

<a id="uint32.__init__"></a>

**`__init__(x int64 | uint64)`** — *`@intrinsic`*

Creates a [`uint32`](#uint32) by truncating `x` to 32 bits.

### `uint64`

```kl
pub class uint64 : Comparable & Arithmetic & BitwiseOperators {
    pub func __init__(x int64 | uint64 | str | float64, base = 10)
    pub func __add__(rhs uint64) uint64
    pub func __sub__(rhs uint64) uint64
    pub func __mul__(rhs uint64) uint64
    pub func __div__(rhs uint64) uint64
    pub func __mod__(rhs uint64) uint64
    pub func __lsh__(rhs uint64) uint64
    pub func __rsh__(rhs uint64) uint64
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
    pub func __ilsh__(rhs uint64) uint64
    pub func __irsh__(rhs uint64) uint64
    pub func __ibitand__(rhs uint64) uint64
    pub func __ibitor__(rhs uint64) uint64
    pub func __ibitxor__(rhs uint64) uint64
    pub func pow(x uint64) uint64
    pub func to_float() float64
    pub func min(other uint64) uint64
    pub func max(other uint64) uint64
}
```
**Bases:** [`Comparable`](#comparable) & [`Arithmetic`](#arithmetic) & [`BitwiseOperators`](#bitwiseoperators)


A 64-bit unsigned integer.

The primary unsigned integer type.

---

<a id="uint64.__init__"></a>

**`__init__(x int64 | uint64 | str | float64, base = 10)`** — *`@native`*

Creates a [`uint64`](#uint64).

The argument `x` can be an integer, a floating-point value, or a string. When `x` is a string, `base` specifies the numeric base used for parsing (default `10`).

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

<a id="uint64.__lsh__"></a>

**`__lsh__(rhs uint64) uint64`** — *`@intrinsic`*

Left shift (`self << rhs`).

<a id="uint64.__rsh__"></a>

**`__rsh__(rhs uint64) uint64`** — *`@intrinsic`*

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

<a id="uint64.__ilsh__"></a>

**`__ilsh__(rhs uint64) uint64`** — *`@intrinsic`*

Left shifts `self` by `rhs` in place (`self <<= rhs`).

<a id="uint64.__irsh__"></a>

**`__irsh__(rhs uint64) uint64`** — *`@intrinsic`*

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

Raises `self` to the power `x` (`self ** x`).

<a id="uint64.to_float"></a>

**`to_float() float64`** — *`@native`*

Converts `self` to a [`float64`](#float64).

<a id="uint64.min"></a>

**`min(other uint64) uint64`** — *`@native`*

Returns the smaller of `self` and `other`.

<a id="uint64.max"></a>

**`max(other uint64) uint64`** — *`@native`*

Returns the larger of `self` and `other`.

### `float16`

```kl
pub class float16 {
    pub func __init__(x float64)
}
```

A 16-bit floating-point number (half precision).

Used to truncate a [`float64`](#float64) value down to 16 bits. Arithmetic operations are performed via implicit promotion to [`float64`](#float64).

---

<a id="float16.__init__"></a>

**`__init__(x float64)`** — *`@intrinsic`*

Creates a [`float16`](#float16) by truncating `x` to 16 bits.

### `float32`

```kl
pub class float32 {
    pub func __init__(x float64)
}
```

A 32-bit floating-point number (single precision).

Used to truncate a [`float64`](#float64) value down to 32 bits. Arithmetic operations are performed via implicit promotion to [`float64`](#float64).

---

<a id="float32.__init__"></a>

**`__init__(x float64)`** — *`@intrinsic`*

Creates a [`float32`](#float32) by truncating `x` to 32 bits.

### `float64`

```kl
pub class float64 : Comparable & Arithmetic {
    pub func __init__(x float64 | int64 | uint64 | str)
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
    pub func min(x float64) float64
    pub func max(x float64) float64
}
```
**Bases:** [`Comparable`](#comparable) & [`Arithmetic`](#arithmetic)


A 64-bit floating-point number (double precision).

The primary floating-point type, conforming to IEEE 754.

---

<a id="float64.__init__"></a>

**`__init__(x float64 | int64 | uint64 | str)`** — *`@native`*

Creates a [`float64`](#float64) from an integer, another floating-point value, or a string.

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

Raises `self` to the power `x` (`self ** x`).

<a id="float64.to_int"></a>

**`to_int() int64`** — *`@native`*

Converts `self` to an [`int64`](#int64), truncating toward zero.

<a id="float64.is_nan"></a>

**`is_nan() bool`** — *`@native`*

Returns `true` if `self` is NaN (not a number).

<a id="float64.is_inf"></a>

**`is_inf() bool`** — *`@native`*

Returns `true` if `self` is positive or negative infinity.

<a id="float64.min"></a>

**`min(x float64) float64`** — *`@native`*

Returns the smaller of `self` and `x`.

<a id="float64.max"></a>

**`max(x float64) float64`** — *`@native`*

Returns the larger of `self` and `x`.

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

It is commonly used for looping a specific number of times in for loops. Since it implements Sequence, it supports O(1) length and indexing.

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

Initialize a range from start (inclusive) to end (exclusive).

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

Return the index of `value` in the range, restricted to [start, end). O(1). If `end` is -1, the search extends to the end of the range. Return -1 if the value is not present.

<a id="range.rindex"></a>

**`rindex(value int, _start = 0, _end = -1) int`** — *`@native`*

Return the last index of `value` in the range, restricted to [start, end). Since all values in a range are unique, this is identical to `index`. If `end` is -1, the search extends to the end of the range. Return -1 if the value is not present.

<a id="range.count"></a>

**`count(value int, _start = 0, _end = -1) int`** — *`@native`*

Return the number of occurrences of `value` in [start, end). O(1). Since all values in a range are unique, this returns 1 if the value is present, otherwise 0. If `end` is -1, the search extends to the end of the range.

<a id="range.__str__"></a>

**`__str__() str`** — *`@native`*

Return the string representation of the range. O(1). Format: "range(start, end, step)".

<a id="range.reversed"></a>

**`reversed() range`** — *`@native`*

Return a new range with the elements in reverse order. O(1).

### `HashSet`

```kl
pub class HashSet[T] : Set[T] {
    pub func __init__()
    pub func __iter__() Iterator[T]
    pub func __len__() int
    pub func empty() bool
    pub func __contains__(v T) bool
    pub func add(v T) bool
    pub func extend(items Iterable[T])
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


A HashSet is a mutable, unordered collection of unique elements, backed by a hash table.

It implements Set, providing membership tests, insertion, removal, and set algebra. Iteration order is unspecified.

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

Add an element to the set. Return true if the element was not already present.

<a id="hashset.extend"></a>

**`extend(items Iterable[T])`** — *`@native`*

Add all elements from `items` to the set.

<a id="hashset.remove"></a>

**`remove(v T) bool`** — *`@native`*

Remove an element from the set. Return true if the element was present.

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

Return a string representation of the set. Format: "{element1, element2}".

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

### `TreeSet`

```kl
pub class TreeSet[T : Comparable] : Set[T] {
    pub func __init__()
    pub func __iter__() Iterator[T]
    pub func __len__() int
    pub func empty() bool
    pub func __contains__(v T) bool
    pub func add(v T) bool
    pub func extend(items Iterable[T])
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


A TreeSet is a mutable, sorted collection of unique elements, backed by a balanced tree.

It implements Set, providing membership tests, insertion, removal, and set algebra. Elements are ordered by their Comparable ordering; iteration, to_list() and reversed() follow that order.

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

Add an element to the set. Return true if the element was not already present.

<a id="treeset.extend"></a>

**`extend(items Iterable[T])`** — *`@native`*

Add all elements from `items` to the set.

<a id="treeset.remove"></a>

**`remove(v T) bool`** — *`@native`*

Remove an element from the set. Return true if the element was present.

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

Return a string representation of the set. Format: "{element1, element2}", in sorted order.

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

A slice object represents a set of indices specified by [start:end:step].

It is internally created by the compiler when the ':' syntax is used inside subscript brackets []. Omitted and negative bounds are completed by the compiler against the target sequence length, so all fields are concrete integers. Follows Python's slicing semantics, including negative steps for reverse slicing.

---

**Fields**

<a id="slice.start"></a>

**`pub let start int`**

The starting index of the slice (inclusive).

<a id="slice.end"></a>

**`pub let end int`**

The ending index of the slice (exclusive). For reverse slices it may be -1, meaning the slice goes down to index 0.

<a id="slice.step"></a>

**`pub let step int`**

The step value of the slice. A negative step reverses the direction. Must not be zero; a zero step triggers a panic.

<a id="slice.__init__"></a>

**`__init__(_start int, _end int, _step = 1)`**

Initialize a new slice object. Typically constructed by the `build_intern` op when the compiler translates the [start:end:step] syntax.

<a id="slice.__str__"></a>

**`__str__() str`** — *`@native`*

Return the string representation of the slice. O(1). Format: "slice(start, end, step)".

### `str`

```kl
pub class str : Sequence & Comparable {
    pub func __init__(s str | bytes)
    pub func __iter__() Iterator[str]
    pub func __len__() int
    pub func empty() bool
    pub func __getitem__(index int) str
    pub func __getslice__(r slice) str
    pub func __contains__(sub str) bool
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
    pub func reversed() Iterator[str]
}
```
**Bases:** [`Sequence`](#sequence) & [`Comparable`](#comparable)


A str is an immutable sequence of characters.

It implements Sequence, providing text processing capabilities: searching, slicing, splitting, joining, case conversion and formatting.

Characters are encoded as UTF-8. str operates at the character level, so code-point level APIs (like Python's `ord`/`chr`) are not provided; use `to_bytes` for byte-level access.

---

<a id="str.__init__"></a>

**`__init__(s str | bytes)`** — *`@native`*

Initialize a string from `s`: a copy if `s` is a str, or a UTF-8 decoding if `s` is bytes. Panic if the bytes are not valid UTF-8.

<a id="str.__iter__"></a>

**`__iter__() Iterator[str]`** — *`@native`*

Return an iterator over the characters of the string.

<a id="str.__len__"></a>

**`__len__() int`** — *`@native`*

Return the length of the string.

<a id="str.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the string is empty, false otherwise.

<a id="str.__getitem__"></a>

**`__getitem__(index int) str`** — *`@native`*

Return the character at the specified index. Panic if the index is out of range.

<a id="str.__getslice__"></a>

**`__getslice__(r slice) str`** — *`@native`*

Return the substring defined by the slice.

<a id="str.__contains__"></a>

**`__contains__(sub str) bool`** — *`@native`*

Return true if the string contains the substring `sub`.

<a id="str.index"></a>

**`index(value str, start = 0, end = -1) int`** — *`@native`*

Return the first index of the substring `value` in [start, end). If `end` is -1, the search extends to the end of the string. Return -1 if the substring is not present.

<a id="str.rindex"></a>

**`rindex(value str, start = 0, end = -1) int`** — *`@native`*

Return the last index of the substring `value` in [start, end). If `end` is -1, the search extends to the end of the string. Return -1 if the substring is not present.

<a id="str.count"></a>

**`count(value str, start = 0, end = -1) int`** — *`@native`*

Return the number of occurrences of the substring `value` in [start, end). If `end` is -1, the search extends to the end of the string.

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

Return a stable hash value of the string. Equal strings must always produce identical hash values.

<a id="str.__str__"></a>

**`__str__() str`** — *`@native`*

Return the string itself.

<a id="str.substr"></a>

**`substr(start = 0, end = -1) str`** — *`@native`*

Return the substring from `start` (inclusive) to `end` (exclusive). If `end` is -1, the substring extends to the end of the string.

<a id="str.to_int"></a>

**`to_int() int`** — *`@native`*

Parse the string as a decimal integer. Panic if the string is not a valid integer.

<a id="str.to_int_or"></a>

**`to_int_or(default int) int`** — *`@native`*

Parse the string as a decimal integer. Return `default` if the string is not a valid integer.

<a id="str.to_float"></a>

**`to_float() float64`** — *`@native`*

Parse the string as a floating-point number. Panic if the string is not a valid number.

<a id="str.to_float_or"></a>

**`to_float_or(default float64) float64`** — *`@native`*

Parse the string as a floating-point number. Return `default` if the string is not a valid number.

<a id="str.format"></a>

**`format(args ...) str`** — *`@native`*

Return a formatted string using `self` as the format. `{}` consumes one argument in order.

<a id="str.__add__"></a>

**`__add__(rhs str) str`** — *`@native`*

Return a new string with `self` followed by `rhs` (`self + rhs`).

<a id="str.__mul__"></a>

**`__mul__(n int) str`** — *`@native`*

Return a new string with `self` repeated `n` times (`self * n`). Return an empty string if `n` is not positive.

<a id="str.join"></a>

**`join(seq Sequence[str]) str`** — *`@native`*

Return a string made by joining the elements of `seq`, with `self` as the separator.

<a id="str.to_bytes"></a>

**`to_bytes() bytes`** — *`@native`*

Return the UTF-8 encoded bytes of the string.

<a id="str.split"></a>

**`split(sep = " ", maxsplit = -1) list[str]`** — *`@native`*

Split the string by the separator `sep` and return the parts as a list. At most `maxsplit` splits are done; if `maxsplit` is -1, there is no limit.

<a id="str.rsplit"></a>

**`rsplit(sep = " ", maxsplit = -1) list[str]`** — *`@native`*

Split the string by the separator `sep` from the right and return the parts as a list. At most `maxsplit` splits are done; if `maxsplit` is -1, there is no limit.

<a id="str.splitlines"></a>

**`splitlines() list[str]`** — *`@native`*

Split the string at line boundaries and return the lines as a list. The line break characters are not included.

<a id="str.upper"></a>

**`upper() str`** — *`@native`*

Return a copy of the string with all characters converted to uppercase.

<a id="str.lower"></a>

**`lower() str`** — *`@native`*

Return a copy of the string with all characters converted to lowercase.

<a id="str.capitalize"></a>

**`capitalize() str`** — *`@native`*

Return a copy of the string with the first character capitalized and the rest lowercased.

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

Return a copy of the string with `prefix` removed if the string starts with it, otherwise return the string unchanged.

<a id="str.removesuffix"></a>

**`removesuffix(suffix str) str`** — *`@native`*

Return a copy of the string with `suffix` removed if the string ends with it, otherwise return the string unchanged.

<a id="str.replace"></a>

**`replace(old str, new str) str`** — *`@native`*

Return a copy of the string with all occurrences of `old` replaced by `new`.

<a id="str.zfill"></a>

**`zfill(width int) str`** — *`@native`*

Return a copy of the string left-padded with zeros to the given `width`. If `width` is not greater than the string length, the string is returned unchanged.

<a id="str.ljust"></a>

**`ljust(width int, fill = " ") str`** — *`@native`*

Return the string left-justified in a field of the given `width`, padded with `fill` (default space). If `width` is not greater than the string length, the string is returned unchanged.

<a id="str.rjust"></a>

**`rjust(width int, fill = " ") str`** — *`@native`*

Return the string right-justified in a field of the given `width`, padded with `fill` (default space). If `width` is not greater than the string length, the string is returned unchanged.

<a id="str.center"></a>

**`center(width int, fill = " ") str`** — *`@native`*

Return the string centered in a field of the given `width`, padded with `fill` (default space). If `width` is not greater than the string length, the string is returned unchanged.

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

<a id="str.reversed"></a>

**`reversed() Iterator[str]`** — *`@native`*

Return a reverse iterator over the characters of the string.

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

It implements Sequence, providing fixed-size, read-only access: random access, searching and slicing.

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

**`__getitem__[infer U](index int) U`** — *`@intrinsic`*

Get the element at the specified index. The element type is inferred from the usage context. Panic if the index is out of range.

<a id="tuple.__getslice__"></a>

**`__getslice__(r slice) tuple[T]`** — *`@native`*

Return the sub-tuple defined by the slice.

<a id="tuple.__contains__"></a>

**`__contains__(v T) bool`** — *`@native`*

Return true if the tuple contains the specified value `v`. O(n).

<a id="tuple.index"></a>

**`index(value T, start = 0, end = -1) int`** — *`@native`*

Return the first index of `value` in [start, end). O(n). If `end` is -1, the search extends to the end of the tuple. Return -1 if the value is not present.

<a id="tuple.rindex"></a>

**`rindex(value T, start = 0, end = -1) int`** — *`@native`*

Return the last index of `value` in [start, end). O(n). If `end` is -1, the search extends to the end of the tuple. Return -1 if the value is not present.

<a id="tuple.count"></a>

**`count(value T, start = 0, end = -1) int`** — *`@native`*

Return the number of occurrences of `value` in [start, end). O(n). If `end` is -1, the search extends to the end of the tuple.

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

The type class represents the runtime type of a value.

Type objects are produced only by the runtime via [`typeof()`](#typeof) and cannot be created directly.

---

<a id="type.__init__"></a>

**`__init__()`**

<a id="type.__str__"></a>

**`__str__() str`** — *`@native`*

Return the string representation of the type. Format: "<class 'name'>", e.g. "<class 'str'>".

<a id="type.name"></a>

**`name() str`** — *`@native`*

Return the name of the type. For types outside std/builtin, the name is prefixed with the module path.

<a id="type.methods"></a>

**`methods() list[str]`** — *`@native`*

Return the names of all methods of the type.

<a id="type.lro"></a>

**`lro() list[str]`** — *`@native`*

Return the names of the traits conformed to by the type, in Linearized Resolution Order.

## Functions

### `print`

```kl
pub func print(objs ..., sep = ' ', end = '\n')
```

*`@native`*

Print objects to the text stream file, separated by `sep` and followed by `end`.

All non-keyword arguments are converted to strings using their `__str__` method. If no objects are given, print() will just write the `end` character. The `sep` and `end` arguments must be strings; the default separator is a space, and the default end-of-line is a newline character.

### `len`

```kl
pub func len(obj any) int
```

*`@intrinsic`*

Return the number of items in a collection or the length of an object.

This function is an intrinsic and is resolved at compile time. Semantically, `len(obj)` is equivalent to calling `obj.__len__()`.

Any object may define a `__len__()` method to customize its length behavior. Built-in container types such as list, dict, tuple, and set provide their own optimized implementations.

**Note:**

```kl
- As an intrinsic, `len()` does not have a user-level implementation.
- The compiler may lower `len(obj)` to specialized VM instructions
  (e.g., OP_SEQ_LEN, OP_MAP_LEN) when the object's type is known,
  enabling high-performance length queries.
```

### `panic`

```kl
pub func panic(msg str)
```

*`@native`*

Immediately abort program execution with an error message.

`panic(msg)` raises a fatal runtime error and terminates the current execution context. The function does not return.

This is a native builtin function implemented directly by the Koala VM. It is used by the standard library and compiler-generated code to signal unrecoverable conditions (such as failed assertions, type errors, or internal invariants).

**Note:**

```kl
- `panic()` does not have a user-level implementation.
- The VM may include additional diagnostic information such as
  stack traces or source locations when reporting the panic.
- User code should prefer returning `T?` or error values when
  appropriate; `panic()` is intended only for fatal errors.
```

### `format`

```kl
pub func format(fmt str, args ...) str
```

*`@native`*

Format a string using positional arguments.

`{}` consumes one argument in order. No named placeholders. Arguments are converted using `__str__()`.

**Example:**

```kl
format("Point({}, {})", x, y)
"Point(10, 20)"
```

### `typeof`

```kl
pub func typeof(obj any) type
```

*`@native`*

Return the runtime type of a value.

**Example:**

```kl
typeof(10)      // <class 'int'>
typeof("hello") // <class 'str'>
typeof(Node(5)) // <class 'Node'>
```
