# 泛型复合赋值（`+=` / `-=` / `*=` / `/=` / `%=` 及位运算复合赋值）

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 泛型场景不走 `__i*__` 协议下降

> 来源：`docs/Koala_TODO.md` §3

---

### 详细描述

<details>
<summary><b>🔍 现状</b></summary>

`var s = a; s += b`（`T : Arithmetic`）报 `error: inplace assignment is not supported for 'T' type.`

</details>

<details>
<summary><b>🧠 根因</b></summary>

复合赋值在**泛型场景**没有走 `__iadd__` 等协议方法的下降路径（**具体类型上的复合赋值工作正常**）。

</details>

---

### 修复方案

<details>
<summary><b>🛠 施工方向</b></summary>

泛型复合赋值下降为对应 `__i*__` 的协议调用，或降级为 `s = s + b` 的协议二元运算 + 赋值。

与 §1（VM handler 族）同步落地 —— §1 已于 2026-11-06 closed，阻塞已解除。

</details>