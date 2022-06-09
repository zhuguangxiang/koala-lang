# Koala Programming

## Hello World

```go
// hello.kl
func main() {
    println("hello, world")
}
```

```bash
klc hello.kl
```

## FFI

```c++
struct Foo {
    s Pointer<u8>
    len u32
}

struct Bar {
    val u32
    foo Foo
    foo_ptr Pointer<Foo>
}

class Baz {
    foo_ptr Pointer<Foo>
    s String
}

func main() {
    baz := Baz()
    foo := Foo()
    foo.s = "hello"
    baz.foo_ptr = addrof(foo)
    bar := call_c_func(addrof(foo), sizeof(Foo))
    bar.foo_ptr[0] = foo
    int_ptr := malloc(100 * sizeof(u8))
    println(typeof(int_ptr)) // Pointer[u8]
    arr := [1,2,300] as [u8]
    int_ptr = arr.raw_ptr()
    s_arr := ["hello", "world"]
    s_ptr := s_arr.raw_ptr() // Pointer<Pointer<u8>>

}

extern "C" {
    func call_c_func(foo Pointer<Foo>, size u8) Pointer<Bar>;
    func malloc(size u32) Pointer<u8>
}
```

## Type System

There are two kinds of types, one is value type, the other is reference type.

primitive types, struct types and enum types are all value types. class is reference type.
string type is value type, dynamic array and hashmap are reference types.

`self` represents Object itself, and is pointer.
example: Int32 type,  operator add looks like below:

```go
func __add__(rhs Int32) Int32 {

}
```

And it's implementation looks like below:

```c
int32_t _ZN3std5Int327__add__(int32_t *self, int32_t rhs) {

}
```

### Interface

Interface in koala is the same with Java's interface. In implementation, it's memory layout is below

| interface             |
| --------------------- |
| virtual table pointer |
| type info pointer     |
| uint32 v1             |
| uint32 v2             |
| gc object pointer     |

This layout is the same with `String`, and compatible with `Number` types. In Number, the gc object pointer is null.
v1 and v2 combined together into uint64 for all number types, integer, float, char and bool.

Other layout is

```bash

+----------+
| vtbl     |
+----------+
| obj      |--------+
+----------+        |
                   \|/
                +----------+
                | typeinfo |
                +----------+
                | u64      |
                +----------+

```

## operator overload

## Box class

The Box class is used for value types, such as primitives and struct.

```go
var s = "hello"

var ts ToString = Box(s)

extern "C" {
    func printf(fmt Pointer<u8>, ...)
}

func print(ts ToString) {
    raw := s.to_string().as_ptr()
    printf("%s".as_ptr(), raw)
}

```

## Module and Package

One Module can contain multi packages, like Java, Jar is a module, and contains multi paths,
each path also can contain multi .class files.

In Koala, one module is a shared object file or an archive file, which contains multi packages, each package has different name,
symbols, in one package, are mangled to solve symbol name conflict.

binary:
/usr/lib/libkoala-rt.so
/usr/lib/koala/meta files and generic source files

standard library:

```bash
+ koala
  + std
    - xx.kl (merged generic into one file one package)
    - xx.klm (merged meta into one file one package)
    - libkoala-std.so (one module, one so)
    + io
        - xx.kl
        - xx.klm
    + sys
        - xx.kl
        - xx.klm
    + socket
        - xx.kl
        - xx.klm
  + net
    + http
    + rpc
  + zip
    + gzip
  + json
  + xml
  + libkoala-rt.so
```
