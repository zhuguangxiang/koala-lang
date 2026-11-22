# `T` 不能透过 `Box[T?]` 形参推断

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 从泛型实例推断时，`T?` 无法与 `int?` 匹配

> 来源：`docs/Koala_TODO.md` §77（2026-11-07，to fix）

---

### 详细描述

<details>
<summary><b>🔁 复现</b></summary>

```kl
class Box[T] {
    var v T
    func __init__(value T) { v = value }
    func get() T { return v }
}

func same[T](b Box[T?]) Box[T?] { return b }

func main() {
    let a Box[int?] = Box[int?](1)
    let c = same(a)    // error: cannot infer type parameter
    print(c.get()!)
}
```

</details>

<details>
<summary><b>✅ 已可用的部分</b></summary>

创建对象、以及返回 `Box[T?]` 都正常，见 `test/test-ut/test_optional_type_arg_new.kl`。

</details>

---

### 修复方案

<details>
<summary><b>🛠 待办</b></summary>

从泛型实例推断时，让 `T?` 与 `int?` 可以匹配；然后把这次调用加进上面提到的测试。

</details>