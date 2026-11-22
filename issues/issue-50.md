# 泛型一元运算符协议分发（`__neg__` / `__bitnot__`）

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 语义层直接拒绝泛型类型的一元运算

> 来源：`docs/Koala_TODO.md` §2

---

### 详细描述

<details>
<summary><b>🔍 现状</b></summary>

`-a`（`T : Arithmetic`）报 `error: unary '-' operator requires int or float type.`

`~a`（`T : BitwiseOperators`）报 `error: unary '~' operator requires int type.`

语义层**直接拒绝**泛型类型的一元运算。

</details>

<details>
<summary><b>🧠 根因</b></summary>

`OP_UNARY_*` 目前只是 **IR 层伪指令**，没有对应的协议下降路径与字节码族（对标二元的 `OP_NUM_*`，需要一元版本）。

</details>

---

### 修复方案

<details>
<summary><b>🛠 施工方向</b></summary>

1. 语义层放开 trait 约束下的一元 `-` / `~`。
2. 新增一元协议指令与对应 `OP_NUM_NEG` / `OP_NUM_BITNOT`（或归入同一族）。
3. isel 与 VM handler 同步。

</details>

<details>
<summary><b>🔗 依赖关系</b></summary>

运行时部分依赖 §1（VM handler 族）—— 但 §1 已于 2026-11-06 全部 closed（16 条 TARGET 齐备），故本条的阻塞已解除。

</details>