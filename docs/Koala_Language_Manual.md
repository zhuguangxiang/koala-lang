
# Koala Language Manual

Koala is a small, safe, efficient and modern programming language targeting application development,
high-performance computing and AI operator development. It combines the fluency of Python with
static, compile-time-enforced typing, and its whole design traces back to three supreme principles:

1. **Python inside a static language** — the semantic intuition follows Python (insertion-ordered
   dicts, dunder protocols, convenience methods), while static strong typing guards everything at
   compile time.
2. **Nothing hidden** — zero dark corners for the user: builtin types are declared in plain standard
   library sources, compiler behavior is visible in the generated tables, and every intermediate
   stage of the compiler can be dumped and inspected.
3. **Simplicity is mandatory** — simplicity constrains both the API surface and the compiler
   implementation; no hidden mechanism is ever introduced for superficial uniformity.

From these principles follow a number of advanced features that most languages do not offer:

- **Zero boxing with a universal `any` and generics.** Values enter the universal `any` type and
  generic containers without any boxing or identity change — a combination usually considered
  mutually exclusive. There is no autoboxing, no `Integer`-cache traps, no struct boxing.
- **Semi-automatic memory management.** Deterministic release is the main channel — explicit `free`
  plus `free` inserted by the compiler where lifetimes are obvious — with a collector kept as a
  safety valve. Deterministic *and* low-ceremony: Rust's ownership rituals and Go's stop-the-world
  nondeterminism are both avoided.
- **FFI without `unsafe`.** A `link` declaration plus `@native` functions is all there is; the
  compiler bridges the call, and objects allocated on the C side are managed by the same memory
  machinery as Koala objects. No handle tables, no manual reference counting, no marshaling.
- **Static interface dispatch, no lookup.** Interface tables (intf-tables) are built at compile
  time, so a trait call is one O(1) slot call and a class method call is a plain direct call.
  Partial trait implementation is allowed: unimplemented methods are filled with a placeholder and
  panic only if actually called.
- **Automatic root contracts.** Every class automatically conforms to `Equatable` / `Hashable` /
  `Printable` with identity defaults, while the root type `any` stays empty. This dissolves Java's
  `equals(Object)` problems — no type erasure, no `instanceof` boilerplate, no binding to a god
  class.
- **Operators as syntax hooks.** Implementing `__add__` grants `a + b` with no declaration
  ceremony, and operator dunders can never be called by name — the syntax is the only entry point.
  The traits `Arithmetic[T]` / `BitwiseOperators[T]` exist purely as generic constraints.
- **Errors are eliminated by design.** Expected failures are return values (C style); unexpected
  failures are panics — unrecoverable and uncatchable, a signal that the code must be fixed. No
  exceptions, no try/catch, no defer.
- **Null safety without ceremony.** There is no null in the language; optionality is expressed with
  `T?`, narrowed automatically in branches, and unwrapped with `if let` / `while let` at zero
  runtime cost.
- **Classes are always final.** No class inheritance exists; traits serve as interfaces, and trait
  inheritance is composition in essence.
- **Safe integer overflow.** Overflow is defined behavior under trap/wrap policies — never
  undefined behavior.

## Contents

