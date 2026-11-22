# `break` / `continue` 会越过函数边界找循环（潜伏）

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复（潜伏）—— 今日不可达；随闭包 / 局部函数一起修

> 来源：`docs/Koala_TODO.md` §51（2026-11-02，latent）

---

### 详细描述

<details>
<summary><b>🔍 现状</b></summary>

`find_loop_scope()`（`src/parser/parser.c`），被 `parse_break()` / `parse_continue()` 使用，沿 `ps->scope` 向外走直到遇到 `WHILE_BLOCK` / `WHILE_LET_BLOCK` / `FOR_BLOCK`，但**从不在 `SCOPE_FUNC` 或 `SCOPE_ANONY` 停下**。

于是嵌套在循环内的函数体里的 `break` 会找到外层函数的循环并通过检查，irgen 就会跳到**另一个函数的**基本块。

</details>

<details>
<summary><b>🔒 今日为何不可达</b></summary>

匿名函数与闭包尚不属于语言的一部分（手册 Functions），函数体内声明 `func` 也是语法错误。

</details>

---

### 修复方案

<details>
<summary><b>🛠 与闭包（或局部函数）一起修</b></summary>

- `find_loop_scope()` 的遍历在 `SCOPE_FUNC` / `SCOPE_ANONY` 处停止，并在那里报 "break statement must be inside a loop."。
- `return` 已通过 `get_current_function()` 解析，它停在最近的 `SCOPE_FUNC` —— 届时需确认它对 `SCOPE_ANONY` 也停。

</details>

<details>
<summary><b>📌 起因</b></summary>

讨论 `if` 表达式里的语句块时读到（该提案未采纳：`if` 表达式保持每分支一个值，只替代 `?:`）。**读代码得出，当时无法复现**。

</details>