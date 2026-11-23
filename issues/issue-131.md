# 131. `/cancel` 无法中断正在运行的 agent turn（单线程阻塞在 `agent.run`）

← 返回 [问题列表](issues.summary.md)

- [ ] 未实现：run 进行中可用 `/cancel` 取消（需并发或非阻塞 stdin）
- [x] 已实现替代：Ctrl-C 经 SIGINT 标志在检查点停止（M5）
- [x] 已实现 `/cancel`：run 结束后丢弃被打断的那一轮（含 user 消息）
- [ ] 待讨论：是否给 readline 增加非阻塞按键/整行轮询

> 来源：M5（流式 + interrupt）实现过程中发现；计划原文要求 `/cancel` 作为显式取消命令

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

`demo/chat` 的 REPL 是单线程的：`Session.agent(task)` 直接调用 `Agent.run`，而 `run` 会阻塞在 HTTP 流读取与工具执行上。因此用户在一次 agent 运行期间键入的 `/cancel` 只会进入终端的行缓冲，等运行结束后才被 REPL 读到——此时已经没有可取消的运行。

目前 `/cancel` 的语义退化为“丢弃上一轮被打断的残留消息”：

```text
> /agent 写一个 hello.c
bot> ...（流式输出）
^C
(interrupted; send /continue to resume, /cancel to drop this turn)
> /cancel
(cancelled the interrupted turn)
```

它能清理历史，但不能在运行中让 agent 提前停下。

</details>

<details>
<summary><b>🧪 最小复现</b></summary>

```bash
source env.sh
cd demo && wombat build chat
# mock 端点慢速流式回复（每个 chunk 100ms）
koala chat/test/support/mock_openai.kl --script slow.json --port-file /tmp/p --requests /tmp/r --delay-ms 100 &
# 配置 base_url 指向该端口后启动 chat，执行
/agent 讲一段话
/cancel        # 立刻键入并回车：不会生效，必须等流结束
```

键入的 `/cancel` 出现在**下一次**提示符读到的输入里。

</details>

<details>
<summary><b>🧠 根因</b></summary>

1. 循环在同一个线程上跑：`repl.kl` → `session.agent()` → `Agent.run()` → `_loop()` → `_stream_once()`，中间没有任何输入读取点。
2. 终端在 agent 运行期间处于 cooked 模式（编辑器不在读键），键入的整行只有按回车且有人 `read` 时才可见；目前唯一的“外部输入”通道是 SIGINT。
3. 因此唯一能在运行中送达的信号是 Ctrl-C；`/cancel` 需要额外机制。

</details>

<details>
<summary><b>🛠 候选修复方案</b></summary>

按代价从低到高：

1. **`/cancel` 转发 SIGINT（最小改动）**：REPL 处理 `/cancel` 时调用 `os.kill(os.getpid(), SIGINT)`（`std/os` 已有 `getpid`/`kill`）。运行中不可达（仍阻塞），但在“暂停/等待输入”的路径上等价于 Ctrl-C，语义统一、无新增原生代码。缺点是解决不了“运行中”这个核心诉求。
2. **readline 增加非阻塞整行轮询**：在 `readline` 原生 shim 里加 `poll(2)`（0 超时）读 stdin，`_stream_once` 每个 chunk 之间检查一次，读到 `/cancel\n` 就按中断处理。需要处理被吞掉的普通输入（要么缓存给下一次编辑器，要么提示丢弃）。pty 测试可覆盖。
3. **引入并发**：等 runtime 提供线程/协程后，把 agent loop 放到后台，主线程继续读输入。这是最彻底的方案，但依赖 runtime 能力，超出当前范围。

建议先做 1（统一语义、零风险），把 2 留作 M7/M8 讨论项。

</details>

<details>
<summary><b>📌 当前状态（M5 已落地部分）</b></summary>

- Ctrl-C：`readline.watch_interrupt` + SIGINT 标志，流中/chunk 边界与 tool call 之间检查，`test_pty.sh` 的 `check_pty_interrupt` 覆盖真实终端路径。
- `/cancel`：`Agent.cancel()` 丢弃 `_turn_mark` 之后的消息并对齐 `_sent_len` / `_compacted_len`，`test_interrupt_before_request_sends_nothing` 覆盖。
- 双击 Ctrl-C 语义退化为“第一次即停”：单线程下第一次中断已直接回到 REPL，不需要第二下。

</details>