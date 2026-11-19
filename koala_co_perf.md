# Koala 协程性能压测报告（M1：N:1 单 P 协作式调度）

> 日期：2026-11-18。实测机器同机对比 Go 1.26.8。
> 基准源码：`bench/co_switch.kl`、`bench/co_idle.kl`、`bench/go/co_switch.go`、`bench/go/co_idle.go`。
> 本报告所有数字均为**本机实测**，非文献引用。

---

## 1. 测试环境

| 项 | 值 |
|---|---|
| CPU | Intel Core Ultra 7 155H（22 逻辑核） |
| 内存 | 31 GB（可用约 20 GB） |
| OS | Linux x86-64 |
| `vm.max_map_count` | **1048576**（本机已调高； stock Linux 默认 65530） |
| Go | 1.26.8 linux/amd64（`go build` 默认优化） |
| Koala | debug（`-O0 -g`）与 release（`-O3 -march=native`）两套均测 |
| 调度模型 | 双方均**单 P**：Koala N:1 单 pthread；Go `GOMAXPROCS(1)` |

**公平性说明**：Go 为优化构建；Koala 主套件在 debug（`-O0`）下运行，故另测 release（`-O3`）以对齐。切换汇编 `fiber_x86_64.S` 为手写最优，外围调度 C 代码在 release 下才优化。

---

## 2. 测量方法

- **切换时间**：两个协程/goroutine 通过 `task.yield_now()` / `runtime.Gosched()` 乒乓，各 `N=2_000_000` 次。`KOALA_CO_STATS` 给出精确切换数（4,000,002）。`ns/switch = elapsed_ns / switches`。含少量循环开销（JIT/release 下可忽略）。
- **内存占用**：spawn N 个**空闲 parked** 协程（阻塞在同一 channel `recv`）/ goroutine（阻塞 `<-ch`），`/usr/bin/time -v` 取峰值 RSS。每协程成本 = (RSS − 基线) / N。
- **最大数量**：递增 N 直到失败，记录失败模式。
- **容量扫描**调高 `KOALA_GC_PARK`（关闭 park 点自动 GC），隔离调度/内存能力，避免 O(N²) 栈扫描干扰。

---

## 3. 切换时间（单 P，4,000,002 次切换）

| 实现 | 构建 | elapsed（中位） | **ns/switch** |
|---|---|---|---|
| **Koala** `yield_now` | release `-O3` | 109.7 ms | **≈ 27 ns**（26.5–32.5） |
| **Koala** `yield_now` | debug `-O0` | 135.2 ms | ≈ 34 ns（33–42） |
| **Koala** `yield_now` | 解释器 `KOALA_JIT=0` | 590 ms | ≈ 148 ns（含逐指令 dispatch） |
| **Go** `runtime.Gosched` | 优化 | 281 ms | **≈ 70 ns**（68–72） |

**结论**：热路径（JIT + release）下 Koala 协作切换 **≈27 ns**，约为 Go `Gosched`（≈70 ns）的 **2.5× 快**。原因：Koala 单 P 协作切换是「手写 asm 保存 6 个 callee-saved 寄存器 + MXCSR/x87 CW → FIFO 链表取下一个 → 切栈」，无锁、无抢占、无 work-stealing；Go `Gosched` 即便单 P 也要走 `runqput → schedule → findrunnable` 等更重的调度路径。

> 注意：这是**让出 CPU 的切换**成本。Koala M1 无抢占（§6.11 延后），CPU 密集且无 park 的协程会饿死他人；Go 自 1.14 起异步抢占。两者语义不同，切换快不等于调度更强。

---

## 4. 内存占用（空闲 parked，峰值 RSS）

基线（仅 main，N=0）：Koala **6.4 MB**，Go **2.4 MB**。

