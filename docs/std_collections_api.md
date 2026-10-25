# Koala `std/collections` API Reference

## Contents

**Classes**

| Name | Summary |
|------|---------|
| [`BitSet`](#bitset) | A fixed-size sequence of bits. |
| [`Deque`](#deque) | A Deque (double-ended queue) is a generalization of a queue that supports efficient insertion and removal at both ends. |
| [`DequeIter`](#dequeiter) | A forward iterator over a `Deque`, advancing by `step` slots each turn. |
| [`DequeRevIter`](#dequereviter) | A backward iterator over a `Deque`, advancing by `step` slots each turn. |
| [`FrozenBytes`](#frozenbytes) | FrozenBytes is an immutable, fixed-size sequence of bytes. |
| [`FrozenDict`](#frozendict) | A FrozenDict is an immutable, unordered hash map that maps unique keys to values. |
| [`FrozenList`](#frozenlist) | A FrozenList is an immutable, ordered sequence of elements. |
| [`FrozenSet`](#frozenset) | A FrozenSet is an immutable, unordered collection of unique elements. |
| [`Heap`](#heap) | A Heap is a min-heap priority queue backed by a binary heap. |
| [`LinkedList`](#linkedlist) | A LinkedList is a doubly-linked list that supports efficient insertion and deletion at both ends. |
| [`LinkedListIter`](#linkedlistiter) | A forward iterator over a `LinkedList`, advancing by `step` nodes each turn. |
| [`LinkedListRevIter`](#linkedlistreviter) | A backward iterator over a `LinkedList`, advancing by `step` nodes each turn. |
| [`Queue`](#queue) | A Queue is a first-in, first-out (FIFO) collection. |
| [`SortedDict`](#sorteddict) | A SortedDict is a mutable, sorted hash map that maps unique keys to values, ordered by key. |
| [`SortedSet`](#sortedset) | A SortedSet is a mutable, sorted collection of unique elements, backed by a balanced tree. |
| [`Stack`](#stack) | A Stack is a last-in, first-out (LIFO) collection. |

## Classes

<a id="bitset"></a>

### `BitSet`

```kl
pub class BitSet {
    pub func __init__(size int)
    pub func fmt(f Formatter)
    pub func set(bit int)
    pub func clear(bit int)
    pub func test(bit int) bool
    pub func flip(bit int)
    pub func count() int
    pub func size() int
    pub func dump() str
    pub func __bitand__(other BitSet) BitSet
    pub func __bitor__(other BitSet) BitSet
    pub func __bitxor__(other BitSet) BitSet
    pub func __bitnot__() BitSet
    pub func to_bytes() bytes
    pub static func from_bytes(data bytes) BitSet
}
```

A fixed-size sequence of bits.

BitSet provides efficient storage for boolean values using individual bits.
Supports arbitrary-length bit operations and bitwise operators.

---

<a id="bitset.__init__"></a>

**`__init__(size int)`** — *`@native`*

Create a BitSet with `size` bits, all initialized to 0.

<a id="bitset.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the BitSet for display.

<a id="bitset.set"></a>

**`set(bit int)`** — *`@native`*

Set the bit at position `bit` to 1.

Panic if `bit` is out of range [0, size).

<a id="bitset.clear"></a>

**`clear(bit int)`** — *`@native`*

Clear the bit at position `bit` to 0.

Panic if `bit` is out of range [0, size).

<a id="bitset.test"></a>

**`test(bit int) bool`** — *`@native`*

Return `true` if the bit at position `bit` is 1.

Panic if `bit` is out of range [0, size).

<a id="bitset.flip"></a>

**`flip(bit int)`** — *`@native`*

Flip the bit at position `bit`.

Panic if `bit` is out of range [0, size).

<a id="bitset.count"></a>

**`count() int`** — *`@native`*

Return the number of bits set to 1.

<a id="bitset.size"></a>

**`size() int`** — *`@native`*

Return the total number of bits in the BitSet.

<a id="bitset.dump"></a>

**`dump() str`** — *`@native`*

Dump all bits in formatted groups (8 bits per group, 4 groups per line).

<a id="bitset.__bitand__"></a>

**`__bitand__(other BitSet) BitSet`** — *`@native`*

Return a new BitSet that is the bitwise AND of `self` and `other`.

Panic if `self` and `other` have different sizes.

<a id="bitset.__bitor__"></a>

**`__bitor__(other BitSet) BitSet`** — *`@native`*

Return a new BitSet that is the bitwise OR of `self` and `other`.

Panic if `self` and `other` have different sizes.

<a id="bitset.__bitxor__"></a>

**`__bitxor__(other BitSet) BitSet`** — *`@native`*

Return a new BitSet that is the bitwise XOR of `self` and `other`.

Panic if `self` and `other` have different sizes.

<a id="bitset.__bitnot__"></a>

**`__bitnot__() BitSet`** — *`@native`*

Return a new BitSet with all bits inverted.

<a id="bitset.to_bytes"></a>

**`to_bytes() bytes`** — *`@native`*

Return the byte representation of this BitSet.

<a id="bitset.from_bytes"></a>

**`static from_bytes(data bytes) BitSet`** — *`@native`*

Create a BitSet from bytes data.

The resulting BitSet has `data.len() * 8` bits.

<a id="deque"></a>

### `Deque`

```kl
pub class Deque[T] : MutableSequence[T] {
    pub func __init__()
    pub func iter(step = 1) Iterator[T]
    pub func len() int
    pub func empty() bool
    pub func __contains__(v T) bool
    pub func __getitem__(index int) T
    pub func __setitem__(index int, v T)
    pub func index(value T, start = 0, end = -1) int
    pub func count(value T, start = 0, end = -1) int
    pub func append(v T)
    pub func extend(items Iterable[T])
    pub func insert(index int, v T)
    pub func remove(v T)
    pub func pop(index = -1) T
    pub func clear()
    pub func reversed(step = 1) Iterator[T]
    pub func push_front(v T)
    pub func push_back(v T)
    pub func pop_front() T
    pub func pop_back() T
    pub func peek_front() T
    pub func peek_back() T
    pub func fmt(f Formatter)
}
```
**Bases:** `MutableSequence[T]`


A Deque (double-ended queue) is a generalization of a queue that supports
efficient insertion and removal at both ends.

It implements `MutableSequence`, providing indexed access, insertion,
and removal. Deque is typically backed by a circular buffer or linked
structure, offering O(1) amortized push/pop at both ends.

---

<a id="deque.__init__"></a>

**`__init__()`** — *`@native`*

Create an empty deque.

<a id="deque.iter"></a>

**`iter(step = 1) Iterator[T]`**

Return an iterator over the elements, advancing by `step` slots.

The cursor is an index rather than a buffer pointer, so growing the deque
never invalidates it; the deque must not be structurally modified
during traversal.

<a id="deque.len"></a>

**`len() int`** — *`@native`*

Return the number of elements.

<a id="deque.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the deque is empty, false otherwise.

<a id="deque.__contains__"></a>

**`__contains__(v T) bool`** — *`@native`*

Return true if the deque contains `v`.

<a id="deque.__getitem__"></a>

**`__getitem__(index int) T`** — *`@native`*

Return the element at `index`. Panic if out of bounds.

<a id="deque.__setitem__"></a>

**`__setitem__(index int, v T)`** — *`@native`*

Set the element at `index` to `v`. Panic if out of bounds.

<a id="deque.index"></a>

**`index(value T, start = 0, end = -1) int`** — *`@native`*

Return the index of the first occurrence of `value` in `[start, end)`,
or -1 if it is not present.

<a id="deque.count"></a>

**`count(value T, start = 0, end = -1) int`** — *`@native`*

Return the number of occurrences of `value` in `[start, end)`.

<a id="deque.append"></a>

**`append(v T)`** — *`@native`*

Append `v` to the end of the deque.

<a id="deque.extend"></a>

**`extend(items Iterable[T])`** — *`@native`*

Append all items from `items` to the end of the deque.

<a id="deque.insert"></a>

**`insert(index int, v T)`** — *`@native`*

Insert `v` at `index`. Panic if out of bounds.

<a id="deque.remove"></a>

**`remove(v T)`** — *`@native`*

Remove the first occurrence of `v`. Panic if it is not present.

<a id="deque.pop"></a>

**`pop(index = -1) T`** — *`@native`*

Remove and return the element at `index`.
If no index is specified, removes and returns the last element.

<a id="deque.clear"></a>

**`clear()`** — *`@native`*

Remove all elements from the deque, leaving it empty.

<a id="deque.reversed"></a>

**`reversed(step = 1) Iterator[T]`**

Return a reverse iterator over the elements, advancing by `step` slots.

<a id="deque.push_front"></a>

**`push_front(v T)`** — *`@native`*

Prepend `v` to the front of the deque. O(1) amortized.

<a id="deque.push_back"></a>

**`push_back(v T)`** — *`@native`*

Append `v` to the end of the deque. O(1) amortized.

<a id="deque.pop_front"></a>

**`pop_front() T`** — *`@native`*

Remove and return the first element. Panic if empty. O(1) amortized.

<a id="deque.pop_back"></a>

**`pop_back() T`** — *`@native`*

Remove and return the last element. Panic if empty. O(1) amortized.

<a id="deque.peek_front"></a>

**`peek_front() T`** — *`@native`*

Return the first element without removing it. Panic if empty. O(1).

<a id="deque.peek_back"></a>

**`peek_back() T`** — *`@native`*

Return the last element without removing it. Panic if empty. O(1).

<a id="deque.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the deque for display.

<a id="dequeiter"></a>

### `DequeIter`

```kl
class DequeIter[T] : Iterator[T] {
    func __init__(src Deque[T], step int)
    func has_next() bool
    func next_strict() T
    func next() T?
}
```
**Bases:** `Iterator[T]`


A forward iterator over a `Deque`, advancing by `step` slots each turn.

A push may reallocate the backing buffer, so the cursor is an index rather
than a pointer into it. The element sequence itself must not change during
traversal; doing so raises `Deque modified during iteration`.

---

<a id="dequeiter.__init__"></a>

**`__init__(src Deque[T], step int)`** — *`@native`*

Initialize a forward iterator over `src`, advancing by `step` slots.
`step` must be positive.

<a id="dequeiter.has_next"></a>

**`has_next() bool`** — *`@native`*

Return true if another element is available.

<a id="dequeiter.next_strict"></a>

**`next_strict() T`** — *`@native`*

Return the next element. Panic if the iterator is exhausted.

<a id="dequeiter.next"></a>

**`next() T?`** — *`@native`*

Return the next element, or `nil` if the iterator is exhausted.

<a id="dequereviter"></a>

### `DequeRevIter`

```kl
class DequeRevIter[T] : Iterator[T] {
    func __init__(src Deque[T], step int)
    func has_next() bool
    func next_strict() T
    func next() T?
}
```
**Bases:** `Iterator[T]`


A backward iterator over a `Deque`, advancing by `step` slots each turn.

It yields the last element first, walking towards the front.

---

<a id="dequereviter.__init__"></a>

**`__init__(src Deque[T], step int)`** — *`@native`*

Initialize a backward iterator over `src`, advancing by `step` slots.
`step` must be positive.

<a id="dequereviter.has_next"></a>

**`has_next() bool`** — *`@native`*

Return true if another element is available.

<a id="dequereviter.next_strict"></a>

**`next_strict() T`** — *`@native`*

Return the next element. Panic if the iterator is exhausted.

<a id="dequereviter.next"></a>

**`next() T?`** — *`@native`*

Return the next element, or `nil` if the iterator is exhausted.

<a id="frozenbytes"></a>

### `FrozenBytes`

```kl
pub class FrozenBytes : Sequence[uint8] {
    pub func __init__(data Iterable[uint8])
    pub func iter(step = 1) Iterator[uint8]
    pub func len() int
    pub func empty() bool
    pub func __contains__(v uint8) bool
    pub func __getitem__(index int) uint8
    pub func reversed(step = 1) Iterator[uint8]
    pub func fmt(f Formatter)
}
```
**Bases:** `Sequence[uint8]`


FrozenBytes is an immutable, fixed-size sequence of bytes.

It implements `Sequence[uint8]` (read-only), providing random access
and iteration. Once created, bytes cannot be modified.
FrozenBytes is hashable and can be used as a dict key or set element.

---

<a id="frozenbytes.__init__"></a>

**`__init__(data Iterable[uint8])`** — *`@native`*

Create a FrozenBytes from a byte array or iterable.

<a id="frozenbytes.iter"></a>

**`iter(step = 1) Iterator[uint8]`** — *`@native`*

Return an iterator over the bytes.

<a id="frozenbytes.len"></a>

**`len() int`** — *`@native`*

Return the number of bytes.

<a id="frozenbytes.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the byte sequence is empty, false otherwise.

<a id="frozenbytes.__contains__"></a>

**`__contains__(v uint8) bool`** — *`@native`*

Return true if the byte sequence contains `v`.

<a id="frozenbytes.__getitem__"></a>

**`__getitem__(index int) uint8`** — *`@native`*

Return the byte at `index`. Panic if out of bounds.

<a id="frozenbytes.reversed"></a>

**`reversed(step = 1) Iterator[uint8]`** — *`@native`*

Return a reverse iterator over the bytes.

<a id="frozenbytes.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the byte sequence for display.

<a id="frozendict"></a>

### `FrozenDict`

```kl
pub class FrozenDict[K, V] : Map[K, V] {
    pub func __init__(items Iterable[(K, V)])
    pub func iter(step = 1) Iterator[(K, V)]
    pub func len() int
    pub func empty() bool
    pub func __contains__(key K) bool
    pub func __getitem__(key K) V
    pub func keys() Iterable[K]
    pub func values() Iterable[V]
    pub func get(key K) V?
    pub func get_or(key K, default_value V) V
    pub func fmt(f Formatter)
}
```
**Bases:** `Map[K, V]`


A FrozenDict is an immutable, unordered hash map that maps unique keys
to values.

It implements `Map` (read-only), providing key lookup and iteration
over key-value pairs. Once created, mappings cannot be added or removed.
FrozenDict is hashable and can be used as a dict key or set element.

---

<a id="frozendict.__init__"></a>

**`__init__(items Iterable[(K, V)])`** — *`@native`*

Create a FrozenDict from an iterable of (key, value) pairs.

<a id="frozendict.iter"></a>

**`iter(step = 1) Iterator[(K, V)]`** — *`@native`*

Return an iterator over the (key, value) pairs.

<a id="frozendict.len"></a>

**`len() int`** — *`@native`*

Return the number of key-value pairs.

<a id="frozendict.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the dict is empty, false otherwise.

<a id="frozendict.__contains__"></a>

**`__contains__(key K) bool`** — *`@native`*

Return true if the dict contains a mapping for `key`.

<a id="frozendict.__getitem__"></a>

**`__getitem__(key K) V`** — *`@native`*

Return the value mapped to `key`. Panic if `key` is not present.

<a id="frozendict.keys"></a>

**`keys() Iterable[K]`** — *`@native`*

Return an iterable over all keys in the dict.

<a id="frozendict.values"></a>

**`values() Iterable[V]`** — *`@native`*

Return an iterable over all values in the dict.

<a id="frozendict.get"></a>

**`get(key K) V?`** — *`@native`*

Return the value for `key` if present, else `nil`.

<a id="frozendict.get_or"></a>

**`get_or(key K, default_value V) V`** — *`@native`*

Return the value for `key` if present, else `default_value`.

<a id="frozendict.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the dict for display.

<a id="frozenlist"></a>

### `FrozenList`

```kl
pub class FrozenList[T] : Sequence[T] {
    pub func __init__(items Iterable[T])
    pub func iter(step = 1) Iterator[T]
    pub func len() int
    pub func empty() bool
    pub func __contains__(v T) bool
    pub func __getitem__(index int) T
    pub func reversed(step = 1) Iterator[T]
    pub func fmt(f Formatter)
}
```
**Bases:** `Sequence[T]`


A FrozenList is an immutable, ordered sequence of elements.

It implements `Sequence` (read-only), providing random access and iteration.
Once created, elements cannot be added, removed, or modified.
FrozenList is hashable (if all elements are hashable) and can be used
as a dict key or set element.

---

<a id="frozenlist.__init__"></a>

**`__init__(items Iterable[T])`** — *`@native`*

Create a FrozenList from an iterable of elements.

<a id="frozenlist.iter"></a>

**`iter(step = 1) Iterator[T]`** — *`@native`*

Return an iterator over the elements.

<a id="frozenlist.len"></a>

**`len() int`** — *`@native`*

Return the number of elements.

<a id="frozenlist.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the list is empty, false otherwise.

<a id="frozenlist.__contains__"></a>

**`__contains__(v T) bool`** — *`@native`*

Return true if the list contains `v`.

<a id="frozenlist.__getitem__"></a>

**`__getitem__(index int) T`** — *`@native`*

Return the element at `index`. Panic if out of bounds.

<a id="frozenlist.reversed"></a>

**`reversed(step = 1) Iterator[T]`** — *`@native`*

Return a reverse iterator over the elements.

<a id="frozenlist.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the list for display.

<a id="frozenset"></a>

### `FrozenSet`

```kl
pub class FrozenSet[T] : Set[T] {
    pub func __init__(items Iterable[T])
    pub func iter(step = 1) Iterator[T]
    pub func len() int
    pub func empty() bool
    pub func __contains__(v T) bool
    pub func is_subset(other Iterable[T]) bool
    pub func is_superset(other Iterable[T]) bool
    pub func union(other Iterable[T]) FrozenSet[T]
    pub func intersect(other Iterable[T]) FrozenSet[T]
    pub func diff(other Iterable[T]) FrozenSet[T]
    pub func fmt(f Formatter)
}
```
**Bases:** `Set[T]`


A FrozenSet is an immutable, unordered collection of unique elements.

It implements `Set` (read-only), providing membership tests and set algebra.
Once created, elements cannot be added or removed.
FrozenSet is hashable and can be used as a dict key or set element.

---

<a id="frozenset.__init__"></a>

**`__init__(items Iterable[T])`** — *`@native`*

Create a FrozenSet from an iterable of elements.

<a id="frozenset.iter"></a>

**`iter(step = 1) Iterator[T]`** — *`@native`*

Return an iterator over the elements.

<a id="frozenset.len"></a>

**`len() int`** — *`@native`*

Return the number of elements.

<a id="frozenset.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the set is empty, false otherwise.

<a id="frozenset.__contains__"></a>

**`__contains__(v T) bool`** — *`@native`*

Return true if the set contains `v`.

<a id="frozenset.is_subset"></a>

**`is_subset(other Iterable[T]) bool`** — *`@native`*

Return true if every element in the set is in `other`.

<a id="frozenset.is_superset"></a>

**`is_superset(other Iterable[T]) bool`** — *`@native`*

Return true if every element in `other` is in the set.

<a id="frozenset.union"></a>

**`union(other Iterable[T]) FrozenSet[T]`** — *`@native`*

Return a new FrozenSet with elements from the set and `other`.

<a id="frozenset.intersect"></a>

**`intersect(other Iterable[T]) FrozenSet[T]`** — *`@native`*

Return a new FrozenSet with elements common to the set and `other`.

<a id="frozenset.diff"></a>

**`diff(other Iterable[T]) FrozenSet[T]`** — *`@native`*

Return a new FrozenSet with elements in the set that are not in `other`.

<a id="frozenset.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the set for display.

<a id="heap"></a>

### `Heap`

```kl
pub class Heap[T : Comparable] {
    pub func __init__()
    pub func push(v T)
    pub func pop() T
    pub func peek() T
    pub func len() int
    pub func empty() bool
    pub func clear()
    pub func fmt(f Formatter)
}
```

A Heap is a min-heap priority queue backed by a binary heap.

Elements are ordered by their `Comparable` ordering; the smallest
element is always at the front. Heap does not implement `Sequence`
because it does not support indexed access.

---

<a id="heap.__init__"></a>

**`__init__()`** — *`@native`*

Create an empty heap.

<a id="heap.push"></a>

**`push(v T)`** — *`@native`*

Push `v` into the heap. O(log n).

<a id="heap.pop"></a>

**`pop() T`** — *`@native`*

Remove and return the smallest element. Panic if empty. O(log n).

<a id="heap.peek"></a>

**`peek() T`** — *`@native`*

Return the smallest element without removing it. Panic if empty. O(1).

<a id="heap.len"></a>

**`len() int`** — *`@native`*

Return the number of elements.

<a id="heap.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the heap is empty, false otherwise.

<a id="heap.clear"></a>

**`clear()`** — *`@native`*

Remove all elements from the heap, leaving it empty.

<a id="heap.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the heap for display.

<a id="linkedlist"></a>

### `LinkedList`

```kl
pub class LinkedList[T] : MutableSequence[T] {
    pub func __init__()
    pub func iter(step = 1) Iterator[T]
    pub func reversed(step = 1) Iterator[T]
    pub func len() int
    pub func empty() bool
    pub func __contains__(v T) bool
    pub func __getitem__(index int) T
    pub func __setitem__(index int, v T)
    pub func index(value T, start = 0, end = -1) int
    pub func count(value T, start = 0, end = -1) int
    pub func append(v T)
    pub func extend(items Iterable[T])
    pub func insert(index int, v T)
    pub func remove(value T)
    pub func pop(index = -1) T
    pub func clear()
    pub func push_front(v T)
    pub func push_back(v T)
    pub func pop_front() T
    pub func pop_back() T
    pub func peek_front() T
    pub func peek_back() T
    pub func fmt(f Formatter)
}
```
**Bases:** `MutableSequence[T]`


A LinkedList is a doubly-linked list that supports efficient insertion
and deletion at both ends.

It implements `MutableSequence`, providing indexed access, insertion,
and removal. Unlike `list`, LinkedList offers O(1) push/pop at the front,
but O(n) random access by index.

Examples

```kl
let values = LinkedList[int]()
values.push_back(2)
values.push_front(1)
values.append(3)
print(values[0])
print(values.pop_front())
```

---

<a id="linkedlist.__init__"></a>

**`__init__()`** — *`@native`*

Create an empty linked list.

<a id="linkedlist.iter"></a>

**`iter(step = 1) Iterator[T]`**

Return an iterator over the elements, advancing by `step` nodes.

<a id="linkedlist.reversed"></a>

**`reversed(step = 1) Iterator[T]`**

Return a reverse iterator over the elements, advancing by `step` nodes.

<a id="linkedlist.len"></a>

**`len() int`** — *`@native`*

Return the number of elements.

<a id="linkedlist.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the list is empty, false otherwise.

<a id="linkedlist.__contains__"></a>

**`__contains__(v T) bool`** — *`@native`*

Return true if the list contains `v`.

<a id="linkedlist.__getitem__"></a>

**`__getitem__(index int) T`** — *`@native`*

Return the element at `index`. Panic if out of bounds.

<a id="linkedlist.__setitem__"></a>

**`__setitem__(index int, v T)`** — *`@native`*

Set the element at `index` to `v`. Panic if out of bounds.

<a id="linkedlist.index"></a>

**`index(value T, start = 0, end = -1) int`** — *`@native`*

Return the index of the first occurrence of `value` in `[start, end)`,
or -1 if it is not present.

<a id="linkedlist.count"></a>

**`count(value T, start = 0, end = -1) int`** — *`@native`*

Return the number of occurrences of `value` in `[start, end)`.

<a id="linkedlist.append"></a>

**`append(v T)`** — *`@native`*

Append `v` to the end of the list.

<a id="linkedlist.extend"></a>

**`extend(items Iterable[T])`** — *`@native`*

Append all items from `items` to the end of the list.

<a id="linkedlist.insert"></a>

**`insert(index int, v T)`** — *`@native`*

Insert `v` at `index`. `index` may be equal to `len`, which appends.
Panic if out of bounds.

<a id="linkedlist.remove"></a>

**`remove(value T)`** — *`@native`*

Remove the first occurrence of `value`. Panic if it is not present.

<a id="linkedlist.pop"></a>

**`pop(index = -1) T`** — *`@native`*

Remove and return the element at `index`.
If no index is specified, removes and returns the last element.

<a id="linkedlist.clear"></a>

**`clear()`** — *`@native`*

Remove all elements from the list, leaving it empty.

<a id="linkedlist.push_front"></a>

**`push_front(v T)`** — *`@native`*

Prepend `v` to the front of the list. O(1).

<a id="linkedlist.push_back"></a>

**`push_back(v T)`** — *`@native`*

Append `v` to the end of the list. O(1).

<a id="linkedlist.pop_front"></a>

**`pop_front() T`** — *`@native`*

Remove and return the first element. Panic if empty. O(1).

<a id="linkedlist.pop_back"></a>

**`pop_back() T`** — *`@native`*

Remove and return the last element. Panic if empty. O(1).

<a id="linkedlist.peek_front"></a>

**`peek_front() T`** — *`@native`*

Return the first element without removing it. Panic if empty. O(1).

<a id="linkedlist.peek_back"></a>

**`peek_back() T`** — *`@native`*

Return the last element without removing it. Panic if empty. O(1).

<a id="linkedlist.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the list for display. Like `Stack`, lists longer than 10
elements only show the first 4 and the last 4, separated by `...`.

<a id="linkedlistiter"></a>

### `LinkedListIter`

```kl
class LinkedListIter[T] : Iterator[T] {
    func __init__(src LinkedList[T], step int)
    func has_next() bool
    func next_strict() T
    func next() T?
}
```
**Bases:** `Iterator[T]`


A forward iterator over a `LinkedList`, advancing by `step` nodes each turn.

The cursor holds a node, so a full traversal is O(n) instead of O(n^2).
The source must not be structurally modified during traversal; doing so
raises `LinkedList modified during iteration`.

---

<a id="linkedlistiter.__init__"></a>

**`__init__(src LinkedList[T], step int)`** — *`@native`*

Initialize a forward iterator over `src`, advancing by `step` nodes.
`step` must be positive.

<a id="linkedlistiter.has_next"></a>

**`has_next() bool`** — *`@native`*

Return true if another element is available.

<a id="linkedlistiter.next_strict"></a>

**`next_strict() T`** — *`@native`*

Return the next element. Panic if the iterator is exhausted.

<a id="linkedlistiter.next"></a>

**`next() T?`** — *`@native`*

Return the next element, or `nil` if the iterator is exhausted.

<a id="linkedlistreviter"></a>

### `LinkedListRevIter`

```kl
class LinkedListRevIter[T] : Iterator[T] {
    func __init__(src LinkedList[T], step int)
    func has_next() bool
    func next_strict() T
    func next() T?
}
```
**Bases:** `Iterator[T]`


A backward iterator over a `LinkedList`, advancing by `step` nodes each turn.

It yields the last element first, walking towards the front.

---

<a id="linkedlistreviter.__init__"></a>

**`__init__(src LinkedList[T], step int)`** — *`@native`*

Initialize a backward iterator over `src`, advancing by `step` nodes.
`step` must be positive.

<a id="linkedlistreviter.has_next"></a>

**`has_next() bool`** — *`@native`*

Return true if another element is available.

<a id="linkedlistreviter.next_strict"></a>

**`next_strict() T`** — *`@native`*

Return the next element. Panic if the iterator is exhausted.

<a id="linkedlistreviter.next"></a>

**`next() T?`** — *`@native`*

Return the next element, or `nil` if the iterator is exhausted.

<a id="queue"></a>

### `Queue`

```kl
pub class Queue[T] {
    pub func __init__()
    pub func len() int
    pub func empty() bool
    pub func push(v T)
    pub func pop() T
    pub func peek() T
    pub func clear()
    pub func fmt(f Formatter)
}
```

A Queue is a first-in, first-out (FIFO) collection.

Elements are added to the back and removed from the front.
Queue does not implement `Sequence` because it does not support
indexed access.

---

<a id="queue.__init__"></a>

**`__init__()`** — *`@native`*

Create an empty queue.

<a id="queue.len"></a>

**`len() int`** — *`@native`*

Return the number of elements.

<a id="queue.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the queue is empty, false otherwise.

<a id="queue.push"></a>

**`push(v T)`** — *`@native`*

Add `v` to the back of the queue.

<a id="queue.pop"></a>

**`pop() T`** — *`@native`*

Remove and return the front element. Panic if empty.

<a id="queue.peek"></a>

**`peek() T`** — *`@native`*

Return the front element without removing it. Panic if empty.

<a id="queue.clear"></a>

**`clear()`** — *`@native`*

Remove all elements from the queue, leaving it empty.

<a id="queue.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the queue for display.

<a id="sorteddict"></a>

### `SortedDict`

```kl
pub class SortedDict[K : Comparable, V] : MutableMap[K, V] {
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
    pub func reversed(step = 1) Iterator[(K, V)]
    pub func fmt(f Formatter)
}
```
**Bases:** `MutableMap[K, V]`


A SortedDict is a mutable, sorted hash map that maps unique keys
to values, ordered by key.

It implements `MutableMap`, providing key lookup, insertion, removal,
and iteration over key-value pairs. Keys are ordered by their
`Comparable` ordering; iteration follows that order.

---

<a id="sorteddict.__init__"></a>

**`__init__()`** — *`@native`*

Create an empty sorted dict.

<a id="sorteddict.iter"></a>

**`iter(step = 1) Iterator[(K, V)]`** — *`@native`*

Return an iterator over the (key, value) pairs in key order.

<a id="sorteddict.len"></a>

**`len() int`** — *`@native`*

Return the number of key-value pairs.

<a id="sorteddict.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the dict is empty, false otherwise.

<a id="sorteddict.__contains__"></a>

**`__contains__(key K) bool`** — *`@native`*

Return true if the dict contains a mapping for `key`.

<a id="sorteddict.__getitem__"></a>

**`__getitem__(key K) V`** — *`@native`*

Return the value mapped to `key`. Panic if `key` is not present.

<a id="sorteddict.__setitem__"></a>

**`__setitem__(key K, value V)`** — *`@native`*

Associate `value` with `key`.

<a id="sorteddict.keys"></a>

**`keys() Iterable[K]`** — *`@native`*

Return an iterable over all keys in the dict, in key order.

<a id="sorteddict.values"></a>

**`values() Iterable[V]`** — *`@native`*

Return an iterable over all values in the dict, in key order.

<a id="sorteddict.items"></a>

**`items() Iterable[(K, V)]`** — *`@native`*

Return an iterable over the key-value pairs in the map.

<a id="sorteddict.clear"></a>

**`clear()`** — *`@native`*

Remove all mappings from the dict, leaving it empty.

<a id="sorteddict.remove"></a>

**`remove(key K) V`** — *`@native`*

Remove the mapping for `key` and return its value.
Panic if `key` is not present.

<a id="sorteddict.remove_or"></a>

**`remove_or(key K, default_value V) V`** — *`@native`*

Remove the mapping for `key` and return its value.
If the key is not found, return `default_value`.

<a id="sorteddict.get"></a>

**`get(key K) V?`** — *`@native`*

Return the value for `key` if present, else `nil`.

<a id="sorteddict.get_or"></a>

**`get_or(key K, default_value V) V`** — *`@native`*

Return the value for `key` if present, else `default_value`.

<a id="sorteddict.update"></a>

**`update(src Iterable[(K, V)])`** — *`@native`*

Merge all (key, value) pairs from `src` into the dict,
replacing the values of existing keys.

<a id="sorteddict.setdefault"></a>

**`setdefault(key K, default_value V) V`** — *`@native`*

Return the value for `key` if it is in the dict.
Otherwise insert `key` with `default_value` and return `default_value`.

<a id="sorteddict.reversed"></a>

**`reversed(step = 1) Iterator[(K, V)]`** — *`@native`*

Return a reverse iterator over the (key, value) pairs in reverse key order.

<a id="sorteddict.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the dict for display.

<a id="sortedset"></a>

### `SortedSet`

```kl
pub class SortedSet[T : Comparable] : MutableSet[T] {
    pub func __init__()
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
    pub func union(other Iterable[T]) SortedSet[T]
    pub func intersect(other Iterable[T]) SortedSet[T]
    pub func diff(other Iterable[T]) SortedSet[T]
    pub func sym_diff(other Iterable[T]) SortedSet[T]
    pub func __or__(other Iterable[T]) SortedSet[T]
    pub func __and__(other Iterable[T]) SortedSet[T]
    pub func __sub__(other Iterable[T]) SortedSet[T]
    pub func reversed(step = 1) Iterator[T]
    pub func to_list() list[T]
}
```
**Bases:** `MutableSet[T]`


A SortedSet is a mutable, sorted collection of unique elements,
backed by a balanced tree.

It implements `MutableSet`, providing membership tests, insertion, removal,
and set algebra. Elements are ordered by their `Comparable` ordering.
Iteration and `to_list()` follow that order; `reversed()` traverses it
in reverse.

---

<a id="sortedset.__init__"></a>

**`__init__()`** — *`@native`*

Create an empty tree set.

<a id="sortedset.iter"></a>

**`iter(step = 1) Iterator[T]`** — *`@native`*

Return an iterator over the elements in sorted order.

<a id="sortedset.len"></a>

**`len() int`** — *`@native`*

Return the number of elements.

<a id="sortedset.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the set is empty, false otherwise.

<a id="sortedset.__contains__"></a>

**`__contains__(v T) bool`** — *`@native`*

Return true if the set contains `v`.

<a id="sortedset.add"></a>

**`add(v T) bool`** — *`@native`*

Add an element to the set.
Return true if the element was not already present.

<a id="sortedset.update"></a>

**`update(items Iterable[T])`** — *`@native`*

Add all elements from `items` to the set.
Unlike `add` which inserts a single element, `update` inserts
multiple elements from any iterable source in one call.

<a id="sortedset.remove"></a>

**`remove(v T) bool`** — *`@native`*

Remove an element from the set.
Return true if the element was present.

<a id="sortedset.clear"></a>

**`clear()`** — *`@native`*

Remove all elements from the set, leaving it empty.

<a id="sortedset.is_subset"></a>

**`is_subset(other Iterable[T]) bool`** — *`@native`*

Return true if every element in the set is in `other`.

<a id="sortedset.is_superset"></a>

**`is_superset(other Iterable[T]) bool`** — *`@native`*

Return true if every element in `other` is in the set.

<a id="sortedset.union"></a>

**`union(other Iterable[T]) SortedSet[T]`** — *`@native`*

Return a new sorted set with elements from the set and `other`.

<a id="sortedset.intersect"></a>

**`intersect(other Iterable[T]) SortedSet[T]`** — *`@native`*

Return a new sorted set with elements common to the set and `other`.

<a id="sortedset.diff"></a>

**`diff(other Iterable[T]) SortedSet[T]`** — *`@native`*

Return a new sorted set with elements in the set that are not in `other`.

<a id="sortedset.sym_diff"></a>

**`sym_diff(other Iterable[T]) SortedSet[T]`** — *`@native`*

Return a new sorted set with elements in the set or `other`, but not both.

<a id="sortedset.__or__"></a>

**`__or__(other Iterable[T]) SortedSet[T]`** — *`@native`*

Return a new sorted set with elements from the set and `other` (`self | other`).

<a id="sortedset.__and__"></a>

**`__and__(other Iterable[T]) SortedSet[T]`** — *`@native`*

Return a new sorted set with elements common to the set and `other` (`self & other`).

<a id="sortedset.__sub__"></a>

**`__sub__(other Iterable[T]) SortedSet[T]`** — *`@native`*

Return a new sorted set with elements in the set that are not in `other` (`self - other`).

<a id="sortedset.reversed"></a>

**`reversed(step = 1) Iterator[T]`** — *`@native`*

Return a reverse iterator over the elements in reverse sorted order.

<a id="sortedset.to_list"></a>

**`to_list() list[T]`** — *`@native`*

Return a list of all elements in sorted order.

<a id="stack"></a>

### `Stack`

```kl
pub class Stack[T] {
    pub func __init__()
    pub func len() int
    pub func empty() bool
    pub func push(v T)
    pub func pop() T
    pub func peek() T
    pub func clear()
    pub func fmt(f Formatter)
}
```

A Stack is a last-in, first-out (LIFO) collection.

Elements are added and removed from the top.
Stack does not implement `Sequence` because it does not support
indexed access.

---

<a id="stack.__init__"></a>

**`__init__()`** — *`@native`*

Create an empty stack.

<a id="stack.len"></a>

**`len() int`** — *`@native`*

Return the number of elements.

<a id="stack.empty"></a>

**`empty() bool`** — *`@native`*

Return true if the stack is empty, false otherwise.

<a id="stack.push"></a>

**`push(v T)`** — *`@native`*

Push `v` onto the top of the stack.

<a id="stack.pop"></a>

**`pop() T`** — *`@native`*

Remove and return the top element. Panic if empty.

<a id="stack.peek"></a>

**`peek() T`** — *`@native`*

Return the top element without removing it. Panic if empty.

<a id="stack.clear"></a>

**`clear()`** — *`@native`*

Remove all elements from the stack, leaving it empty.

<a id="stack.fmt"></a>

**`fmt(f Formatter)`** — *`@native`*

Format the stack for display.
