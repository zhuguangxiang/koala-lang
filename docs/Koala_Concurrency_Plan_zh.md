# Koala 并发计划：Fiber、调度器、GC

> 状态：**提案**，2026-11-17。
> 基于截至该日的代码。文中提到的每个文件和函数都存在于仓库中，除非标注 *(新增)*。
>
> 【译者说明】本文是 `Koala_Concurrency_Plan.md`（英文）的中文翻译，原文件未作任何改动。
> 约定：代码块（C / 汇编 / kl）保留原文不译；散文、标题与表格译为中文；代码标识符、文件名、
> 环境变量名保持原样。章节编号与原文一一对应。

---

## 目录

0. [摘要](#0-摘要)
1. [目标与非目标](#1-目标与非目标)
2. [现有代码给了我们什么](#2-现有代码给了我们什么)
3. [选择模型](#3-选择模型)
4. [架构总览](#4-架构总览)
5. [Phase 1：fiber 与每协程一个 KoalaState](#5-phase-1fiber-与每协程一个-koalastate)
6. [Phase 2：调度器、task、channel、group](#6-phase-2调度器taskchannelgroup)
7. [Phase 3：I/O 集成与阻塞池](#7-phase-3io-集成与阻塞池)
8. [Phase 4：带协程的 stop-the-world GC](#8-phase-4带协程的-stop-the-world-gc)
9. [Phase 5：增量 SATB 标记与后台清扫](#9-phase-5增量-satb-标记与后台清扫)
10. [Phase 6：用 isolate 实现多核](#10-phase-6用-isolate-实现多核)
11. [JIT 影响（逐阶段）](#11-jit-影响逐阶段)
12. [调试与工具](#12-调试与工具)
13. [风险与缓解](#13-风险与缓解)
14. [里程碑与测试](#14-里程碑与测试)
15. [开放问题](#15-开放问题)
16. [附录 A：其他语言怎么做](#附录-a其他语言怎么做)
17. [附录 B：术语表](#附录-b术语表)

---

## 0. 摘要

| 决策 | 选择 |
|---|---|
| 并发单元 | **有栈协程（fiber）**：一个 `KoalaState` 加一块私有栈区 |
| v1 线程数 | **一个** pthread 运行所有 Koala 代码（N:1）。一个小型 pthread 池运行阻塞 C 调用，且绝不触碰 GC 堆 |
| 语法 | **无新关键字**。没有 `async`、`await` 或 `yield`。一切都是 `std/task` 里的库 API |
| 挂起 | 仅发生在 native（C）函数内部：socket I/O、channel 操作、sleep、join、阻塞池调用 |
| 结构化并发 | `with task.group() as g { ... }`，复用现有的 `__enter__`/`__exit__` 协议 |
| 取消 | 待决的 park 返回"cancelled"。native 抛出一个 `Cancelled` 错误，沿现有错误路径 unwind |
| GC，v1 | 现有的 stop-the-world 标记清除。根来自**每一个**协程的 VM 栈 |
| GC，v2 | mutator 线程上的**增量 SATB 标记**（按分配节奏推进、在事件循环空闲时运行）加辅助线程上的**后台清扫** |
| 多核 | **Isolate**：每个 pthread 一套 VM、堆、调度器和事件循环，通过消息传递通信。不计划共享堆上的 M:N |
| 编译器与 JIT 改动 | 直到 GC 写屏障（Phase 5）之前**为零** |

---

## 1. 目标与非目标

### 目标

1. 单进程内数千到数十万个并发网络任务（HTTP、Redis、LLM 客户端、SSE、聊天 agent：即 `libs/koala` 与 `demo/` 中已有的那些工作负载）。
2. **现有同步代码继续工作，且无需改动就变成并发。** 一个调用 `conn.read_line()` 的函数，不应为了在 task 里运行而被重写。
3. 不改动语言文法、KLR IR、优化 pass 或 `.klc` 格式。
4. 从不 spawn task 的单线程程序不减速（`bench/` 不得回归）。
5. 为 GC 和 JIT 写下清晰的不变式（就在本文），使后续工作（unboxed JIT、增量 GC、isolate）不破坏并发。

### 非目标（暂时）

- 在共享同一个堆的多个线程上运行 Koala 字节码。
- 抢占式调度（可选的时间片检查见 §6.11，留待以后）。
- 异步常规文件 I/O（`io_uring` 是后续选项，见 §7.6）。
- 语言层面的 `select` 语句。库版 `task.select` 以后再加，见 §6.8。

---

## 2. 现有代码给了我们什么

下面这些事实驱动了后续的每一个决策。

### 2.1 解释器：每个 Koala 调用都在 C 栈上递归

`OP_CALL`（`include/runtime/vm_ops.h`，`TARGET(OP_CALL)`）调用 `kl_call_code`（`src/runtime/eval.c`），后者调用 `_eval_frame`。所以一条 Koala 调用链同时也是一条 C 帧链。挂起一个协程意味着保住它的 C 帧：要么编译器把函数改写成状态机，要么每个协程拥有自己的 C 栈。本计划采用后者。

### 2.2 `KoalaState` 已经具备协程所需的一切

`include/runtime/vm.h`：

```c
typedef struct _KoalaState {
    CallFrame *cf;          int depth;
    int stack_size;         TValue *stack_base;
    TValue *stack_top;      TValue *stack_limit;   /* read by JIT */
    char *c_stack_limit;                            /* read by JIT */
    CallFrame *cf_cache;
    Object *builtin;        Object *sys;
    Object *exc;
    struct _ThreadState *ts;
} KoalaState;

typedef struct _ThreadState { KoalaState *current; } ThreadState;
extern __thread ThreadState *__ts;
```

VM 栈、帧链、帧缓存、待决异常以及两个栈限制，全都是每 `KoalaState` 一份。`ThreadState.current` 已经表示"本 pthread 上正在运行的 KoalaState"。切换协程就是切换 `__ts->current` 加上机器上下文。

### 2.3 JIT 从不读取线程局部状态

native 签名是 `void fn(TValue *regs, TValue *out, KoalaState *ks)`（`src/jit/DESIGN.md` §4.1）。JIT 到 JIT 直接调用的栈检查读取 `ks->stack_limit` 与 `ks->c_stack_limit`（§5.3，步骤 2）。因为 `ks` 是参数，**只要该协程的 `ks` 持有正确的限制值，JIT 代码就能在任何协程的栈上正确运行。**

### 2.4 native 栈上不持有对象引用

JIT DESIGN §14.1，规则 5：每个活跃值都在 VM 栈的 `cf->locals` 里。native 栈（`rbp`/`rsp`、返回地址、临时槽）不持有根。因此 **GC 从不扫描协程的 C 栈**——而这通常正是垃圾回收运行时里有栈协程最难的部分。

### 2.5 GC 安全点就是调用点

JIT DESIGN §14.1，规则 4：GC 只在 helper 或运行时调用内部运行，此时 `ks->stack_top` 覆盖所有活跃寄存器（self-only 函数在 `emit_call_abs` 中每次 helper 调用前设置它）。**协程只能在 native 调用内部挂起，而那总是一个安全点。** 所以挂起协程的 VM 栈总能用 GC 今天使用的同一边界来扫描。

### 2.6 GC：全局状态、单线程、非移动、手动触发

`src/runtime/gc.c`：

- 所有状态都是文件级 `static`：`gc_normal_list`、`gc_perm_list`、`gc_root_list`、`gc_gray_dq`、计数器、`gc_cur_state`。
- `gc_collect(KoalaState *ks)` 从**一个** `ks` 扫描根（`gc_mark_roots`）。
- 块用 `calloc` 分配（`GCHeader` + 载荷），且**永不移动**。指向堆对象中某个 `TValue` 槽的指针始终有效。
- 回收只在 Koala 代码调用 `gc.collect()` 时运行（`src/modules/gc.c`）。
- 状态机（`GC_MARK_ROOTS → GC_MARK → GC_SWEEP`）以及用作灰栈的 Chase-Lev 双端队列，形态上正适合日后的增量与并行标记。
- 内存模型（Design Overview §12）：显式 `free`、编译器插入的 `free`、`noesc` 逃逸分析是主路径；收集器是安全网。

### 2.7 并发会加剧的已知 GC 隐患

来自 `koala_usage_issues.md`：

- **#11**：native 代码把新对象持有在 C 局部变量里，而这些不是根。今天 GC 只可能发生在 `gc.collect()`，所以很少咬人。一旦某个 native 挂起期间另一个协程能运行，它就会咬人。
- **#12**：旧帧之上的 VM 栈槽可能持有陈旧指针，被后续帧暴露给 GC。每协程一份全新、清零的栈有助于此（§5.5）。

### 2.8 仓库中已有的部件

| 部件 | 位置 | 在本计划中的用途 |
|---|---|---|
| Chase-Lev 工作窃取双端队列，带 TSan 测试 | `include/common/wsdeque.h`、`test/test-c/tsan_wsdeque.sh` | 今天用作 GC 灰栈；日后用于并行标记或 isolate 工作共享 |
| 侵入式链表 | `include/common/list.h` | 就绪队列、注册表、等待链表（单线程，无锁） |
| 环形缓冲 | `include/common/ring.h` | 带缓冲 channel 的存储 |
| 每 `ks` 的 native 根 shadow stack（`kl_gc_protect`） | `include/backup/shadowstack.h` | 为 issue #11 复活它（§8.3） |
| 旧的多线程调度器骨架 | `src/backup/run.c` | 仅作参考；v1 是单线程 |
| 旧的并发 GC（GC 线程、通过守护页陷阱实现 STW、自旋锁分配） | `src/backup/gc.c` | 仅作参考；Phase 5 用不同设计（§9.1） |
| `with` 协议 | 语言手册、`test/test-kl/test_with_*.kl` | `task.group()`、`task.timeout()` |
| `os` 中的非阻塞线索 | `src/modules/os.c` | Phase 3 的管道与 `os.run` |

### 2.9 `wsdeque.c`：评审与状态

`src/common/wsdeque.c` 是 Lê 等人（PPoPP 2013）的 C11 Chase-Lev 双端队列，即 `wsdeque.h` 引用的那篇论文。它逐步符合论文：

| 操作 | 做什么 | 为何正确 |
|---|---|---|
| `wsdq_push` | 存元素（relaxed）→ release 栅栏 → 存 `bottom`（relaxed） | 以 acquire 读 `bottom` 的窃取者也能看到该元素 |
| `wsdq_pop` | 存 `bottom - 1` → **seq_cst 栅栏** → 读 `top`；仅对最后一个元素 CAS `top` | 该栅栏阻止 owner 与窃取者同时拿走最后一个元素 |
| `wsdq_steal` | 读 `top`（acquire）→ seq_cst 栅栏 → 读 `bottom`（acquire）；以 acquire 读 `array`；CAS `top` | 与 `ws_grow` 中扩容数组的 release 存储配对 |
| `ws_grow` | 拷入翻倍数组，release 存储它，**退役**（而非释放）旧数组直到 `wsdq_fini` | 仍在读旧数组的窃取者绝不会触碰已释放内存 |

其他优点：带符号 `int64_t` 索引使空判断（`b == -1` 时的 `t > b`）无需无符号回绕即正确；`top` 与 `bottom` 位于不同缓存行；测试覆盖 FIFO 窃取、LIFO 弹出、从容量 4 扩容、以及 TSan 下 4 个窃取者对一个 owner（`test/test-c/test_wsdeque.c`、`tsan_wsdeque.sh`）。

发现与状态：

| # | 发现 | 状态 |
|---|---|---|
| 1 | 数组来自 `mm_alloc`/`mm_free`。开启 `MM_TRACK_BLOCKS`（所有 debug 构建）时每个块都链入一个**无锁**全局链表（`src/common/mm.c`）。每个 deque 一个 owner 是安全的，这也是测试通过的原因；但并行标记器各自拥有一个 deque 且可能同时扩容，从而破坏链表。`size_t` 字节数还会在约 2^28 槽处通过 `mm_alloc` 的 `int` 参数溢出 | **已完成（2026-11-17）：**`ws_array_new` 改用 `calloc` 配 `size_t` 字节数并在失败时 abort；`wsdq_fini` 改用 `free`。常规与 TSan 下测试均通过 |
| 2 | 每次 `wsdq_pop` 都跑一个 seq_cst 栅栏（x86 上是 `mfence` 或带锁指令，数十周期）。`gc_scan` 每个灰对象弹一次，但没人从 `gc_gray_dq` 窃取，所以栅栏毫无收益 | 计划中：单线程标记改用普通灰栈（§8.7） |
| 3 | 没有**多个 owner 同时扩容各自 deque** 的测试（即发现 1 所指的情形） | **已完成（2026-11-17）：**向 `test_wsdeque.c` 新增两个测试（见下）。它们在 `malloc` 版本下通过；针对旧 `mm_alloc` 版本运行时，`tsan_wsdeque.sh` 会报告 `mm_alloc` 块链表上的数据竞争 |
| 4 | 一次大回收后 deque 从不收缩 | 接受：对 GC 标记栈来说没问题 |

`test/test-c/test_wsdeque.c` 中的测试（共 9 个）：

| 测试 | 检查什么 |
|---|---|
| 1–6 | 空 deque、从空窃取、超过容量扩容、FIFO 窃取、LIFO 弹出、排空后弹出 |
| 7 `stress` | 一个 owner 推入 200,000 个元素并穿插弹出，同时 4 个窃取者窃取；每个元素恰好被取走一次 |
| 8 `multi_owner` *(新增)* | 4 个 owner，各自拥有从容量 4 起步的 deque，各推入 100,000 个元素（每第 7 次推送弹一次），于是 4 个 deque 同时扩容到 2^17，同时 4 个窃取者轮流从所有 deque 窃取；400,000 个元素每个恰好被取走一次，且结束时每个 deque 都空。这正是并行标记的形态（§9.9） |
| 9 `churn` *(新增)* | 4 个线程各自在自己的 deque 上跑 200 轮 init → 推 1,024 → 全弹（检查 LIFO 顺序）→ fini，于是分配与释放从每个线程并发发生 |

任何对 `wsdeque.c` 或其内存分配方式的改动之后，都要跑 `test/test-c/tsan_wsdeque.sh [rounds]`。

它在本计划中的使用位置：

| 计划部分 | 用 `wsdeque`？ | 原因 |
|---|---|---|
| Phase 2 就绪队列 | 否 | 单线程；普通侵入式 `List` |
| Phase 4–5 标记（先 STW，后在 mutator 上增量） | 否 | 无人窃取；普通灰栈（§8.7） |
| Phase 5 并行标记（§9.9） | **是** | 每个标记线程一个 deque；空闲标记器窃取 |
| 阻塞池任务队列（§7.6） | 否 | 多线程添加任务；Chase-Lev 只允许一个添加者。改用互斥量加条件变量队列 |
| Isolate（Phase 6） | 否 | 协程绝不在 isolate 之间移动 |
| 共享堆上的 M:N（不计划，§10.4） | 是 | 每线程就绪队列加窃取，如 Go 与 Tokio |

---

## 3. 选择模型

### 3.1 选项

| 选项 | 对 Koala 意味着什么 |
|---|---|
| A. 共享一个堆的 pthread | 要把 `gc.c`、atom 表、模块表、JIT 计数器与代码 arena、以及每个 native 库都做成线程安全。要加多线程 stop-the-world 握手。要定义 `list`/`dict` 竞争访问时的行为。数年的工作，且带单线程代价（CPython free-threading：数年后约 5–10%）。 |
| B. 无栈协程（`async`/`await`） | 编译器把 async 函数变成带堆帧的状态机。需要新语法、一个 IR pass、`.klc` 改动，以及每个 I/O API 的 async 版本。无法在被 C 调用的回调内部挂起（sort key、`__eq__`、迭代器 `next()`、`__exit__`）。async trait 方法很难。 |
| C. **有栈协程（fiber）** | 每个协程拥有自己的 C 栈和 VM 栈。挂起发生在 C 函数内部。无编译器改动；现有同步 API 变成非阻塞。代价：每架构一份上下文切换、每协程几 KB 到几十 KB 内存、native 代码必须在 park 期间为对象 rooting。 |

### 3.2 为什么选 C

1. §2.3–§2.5 意味着 JIT 和 GC 已经遵循 fiber 所需的规则。
2. 目标 2（现有代码变成并发）只有 C 才可能。
3. Phase 1–4 不需要编译器改动。整个计划中唯一的编译器或 JIT 改动是 Phase 5 的写屏障，而那是任何增量 GC 本来就需要的。
4. 其他运行时也这么做：PHP 8.1 Fiber 在 PHP 的 JIT 旁切换完整栈；Java 虚拟线程运行 JIT 编译的代码；Ruby fiber；Zig 0.16 把 I/O 挪到一个 `Io` 接口后面，使同一份代码既能阻塞运行也能事件化运行（附录 A）。

### 3.3 C 放弃了什么，我们如何回应

| 代价 | 回应 |
|---|---|
| 挂起点不在调用点标注 | 只有 native 能 park。在文档里标注，并在 `.kl` 声明上加 `@parks` 文档标签（§6.12）。linter 可以列出它们。 |
| 每协程内存 | 惰性提交的 `mmap`：实际内存只有被触碰的页（§5.4）。 |
| 每架构一份上下文切换 | x86-64 与 arm64 各约 30 行汇编（§5.2）。JIT 今天仅 x86-64。 |
| native 必须在 park 期间为临时对象 rooting | 每 `ks` 的 shadow stack（§8.3）加 GC 压力模式（§12.4）。 |
| native 帧把协程绑在其线程上 | 对 N:1 无关紧要。用 isolate（§10）时，协程绝不换线程。 |

---

## 4. 架构总览

```
 pthread (isolate) ─────────────────────────────────────────────────────────────
 │ ThreadState (__ts)
 │   current ──► KoalaState (running coroutine)
 │   sched   ──► Scheduler
 │                 runq:     [ks3] → [ks7] → ...        (intrusive list)
 │                 all:      registry of every live KoalaState (GC roots)
 │                 poller:   epoll / kqueue fd + per-fd wait records
 │                 timers:   min-heap of (deadline, wait record)
 │                 offload:  completion eventfd from the blocking pool
 │
 │ KoalaState (one per coroutine)
 │   VM stack, cf chain, cf_cache, exc, shadow stack, stack limits
 │   fiber ──► FiberCtx { saved rsp } + Region (mmap: C stack + VM stack)
 │   task  ──► TaskObject (GC object, visible to Koala code)
 │
 │ GC heap (gc.c): one per isolate, scanned from every ks in sched.all
 └──────────────────────────────────────────────────────────────────────────────
 blocking pool (pthreads) ── runs C jobs only, never touches Koala objects or mm_*
 GC sweeper thread (Phase 5) ── frees white blocks, never runs finalizers
```

主循环形态：**没有独立的调度器 fiber**。`kl_park()` 挑选下一个可运行协程并直接切换过去。若没有可运行的，就在当前栈上轮询 I/O 和定时器，然后切换。每次交接一次切换，而非两次。

---

## 5. Phase 1：fiber 与每协程一个 KoalaState

### 5.1 新文件

| 文件 | 内容 |
|---|---|
| `include/runtime/fiber.h` *(新增)* | `FiberCtx`、`Region`、`kl_fiber_switch`、`kl_fiber_init`、栈区池 API |
| `src/runtime/fiber_x86_64.S` *(新增)* | 上下文切换与入口 trampoline，System V x86-64 |
| `src/runtime/fiber_arm64.S` *(新增)* | AArch64 版同上（当 JIT 或构建需要 arm64 时） |
| `src/runtime/fiber.c` *(新增)* | 栈区分配、池、sanitizer 与 Valgrind 注解 |

### 5.2 上下文切换（x86-64）

只保留 callee-saved 状态：`rbx`、`rbp`、`r12`–`r15`、`rsp`，以及 MXCSR 和 x87 控制字（System V 规定这些跨调用保留）。caller-saved 寄存器已由调用 `kl_fiber_switch` 的 C 代码保存。

```asm
# void kl_fiber_switch(FiberCtx *from, FiberCtx *to)
#   rdi = from, rsi = to; FiberCtx's first field is `void *sp`
    .globl kl_fiber_switch
kl_fiber_switch:
    push    %rbp
    push    %rbx
    push    %r12
    push    %r13
    push    %r14
    push    %r15
    sub     $8, %rsp
    stmxcsr (%rsp)
    fnstcw  4(%rsp)
    mov     %rsp, (%rdi)        # from->sp = rsp
    mov     (%rsi), %rsp        # rsp = to->sp
    ldmxcsr (%rsp)
    fldcw   4(%rsp)
    add     $8, %rsp
    pop     %r15
    pop     %r14
    pop     %r13
    pop     %r12
    pop     %rbx
    pop     %rbp
    ret

# first entry of a new fiber: r12 = entry function, r13 = its argument
    .globl kl_fiber_trampoline
kl_fiber_trampoline:
    mov     %r13, %rdi
    and     $-16, %rsp          # SysV: rsp % 16 == 0 at the call
    call    *%r12
    ud2                         # entry functions never return (see §6.4)
```

`kl_fiber_init(ctx, region, fn, arg)` 构造一个让 `kl_fiber_switch`"返回"进去的帧。`top` 是 C 栈部分 16 字节对齐的高端：

| 地址 | 值 | 弹入 |
|---|---|---|
| `top - 16` | `kl_fiber_trampoline` | `ret` |
| `top - 24` | 0 | `rbp` |
| `top - 32` | 0 | `rbx` |
| `top - 40` | `fn` | `r12` |
| `top - 48` | `arg` | `r13` |
| `top - 56` | 0 | `r14` |
| `top - 64` | 0 | `r15` |
| `top - 72` | MXCSR `0x1F80`，x87 CW `0x037F` | `ldmxcsr` / `fldcw` |

`ctx->sp = top - 72`。`ret` 之后 `rsp = top - 8`，正是被调函数期望的对齐。

为什么不用 `ucontext`：`swapcontext` 每次切换都做一次 `sigprocmask` 系统调用。做第一个原型可以，但别发布它。

### 5.3 栈区布局：每协程一个映射

每个协程得到**一个** `mmap` 栈区，同时容纳两个栈：

```
 low address                                                          high address
 ┌───────┬──────────────────────────────┬─────────────────────────────────────────┐
 │ guard │   C stack (grows down) ◄──── │ ────► VM TValue stack (grows up)         │
 │ page  │ c_stack_limit = lo+G+margin  │ stack_base = mid    stack_limit = hi     │
 └───────┴──────────────────────────────┴─────────────────────────────────────────┘
          ▲ lo + G                       ▲ mid (C stack top; trampoline frame here)
```

- **C 栈在低端，其下是守护页。** 越过 `c_stack_limit` 的 C helper（例如 native 库内部的深递归）会撞上守护页并 fault，正如今天 pthread 的栈那样。
- **VM 栈在高端，无需守护。** 每次增长都对照 `stack_limit` 检查（`kl_call_code` 里的 `vm_frame_fits` 与 JIT 直接调用检查），所以 VM 栈绝不会写过 `hi`。
- C 栈的栈顶是 `mid`，`kl_fiber_init` 把 trampoline 帧放在那里。

**为什么用一个映射：** 每个 `mprotect` 的守护页都会分裂一个映射（VMA）。Linux 默认 `vm.max_map_count` 是 65530。一个栈区加一个守护页花费**每协程 2 个 VMA**，默认上限约 32,000 个协程。VM 栈与 C 栈分开建栈区会花 3–4 个 VMA，把上限减半。要更多协程：

- 调高 `vm.max_map_count`，或
- 设 `KOALA_CO_GUARD=0`：无守护页，1 个 VMA。此时溢出只由 `c_stack_limit` 检查捕获——它在每次 Koala 调用时发生，但在深层 C helper 内部不发生。用余量（§5.4）覆盖最深的 C helper。

### 5.4 尺寸与限制

| 设置 | 默认 | 环境变量 | 备注 |
|---|---|---|---|
| 每协程 C 栈 | 预留 1 MB | `KOALA_CO_CSTACK` | 仅虚拟；物理内存 = 被触碰的页 |
| C 栈余量 | 64 KB | `KOALA_CO_CMARGIN` | 在 `c_stack_limit` 之下留空给 C helper。今天的 `C_STACK_MARGIN`（256 KB，`src/runtime/vm.c`）保留给主线程 |
| 每协程 VM 栈 | 1 MB（64K 槽） | `KOALA_CO_VMSTACK` | 主 `ks` 保留 `MAX_STACK_SIZE` |
| 守护页 | 开 | `KOALA_CO_GUARD` | §5.3 |
| 池大小 | 256 个空闲栈区 | `KOALA_CO_POOL` | §5.5 |

用 `mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE|MAP_STACK, -1, 0)` 映射。10 万协程 × 2 MB 是 200 GB 的*地址空间*：在默认 overcommit 设置的 64 位 Linux 上没问题，但在 `vm.overcommit_memory=2` 下会失败。要写进文档，并保持尺寸可配置。

**冻结默认值之前先测量。** 每个解释执行的 Koala 调用所用的 C 栈（`kl_call_code` + `_eval_frame` 帧）决定了协程内部递归能有多深。用在协程内运行的 `test/test-run/test_interpreter_stack.kl` 与 `test_jit_direct_call_deep.kl` 测量它，然后选一个至少允许 VM 栈所允许递归深度的 C 栈默认值。

### 5.5 栈区池与清零的栈

- 结束的协程把栈区归还空闲链表（最多 `KOALA_CO_POOL`）。超出则 `munmap`。
- 归还池时，对 VM 栈部分和被触碰的 C 栈部分做 `madvise(MADV_DONTNEED)`。物理页被释放，且**回来时是零填充**。
- `TAG_NIL == 0` 且 `nil_value` 全零（`include/runtime/object.h`），所以全新或复用的 VM 栈全是 nil。这就为协程栈解决了 issue #12：GC 永远看不到陈旧指针。
- 对**主** `ks` 也这么做：把 `kl_new_ks` 里的 `aligned_alloc` + `memset` 换成 `mmap`。启动时那 2 MB 的 `memset` 也消失了。

### 5.6 对 `KoalaState` 的改动

```c
typedef struct _KoalaState {
    /* ... existing fields unchanged ... */

    /* (new) coroutine support */
    struct _FiberCtx  ctx;          /* saved rsp while not running */
    struct _Region   *region;       /* NULL for the main ks (runs on the pthread stack) */
    struct _TaskObject *task;       /* owning task, NULL for main */
    struct _Scheduler *sched;
    List   sched_link;              /* run queue or a wait list */
    List   all_link;                /* scheduler registry */
    uint32_t state;                 /* KS_RUNNABLE, KS_RUNNING, KS_PARKED, KS_DEAD */
    uint32_t cancel;                /* cancellation requested (§6.7) */
    uint32_t gc_epoch;              /* last GC cycle that scanned this stack (§9.4) */
    struct _ShadowStack *shadow;    /* native temporary roots (§8.3) */
    struct _WaitRecord *wait;       /* what this ks is parked on */
} KoalaState;
```

新构造函数 `KoalaState *kl_new_co_ks(Region *r)`：

- `stack_base = mid`、`stack_top = stack_base`、`stack_limit = hi`、`stack_size = (hi - mid) / sizeof(TValue)`
- `c_stack_limit = lo + guard + margin`（对协程取代 `pthread_getattr_np`）
- `builtin` 与 `sys` 从主 `ks` 拷贝（模块共享）
- `ts = __ts`

`kl_free_ks` 目前断言 `!ks->cf`。为程序退出时仍挂起的协程（§6.10）增加 `kl_abandon_ks`：它释放栈区和 `cf_cache`，不做该断言。

### 5.7 Sanitizer 与 Valgrind

测试已经用 Valgrind 和 TSan（`test/test-c/tsan_wsdeque.sh`），所以切换必须加注解，否则这些工具会误报：

- ASan：每次切换前后 `__sanitizer_start_switch_fiber` / `__sanitizer_finish_switch_fiber`。
- TSan：`__tsan_create_fiber`、`__tsan_switch_to_fiber`、`__tsan_destroy_fiber`。
- Valgrind：创建栈区时 `VALGRIND_STACK_REGISTER(lo, hi)`，释放时 `VALGRIND_STACK_DEREGISTER`。

把这些包在 `fiber.c` 的 `#ifdef` 后面，使 release 构建零代价。

### 5.8 Phase 1 交付物

`kl_spawn_raw(fn, args, n)` 在一个新 fiber 上运行一个 Koala 函数，并用仅测试用的 `kl_yield()` 切换回来。这足以在调度器存在之前完成 §14 的 M1 测试。

---

## 6. Phase 2：调度器、task、channel、group

### 6.1 调度器结构

`include/runtime/sched.h` *(新增)*、`src/runtime/sched.c` *(新增)*：

```c
typedef struct _Scheduler {
    KoalaState *main;        /* root coroutine: the pthread's original ks */
    List runq;               /* KoalaState.sched_link, FIFO */
    List all;                /* KoalaState.all_link, every live ks */
    int  nlive;
    Poller poller;           /* §7.1 */
    TimerHeap timers;        /* min-heap keyed by deadline */
    int  offload_fd;         /* eventfd: blocking-pool completions (§7.5) */
    List dead;               /* finished ks whose region can be recycled */
    uint64_t switches;       /* stats */
} Scheduler;
```

单线程，所以用 `include/common/list.h` 的普通侵入式链表，而非 `lldq`（后者每次操作都取自旋锁）。

### 6.2 核心 C API

```c
/* make ks runnable; append to runq; never switches */
void kl_ready(KoalaState *ks);

/* suspend the current coroutine until someone calls kl_ready on it.
   returns KL_PARK_OK, KL_PARK_CANCELLED or KL_PARK_TIMEOUT. */
int  kl_park(WaitRecord *w);

/* create a coroutine running fn(args); runnable but not started */
KoalaState *kl_spawn(Object *fn, TValue *args, int nargs);

/* give up the CPU; stay runnable */
void kl_yield(void);
```

`kl_park` 算法：

```
current.state = PARKED; current.wait = w
loop:
    if runq not empty:        next = pop(runq); break
    timeout = timers.empty ? -1 : max(0, timers.min - now)
    if nothing can ever wake anyone (no fds, no timers, no pending offloads):
        report deadlock (§12.2) and exit with error
    poller_wait(timeout)       # wakes coroutines with kl_ready
    fire expired timers        # kl_ready, with result TIMEOUT
switch_to(next)                # kl_fiber_switch(&cur->ctx, &next->ctx); __ts->current = next
on return: recycle regions in sched.dead; return current.wait->result
```

切换规则：

1. `__ts->current` 在 `kl_fiber_switch` **之前**更新，因为新栈上的代码会立刻读 `__ks()`。
2. 切换只发生在 native 函数内部：由 §2.5 那是安全点，所以挂起的 `ks` 有有效的 `stack_top`。
3. **没人写挂起协程的 VM 栈。** 结果放进它的 `WaitRecord`，协程醒来后自己拷进寄存器。Phase 5 依赖这条规则（§9.4）。

### 6.3 等待记录

```c
typedef struct _WaitRecord {
    KoalaState *ks;
    int kind;           /* WAIT_FD, WAIT_TIMER, WAIT_CHAN_SEND, WAIT_CHAN_RECV, WAIT_JOIN, WAIT_OFFLOAD */
    int result;         /* KL_PARK_* */
    TValue value;       /* value handed over (channel recv, join) */
    List link;          /* on the object being waited for */
    TimerNode timer;    /* optional deadline */
} WaitRecord;
```

`WaitRecord` 活在挂起 native 的 C 帧里（在挂起协程的 C 栈上），所以无需分配。它的 `value` 字段持有一个 **GC 必须看到的**引用：等待记录经 `ks->wait` 可达，`gc_mark_roots` 标记 `ks->wait->value`（§8.1）。

### 6.4 协程生命周期

```
kl_spawn ──► RUNNABLE ──(picked)──► RUNNING ──(kl_park)──► PARKED
                ▲                      │                       │
                └──────(kl_ready)──────┴───────────────────────┘
RUNNING ──(entry function returns)──► DEAD ──(region recycled by the next park)──► freed
```

fiber 入口函数 `kl_co_main(KoalaState *ks)`：

1. 调用 `kl_call_code(task->fn, task->args, n)`。
2. 把结果存进 `TaskObject`。若是 `error_value`，把异常从 `ks->exc` 移进 `task->exc`。
3. 唤醒每个 joiner（`kl_ready`）。
4. 设 `state = DEAD`，把 `ks` 从 `sched.all` 移到 `sched.dead`。
5. 切换走且永不返回。协程不能释放自己正在运行的栈，所以另一个协程的下一次 `kl_park` 回收该栈区。

### 6.5 `TaskObject`（GC 对象）

```c
typedef struct _TaskObject {
    OBJECT_HEAD
    KoalaState *ks;        /* NULL once dead and recycled */
    int state;             /* PENDING, RUNNING, DONE, FAILED, CANCELLED */
    Object *fn;            /* until started */
    TValue *args; int nargs;
    TValue result;
    Object *exc;           /* when FAILED */
    List joiners;          /* WaitRecord.link */
    struct _GroupObject *group;
    uint8_t joined;        /* someone saw the result or error */
} TaskObject;
```

`_mark` 标记 `fn`、`args`、`result`、`exc`、`group`。协程活着时，它的 `ks`（在 `sched.all` 中）标记 `ks->task`，所以运行中的 detached task 绝不会被回收。

### 6.6 Koala API：`libs/std/task` *(新增)*

在薄 `@native` 函数之上的可读 `.kl` 声明，遵循 `libs/std` 其余部分的同一约定。

```kl
// task.kl (outline)
pub class Task[T] {
    pub func join() T            // parks until done; re-raises the child's error
    pub func cancel()
    pub func done() bool
}

pub func spawn[T](fn func(...) T, args ...any) Task[T]   // final generic form TBD by the type checker
pub func sleep(d time.Duration)
pub func yield_now()
pub func current() Task[any]?

pub class Channel[T] {
    pub func __init__(cap int = 0)
    pub func send(v T)           // parks while full; error if closed
    pub func recv() T?           // parks while empty; nil when closed and drained
    pub func try_send(v T) bool
    pub func try_recv() T?
    pub func close()
    pub func __iter__() ...      // for v in ch { ... } until closed
}

pub class Group {               // returned by task.group()
    pub func spawn[T](fn func(...) T, args ...any) Task[T]
    pub func __enter__() Group
    pub func __exit__(...)       // join all; on a child error cancel the rest, then re-raise the first
}

pub func group() Group
pub func timeout(d time.Duration) Group   // a Group that cancels its children at the deadline
```

示例：

```kl
import task
import socket

func handle(c socket.Conn) {
    while true {
        let line = c.read_line()     // parks this coroutine only
        if line == nil { break }
        c.write(line!)
    }
}

func main() {
    let ln = socket.listen(":8080")
    with task.group() as g {
        while true {
            g.spawn(handle, ln.accept())
        }
    }
}
```

`spawn` 使 task 可运行并**不切换**就返回：调用者继续运行直到它 park。这使顺序确定、spawn 廉价。

### 6.7 取消

- `task.cancel()` 设 `ks->cancel = 1`。若 task 已挂起，把它从所等待的对象（fd、定时器、channel 队列、join 链表）上移除，并以 `result = KL_PARK_CANCELLED` 使其可运行。
- park 的 native 看到 `KL_PARK_CANCELLED`，通过现有 `raise_exc_*` helper 抛出 `Cancelled` 异常并返回 `error_value`。解释器（`if (is_error(&ret)) goto error;`）和 JIT（`cmp rax, TAG_ERROR`）都传播它。编译器或 JIT 无新东西。
- **边沿触发：** 取消只投递一次。同一协程后续的 park（例如优雅关闭连接的 `__exit__`）正常工作。带截止时间的 group 通过在截止时间再次取消来强制上界。
- 若 task 未挂起，取消在其下一次 park 时投递。没有 park 的 CPU 密集代码在 §6.11 存在之前无法被取消。
- 取消尚未启动的 task 会把它标为 `CANCELLED` 且永不运行。

### 6.8 Channel

- 存储：`cap > 0` 用 `ring.h`。`cap == 0` 是 rendezvous：`send` 把值直接交给等待接收者的 `WaitRecord.value`。
- 等待队列：`sendq` 与 `recvq` 是 `WaitRecord` 链表，FIFO，所以等待公平。
- `close()` 以 `nil` 唤醒所有接收者，以错误唤醒所有发送者。
- `_mark` 标记缓冲的值以及每个排队发送者等待记录的 `value`。
- 以后：`task.select([ch1.recv_op(), ch2.send_op(v), task.after(d)])` 返回完成操作的索引。它是建在同一等待记录上的库函数，无新语法。

### 6.9 Group（结构化并发）

- `Group.__exit__` park 直到每个子任务完成。
- 若某个子任务失败：取消所有兄弟，等待它们，然后在父任务中重新抛出**第一个**错误（后续错误作为 suppressed error 附于其上，供 traceback）。
- 若父任务自己在 `__exit__` 等待时被取消，group 取消其子任务并仍等待它们。group 绝不泄漏运行中的子任务。
- `task.timeout(d)` 是一个 group 加一个在截止时间取消该 group 的定时器。`__exit__` 随后抛出 `Timeout` 而非 `Cancelled`。

### 6.10 程序启动与退出

- `koala_initialize`（`src/runtime/vm.c`）创建 `Scheduler` 并把主 `ks` 注册为根协程。从不 spawn 的程序只付那份初始化。
- `kl_run_main` 像今天一样运行 `main`。`main` 返回时，**程序结束**（如 Go）。若子任务必须先完成，用 group。
- 退出时，仍挂起的协程被放弃而不运行（`kl_abandon_ks`），然后 `koala_finalize` 像现在一样运行。
- 错误：失败且从未被 join 的 task（无 group、没人调 `join`）在其 `TaskObject` 被释放时或程序退出时打印 traceback，如同 `main` 中未捕获的错误（`print_exc_and_free`）。

### 6.11 公平性与可选抢占（以后）

v1 是协作式：协程运行直到 park。CPU 密集循环会饿死其他协程。v1 加 `task.yield_now()`，并在文档中注明该限制。

以后的选项，对不需要它的代码零代价：

- 定时器信号（`setitimer`，例如每 10 ms）设一个 `volatile` 标志 `ks->preempt`。
- 解释器在回边和调用处检查它。JIT 在计划中的 OSR 回边计数器（JIT DESIGN §17）已经递减的地方检查，所以循环里没有额外分支。
- 置位时，检查调用 `kl_yield()`。回边和调用是安全点，所以这对 GC 安全。

### 6.12 标注会 park 的 native

每个能 park 的 `@native` 函数在其 `.kl` 声明里得到一个文档标签：

```kl
/// Reads one line. @parks
@native
pub func read_line() str?
```

`tools/kl-doc.py` 与 `tools/kl-html-doc.py` 展示它，linter 可以列出一个函数里每个 parking 调用。这给读者提供 `await` 本会提供的"这里哪里会挂起？"信息，而无需染色的函数类型。

---

## 7. Phase 3：I/O 集成与阻塞池

### 7.1 轮询器

`src/runtime/poller_epoll.c`、`src/runtime/poller_kqueue.c` *(新增)*，置于一个接口之后：

```c
int  poller_init(Poller *p);
int  poller_wait_fd(int fd, int events, int64_t deadline_ns);   /* parks; returns KL_PARK_* */
void poller_poll(Poller *p, int timeout_ms);                    /* called from kl_park */
void poller_forget(int fd);                                     /* before close() */
```

- 用一次性注册（`EPOLLONESHOT`，或 kqueue 的 `EV_ONESHOT`），使陈旧注册绝不会唤醒错误的协程。
- 每 fd 一张表，把 fd 映射到读、写各自的等待 `WaitRecord`（每 fd 至多一个读者和一个写者；第二个并发读者得到错误，如 Go）。
- socket 上的 `close()` 调用 `poller_forget` 并以错误唤醒任何等待者。

### 7.2 所有 I/O native 的一条规则

```c
/* the pattern every socket native follows */
for (;;) {
    ssize_t n = recv(fd, buf, len, flags);
    if (n >= 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) return n;
    int r = poller_wait_fd(fd, POLL_IN, deadline);
    if (r != KL_PARK_OK) return r == KL_PARK_TIMEOUT ? raise_timeout() : raise_cancelled();
}
```

当不存在其他协程时，`poller_wait_fd` 就在 `epoll_wait` 里等待，行为如同今天的阻塞调用。**一条代码路径同时服务单 task 与并发程序。**

这里的 `buf` 是 Koala `bytes` 对象的数据。它跨 park 保持有效，因为该对象由 native 在 VM 栈上的参数持有（一个根），且 GC 永不移动对象。

### 7.3 `src/modules/socket.c`

| 函数 | 改动 |
|---|---|
| socket 创建 | 总是 `O_NONBLOCK` 与 `FD_CLOEXEC` |
| `_sys_connect` | `EINPROGRESS` → 等可写 → `getsockopt(SO_ERROR)` |
| `_sys_accept` | `accept4(..., SOCK_NONBLOCK \| SOCK_CLOEXEC)`；`EAGAIN` 时等可读 |
| `_sys_send`、`_sys_recv` | §7.2 模式；部分写像今天一样在 `.kl` 层循环 |
| 超时 | `Conn.set_deadline(t)` 存一个截止时间，供该 socket 上每次等待使用 |

`socket` 之上的一切（`libs/koala/http`、`redis`、`openai`、`sse`）无需改动就变成并发。

### 7.4 TLS（`libs/koala/tls`）

用 OpenSSL，socket 处于非阻塞模式。遇到 `SSL_ERROR_WANT_READ` 或 `SSL_ERROR_WANT_WRITE` 时，等待该事件并重试同一个 `SSL_*` 调用。握手同理。

### 7.5 管道、进程、信号、定时器、stdin

- **来自 `os.pipe()` 与 `os.run()` 的管道**：以 `O_NONBLOCK` 创建并走轮询器。`Process.wait()` 在 `pidfd`（Linux 5.3+）或 `SIGCHLD` signalfd 上 park。
- **信号**：一次性注册的 `signalfd`（Linux）或 `EVFILT_SIGNAL`（kqueue）。`os.signal` 处理器作为 task 运行。
- **`time.sleep`**：在定时器上 park。main 与 spawn 的 task 同一路径。
- **继承的 fd（0、1、2 及传入的 fd）**：**绝不**设 `O_NONBLOCK`。它会改变 open file description，而那是与父 shell 共享的。改为用轮询器等就绪，然后做普通阻塞 `read`——对 tty 和管道一旦可读就不会阻塞。stdout 与 stderr 的写保持阻塞。

### 7.6 必须阻塞的调用：offload 池

有些 C 调用无法非阻塞化：`getaddrinfo`、sqlite、常规文件 I/O、readline 的终端处理、某些 native 库。

C API（不暴露给 Koala 代码：池不能运行 Koala 函数）：

```c
/* run job(ctx) on a pool thread; park until it finishes */
int kl_run_blocking(void (*job)(void *ctx), void *ctx);
```

- 池：默认 4 个 pthread（`KOALA_BLOCKING_THREADS`），首次使用时启动。
- 完成时 worker 写调度器的 `eventfd`；轮询器唤醒等待的协程。
- **规则：池线程绝不触碰 GC 堆、`TValue`、`mm_*` 或任何 Koala 对象。** native 在 park 前把输入拷进一个 C 结构（`ctx`），醒来后再构造 Koala 对象。`mm.c` 在 debug 构建跟踪块（`MM_TRACK_BLOCKS`）且非线程安全。
- Koala 对象拥有的 C 句柄（例如连接对象内部的 `sqlite3 *`）可被 worker 使用，因为挂起的协程在其栈上保持该对象存活。两个协程同时使用同一句柄必须串行化：给这类对象一个协程互斥量（一个等待链表，无 pthread 锁）。
- 取消：协程停止等待（`KL_PARK_CANCELLED`），但 job 在 worker 上运行到完成，其结果被丢弃。此时 `ctx` 必须由 job 拥有，而非 C 栈。

用途：

| 库 | 用途 |
|---|---|
| DNS（`getaddrinfo`） | offload |
| `libs/koala/sqlite3` | 每个 `step`/`exec` offload；每连接一个协程互斥量 |
| 常规文件（`src/modules/fs.c`） | 小读写保持阻塞（快）；大文件 `read_all` offload。`io_uring` 以后 |
| `libs/koala/readline` | 优先用由 stdin 就绪驱动的回调 API（`rl_callback_read_char`）；仅在不可行时 offload |

---

## 8. Phase 4：带协程的 stop-the-world GC

### 8.1 来自每个协程的根

把 `size_t gc_collect(KoalaState *ks)` 改为 `size_t gc_collect(void)`。`src/modules/gc.c` 里的 `gc.collect()` native 继续工作。`gc_mark_roots` 变为：

```c
static void gc_mark_roots(void)
{
    /* perm root objects: unchanged */
    list_foreach(hdr, link, &gc_root_list) { ... tp->_mark(obj) ... }

    Scheduler *s = __ts->sched;
    gc_mark(s->main->builtin);
    gc_mark(s->main->sys);

    KoalaState *ks;
    list_foreach(ks, all_link, &s->all) gc_mark_ks(ks);
}

static void gc_mark_ks(KoalaState *ks)
{
    for (CallFrame *cf = ks->cf; cf; cf = cf->back) {
        gc_mark(cf->code);
        gc_mark(cf->module);
    }

    /* same bound as today (JIT DESIGN §14.1 rule 2), valid for parked ks by §2.5 */
    TValue *end = ks->stack_top;
    if (ks->cf && ks->cf->code) end += ks->cf->code->cs.max_call_args;
    for (TValue *v = ks->stack_base; v < end; v++) gc_mark_value(v);

    gc_mark(ks->task);
    if (ks->exc) gc_mark(ks->exc);
    if (ks->wait) gc_mark_value(&ks->wait->value);
    gc_mark_shadow(ks->shadow);           /* §8.3 */
}
```

等待栈区回收的死协程已从 `all` 移除，所以不被扫描。

### 8.2 不变式（必须成立，在 debug 构建中测试）

1. **协程只在 native 函数内部切换**（§6.2，规则 2）。在 `kl_fiber_switch` 的 C 包装里断言调用者是 C 函数（例如每 `ks` 的 `in_native` 深度计数器 > 0）。
2. **对每个挂起的 `ks`，`stack_top` 覆盖所有活跃寄存器。** 由 JIT 规则 4 成立，因为 park 发生在 helper 调用内部。
3. **没人写挂起协程的 VM 栈**（§6.2，规则 3）。
4. **跨 park 或跨分配，没有任何对 Koala 对象的引用仅持有在 C 局部变量里**，除非它被保护（§8.3）。

### 8.3 native 临时根（issue #11）

复活 `include/backup/shadowstack.h` 的 shadow stack。它已经推入一个每 `ks` 的链表（`__ks()->shadow_stacks`），这正是协程所需：

```c
static TValue _os_pipe(TValue *self, TValue *args, int nargs)
{
    Object *r = kl_new_raw_reader(...);
    Object *w = NULL;
    kl_gc_protect(r, w);          /* both scanned until this C scope ends */
    w = kl_new_raw_writer(...);   /* may allocate, park or call back into Koala */
    return obj_value(kl_new_tuple2(r, w));
}
```

与 backup 版本的区别：

- 存槽地址（`void **`）而非值，使后续赋值（`w = ...`）能被看到。
- 去掉每次 push/pop 的 `log_info`（或保留在 `trace` 级别）。
- 字段名 `ks->shadow`（§5.6）。

审计顺序，从最可能在期间 park 或分配的 native 函数开始：`src/modules/os.c`（`run`、`pipe`）、`socket.c`、调用 `__hash__`/`__eq__` 的 `dictobj.c` 与 `hashset.c` 路径，以及构造多个对象的 `libs/koala/*` native。

### 8.4 GC 何时运行

两步，以免 issue #11 变成崩溃：

- **4a：仅在 park 点触发。** 在 `kl_park` 中、切换之前：若 `gc_get_count()` 或 `gc_used()` 超过阈值，`gc_collect()`。会 park 的 native 必须已遵循不变式 4，而压力模式 `KOALA_GC_STRESS=park`（§12.4）在**每次** park 都回收以找出违规。
- **4b：在分配时触发**（`gc_alloc_block`），待 §8.3 审计完成后。`KOALA_GC_STRESS=alloc` 每第 N 次分配回收。

阈值：从 Lua 式节奏起步，当 `gc_used()` 达到上次回收后存活大小的两倍时回收，带一个下限（例如 4 MB）。做成可配置（`KOALA_GC_PAUSE`）。

### 8.5 显式 `free` 与逃逸分析

有栈协程不改写函数体，所以 `free`、编译器插入的 `free` 和 `noesc` 照常工作。`src/parser/opt/` 中逃逸分析的新规则：

- `task.spawn`、`Group.spawn`、`Channel.send`、`try_send` 的实参，以及传给 `spawn` 的 lambda 所捕获的值，**总是逃逸**。编译器绝不自动 free 它们。
- 显式 `free(x)` 一个别的 task 仍能看到的对象是 use-after-free，正如今天单 task 内有别名时一样。并发不创造新种类的 bug，但使其更容易触发。写进文档，并在 debug 构建中给已释放块下毒，使这类读取快速失败。

### 8.6 终结器

`_free` 终结器（`raw_free`、`process_free`、文件关闭）像今天一样在 mutator 线程的清扫期间运行。Phase 5 它们移到一个队列（§9.7）。

### 8.7 灰栈

`gc_gray_dq` 是 `WSDeque`，但标记在 §9.9 之前是单线程的，所以每次 `wsdq_pop` 里的 seq_cst 栅栏是纯代价（§2.9，发现 2）。用一个普通可增长数组取代它：

```c
/* include/runtime/gc.h or private to gc.c (new) */
typedef struct _GrayStack {
    void **items;
    size_t top;
    size_t cap;
} GrayStack;

static inline void gray_push(GrayStack *s, void *p)
{
    if (__builtin_expect(s->top == s->cap, 0)) gray_grow(s);   /* realloc to 2 * cap */
    s->items[s->top++] = p;
}

static inline void *gray_pop(GrayStack *s)
{
    return s->top ? s->items[--s->top] : NULL;
}
```

- 用 `malloc`/`realloc` 分配，而非 `mm_*`（一旦存在清扫线程，理由同 §2.9 发现 1）。
- LIFO 顺序使标记保持深度优先，如今天。
- 保留一个高水位；某轮峰值远低于 `cap` 之后，缩回默认（例如 1024），使一次巨大回收不钉住内存。
- `gc_mark`（push）与 `gc_scan`（pop）是唯一使用者。Phase 5 的增量切片每片弹出有界数量的条目。
- 改动前后**测量**：大堆上的 `GCCycleStats.elapsed_ms`（例如带数百万存活对象的 `test_gc_*.kl`）。预期有收益，因为今天每次 pop 都跑一个完整栅栏；仅当数字吻合才保留改动。
- 并行标记（§9.9）把它换回每标记器一个 `WSDeque`。

---

## 9. Phase 5：增量 SATB 标记与后台清扫

### 9.1 为什么不用旧的并发设计（`src/backup/gc.c`）

backup GC 在 mutator 运行时于一个独立 pthread 上标记。以 Koala 的值表示，这有一个真实隐患：**16 字节 `TValue` 的撕裂读。** interface 或 optional 类型的堆槽可能在标量与引用之间变化（`{tag, payload}`，见 `gc_mark_value` 的 `is_intf` 分支）。JIT 内联存储和 C 结构赋值分别写两个 8 字节半部。标记线程于是可能看到一个引用 tag 配标量 payload，并跟随一个垃圾指针。

让每个堆 `TValue` 存储原子化是可能的（16 字节对齐的 `MOVDQA`/`VMOVDQA` 在支持 AVX 的 CPU 上是原子的；`STP` 在 ARMv8.4 LSE2 下原子），但 C、解释器和 JIT 里的每个存储点都得遵循。那是一个大审计，而在只有一个 mutator 线程时收益甚微。

**所以在 mutator 线程上增量标记，只把清扫放到另一个线程。** 清扫只读不可达块的块头，所以不会撕裂。

### 9.2 状态

```
GC_DONE
  └─► GC_SNAPSHOT     atomic step (short): roots of running ks, globals, perm roots, builtin/sys
        └─► GC_MARK    incremental slices on the mutator thread; parked stacks scanned lazily
              └─► GC_REMARK   atomic step: drain gray stack, scan stacks not yet scanned
                    └─► GC_SWEEP   background thread frees white blocks
                          └─► GC_DONE
```

这扩展 `src/runtime/gc.c` 中现有的 `GCState`。

### 9.3 SATB 写屏障

SATB（snapshot-at-the-beginning，Yuasa 删除屏障）保证：**周期开始时可达的一切，加上周期期间分配的一切，都存活。** 当 mutator 覆写堆中的一个引用时，先给旧值着色：

```c
extern uint8_t gc_marking;          /* 1 during GC_SNAPSHOT..GC_REMARK */

static inline void gc_write(TValue *slot, TValue val)
{
    if (__builtin_expect(gc_marking, 0)) gc_mark_value(slot);   /* shade old value */
    *slot = val;
}

static inline void gc_write_obj(Object **slot, Object *val)
{
    if (__builtin_expect(gc_marking, 0)) gc_mark(*slot);
    *slot = val;
}
```

因为标记在同一线程运行，屏障可以直接灰化旧值。无 SATB 缓冲，无原子操作。

**无需屏障的存储：**
- 写 VM 栈寄存器：解释器的寄存器 move、JIT 的写回寄存器缓存（DESIGN §15）、调用实参。栈是根，SATB 不需要在根上加屏障。
- `OP_GLOBAL_SET`：模块值数组在原子 `GC_SNAPSHOT` 步扫描（§9.4），所以算作根。
- 对周期期间分配对象的初始化存储（它们分配为黑，§9.5）。留着屏障无害；去掉是一项优化。

**需要屏障的存储**（审计清单）：

| 位置 | 点 |
|---|---|
| 解释器 | `OP_SET_FIELD`（`vm_ops.h`） |
| 解释器 → C setter | `OP_SEQ_SET`、`OP_SEQ_SET_IMM`、`OP_SEQ_SET_SLICE` 到达 `listobj.c`、`dictobj.c`、`hashset.c`、`src/objects/collections/*` 的槽函数 |
| C 对象 | 每个把 `TValue` 或 `Object *` 存进已有堆对象的操作：list 增长/插入/删除（移动槽会覆写引用）、dict 插入/删除/resize、异常字段（`excobj.c`）、函数对象捕获（`funcobj.c`）、加载后的模块对象（`modobj.c`）、`bytebuf.c`（仅当它持有引用） |
| native 库 | 存进 Koala 对象的 `libs/std/*` 与 `libs/koala/*` native |
| JIT | 内联 `SET_FIELD`（DESIGN §5.7）：存储前发射 `cmp byte [gc_marking], 0; jne .slow`，`.slow` 调用 `gc_mark_value(slot)` 并跳回 |

用 grep 找 C 点：`src/objects/` 与 `src/modules/` 中对 `->values[`、`fields[`、`->items`、`Storage.values` 的存储以及 `TValue *` 赋值。加一个 debug 检查：在 `GC_MARK` 中，`GC_REMARK` 处的校验 pass 从根重新遍历并断言每个可达块为黑（`KOALA_GC_VERIFY=1`）。缺失屏障表现为白色可达块。

非标记时的代价：每次堆引用存储一次全局字节加载加一个可预测分支。

### 9.4 快照与惰性栈扫描

原子 `GC_SNAPSHOT` 步扫描：
- `gc_root_list`（perm 根对象的 `_mark`，含模块全局）
- `builtin`、`sys`
- **仅运行中协程的**栈（`gc_mark_ks(current)`），然后设 `current->gc_epoch = gc_cycle`

挂起协程**稍后、但在它们能改变之前**扫描：

- `kl_park` 在切换到 `next` 之前：若 `gc_marking && next->gc_epoch != gc_cycle`，调用 `gc_mark_ks(next)` 并设其 epoch。
- 标记切片也扫描一些尚未扫描的挂起栈，使 `GC_REMARK` 几乎无事可做。
- `GC_REMARK` 扫描每个仍未到当前 epoch 的 `ks`。

**为何正确：** SATB 需要恰好是快照时刻的根。挂起协程的栈在协程再次运行之前不变（§6.2 规则 3：没人写它）。所以在它恢复之前的任何时刻扫描，结果都与在快照时扫描相同。周期期间 spawn 的新协程以 `gc_epoch = gc_cycle` 起步（其栈起始为空；函数与实参活在黑色 `TaskObject` 里）。

**停顿：** 原子步花费一个栈加根，无论有多少协程挂起。

### 9.5 周期期间的分配

- `GC_SNAPSHOT..GC_REMARK` 期间：新块分配为**黑**，挂到 `gc_normal_list`（标记在 mutator 线程，无需加锁）。
- `GC_SWEEP` 期间：清扫器拥有 `gc_normal_list`。新块进入独立的 `gc_new_list`（白，age 0），清扫结束时追加到 `gc_normal_list`。
- `gc_used_size` 与 `gc_count` 变为 `_Atomic`，因为清扫器会从它们减去。
- 从 `gc_alloc_block` 移除 `ASSERT(gc_cur_state == GC_DONE)`。

### 9.6 周期期间的显式 `free`

`gc_free` 今天断言 `GC_DONE`。周期期间：

- 在块头设 `GC_FLAG_PENDING_FREE` 位并把块推入 pending 链表。不释放它：它可能在灰栈上，或清扫器正在读它的头。
- 标记把 pending 块视为黑且不扫描（程序说它们死了，扫描会跟随可能也被释放的指针）。
- 清扫器跳过 pending 块。
- 周期之后，mutator 释放 pending 链表。

### 9.7 后台清扫与终结器

- 一个清扫 pthread（随首个周期启动）遍历 `gc_normal_list`：
  - 黑 → 白，`age++`（如今天）
  - 白且无 `_free` → `free()`（libc `free` 线程安全）
  - 白且有 `_free` → 移入**终结队列**，尚不释放
- mutator 在 `kl_park` 空闲时（以非零超时轮询之前）或下一周期开始时清空终结队列：运行 `_free`，然后释放块。
- 因此终结器总在 mutator 线程运行，那里可以安全使用 `mm_*`、关闭 fd、触碰其他状态。
- `gc_hooks.on_free`（`src/modules/gc.c` 的 debug 输出）也从队列调用，在 mutator 线程。

### 9.8 节奏

Lua 式：

- **按分配节奏的切片：** 每分配 `KOALA_GC_STEP` 字节（默认 64 KB）后，做该量 `KOALA_GC_STEPMUL`% 的标记工作（默认 200%）。在下一个安全点运行，而非 `gc_alloc_block` 内部，直到 §8.3 审计完成。
- **空闲切片：** 在 `kl_park` 中，若没有可运行协程且下一个定时器未到期，轮询前标记最多约 1 ms。事件驱动程序于是主要在等 I/O 时回收。
- 当 `gc_used()` 达到上轮结束后存活大小的 `KOALA_GC_PAUSE`%（默认 200%）时启动一轮。

### 9.9 以后：并行标记

若标记在多核机器上成为瓶颈：

- 标记期间让堆 `TValue` 存储 16 字节原子（§9.1）。
- 把灰栈（§8.7）换成每标记线程一个 `WSDeque`。每个标记器 push/pop 自己的 deque；空闲标记器对其他 deque 调 `wsdq_steal`（`WS_ABORT` 重试，`WS_EMPTY` 试下一个受害者）。`malloc` 修复与 multi-owner 测试（§2.9，发现 1 和 3）已就位；`test_wsdeque.c` 的 `multi_owner` 正是演练这个模式。
- 终止：发现所有 deque 都空的标记器递减一个原子"活跃标记器"计数并持续检查；计数归零且所有 deque 空时标记结束。
- 把屏障改为推入每线程 SATB 缓冲。

首个增量 GC 不需要这些。

---

## 10. Phase 6：用 isolate 实现多核

### 10.1 模型

- `isolate.spawn("worker.kl", args...)` 启动一个运行自己 VM 的 pthread：自己的 GC 堆、调度器、事件循环和模块实例。
- isolate 之间**不共享可变 Koala 对象**。它们通过跨 isolate channel 通信，携带值的深拷贝，或不可变的 frozen 对象。
- 单个 isolate 内部，Phase 1–5 的一切原样适用。

### 10.2 重构：把全局状态移入每 isolate 结构

| 状态 | 今天 | isolate 设计 |
|---|---|---|
| GC 链表、deque、计数器、状态 | `src/runtime/gc.c` 中的 `static` | `GCHeap` 结构，经 `__ts->heap` 访问 |
| 调度器、轮询器、定时器 | *(新增)* | `__ts->sched` |
| 模块表 | `kl_init_module_stbl` / `src/runtime/symtbl.c` | 每 isolate：各自加载自己的模块（如 JS worker）。简单且安全 |
| atom 表 | `src/common/atom.c` | 共享加读写锁，或每 isolate |
| 静态类型（`kl_fini_static_types`） | 全局 | 共享只读，前提是检查它们不持有每运行可变缓存 |
| 代码对象与 JIT 代码 | 每模块 | 当模块来自同一 `.klc` 时可共享：在锁下编译，chunked 代码 arena（DESIGN §16）每 isolate 或加锁。`jit_hotness` 竞争无害 |
| 日志器 | `src/common/log.c` | 做成线程安全或每 isolate |
| `mm_*` 块跟踪（debug） | 全局 | debug 构建加锁 |

### 10.3 跨 isolate channel

- 允许的值：数字、bool、`str`、`bytes`、`nil`、它们的 tuple 与容器（深拷贝），以及 frozen 对象（共享、绝不拷贝，待 frozen 家族存在后）。
- 实现：一个互斥量保护的队列加每个接收 isolate 一个 `eventfd`，使接收者的轮询器唤醒挂起协程。发送绝不阻塞发送者线程：若队列满，发送协程在自己 isolate 的轮询器上 park。

### 10.4 为什么不在一个共享堆上做 M:N

它需要 §3.1 选项 A 的全部：线程安全分配、多线程 stop-the-world 握手或带原子 `TValue` 存储的完全并发标记（§9.1）、`list` 与 `dict` 的锁或定义好的竞争、线程安全 native 库。协程也不能在 native 帧内部跨线程移动，而那正是 Java 虚拟线程剩余的"pinning"来源。仅当 isolate 被证明太受限时再重新考虑。

---

## 11. JIT 影响（逐阶段）

| 阶段 | JIT 改动 |
|---|---|
| 1 | 无。JIT 代码读 `ks->stack_limit` 与 `ks->c_stack_limit`，由 `kl_new_co_ks` 每协程设置。用在协程内运行的 JIT 直接调用测试验证（`KOALA_JIT_THRESHOLD=1`） |
| 2–3 | 无。park 发生在 helper 内部（`kl_jit_call_helper` → C 函数），那里 `stack_top` 有效（DESIGN §14.1 规则 4） |
| 4 | 无。根扫描对每 `ks` 相同 |
| 5 | 内联 `SET_FIELD` 得到屏障检查（§9.3）。全局存储不需要。基于 helper 的存储经其 C setter 获得 |
| §6.11（可选） | 在 OSR 回边计数器处的抢占检查（DESIGN §17） |
| Unboxed JIT（Yowie，DESIGN §14.2） | 保持"安全点 = 调用点"。有了 stack map 或 spill-at-call，挂起协程与运行中协程以同样方式扫描，只要每个 park 都在带 stack map 的调用点 |
| 6 | 若代码跨 isolate 共享，代码 arena 与编译路径需要锁 |

---

## 12. 调试与工具

### 12.1 每协程 traceback

- `task.dump()` 打印每个存活 task 及其状态与 Koala traceback，像今天 `trace_here` 那样遍历每个 `ks->cf` 链。
- `SIGQUIT`（Ctrl-\）打印同样内容，如 Go。

### 12.2 死锁检测

当 `kl_park` 发现没有可运行协程、没有注册的 fd、没有定时器、没有待决 offload 时，没有任何东西能唤醒任何人。打印"all tasks are parked"加 `task.dump()` 并以错误码退出（这也使 usage issue #8——出错时退出码 0——更值得修）。

### 12.3 统计

- 退出时 `KOALA_CO_STATS=1`：spawn 的 task 数、峰值存活、上下文切换、平均与最大 park 时间、栈区池命中。
- `gc` 模块（`src/modules/gc.c`）：向 `GCCycleStats` 增加每轮原子步停顿时间、切片数、屏障命中、终结队列长度。

### 12.4 压力模式

| 变量 | 效果 |
|---|---|
| `KOALA_GC_STRESS=park` | 每次 park 全回收（找不变式 4 违规） |
| `KOALA_GC_STRESS=alloc:N` | 每 N 次分配回收（§8.3 之后） |
| `KOALA_GC_VERIFY=1` | `GC_REMARK` 处的校验 pass（找缺失屏障，§9.3） |
| `KOALA_CO_SHUFFLE=seed` | 随机化就绪队列顺序（找测试中的顺序假设） |
| `KOALA_CO_TRACE=1` | 经 `log.c` 记录每次 spawn、park、wake、switch |

### 12.5 Sanitizer

为 ASan、TSan、Valgrind 加 fiber 注解（§5.7）。为阻塞池与清扫线程增加 `test/test-c/tsan_sched.sh`，以 `tsan_wsdeque.sh` 为模板。

---

## 13. 风险与缓解

| 风险 | 缓解 |
|---|---|
| C 栈对协程内深递归太小 | 测量每调用 C 栈用量（§5.4）；尺寸可配置；经现有检查给出干净的"stack overflow"错误 |
| `vm.max_map_count` 把协程数限制在约 32k | 每协程一个栈区（§5.3）；`KOALA_CO_GUARD=0`；文档说明该 sysctl |
| native 跨 park 持有未 rooting 对象（issue #11） | shadow stack（§8.3）；先仅在 park 点 GC（§8.4）；`KOALA_GC_STRESS=park` |
| 缺失写屏障 → 释放存活对象 | `KOALA_GC_VERIFY`；基于 grep 的审计清单（§9.3）；到处使用屏障 helper `gc_write`/`gc_write_obj` |
| CPU 密集 task 饿死其他 | `task.yield_now()`；可选抢占（§6.11） |
| 阻塞 native 调用拖停每个 task | offload 池（§7.6）；`@parks` 式文档；`KOALA_CO_STATS` 报告 park 之间的长间隔 |
| O_NONBLOCK 泄漏到父 shell | 绝不在继承 fd 上设置（§7.5） |
| 平台差异（kqueue vs epoll、无 `pidfd`） | 轮询器接口（§7.1）；回退到 `SIGCHLD` self-pipe |
| 栈切换带来的 sanitizer 噪声 | 注解（§5.7） |

---

## 14. 里程碑与测试

除非另有说明，所有测试进入现有 lit 与 FileCheck 套件（`test/test-run`、`test/test-kl`、`test/test-c`）。

### M1：fiber 与调度器核心（Phase 1–2，仅定时器）

交付物：`fiber_x86_64.S`、栈区与池、`kl_new_co_ks`、调度器、`TaskObject`、`spawn`/`join`、`Channel`、`group`、`sleep`、取消、来自所有 `ks` 的 GC 根（§8.1），GC 仅在显式 `gc.collect()` 时。

测试：
- `test_task_basic.kl`：spawn、join、各种结果值（int、float、ref、intf、optional）
- `test_task_error.kl`：子错误由 `join` 重抛；未 join 的错误退出时打印
- `test_channel.kl`：带缓冲、rendezvous、close、`for v in ch`
- `test_group.kl`：join 全部；首个错误取消兄弟；嵌套 group
- `test_cancel.kl`：挂起时、可运行时、启动前取消；`with` 清理运行
- `test_task_gc.kl`：1 万个挂起 task，`gc.collect()`，每个 task 的局部变量存活且醒后正确
- `test_task_stack_overflow.kl`：task 内深递归给出"stack overflow"，其他 task 继续
- `test_task_jit.kl`：task 内 JIT 直接调用与 self-only 递归，`KOALA_JIT_THRESHOLD=1`
- `test_deadlock.kl`：所有 task 挂在 channel 上，带 task dump 的错误退出
- C 测试 `test_fiber.c`：切换与 trampoline、MXCSR 保留、对齐
- 内存：10 万空闲 task 的 RSS（记录数字；目标低于约 2 GB）
- `bench/`：从不 spawn 的程序无回归

### M2：I/O（Phase 3，socket、管道、信号）

交付物：轮询器（先 epoll）、socket 改动（§7.3）、管道与 `Process.wait`、信号、stdin 就绪、截止时间。

测试：1k 并发客户端的 echo server；带 group 的 `libs/koala/http` server accept 循环；多个 task 同时 `os.run` 大输出（不挂起，如 `Koala_TODO.md` 中 `communicate()` 修复）；截止时间与超时错误；继承 fd 保持阻塞模式。

### M3：阻塞池与 TLS

交付物：`kl_run_blocking`、DNS、带每连接互斥量的 sqlite、非阻塞 TLS。

测试：慢 sqlite 查询不阻塞一个滴答 task；offload 期间取消；TLS 客户端对本地 server；`tsan_sched.sh`。

### M4：GC 触发与 native 根（Phase 4）

交付物：复活 shadow stack（§8.3）、审计 `os.c`、`socket.c`、`dictobj.c`、`hashset.c`、`libs/koala/*` native；park 点自动 GC（4a），然后分配时（4b）；逃逸分析规则（§8.5）。

测试：全套件在 `KOALA_GC_STRESS=park` 下，然后 `alloc:1000`；带 task 的 `test_gc_*.kl`。

### M5：增量 GC（Phase 5）

交付物：解释器、C setter、native、JIT 中的 SATB 屏障；惰性栈扫描；pending free；清扫线程；终结队列；节奏；校验器。

测试：全套件在 `KOALA_GC_VERIFY=1` 配小步长下；JIT 开/关的差分运行；分配密集 server 基准上 `gc` 模块的停顿时间报告；标记空闲时 `bench/` 无可测变化。

### M6：isolate（Phase 6）

交付物：每 isolate 的 `GCHeap` 与其他全局（§10.2）、跨 isolate channel、`isolate.spawn`。

测试：并行 CPU 基准随核数扩展；消息拷贝正确性；TSan 干净。

---

## 15. 开放问题

1. **`spawn` 的泛型签名。** `spawn(fn, args...)` 还是仅 `spawn(lambda)`。取决于类型检查器对带变参函数值类型推断得有多好。
2. **程序退出。** `main` 返回应等待 detached task（Python asyncio 风格）还是结束程序（Go 风格，§6.10 提议）？
3. **未 join 的 task 错误。** task 被释放时打印（提议）还是让程序失败？
4. **每 fd 并发。** 每 socket 一个读者一个写者（提议，如 Go）还是读者队列。
5. **默认尺寸**（§5.4）：测量每调用 C 栈用量后设定。
6. **arm64 时机**：M1 就写 `fiber_arm64.S` 还是等 arm64 JIT 支持？
7. **isolate 的 frozen 对象**：取决于 frozen 集合家族设计。

---

## 附录 A：其他语言怎么做

| 语言 | 模型 | 对 Koala 的备注 |
|---|---|---|
| Go | 有栈 goroutine，M:N，可增长（拷贝）栈 | 拷贝栈需要每个 C 帧的精确映射；Koala 的 C 栈无法拷贝，所以我们改用固定惰性提交栈区 |
| Java 21+（Loom） | 虚拟线程，有栈，JIT 编译代码 | 表明栈切换与 JIT 兼容。自 JDK 24（JEP 491）起剩余的 pinning 来自 native 帧，isolate 避免了它 |
| PHP 8.1+ | Fiber：带 JIT 的 C VM 中全栈切换 | 最接近本计划：C 解释器、JIT、从任何地方挂起 |
| Ruby 3 | Fiber 加可插拔 fiber 调度器，使阻塞 I/O 非阻塞 | 与 §7 相同的"现有同步 API 变并发"思路 |
| Lua | 协程没有自己的 C 栈；不能跨 C 调用 yield（需要 `lua_yieldk` 续延） | 表明在 C 栈上递归的 Koala 为何需要每协程真正的 C 栈 |
| Python | asyncio（染色的 `async`/`await`）；3.14 起支持 free-threaded 构建，约 5–10% 单线程代价 | 表明选项 A 的代价与选项 B 中同步/异步库的分裂 |
| Rust、C#、JS、Kotlin、Swift | 无栈，编译器状态机，染色函数 | 每 task 内存最低；需要编译器工作与 API 的 async 版本 |
| Zig 0.16 | 调用者传入 `std.Io` 接口；同一代码阻塞、线程或事件化运行 | 与 §7 同一目标：一套 I/O API 供同步与并发使用 |
| OCaml 5 | effect handler（fiber）配直接风格 I/O（Eio） | 无染色的直接风格；Ocsigen 保留染色的 Lwt 部分是为了让挂起可见，§6.12 处理了这一点 |
| Erlang、Dart、JS worker | 隔离堆加消息传递 | §10 的模型 |

## 附录 B：术语表

| 术语 | 含义 |
|---|---|
| Fiber / 有栈协程 | 拥有自己栈的协程；可从任何调用深度挂起 |
| Park | 挂起当前协程直到某物使其可运行 |
| 安全点（Safepoint） | GC 可运行且所有活跃引用都在 GC 查找处的点；在 Koala 中是调用点 |
| SATB | Snapshot-at-the-beginning 标记：周期开始时可达的一切存活；被覆写的引用由写屏障着色 |
| Isolate | 自己线程上、拥有自己堆的 VM 实例；isolate 之间不共享可变对象 |
| 结构化并发 | task 绑定到词法作用域（`with task.group()`）；离开作用域等待或取消它们 |
