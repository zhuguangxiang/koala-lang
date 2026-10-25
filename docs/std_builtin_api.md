# Koala `std/builtin` API Reference

## Contents

**Traits**

| Name | Summary |
|------|---------|
| [`any`](#any) | The `any` trait is the root of the Koala type hierarchy. Every class and trait implicitly inherits from `any`. It defines no methods and imposes no behavioral requirements. It exists solely as the universal supertype for all Koala values. |
| [`Equatable`](#equatable) | `Equatable` defines semantic equality between two values of the same type. The type parameter `T` is the type compared against, which is the conforming type itself. |
| [`Comparable`](#comparable) | `Comparable` defines a total ordering between values of the same type. The type parameter `T` is the type compared against, which is the conforming type itself. Types conforming to `Comparable` can be sorted, compared, and used in ordered data structures such as priority queues or tree-based collections. |
| [`Hashable`](#hashable) | `Hashable` defines the ability for a type to produce a stable hash value. Conforming to `Hashable` allows a type to be used as a key in dictionaries, sets, and other hash-based collections. |
| [`Printable`](#printable) | `Printable` defines a human-readable representation for a type. |
| [`ToString`](#tostring) | `ToString` defines a lossless, programmatic conversion to a string representation. |
| [`Iterator`](#iterator) | An Iterator represents the state of an ongoing traversal of elements. |
| [`Iterable`](#iterable) | Iterable is the basis for objects that can be traversed. |
| [`Sized`](#sized) | A Sized has a known length, obtainable without traversing its elements. |
| [`Sequence`](#sequence) | A Sequence provides indexed access to an ordered collection of elements. |
| [`MutableSequence`](#mutablesequence) | A MutableSequence is an ordered sequence whose elements and size can be modified. |
| [`Map`](#map) | A Map is a read-only view of a collection of key-value mappings. |
| [`MutableMap`](#mutablemap) | A MutableMap is a map whose mappings can be added, replaced, and removed. |
| [`Set`](#set) | A Set is a read-only collection that contains no duplicate elements. |
| [`MutableSet`](#mutableset) | A MutableSet is a set whose elements can be added and removed. |
| [`Arithmetic`](#arithmetic) | Trait for basic arithmetic operations. |
| [`Bitwise`](#bitwise) | Trait for bitwise operations. |

**Classes**

| Name | Summary |
|------|---------|
| [`bool`](#bool) | Represents a logical value that can be either true or false. |
| [`Buffer`](#buffer) | Buffer is a mutable, growable byte buffer. |
| [`bytes`](#bytes) | The `bytes` type is a fixed-size, readable and writable byte array. |
| [`BytesIter`](#bytesiter) | An iterator over the bytes of a byte array, from left to right. |
| [`BytesRevIter`](#bytesreviter) | An iterator over the bytes of a byte array, from right to left. |
| [`dict`](#dict) | A dict is a mutable, insertion-ordered hash map that maps unique keys to values. |
| [`DictKeys`](#dictkeys) | Lightweight view returned by `dict.keys()`. keys are returned in insertion order. |
| [`DictKeysIter`](#dictkeysiter) | Iterator over dict keys in insertion order. |
| [`DictValues`](#dictvalues) | Lightweight view returned by `dict.values()`. Values are returned in insertion order. |
| [`DictValuesIter`](#dictvaluesiter) | Iterator over dict values in insertion order. |
| [`DictView`](#dictview) | Lightweight view returned by `dict.items()`. Key-value pairs are returned in insertion order. |
| [`DictViewIter`](#dictviewiter) | Iterator over dict items in insertion order. Used by `DictItems` and `dict.iter()` to provide iteration functionality. |
| [`DictRevIter`](#dictreviter) | Iterator over dict items in reverse insertion order. Used by `dict.reversed()` to provide reverse iteration functionality. |
| [`Exception`](#exception) | An exception object represents a raised exception. Once raised, it cannot be caught and must be resolved. |
| [`Formatter`](#formatter) | `Formatter` builds formatted text in an internal buffer. |
| [`hashset`](#hashset) | A hashset is a mutable, unordered collection of unique elements, backed by a hash table. |
| [`HashSetIter`](#hashsetiter) | Iterator over hashset elements. Order is unspecified. |
| [`list`](#list) | A list is a mutable sequence of elements. |
| [`ListIter`](#listiter) | A forward iterator over a `list`, advancing by `step` elements each turn. |
| [`ListRevIter`](#listreviter) | A reverse iterator over a `list`, moving backward by `step` elements each turn. |
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
| [`bfloat16`](#bfloat16) | A 16-bit brain floating-point type (bfloat16). |
| [`range`](#range) | A range represents an immutable sequence of integers. |
| [`slice`](#slice) | A slice object represents a set of indices specified by `[start:end:step]`. |
| [`str`](#str) | A str is an immutable sequence of characters. |
| [`StrIter`](#striter) | An iterator over the characters of a string, from left to right. |
| [`StrRevIter`](#strreviter) | An iterator over the characters of a string, from right to left. |
| [`tuple`](#tuple) | A tuple is an immutable sequence of elements. |
| [`type`](#type) | The `type` class represents the runtime type of a value. |

**Functions**

| Name | Summary |
|------|---------|
| [`panic`](#panic) | Immediately abort program execution with an error message. |
| [`unreachable`](#unreachable) | Signal that execution has reached code that should be unreachable. |
| [`format`](#format) | Format a string using positional arguments. |
| [`typeof`](#typeof) | Return the runtime type of a value. |
| [`iter`](#iter) | Return an iterator over the elements of an iterable. |
| [`next`](#next) | Return the next element, or nil when exhausted. Consume elements with `while let value = next(it)`. |
| [`len`](#len) | Return the number of elements in a sized collection. |
| [`abs`](#abs) | Computes the absolute value using a matching specialization. |
| [`abs_int64`](#abs_int64) | Specialized explicit implementation of `abs()` for 64-bit signed integers. |
| [`abs_float64`](#abs_float64) | Specialized explicit implementation of `abs()` for 64-bit floating-point numbers. |
| [`min`](#min) | Constrained generic minimum function, auto-specialized for primitives. |
| [`max`](#max) | Constrained generic maximum function, auto-specialized for primitives. |
| [`pow`](#pow) | Computes a power using a matching specialization. |
| [`pow_int64_int64`](#pow_int64_int64) | Specialized explicit implementation for 64-bit integer base and exponent. |
| [`pow_float64_int64`](#pow_float64_int64) | Specialized explicit implementation for float base and integer exponent. |
| [`pow_float64_float64`](#pow_float64_float64) | Specialized explicit implementation for float base and float exponent. |
| [`round`](#round) | Return `x` rounded to the nearest integer value (as a float). |
| [`floor`](#floor) | Return the largest integer value less than or equal to `x` (as a float). |
| [`ceil`](#ceil) | Return the smallest integer value greater than or equal to `x` (as a float). |
| [`divmod`](#divmod) | Return `(a / b, a % b)` as a tuple. |
| [`divmod_int64`](#divmod_int64) | Specialized implementation of `divmod()` for 64-bit signed integers. |
| [`divmod_float64`](#divmod_float64) | Specialized implementation of `divmod()` for 64-bit floating-point numbers. |

**Constants**

| Name | Summary |
|------|---------|
| [`INT8_MIN`](#int8_min) | Built-in integer limits constants |
| [`INT8_MAX`](#int8_max) |  |
| [`INT16_MIN`](#int16_min) |  |
| [`INT16_MAX`](#int16_max) |  |
| [`INT32_MIN`](#int32_min) |  |
| [`INT32_MAX`](#int32_max) |  |
| [`INT64_MIN`](#int64_min) |  |
| [`INT64_MAX`](#int64_max) |  |
| [`UINT8_MIN`](#uint8_min) |  |
| [`UINT8_MAX`](#uint8_max) |  |
| [`UINT16_MIN`](#uint16_min) |  |
| [`UINT16_MAX`](#uint16_max) |  |
| [`UINT32_MIN`](#uint32_min) |  |
| [`UINT32_MAX`](#uint32_max) |  |
| [`UINT64_MIN`](#uint64_min) |  |
| [`UINT64_MAX`](#uint64_max) |  |
| [`BIG_ENDIAN`](#big_endian) |  |
| [`LITTLE_ENDIAN`](#little_endian) |  |

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

NOTES:
Koala splits Java’s `Object` into three independent traits
(`Equatable`, `Hashable`, `Printable`). Classes auto‑conform to
these traits, while traits remain pure. This enables type‑safe,
generic equality (`Equatable[T]`) without Java’s `equals(Object)`
limitation.

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
    func hash() int
}
```

`Hashable` defines the ability for a type to produce a stable hash value.
Conforming to `Hashable` allows a type to be used as a key in dictionaries,
sets, and other hash-based collections.

Equal values must always produce identical hash values. The hash should
be well-distributed to ensure good performance in hashed structures.

---

<a id="hashable.hash"></a>

**`hash() int`**

Return a stable hash value for `self`.

Equal values must always produce identical hash values.

<a id="printable"></a>

### `Printable`

```kl
pub trait Printable {
    func fmt(f Formatter)
}
```

`Printable` defines a human-readable representation for a type.

The representation is written directly to a `Formatter` and is intended
for display, debugging, and other formatting operations.

---

<a id="printable.fmt"></a>

**`fmt(f Formatter)`**

Write a human-readable representation of `self` to `f`.

<a id="tostring"></a>

### `ToString`

```kl
pub trait ToString {
    func to_str() str
}
```

`ToString` defines a lossless, programmatic conversion to a string representation.

Unlike `Printable`, which focuses on user-facing display and formatting,
`ToString` is intended for exact data extraction or serialization.

For example, a `Path` object's `Printable` output might be `Path("/a/b")` for debugging,
but its `ToString.to_str()` must yield the exact raw path `"/a/b"`.

---

<a id="tostring.to_str"></a>

**`to_str() str`**

Return the raw, programmatic string representation of `self`.

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

It provides two ways to advance the iterator:
`next()` returns an optional element, while `has_next()` followed by
`next_strict()` provides a non-nullable result when an element is available.

Iterators are stateful and do not support rewinding.

---

<a id="iterator.has_next"></a>

**`has_next() bool`**

Return true if another element is available.

<a id="iterator.next_strict"></a>

**`next_strict() T`**

Return the next element and advance the iterator. Panic if the iterator is exhausted.

<a id="iterator.next"></a>

**`next() T?`**

Return the next element and advance the iterator, or `nil` if the iterator is exhausted.

When `T` is nullable, an element may also be `nil`. Use `has_next()`
with `next_strict()` to distinguish such elements from exhaustion.

<a id="iterable"></a>

### `Iterable`

```kl
pub trait Iterable[T] {
    func iter(step = 1) Iterator[T]
}
```

Iterable is the basis for objects that can be traversed.

It provides a way to create an iterator for the elements.

---

<a id="iterable.iter"></a>

**`iter(step = 1) Iterator[T]`**

Return a new iterator for traversing the elements.

`step` specifies the number of elements to advance after each iteration.

<a id="sized"></a>

### `Sized`

```kl
pub trait Sized {
    func len() int
}
```

A Sized has a known length, obtainable without traversing its elements.

It is the common protocol for collections whose number of items can be
queried directly, independent of any traversal capability.

---

<a id="sized.len"></a>

**`len() int`**

Return the number of items.

<a id="sequence"></a>

### `Sequence`

```kl
pub trait Sequence[T] : Iterable[T] & Sized {
    func __contains__(item T) bool
    func __getitem__(index int) T
    func index(value T, start = 0, end = -1) int
    func count(value T, start = 0, end = -1) int
}
```
**Supertraits:** [`Iterable[T]`](#iterable) & [`Sized`](#sized)


A Sequence provides indexed access to an ordered collection of elements.

It provides random access and searching capabilities without modification.

---

<a id="sequence.__contains__"></a>

**`__contains__(item T) bool`**

Return true if the sequence contains the specified value `item`.

<a id="sequence.__getitem__"></a>

**`__getitem__(index int) T`**

Return the item at the specified `index`.

Panic if `index` is negative or is not less than the sequence length.

<a id="sequence.index"></a>

**`index(value T, start = 0, end = -1) int`**

Return the first index of `value` in `[start, end)`.

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
    func append(value T)
    func extend(items Iterable[T])
    func insert(index int, value T)
    func remove(value T)
    func pop(index = -1) T
    func clear()
}
```
**Supertraits:** [`Sequence[T]`](#sequence)


A MutableSequence is an ordered sequence whose elements and size can be modified.

It extends the Sequence trait with operations for updating, inserting,
and removing elements.

---

<a id="mutablesequence.__setitem__"></a>

**`__setitem__(index int, value T)`**

Set the item at `index` to `value`.

Panic if `index` is negative or is not less than the sequence length.

<a id="mutablesequence.append"></a>

**`append(value T)`**

Append `value` to the end of the sequence.

<a id="mutablesequence.extend"></a>

**`extend(items Iterable[T])`**

Append all elements from `items` to the end of the sequence.

<a id="mutablesequence.insert"></a>

**`insert(index int, value T)`**

Insert `value` at `index`.

Elements at or after `index` are shifted to the right.

Panic if `index` is negative or greater than the sequence length.

<a id="mutablesequence.remove"></a>

**`remove(value T)`**

Remove the first occurrence of `value` from the sequence.

Panic if `value` is not present.

<a id="mutablesequence.pop"></a>

**`pop(index = -1) T`**

Remove and return the item at `index`.

If `index` is `-1`, remove and return the last item.

Panic if `index` is out of bounds or sequence is empty.

<a id="mutablesequence.clear"></a>

**`clear()`**

Remove all items from the sequence, leaving it empty.

<a id="map"></a>

### `Map`

```kl
pub trait Map[K, V] : Iterable[(K, V)] {
    func len() int
    func __contains__(key K) bool
    func __getitem__(key K) V
    func keys() Iterable[K]
    func values() Iterable[V]
    func items() Iterable[(K, V)]
    func get(key K) V?
    func get_or(key K, default_value V) V
}
```
**Supertraits:** [`Iterable[(K, V)]`](#iterable)


A Map is a read-only view of a collection of key-value mappings.

A map cannot contain duplicate keys; each key maps to at most one value.
Iterating a map yields `(key, value)` pairs; the order is determined by
the concrete implementation (e.g. `dict` uses insertion order).

---

<a id="map.len"></a>

**`len() int`**

Return the number of mappings in the map.

<a id="map.__contains__"></a>

**`__contains__(key K) bool`**

Return true if the map contains a mapping for `key`.

<a id="map.__getitem__"></a>

**`__getitem__(key K) V`**

Return the value associated with `key`. Panic if `key` is not present.

<a id="map.keys"></a>

**`keys() Iterable[K]`**

Return an iterable over the keys in the map.

<a id="map.values"></a>

**`values() Iterable[V]`**

Return an iterable over the values in the map.

<a id="map.items"></a>

**`items() Iterable[(K, V)]`**

Return an iterable over the key-value pairs in the map.

<a id="map.get"></a>

**`get(key K) V?`**

Return the value associated with `key`, or `nil` if `key` is not present.

Unlike indexing with `map[key]`, this method does not panic when
`key` is not present and is intended for use with `if let`.

<a id="map.get_or"></a>

**`get_or(key K, default_value V) V`**

Return the value associated with `key`, or `default_value` if `key` is
not present.

<a id="mutablemap"></a>

### `MutableMap`

```kl
pub trait MutableMap[K, V] : Map[K, V] {
    func __setitem__(key K, value V)
    func clear()
    func remove(key K) V
    func remove_or(key K, default_value V) V
}
```
**Supertraits:** [`Map[K, V]`](#map)


A MutableMap is a map whose mappings can be added, replaced, and removed.

---

<a id="mutablemap.__setitem__"></a>

**`__setitem__(key K, value V)`**

Associate `value` with `key`, inserting a new mapping if `key` is absent
or replacing its associated value if `key` already exists.

<a id="mutablemap.clear"></a>

**`clear()`**

Remove all mappings from the map, leaving it empty.

<a id="mutablemap.remove"></a>

**`remove(key K) V`**

Remove the mapping for `key` and return its value. Panic if `key` is not present.

<a id="mutablemap.remove_or"></a>

**`remove_or(key K, default_value V) V`**

Remove the mapping for `key` and return its value.

If `key` is not present, return `default_value`.

<a id="set"></a>

### `Set`

```kl
pub trait Set[T] : Iterable[T] {
    func len() int
    func __contains__(value T) bool
    func is_subset(other Iterable[T]) bool
    func is_superset(other Iterable[T]) bool
    func union(other Iterable[T]) Set[T]
    func intersect(other Iterable[T]) Set[T]
    func diff(other Iterable[T]) Set[T]
}
```
**Supertraits:** [`Iterable[T]`](#iterable)


A Set is a read-only collection that contains no duplicate elements.

It models the mathematical set abstraction, providing membership tests
and set algebra without modification.

---

<a id="set.len"></a>

**`len() int`**

Return the number of elements in the set.

<a id="set.__contains__"></a>

**`__contains__(value T) bool`**

Return true if the set contains `value`.

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

<a id="mutableset"></a>

### `MutableSet`

```kl
pub trait MutableSet[T] : Set[T] {
    func add(value T) bool
    func update(items Iterable[T])
    func remove(value T) bool
    func clear()
}
```
**Supertraits:** [`Set[T]`](#set)


A MutableSet is a set whose elements can be added and removed.

---

<a id="mutableset.add"></a>

**`add(value T) bool`**

Add `value` to the set.

Return true if `value` was not already present.

<a id="mutableset.update"></a>

**`update(items Iterable[T])`**

Add all elements from `items` to the set.

<a id="mutableset.remove"></a>

**`remove(value T) bool`**

Remove `value` from the set.

Return true if `value` was present.

<a id="mutableset.clear"></a>

**`clear()`**

Remove all elements from the set.

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
pub class bool : ToString {
    pub func __and__(other bool) bool
    pub func __or__(other bool) bool
    pub func __not__() bool
    pub func __eq__(other bool) bool
    pub func __ne__(other bool) bool
    pub func fmt(f Formatter)
    pub func to_str() str
}
```
**Bases:** [`ToString`](#tostring)


Represents a logical value that can be either true or false.

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

<a id="bool.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the boolean value for display.

<a id="bool.to_str"></a>

**`to_str() str`** — *`@native`*

Convert the boolean value to its string ("true" or "false").

<a id="buffer"></a>

### `Buffer`

```kl
pub class Buffer {
    pub func __init__(capacity = 256)
    pub func write_byte(b uint8)
    pub func write_str(s str)
    pub func write(b bytes)
    pub func to_str() str
    pub func to_bytes() bytes
    pub func len() int
    pub func empty() bool
    pub func clear()
}
```

Buffer is a mutable, growable byte buffer.

It supports efficient byte and string appends and can be converted to `str` or `bytes`.

Examples

```kl
let buffer = Buffer()
buffer.write_str("Hello, ")
buffer.write_str("Koala!")
print(buffer.to_str())
```

---

<a id="buffer.__init__"></a>

**`__init__(capacity = 256)`** — *`@native`*

Create an empty buffer with an optional initial capacity.

<a id="buffer.write_byte"></a>

**`write_byte(b uint8)`** — *`@native`*

Append a single byte to the end of the buffer.

<a id="buffer.write_str"></a>

**`write_str(s str)`** — *`@native`*

Append the UTF-8 encoded bytes of `s` to the buffer.

<a id="buffer.write"></a>

**`write(b bytes)`** — *`@native`*

Append raw bytes to the end of the buffer.

<a id="buffer.to_str"></a>

**`to_str() str`** — *`@native`*

Decode the buffered content as UTF-8 and return a new `str`.

Panic if the buffered bytes are not valid UTF-8.

<a id="buffer.to_bytes"></a>

**`to_bytes() bytes`** — *`@native`*

Return a view of the buffered content as `bytes`.

<a id="buffer.len"></a>

**`len() int`** — *`@native`*

Return the current number of bytes in the buffer.

<a id="buffer.empty"></a>

**`empty() bool`**

Return true if the buffer contains no bytes.

<a id="buffer.clear"></a>

**`clear()`** — *`@native`*

Clear the buffer, resetting its length to zero.

The buffer can be reused without reducing its capacity.

<a id="bytes"></a>

### `bytes`

```kl
pub class bytes : Sequence[uint8] {
    pub func __init__(size int)
    pub func iter(step = 1) Iterator[uint8]
    pub func reversed(step = 1) Iterator[uint8]
    pub func len() int
    pub func empty() bool
    pub func __getitem__(index int) uint8
    pub func __getslice__(s slice) bytes
    pub func __setitem__(index int, value uint8)
    pub func __setslice__(s slice, val bytes)
    pub func __contains__(value uint8) bool
    pub func copy(src bytes, start = 0, end = -1) int
    pub func copy_str(s str, start = 0, end = -1) int
    pub func fill(value uint8, start = 0, end = -1)
    pub func zero(start = 0, end = -1)
    pub func view(start = 0, end = -1) bytes
    pub func index(value uint8, start = 0, end = -1) int
    pub func rindex(value uint8, start = 0, end = -1) int
    pub func count(value uint8, start = 0, end = -1) int
    pub func index_bytes(sub bytes, start = 0, end = -1) int
    pub func rindex_bytes(sub bytes, start = 0, end = -1) int
    pub func starts_with(prefix bytes) bool
    pub func ends_with(suffix bytes) bool
    pub func replace(old bytes, new bytes) bytes
    pub func split(sep bytes, maxsplit = -1) list[bytes]
    pub func rsplit(sep bytes, maxsplit = -1) list[bytes]
    pub func splitlines() list[bytes]
    pub func strip() bytes
    pub func lstrip() bytes
    pub func rstrip() bytes
    pub func fmt(f Formatter)
    pub func to_str() str
}
```
**Bases:** [`Sequence[uint8]`](#sequence)


The `bytes` type is a fixed-size, readable and writable byte array.

It implements `Sequence`, providing random access, searching, slicing,
and sub-sequence matching.
The size is determined at construction and never changes.

---

<a id="bytes.__init__"></a>

**`__init__(size int)`** — *`@native`*

Create a byte array of the given `size`, initialized to zero.

<a id="bytes.iter"></a>

**`iter(step = 1) Iterator[uint8]`**

Return an iterator over the bytes.

The iterator reads directly from the underlying byte array without
allocating. `step` specifies the stride; `step = 1` visits every byte.

<a id="bytes.reversed"></a>

**`reversed(step = 1) Iterator[uint8]`**

Return a reverse iterator over the bytes.

The iterator reads directly from the underlying byte array without
allocating. `step` specifies the stride; `step = 1` visits every byte.

<a id="bytes.len"></a>

**`len() int`** — *`@native`*

Return the size of the byte array.

<a id="bytes.empty"></a>

**`empty() bool`**

Return true if the byte array is empty.

<a id="bytes.__getitem__"></a>

**`__getitem__(index int) uint8`** — *`@native`*

Return the byte at `index`.

Panic if `index` is out of range.

<a id="bytes.__getslice__"></a>

**`__getslice__(s slice) bytes`** — *`@native`*

Return the bytes selected by `s` (the `b[start:end:step]` syntax).

Indices follow Python slice semantics: negative indices count from
the end, omitted bounds default to the ends, and `step` may be
negative. A `step` of 1 returns a zero-copy view; other steps copy.

<a id="bytes.__setitem__"></a>

**`__setitem__(index int, value uint8)`** — *`@native`*

Set the byte at `index` to `value`.

Panic if `index` is out of range.

<a id="bytes.__setslice__"></a>

**`__setslice__(s slice, val bytes)`** — *`@native`*

Replace the bytes selected by `s` with `val` (the `b[start:end:step] = val` syntax).

`val` must supply exactly one byte per selected slot; the length of
the byte array never changes.

<a id="bytes.__contains__"></a>

**`__contains__(value uint8) bool`** — *`@native`*

Return true if the byte array contains `value`.

<a id="bytes.copy"></a>

**`copy(src bytes, start = 0, end = -1) int`** — *`@native`*

Copy `src[start:end]` into the beginning of this byte array.

This is a raw memory copy operation, similar to C's `memcpy`.
`start` defaults to 0. `end` defaults to -1, meaning the end of `src`.

Return the number of bytes copied, or -1 if the slice exceeds
the capacity of this byte array.

<a id="bytes.copy_str"></a>

**`copy_str(s str, start = 0, end = -1) int`** — *`@native`*

Copy the UTF-8 encoding of `s[start:end]` into the beginning of this
byte array.

`start` and `end` select text in `s` before UTF-8 encoding.
`start` defaults to 0. `end` defaults to -1, meaning the end of `s`.

Return the number of bytes copied, or -1 if the encoded length exceeds
the capacity of this byte array.

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
Out-of-range bounds clamp instead of panicking.

<a id="bytes.index"></a>

**`index(value uint8, start = 0, end = -1) int`** — *`@native`*

Return the first index of `value` in `[start, end)`. O(n).

If `end` is `-1`, the search extends to the end of the byte array.
Return `-1` if `value` is not present.

<a id="bytes.rindex"></a>

**`rindex(value uint8, start = 0, end = -1) int`** — *`@native`*

Return the last index of `value` in `[start, end)`. O(n).

If `end` is `-1`, the search extends to the end of the byte array.
Return `-1` if `value` is not present.

<a id="bytes.count"></a>

**`count(value uint8, start = 0, end = -1) int`** — *`@native`*

Return the number of occurrences of `value` in `[start, end)`. O(n).

If `end` is `-1`, the search extends to the end of the byte array.

<a id="bytes.index_bytes"></a>

**`index_bytes(sub bytes, start = 0, end = -1) int`** — *`@native`*

Return the first index of the sub-sequence `sub` in `[start, end)`. O(n).

If `end` is `-1`, the search extends to the end of the byte array.
Return `-1` if `sub` is not present.

<a id="bytes.rindex_bytes"></a>

**`rindex_bytes(sub bytes, start = 0, end = -1) int`** — *`@native`*

Return the last index of the sub-sequence `sub` in `[start, end)`. O(n).

If `end` is `-1`, the search extends to the end of the byte array.
Return `-1` if `sub` is not present.

<a id="bytes.starts_with"></a>

**`starts_with(prefix bytes) bool`** — *`@native`*

Return true if the byte array starts with `prefix`.

<a id="bytes.ends_with"></a>

**`ends_with(suffix bytes) bool`** — *`@native`*

Return true if the byte array ends with `suffix`.

<a id="bytes.replace"></a>

**`replace(old bytes, new bytes) bytes`** — *`@native`*

Replace all occurrences of `old` with `new`, returning a new byte array.

<a id="bytes.split"></a>

**`split(sep bytes, maxsplit = -1) list[bytes]`** — *`@native`*

Split the byte array by `sep` and return the parts as a list.

At most `maxsplit` splits are performed.
If `maxsplit` is `-1`, there is no limit.

<a id="bytes.rsplit"></a>

**`rsplit(sep bytes, maxsplit = -1) list[bytes]`** — *`@native`*

Split the byte array by `sep` from the right and return the parts as a list.

At most `maxsplit` splits are performed.
If `maxsplit` is `-1`, there is no limit.

<a id="bytes.splitlines"></a>

**`splitlines() list[bytes]`** — *`@native`*

Split the byte array at line boundaries and return the lines as a list.

The line break characters are not included.

<a id="bytes.strip"></a>

**`strip() bytes`** — *`@native`*

Remove leading and trailing whitespace bytes.

Whitespace consists of space (`0x20`), tab (`\t`), newline (`\n`),
and carriage return (`\r`).

<a id="bytes.lstrip"></a>

**`lstrip() bytes`** — *`@native`*

Remove leading whitespace bytes.

Whitespace consists of space (`0x20`), tab (`\t`), newline (`\n`),
and carriage return (`\r`).

<a id="bytes.rstrip"></a>

**`rstrip() bytes`** — *`@native`*

Remove trailing whitespace bytes.

Whitespace consists of space (`0x20`), tab (`\t`), newline (`\n`),
and carriage return (`\r`).

<a id="bytes.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the byte array for display.

<a id="bytes.to_str"></a>

**`to_str() str`** — *`@native`*

Decode the bytes as UTF-8 and return a new `str`.

Panic if the bytes are not valid UTF-8.

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
    pub func fmt(f Formatter)
}
```
**Bases:** [`Iterator[uint8]`](#iterator)


An iterator over the bytes of a byte array, from left to right.

`BytesIter` returns each byte as `uint8`. `step` specifies the stride;
`step = 1` visits every byte.

Iteration ends when no byte remains. Create a new iterator to iterate again.

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

Initialize a forward iterator over `b` with an optional stride `step`.

<a id="bytesiter.has_next"></a>

**`has_next() bool`**

Return true if there are remaining bytes to iterate.

<a id="bytesiter.next_strict"></a>

**`next_strict() uint8`**

Return the next byte and advance by `step`. Panic if exhausted.

<a id="bytesiter.next"></a>

**`next() uint8?`**

Return the next byte, or `nil` if exhausted.

<a id="bytesiter.fmt"></a>

**`fmt(f Formatter)`**

Format the iterator state for display.
Produce a human-readable description of the iterator state.

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
    pub func fmt(f Formatter)
}
```
**Bases:** [`Iterator[uint8]`](#iterator)


An iterator over the bytes of a byte array, from right to left.

`BytesRevIter` returns each byte as `uint8`. `step` specifies the stride;
`step = 1` visits every byte in reverse order.

Iteration ends when no byte remains. Create a new iterator to iterate again.

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

Initialize a reverse iterator over `b` with an optional stride `step`.

<a id="bytesreviter.has_next"></a>

**`has_next() bool`**

Return true if there are remaining bytes to iterate.

<a id="bytesreviter.next_strict"></a>

**`next_strict() uint8`**

Return the next byte and move backward by `step`. Panic if exhausted.

<a id="bytesreviter.next"></a>

**`next() uint8?`**

Return the next byte in reverse order, or `nil` if exhausted.

<a id="bytesreviter.fmt"></a>

**`fmt(f Formatter)`**

Format the iterator state for display.
Produce a human-readable description of the iterator state.

<a id="dict"></a>

### `dict`

```kl
pub class dict[K, V] : MutableMap[K, V] {
    pub func __init__()
    pub func iter(step = 1) Iterator[(K, V)]
    pub func len() int
    pub func empty() bool
    pub func __contains__(key K) bool
    pub func __getitem__(key K) V
    pub func __setitem__(key K, value V)
    pub func keys() Iterable[K]
    pub func values() Iterable[V]
    pub func items() Iterable[(K, V)]
    pub func clear()
    pub func remove(key K) V
    pub func remove_or(key K, default_value V) V
    pub func get(key K) V?
    pub func get_or(key K, default_value V) V
    pub func update(src Iterable[(K, V)])
    pub func setdefault(key K, default_value V) V
    pub func __bitor__(src Iterable[(K, V)]) dict[K, V]
    pub func __ior__(src Iterable[(K, V)]) dict[K, V]
    pub func reversed(step = 1) Iterator[(K, V)]
    pub func fmt(f Formatter)
}
```
**Bases:** [`MutableMap[K, V]`](#mutablemap)


A dict is a mutable, insertion-ordered hash map that maps unique keys
to values.

It implements `MutableMap`, providing key lookup, insertion, removal,
and iteration over key-value pairs. Iteration, `keys()` and `values()`
all follow insertion order.

---

<a id="dict.__init__"></a>

**`__init__()`** — *`@native`*

Create an empty dict.

<a id="dict.iter"></a>

**`iter(step = 1) Iterator[(K, V)]`**

Return an iterator over the (key, value) pairs in insertion order.

<a id="dict.len"></a>

**`len() int`** — *`@native`*

Return the number of key-value pairs.

<a id="dict.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the dict is empty, false otherwise.

<a id="dict.__contains__"></a>

**`__contains__(key K) bool`** — *`@native`*

Return true if the dict contains a mapping for `key`.

<a id="dict.__getitem__"></a>

**`__getitem__(key K) V`** — *`@native`*

Return the value mapped to `key`. Panic if `key` is not present.

<a id="dict.__setitem__"></a>

**`__setitem__(key K, value V)`** — *`@native`*

Associate `value` with `key`, inserting a new mapping if `key` is absent
or replacing its associated value if `key` already exists.

<a id="dict.keys"></a>

**`keys() Iterable[K]`**

Return an iterable over all keys in the dict, in insertion order.

<a id="dict.values"></a>

**`values() Iterable[V]`**

Return an iterable over all values in the dict, in insertion order.

<a id="dict.items"></a>

**`items() Iterable[(K, V)]`**

Return an iterable over the key-value pairs in the map.

<a id="dict.clear"></a>

**`clear()`** — *`@native`*

Remove all mappings from the dict, leaving it empty.

<a id="dict.remove"></a>

**`remove(key K) V`** — *`@native`*

Remove the mapping for `key` and return its value.

Panic if `key` is not present.

<a id="dict.remove_or"></a>

**`remove_or(key K, default_value V) V`** — *`@native`*

Remove the mapping for `key` and return its value.
If the key is not found, return `default_value`.

<a id="dict.get"></a>

**`get(key K) V?`** — *`@native`*

Return the value for `key` if present, else `nil`.

Missing keys return `nil` instead of panicking; use `if let`
to unwrap the result.

<a id="dict.get_or"></a>

**`get_or(key K, default_value V) V`** — *`@native`*

Return the value for `key` if present, else `default_value`.

<a id="dict.update"></a>

**`update(src Iterable[(K, V)])`** — *`@native`*

Merge all (key, value) pairs from `src` into the dict,
replacing the values of existing keys.

<a id="dict.setdefault"></a>

**`setdefault(key K, default_value V) V`** — *`@native`*

Return the value for `key` if it is in the dict.
Otherwise insert `key` with `default_value` and return `default_value`.

<a id="dict.__bitor__"></a>

**`__bitor__(src Iterable[(K, V)]) dict[K, V]`** — *`@native`*

Return a new dict with mappings of `self` followed by `src` (`self | src`).
Values from `src` replace those of duplicate keys.

<a id="dict.__ior__"></a>

**`__ior__(src Iterable[(K, V)]) dict[K, V]`** — *`@native`*

Merge all (key, value) pairs from `src` into the dict in place,
replacing the values of existing keys (`self |= src`).

<a id="dict.reversed"></a>

**`reversed(step = 1) Iterator[(K, V)]`**

Return a reverse iterator over the (key, value) pairs in reverse insertion order.

<a id="dict.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the dict for display.

<a id="dictkeys"></a>

### `DictKeys`

```kl
class DictKeys[K, V] : Iterable[K] {
    let _dict dict[K, V]
    func __init__(d dict[K, V])
    func iter(step = 1) Iterator[K]
}
```
**Bases:** [`Iterable[K]`](#iterable)


Lightweight view returned by `dict.keys()`. keys are returned in insertion order.

---

**Fields**

<a id="dictkeys._dict"></a>

**`let _dict dict[K, V]`**

internal reference to the underlying dictionary.

<a id="dictkeys.__init__"></a>

**`__init__(d dict[K, V])`**

Initialize the DictKeys view with the given dictionary.

<a id="dictkeys.iter"></a>

**`iter(step = 1) Iterator[K]`**

Return an iterator over the keys in the dict.

<a id="dictkeysiter"></a>

### `DictKeysIter`

```kl
class DictKeysIter[K, V] : Iterator[K] {
    func __init__(d dict[K, V], step int)
    func has_next() bool
    func next_strict() K
    func next() K?
}
```
**Bases:** [`Iterator[K]`](#iterator)


Iterator over dict keys in insertion order.

---

<a id="dictkeysiter.__init__"></a>

**`__init__(d dict[K, V], step int)`** — *`@native`*

Initialize the DictKeysIter with the given dictionary and step size.

<a id="dictkeysiter.has_next"></a>

**`has_next() bool`** — *`@native`*

Return true if there are more keys to iterate over.

<a id="dictkeysiter.next_strict"></a>

**`next_strict() K`** — *`@native`*

Return the next value in the iteration. Raises an error if iteration is complete.

<a id="dictkeysiter.next"></a>

**`next() K?`** — *`@native`*

Return the next key in the iteration, or `nil` if iteration is complete.

<a id="dictvalues"></a>

### `DictValues`

```kl
class DictValues[K, V] : Iterable[V] {
    let _dict dict[K, V]
    func __init__(d dict[K, V])
    func iter(step = 1) Iterator[V]
}
```
**Bases:** [`Iterable[V]`](#iterable)


Lightweight view returned by `dict.values()`. Values are returned in insertion order.

---

**Fields**

<a id="dictvalues._dict"></a>

**`let _dict dict[K, V]`**

internal reference to the underlying dictionary.

<a id="dictvalues.__init__"></a>

**`__init__(d dict[K, V])`**

Initialize the DictValues view with the given dictionary.

<a id="dictvalues.iter"></a>

**`iter(step = 1) Iterator[V]`**

Return an iterator over the values in the dict.

<a id="dictvaluesiter"></a>

### `DictValuesIter`

```kl
class DictValuesIter[K, V] : Iterator[V] {
    func __init__(d dict[K, V], step int)
    func has_next() bool
    func next_strict() V
    func next() V?
}
```
**Bases:** [`Iterator[V]`](#iterator)


Iterator over dict values in insertion order.

---

<a id="dictvaluesiter.__init__"></a>

**`__init__(d dict[K, V], step int)`** — *`@native`*

Initialize the DictValuesIter with the given dictionary and step size.

<a id="dictvaluesiter.has_next"></a>

**`has_next() bool`** — *`@native`*

Return true if there are more values to iterate over.

<a id="dictvaluesiter.next_strict"></a>

**`next_strict() V`** — *`@native`*

Return the next value in the iteration. Raises an error if iteration is complete.

<a id="dictvaluesiter.next"></a>

**`next() V?`** — *`@native`*

Return the next value in the iteration, or `nil` if iteration is complete.

<a id="dictview"></a>

### `DictView`

```kl
class DictView[K, V] : Iterable[(K, V)] {
    let _dict dict[K, V]
    func __init__(d dict[K, V])
    func iter(step = 1) Iterator[(K, V)]
}
```
**Bases:** [`Iterable[(K, V)]`](#iterable)


Lightweight view returned by `dict.items()`. Key-value pairs are returned in insertion order.

---

**Fields**

<a id="dictview._dict"></a>

**`let _dict dict[K, V]`**

internal reference to the underlying dictionary.

<a id="dictview.__init__"></a>

**`__init__(d dict[K, V])`**

Initialize the DictView view with the given dictionary.

<a id="dictview.iter"></a>

**`iter(step = 1) Iterator[(K, V)]`**

Return an iterator over the key-value pairs in the dict.

<a id="dictviewiter"></a>

### `DictViewIter`

```kl
class DictViewIter[K, V] : Iterator[(K, V)] {
    func __init__(d dict[K, V], step int)
    func has_next() bool
    func next_strict() (K, V)
    func next() (K, V)?
}
```
**Bases:** [`Iterator[(K, V)]`](#iterator)


Iterator over dict items in insertion order.
Used by `DictItems` and `dict.iter()` to provide iteration functionality.

---

<a id="dictviewiter.__init__"></a>

**`__init__(d dict[K, V], step int)`** — *`@native`*

Initialize the DictViewIter with the given dictionary and step size.

<a id="dictviewiter.has_next"></a>

**`has_next() bool`** — *`@native`*

Return true if there are more items to iterate over.

<a id="dictviewiter.next_strict"></a>

**`next_strict() (K, V)`** — *`@native`*

Return the next key-value pair in the iteration. Raises an error if iteration is complete.

<a id="dictviewiter.next"></a>

**`next() (K, V)?`** — *`@native`*

Return the next key-value pair in the iteration, or `nil` if iteration is complete.

<a id="dictreviter"></a>

### `DictRevIter`

```kl
class DictRevIter[K, V] : Iterator[(K, V)] {
    func __init__(d dict[K, V], step int)
    func has_next() bool
    func next_strict() (K, V)
    func next() (K, V)?
}
```
**Bases:** [`Iterator[(K, V)]`](#iterator)


Iterator over dict items in reverse insertion order.
Used by `dict.reversed()` to provide reverse iteration functionality.

---

<a id="dictreviter.__init__"></a>

**`__init__(d dict[K, V], step int)`** — *`@native`*

Initialize the DictRevIter with the given dictionary and step size.

<a id="dictreviter.has_next"></a>

**`has_next() bool`** — *`@native`*

Return true if there are more items to iterate over.

<a id="dictreviter.next_strict"></a>

**`next_strict() (K, V)`** — *`@native`*

Return the next key-value pair in the iteration. Raises an error if iteration is complete.

<a id="dictreviter.next"></a>

**`next() (K, V)?`** — *`@native`*

Return the next key-value pair in the iteration, or `nil` if iteration is complete.

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

<a id="formatter"></a>

### `Formatter`

```kl
pub class Formatter {
    pub func __init__(size = 32)
    pub func write_fmt(fmt str, args ...)
    pub func write_str(s str)
    pub func write_byte(value uint8)
    pub func clear()
    pub func result() str
    pub func write(value Printable)
}
```

`Formatter` builds formatted text in an internal buffer.

It provides formatted output through [`write_fmt`](#write_fmt). A
`Formatter` is passed to [`Printable`](#printable) implementations so
values can write their human-readable representation directly into the
same buffer without creating intermediate strings.

---

<a id="formatter.__init__"></a>

**`__init__(size = 32)`** — *`@native`*

Initialize the formatter with an optional buffer size.

<a id="formatter.write_fmt"></a>

**`write_fmt(fmt str, args ...)`** — *`@native`*

Append a formatted string to the buffer.

<a id="formatter.write_str"></a>

**`write_str(s str)`** — *`@native`*

Append a raw string to the buffer.

<a id="formatter.write_byte"></a>

**`write_byte(value uint8)`** — *`@native`*

Append a single byte to the buffer.

<a id="formatter.clear"></a>

**`clear()`** — *`@native`*

Clear all formatted content from the buffer.

<a id="formatter.result"></a>

**`result() str`** — *`@native`*

Return the formatted result.

<a id="formatter.write"></a>

**`write(value Printable)`**

Writes the formatted representation of `value` into the formatter.

<a id="hashset"></a>

### `hashset`

```kl
pub class hashset[T] : MutableSet[T] {
    pub func __init__(items Iterable[T]? = nil)
    pub func iter(step = 1) Iterator[T]
    pub func len() int
    pub func empty() bool
    pub func __contains__(v T) bool
    pub func add(v T) bool
    pub func update(items Iterable[T])
    pub func remove(v T) bool
    pub func clear()
    pub func is_subset(other Iterable[T]) bool
    pub func is_superset(other Iterable[T]) bool
    pub func union(other Iterable[T]) hashset[T]
    pub func intersect(other Iterable[T]) hashset[T]
    pub func diff(other Iterable[T]) hashset[T]
    pub func sym_diff(other Iterable[T]) hashset[T]
    pub func __bitor__(other Iterable[T]) hashset[T]
    pub func __bitand__(other Iterable[T]) hashset[T]
    pub func __sub__(other Iterable[T]) hashset[T]
    pub func to_list() list[T]
}
```
**Bases:** [`MutableSet[T]`](#mutableset)


A hashset is a mutable, unordered collection of unique elements,
backed by a hash table.

It implements `MutableSet`, providing membership tests, insertion, removal,
and set algebra. Iteration order is unspecified.

---

<a id="hashset.__init__"></a>

**`__init__(items Iterable[T]? = nil)`** — *`@native`*

Create an empty hash set, or populate from `items` if provided.

<a id="hashset.iter"></a>

**`iter(step = 1) Iterator[T]`**

Return an iterator over the elements. Order is unspecified.

<a id="hashset.len"></a>

**`len() int`** — *`@native`*

Return the number of elements.

<a id="hashset.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the set is empty, false otherwise.

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

**`union(other Iterable[T]) hashset[T]`** — *`@native`*

Return a new set with elements from the set and `other`.

<a id="hashset.intersect"></a>

**`intersect(other Iterable[T]) hashset[T]`** — *`@native`*

Return a new set with elements common to the set and `other`.

<a id="hashset.diff"></a>

**`diff(other Iterable[T]) hashset[T]`** — *`@native`*

Return a new set with elements in the set that are not in `other`.

<a id="hashset.sym_diff"></a>

**`sym_diff(other Iterable[T]) hashset[T]`** — *`@native`*

Return a new set with elements in the set or `other`, but not both.

<a id="hashset.__bitor__"></a>

**`__bitor__(other Iterable[T]) hashset[T]`** — *`@native`*

Return a new set with elements from the set and `other` (`self | other`).

<a id="hashset.__bitand__"></a>

**`__bitand__(other Iterable[T]) hashset[T]`** — *`@native`*

Return a new set with elements common to the set and `other` (`self & other`).

<a id="hashset.__sub__"></a>

**`__sub__(other Iterable[T]) hashset[T]`** — *`@native`*

Return a new set with elements in the set that are not in `other` (`self - other`).

<a id="hashset.to_list"></a>

**`to_list() list[T]`** — *`@native`*

Return a list of all elements. Order is unspecified.

<a id="hashsetiter"></a>

### `HashSetIter`

```kl
class HashSetIter[T] : Iterator[T] {
    func __init__(s hashset[T], step int)
    func has_next() bool
    func next_strict() T
    func next() T?
}
```
**Bases:** [`Iterator[T]`](#iterator)


Iterator over hashset elements. Order is unspecified.

---

<a id="hashsetiter.__init__"></a>

**`__init__(s hashset[T], step int)`** — *`@native`*

Initialize the HashSetIter with the given hashset and step size.
`step` must be positive; panics otherwise.

<a id="hashsetiter.has_next"></a>

**`has_next() bool`** — *`@native`*

Return true if there are more elements to iterate over.

<a id="hashsetiter.next_strict"></a>

**`next_strict() T`** — *`@native`*

Return the next element in the iteration. Panic if the iterator is exhausted.

<a id="hashsetiter.next"></a>

**`next() T?`** — *`@native`*

Return the next element in the iteration, or `nil` if iteration is complete.

<a id="list"></a>

### `list`

```kl
pub class list[T] : MutableSequence[T] {
    pub func __init__(args ...T)
    pub func iter(step = 1) Iterator[T]
    pub func len() int
    pub func empty() bool
    pub func __getitem__(index int) T
    pub func __getslice__(s slice) list[T]
    pub func __contains__(v T) bool
    pub func index(value T, start = 0, end = -1) int
    pub func count(value T, start = 0, end = -1) int
    pub func __setitem__(index int, value T)
    pub func __setslice__(s slice, val Iterable[T])
    pub func append(value T)
    pub func extend(items Iterable[T])
    pub func insert(index int, value T)
    pub func remove(value T)
    pub func delete(start int, end int? = nil)
    pub func pop(index = -1) T
    pub func copy(start = 0, end = -1) list[T]
    pub func clear()
    pub func reverse()
    pub func __add__(seq list[T]) list[T]
    pub func __iadd__(seq Iterable[T]) list[T]
    pub func reversed(step = 1) Iterator[T]
    pub func fmt(f Formatter)
    pub func sort()
}
```
**Bases:** [`MutableSequence[T]`](#mutablesequence)


A list is a mutable sequence of elements.

It implements `MutableSequence`, providing dynamic array functionality:
random access, appending, insertion, deletion and slicing.

Examples

```kl
let values = list[int](1, 2, 3)
values.append(4)
print(values[0])
print(values.pop())
```

---

<a id="list.__init__"></a>

**`__init__(args ...T)`** — *`@native`*

Initialize a list with the given items.

<a id="list.iter"></a>

**`iter(step = 1) Iterator[T]`**

Return an iterator over the list, advancing by `step` elements.

<a id="list.len"></a>

**`len() int`** — *`@native`*

Return the number of items in the list.

<a id="list.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the list is empty, false otherwise.

<a id="list.__getitem__"></a>

**`__getitem__(index int) T`** — *`@native`*

Get the item at the specified index.  Panic if the index is out of range.

<a id="list.__getslice__"></a>

**`__getslice__(s slice) list[T]`** — *`@native`*

Return a list that is a sub-slice of the current list.

<a id="list.__contains__"></a>

**`__contains__(v T) bool`** — *`@native`*

Return true if the list contains the specified value `v`.

<a id="list.index"></a>

**`index(value T, start = 0, end = -1) int`** — *`@native`*

Return the first index of `value` in `[start, end)`.
If `end` is `-1`, the search extends to the end of the list.
Return `-1` if the value is not present.

<a id="list.count"></a>

**`count(value T, start = 0, end = -1) int`** — *`@native`*

Return the number of occurrences of `value` in `[start, end)`.
If `end` is `-1`, the search extends to the end of the list.

<a id="list.__setitem__"></a>

**`__setitem__(index int, value T)`** — *`@native`*

Set the item at the specified index to `value`.  Panic if the index is out of range.

<a id="list.__setslice__"></a>

**`__setslice__(s slice, val Iterable[T])`** — *`@native`*

Replace a slice of the list with elements from the iterable `val`.
The length of the list never changes.

<a id="list.append"></a>

**`append(value T)`** — *`@native`*

Append a new item with `value` to the end of the list.

<a id="list.extend"></a>

**`extend(items Iterable[T])`** — *`@native`*

Extend the list by appending all items from `items`.

<a id="list.insert"></a>

**`insert(index int, value T)`** — *`@native`*

Insert a new item with `value` at the given `index`.

<a id="list.remove"></a>

**`remove(value T)`** — *`@native`*

Remove the first occurrence of `value` from the list. Panic if `value` is not present.

<a id="list.delete"></a>

**`delete(start int, end int? = nil)`** — *`@native`*

Remove elements in the index range `[start, end)`.
Negative `start` or `end` counts from the end of the list.
If `end` is `nil`, it removes elements from `start` to the end of the list.
Panic if `start` is out of bounds. An empty range (start >= end) is a no-op.

<a id="list.pop"></a>

**`pop(index = -1) T`** — *`@native`*

Remove and return the item at the given `index`.
If no index is specified, removes and returns the last item (`index = -1`).

<a id="list.copy"></a>

**`copy(start = 0, end = -1) list[T]`** — *`@native`*

Return a shallow copy of the list.

<a id="list.clear"></a>

**`clear()`** — *`@native`*

Remove all items from the list, leaving it empty.

<a id="list.reverse"></a>

**`reverse()`** — *`@native`*

Reverse the elements of the list in place.

<a id="list.__add__"></a>

**`__add__(seq list[T]) list[T]`** — *`@native`*

Return a new list with items of `self` followed by `seq`

<a id="list.__iadd__"></a>

**`__iadd__(seq Iterable[T]) list[T]`** — *`@native`*

Extend the list in place by appending all items from `seq`

<a id="list.reversed"></a>

**`reversed(step = 1) Iterator[T]`**

Return a reverse iterator over the elements of the list.

<a id="list.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the list for display.

<a id="list.sort"></a>

**`sort()`** — *`@native`*

Sort the elements of the list in place.

<a id="listiter"></a>

### `ListIter`

```kl
pub class ListIter[T] : Iterator[T] {
    let source list[T]
    var cursor int
    let step int
    let len int
    pub func __init__(_source list[T], _step = 1)
    pub func has_next() bool
    pub func next_strict() T
    pub func next() T?
    pub func fmt(f Formatter)
}
```
**Bases:** [`Iterator[T]`](#iterator)


A forward iterator over a `list`, advancing by `step` elements each turn.

It yields `source[0]`, `source[step]`, `source[2*step]`, ... while the
cursor stays within the list. The iterator is stateful and does not
support rewinding; the length is snapshotted at construction, so
mutating the source during traversal is undefined.

---

**Fields**

<a id="listiter.source"></a>

**`let source list[T]`**

<a id="listiter.cursor"></a>

**`var cursor int`**

<a id="listiter.step"></a>

**`let step int`**

<a id="listiter.len"></a>

**`let len int`**

<a id="listiter.__init__"></a>

**`__init__(_source list[T], _step = 1)`**

Initialize a forward iterator over `_source`, advancing by `_step`
elements per step. `_step` must be positive; defaults to 1.
Panic if `_step <= 0`.

<a id="listiter.has_next"></a>

**`has_next() bool`**

Return true if another element is available (the cursor is within bounds).

<a id="listiter.next_strict"></a>

**`next_strict() T`**

Return the element at the current cursor and advance by `step`.
Panic if the iterator is exhausted.

<a id="listiter.next"></a>

**`next() T?`**

Return the element at the current cursor and advance by `step`,
or `nil` if the iterator is exhausted.

<a id="listiter.fmt"></a>

**`fmt(f Formatter)`**

Format the iterator for display.

<a id="listreviter"></a>

### `ListRevIter`

```kl
pub class ListRevIter[T] : Iterator[T] {
    let source list[T]
    var cursor int
    let step int
    let len int
    pub func __init__(_source list[T], _step = 1)
    pub func has_next() bool
    pub func next_strict() T
    pub func next() T?
    pub func fmt(f Formatter)
}
```
**Bases:** [`Iterator[T]`](#iterator)


A reverse iterator over a `list`, moving backward by `step` elements each turn.

It yields `source[len-1]`, `source[len-1-step]`, ... while the cursor stays
at or above index 0, i.e. `reversed` traversal with a positive stride. The
iterator is stateful and does not support rewinding; the length is
snapshotted at construction, so mutating the source during traversal is
undefined.

---

**Fields**

<a id="listreviter.source"></a>

**`let source list[T]`**

<a id="listreviter.cursor"></a>

**`var cursor int`**

<a id="listreviter.step"></a>

**`let step int`**

<a id="listreviter.len"></a>

**`let len int`**

<a id="listreviter.__init__"></a>

**`__init__(_source list[T], _step = 1)`**

Initialize a reverse iterator over `_source`, moving backward by
`_step` elements per step. `_step` must be positive; defaults to 1.
Panic if `_step <= 0`.

<a id="listreviter.has_next"></a>

**`has_next() bool`**

Return true if another element is available (the cursor is at or above index 0).

<a id="listreviter.next_strict"></a>

**`next_strict() T`**

Return the element at the current cursor and move backward by `step`.
Panic if the iterator is exhausted.

<a id="listreviter.next"></a>

**`next() T?`**

Return the element at the current cursor and move backward by `step`,
or `nil` if the iterator is exhausted.

<a id="listreviter.fmt"></a>

**`fmt(f Formatter)`**

Format the iterator for display.

<a id="niltype"></a>

### `NilType`

```kl
pub class NilType {
    func __init__()
    pub func fmt(f Formatter)
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

<a id="niltype.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format `nil` for display.
Always writes `"nil"` to the formatter.

<a id="int8"></a>

### `int8`

```kl
pub class int8 : ToString {
    pub func __init__(x int64 | uint64 | float64)
    pub func fmt(f Formatter)
    pub func to_str() str
}
```
**Bases:** [`ToString`](#tostring)


An 8-bit signed integer.

Used to truncate a wider integer value down to 8 bits. Arithmetic operations
are performed via implicit promotion to `int64`.

---

<a id="int8.__init__"></a>

**`__init__(x int64 | uint64 | float64)`** — *`@intrinsic`*

Creates an `int8` by truncating `x` to 8 bits.

<a id="int8.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the `int8` value for display.

<a id="int8.to_str"></a>

**`to_str() str`** — *`@native`*

Convert the `int8` value to its string form.

<a id="int16"></a>

### `int16`

```kl
pub class int16 : ToString {
    pub func __init__(x int64 | uint64 | float64)
    pub func fmt(f Formatter)
    pub func to_str() str
}
```
**Bases:** [`ToString`](#tostring)


A 16-bit signed integer.

Used to truncate a wider integer value down to 16 bits. Arithmetic operations
are performed via implicit promotion to `int64`.

---

<a id="int16.__init__"></a>

**`__init__(x int64 | uint64 | float64)`** — *`@intrinsic`*

Creates an `int16` by truncating `x` to 16 bits.

<a id="int16.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the `int16` value for display.

<a id="int16.to_str"></a>

**`to_str() str`** — *`@native`*

Convert the `int16` value to its string form.

<a id="int32"></a>

### `int32`

```kl
pub class int32 : ToString {
    pub func __init__(x int64 | uint64 | float64)
    pub func fmt(f Formatter)
    pub func to_str() str
}
```
**Bases:** [`ToString`](#tostring)


A 32-bit signed integer.

Used to truncate a wider integer value down to 32 bits. Arithmetic operations
are performed via implicit promotion to `int64`.

---

<a id="int32.__init__"></a>

**`__init__(x int64 | uint64 | float64)`** — *`@intrinsic`*

Creates an `int32` by truncating `x` to 32 bits.

<a id="int32.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the `int32` value for display.

<a id="int32.to_str"></a>

**`to_str() str`** — *`@native`*

Convert the `int32` value to its string form.

<a id="int64"></a>

### `int64`

```kl
pub class int64 : Comparable & Arithmetic & Bitwise & ToString {
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
    pub func fmt(f Formatter)
    pub func to_str() str
    pub func bit_length() int
    pub func bit_count() int
    pub func bit_ctz() int
    pub func bit_clz() int
    pub func bit_ffs() int
    pub func bit_reverse() int
    pub func to_bytes(length int, byteorder int) bytes
    pub static func from_bytes(b bytes, byteorder int) int64
}
```
**Bases:** [`Comparable`](#comparable) & [`Arithmetic`](#arithmetic) & [`Bitwise`](#bitwise) & [`ToString`](#tostring)


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

<a id="int64.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the `int64` value for display.

<a id="int64.to_str"></a>

**`to_str() str`** — *`@native`*

Convert the `int64` value to its string form.

<a id="int64.bit_length"></a>

**`bit_length() int`** — *`@native`*

Returns the number of bits required to represent `self`.

<a id="int64.bit_count"></a>

**`bit_count() int`** — *`@native`*

Returns the number of 1-bits in the binary representation of `self`.

<a id="int64.bit_ctz"></a>

**`bit_ctz() int`** — *`@native`*

Returns the number of trailing 0-bits in the binary representation of `self`.

<a id="int64.bit_clz"></a>

**`bit_clz() int`** — *`@native`*

Returns the number of leading 0-bits in the binary representation of `self`.

<a id="int64.bit_ffs"></a>

**`bit_ffs() int`** — *`@native`*

Returns the position of the first set bit (1-indexed), or 0 if `self` is 0.

<a id="int64.bit_reverse"></a>

**`bit_reverse() int`** — *`@native`*

Returns the value with bits reversed (bit 0 swaps with bit 63, etc.).

<a id="int64.to_bytes"></a>

**`to_bytes(length int, byteorder int) bytes`** — *`@native`*

Converts `self` to a byte array of the specified `length` and `byteorder`.

<a id="int64.from_bytes"></a>

**`static from_bytes(b bytes, byteorder int) int64`** — *`@native`*

Creates an `int64` from a byte array with the specified `byteorder`.

<a id="uint8"></a>

### `uint8`

```kl
pub class uint8 : ToString {
    pub func __init__(x int64 | uint64 | float64)
    pub func fmt(f Formatter)
    pub func to_str() str
}
```
**Bases:** [`ToString`](#tostring)


An 8-bit unsigned integer.

Used to truncate a wider integer value down to 8 bits. Arithmetic operations
are performed via implicit promotion to `uint64`.

---

<a id="uint8.__init__"></a>

**`__init__(x int64 | uint64 | float64)`** — *`@intrinsic`*

Creates a `uint8` by truncating `x` to 8 bits.

<a id="uint8.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the `uint8` value for display.

<a id="uint8.to_str"></a>

**`to_str() str`** — *`@native`*

Convert the `uint8` value to its string form.

<a id="uint16"></a>

### `uint16`

```kl
pub class uint16 : ToString {
    pub func __init__(x int64 | uint64 | float64)
    pub func fmt(f Formatter)
    pub func to_str() str
}
```
**Bases:** [`ToString`](#tostring)


A 16-bit unsigned integer.

Used to truncate a wider integer value down to 16 bits. Arithmetic operations
are performed via implicit promotion to `uint64`.

---

<a id="uint16.__init__"></a>

**`__init__(x int64 | uint64 | float64)`** — *`@intrinsic`*

Creates a `uint16` by truncating `x` to 16 bits.

<a id="uint16.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the `uint16` value for display.

<a id="uint16.to_str"></a>

**`to_str() str`** — *`@native`*

Convert the `uint16` value to its string form.

<a id="uint32"></a>

### `uint32`

```kl
pub class uint32 : ToString {
    pub func __init__(x int64 | uint64 | float64)
    pub func fmt(f Formatter)
    pub func to_str() str
}
```
**Bases:** [`ToString`](#tostring)


A 32-bit unsigned integer.

Used to truncate a wider integer value down to 32 bits. Arithmetic operations
are performed via implicit promotion to `uint64`.

---

<a id="uint32.__init__"></a>

**`__init__(x int64 | uint64 | float64)`** — *`@intrinsic`*

Creates a `uint32` by truncating `x` to 32 bits.

<a id="uint32.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the `uint32` value for display.

<a id="uint32.to_str"></a>

**`to_str() str`** — *`@native`*

Convert the `uint32` value to its string form.

<a id="uint64"></a>

### `uint64`

```kl
pub class uint64 : Comparable & Arithmetic & Bitwise & ToString {
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
    pub func fmt(f Formatter)
    pub func to_str() str
    pub func bit_length() int
    pub func bit_count() int
    pub func bit_ctz() int
    pub func bit_clz() int
    pub func bit_ffs() int
    pub func bit_reverse() int
    pub func to_bytes(length int, byteorder int) bytes
    pub static func from_bytes(b bytes, byteorder int) uint64
}
```
**Bases:** [`Comparable`](#comparable) & [`Arithmetic`](#arithmetic) & [`Bitwise`](#bitwise) & [`ToString`](#tostring)


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

<a id="uint64.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the `uint64` value for display.

<a id="uint64.to_str"></a>

**`to_str() str`** — *`@native`*

Convert the `uint64` value to its string form.

<a id="uint64.bit_length"></a>

**`bit_length() int`** — *`@native`*

Returns the number of bits required to represent `self`.

<a id="uint64.bit_count"></a>

**`bit_count() int`** — *`@native`*

Returns the number of 1-bits in the binary representation of `self`.

<a id="uint64.bit_ctz"></a>

**`bit_ctz() int`** — *`@native`*

Returns the number of trailing 0-bits in the binary representation of `self`.

<a id="uint64.bit_clz"></a>

**`bit_clz() int`** — *`@native`*

Returns the number of leading 0-bits in the binary representation of `self`.

<a id="uint64.bit_ffs"></a>

**`bit_ffs() int`** — *`@native`*

Returns the position of the first set bit (1-indexed), or 0 if `self` is 0.

<a id="uint64.bit_reverse"></a>

**`bit_reverse() int`** — *`@native`*

Returns the value with bits reversed (bit 0 swaps with bit 63, etc.).

<a id="uint64.to_bytes"></a>

**`to_bytes(length int, byteorder int) bytes`** — *`@native`*

Converts `self` to a byte array of the specified `length` and `byteorder`.

<a id="uint64.from_bytes"></a>

**`static from_bytes(b bytes, byteorder int) uint64`** — *`@native`*

Creates a `uint64` from a byte array with the specified `byteorder`.

<a id="float16"></a>

### `float16`

```kl
pub class float16 : ToString {
    pub func __init__(x int64 | uint64 | float64)
    pub func fmt(f Formatter)
    pub func to_str() str
}
```
**Bases:** [`ToString`](#tostring)


A 16-bit floating-point number (half precision).

Used to truncate a `float64` value down to 16 bits. Arithmetic operations
are performed via implicit promotion to `float64`.

---

<a id="float16.__init__"></a>

**`__init__(x int64 | uint64 | float64)`** — *`@intrinsic`*

Creates a `float16` by truncating `x` to 16 bits.

<a id="float16.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the `float16` value for display.

<a id="float16.to_str"></a>

**`to_str() str`** — *`@native`*

Convert the `float16` value to its string form.

<a id="float32"></a>

### `float32`

```kl
pub class float32 : ToString {
    pub func __init__(x int64 | uint64 | float64)
    pub func fmt(f Formatter)
    pub func to_str() str
}
```
**Bases:** [`ToString`](#tostring)


A 32-bit floating-point number (single precision).

Used to truncate a `float64` value down to 32 bits. Arithmetic operations
are performed via implicit promotion to `float64`.

---

<a id="float32.__init__"></a>

**`__init__(x int64 | uint64 | float64)`** — *`@intrinsic`*

Creates a `float32` by truncating `x` to 32 bits.

<a id="float32.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the `float32` value for display.

<a id="float32.to_str"></a>

**`to_str() str`** — *`@native`*

Convert the `float32` value to its string form.

<a id="float64"></a>

### `float64`

```kl
pub class float64 : Comparable & Arithmetic & ToString {
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
    pub func hash() int
    pub func fmt(f Formatter)
    pub func to_str() str
    pub func to_str_exact() str
    pub func is_integer() bool
    pub func as_integer_ratio() (int64, int64)
}
```
**Bases:** [`Comparable`](#comparable) & [`Arithmetic`](#arithmetic) & [`ToString`](#tostring)


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

<a id="float64.hash"></a>

**`hash() int`** — *`@native`*

Returns a hash value for `self`.

<a id="float64.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the `float64` value for display.

<a id="float64.to_str"></a>

**`to_str() str`** — *`@native`*

Convert the `float64` value to its string form.

<a id="float64.to_str_exact"></a>

**`to_str_exact() str`** — *`@native`*

Convert the `float64` value to the shortest string that parses back to
the exact same value. Prefer this over `to_str()` when the text will be
stored or transmitted: `to_str()` keeps 16 significant digits for
readability, which cannot name every `float64`.

<a id="float64.is_integer"></a>

**`is_integer() bool`** — *`@native`*

Returns `true` if `self` is an integer value.

<a id="float64.as_integer_ratio"></a>

**`as_integer_ratio() (int64, int64)`** — *`@native`*

Returns a pair (numerator, denominator) such that `self = numerator / denominator`.

<a id="bfloat16"></a>

### `bfloat16`

```kl
pub class bfloat16 : ToString {
    pub func __init__(x int64 | uint64 | float64)
    pub func fmt(f Formatter)
    pub func to_str() str
}
```
**Bases:** [`ToString`](#tostring)


A 16-bit brain floating-point type (bfloat16).

`bfloat16` uses 8 exponent bits and 7 mantissa bits, providing the same
dynamic range as `float32` but with reduced precision. It is primarily
designed for machine learning workloads where memory bandwidth and storage
efficiency matter more than exact numerical accuracy.

---

<a id="bfloat16.__init__"></a>

**`__init__(x int64 | uint64 | float64)`** — *`@intrinsic`*

Creates a `bfloat16` by truncating the lower bits of `x`.

When converting from `float64`, the value is rounded to the nearest
representable `bfloat16`. Integer inputs are converted via their
exact float representation before truncation.

<a id="bfloat16.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the `bfloat16` value for display.

<a id="bfloat16.to_str"></a>

**`to_str() str`** — *`@native`*

Convert the `bfloat16` value to its string form.

<a id="range"></a>

### `range`

```kl
pub class range : Sequence[int] {
    pub let start int
    pub let end int
    pub let step int
    pub func __init__(_start int, _end int, _step = 1)
    pub func iter(_step = 1) Iterator[int]
    pub func len() int
    pub func empty() bool
    pub func __getitem__(index int) int
    pub func __getslice__(s slice) range
    pub func __contains__(item int) bool
    pub func index(value int, _start = 0, _end = -1) int
    pub func count(value int, _start = 0, _end = -1) int
    pub func fmt(f Formatter)
    pub func to_str() str
}
```
**Bases:** [`Sequence[int]`](#sequence)


A range represents an immutable sequence of integers.

It is commonly used for looping a specific number of times in `for` loops.

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

<a id="range.iter"></a>

**`iter(_step = 1) Iterator[int]`** — *`@native`*

Return an iterator for the range.

<a id="range.len"></a>

**`len() int`** — *`@native`*

Return the number of elements in the range.

<a id="range.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the range is empty.

<a id="range.__getitem__"></a>

**`__getitem__(index int) int`** — *`@native`*

Get the integer at the specified index.

<a id="range.__getslice__"></a>

**`__getslice__(s slice) range`** — *`@native`*

Return a new range that is a sub-slice of the current range.

<a id="range.__contains__"></a>

**`__contains__(item int) bool`** — *`@native`*

Return true if the specified item is part of the range.

<a id="range.index"></a>

**`index(value int, _start = 0, _end = -1) int`** — *`@native`*

Return the index of `value` in the range, restricted to `[_start, _end)`.
If `_end` is `-1`, the search extends to the end of the range.
Return `-1` if the value is not present.

<a id="range.count"></a>

**`count(value int, _start = 0, _end = -1) int`** — *`@native`*

Return the number of occurrences of `value` in `[_start, _end)`.
Since all values in a range are unique, this returns 1 if the value
is present, otherwise 0.
If `_end` is `-1`, the search extends to the end of the range.

<a id="range.fmt"></a>

**`fmt(f Formatter)`**

Format the range for display.

<a id="range.to_str"></a>

**`to_str() str`**

Convert the range to its string representation.

<a id="slice"></a>

### `slice`

```kl
pub class slice {
    pub let start int
    pub let end int
    pub let step int
    pub func __init__(_start int, _end int, _step = 1)
    pub func fmt(f Formatter)
    pub func to_str() str
}
```

A slice object represents a set of indices specified by `[start:end:step]`.

It is created by the compiler when the `:` syntax is used inside subscript
brackets `[]`. Omitted bounds are represented internally by `INT_MIN` and
resolved against the target sequence length when the slice is applied.

---

**Fields**

<a id="slice.start"></a>

**`pub let start int`**

The starting index of the slice (inclusive).

<a id="slice.end"></a>

**`pub let end int`**

The ending index of the slice (exclusive).

For reverse slices, `-1` means the slice extends through index 0.

<a id="slice.step"></a>

**`pub let step int`**

The step value of the slice.

A negative step reverses the direction. Must not be zero.

<a id="slice.__init__"></a>

**`__init__(_start int, _end int, _step = 1)`**

Initialize a slice.

Pass `INT_MIN` for `_start` or `_end` to represent an omitted bound.
The bound is resolved against the target sequence length when the slice is applied.

`_step` defaults to 1 and must not be zero.

<a id="slice.fmt"></a>

**`fmt(f Formatter)`**

Format the slice as `slice(start, end, step)`.

<a id="slice.to_str"></a>

**`to_str() str`**

Convert the slice to its string representation.

<a id="str"></a>

### `str`

```kl
pub class str : Sequence & Comparable & ToString {
    pub func __init__(s ToString)
    pub func iter(step = 1) Iterator[str]
    pub func reversed(step = 1) Iterator[str]
    pub func len() int
    pub func empty() bool
    pub func __getitem__(index int) str
    pub func __getslice__(s slice) str
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
    pub func substr(start = 0, end = -1) str
    pub func fmt(f Formatter)
    pub func to_str() str
    pub func to_int() int
    pub func to_int_or(default int) int
    pub func to_float() float64
    pub func to_float_or(default float64) float64
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
    pub func starts_with(prefix str) bool
    pub func ends_with(suffix str) bool
    pub func remove_prefix(prefix str) str
    pub func remove_suffix(suffix str) str
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
    pub func view(start = 0, end = -1) str
}
```
**Bases:** [`Sequence`](#sequence) & [`Comparable`](#comparable) & [`ToString`](#tostring)


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

**`__init__(s ToString)`** — *`@native`*

Initialize a string by converting a value with `to_str()`.

`str(value)` is lowered by the compiler to `OP_STR`, which invokes
the value's `to_str()` method.

<a id="str.iter"></a>

**`iter(step = 1) Iterator[str]`**

Return an iterator over the characters of the string.

The iterator traverses the string from left to right, returning
one character per step. The optional `step` argument controls
the stride of iteration:

- `step = 1` produces a normal character-by-character traversal.
- `step > 1` skips characters, yielding a strided iteration.

The iterator is stateful and does not allocate; it reads directly
from the underlying string.

<a id="str.reversed"></a>

**`reversed(step = 1) Iterator[str]`**

Return a reverse iterator over the characters of the string.

The iterator traverses the string from right to left, returning
one character per step. The optional `step` argument controls
the stride of reverse iteration:

- `step = 1` produces a normal reverse traversal.
- `step > 1` skips characters in reverse order.

The iterator is stateful and does not allocate; it reads directly
from the underlying string.

<a id="str.len"></a>

**`len() int`** — *`@native`*

Return the length of the string.

<a id="str.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the string is empty, false otherwise.

<a id="str.__getitem__"></a>

**`__getitem__(index int) str`** — *`@native`*

Return the character at the specified index. Panic if the index is out of range.

<a id="str.__getslice__"></a>

**`__getslice__(s slice) str`** — *`@native`*

Return the substring defined by the slice.

<a id="str.__contains__"></a>

**`__contains__(sub str) bool`** — *`@native`*

Return true if the string contains the substring `sub`.

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

<a id="str.substr"></a>

**`substr(start = 0, end = -1) str`** — *`@native`*

Return the substring from `start` (inclusive) to `end` (exclusive).
If `end` is `-1`, the substring extends to the end of the string.

<a id="str.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the string for display.

Strings are already in their printable form, so no additional
formatting or conversion is performed.

<a id="str.to_str"></a>

**`to_str() str`**

Convert `self` to a string.

Returns `self`: a `str` is already its own string form. This keeps
generic code bounded by `ToString` uniform when `T` is instantiated with `str`.

<a id="str.to_int"></a>

**`to_int() int`** — *`@native`*

Parse the string as a decimal integer. Panic if the string is not a valid integer.

<a id="str.to_int_or"></a>

**`to_int_or(default int) int`** — *`@native`*

Parse the string as a decimal integer.
Return `default` if the string is not a valid integer.

<a id="str.to_float"></a>

**`to_float() float64`** — *`@native`*

Parse the string as a floating-point number. Panic if the string is not a valid number.

<a id="str.to_float_or"></a>

**`to_float_or(default float64) float64`** — *`@native`*

Parse the string as a floating-point number.
Return `default` if the string is not a valid number.

<a id="str.__add__"></a>

**`__add__(rhs str) str`** — *`@native`*

Return a new string with `self` followed by `rhs` (`self + rhs`).

<a id="str.__mul__"></a>

**`__mul__(n int) str`** — *`@native`*

Return a new string with `self` repeated `n` times (`self * n`).
Return an empty string if `n` is not positive.

<a id="str.join"></a>

**`join(seq Sequence[str]) str`** — *`@native`*

Return a string made by joining the elements of `seq`, with `self` as the separator.

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

<a id="str.starts_with"></a>

**`starts_with(prefix str) bool`** — *`@native`*

Return true if the string starts with `prefix`.

<a id="str.ends_with"></a>

**`ends_with(suffix str) bool`** — *`@native`*

Return true if the string ends with `suffix`.

<a id="str.remove_prefix"></a>

**`remove_prefix(prefix str) str`** — *`@native`*

Return a copy of the string with `prefix` removed
if the string starts with it, otherwise return the string unchanged.

<a id="str.remove_suffix"></a>

**`remove_suffix(suffix str) str`** — *`@native`*

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

<a id="str.view"></a>

**`view(start = 0, end = -1) str`** — *`@native`*

Return a view (substring) of the string from `start` to `end`.

If `end` is `-1`, the view extends to the end of the string.
Other negative indices are interpreted relative to the string length.

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
    pub func fmt(f Formatter)
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

<a id="striter.fmt"></a>

**`fmt(f Formatter)`**

Format the iterator state for display.
Produce a human-readable description of the iterator state.
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
    pub func fmt(f Formatter)
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

<a id="strreviter.fmt"></a>

**`fmt(f Formatter)`**

Format the iterator state for display.
Produce a human-readable description of the iterator state.

<a id="tuple"></a>

### `tuple`

```kl
pub class tuple[infer T] : Sequence[T] {
    pub func __init__(args ...T)
    pub func iter(step = 1) Iterator[T]
    pub func len() int
    pub func empty() bool
    pub func __getitem__[infer U](index int) U
    pub func __getslice__(s slice) tuple[T]
    pub func __contains__(v T) bool
    pub func index(value T, start = 0, end = -1) int
    pub func count(value T, start = 0, end = -1) int
    pub func to_list() list[T]
    pub func reversed(step = 1) Iterator[T]
    pub func fmt(f Formatter)
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

<a id="tuple.iter"></a>

**`iter(step = 1) Iterator[T]`** — *`@native`*

Return an iterator over the tuple.

<a id="tuple.len"></a>

**`len() int`** — *`@native`*

Return the number of elements in the tuple.

<a id="tuple.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the tuple is empty, false otherwise.

<a id="tuple.__getitem__"></a>

**`__getitem__[infer U](index int) U`** — *`@native`*

Get the element at the specified index.
The element type is inferred from the usage context.

Panic if the index is out of range.

<a id="tuple.__getslice__"></a>

**`__getslice__(s slice) tuple[T]`** — *`@native`*

Return the sub-tuple defined by the slice.

<a id="tuple.__contains__"></a>

**`__contains__(v T) bool`** — *`@native`*

Return true if the tuple contains the specified value `v`.

<a id="tuple.index"></a>

**`index(value T, start = 0, end = -1) int`** — *`@native`*

Return the first index of `value` in `[start, end)`.
If `end` is `-1`, the search extends to the end of the tuple.
Return `-1` if the value is not present.

<a id="tuple.count"></a>

**`count(value T, start = 0, end = -1) int`** — *`@native`*

Return the number of occurrences of `value` in `[start, end)`.
If `end` is `-1`, the search extends to the end of the tuple.

<a id="tuple.to_list"></a>

**`to_list() list[T]`** — *`@native`*

Return a new list containing the elements of the tuple.

<a id="tuple.reversed"></a>

**`reversed(step = 1) Iterator[T]`** — *`@native`*

Return a reverse iterator over the elements of the tuple.

<a id="tuple.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the tuple for display.

<a id="type"></a>

### `type`

```kl
pub class type {
    func __init__()
    pub func fmt(f Formatter)
    pub func to_str() str
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

<a id="type.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the type for display.

<a id="type.to_str"></a>

**`to_str() str`** — *`@native`*

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

<a id="unreachable"></a>

### `unreachable`

```kl
pub func unreachable()
```

*`@native`*

Signal that execution has reached code that should be unreachable.

`unreachable()` raises a fatal runtime error indicating a logic bug.
It is used to mark code paths that should never execute (e.g., after
exhaustive pattern matching or in default branches that should be
impossible to reach).

If execution reaches `unreachable()`, it indicates a bug in the program.

<a id="format"></a>

### `format`

```kl
pub func format(fmt str, args ...) str
```

*`@native`*

Format a string using positional arguments.

`{}` consumes one argument in order. No named placeholders.
Arguments are formatted using their `Printable` implementation.

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

<a id="iter"></a>

### `iter`

```kl
pub func iter[T](obj Iterable[T]) Iterator[T]
```

Return an iterator over the elements of an iterable.

<a id="next"></a>

### `next`

```kl
pub func next[T](it Iterator[T]) T?
```

Return the next element, or nil when exhausted.
Consume elements with `while let value = next(it)`.

<a id="len"></a>

### `len`

```kl
pub func len(obj Sized) int
```

*`@intrinsic`*

Return the number of elements in a sized collection.

<a id="abs"></a>

### `abs`

```kl
pub func abs[T](a T) T
```

Computes the absolute value using a matching specialization.

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

Panic if execution reaches the unspecialized fallback.

<a id="pow_int64_int64"></a>

### `pow_int64_int64`

```kl
pub func pow_int64_int64(base int, exp int) int
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

<a id="round"></a>

### `round`

```kl
pub func round(x float) float
```

*`@native`*

Return `x` rounded to the nearest integer value (as a float).

<a id="floor"></a>

### `floor`

```kl
pub func floor(x float) float
```

*`@native`*

Return the largest integer value less than or equal to `x` (as a float).

<a id="ceil"></a>

### `ceil`

```kl
pub func ceil(x float) float
```

*`@native`*

Return the smallest integer value greater than or equal to `x` (as a float).

<a id="divmod"></a>

### `divmod`

```kl
pub func divmod[T](a T, b T) (T, T)
```

Return `(a / b, a % b)` as a tuple.

Panic if `divmod` is not implemented for the given type.

<a id="divmod_int64"></a>

### `divmod_int64`

```kl
pub func divmod_int64(a int, b int) (int, int)
```

Specialized implementation of `divmod()` for 64-bit signed integers.

<a id="divmod_float64"></a>

### `divmod_float64`

```kl
pub func divmod_float64(a float, b float) (float, float)
```

Specialized implementation of `divmod()` for 64-bit floating-point numbers.

Returns `(q, r)` such that `a = q * b + r` and `q = floor(a / b)`.
This matches Python's `divmod` behavior (floor division).
The remainder `r` has the same sign as the divisor `b`.

## Constants

<a id="int8_min"></a>

### `INT8_MIN`

```kl
pub const INT8_MIN = -128
```

Built-in integer limits constants

<a id="int8_max"></a>

### `INT8_MAX`

```kl
pub const INT8_MAX = 127
```

<a id="int16_min"></a>

### `INT16_MIN`

```kl
pub const INT16_MIN = -32768
```

<a id="int16_max"></a>

### `INT16_MAX`

```kl
pub const INT16_MAX = 32767
```

<a id="int32_min"></a>

### `INT32_MIN`

```kl
pub const INT32_MIN = -2147483648
```

<a id="int32_max"></a>

### `INT32_MAX`

```kl
pub const INT32_MAX = 2147483647
```

<a id="int64_min"></a>

### `INT64_MIN`

```kl
pub const INT64_MIN = -9223372036854775808
```

<a id="int64_max"></a>

### `INT64_MAX`

```kl
pub const INT64_MAX = 9223372036854775807
```

<a id="uint8_min"></a>

### `UINT8_MIN`

```kl
pub const UINT8_MIN = 0
```

<a id="uint8_max"></a>

### `UINT8_MAX`

```kl
pub const UINT8_MAX = 255
```

<a id="uint16_min"></a>

### `UINT16_MIN`

```kl
pub const UINT16_MIN = 0
```

<a id="uint16_max"></a>

### `UINT16_MAX`

```kl
pub const UINT16_MAX = 65535
```

<a id="uint32_min"></a>

### `UINT32_MIN`

```kl
pub const UINT32_MIN = 0
```

<a id="uint32_max"></a>

### `UINT32_MAX`

```kl
pub const UINT32_MAX = 4294967295
```

<a id="uint64_min"></a>

### `UINT64_MIN`

```kl
pub const UINT64_MIN = 0
```

<a id="uint64_max"></a>

### `UINT64_MAX`

```kl
pub const UINT64_MAX = 18446744073709551615
```

<a id="big_endian"></a>

### `BIG_ENDIAN`

```kl
pub const BIG_ENDIAN = 0
```

<a id="little_endian"></a>

### `LITTLE_ENDIAN`

```kl
pub const LITTLE_ENDIAN = 1
```
