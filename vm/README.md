# Koala Virtual Machine

The byte code of `koala` is executed by virtual machine.

## execution model

### stack

Reference (not Any and final) in stack is `fat pointer`.
Primitive, Any and final class in stack is `uintptr`.
When call a function or method, vm will create a `CallStack`.

### code relocation
