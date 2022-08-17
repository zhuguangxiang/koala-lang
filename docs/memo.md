# 备忘

## 类型

整数，浮点数，字符，布尔，字符串，数组，Map，Any

- 结构体
  - 值类型
- 类
  - 引用类型
- 接口
  - 胖指针
- 枚举
- 元组

## 指令集&虚拟机

### 整型

```go

func foo(a int) int {
   return a + 200
}

/*
int32.const 200
local.get 0
int32.add
ret 1
*/

var i = 100
var r = foo(i)

/*

int32.const 100
int32.store 0

int32.load 0
call index of func table

int32.store 1
*/

/*
+---------+
| module  |
+---------+
|  name   |
+---------+
| pointer |
+---------+
*/

```

### 结构体

```go
struct Foo {
   var a int
   var b int

   func __init__() {
      a = 1
      b = 2
   }
   /*
      i32.const 1
      local.get 0, 8
      i32.store 0
      -------------------


      i32.const 2
      local.get 0, 8
      i32.store 4

      ret 0
   */

   func add(o Foo) {
      this.a += o.a
      this.b += o.b
   }
   /*
      local.get_ref 1
      i32.load 0

      local.get 0
      i32.load 0

      add

      local.get 0, 8
      i32.store 0

      ref.load 8
      field.i32.get 4
      ref.load 0
      field.i32.get 4
      add
      ref.load 0
      field.i32.set 4
   */
}

var foo = Foo()
/*
   local.get_ref 0
   call index of func table
*/

var bar = Foo()
foo.add(bar)

/*
   local.get_ref 0
   local.get 8, 8
   call index of func table
*/

```

### 不可继承类

abstract class 可继承的类
class 不可继承的类（默认）

### 可继承类和接口(trait)























1. primitives
   1. integer: i8, i16, i32, i64
   2. float: f32, f64
   3. char: i32
   4. bool: i8
   5. string?

2. builtin types
   1. any
   2. array
   3. map
   4. pointer
   5. string?
   6. Option?
   7. Result?
   8. Union? vs enum?
   9. tuple

## literal

1. int: int64/int32
2. float: float64/float32

## expression cast & unary operations

1. expression cast
   1. integer
      1. overflow
   2. float
      1. overflow
   3. object
      1. runtime
2. literal expr cast
   1. integer
      1. overflow, compiling
   2. float
      1. overflow, compiling
3. literal expr merge AST node
   1. unary expr +, -, ~, !
   2. cast expr
4. unary +:
   1. only number
   2. other error
5. unary -:
   1. only number
   2. other error
6. unary ~:
   1. only integer
   2. literal integer get the result?
   3. other error
7. unary !:
   1. only bool
   2. other error
8. cast
   1. merge?
9. literal handle moves to build AST?

## binary operations

1. integer: add/sub/mul/div/mod/shl/shr/bit_op/cmp_op
2. float: add/sub/mul/div/mod
3. relation: and/or : bool
4. string: add
5. literal no merge

## architecture

1. klm_xx merged into symbol?
