# `std/os` 的信号处理（等待并发 API）

← 返回 [问题列表](issues.summary.md)

- [ ] 未实现 —— 等并发 API 落地后再设计

> 来源：`docs/Koala_TODO.md` §39（2026-11-01）

---

### 详细描述

<details>
<summary><b>🔍 现状</b></summary>

`std/os` 能**发**信号（`os.kill(pid, sig)`、`Process.terminate` / `kill`、`SIGHUP` .. `SIGTERM` 常量），但程序**无法处理**信号：没有 `signal.signal` 的对应物，所以 SIGINT / SIGTERM 以默认动作结束进程，运行期唯一的信号设置是忽略 SIGPIPE（§28、§38.5，见 [#64](issue-64.md)）。

</details>

<details>
<summary><b>⏳ 为什么要等</b></summary>

C 信号处理器是**异步**运行的：内核挑哪个线程都行，可能打断任意一条 VM 指令；它只能做 async-signal-safe 的工作，因此**不能跑 Koala 代码**。

处理器只能记录信号（如 self-pipe 或 `signalfd`），Koala 层的处理器必须在之后、在正常的执行线程上运行。而「在哪、何时」取决于并发 API —— 运行期的绿色线程已有，但 std 还没暴露（§25.3）。

</details>

---

### 修复方案

<details>
<summary><b>❓ 设计时需决定</b></summary>

1. **API 形态**：回调（`os.on_signal(sig, func)`，像 Python）、收到信号的 channel / 迭代器（像 Go 的 `signal.Notify`），或两者都有。
2. **哪些信号可被处理**：SIGKILL / SIGSTOP 不行；SIGSEGV / SIGBUS / SIGFPE 最好留给运行期（panic、traceback）。

</details>