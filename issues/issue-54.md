# OP_SEQ_GET_IMM / OP_SEQ_SET_IMM 负下标语义未定义

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 四个语义问题待定案

> 来源：`docs/Koala_TODO.md` §11

---

### 详细描述

<details>
<summary><b>🔍 现状</b></summary>

`OP_SEQ_GET_IMM` / `OP_SEQ_SET_IMM` 的 IMM 是 signed 8-bit（`-128..127`），但**注释未定义负下标的语义**。

</details>

---

### 修复方案

<details>
<summary><b>❓ 待定案的问题</b></summary>

1. Python 式负下标（`a[-1]` 从尾计数）是否支持？
2. 若支持，是编译期展开（重写为 `len - |imm|`）还是运行时解释（handler 内做 `if idx < 0 { idx += len }`）？
3. bounds check 是否覆盖负索引场景（编译期展开方案下，重写后的索引仍需运行时 bounds check；运行时解释方案下，负索引越界报错语义需明确定义）？
4. 与 `OP_SEQ_GET`（寄存器下标版）的负索引语义是否保持一致？

</details>

<details>
<summary><b>🛠 建议</b></summary>

两个指令的负下标语义**必须一致**；若当前 `OP_SEQ_GET` 运行时 handler 不处理负索引，IMM 版本也应**拒绝负数（编译期报错）**，避免两套语义。

</details>