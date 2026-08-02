
# Development Plan

## Version 0.8.0

1. Support non primitive types(user defined class) number operators
    - __add__
    - __sub__
    - __mul__
    - cmp&eq(__lt__ etc)

```swift
let s0 = "hello"
let s1 = "world"
s0 == s1, s0 > s1
```

2. Support load type as a normal object for more easily used.

```
class A {

}

print(A.methods()) // this is static method call, not type's type method

```
