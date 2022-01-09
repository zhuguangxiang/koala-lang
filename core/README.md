# Koala Virtual Machine

The byte code of `koala` is executed by virtual machine.

## execution model

### stack

Reference (not Any and final) in stack is `fat pointer`.
Primitive, Any and final class in stack is `uintptr`.
When call a function or method, vm will create a `CallStack`.

### code relocation

## `core` package

The `core` package is built-in and auto-imported package, which includes

- `TypeInfo`: is a class, trait and enum meta-data.
- `Package`: represents a `koala` package and managed by `path`
- `Any`: is the root trait, which has `__hash__()`, `__eq__()`, `__class__()` and `__str__()`
- `Array`
- `Map`
- `string`
- `Class`
- `Field`
- `Method`

## generic type

The generic type is used by runtime for stack map and object map of gc.
If the up bound of generic type is `Any`, it can be primitive and reference value and its variable layout is single `uintptr`.
If it is other detailed class or trait, its layout is `fat pointer`.
The single `uintptr` is reference or not is determined by real type, so when new an object or call a generic function, compiler will pass the real type to runtime by `type bitmap`, which is `uint64` and one type occupy 4 bits and the max number of generic type of one class or function is 16, it's enough.

## package

The `koala` compiler will generate `.klc` from source `.kl`, and may generate native code (shared object).

- only `.klc`: includes meta-data and byte code
- `.klc` and `so`: includes meta-data in `.klc` and native code in `so`

After an image (`.klc`) is loaded, the runtime need link relocation symbols, it's the same with `ELF`.

The byte code will be interpreted by `koala virtual machine`.

The core package is hashed by `'/'` path.

## class

## trait

## inheritance

## `init_core_pkg()`

initial the core package

## `fini_core_pgk()`

finalize the core package

## API

### `TypeInfo`

### `Package`

### `Any`
