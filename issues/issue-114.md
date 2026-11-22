# 支配树与 GVN 演化路径（阶段 1 已完成）

← 返回 [问题列表](issues.summary.md)

- [ ] 部分完成 —— 阶段 1（支配树 / DVNT / RPO VN）+ 支配树接入、循环展开、自递归内联已完成；阶段 2/3 与 GVN 接入 koalac 未做

> 来源：`docs/Koala_TODO.md` §103（2026-11-14，阶段 1 已完成）

---

### 详细描述

<details>
<summary><b>📌 模块定位</b></summary>

独立模块，放在 `koala_common`，**不依赖 KLR**，目前尚未接入 koalac。输入是抽象图或最小 SSA 接口（`include/common/gvn.h`），SSA 和非 SSA 流水线都能使用。

每一步都用 `test/test-c` 中的随机程序与同步执行的解释器做**差分验证**：替换目标必须支配使用点，原程序与替换后的程序每个值相同。

</details>

<details>
<summary><b>✅ 已完成（103.1）</b></summary>

- **支配树 / 支配边界**：`src/common/dominance.c`，ctest `c_dominance`。
  - Cooper, Harvey, Kennedy, "A Simple, Fast Dominance Algorithm", SP&E 2001。
  - 在逆后序上迭代求 idom；支配树先序进 / 出编号，`dom_dominates` 为 O(1)；支配边界按论文 Figure 5 计算。
  - 测试：论文 Figure 2/4、菱形、循环、不可达节点、自环、入口回边；另有 2000 个随机图与按定义计算的暴力结果比对。
- **DVNT**：`src/common/gvn_dvnt.c`，ctest `c_gvn`。
  - Briggs, Cooper, Simpson, "Value Numbering", SP&E 27(6), 1997。
  - 悲观算法：沿支配树先序遍历一遍，用作用域哈希表编号；支持交换律、常量折叠、无意义 phi、冗余 phi。
- **RPO 乐观 VN**：`src/common/gvn_rpo.c`，ctest `c_gvn`。
  - L. T. Simpson, "Value-Driven Redundancy Elimination", PhD thesis, Rice 1996；以及上面的 Briggs, Cooper, Simpson 1997。
  - 先按逆后序迭代到不动点，再沿支配树做消除；能证明循环中同步递增的归纳变量相等，以及 `k = phi(a, k)` ≡ a。
  - 测试：5000 个随机 SSA 程序，RPO 替换数 ≥ DVNT（17114 对 16981）；11 个变异全部被测试捕获。

---

### 修复方案

<details>
<summary><b>阶段 2：Predicated GVN（Gargi 2002，LLVM NewGVN 的基础）</b></summary>

K. Gargi, "A Sparse Algorithm for Predicated Global Value Numbering", PLDI 2002。

在**新文件**中实现，以 `gvn_rpo.c` 为起点，按下列顺序逐步增强，每步都要求随机差分测试通过，并且结果不弱于上一步：

1. **边可达性**：乐观地假设边不可达，常量条件的分支只标记一条出边可达；phi 忽略不可达的入边（相当于把 SCCP 合进 GVN）。最小 SSA 接口需要增加分支条件。
2. **谓词推断**：在 `a == b` 的真边所支配的区域里，把 a 和 b 视为同一个值；在 `c` 为真的边上 `c` ≡ true。
3. **phi-of-ops / value-phi**：`phi(a+1, b+1)` ≡ `phi(a, b) + 1`，并发现跨路径的等价。
4. **稀疏工作表**：只重算被改动的值所影响的指令，代替整轮 RPO 迭代；只影响性能，不影响结果。

</details>

<details>
<summary><b>阶段 3：GVN-PRE（可选）</b></summary>

VanDrunen, Hosking, "Value-Based Partial Redundancy Elimination", CC 2004。

在值编号上做部分冗余消除：循环不变量外提，在只有部分路径冗余的计算处补插入。

</details>

<details>
<summary><b>阶段 4：接入 koalac</b></summary>

**未完成**：

- KLR → 最小 SSA 接口的适配；默认流水线是非 SSA 的：
  - 指令结果（`%N`）只定义一次，可以直接使用；
  - `local` 变量会被多次赋值，需要先走 `--ssa`，或者只对单定义的值编号。
