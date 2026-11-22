# trait 值上的运算符：一元运算符与边角情况

← 返回 [问题列表](issues.summary.md)

- [ ] 部分修复（1/4）：除法两条路径已对齐；一元运算符、`uint64.__neg__`、回归测试仍缺

> 来源：`docs/Koala_TODO.md` §63（2026-11-06，to fix）

---

### 详细描述

<details>
<summary><b>✅ 已可用</b></summary>

`intobj.c` 与 `floatobj.c` 注册了运算符，因此 `+ - * / %`、比较以及 `& | ^ << >>` 在 trait 值上可用（如 `Arithmetic[int]`）。

</details>

<details>
<summary><b>1️⃣ 一元运算符编译不了</b></summary>

`-a`、`~b` 用在 trait 值上报 `unary '-' operator requires int or float type.`

见 §2（需要一元协议指令）→ [#50](issue-50.md)。`_int_neg` / `_int_bitnot` / `_float_neg` 从未被执行到。

</details>

<details>
<summary><b>2️⃣ `uint64` 按设计没有 `__neg__`，但 `Arithmetic` 要求它</b></summary>

`uint64` 实现了 `Arithmetic`，而**没有任何诊断**指出缺失的方法（见 §57）。`intobj.c` 注册 `__neg__` 只是因为 `int64` 与 `uint64` 共用方法表；`uint64` 的代码永远走不到它。

</details>

<details>
<summary><b>3️⃣ ✅ 除法两条路径不一致 —— 已修（2026-11-13）</b></summary>

`OP_INT_DIV` / `OP_INT_MOD` / `OP_UINT_DIV` / `OP_UINT_MOD` 现在会报 `division by zero`，并把 `INT64_MIN / -1` 回绕为 `INT64_MIN`（`INT64_MIN % -1` 为 0），与 native 一致；常量零除数成为编译错误（设计文档 §5.1）。

测试：`test-kl/test_div_by_zero_error.kl`、`test-ut/test_div_by_zero.kl`。位移见 §64。

</details>

---

### 修复方案

<details>
<summary><b>4️⃣ 回归测试缺失</b></summary>

trait 值上只有位移有回归测试（`test-run/test_shift_trait.kl`）；算术、比较与 `& | ^` **都没有**。

</details>

<details>
<summary><b>🛠 待办汇总</b></summary>

- 第 1 项：随 [#50](issue-50.md)（一元协议指令）一起落地。
- 第 2 项：让 trait 方法缺失可见（关联 §57 的方法匹配检查）。
- 第 4 项：补齐算术 / 比较 / `& | ^` 在 trait 值上的回归测试。

</details>