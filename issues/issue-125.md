# `let-else` 与 `panic()` 作终结语句：遗留项（已结案）

← 返回 [问题列表](issues.summary.md)

- [x] 已结案 —— `let-else` 已实现；两项「Pending」均已定案（一项决定不做，一项已修）

> 来源：`docs/Koala_TODO.md` §30（2026-10-27）

---

### 详细描述

<details>
<summary><b>✅ 现状：`let v = expr else { ... }` 已实现（仅单值，不支持解构）</b></summary>

文法在 `let_else_stmts()`（`koala.y`）里做脱糖：拆成两条兄弟语句 `let v = expr` 与 `if v == nil { ... }`（`IfStmt.is_let_else = 1`），复用 early-return 的 shadow 窄化。

`parse_if()` 对 `is_let_else` 做两项检查：

1. `v` 必须是 optional（在生成的 `v == nil` 被访问**之前**检查）；
2. else 块必须退出（`return` / `break` / `continue` / `panic`）。

`panic()` 通过 `is_panic_call()`（匹配已解析的 `std/builtin.panic` 符号）在 `is_terminal()` 里算作终结。

</details>

---

### 修复方案

<details>
<summary><b>⏸ 遗留项 1：irgen 在 `panic()` 之后仍有一条落空边 —— 决定不做（2026-10-28）</b></summary>

`panic` 是普通 `@native` 函数，所以持有该调用的基本块**仍有一条**指向后续代码的边（运行期永不走到：VM 已停）。对 let-else / early-return 窄化来说，这条假边是一条「`v` 为 nil 却流入非 nil 使用」的 IR 路径，优化器仍会推理它并保留汇合点。

**原定修法**：在 `emit_ir_expr()` 里，对 `is_panic_call(exp)` 为真的调用之后发射 `klr_build_unreachable`（`irgen.c` 里原生函数体已在用），并把当前块当作 return / break / continue 处理，不发射落空跳转。

**Decided not to do (2026-10-28)**：多出的 `unreachable` 指令只会增大代码体积；那条落空边运行期永不走到，**保持现状**。

</details>

<details>
<summary><b>✅ 遗留项 2：`unreachable()` 也应算终结 —— 已修（2026-10-27）</b></summary>

`std/builtin.unreachable()` 同样永不返回，但 `is_terminal()` 不认它，于是 `let v = e else { unreachable() }` 报 `block must exit`。

**Resolved (2026-10-27)**：`is_panic_call()` 升级为 `is_noreturn_call()`，同时匹配 `std/builtin` 里的 `panic` 与 `unreachable`；`is_terminal()` 与返回类型检查都用它。

</details>

<details>
<summary><b>🔗 关联</b></summary>

- [#108](issue-108.md)（§31）：解构、`for`、窄化的延后任务（含 `if let (a, b) = t` 可选元组解构，那条是**未实现**的真实待办，与本节无关）。
- [#119](issue-119.md)（§32）：`parse_tuple()` 与期望类型，tuple 解构的类型侧前提。

</details>