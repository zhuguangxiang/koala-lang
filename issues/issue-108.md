# 解构、`for` 与窄化：延后任务（6 项）

← 返回 [问题列表](issues.summary.md)

- [ ] 未实现 —— 计划约 2026-11-28 前后启动

> 来源：`docs/Koala_TODO.md` §31（2026-10-28）

---

### 详细描述

<details>
<summary><b>✅ 已完成</b></summary>

`for (x, y) in ...` / `for x, y in ...`（含 `_`）、局部 `let (a, b) = t` / `var (a, b) = t`（可按字段写类型）、以及坏 `for` 头的安静错误恢复。

</details>

---

### 修复方案

<details>
<summary><b>1️⃣ `enumerate` / `zip`（规模 M，先设计）</b></summary>

目标：`for (i, x) in enumerate(xs)` 与 `for (a, b) in zip(xs, ys)`。

**待定问题**：

- `std/builtin` 里的自由函数（Python 风格）还是迭代器方法（`xs.iter().enumerate()`，Rust 风格）？
- 产出的类型（`Iterator[(int, T)]` / `Iterator[(A, B)]` 类，如 `EnumerateIter[T]` / `ZipIter[A, B]`）？
- `zip` 是否在较短输入处停止？
- 它们接受任何有 `__iter__` 的东西，还是只接序列？

**文件**：`libs/std/builtin/*.kl`（或 native C），以及 3 个文档。**风险**：可能暴露泛型迭代器推断的缺口。

**2026-10-28 核实：仍未解决。** `libs/std/builtin/enumerate.kl` 与 `zip.kl` 里只有注释掉的桩（`Enumerate[T] : Iterator[(int, T)]`、`Zip[T, U] : Iterator[(T, U)]`），所以 `enumerate(xs)` / `zip(xs, ys)` 报 "'enumerate' is not found"。§25 的清单把它们列在 `std/builtin` 下，但**尚未实现**。（同样见于 [#88](issue-88.md) 的泛型场景第 5 项。）

</details>

<details>
<summary><b>2️⃣ 顶层与类字段中的解构（规模 M–L）</b></summary>

目标：模块级 `let (a, b) = f()`，也许类体内的 `var (x, y) = (0, 0)`。

**工作**：让 `tuple_decl` 能出现在顶层（与类成员）文法；`parse_tuple_decl` 目前只处理 `VAR_SCOPE_LOCAL`，需要 global / field 变体并处理全局初始化顺序；irgen 要在模块 init 里写入全局（或在 `__init__` 里写字段）；每个名字都要像普通 `let` / `var` 一样导出到 klc。

**待定**：类字段值不值得做（Rust / Go 都不允许）？**建议：只做顶层。**

**文件**：`koala.y`、`parser.c`、`irgen.c`，也许 `write_klc.c`。

</details>

<details>
<summary><b>3️⃣ 嵌套解构（规模 M）</b></summary>

目标：`let ((a, b), c) = t` 与 `for (k, (x, y)) in m`。

**工作**：`tuple_decl_list` / `id_list` 里支持带括号的项；`IdentType` 项在 AST 里变成 pattern 树；递归的类型 / 数量检查；递归的 `build_unpack_vars`。

**待定**：内层是否可带类型（**建议：可以，与顶层一致**）。

**文件**：`koala.y`、`ast.h` / `ast.c`、`parser.c`、`irgen.c`。**风险**：改变 `for` 与 `tuple_decl` 共享的 AST 形状。

<details>
<summary><b>4️⃣ 通过 `&&` / `||` / `!` 的窄化（规模 M）</b></summary>

**Level 1**：`if x != nil && y != nil { x + y }` 在 then 块里窄化两者。

**Level 2**：`if x == nil || y == nil { return }` 在 `if` 之后（以及 else 块里）窄化两者；真正的 `!` 翻转事实；任意嵌套深度。

**约束**：只改窄化代码；不引入游离符号（`stbl == NULL`）；未经同意不重构 `parse_if` / `parse_block` / `symbol.c`。

**开放的设计点**：如何把 `||` 的「仅 else」事实带到 else 块与提前返回路径上，而不引入游离符号。

**测试**：test-run 覆盖被窄化的结果；test-kl 覆盖不该窄化的（then 块里的 `||`、else 块里的 `&&`、`&&` + 提前返回）。

**Note (2026-11-03)**：`if` 表达式通过 `parse_if` 同一套 `unwrap_for_if()` / `inherit_if_shadows()` 窄化分支（`parser_expr.c` 的 `parse_if_expr()`），所以第 4、5 项也适用于 `let n = if v != nil && v.len() > 1 { ... } else { ... }`；届时把这些 if 表达式用例加进测试。

</details>

<details>
<summary><b>5️⃣ 右操作数内部的窄化（level 3，规模 S–M，在第 4 项之后）</b></summary>

目标：`if s != nil && s.len() > 0`，检查 `s.len()` 时 `s` 已被窄化。

**工作**：访问 `A && B` 时，在持有 `A` 真事实的临时作用域里访问 `B`（`A || B` 则用假事实）；这发生在表达式访问期间（`parser_expr.c`）。

**风险**：`B` 的 IR 必须使用窄化后的符号。

</details>

<details>
<summary><b>6️⃣ `if let (a, b) = t` / `while let (a, b) = t`：可选元组的解构（规模 M，暂不实现，2026-10-30 记录）</b></summary>

目标：`t` 是可选元组 `(T1, T2)?`，不为 `nil` 时解构并进入 then 块（或循环体），规则与元组 let-else 相同：支持 `_`、名字可带类型、名字个数必须与元组大小一致。

```koala
if let (host, port) = split_host_port(url) {
    print(host, port)
}
```

**为什么暂不实现**：

- 常见需求是「取到值，否则提前退出」，已由元组 let-else（`let (a, b) = t else { ... }`）覆盖；`if let` 用于「只有存在时才做某事」，配合元组的场景较少。
- 现有写法只多一行，可以接受：`if let t = f() { let (a, b) = t ... }`。
- 实现不是零成本：需新增 `IF LET '(' tuple_decl_list ')' ...` 与 `WHILE LET ...` 语法规则；`parse_if_let()` / `parse_while_let()` 现在只绑定一个符号，要改为绑定多个名字；irgen 要在 then 块（或循环体）开头用 `build_unpack_vars()` 解构。

**为什么以后可能要支持**：

- 补齐一致性：`let (a, b) = t`、`let (a, b) = t else { }`、`for (a, b) in ...` 都已支持解构，`if let` / `while let` 是剩下的地方。
- 其他语言都有：Rust `if let Some((a, b)) = f() { }`，Swift `if let (a, b) = f() { }`。

**注意区分**：这**不是**之前已放弃的「多值 if let / while let」（8 条 NYI 规则，同时要求多个可选值都不为 `nil`，如 `if let a = x, b = y`）。这里只有一个可选元组，语义与元组 let-else 一致，**不会推翻那个决定**。等实际代码需要时再实现。

</details>

<details>
<summary><b>🔗 关联</b></summary>

- `if` 表达式的分支窄化缺失：见 [#63](issue-63.md) 第 1 项。
- `enumerate` / `zip` 不可用：见 [#88](issue-88.md) 泛型场景第 5 项。

</details>
</details>