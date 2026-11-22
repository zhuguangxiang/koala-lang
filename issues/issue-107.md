# `os.run` 子进程管道：大 payload「先写后读」死锁 + 无 finalizer 致 fd 泄漏与 zombie

← 返回 [问题列表](issues.summary.md)

- [ ] 部分修复 —— 死锁已由 `communicate()` 解决；finalizer 已接线但**运行期无人调用**

> 来源：`docs/Koala_TODO.md` §28
>
> **Status (2026-10-31)**: partly fixed; the `communicate()` and SIGPIPE-restore tests pass.

---

### 详细描述

<details>
<summary><b>1️⃣ 死锁 —— 已修</b></summary>

**现象**：对 `os.run` 返回的 Process，若「先把 stdin 全部写完，再 `read_all()` 读 stdout」，当 payload 超过管道容量时死锁 —— 父进程阻塞在 `write(stdin)`、子进程阻塞在 `write(stdout)`，两侧永久睡眠（`ps` TIME=00:00:00，wchan 分别为 `anon_pipe_write` / `pipe_wait_writable`）。

**原阈值**：`src/modules/os.c:422` 的 `kl_pipe_cloexec` 用裸 `pipe()`（全仓无 `F_SETPIPE_SZ`），容量为 Linux 默认 64KB。子进程为 echo（如 `cat`）时：stdout 管道灌满 64KB → 子进程停读 stdin → stdin 管道再灌满 64KB → 父进程停写，故 payload 越过约 128KB（2×64KB）即死锁。

**已做的修复**：`Process.communicate(input bytes? = nil) (bytes, bytes)`（`src/modules/os.c`）在一个 `poll()` 循环里同时处理 stdin / stdout / stderr，关闭全部三个父侧端并回收子进程。

`os_native_lib_init` 现在忽略 SIGPIPE（`socket` 早已如此），所以子进程停止读取时写操作以 EPIPE 失败而不是杀掉进程。被忽略的信号会跨 exec 保留，所以 `os.run`（`posix_spawnp`）与 `os.system`（改为 `posix_spawn` 调 `/bin/sh -c`，仍返回原始 wait status）都传 `POSIX_SPAWN_SETSIGDEF` 恢复 SIGPIPE 默认（`init_spawn_attr`），与 Python 的 `restore_signals` 一致。与 `system(3)` 不同，shell 运行期间 `os.system` 不会在父进程里忽略 SIGINT / SIGQUIT。

新测试在 `test/test-ut/test_run_pipe.kl`（`test_communicate_*`；`test_communicate_large` 发 300000 字节；`test_*_sigpipe_default`、`test_system_exit_code`）。

</details>

<details>
<summary><b>1️⃣b 阈值并非恒为 2×64KB（2026-11-01 发现）</b></summary>

一旦用户的管道总用量超过 `/proc/sys/fs/pipe-user-pages-soft`（16384 页），Linux 只给新管道 8 KiB（在 40 个测试并行跑时用 `F_GETPIPE_SZ` 实测过），于是两条管道只容纳 16384 字节 —— 17000 字节先写后读就会死锁。

实测：40 次并行跑 `test_write_chain.kl` 挂 1 次（它正是为此被 `// SKIP`）。现在它用 `communicate()` 发那 17000 字节，重新启用（120 次并行跑，无挂起）。

`test_run_read_all_large` 是同因同形：现在让子进程自己产生 17000 字节（`yes ... | head -n 1000`）而只用 `read_all()` 读，重新启用（去掉 `@skip`；120 次并行跑 `test_run_pipe.kl`，无挂起）。

</details>

<details>
<summary><b>2️⃣ finalizer —— 已接线但当前无法运行</b></summary>

RawReader / RawWriter 有 `_free = raw_free`（若从未 `close()` 则关闭 fd），Process 有 `_free = process_free`（`waitpid(WNOHANG)`：回收已退出的子进程，对运行中的绝不阻塞）。

`kl_free_instance()` 在 `mm_free` 之前调用 `tp->_free`，且 `TypeObject._free` 被文档定义为「不释放内存的 finalizer」。

**但今天运行期不释放任何对象**（没有显式 `free`、没有编译器插入的 free、CMS GC 关闭；见 Design Overview §12），所以**这些 finalizer 从不触发**。在有 free 通道之前，不调 `close()` / `wait()` 的代码仍有泄漏 ① / ②；`communicate()` 是无泄漏路径。

</details>

<details>
<summary><b>📌 现象二（fd 泄漏 / zombie）的原始描述</b></summary>

`process_type`（os.c:626）、`raw_reader_type`（os.c:273）、`raw_writer_type`（os.c:400）均未注册 `_free` finalizer；运行时也从不调用 `_free`（`kl_free_instance` 仅 `mm_free`，`src/objects/typeobj.c:118`），且无 SIGCHLD 处理。后果：

① `read_all()`（os.c:205）读完不关 fd，测试一般只 `close` stdin，故每个 `os.run` 泄漏 stdout/stderr 两个读端 fd；
② 未显式 `wait()` 的子进程永不回收 → zombie。

</details>

---

### 修复方案

<details>
<summary><b>🛠 剩余待办</b></summary>

- [ ] **`Process` 的三个管道对象是裸 `Object *`**，`_nfields = 0` 且无 `_mark`，所以 tracing GC 看不到它们；开 GC 时要加 `_mark`。
- [ ] **仍在运行的子进程其 Process 被释放时不会被回收** —— 需要 SIGCHLD 处理或 reaper，**方案待定**。
- [ ] **`fs.File` 同样缺 `_free`**（`src/modules/fs.c` 的 `file_type`）：从不 `close()` 的 File 会泄漏 fd。要与其上的 free 通道一起加一个关闭 fd 的 `_free`。
- [ ] 本节全部内存 / finalizer 项**等待运行期 free 通道**（显式 `free`、编译器插入的 free、或 GC）；目前仅在此挂账，不进工作计划。

</details>

<details>
<summary><b>🔗 关联</b></summary>

- finalizer 机制在 `TypeObject` 已声明 `_free` / `_mark`（`include/runtime/object.h:261-263`）但**全仓未接线**。
- [#106](issue-106.md)（§16 声明-实现欠账）、[#97](issue-97.md)（§79 native handle 无 finalizer）同源。

</details>