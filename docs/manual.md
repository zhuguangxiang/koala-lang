
# The Koala Language Manual

Koala is a fast, flexible and modularized modern programming language.

It's features are below and more:

## Koala is object-oriented

Everything is an object in Koala. Types are described by classes, traits and enumerations. Classes are extended by subclass and a mixin mechanism as a replacement for multiple inheritance.

## Koala is functional

Koala is also a functional language in some sense that every function is an object. Koala also supports anonymous functions and allows anonymous functions to be nested. One function can be as a common value assigned to a variable, and as a function's or anonymous function's return value. It also supports map, filter and reduce operations.

## Koala is a type-safe language

Koala has a strong type system. All types are checked at compile-time. And it also support generic types.

## More in Koala

- Koala has exception mechanism, but only used internal. Once exception is occurred, Koala will kill current thread.
- Koala introduces Option and Result enums to handle null exception and other exceptions.
- Multi values return is supported by Tuple as return value.
- Koala can be extended by c-language to develop modules.

## Trying Koala

Koala supports three running mode. One is interactive mode, it's like python and other language's similar mode. The second mode is compile mode. It's like `javac` and other compiled languages to convert source code into binary code. The last mode is running mode. It's like `java` to run koala's binary code, called '.klc' ('dot klc')

For examples, interactive mode:

```shell

[james@host ~]$ koala
koala 0.9.1 (Oct 27 2019)
[GCC 8.3.0] on Linux/x86_64 5.0.0-32-generic
> "hello world"
hello world
> 100
100
> true
true
> false
false
> 1.23
1.23
> [1,2,3]
[1, 2, 3]
> (1,2,3)
(1, 2, 3)

```

The below command to compile koala source into binary code.

`koala -c <file-name.kl>[<file-name>][<dir-name>]`

```shell

[james@host ~]$ ls
foo.kl
[james@host ~]$ koala -c foo.kl
[james@host ~]$ ls
foo.kl  foo.klc

[james@host ~]$ rm *klc

[james@host ~]$ ls
foo.kl
[james@host ~]$ koala -c foo
[james@host ~]$ ls
foo.kl  foo.klc

```

If given dir-name to koala, it means see the source files in directory as a module named `dir-name`.

```shell
[james@host ~]$ ls github.com/koala-examples/foobar
bar.kl foo.kl
[james@host ~]$ koala github.com/koala-examples/foobar
[james@host ~]$ ls github.com/koala-examples
foobar  foobar.klc

```

After compiled .kl into .klc, you can run it using koala command

`koala <file-name>[<file-name>.kl][<file-name>.klc]`

```shell

[james@host ~]$ ls
foo.kl  foo.klc main.kl
[james@host ~]$ koala main
[james@host ~]$ ls
foo.kl  foo.klc main.kl main.klc

```

If there is no .klc file, using `koala <file-name>` command will compile and run it automatically. If `<fil-name>` is a directory, koala will try to compile the directory as a module and run it.

## Keywords And Identifer

## Variable And Constant

## Comments

## Semicolons

## Literals

## Basic Operators

## Flow Control

## Function, Callback And Closure

## Namespace And Import

## Class

Class is likely other languages. Using `class` keyword to define a class.

```go

class Foo {
  // class members definition
}

```

You can add some fields and methods into class. Example:

```go

class Rectangle {
  x int;
  y int;

  func __init__(x int, y int) {
    self.x = x;
    self.y = y;
  }

  func area() int {
    x * y
  }
}

```

The `__init__` function is special function, will be called after new object.
It's likely constructor in other languages. And it has no return value.

You can initial field's value at its definition.

```go

class Rectangle {
  x int = 0;
  y int = 1;
  // other members definition
}

```

Notes: all members of class/trait/enum can be accessed directly. That's `Koala`
hasn't access control to access fields, methods or enumeration's labels.

```go

rect := new Rectangle(1, 1)
rect.x
rect.y
rect.area()

```

## Trait

## Enum

## Inheritance

## Operator Overloading

## Built-in Data types

# Module And Extension

## Module

## Using External Modules

## Extension Using C

# More Topics

## Files, Formatting And IO

## Map, Filter And Reduce

## Error Handling

## Option And Result

## Generic Type

## Type Casting

## Concurrence

## Auto Garbage Collection

### Reference Count

### Mark And Sweep