| N | Koala RSS（guard on） | Koala RSS（guard off） | Go RSS |
|---|---|---|---|
| 100,000 | 865 MB | — | 263 MB |
| 300,000 | 2.61 GB | — | — |
| 500,000 | 4.25 GB | 4.25 GB | 1.28 GB |
| 800,000 | — | 6.79 GB | — |
| 1,000,000 | — | 8.49 GB | 2.55 GB |
| 1,040,000 | — | 8.82 GB | — |
| 2,000,000 | — | — | 5.10 GB |
| 4,000,000 | — | — | 10.24 GB |
| 6,000,000 | — | — | 15.31 GB |

**每协程物理成本**（减基线后线性拟合）：

| 实现 | 每协程 RSS | 栈模型 |
|---|---|---|
| **Koala** | **≈ 8.9 KB/G** | 固定 2 MB 保留区（1 MB C 栈 + 1 MB VM 栈），`MAP_NORESERVE` 惰性提交，实际仅触页计费 |
| **Go** | **≈ 2.68 KB/G** | 初始 2 KB **可增长**栈（拷贝扩容）+ `g` 结构 + `sudog` |

**结论**：Go 每空闲 goroutine **≈2.68 KB**，约为 Koala（≈8.9 KB）的 **1/3.3**。差距来自栈模型：Go 栈从 2 KB 起步、按需拷贝增长；Koala 为支持「native C 帧内 park」必须给每个 G 独立的 C 栈 + VM 栈固定区，空闲协程触发的页（VM 帧 + C 帧 + ks/task 结构）约 8–9 KB。Koala 的 2 MB 是**虚拟保留**（100 万 G ≈ 2 TB 地址空间，惰性提交），不是物理占用。

> 优化方向（后续里程碑）：缩小默认 C/VM 栈（`KOALA_CO_CSTACK/VMSTACK`）、空闲 G 栈页回收、或更小的初始 VM 栈，可降低每 G 触页量。

---

## 5. 最大协程数量与限制因素

| 实现 | 实测上限（本机） | 限制因素 | 失败模式 |
|---|---|---|---|
| **Koala guard on（默认）** | **≈ 524,000** | `vm.max_map_count`，**2 VMA/G**（guard page 分裂映射） | `mmap … failed`（524,288 时 ENOMEM） |
| **Koala guard off**（`KOALA_CO_GUARD=0`） | **≈ 1,048,000** | `vm.max_map_count`，**1 VMA/G** | 同上（撞 1,048,576） |
| **Go** | **≥ 6,000,000**（实测，仍可更高） | **物理内存**（栈在堆上分配，无 per-G VMA） | 内存耗尽/OOM |

**关键差异**：
- Koala 的上限是 **VMA 数量**（`vm.max_map_count`），**不是内存**——100 万 G 仅用 8.5 GB（本机可用 20 GB）。调高 sysctl 即可继续提升；按 8.9 KB/G 估算，20 GB 内存理论可容 **≈220 万** G。stock Linux（`max_map_count=65530`）下 guard-on 约 **3.2 万**、guard-off 约 **6.5 万**，需 `sysctl vm.max_map_count=…` 提升。
- Go 的上限是**内存**：goroutine 栈从堆分配，无 per-G mmap/VMA，故可轻松数百万；6M 用 15.3 GB，继续增到内存耗尽为止。

---

## 6. 创建吞吐（spawn + park + 退出回收，端到端）

由容量扫描 wall time 估算（含 200 ms sleep 与退出 abandon，N=1M）：

| 实现 | 1M 端到端 wall | 约每协程 | 主要开销 |
|---|---|---|---|
| Koala | ≈ 7.2 s | ≈ 7 µs/G | 每 G `mmap`+`mprotect`(guard)+`madvise`(回收) 系统调用 + ks/fiber 初始化 |
| Go | ≈ 1.9 s | ≈ 1.7 µs/G | 栈从堆缓存分配，无 per-G 系统调用 |

