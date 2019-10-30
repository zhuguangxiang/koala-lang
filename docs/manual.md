
# Basics

Koala is a new programming language for general app development.

In Koala, everything is an object.
Like common object, function can be assigned to a variable and as return value.
And it also supports map, filter and reduce operations.

Koala is a type-safe language. All types are checked at compiled time.

Koala has exception mechanism, but only used internal.
Once exception is occurred, Koala will kill current thread.
Koala introduces Option and Result enums to handle null exception.

Multi values return is supported by Tuple as return value.

## Trying Koala

## Keywords And Identifer

## Variable And Constant

## Comments

## Semicolons

## Literals

## Basic Operators

## Flow Control

## Function, Callback And Closure

## Namespace And Import

# Class, Trait And Enum

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
