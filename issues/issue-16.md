# 16. list 构造改为 `new` + `__init__` 的额外开销未量化

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 烟测已过，吞吐/延迟基准未做

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

每个 list 字面量现在比原来多一次方法 dispatch（查 `list::__init__` 的 import slot）和一个 self slot：

- IR 值 **+2**
- LIR 指令 **+5**
- 字节码多 `new` / `call` **两条**

</details>

<details>
<summary><b>✅ 已做</b></summary>

20 万次嵌套 list/tuple 构造的 **GC 压力烟测**，结果正确、无崩溃、无 valgrind 新增报错。

</details>

<details>
<summary><b>⏸ 未做</b></summary>

- 没有跑吞吐/延迟基准，**无法给出相对 `build_intern` 的开销百分比**。
- 没有确认 JIT 下这次 dispatch 能否被内联或消除。

</details>

---

### 修复方案

<details>
<summary><b>🛠 待办</b></summary>

补一个**不会被 JIT 优化掉**的 list 构造基准（参考 [#13](issue-13.md) 的思路），对比改动前后。

若开销显著，考虑让 isel 把「`new` 紧跟 `__init__`」**重新折叠成单条指令**。

</details>

<details>
<summary><b>🕐 记录状态</b></summary>

2026-11-21 记录，待后续处理。

</details>