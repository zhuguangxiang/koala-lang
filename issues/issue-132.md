# 132. 非 tty 下 API key 提示会吞掉 stdin 第一行，命令静默丢失

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复：未设 `$OPENAI_COMPAT_API_KEY` 且 stdin 非 tty 时，首个输入行被当作 key 读取
- [ ] 未修复：key 无脱敏/无来源提示，脚本化运行难以察觉
- [ ] 待补测试：非 tty + 无 env 时应在读 stdin 之前失败

> 来源：2026-11-23 用 `printf '...' | koala chat.klc` 做端到端烟测时连续踩中两次

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

未导出 `OPENAI_COMPAT_API_KEY` 时，`demo/chat` 会打印 `API key: ` 并从 stdin 读一行。在管道/重定向下 stdin 正是调用方的脚本输入，于是**第一条输入行被当成 API key 消费掉**：

```bash
printf '/agent 写一个 hello.c 然后编译运行\n/quit\n' | koala demo/dist/demo/chat.klc
```

实际效果：卡片后只有一个 `> `，agent 完全没有启动，也没有任何错误；`/quit` 正常退出，退出码 0。若第一行是 `/context`、`/sessions` 等命令，同样会静默消失；剩下的行照常执行，所以从输出上很难看出少了一条。

更隐蔽的是：key 取到的是任意文本（如 `/context`），如果后端不校验 key，整条链路“看起来正常”，问题只在后续真实端点上变成 401。

</details>

<details>
<summary><b>🧪 最小复现</b></summary>

```bash
source env.sh && cd demo && wombat build chat
cd /tmp && printf '/help\n/quit\n' | koala /path/to/demo/dist/demo/chat.klc
# 期望：打印帮助
# 实际：卡片 + `> `，无帮助输出（`/help` 被当作 API key）
printf '\n/help\n/quit\n' | koala /path/to/demo/dist/demo/chat.klc
# 实际：`Error: no API key: set $OPENAI_COMPAT_API_KEY or enter it at startup`
```

对照组（设了 env）：`printf '/help\n/quit\n' | OPENAI_COMPAT_API_KEY=sk koala chat.klc` 正常打印帮助。

pty 下的手工运行不受影响，因为终端里用户本来就该看到提示并输入 key；问题只在“stdin 被脚本占用”的场景。

</details>

<details>
<summary><b>🧠 根因</b></summary>

1. `__chat__.kl` 的 `main` 在 `$OPENAI_COMPAT_API_KEY` 为空时调用 `resolve_key()`（`demo/chat/config.kl:110` 附近），后者用 `readline.input("API key: ")` 从 stdin 读一行。
2. `readline` 在非交互终端走 `_read_plain()`（`libs/koala/readline/editor.kl:1791`），直接从同一个 fd 读，不做 tty 判断也不区分“交互输入”与“脚本输入”。
3. 因此 prompt 的读取和 REPL 的读取共享同一个 stdin 流，先跑的那个拿到第一行；没有任何“我是从管道读到的”提示，也没有回显脱敏，用户无法从输出判断第一行去了哪里。

</details>

### 修复方案

<details>
<summary><b>🛠 候选方案</b></summary>

按侵入性从低到高：

1. **非 tty 直接失败**（推荐，最小改动）：在 `resolve_key()` 读 stdin 之前判断 stdin 是否为终端（`std/sys` 或现有 `tui.Screen.is_tty()` 同源能力），不是终端就 `panic("no API key: set $OPENAI_COMPAT_API_KEY (stdin is not a terminal)")`，与人机提示语义分开。脚本化调用的正确做法本来就是导环境变量。
2. **保留提示但标注来源并脱敏**：打印 `API key (stdin, not echoed):`，读取时按 `getpass` 的方式不回显，输入若以 `/` 开头再追加一行警告（`(that looked like a command; is $OPENAI_COMPAT_API_KEY set?)`）。属于缓解，不解决“吞掉合法输入”。
3. **把 key 读取从 stdin 挪走**：新增 `--api-key-file PATH` / `CHAT_API_KEY_FILE`，非 tty 时只认 env 或文件，彻底避免争用 stdin。

建议做 1，并顺手在 `demo/chat/__chat__.kl` 顶部 Usage 里写清“stdin 不是终端时必须用环境变量”。

</details>

<details>
<summary><b>🧪 建议新增测试</b></summary>

- `test/test_repl.kl`：`resolve_key` 在“无 env + 非 tty”分支返回错误（需要把 isatty 判断抽成可注入的依赖，或用一个显式参数）。
- `test/test_pty.sh` 新增 `check_pty_piped_stdin_keeps_first_line`：不设 env，`printf '/help\n/quit\n' | chat`，断言输出含帮助标题且不含 “API key: ” 交互等待。
- 文档向：`test_config.kl` 断言错误文案包含 `OPENAI_COMPAT_API_KEY`。

</details>