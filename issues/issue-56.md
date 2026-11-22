# Truthiness 协议与 all/any 函数（未拍板，不一定支持）

← 返回 [问题列表](issues.summary.md)

- [ ] 未实现 —— 作者未最终拍板，标注「**不一定支持**」

> 来源：`docs/Koala_TODO.md` §10（2026-08-31 建立）

---

### 详细描述

<details>
<summary><b>💡 设想</b></summary>

引入 `Truthiness` trait，作为语法钩子支撑 `if obj:` 语法和 `all()` / `any()` 泛型函数。

```koala
pub trait Truthiness {
    func __bool__() bool
}
```

</details>

<details>
<summary><b>⚙️ 编译器行为</b></summary>

`if obj:` → 编译期检查 `T` 是否实现 `Truthiness` → 是则下降为 `if obj.__bool__():`，否则编译报错。

与现有运算符钩子（`+` / `==`）模式**完全对称，无隐式转换**。

</details>

<details>
<summary><b>📦 实现范围</b></summary>

- 基本类型实现：`int`（0 = false）、`str`（空 = false）、`bool`（自身）
- 容器类**不**实现（`if my_list:` 不合法，必须 `if my_list.len() > 0:`）

</details>

<details>
<summary><b>⏳ 未拍板原因</b></summary>

引入 `Truthiness` trait + `if obj:` 语法糖是否符合 Koala 的「简洁」原则，尚需作者最终决定。

**不做也没有功能缺失** —— 用户可显式写 `if x != 0` / `if x != ""`。

</details>

---

### 修复方案

<details>
<summary><b>🛠 下游产物：all() / any()</b></summary>

`Truthiness` 落地后 `all()` / `any()` 变为可行：

```koala
func all[T: Truthiness](items Iterable[T]) bool { ... }
func any[T: Truthiness](items Iterable[T]) bool { ... }
```

检查的是**元素**的真假，不是容器的真假，与 Python 语义一致。

</details>