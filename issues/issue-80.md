# union 成员里可以含类型参数

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— `check_union_subtypes` 只看成员本身，漏掉成员内部的类型参数

> 来源：`docs/Koala_TODO.md` §76（2026-11-07，to fix）

---

### 详细描述

<details>
<summary><b>🔁 复现</b></summary>

```kl
trait Shapes[T] { func uni(x list[T] | bool) int }    // no error here

class S : Shapes[str] {
    pub func uni(x list[str] | bool) int { return 2 }
}

func main() {
    let s Shapes[str] = S()
    s.uni(["x"])    // error: expected 'bool | list[Shapes:T]', but got 'list<u>'
}
```

</details>

<details>
<summary><b>🧠 根因</b></summary>

union 是**具体类型**，所以 `T | bool` 会被报成 `Union subtype must be a class`。

但成员**内部**的类型参数没被报出来：`src/parser/parser.c` 的 `check_union_subtypes` 只看成员本身，而 `list[T]` 是某个 class 的实例。

</details>

---

### 修复方案

<details>
<summary><b>🛠 待办</b></summary>

1. 在**声明处**报错：对每个成员，检查其中是否含类型参数。
2. 随后从 `test/test-ut/test_trait_impl_match.kl` 的 `Shapes` 里删掉 `uni`。
3. 把该错误加入 `test/test-kl` 的某个用例。

</details>