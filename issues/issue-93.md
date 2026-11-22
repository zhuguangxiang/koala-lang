# JIT 代码质量（阶段 3，进行中）

← 返回 [问题列表](issues.summary.md)

- [ ] 进行中 —— WB 寄存器缓存与分块代码区已完成；`ucomisd` 内联、阈值调整未做

> 来源：`docs/Koala_TODO.md` §111（2026-11-15，进行中）
>
> 设计见 `src/jit/DESIGN.md` §15。

---

### 详细描述

<details>
<summary><b>✅ 写回（WB）寄存器缓存（已完成）</b></summary>

热点 VM 寄存器的副本放在 RSI/RDI/R8–R10、XMM1–7。标量的写只更新副本并标记为 dirty，到非感知指令、`ret`、跳转边（目标入口不持有该 dirty slot 时）、错误路径之前才把 payload 和 tag 一起写回；**引用总是立即写回**，GC 枚举不变。

数据流（valid / dirty / 待写 tag）由多遍发射求不动点（发射器即转移函数）；自尾调用的函数在入口预装副本；`rc_dst` 就地计算。loop-sum 54 → 30ms（luajit 47）。

开关：`KOALA_JIT_REGCACHE=0` 关闭缓存；`KOALA_JIT_WB=wt` 改为写穿（默认 `wb`）；`KOALA_JIT_CHECK_CACHE=1` 每次命中不脏的副本前比对内存（不符 `ud2`）。详见 DESIGN §15。

- **性能**：`test_float_arith` 190 → 54ms（3.5×）、`test_float_logic` 2.1×、loop-sum / sum / `test_bitwise` ~1.35×；调用密集与 `idiv` 主导的不变。
- **测试**：`test/test-run/test_jit_regcache.kl`（五跑：JIT / 解释 / 阈值 1 / 阈值 1+自检 / trace）；全量 lit 在自检 + 阈值 1 下通过；7 个变异全部捕获，3 个等价变异见 DESIGN §15.7。

</details>

<details>
<summary><b>✅ 分块代码区（已完成）</b></summary>

函数 16 字节对齐顺序分配进 1MB chunk（PROT_NONE 预留），写入只翻涉及的页 RW→RX，W^X 不变；不用 `mremap`（会搬走代码）。代码页数减少 1.8–4.7×，运行时间不变（噪声内）。

`KOALA_JIT_STATS` 增加 `code B bytes, P pages, C chunks`；`KOALA_JIT_CODE_CHUNK=<bytes>` 供测试用。测试 `test_jit_code_arena.kl`，7 个变异全部捕获。设计见 DESIGN §16。

</details>

---

### 修复方案

<details>
<summary><b>🛠 未做 / 撤销</b></summary>

- **内联 `ucomisd`**：浮点比较 / 融合浮点跳转不再调 C helper（PF 处理 NaN）。未做。
- **去冗余 tag 写**：试做后**按用户要求撤销**（收益小，与「内存即真相」的简单性冲突）。
- [ ] **阈值调整**：没有 OSR 时阈值只能是 1（`main` 里的热循环），因此**先做 OSR + 回边计数**（DESIGN §17），再实测决定默认阈值。

</details>