# 13. JIT 模式下 GC 覆盖不足

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 缺少不会被 JIT 优化掉的 GC 压测

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

`test_gc_*.kl` 去掉 `--no-jit` 全过，但**耗时极短**，churn 循环可能被 JIT 优化掉，JIT 帧 / 直调路径下的 root 正确性**没有被真正压测**。

</details>

---

### 修复方案

<details>
<summary><b>🛠 待办</b></summary>

设计不会被优化掉的 JIT 场景 GC 测试（如**自递归直调深处触发 GC**）。

</details>