Go 创建约 **4× 快**：Koala 每个 region 生命周期涉及 3 次系统调用（mmap/mprotect/madvise），Go 栈复用堆内存。region 池（`KOALA_CO_POOL`，默认 256）缓解高频创建/销毁，但百万级一次性创建仍受系统调用主导。

---

## 7. 汇总对比

| 维度 | Koala M1 | Go 1.26 | 优势方 |
|---|---|---|---|
| 让出切换（单 P，热路径） | ≈ 27 ns | ≈ 70 ns | **Koala ~2.5×** |
| 空闲每协程内存 | ≈ 8.9 KB | ≈ 2.68 KB | **Go ~3.3×** |
| 最大数量（本机） | ~524k / ~1.048M（VMA 限） | ≥ 6M（内存限） | **Go** |
| 创建吞吐 | ≈ 7 µs/G | ≈ 1.7 µs/G | **Go ~4×** |
| 栈模型 | 固定 2 MB 惰性区（C+VM） | 2 KB 可增长拷贝栈 | 各有取舍 |
| 抢占 | 无（协作，M1） | 异步抢占 | Go |
| 多核 | N:1 单 P（isolates 规划中） | M:N work-stealing | Go |
| native 内 park | ✅ 任意 C 帧深度 | ✅（cgo 会 pin P） | 相当 |

---

## 8. 分析与取舍

1. **切换更快**：Koala 单 P 协作调度极简（无锁 FIFO + 手写 asm），热路径切换 ≈27 ns 优于 Go ≈70 ns。代价是无抢占、无多核——这是 M1 的明确取舍（§6.11 抢占、Phase 6 isolates 延后）。
2. **内存更费**：固定 C+VM 栈区是「native C 帧内 park」能力的必要成本（Lua 式无 C 栈协程无法跨 C 调用让出）。Go 拷贝栈更省，但需要精确栈图且不能拷贝 C 帧。Koala 用惰性提交把物理成本压到 ~8.9 KB/G，虚拟 2 MB 不计费。
3. **数量上限性质不同**：Koala 撞 **VMA 墙**（可 sysctl 提升，内存其实充裕），Go 撞 **内存墙**。生产部署 Koala 应调高 `vm.max_map_count` 或 `KOALA_CO_GUARD=0`（牺牲 guard page 越界保护，靠 `c_stack_limit` 检查）。
4. **创建更慢**：per-region 系统调用是主因；region 池适合稳态高频 spawn/finish，一次性百万创建仍慢于 Go。

---

## 9. 复现命令

```bash
source env.sh
# 切换（debug）
KOALA_CO_STATS=1 build/debug/bin/koala bench/co_switch.kl
# 切换（release）
(cd build/release && cmake -G Ninja ../.. -DCMAKE_BUILD_TYPE=release -DSKIP_TESTS=ON && ninja koala_out koalac_out)
LD_LIBRARY_PATH=build/release/lib PATH=build/release/bin:$PATH \
  KOALA_CO_STATS=1 koala bench/co_switch.kl
# 内存/数量（改 bench/co_idle.kl 的 const N）
KOALA_CO_STATS=1 KOALA_GC_PARK=1000000000 /usr/bin/time -v build/debug/bin/koala bench/co_idle.kl
KOALA_CO_GUARD=0 …（guard off）
# Go 对比
go run bench/go/co_switch.go
/usr/bin/time -v go run bench/go/co_idle.go
```

---

## 10. 注意事项与边界

- Koala 主数据为 **debug `-O0`**；release 切换 ≈27 ns（更优），内存不变（页粒度）。Go 为优化构建。
- 双方均**单 P** 对比（Go `GOMAXPROCS(1)`）；Go 多核 M:N 能力未在此体现。
- 切换数含乒乓循环开销，JIT/release 下可忽略；解释器模式（≈148 ns）含逐指令 dispatch，不代表纯切换成本。
- 本机 `vm.max_map_count=1048576` 已调高；stock 默认 65530 会大幅降低 Koala 上限（见 §5）。
- 空闲内存为「parked 在 channel recv」场景；运行中协程触页更多，成本随栈深上升。
- Go 数字为本机实测，随版本/硬件变化；非官方文献值。

