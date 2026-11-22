# `parse_binary` 的具名常量通道缺失（to fix）

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 两个 gap 已作为 error 记录在 `test/test-kl/test_number_errors.kl`

> 来源：`docs/Koala_TODO.md` §17（2026-10-03 建立，2026-11-07 重写）
>
> `promote_integer_type` 已被移除，随之删掉了此前记录在此的错误诊断与错误日志。修复方案见设计文档 **§5.4**。

---

### 详细描述

<details>
<summary><b>🔍 两个通道</b></summary>

`parse_binary` 有两条把 `lhs` 的类型传给 `rhs` 的通道：

- **Literal**：`rhs` 是字面量且 `lhs` 是 `uint64`。
- **Named constant**：`rhs` 是常量且 `lhs` 不是 `uint64`；`rhs` 被重新解析一次。

</details>

<details>
<summary><b>❌ 由此留下的两个 gap</b></summary>

1. `x + UINT8_MAX`，`x` 为 `uint64`：两条通道都不适用，常量保持 `int64`，`uint64.__add__` 的实参不匹配 —— 而 `x + 255` 合法。
2. `n + INT16_MAX`，`n` 为 `int8`：常量被读成 `int8` 并报越界 —— 而 `n + 32767` 合法。右操作数的类型只是**提示**，这里过于严格。

</details>

---

### 修复方案

<details>
<summary><b>🛠 依据设计文档 §5.4</b></summary>

原记录的方案在重写时已被删除（`promote_integer_type` 一并移除）。当前唯一明确的落点是设计文档 **§5.4**：

- 两个 gap 均已固化为 `test/test-kl/test_number_errors.kl` 中的 **error 用例**，即测试**锁定的是错误行为**。
- 修复时需同时反转这两条测试用例的期望。

</details>