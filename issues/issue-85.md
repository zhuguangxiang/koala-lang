# `koala/readline` 遗留项

← 返回 [问题列表](issues.summary.md)

- [ ] 部分完成 —— Ctrl-Z / Ctrl-\ 已修（2026-11-08）；仍有 8 类功能未做

> 来源：`docs/Koala_TODO.md` §87（2026-11-08，to do）

---

### 详细描述

<details>
<summary><b>📌 定位与已记录的有意差异</b></summary>

`koala/readline` 跟随 CPython 的 `readline` 模块，编辑器跟随 GNU readline 8.3 的 emacs keymap。

`docs/koala_readline_api.md` 列出了有意不同的地方：`input` 返回 nil 或 `""` 而不抛异常、回调是 trait 对象、文件类函数返回 false。

</details>

<details>
<summary><b>✅ 已修（2026-11-08）：Ctrl-Z / Ctrl-\ 无效</b></summary>

原因是 raw 模式清掉了 `ISIG`，编辑器把字节丢掉了。现在编辑器自己处理终端的挂起与退出键（从 termios 读，所以 `stty susp`、`stty -isig` 会被遵循）：先退出 raw 模式，通过 `Terminal.suspend()` / `quit()` 向进程组发 SIGTSTP 或 SIGQUIT，然后回到 raw 模式继续编辑、不重绘，与 GNU readline 一致。

Ctrl-C 仍是一个按键，以便 `input` 能返回 `""`。用例：`test_editor.kl`（基于 `MemoryTerminal`）。

</details>

---

### 修复方案

<details>
<summary><b>🛠 尚未完成</b></summary>

- **行编辑之外的挂起 / 退出键**：目前只在一行编辑期间被捕获。增量搜索、quoted-insert、补全的 `y or n` 询问、`--More--` 分页器期间，Ctrl-Z 与 Ctrl-\ 被当普通按键读入；GNU readline 下终端驱动在那里也会发信号。
- **vi 模式**：`editing-mode vi` 与 vi keymap；该行被接受但忽略。
- **数值参数与 region**：`digit-argument`（M-0 … M-9、M--）、`universal-argument`、`set-mark`、`exchange-point-and-mark`、`kill-region`、`copy-region-as-kill`。
- **其它 GNU 命令**：`menu-complete`、`history-search-backward` / `-forward`、`non-incremental-*-search-history`、`character-search`、`overwrite-mode`、键盘宏（C-x ( / C-x ) / C-x e）、`dump-functions` / `dump-variables`、`tilde-expand`。
- **长行**：宽于屏幕的行像 linenoise 那样在一行内横向滚动；GNU readline 会折成多行。
- **`bell-style visible`** 无声；GNU readline 会闪屏。
- **`readline.backend`**（Python 3.13）未提供。
- **阻塞**：读一个键会阻塞线程；Koala 有并发 API 后会拖住调度器。
- **平台**：shim 用 POSIX termios；无 Windows 控制台支持。

</details>