- [Hello World](#hello-world)
- [Basics](#basics)
  - [Constants and Variables](#constants-and-variables)
  - [Comments and Doc Comments](#comments-and-doc-comments)
  - [Integers](#integers)
  - [Floating-Point Numbers](#floating-point-numbers)
  - [Integer Conversions](#integer-conversions)
  - [Integer-Floating-Point Conversions](#integer-floating-point-conversions)
  - [Booleans](#booleans)
  - [Tuples and Variadic Types](#tuples-and-variadic-types)
  - [Null Safety](#null-safety)
  - [Union Types](#union-types)
- [Strings and Characters](#strings-and-characters)
- [Containers](#containers)
- [Branching and Loops](#branching-and-loops)
- [Functions and Closures](#functions-and-closures)
- [Classes and Interfaces](#classes-and-interfaces)
- [Generics](#generics)
- [Operator Overloading](#operator-overloading)
- [Error Handling](#error-handling)
- [Memory Management](#memory-management)
- [Package Management](#package-management)
- [Native Extensions](#native-extensions)

## Hello World

A Koala program may define a `main` function. The classic hello world looks like this:

```kl
func main() {
    print('hello, world')
}
```

Compile a source file with the command below; it produces a bytecode file with the `.klc` suffix:

```bash
koala -c hello.kl
```

Run a `.kl` source file directly:

```bash
koala hello.kl
```

Or run a compiled `.klc` bytecode file:

```bash
koala hello.klc
```

## Basics

Koala provides a rich set of basic data types — integers, floating-point numbers, booleans and
strings — as well as container types such as list, tuple, dict, set and range.

### Constants and Variables

Variables are declared with `let` (immutable binding) or `var` (mutable binding):

```kl
let name = "koala"      // type inferred as str, cannot be reassigned
var count = 0           // type inferred as int, can be reassigned
count = count + 1
```

An explicit type annotation may be given; the initializer must match it:

```kl
var total int = 0
let msg str? = null
```

Local variables inside functions follow the same rules:

```kl
func foo(n int) {
    var i = 0
    while i < n {
        print(i)
        i += 1
    }
}
```

Every variable must be definitely assigned before it is read; the compiler rejects uses of
uninitialized values. There is no uninitialized-state undefined behavior.

Type inference works wherever an initializer or a context provides enough information: parameter
types may be omitted when a default value pins them down, and generic type arguments are inferred
from the call site (see [Generics](#generics)).

### Comments and Doc Comments

Line comments start with `//`, block comments are written `/* ... */`:

```kl
// a line comment
/* a block comment,
   spanning several lines */
```

Documentation comments use the Rust style `///`. They are bound to the declaration that immediately
follows them (a blank line breaks the binding) and are extracted by the documentation tool
`tools/kl-doc.py`:

```kl
/// Return the larger of two values.
///
/// Example:
///     print(max(3, 4))
func max(a int, b int) int {
    if a >= b { return a }
    return b
}
```

The first line is a summary written in the third person and verb-first. Label lines such as
`Example:`, `Note:`, `Precondition:`, `Panics:` and `See:` receive special rendering.

### Integers

The integer family is:

| Type | Description |
|------|-------------|
| `int` | machine-word signed integer (alias of `int64`) |
| `int8`, `int16`, `int32`, `int64` | signed integers of 8/16/32/64 bits |
| `uint8`, `uint16`, `uint32`, `uint64` | unsigned integers of 8/16/32/64 bits |

Integer literals are written in decimal, and negative literals are supported:

```kl
let a = 100          // int (int64)
let b int8 = -128    // narrow literal accepted at initialization
```

Narrow integer types are *init-only*: they accept narrow literals at initialization, while in
expressions values are automatically promoted to the natural operand width (int8/int16/int32 to
int64, unsigned types to uint64). The promotion is a compiler mechanism; nothing hidden appears in
the standard library sources.

Integer overflow is safe by definition. The implementation provides trap and wrap overflow
policies, controlled at the compiler/VM level; literal values that do not fit the annotated type are
rejected at compile time:

```kl
let x int8 = 300    // compile-time error: overflows 'int8' range
```

### Floating-Point Numbers

The floating-point family is `float16`, `float32` and `float64`; `float` is an alias of `float64`:

```kl
let pi = 3.14159     // float (float64)
let eps float32 = 0.001
```

Like narrow integers, narrow float types are init-only and are promoted to `float64` inside
expressions. Floating-point overflow behavior is likewise a defined trap/wrap policy, not undefined
behavior.

### Integer Conversions

Conversions between integer types are explicit and use constructor-call syntax. Narrowing always
requires an explicit conversion:

```kl
func wrap(n int) int8 {
    return int8(n)        // explicit narrowing conversion
}

func widen(n int16) uint32 {
    return uint32(n)      // explicit conversion between integer types
}
```

Widening between same-sign integer widths is implicit: where a wider type is expected, the compiler
inserts the conversion automatically.

Conversions that lose information are checked: converting a value that does not fit the target range
produces a runtime panic (for example, casting `300` to `int8`).

### Integer-Floating-Point Conversions

Conversions between integers and floating-point numbers are always explicit:

```kl
let n = int(3.9)        // float -> int
let f = float(42)       // int -> float
let g = float32(2.5)    // to a narrower float type
```

No silent precision loss happens through implicit conversion: the programmer states the intent, the
compiler checks the rest.

### Booleans

The boolean type is `bool` with the two values `true` and `false`. Boolean operators are `&&`,
`||` and `!`, with short-circuit evaluation; comparison operators produce `bool`:

```kl
let ok = a > 0 && b < 10
let flag = !ok || c == 0
```

A `bool` value is also what `if`, `while` and `guard` conditions expect.

### Tuples and Variadic Types

Tuples are written with parentheses and may mix element types:

```kl
let t = (1, "a", true)
let pair = (6, 7)
```

Tuple types appear in signatures both for parameters and for multiple return values:

```kl
func foo(x int, y int, z (int, int)) (int, int, int, int, (int, int)) {
    return (x, y, x + y, x - y, (x * y, x / y))
}
```

Variadic arguments are declared with `...` after the parameter name. Inside the function the
collected arguments behave as a tuple:

```kl
func log(msgs ..., sep = ' ') {
    // msgs is a tuple of all collected arguments
}
```

The `print` builtin is the canonical example: `print(objs ..., sep = ' ', end = '\n')`.

### Null Safety

Koala is null-safe: there is no null pointer at the type level. A value that may be absent is
declared with a trailing `?` on its type, such as `int?` or `str?`. The empty value is written
`null`:

```kl
let v int? = 100
let empty str? = null
```

**Automatic unwrapping.** A `T?` value can be compared against `null` directly; inside the branch
where the value is known to be present, it is used as the unwrapped type automatically — no
explicit unwrap needed:

```kl
func show(msg str?) {
    if msg == null {
        print("no message")
    } else {
        // msg is narrowed from str? to str here
        print(len(msg))
        print(msg)
    }
}
```

The same narrowing works with `!=`:

```kl
func first_char(msg str?) {
    if msg != null {
        // msg is str inside this branch
        print(msg[0])
    }
}
```

**`if let`.** `if let` binds the unwrapped value to a new name for the branch; the binding has
type `T` and costs nothing at runtime:

```kl
func foo(v int?) int {
    if let x = v {
        return x
    } else {
        return 0
    }
}

func double_or_default(v int?, d int) int {
    if let x = v {
        return x * 2
    }
    return d
}
```

**`while let`.** `while let` unwraps repeatedly until the value becomes `null`, which makes it
the idiomatic way to walk nullable chains such as linked-list nodes:

```kl
class ListNode {
    let value int
    var next ListNode?

    func __init__(_value int) {
        value = _value
        next = null
    }
}

func find_tail(head ListNode?) ListNode? {
    if head == null {
        return null
    }
    var curr = head
    // next has type ListNode inside the loop body
    while let next = curr.next {
        curr = next
    }
    return curr
}
```

**Member access requires an unwrapped value.** A `T?` value cannot be used with the plain dot
operator — accessing a field or calling a method on it is a compile error:

```kl
class Box {
    var value int = 0

    func get() int {
        return value
    }
}

let b Box? = Box()

let v1 = b.get()     // compile error: optional cannot use dot operator.
```

Two forms are sanctioned. Optional chaining `?.` passes `null` through and yields an optional
result; force-unwrapping with `!` followed by `.` calls directly:

```kl
let v2 = b?.get()    // OK, and v2 has type int?
let v3 = b!.get()    // OK, v3 has type int; panics if b is null
```

Narrowing first works too — inside the branch the value is already unwrapped:

```kl
if let x = b {
    print(x.get())   // x is Box here
}
```

The postfix `!` forces an unwrap where the caller knows the value is present; forcing a `null`
value panics:

```kl
func must(v int?) int {
    return v!
}
```

### Union Types

A union type `A | B` lets a value be one of several concrete types. It is the sanctioned way to
accept heterogeneous inputs without overloading or `any`:

```kl
pub func __init__(s str | bytes) {}                          // str accepts bytes too
pub func open(path_or_fd str | int, mode = "r") File? {}     // a path or an open descriptor

var v int | str = 0
v = "hello"      // also fine
```

Every member of a union must be a final class — traits are never allowed as union members. Since
all Koala classes are final by construction, the constraint is naturally satisfiable; it simply
keeps unions concrete and statically checkable.

A union-typed value must be narrowed before use: comparing values of different static types
(such as `1 == "abc"`) is a compile error, and the value must be narrowed to a member type before
its type-specific operations become available.


## Strings and Characters

The string type is `str` — an immutable sequence of UTF-8 characters. String literals use single or
double quotes:

```kl
let s1 = "hello world"
let s2 = 'koala'
```

Strings support length, indexing and searching:

```kl
let s = str("hello world")
print(len(s))            // 11
print(s[0])              // first character
print(s.find("world"))   // position of a substring
```

Strings are values of the `str` class declared in the standard library itself — like every builtin
type, it has no hidden compiler magic and its full API is visible in `libs/std/builtin/str.kl`.

## Containers

Koala ships the usual container family, all declared in the standard library:

| Type | Description |
|------|-------------|
| `list` | dynamic array |
| `tuple` | immutable fixed-size sequence |
| `dict` | hash map preserving insertion order |
| `HashSet` / `TreeSet` | sets with algebra operations |
| `range` | integer interval, used heavily by `for` |
| `bytes` | fixed-length byte array |
| `ByteBuf` | mutable byte buffer |

List literals and generic instantiation:

```kl
let nums = [1, 2, 3]
let more list[int] = list[int]()
```

`dict` literals are written `{key: value}` and preserve insertion order, just like Python:

```kl
let m = {1: 2, 3: 4}
```

Containers are indexed and sliced with the usual syntax; slicing bounds follow the half-open
`[start, end)` convention:

```kl
let head = nums[0]
```

Iterating containers is covered in [Branching and Loops](#branching-and-loops): `for` accepts ranges, tuples, lists,
dicts and anything implementing the iterator protocol.

## Branching and Loops

Conditions use the C-family keywords; braces are mandatory:

```kl
if a > b {
    print(a)
} else if a == b {
    print("equal")
} else {
    print(b)
}
```

Loops come in two forms. `while` repeats on a condition, and `while let` repeats on a `T?` value:

```kl
var i = 0
while i < n {
    i += 1
}

while let x = next_node {
    // x has the unwrapped type here
}
```

`for` iterates ranges, containers and iterators. A `range` loop is compiled directly into loop
code — no range object and no iterator dispatch are created on this hot path:

```kl
for i in range(0, 10) {
    if (i % 2 == 0) { print(i) }
}

for i in range(n) {
    // range(n) is completed by the compiler to range(0, n)
}
```

`for` also unpacks tuple-shaped elements and iterates user types through the iterator protocol
(`__iter__` / `__next__` / `__has_next__`). Loop bodies may use `break` and `continue`.

Top-level iteration combinators `enumerate` and `zip` are available.

## Functions and Closures

Functions are declared with `func`. The return type may be omitted when the function returns
nothing:

```kl
func add(a int, b int) int {
    return a + b
}

func greet(name str) {
    print("hello", name)
}
```

Parameters with default values omit the type annotation — the default pins the type:

```kl
func foo(n int, msg = "hello") {
    // msg is str, defaults to "hello"
}

foo(5, "hi")     // positional
foo(5)           // msg defaults to "hello"
```

Arguments may also be passed by name:

```kl
print("hello", "world", sep=',', end='\n')
```

Multiple values are returned as tuples:

```kl
func split(x int) (int, int) {
    return (x / 2, x % 2)
}
```

Generic functions constrain type parameters with traits (see the next chapter):

```kl
func add_pair[T : Arithmetic](a T, b T) T {
    return a + b
}
```

Anonymous functions and closures are not part of the current language: named functions — including
functions defined inside packages — are the supported unit of abstraction.

## Classes and Interfaces

Classes are declared with `class`. A class is always final: no class may inherit from another.
Fields are declared with `var`, and `__init__` is the initializer:

```kl
class Foo[T] {
    var value T

    func __init__(_value T) {
        value = _value
    }

    func get_value() T {
        return value
    }
}

let foo = Foo[int](42)
print(foo.get_value())
```

Traits serve as interfaces and are the only form of subtype relationship. A class conforms to a
trait by listing it after `:`; trait inheritance is composition:

```kl
trait Fly {
    func fly() str
}

class Foo : Fly {
    func fly() str {
        return "Foo is flying"
    }
}
```

Dispatch is fully static: every type carries an intf-table built at compile time, so a trait call
is one indirect slot call (O(1), no lookup), and a direct class method call is a plain direct call.
Partial trait implementation is allowed: a conforming class may leave some methods unimplemented;
they are filled with a placeholder and panic only if actually called.

Generics are written with square brackets and constrained with trait bounds. Bare conformance infers
the self type automatically (a class conforming to `Equatable` becomes `Equatable[Self]` without
spelling it out):

```kl
func max[T : Comparable](a T, b T) T {
    if a >= b { return a }
    return b
}
```

Operators are syntax hooks: implementing `__add__` makes `a + b` work, with no declaration ceremony.
Operator dunders may never be called by name — the syntax is the only entry point. The traits
`Arithmetic[T]` and `BitwiseOperators[T]` exist as generic constraints, so functions like
`add_pair` above may require operator capabilities of an unknown type.

A concrete value can be used wherever a trait instance is expected; the value is widened to the
trait type (for example `int` where `Arithmetic[int]` is required). The reverse direction —
recovering the concrete type from a trait value — is not allowed.

Every class automatically conforms to the root contracts `Equatable` / `Hashable` / `Printable`
with identity defaults; value types override them with value semantics. The empty trait `any` is the
universal root type.

The error model — expected failures as return values, unexpected failures as panics — is covered
in its own chapter: [Error Handling](#error-handling).

## Generics

Generic classes and functions declare type parameters in square brackets:

```kl
class Foo[T] {
    var value T

    func __init__(_value T) {
        value = _value
    }

    func get_value() T {
        return value
    }
}

func add_pair[T : Arithmetic](a T, b T) T {
    return a + b
}
```

A trait bound such as `T : Arithmetic` states what the unknown type must be able to do; without a
bound, only the universally available capabilities may be used.

**Type arguments are inferred.** Koala performs generic inference so that type arguments are
written only when necessary:

- At a call site, the type argument is inferred from the arguments — the caller writes nothing:

```kl
print(add_pair(3, 4))        // T inferred as int
let foo = Foo(42)            // T inferred as int
let bar = Foo("koala")       // T inferred as str
```

- In a base list, a generic trait may be written **bare** — the type argument is inferred as the
  conforming type itself. `Comparable` means `Comparable[int64]` on `int64`; there is no `Self`
  keyword because none is needed. This is how the standard library declares the numeric types:

```kl
pub class int64 : Comparable & Arithmetic & BitwiseOperators { ... }
pub class float64 : Comparable & Arithmetic { ... }
```

  Explicit instantiation remains available when the argument differs from the conforming type,
  for example `Equatable[int]`.

**Four conformances are added automatically.** Every class automatically conforms to `any`,
`Equatable`, `Hashable` and `Printable` — they are never written in the base list. The three
contracts start with identity defaults (`__eq__` compares pointers, `__hash__` derives from the
pointer, `__str__` prints `Foo@0x7fXXXX`), and a class may override any of them with value
semantics; overriding `__eq__` without overriding `__hash__` produces a compiler warning. The
automatic conformances are visible through type introspection — nothing is hidden.

Where inference cannot apply, the argument is spelled out — for instance, instantiating a generic
class without arguments to infer from:

```kl
let foo = Foo[int](42)
let empty list[int] = list[int]()
```

Note the asymmetry for parameter types: a bare trait name is not a valid parameter type —
concrete trait instances such as `Arithmetic[int]` must be spelled out there.

## Operator Overloading

Operator overloading in Koala is a *syntax hook*: the capability is granted by the dunder method
itself, not by any trait declaration. Implementing `__add__` makes `+` work on your type — there is
no conformance ceremony. Writing the operator traits into a base list may be omitted entirely:
implementing the dunders *is* the conformance (structural conformance), and at generic call sites
the type arguments are inferred automatically.

```kl
class Point {
    var x int = 0
    var y int = 0

    func __init__(_x int, _y int) {
        x = _x
        y = _y
    }

    func __add__(other Point) Point {
        return Point(x + other.x, y + other.y)
    }

    func __sub__(other Point) Point {
        return Point(x - other.x, y - other.y)
    }

    func __neg__() Point {
        return Point(-x, -y)
    }

    func __eq__(other Point) bool {
        return x == other.x && y == other.y
    }
}

let p = Point(1, 2)
let q = Point(3, 4)
print(p + q)        // Point(4, 6)
print(p - q)        // Point(-2, -2)
print(-p)           // Point(-1, -2)
print(p == q)       // false
```

Each type defines the operand types of its operators freely; the result type is taken from the
dunder's return type. The dunder families are:

| Operators | Dunder methods |
|-----------|----------------|
| `+ - * / %` | `__add__` `__sub__` `__mul__` `__div__` `__mod__` |
| unary `-` | `__neg__` |
| `+= -= *= /= %=` | `__iadd__` `__isub__` `__imul__` `__idiv__` `__imod__` |
| `& \| ^ ~ << >>` | `__bitand__` `__bitor__` `__bitxor__` `__bitnot__` `__shl__` `__shr__` |
| `== != < <= > >=` | `__eq__` `__ne__` `__lt__` `__le__` `__gt__` `__ge__` |
| `x[i]`, `x[i] = v` | `__getitem__`, `__setitem__` |
| `x[a:b]`, `x[a:b] = v` | `__getslice__`, `__setslice__` |
| `obj(...)` | `__call__` |
| `x in seq` | `__contains__` |

Operator dunders can never be called by name — the corresponding syntax is the only entry point,
and an explicit call is a compile error:

```kl
p.__add__(q)        // compile error: use the '+' operator instead
```

Protocol hooks such as `__len__`, `__str__`, `__hash__` and the iterator dunders are not operator
hooks and may be called normally.

The traits `Arithmetic[T]` and `BitwiseOperators[T]` do not grant operator capability; they exist
purely as generic constraints. A function that wants to use operators on an unknown type states
the bound once, and the caller writes nothing extra — the type argument is inferred:

```kl
func add_pair[T : Arithmetic](a T, b T) T {
    return a + b
}

print(add_pair(3, 4))        // T inferred as int; prints 7
print(add_pair(p, q))        // T inferred as Point
```

On a trait-typed value, operators follow the substituted instance signature: with
`a Arithmetic[int]`, the expression `a + 100` is valid because `100` matches the right-hand type
`int`, while `a + b` with another trait value `b` is a natural type error.

## Error Handling

Koala's error model is a deliberate design, not a missing feature. There are exactly two kinds of
failure, and each has exactly one mechanism:

**Expected failures are return values.** When a failure is part of the normal contract — a lookup
that may miss, a parse that may reject — the function reports it through its return value, C
style: an int-returning function returns `-1`, a lookup returns a caller-provided default:

```kl
// str.to_int() panics on invalid input; to_int_or returns the default instead
let n = "42".to_int_or(0)

// dict access: get_or never panics on a missing key
let v = m.get_or(key, -1)
let old = m.remove_or(key, 0)
```

**Unexpected failures are panics.** A panic means the code is wrong — an out-of-range index, a
forced unwrap of `null`, a violated precondition. Panics are lightweight, unrecoverable and
uncatchable: there is no try/catch and no error propagation ceremony. The fix is always to fix the
code.

This model works because memory never depends on stack unwinding: the semi-automatic memory
management (see below) means a panic can simply stop the program — no destructors to run, no
resources to leak, no unwind tables to maintain. That is what keeps panics truly lightweight.

The fallible-conversion idiom is uniform across the standard library: a panic-on-failure primary
method plus a `_or(default)` variant — `to_int()` / `to_int_or(0)`, `to_float()` /
`to_float_or(0.0)`, `get()` / `get_or()`, `remove()` / `remove_or()`.

Explicitly rejected alternatives: Java-style try/catch, Go-style defer, and Result-type
composition — none of them was found to beat return values plus panic on clarity.

## Memory Management

> **Status: not yet implemented.** The model below is the announced design target of Koala's memory
> management; its pieces are on the roadmap rather than shipped behavior. Do not rely on any of it
> in current code.

Koala's memory model is officially named **semi-automatic**: deterministic release is the main
channel, and a collector exists as a safety valve rather than the primary mechanism.

**Explicit `free`.** When the lifetime of an object is known, the programmer may reclaim it
early:

```kl
free(obj)
```

**Compiler-inserted `free`.** Where lifetimes are self-evident, the compiler does the work — no
programmer attention required:

- Short-lived temporaries produced inside `for` loop iterations are reclaimed per iteration, so
  hot loops never accumulate garbage.
- Variadic arguments are automatically boxed into a tuple object; that tuple is freed
  automatically after the call. The hidden allocation and the hidden release are paired — the
  programmer never tracks either.

**Escape analysis and `noesc`.** The compiler tracks whether objects escape their scope; objects
marked `noesc` stay out of the collector's jurisdiction entirely.

**A CMS collector stands by.** A concurrent mark-sweep implementation exists in the repository but
is not enabled — the deterministic channel is the design intent, and the collector's dormant state
is public knowledge, not a secret.

The payoff is a flat memory profile: hot loops with many short-lived objects reclaim them on the
spot instead of piling pressure toward a stop-the-world pause. In design-space terms, Koala lands
in the unoccupied zone between Rust/C++ (deterministic but ceremony-heavy) and Go/Java
(ceremony-free but nondeterministic): deterministic *and* low-ceremony, because the compiler
inserts `free` where no thinking is needed and the programmer intervenes only for earlier
reclamation.

## Package Management

Packages are imported by path; an alias may be attached with `as`:

```kl
import "test_pkg/pkg1"
import "test_pkg/pkg1" as z
```

Imported symbols are accessed through the package name:

```kl
pkg1.hello()
let p = pkg1.Point(1, 2)
let fly pkg1.Fly = foo      // a value typed as an imported trait
```

Visibility is controlled with `pub` (public). Symbols without `pub` stay inside their package.

The standard library is a regular package (`std/builtin`) written in Koala itself: every builtin
type is declared in plain `.kl` sources under `libs/std/builtin`, so the language core contains no
hidden compiler magic. Package layout follows two simple rules: a simple package is a single
`xxx.kl` file; a complex package is a directory whose name is the package name, with
`__<package>__.kl` as the entry file holding only `link` statements, global variables and top-level
functions, and one class or trait per `xxx.kl` file.

Native code is attached with a `link` declaration and `@native` functions:

```kl
link "koala_ext_native"

@native
pub func pretty(s str) str {}
```

The compiler bridges the call; there is no `unsafe` and no manual marshaling, and objects allocated
on the native side are managed by the same memory machinery as Koala objects.

## Native Extensions

Native code ships as shared libraries (`.so`). This section shows how to build one and which C
interfaces are involved.

### The Koala side

A package declares the library with `link` and the foreign functions with `@native` — the body
stays empty, the ordinary Koala signature is the whole contract:

```kl
link "pkg1_native"

@native
pub func test_pkg_foo(a Animal) {}
```

When the module is loaded, the VM `dlopen`s `libpkg1_native.so` and looks up an initializer named
after the last package segment plus `_native_lib_init` — for this package, `pkg1_native_lib_init`.

### The C side

Native functions share one signature — `self`, an argument array, and an argument count, returning
a `TValue`:

```c
#include "object.h"

// func swim(name str, age int) str
TValue foo_swim_func(TValue *self, TValue *args, int nargs)
{
    TValue *name = &args[0];
    TValue *age = &args[1];

    char *_name = STR_BUF(to_obj(name));
    int _age = to_int64(age);

    Object *s = kl_new_fmt_str("<%s is swimming, age %d>", _name, _age);
    return obj_value(s);
}
```

The initializer registers everything the library provides:

```c
void pkg1_native_lib_init(NativeLib *lib)
{
    kl_reg_func(lib, "test_pkg_foo", test_pkg_foo_func);   // a top-level function
    kl_reg_meth(lib, "Foo", "swim", foo_swim_func);        // a method on class Foo
    kl_reg_type(lib, &file_type);                          // a native-defined type
}
```

### The C interfaces

Registration (declared in `object.h`):

| Interface | Purpose |
|-----------|---------|
| `kl_reg_func(lib, name, fn)` | Register a top-level function |
| `kl_reg_meth(lib, cls, meth, fn)` | Register a method on class `cls` |
| `kl_reg_type(lib, tp)` | Register a native-defined type |

Value construction and extraction:

| Interface | Purpose |
|-----------|---------|
| `int64_value(v)`, `bool_value(v)`, `obj_value(o)`, `none_value` | Build a `TValue` |
| `to_int64(v)`, `to_obj(v)` | Extract an int64 / object from a `TValue` |
| `STR_BUF(o)` | Get the C string buffer of a str object |
| `kl_new_str(s)`, `kl_new_fmt_str(fmt, ...)` | Create str objects |
| `kl_typeof(v)` | Runtime type of a value |

Calling back into Koala (for example, invoking a trait method on a passed object):

```c
Object *fn = kl_get_intf_func(args, 0);                  // fetch a method slot
TValue _args[] = { args[0], obj_value(kl_new_str("Eagle")), int64_value(3) };
return kl_object_call(fn, _args, 3);                     // call it
```

### Building the library

Compile the C file as a shared library and link against the Koala runtime:

```bash
gcc -fPIC -shared pkg1_native_impl.c -o libpkg1_native.so \
    -Iinclude/runtime -Iinclude/common \
    -Lbuild/DebugTest/lib -lkoala
```

### Binding rules

- `@native` with a matching C implementation binds at module load; a missing implementation fills
  a placeholder and prints a warning — calling it panics with "user must implement it".
- A plain Koala function may be *overridden* by a same-name C implementation — the hot-path entry
  for optimization.
- There is no `unsafe`: the type system trusts the declared Koala signature, and objects allocated
  on the C side are registered as GC roots through the shadow stack, so they live and die exactly
  like objects allocated in `.kl` code.
