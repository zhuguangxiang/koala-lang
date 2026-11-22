# `if` 表达式的后续扩展（6 项，to discuss）

← 返回 [问题列表](issues.summary.md)

- [ ] 未实现 —— 6 项延后；其中第 5 项已决策不再支持

> 来源：`docs/Koala_TODO.md` §36（2026-10-31）

---

### 详细描述

<details>
<summary><b>🔍 现状</b></summary>

`if c { a } else { b }` 在 `let` / `var` 初始化式、`=`、`return` 与 `= expr` 函数体中是**值**（手册 "Branching and Loops"；`test/test-run/test_if_expr.kl`）。以下留待以后：

</details>

---

### 修复方案

<details>
<summary><b>1️⃣ 分支内的 narrowing</b></summary>

`parse_if()` 对 `x != nil` / `x is T` 在 then/else 块内做窄化；`parse_if_expr()` **不做**，所以 `let n = if x != nil { x! } else { 0 }` 需要 `!`（或 `x ?? 0`）。

需要 `parse_if()` 的 shadow 变量作用域套到每个分支上，以及 irgen 侧的对应改动。

</details>

<details>
<summary><b>2️⃣ `if let` 形式</b></summary>

`let n = if let v = x { v } else { 0 }`。

</details>

<details>
<summary><b>3️⃣ `x = if ...` 的期望类型</b></summary>

`parse_assign()` 先访问右侧再访问左侧，所以分支拿不到期望类型，必须自己互相一致 —— `f = if c { Bird() } else { Plane() }`（`f Fly`）失败；用带类型的 `let` 可以。

</details>

<details>
<summary><b>4️⃣ `panic()` / `unreachable()` 分支</b></summary>

其类型是 no-type，与另一分支不匹配；可作为「never returns」放行。

</details>

<details>
<summary><b>5️⃣ 其他位置（已决策：不支持）</b></summary>

实参、操作数、`x += if ...`。**有意排除**，跟随 Swift（2026-10-31 决定，调用实参同样要求先用 `let` 绑定值）；仅在出现具体需求时重新考虑。

</details>

<details>
<summary><b>6️⃣ `match` 作为表达式</b></summary>

在同样的位置上支持 `match` 表达式。

</details>