---

## 11. 附：JIT 计算性能对比（Koala 全优化 vs Go AOT）

> 本质差异：**Go 是 AOT 原生编译**（构建期 SSA + 内联 + 逃逸分析 + 寄存器分配，无运行时 JIT）；**Koala 是解释器 + baseline JIT**（热点阈值默认 1，首次热调用即编译）。公平对比 = 「Koala JIT 热点原生码 + 全部编译优化」vs「Go AOT 优化原生码」，同机、单线程、相同算法与规模。

### 11.1 全优化配置

- **运行时**：release 构建（`-O3 -march=native`），JIT 默认开启（`KOALA_JIT_THRESHOLD=1`）。
- **编译优化**：默认已含 `inline` + `kl_optimize`（const-copy-prop / LVN / CFG 分支折叠 / DCE）+ `fusion` + `tail-call`；额外开启 **`--domtree --unroll --recursive-self-inline`**（按用户要求**不含 `--ssa`**）。
- **隔离执行时间**：先 `koala -c … -o x.klc` 预编译，再重复运行 `.klc`（排除每次编译开销）；Go 预编译二进制。各取中位数（JIT/Go 5 次，解释器 3 次）。
- **正确性**：5 个基准 Koala 全优化输出与 Go **逐一相等**（fib=102334155、ack=8189、tak=9、loop-sum=sum=5000000050000000）。
- **启动基线**（noop 程序）：Koala ≈ **8.8 ms**，Go ≈ **4.7 ms**。下表同时给出原始 wall 与扣除启动的纯计算时间。

### 11.2 原始 wall time（ms，含启动，中位数）

| 基准（规模） | 解释器 | JIT 默认 | **JIT 全优化** | Go AOT |
|---|---|---|---|---|
| fib(40) 二叉递归 | 3730.6 | 822.2 | **278.5** | 359.2 |
| ack(3,10) 深递归 | 710.0 | 253.9 | **91.7** | 199.2 |
| tak(32,16,8) 三重递归 | 574.4 | 142.8 | **72.8** | 57.6 |
| loop-sum(1e8) for 循环 | 290.8 | 35.4 | **32.5** | 28.7 |
| sum(1e8) 尾递归→TCO 循环 | 593.4 | 78.0 | **36.6** | 32.8 |

### 11.3 扣除启动后的纯计算时间（ms）与 Koala/Go 比

| 基准 | JIT 全优化 | Go | **Koala/Go**（>1 Koala 更快） |
|---|---|---|---|
| fib(40) | 269.7 | 354.5 | **1.31×（Koala 快）** |
| ack(3,10) | 82.9 | 194.5 | **2.35×（Koala 快）** |
| tak(32,16,8) | 64.0 | 52.9 | 0.83（Go 快 ~1.2×） |
| loop-sum(1e8) | 23.7 | 24.0 | ≈ 1.01（持平） |
| sum(1e8) | 27.8 | 28.1 | ≈ 1.01（持平） |

### 11.4 优化收益

| 基准 | 解释器→JIT 全优化 | JIT 默认→全优化 |
|---|---|---|
| fib(40) | 13.4× | **2.95×** |
| ack(3,10) | 7.7× | **2.77×** |
| tak(32,16,8) | 7.9× | 1.96× |
| loop-sum(1e8) | 8.9× | 1.09× |
| sum(1e8) | 16.2× | **2.13×** |

### 11.5 分析

