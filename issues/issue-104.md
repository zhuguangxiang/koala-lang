# JIT v2「Yowie」路线图（阶段 2–5，延后）

← 返回 [问题列表](issues.summary.md)

- [ ] 规划 —— v1 Drop Bear 保持 TValue 镜像；阶段 2–5 整体归入 v2 Yowie

> 来源：`docs/Koala_TODO.md` §100（2026-11-14，规划）

---

### 详细描述

<details>
<summary><b>🏷 代号与作者决定</b></summary>

代号（承接 koala/wombat 的澳洲动物命名，见 `src/jit/DESIGN.md` §1）：v1 = **Drop Bear**（基线），v2 = **Yowie**（优化），v3 = **Bunyip**（进阶）。

**作者决定**：v1 JIT 保持 TValue 镜像结构，**不拆箱、不再物化**。下述性能与进阶方案（原阶段 2–5）整体归入 **v2 Yowie**（v3 Bunyip 承接更进阶项）。

v1 继续在 TValue 模型内做类型覆盖：int64 / uint64 / 窄整型 / float64 / bool **已完成**。v1 设计见 `src/jit/DESIGN.md`。

---

### 修复方案

<details>
<summary><b>阶段 2 — 性能基础</b></summary>

- [ ] **拆箱**：字节码静态 tag 数据流（每个 def 的 tag 由 opcode 确定）。int/bool 留 GPR、float 留 XMM，仅在逃逸点（call 参数、ret、field/global 写、cast、intf 装箱）物化 TValue。
- [ ] **机器 RA**：字节码 CFG 重算 liveness + 线性扫描（复用 `src/parser/backend/liveness.c` 的数据流与带洞区间，见 §105）。
- [ ] **JIT→JIT 直调**：非自递归调用绕过 helper / `kl_call_code`，可 patch 调用点（W^X 双映射）。
- [ ] **peephole**：冗余 tag 写、mov 链。
- 验收：fib/tak/ack/loop-sum 至少 5 倍于解释器；`bench-all.sh` 对标 luajit（开 JIT）；lit 差分绿。
- 风险：**拆箱正确性风险最高**，join 点 tag 不一致需保守回退 boxed；lit + 差分兜底。

</details>

<details>
<summary><b>阶段 3 — 覆盖扩展（按拆箱后模型发射）</b></summary>

- [ ] float64 族（XMM；float16/32/bfloat16 同窄 int 模式，作用于 float64）。注：float/bool 的**基础覆盖已提前在 v1（TValue 镜像）落地**，此处指拆箱后的重新发射。
- [ ] `get_field`/`set_field`、`get_global`/`set_global`。
- [ ] `new`/`build_intern`/float 与 int 互转 cast（C helper）。
- [ ] `OP_CALL` flag=1（itab）/flag=2（callable）；方法资格（owner=TypeObject，经 `tp->module` 解析）。
- 验收：`--jit=force` 下 lit 全套通过；stdlib 热点基本全覆盖。

</details>
<details>
<summary><b>阶段 4 — 工程化</b></summary>

- [ ] CLI `++ --jit=off|on|force`（env 变量保留兼容）。
- [ ] `test/lit.cfg` 透传 `KOALA_JIT*`，支持套件级 JIT=0/1 双跑差分。
- [ ] 回边计数 + 阈值调优；OSR（届时重议哨兵 CallFrame）。
- [ ] 代码缓存回收（LRU / 引用计数，`kl_jit_free` 已就位）。

</details>

<details>
<summary><b>阶段 5 — 可选进阶</b></summary>

- [ ] itab 调用单态 inline cache（guard + 回退解释，静态类型无需 deopt）。
- [ ] 热点内联（字节码级或 klc 保留 IR）；emitter 复用 AOT（`koalac --native`）。
- [ ] aarch64；GC 落地时对接 safepoint（帧约定已兼容：call 点活引用在 `cf->locals`）。

</details>

<details>
<summary><b>已锁定决策</b></summary>

- 哨兵 CallFrame：v1 不加（当前无活路径缺口；OSR / 直调时重议）。
- unary：暂不扩展（见 [#95](issue-95.md)），待语言侧整改后再镜像。
- 输入层 / 后端：字节码 / 手写 x86-64（不变）。
- **v1 不拆箱 / 不物化**（本条核心决定）。

</details>

<details>
<summary><b>关联</b></summary>

- [#95](issue-95.md)（§94 unary 语义未整改）；`src/jit/DESIGN.md`（v1 设计，§5.4 int 类型规则）。
- 阶段 3 的部分内容已提前在 v1 落地，见 [#93](issue-93.md)（§111 JIT 代码质量）。

</details>
</details>