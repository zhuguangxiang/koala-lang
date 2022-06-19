# Memo

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
3. merge AST node
   1. unary +, -, ~, ! literal
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

## binary operations

1. integer: add/sub/mul/div/mod/shl/shr/bit_op/cmp_op
2. float: add/sub/mul/div/mod
3. relation: and/or : bool
4. string: add