1. **全优化对自递归收益巨大**：`--recursive-self-inline`（+`--unroll`）把 fib/ack/sum 的自递归部分内联/展开，相对默认 JIT 提速 **2–3×**；紧循环 loop-sum 仅 1.09×（已无多少可优化）。
2. **JIT 相对解释器 8–16×**：baseline JIT 消除逐指令 dispatch，递归与循环均大幅受益。
3. **对比 Go**：
   - **递归密集（fib/ack）Koala 反而更快**（1.3× / 2.3×）——recursive-self-inline 削减调用开销，Go 递归无 TCO、每层真实 call。
   - **紧循环（loop-sum/sum）基本持平**——双方都生成紧凑原生循环；原始 wall Go 略快主要因启动低 4ms，扣除后持平。
   - **tak Go 略快 ~1.2×**——三重嵌套递归 Go 的优化寄存器分配/内联稍优。
4. **启动开销**：Koala 8.8ms vs Go 4.7ms，短任务占比明显；长计算可忽略。

### 11.6 caveat

- Koala 为 release 运行时 + baseline JIT + 全编译优化；Go 为 AOT 优化。两者优化深度不同：Go 有逃逸分析/栈增长/更成熟 regalloc，Koala baseline JIT 以模板式快速编译为主，靠 recursive-self-inline/unroll 在递归上扳回。
- `sum` 的 Go 版写为**迭代**（Go 无尾调用优化，直接尾递归 1e8 层会栈溢出）；Koala 尾递归经 `--tail-call`+recursive-self-inline 优化为循环——对比的是双方优化后的循环形态，公平。
- 单线程微基准，不含协程调度；真实负载（分配、GC、I/O、多核）差异另计。
- 原始 wall 含进程启动；纯计算时间已扣除 noop 基线（Koala 8.8ms / Go 4.7ms）。

### 11.7 复现

```bash
# Go 基准：bench/go/{fib,ack,tak,loop_sum,sum}.go
go build -o /tmp/gobench_fib bench/go/fib.go   # 其余同理
# Koala 全优化预编译 + 运行（release）
export PATH=build/release/bin:$PATH LD_LIBRARY_PATH=build/release/lib
koala -c --domtree --unroll --recursive-self-inline -o /tmp/fib.klc bench/fib.kl
koala /tmp/fib.klc            # JIT 全优化
KOALA_JIT=0 koala /tmp/fib.klc  # 解释器对照
```

---

## 12. 平衡画像：Go 主场基准（分配 / GC / map / 字符串，单线程）

> §11 的递归微基准是 Koala 主场。本节补上 Go 擅长的负载，**全部单线程**（Koala 尚不支持多核，故不做多核对比），算法与规模同 Go 一致，输出逐一相等。配置同 §11（release 运行时 + JIT + `--domtree --unroll --recursive-self-inline`）。

### 12.1 基准

| 基准 | 负载 | 规模 |
|---|---|---|
| alloc_list | list 动态增长 append + 索引求和 | 2,000,000 |
| alloc_obj | 保留 N 个 GC 对象（Point）+ 字段求和 | 1,000,000 |
| map_ops | dict/map 插入 N + 查找求和 | 1,000,000 |
| str_concat | 不可变字符串循环拼接（O(N²) 拷贝） | 20,000 |

### 12.2 原始 wall time（ms，含启动，中位数）

| 基准 | 解释器 | JIT 默认 | JIT 全优化 | Go AOT |
|---|---|---|---|---|
| alloc_list | 78 | 62 | 63 | **36** |
| alloc_obj | 102 | 89 | 88 | **75** |
| map_ops | ~390 | ~383 | ~387 | **134** |
| str_concat | 169 | 166 | 166 | **31** |

### 12.3 扣除启动后纯计算（ms）与 Go/Koala 比

| 基准 | JIT 全优化 | Go | **Go 快** |
|---|---|---|---|
| alloc_list | 54 | 32 | **~1.7×** |
| alloc_obj | 80 | 71 | **~1.1×** |
| map_ops | 378 | 129 | **~2.9×** |
| str_concat | 158 | 26 | **~6.1×** |

