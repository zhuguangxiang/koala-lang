### class object
The `object` class is the base class of all other classes and
has some basic functions like java/kotlin. All functions are implemented by runtime.

usage:

```swift
let o = object()
```

The example creates an object. The base object has very less functions, so it's useless.

The `object` type is builtin type and implemented by runtime, so
the static `__new__` function is defined here.

> Q: When does a class need `__new__`?
> A: If the class is implemented by runtime(c language), most of cases need it.
> And if the class is implemented by koala, it is NOT necessary.
>
> Q: When does a class need `__init__`?
> A: If the class is implemented by runtime(c language), it is optional.
> And if the class is implemented by koala, most of cases need it.
>
> The `__init__` function is like other language's constructor, but `__new__` does not.
> The `__new__` and `__init__` functions MUST have the same arguments.
> The `__new__` must be a static function, and `__init__` must be a common function.

- public static func \_\_new\_\_\(\)

- public func \_\_add\_\_()
