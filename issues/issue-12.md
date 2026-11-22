# 12. GC 扫描 VM 栈时可能把残留值当 root

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 候选修法倾向 B，构造用例未能复现

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

`stack_base`（`vm.c:51`，`aligned_alloc`）**未初始化**，`_new_frame`（`eval.c:250`）与 JIT 直调建帧都不清零新帧 locals。

函数返回后其槽位落在扫描上界外，引用的对象被回收；之后新帧复用这些槽位，**未写过的寄存器残留悬空指针**，若此时触发 GC 会 mark 已释放内存。

valgrind 报的 `gc_mark_roots`（`gc.c:250`）`uninitialised value` 即扫到从未写过的栈槽。

</details>

<details>
<summary><b>📐 现有设计文档的建议不充分</b></summary>

`src/jit/DESIGN.md` §14.1 规则 3 推荐的「每次按规则 2 的上界扫」**不充分**，因为上界随调用深度伸缩。

</details>

<details>
<summary><b>🧪 复现状态</b></summary>

构造用例**未能复现**悬空指针（解释器 / JIT 下 valgrind 0 错误）。

</details>

---

### 修复方案

<details>
<summary><b>🛠 候选修法（倾向 B）</b></summary>

- **A**：`stack_base` 初始化为 nil，`_new_frame` 与 JIT 直调建帧时清零 locals。
- **B**：整段栈初始化为 nil，GC 以**历史最高水位**为扫描上界（不改 JIT，略保守）。

**倾向 B。**

</details>