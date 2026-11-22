# Shift 语义的遗留项

← 返回 [问题列表](issues.summary.md)

- [ ] 部分完成 —— 主体语义已实现（2026-11-06）；5 项遗留

> 来源：`docs/Koala_TODO.md` §64（2026-11-06，implemented; leftovers to do）

---

### 详细描述

<details>
<summary><b>✅ 已定案：走 Rust 路线</b></summary>

位移前整数被提升为 int64 / uint64，所以宽度是 **64**。

| 计数来源 | 检查方 | 结果 |
|---|---|---|
| 字面量或 `const`，负数或 ≥ 64 | parser（`check_shift_count`） | error |
| 字面量或 `const` 为 0 | parser | warning |
| 传播后的常量，负数或 ≥ 64 | `const_copy_prop.c` | error |
| 运行期才知道，负数或 ≥ 64 | VM（`kl_check_shift_count`） | panic |

只对内建整数检查：类可以给自己的 `__shl__` 任意含义。

测试：`test-kl/test_shift_count_const.kl`、`test-kl/test_shift_count_fold.kl`、`test-run/test_shift.kl`、`test-run/test_shift_trait.kl`、`test-run/test_shift_traceback.kl`、`test-ut/test_shift_panic.kl`。

</details>

---

### 修复方案

<details>
<summary><b>🛠 遗留 5 项</b></summary>

- 语言手册还没有描述这套语义。
- 没有测试表明类的 `__shl__` **不受检查**（`stream << 100`）。
- `let n = 0; x << n` 不报 warning：优化器只检查 error，因为字面量 0 会被报两次。
- 不加 `--opt` 时，只有在传播后才变坏的计数常量**不算错**。isel 依赖优化器，所以这种计数会被编进 `_IMM` 指令，而该指令没有检查。`koala` 总是跑优化器；单独用 `koalac` 且不加 `--opt` 到不了 isel。
- 常量 `int8(1) << 40`（两侧都是常量、左侧窄类型）没有专门测试。

</details>

<details>
<summary><b>🔗 关联</b></summary>

除法在 trait 值上的对齐见 [#75](issue-75.md)（§63 第 3 项）。

</details>