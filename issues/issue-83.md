# `std` 的两个缺口（`koala/tarfile` 不得不绕过）

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 两个缺口都只是 `std` 的小增补

> 来源：`docs/Koala_TODO.md` §84（2026-11-08，to do）

---

### 详细描述

<details>
<summary><b>1️⃣ `std/fs` 没有 `mkfifo()` / `mknod()`</b></summary>

`TarFile.makefifo()` 与 `TarFile.makedev()` 因此是会 panic 的桩（"Koala has no mkfifo"），FIFO 或设备成员只能**列出**、永远**不能解出** —— 尽管 `dir()` / `chmod()` / `chown()` / `set_times()` 都在。

- 在 `libs/std/fs/` 加一个 `mkfifo(2)` 包装即可吃掉 FIFO 那一半。
- `mknod(2)` 在非 Linux 上要处理 `<sys/mknod.h>`，所以可以**只支持 Linux**。

</details>

<details>
<summary><b>2️⃣ `std/time` 没有日历函数</b></summary>

只有 `Duration` / `Instant` —— 没有 `localtime()`、没有 `gmtime()`、没有 `strftime()`。

因此 `TarFile.print_list()` 的时间列打印的是 **UTC**（Python 打本地时间），且格式化是手写的（`utc_datetime()`：把 epoch 秒数按 civil-from-days 换算）。任何想要 `ls -l` 式时间戳的程序都得重写同样的代码 —— 所以一个返回拆分时间的 `time.gmtime()` 本身就值得有。

</details>

<details>
<summary><b>📌 背景</b></summary>

`koala/tarfile` 是 CPython 3.14 `tarfile` 的 1:1 移植；这两个特性没有 Koala 对应物，且两者都只是 `std` 的小增补。

都在移植过程中发现；变通做法记录在 `docs/koala_tarfile_api.md` 的 "Differences from Python"。

</details>

---

### 修复方案

<details>
<summary><b>🛠 待办</b></summary>

1. `libs/std/fs/`：新增 `mkfifo(2)` 包装（跨平台）；`mknod(2)` 可仅 Linux。随后把 `TarFile.makefifo()` / `makedev()` 的 panic 桩换成真实实现。
2. `libs/std/time/`：新增 `gmtime()`（返回拆分时间）；`localtime()` 取决于时区支持（见 [#61](issue-61.md) 的约束前提）；`strftime()` 待定。随后让 `TarFile.print_list()` 去掉手写的 `utc_datetime()`。

</details>