- GVN 前后的 IR dump，并补充 lit 测试。

**已完成**（同日）：

- ~~支配树接入 KLR~~：`include/parser/domtree.h` / `src/parser/klr/domtree.c`，只依赖 CFG，不需要 SSA。`KlrBasicBlock.dom_index` 映射到节点编号，CFG 改变后需要重建。`--domtree` 开关（默认关闭，`koala` 和 `koalac` 都支持）在 opt / ssa 之后运行支配树 pass，结果存入 `KlrFunc.domtree`（`kl_domtree`；`klr_func_free_domtree` 释放，CFG 改变后需要重跑）。`--dump=domtree` 只打印 pass 的结果：支配树、支配边界、不可达块和回边；不开 `--domtree` 时不输出。测试为 `test/test-ir/test_domtree.kl`（循环加 if、提前 return、嵌套循环带 break/continue、方法、EaC 第 9 章 B0..B8），以及不开 `--domtree` 时无输出。
- 支配树也供循环识别（回边 = 跳到支配自己的块）、循环展开和递归内联使用（见 [#90](issue-90.md)）。

</details>

<details>
<summary><b>参考（暂不实现，103.5）</b></summary>

- Gulwani, Necula, "A Polynomial-Time Algorithm for Global Value Numbering", SAS 2004：完备的 Herbrand 等价，代价大，仅作理论参考。
- GCC 9 的 RPO-VN（`tree-ssa-sccvn`）：Simpson RPO 加边可达性，只在循环区域内迭代；工程实现可参考。

</details>

<details>
<summary><b>📎 附：与本节同期落地的两个 opt 特性（原记录在阶段 4 内）</b></summary>

**循环完全展开**（已完成）：`src/parser/opt/unroll.c`，`--unroll` 开关（默认关闭，`koala` 和 `koalac` 都支持），需要 `--opt` 和 `--domtree`，否则打印 warning 并跳过。在 opt 之后运行，展开后重跑 opt，最多 4 轮（内层循环先展开，opt 去掉残留循环后外层循环变成最内层，下一轮再展开）。

- 自然循环：头块 H 的回边来自 H 支配的前驱；只处理最内层循环，H 在循环外只有一个前驱 P，循环里定义的值不在循环外使用。
- 计数：退出测试 `branch (cmp %i, C)` 所在块和唯一的 `move %i, (add|sub %i, C)` 所在块都支配所有回边，`%i` 是有符号整数 local，初值是循环前的 `move %i, C`；模拟得到头块进入次数 K（不溢出），K ≤ 16 且 K × 循环指令数 ≤ 256。
- **部分展开**（完全展开条件不满足时）：上界是常量（K > 16）或循环内不被修改、宽度与 `%i` 相同的 local / 参数，比较是 `<` / `<=`（步长 > 0）或 `>` / `>=`（步长 < 0），且 4 × 循环指令数 ≤ 256。按因子 U = 4 复制循环体。
- 性能：`bench/loop-sum.kl`（debug、`KOALA_JIT=0`）从 11.09s 降到 8.31s。

**直接自递归内联**（已完成）：`--recursive-self-inline` 默认关闭，在首次 opt 后逐层展开当前模块的直接自调用，最多八层，然后重跑 opt；**不处理互递归**。`inline_cost.c` 借鉴 GCC 的成本参数、阈值按 Koala 调整：每层预估总量不超过 4096 个 KLR 指令单位与 1024 个基本块；用静态 50/50 分支估算调用概率，逐层相乘，不超过 0.5% 的调用点不展开。fib 目前实际展开七层。

原函数限制为 16 个基本块、32 条指令、两个自调用，并跳过 void、开放泛型、变长参数、被写的参数、PHI 和不支持的终止指令。启用尾调用优化时跳过含尾调用的函数，保留尾调用优化。`--dump=recursive-self-inline-ir` 输出变换后、再次优化前的 IR。需要 opt；`--ssa` 默认关闭 opt。内联保留 traceback 逻辑帧，但真实栈帧减少，**栈溢出阈值可能改变**。

</details>
</details>