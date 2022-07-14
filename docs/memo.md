# Memo

## types

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