### 12.4 发现

1. **Go 全胜**（1.1×–6×），符合预期：这些是 Go 成熟运行时的主场——优化过的 `mallocgc`/`memmove`、**Go 1.24+ 的 swiss-table map**、逃逸分析栈分配、并发 GC。
2. **JIT 在这类负载收益甚微**（≈1.0–1.3× vs 解释器）：append/索引/dict/字符串拼接都是 **native C 调用主导**，循环本身被 JIT 编译但真正的工作在 native 方法里，bytecode 优化空间小。map_ops 经多次采样确认 **JIT ≈ 解释器**（差异在噪声内），并非 JIT 更慢。
3. **map 差距最大之一（~2.9×）**：Go swiss-table vs Koala `hashmap.c` 开放寻址；字符串拼接 ~6× 来自 Go 优化的分配+拷贝路径。
4. **对象分配差距最小（~1.1×）**：双方都在堆上建 N 个对象，Koala GC 对象分配与 Go 逃逸堆分配接近。

### 12.5 重要 GC 缺口（str_concat OOM）

`str_concat` 在 **N=50000**（churn ≈1.25 GB）时：**Koala 默认 1 GB GC 上限 panic**（`gc: out of memory`），需 `KOALA_GC_MAX=4G` 才能完成；**Go 并发 GC 自动回收，轻松完成**。

根因：**M1 尚无分配触发 GC（§8.4b 未实现）**，垃圾只在 park 点或显式 `gc.collect()` 回收。纯 churn 且无 park 的循环会一路涨到 GC 上限。这是当前实现与 Go 的**真实能力差距**，也是后续里程碑（分配触发 GC + native shadow-stack 审计）要解决的核心问题。本节计时用 N=20000（~200 MB < 1 GB）使双方无需回收即可完成，测纯拼接速度。

### 12.6 综合结论（递归 + 数据结构两面）

| 负载类别 | 优势方 | 幅度 |
|---|---|---|
| 整数自递归（fib/ack） | **Koala** | 1.3×–2.3× |
| 紧循环（loop-sum/sum） | 持平 | ~1× |
| 嵌套递归（tak） | Go | ~1.2× |
| 切片增长/索引 | Go | ~1.7× |
| GC 对象分配 | Go | ~1.1× |
| hash map | Go | ~2.9× |
| 字符串拼接 | Go | ~6× |
| churn GC 承压 | Go（Koala 会 OOM） | 能力差距 |

**客观画像**：Koala 全优化 JIT 在**计算密集自递归**上胜过 Go AOT，紧循环持平；但在**分配/GC/集合/字符串**等运行时密集负载上落后 Go 1.1×–6×，且 M1 缺分配触发 GC 导致 churn 场景 OOM。Koala 的亮点是优化器（recursive-self-inline）与极简调度，短板是运行时成熟度（GC、map、string）——这与 Go 二十年运行时打磨的差距一致，也是后续里程碑的方向。

### 12.7 caveat 与复现

- 单线程微基准；Go map 为 1.26 swiss-table，Koala dict 为 `hashmap.c`；字符串双方均 O(N²) 不可变拼接。
- 原始 wall 含启动（Koala 8.8ms / Go 4.7ms），纯计算已扣除。
- map_ops 数值为多次采样稳定值（JIT≈解释器）。

```bash
# Go：bench/go/{alloc_list,alloc_obj,map_ops,str_concat}.go
go build -o /tmp/gobench_map_ops bench/go/map_ops.go
# Koala 全优化
koala -c --domtree --unroll --recursive-self-inline -o /tmp/m.klc bench/map_ops.kl
koala /tmp/m.klc
# GC 缺口复现（bench/str_concat.kl 改 N=50000）
koala /tmp/str50k.klc            # 默认 1G → panic
KOALA_GC_MAX=4G koala /tmp/str50k.klc  # 提高上限 → 完成
```




