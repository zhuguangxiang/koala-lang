# demo/chat 其他模块优化分析与实施计划

> 状态：分析与计划完成；P0/P1 中与 agent loop、会话、压缩、中断相关的部分已随 `agent_loop_plan.md` M1–M7 落地，剩余（模块重构、启动诊断、P2）待实施，逐项状态见 §1.3
> 范围：`demo/chat` 中除 `session.kl` / `sessions.kl` 之外的模块，以及它们直接依赖的 `libs/koala`、`libs/std` 模块
> 关系：
> - `session_plan.md` 负责会话持久化、恢复、活动上下文与生命周期，本文不重复展开。
> - `agent_loop_plan.md` 负责 agent loop、native tool calling、工具集与权限模型，本文补充模块边界与依赖落点。
> - `demo/chat/todo.md` 已完成 UI Phase 0–4，本文只分析它留下的后续缺口。
> 结论：**还需要优化**。`session` 不是唯一短板；`config`、`repl`、`agent`、`compact`、`ui`、`openai`、`json`、`readline` 都有阻断 native tool calling、长任务和稳定运行的模块级问题。

## 1. 总体判断

当前 `demo/chat` 的模块划分是合理的：`config` 管配置，`ui` 是纯渲染，`repl` 管输入和命令，`agent` 管工具循环，`compact` 管上下文，`session` / `sessions` 管会话。问题不在“模块切得不对”，而在以下几点：

1. **多个模块仍以 `dict[str, str]` 的 `role/content` 为唯一消息模型**。这会同时卡住 `agent`、`compact`、`session` 和 `openai`。
2. **错误处理靠 `panic`**。`json.parse_*`、`Config`、`fs.read_str`、`resp.choices[0]`、`parse_str` 的失败路径都可能直接结束进程。
3. **阻塞式 I/O 没有取消路径**。HTTP、SSE、`os.output`、readline 输入都属于阻塞调用，Ctrl-C 不能在请求或工具执行中安全停止。
4. **`repl.kl` 是 1096 行的单一分发器**。命令表只服务补全，实际 handler 是另一套 `if` 链，新增命令容易半接入。
5. **UI 只适合 fenced `write/shell/done`**。`tool_result` 靠 `exit 0` / `write ok:` 字符串判断成功，native `Read/Edit/Glob/Grep` 会被误判为失败。
6. **启动顺序让错误不可见**。`SessionManager` 在 TUI 初始化前就可能输出坏会话警告；`Config` / API key 失败则直接 panic。
7. **缺少 `tools.kl` 这一层**。`agent.kl` 同时承担提示词、协议解析、权限、写文件、跑 shell，职责过重。

### 1.1 优先级分层

| 层级 | 模块 | 结论 | 阻塞什么 |
|---|---|---|---|
| P0 | `libs/koala/json` | 必须新增 safe parse | malformed session、tool arguments、HTTP body 都可能崩进程 |
| P0 | `libs/koala/openai` | 必须支持 rich message、tool_calls、tool delta | native tool calling 无法成立 |
| P0 | `agent.kl` | 必须重写主循环并拆出工具层 | 只能一次一个 fenced block，20 步硬停 |
| P0 | `session.kl` / `sessions.kl` | 见 `session_plan.md` | 重启丢工具历史、坏文件启动崩溃 |
| P0 | `config.kl` | 需要可诊断的加载与范围校验 | 配置错误直接 panic，0 值导致除零 |
| P1 | `compact.kl` | 需要 pair-safe 与 rich token 估算 | 压缩会切断 `assistant(tool_calls)` / `tool` 配对 |
| P1 | `repl.kl` | 命令注册表、active mode、取消与保存点 | 命令漂移、模式不持久、长任务不可停 |
| P1 | `ui.kl` | 结构化工具结果、多工具块、状态扩展 | native 工具显示错误，无法展示并行/多调用 |
| P1 | `__chat__.kl` | CLI 参数、启动错误页、警告可见性 | 用户看不到启动失败原因，参数能力不足 |
| P1 | `readline` / `http` / `sse` | 可轮询/可关闭的读取 | Ctrl-C 中断、流式 tool call 无法实现 |
| P2 | `tui` | inline 模式、滚动回看、内存上限 | alt-screen 退出丢 scrollback，长会话内存增长 |
| P2 | `std/fs` / `std/os` | 原子写辅助、锁、超时封装 | 已有原语，但需要模块层正确使用 |
| P2 | `koala/cli` / `koala/log` | 统一参数与诊断 | 手写 `--config` 解析、无结构化日志 |

### 1.2 本文不重复的内容

- 会话 v2 schema、v1 迁移、原子保存、`.current`、`/resume`、`/continue`：见 `session_plan.md`。
- native tool 协议、`Read/Write/Edit/Bash/Glob/Grep`、权限模式、plan mode：见 `agent_loop_plan.md` M1–M4。
- UI Phase 0–4 已完成的 golden、resize、plain fallback：见 `demo/chat/todo.md`。

### 1.3 实施状态（2026-11-22，M1–M7 落地后）

| 层级 | 模块 | 状态 |
|---|---|---|
| P0 | `libs/koala/json` | ✅ 已落地（M1）：`try_parse*` 系列不 panic，`parse_*` 保留严格语义 |
| P0 | `libs/koala/openai` | ✅ 已落地（M1/M2/M7）：rich message、`tool_calls`/`tool_call_id`/`name`、流式 tool delta、`message_to_object`/`message_from_object` |
| P0 | `agent.kl` | ✅ 已落地（M2/M3）：native loop + fenced 回落，工具层拆到 `tools.kl`，`/continue` 与 `/cancel` |
| P0 | `session.kl` / `sessions.kl` | ✅ 已落地（M7）：rich 持久化、原子保存、`/file` 活动上下文、`/resume`/`/unpause`、`/sessions` 统计与坏文件 `!` 行、稳定 id + `.current` + `--new`/`--resume`、跨重启 pending；未落地项见 `session_plan.md` §13.1 |
| P0 | `config.kl` | 部分：新增 agent 相关项（`agent_max_steps`、超时、输出上限、权限模式与 allow/deny），但缺字段仍 `panic`，未做范围校验与诊断页 |
| P1 | `compact.kl` | ✅ 已落地（M6）：pair-safe 切分、rich token 估算、请求前 guard、失败报错保留历史 |
| P1 | `repl.kl` | 部分：命令已集中到 `command_infos()`（`/help`、补全、`/sessions` 共用），但分发仍是 `if` 链；`active_mode` 未持久化 |
| P1 | `ui.kl` | 部分：结构化工具块、多工具、diff、审批已到位（M2/M3/M5）；状态行仍只有 mode/model/workdir/branch/token，没有权限模式与后台任务 |
| P1 | `__chat__.kl` | 部分：`--config`、`--continue` 已支持；启动失败（缺 config/key）仍是 `panic`，没有错误页 |
| P1 | `readline` / `http` / `sse` | ✅ 已落地（M5）：可中断读取支撑 Ctrl-C 与流式 tool call |
| P2 | `tui` / `std/fs` / `koala/cli` / `koala/log` | 未动 |

## 2. 模块清单

| 模块 | 行数级别 | 当前职责 | 主要风险 | 结论 |
|---|---:|---|---|---|
| `__chat__.kl` | 112 | 入口、`--config`、组装对象 | 启动顺序、panic、无 `--help/--resume` | 需要小改 |
| `config.kl` | 174 | `Config`、API key、路径 guard | 全字段必填、parse panic、无范围校验 | 需要重构加载层 |
| `ui.kl` | 775 | 纯渲染、工具块、diff、审批 | 字符串判断成功、仅单工具、状态维度少 | 保留架构，改事件模型 |
| `repl.kl` | 1096 | 输入循环、命令、TUI 粘合 | 单体、命令漂移、无取消、保存点少 | 需要拆分 |
| `agent.kl` | 307 | fenced agent loop、写/shell | 单 block、20 步、无超时/输出上限 | 按 M2/M3 重写 |
| `compact.kl` | 115 | 自动/手动压缩 | 切断 tool 配对、token 估算错误 | 需要重写切分 |
| `session.kl` | 425 | chat/agent 上下文、模型、workdir | 双上下文、token 串线、`read_str` panic | 见 `session_plan.md` |
| `sessions.kl` | 258 | 文件 store、manager | 非原子写、坏 JSON panic、mtime 选当前 | 见 `session_plan.md` |
| `test/*` | 约 2200 | 纯函数、传输失败 smoke、pty | 无 mock LLM 多步 E2E | 需要测试床 |
| **缺失** | — | `tools.kl` | 工具协议与执行散在 `agent.kl` | 建议新增 |
| **缺失** | — | `message.kl` / `tool_types.kl` | rich message 没有唯一类型 | 建议随 M1 新增 |

## 3. 模块依赖图

```text
__chat__.kl
  ├─ Config ─────────────── koala/json (parse_file, unsafe)
  ├─ resolve_key ────────── koala/readline(password)
  └─ SessionManager
       ├─ SessionStore ───── std/fs, koala/json (unsafe)
       ├─ Session
       │    ├─ Agent
       │    │    ├─ parse_block / run_action
       │    │    ├─ std/fs.write_str (non-atomic)
       │    │    └─ std/os.output (no timeout)
       │    ├─ compact ───── koala/openai.create (non-stream)
       │    ├─ ask ───────── koala/openai.stream
       │    └─ list_models ─ koala/http.Client
       └─ repl
            ├─ ui (pure)
            ├─ koala/readline (input, key hook)
            ├─ koala/tui.Screen/WrappedView
            └─ emit / busy / status
```

关键路径：

```text
safe JSON
  -> OpenAI rich message / tool protocol
  -> Agent native loop
  -> tools.kl
  -> compact pair-safe
  -> session rich persistence
  -> repl active-context + interrupt
  -> ui structured tool events
  -> CLI/TUI polish
```

这条顺序的原则是：**先保证数据结构和错误模型正确，再改显示与命令**。先做 UI 多工具块、而消息仍是 `dict[str,str]`，会在 M1/M2 后返工。

## 4. 模块逐项分析

### 4.1 `config.kl`：从“能读”升级为“可诊断、可校验、可演进”

**现状**

- `Config.__init__` 一次读完所有字段（`config.kl:44-57`），缺字段直接 `panic`（`config.kl:92-119`）。
- `_load_object` 用会 panic 的 `json.parse_file`（`config.kl:81-89`），错误信息只有“配置缺字段/不是对象”。
- `resolve_key` 在无环境变量且输入为空时 `panic`（`config.kl:63-69`）。
- `agent_max_steps` 默认 20（`config.kl:56`），与 `agent_loop_plan.md` 已确认的默认 100 不一致。
- 只校验 `temperature` 是数字（`config.kl:111-119`），没有校验 `context_window > 0`、`max_code_bytes > 0`、`max_tokens >= 0`、`agent_max_steps > 0`。
- `context_window = 0` 会让 `session.kl:226-229`、`session.kl:277-281` 的除法直接 panic。
- 没有 `request_timeout_ms`、`tool_timeout_ms`、`max_tool_output_bytes`、`session_dir`、`permission_mode`、`retry`、`compact_percent`、`keep_recent` 等配置。
- `no_access_dirs` 类型不匹配时静默忽略（`config.kl:123-136`），虽然安全上可以接受，但应给 warning。
- `openai.Client` 对非法 `base_url` 会静默回落到 OpenAI 官方地址（`libs/koala/openai/client.kl:25-36`），这是危险的“配置写错却发到别处”。

**目标**

1. 新增 `ConfigResult` 或 `Config?` + `config_error()` 的加载路径，供入口打印可读错误；保留 `Config()` 的 panic 包装，避免一次性破坏现有测试。
2. 区分“必需字段”和“可选字段”。建议：
   - 必需：`model`、`base_url`、`history_file`、`temperature`、`max_tokens`、`context_window`、`system_prompt`。
   - 可选：`max_code_bytes`（默认 65536）、`agent_max_steps`（默认 100）、`stream_delay_ms`（默认 0 或 20）、`no_access_dirs`、`session_dir`、`request_timeout_ms`（默认 180000）、`tool_timeout_ms`（默认 30000）、`max_tool_output_bytes`（默认 65536）。
3. 增加范围校验，错误信息带字段名、实际值、期望范围。
4. `base_url` 必须是 `http://` 或 `https://`，非法时直接拒绝，不允许静默回落。
5. `resolve_key` 支持 `$OPENAI_COMPAT_API_KEY`、`$CHAT_API_KEY` 和可选的 key 文件；EOF / 空输入返回可诊断错误，不 panic。
6. 配置文件支持 `version`（可选，默认 1），为后续 schema 演进留位置。
7. 明确“配置目录”和“工作目录”的区别：`session_dir` 只影响会话文件，`workdir` 只影响 agent。

**任务**

- `config.kl`：拆成 `load_object_safe`、`validate`、`field_*` 三层；`field_*` 不再直接 panic，而是返回错误。
- `config.kl`：增加 `ConfigError` 类（`field`、`message`、`fatal`）。
- `config.kl`：增加 `try_load(path?) -> ConfigResult`，保留 `Config(path?)` 作为 panic 兼容层。
- `config.kl`：增加字段默认值与范围常量。
- `config.kl`：`path_blocked` 保持现状，但补一个“canonical 失败时 fail closed 还是 fail open”的明确注释；当前 `_canonical` 返回 nil 时直接放行（`config.kl:142-174`），在无法解析路径时会 fail open。
- `test_config.kl`：增加默认值、范围错误、非法 base_url、非法 session_dir、key 文件、错误类型分支。

**验收**

- 缺少可选字段能启动；缺少必需字段给出字段名和配置文件路径。
- `context_window = 0`、`agent_max_steps = 0`、负 `max_tokens` 被拒绝，不会进入运行期。
- 非法 `base_url` 不会静默请求 OpenAI 官方地址。
- `wombat test chat` 中原有的 `@should_panic` 用例改为验证 `try_load` 错误，或保留兼容层并新增错误路径测试。

### 4.2 `ui.kl`：保留纯函数架构，替换字符串启发式

**做得好的地方**

- `ui.kl` 不碰终端，`render`、`wrap_lines`、`session_lines`、`diff_lines` 都可单测（`ui.kl:73-159`、`ui.kl:364-434`、`ui.kl:642-714`）。
- `tool_result` 对 stderr 尾部和 `… N lines omitted` 已有明确格式（`ui.kl:215-260`）。
- 审批有 diff、有默认拒绝、有 plain 降级（`ui.kl:731-771`）。
- `test_ui.kl` 覆盖了 80/120/40 列、CJK、ANSI、plain、diff、审批等场景（`test_ui.kl:1-582`）。

**问题**

1. **成功判断是字符串启发式**：`tool_result` 只认 `exit 0` 和 `write ok:`（`ui.kl:218`）。native 工具的 `Read` 返回文件内容、`Glob` 返回路径列表、`Edit` 返回 diff、`TodoWrite` 返回清单，都会被当成失败。
2. **没有结构化工具事件**：`tool_start(step, info)` 只有一行字符串（`ui.kl:205-209`），没有 tool call id、工具名、参数摘要、状态（pending/running/ok/error）、耗时。
3. **只支持单工具**：`tool_end` 与 `Ctrl-O` 只保留最近一次工具输出（`agent.kl:37-39`、`repl.kl:380-401`）。一轮多个 tool calls 时无法逐个展示。
4. **状态行维度不足**：`status_event(mode, detail)` 只有 mode + 一段 detail（`ui.kl:454-457`）；没有权限模式、pending、活动上下文、后台任务、压缩状态。
5. **`session_lines` 的 current 与 paused 互斥**：当前会话如果同时 paused，只显示 `⏸`，不显示 current（`ui.kl:411-417`）。应同时表达。
6. **HTTP 错误提示覆盖窄**：只有 401/429 有专门文案（`ui.kl:438-450`）；400 context overflow、400 unknown tools、5xx、超时、TLS 失败都需要区分。
7. **diff 只按行前缀染色**：`_diff_rows` 对 `+ ` / `- ` 着色（`ui.kl:694-708`），没有 hunk 头、行号、文件头；大文件走 `_simple_edits`，可读性有限。
8. **plain 与颜色路径仍靠调用方传 `plain`**：多处同时传 `plain_mode()`，漏传就会混入 ANSI。应统一由 `UiContext` 或 `Style` 负责。

**目标**

- 新增 `ToolCallView` / `ToolResultView`：
  - `id`、`name`、`summary`、`status`、`duration_ms`、`exit_code`、`body`、`truncated`。
- `tool_start` / `tool_end` 接收结构体，不再从结果字符串猜成功。
- `session_lines` 支持 `current`、`paused`、`active_mode`、`updated_at`、`chat_msgs`、`agent_msgs`、`corrupt`。
- `http_error` 支持 400/401/403/404/408/409/413/429/5xx/0（transport），并给出“下一步动作”。
- diff 增加文件头、可选行号、`@@` 上下文；保持 pure。
- 所有渲系统一接收 `Style {plain, color, glyph}`，禁止调用方各自判断。

**任务**

- `ui.kl`：新增 `ToolCallView`、`ToolResultView`、`ToolStatus`。
- `ui.kl`：`tool_result` 改为 `render_result(ToolResultView, style)`；旧函数保留一个兼容包装。
- `ui.kl`：`session_lines` 扩展字段；current + paused 同时显示。
- `ui.kl`：`http_error` 扩展错误表。
- `test_ui.kl`：为每个 native 工具结果写 golden；多工具、长输出、截断、current+paused、400/500/0 错误。

**验收**

- `Read` 成功结果不再显示 `✗`。
- 一轮两个 tool calls 能显示两个 `⏺` 块，Ctrl-O 能选择最近一个或全部。
- 80/40 列、plain、CJK 不回归。

### 4.3 `repl.kl`：拆出命令注册表、应用状态与输入控制

**现状**

- `run_repl` 从 523 行到 763 行，输入、命令分发、模式切换、会话切换、TUI 绘制全部在一个函数里。
- 命令元数据 `command_infos()`（`repl.kl:784-805`）只服务补全和 `/help`，真正的执行仍是 `if line == ...` / `line.starts_with(...)` 的长链（`repl.kl:586-758`）。新增命令必须同时改两处，容易漂移。
- `mode` 是 `run_repl` 的局部变量（`repl.kl:546`），`_status_mode` 是包级变量（`repl.kl:22`）；`Session` 没有持久化 `active_mode`。恢复一个只有 agent 历史的会话后，模式仍然从 chat 开始。
- 保存点在循环顶部 `mgr.save_current()`，且已在 `agent` / `ask` / `/continue` / `/cancel` 之后补了立即保存；但 tool 轮内部仍不落盘（进程在长任务中途被杀会丢本轮已发生的 tool 结果），详见 `session_plan.md` S5。
- `record_input` 只在 TUI 下记录（`repl.kl:60-70`），plain 模式没有用户消息回显；这不是错误，但会让 non-tty 日志与 TUI 日志不一致。
- `poll_resize` 只在流式 chunk 之间被调用（`session.kl:133-136`、`repl.kl:302-310`）。agent 的 `os.output`、非流式请求、压缩、审批期间没有 resize 处理。
- 没有请求/工具执行期间的取消路径。readline 的 Ctrl-C 只在编辑行时返回 `""`（`libs/koala/readline/editor.kl:304-313`）；请求进行中按 Ctrl-C 会走终端信号默认路径，不能安全停止工具。
- `_ChatCompleter` 的模型列表只取一次并永久缓存（`repl.kl:1021-1028`）；`/model` 切换后补全仍然陈旧。
- `_ChatDisplay` 只处理“全部匹配都是命令”或“按列列出”两种情况（`repl.kl:423-455`），没有“命令 + 参数”混合候选。
- `_approval_input` 直接写 `screen` / `readline`（`repl.kl:499-515`），不可注入，审批流程只能 pty 测。
- `_Chrome` 直接持有 `tui.Screen` / `WrappedView`（`repl.kl:111-241`），没有接口抽象，纯 UI 测试覆盖不到布局与绘制。
- `/clear` 只清 `view`（`repl.kl:673-677`），`/reset` 只清 Session（`repl.kl:610-614`）；两者都不会立即保存或更新 `.current`。
- 未知命令/参数错误已经本地拦截（`repl.kl:746-751`、`repl.kl:940-946`），这是好设计，应保留。

**目标**

1. **命令注册表**：把 `CommandInfo` 升级为 `CommandSpec`，包含 `name`、`args`、`desc`、`group`、`usage`、`run(ctx)`。
   - 补全、`/help`、`unknown_command`、执行都从同一张表来。
   - 参数解析统一走 `split_args`，支持引号、转义、空参数；不再用 `substr` + `strip` 手拆。
2. **应用状态对象**：`AppState { mode, current_session_id, pending, style, ... }`，`Session` 持久化 `active_mode`。`mode` 不再靠局部变量和全局变量双写。
3. **输入控制层**：`InputController` 封装 `readline.input`、审批输入、取消检查；pty 测试可注入 fake。请求/工具/压缩期间通过短轮询检查取消标志。
4. **保存边界**：不再只在循环顶部保存；`/reset`、`/switch`、`/new`、`/rename`、`/pause`、`/unpause`、agent checkpoint 都触发保存。失败必须显示 `(save failed: ...)`。
5. **命令拆分**：把 `/sessions`、`/switch`、`/resume`、`/unpause`、`/continue` 等会话命令移到 `commands_session.kl` 或 `commands.kl`；`repl.kl` 只保留循环和 TUI 粘合。
6. **启动警告可见**：`SessionManager` 的坏文件警告不能早于 TUI；改为收集 `startup_warnings`，`run_repl` 初始化 chrome 后再 emit。

**任务**

- 新增 `commands.kl`：`CommandSpec`、`command_specs()`、`dispatch(line, ctx)`、`parse_args`。
- `repl.kl`：`run_repl` 缩到输入循环 + 绘制；命令执行改为 `dispatch`。
- `repl.kl`：`_status_mode` 改为从 `AppState` 读取；`Session.active_mode` 持久化。
- `repl.kl`：`_ChatCompleter` 增加 `invalidate_models()`，`/model` 后调用。
- `repl.kl`：`poll_resize` 在审批、agent 步间、压缩前后、工具执行前后都调用。
- `repl.kl`：`/clear` 与 `/reset` 后显式保存/更新 current。
- `repl.kl`：`request_approval` / `_approval_input` 抽出 `ApprovalIO`，默认真实终端，测试用内存实现。
- `test_repl.kl`：命令表与 dispatch 一致性、参数解析、active_mode 恢复、complete 缓存失效、startup warning 顺序。

**验收**

- 新增一个命令只需改 `CommandSpec` 表，不会忘记补全或 help。
- `/agent` 后重启仍进入 agent 模式（如果 `active_mode` 持久化）。
- `/model` 后 Tab 补全立即反映新模型。
- 坏会话警告在 TUI 和 plain 下都可见，不会在 alt-screen 切换时丢失。
- `/reset`、`/switch` 后立即落盘；保存失败有明确提示。

### 4.4 `agent.kl`：按 M2/M3 重写，模块只保留编排

**现状**

- 系统提示词要求“一次只发一个 fenced block”（`agent.kl:18`），`parse_block` 只取第一个块（`agent.kl:214-235`）。
- 固定 `while step < cfg.agent_max_steps`（`agent.kl:100`），到上限只提示停止（`agent.kl:155`），没有 `/continue`。
- 非流式请求（`agent.kl:103`），没有 tool_calls、没有多工具、没有流式显示。
- 工具结果以 `role: "user"` 回传（`agent.kl:153`），没有 `tool_call_id`，不符合 OpenAI tool 协议。
- 只有 `write` / `shell` / `done` 三个动作（`agent.kl:282-307`），没有 Read/Edit/Glob/Grep/TodoWrite。
- `os.output` 无超时、无输出上限（`agent.kl:299`）；大输出直接进入上下文和 transcript。
- `write` 直接 `fs.write_str`（`agent.kl:291`），非原子，也不创建父目录。
- 写/壳审批只覆盖 `write` 和 `shell`（`agent.kl:162-173`）；shell 内部的路径访问无法被 `path_blocked` 拦截。
- `allow_all` 只在当前 Agent 实例内有效（`agent.kl:44`、`agent.kl:178-187`），重启后恢复为逐个询问，这是安全默认，但要写清楚。
- `parse_block` 接受未闭合 fence（`agent.kl:231`），也接受 ` ```python` 这类未知 info，然后报 `unknown action`；模型的普通 Markdown 代码块容易被误判。
- `messages.pop()` 只在请求失败时回滚 user task（`agent.kl:105-108`），没有处理“assistant 已追加、tool 执行失败、进程退出”的半完成状态。
- `restore_agent` 先建带 system prompt 的 Agent 再 clear（`agent.kl:203-209`）；如果保存的 agent 消息为空或缺少 system，需要显式校验。

**目标**

- `Agent` 只负责：请求模型、解析 tool calls、调用 `tools.dispatch`、追加消息、checkpoint、停止条件。
- 新增 `tools.kl`：工具 schema、名称归一化、路径 guard、执行、结果格式化、超时/截断。
- 使用 `agent_loop_plan.md` M2 的统一 `run_turn`；`chat` 只是 `tools=[]` 的特例，暂不强制合并上下文。
- 结构化 `ToolCall` / `ToolResult`，不再用字符串协议。
- 保留 fenced fallback，但只在 endpoint 不支持 tools 或显式配置时启用。

**任务**

- `agent.kl`：删除 `run_action` / `parse_block` 的主路径依赖，改为调用 `tools.dispatch`。
- 新增 `tools.kl`：`Read`、`Write`、`Edit`、`Bash`、`Glob`、`Grep`、`TodoWrite`。
- `Bash` 用 `os.run` + `Process.communicate(timeout)`；超时 `terminate()`，再 `kill()`；输出截断到 `max_tool_output_bytes`，完整日志写临时文件。
- `Write` 复用 `action_target` / `path_blocked`，并改为临时文件 + rename（与 session 原子写共用 helper）。
- `Edit` 必须唯一匹配，支持 `replace_all`，返回短 diff。
- `Read` 带行号、offset/limit，二进制/超大文件明确报错。
- `Agent` 增加 `on_checkpoint` 回调；每个 assistant/tool 消息后调用。
- `Agent` 增加 `pending` / `run_state` 接口，供 `/continue` 使用。
- `test_agent.kl`：工具单测 + mock LLM 多步 loop；`test_pty.sh` 增加多步工具展示。

**验收**

- 一轮两个 tool calls 都执行，结果带正确 `tool_call_id`。
- Bash 超时能在有限时间内返回，不挂住测试。
- 大输出截断后模型仍能拿到可操作的 head/tail。
- 旧 fenced fallback 测试保留并通过。

### 4.5 `compact.kl`：pair-safe、rich token、失败可见

**现状**

- 只在 assistant 回复后调用 `maybe_compact`（`agent.kl:113-117`、`session.kl:168-172`），工具结果之后不检查。
- `KEEP_RECENT = 4` 是固定消息条数（`compact.kl:18-19`），会从中间切断 `assistant(tool_calls)` 与对应 `tool` 结果。
- `_compact` 假设 `messages[0]` 一定是 system（`compact.kl:96-100`），没有校验。
- `estimate_tokens` 只看 `content`（`compact.kl:65-71`），不看 tool name/arguments/result。
- `_summarize` 只拼 `role + content`（`compact.kl:105-115`），rich message 会丢失工具信息。
- `_summarize` 不限制输入长度，长历史可能再次超窗。
- `maybe_compact` 摘要失败时静默返回 nil（`compact.kl:31-36`）；`compact_now` 有错误提示（`compact.kl:53-55`），但两者行为不一致。
- `resp.choices[0]` 无条件索引（`compact.kl:115`），空 choices 会 panic。

**目标**

- 按 **turn group** 切分：一个 group 是 `user`、`assistant`、`assistant(tool_calls)` + 对应 `tool` 结果、后续 assistant 的完整链。
- 保留最近 N 个 group，而不是 N 条消息。
- token 估算覆盖 content、tool name、arguments、tool result、name。
- 请求前预检：`last_prompt_tokens + estimate(new_messages)` 接近上限就压缩。
- 摘要失败必须 emit 错误；自动压缩失败不能静默继续。
- 摘要输入有上限；超长时先截断旧工具输出，保留头尾和文件路径。
- 摘要请求用 `tools=[]` 的普通 create，避免 endpoint 对 tools 的差异。

**任务**

- `compact.kl`：新增 `message_group(messages)` / `compact_groups(messages, keep_groups)`。
- `compact.kl`：`estimate_tokens` 接受 rich message；未知字段按 JSON 长度估算。
- `compact.kl`：`_summarize` 生成带工具调用的摘要文本，限制输入字节数。
- `compact.kl`：空 choices、summary 为空、请求失败都返回明确错误。
- `agent.kl`：每个 tool result 后调用 `maybe_compact`；请求前预检。
- `test_agent.kl`：rich token 估算、tool 配对保留、空 choices、summary 失败、超长摘要输入。

**验收**

- 压缩后不会出现孤立 `tool` 消息。
- 大工具输出后触发压缩，下一轮请求不超窗。
- `compact_note` 的前后 token 与估算一致，不再用“最后一条 assistant 的 content 长度”误导。

### 4.6 `session.kl` / `sessions.kl`：以 `session_plan.md` 为准，补三个模块级问题

会话持久化的 v2 schema、原子保存、迁移、`.current`、`/resume`、`/continue` 已在 `session_plan.md` 详述。这里只补三个容易遗漏的模块级问题：

1. **`/file` 大文件会 panic**：`Session.load_file` 直接 `fs.read_str(path, cfg.max_code_bytes)`（`session.kl:108`），而 `fs.read_str` 超过 `max` 会 panic（`libs/std/fs/__fs__.kl:583-585`）。应先 `fs.stat` 判断大小，超限返回可读错误，不要进入读取。
2. **流式回复中途失败被当成成功**：`ask` 只在 `stream.ok()` 失败时回滚（`session.kl:124-129`）；如果已经收到部分 delta 后读取失败，仍会把部分 content 追加为完整 assistant 消息（`session.kl:133-165`）。应检测 `ChatStream` 的结束状态/错误，标记 `interrupted`，并把已收到的部分保存为可恢复消息。
3. **`list_models` 的解析同样会 panic**：`parse_str`（`session.kl:313`）、`get_array("data")`（`session.kl:317`）在 200 但非 JSON / data 非数组时崩溃。应使用 safe parse + 类型检查，并在 `/models` 显示错误原因。

另外，`status_line` 永远读 `_agent` token（`session.kl:274-281`），chat 模式会显示 0 或旧值；`show_context` 虽然按 `is_agent` 分支（`session.kl:218-230`），但 `status_line` 没有这个参数。这个修复属于 `session_plan.md` S3，但实现时不要只改 `show_context`，忘记状态行。

`branch()` 每次工作目录变化会同步执行 `git rev-parse`（`session.kl:257-269`），没有超时；在超大仓库或网络文件系统上，状态行可能卡住。建议给 `os.output` 加短超时，失败就显示无分支。

### 4.7 `__chat__.kl`：启动顺序、CLI 参数与错误边界

**现状**

- 顺序是 `Config` → `resolve_key` → `SessionManager` → `run_repl`（`__chat__.kl:93-98`）。
  - `Config` 失败 panic。
  - 无 key panic。
  - `SessionManager` 构造时就会 `emit` 坏会话警告（`sessions.kl:118-125`），此时 TUI chrome 还没创建，警告会走 `print`；随后 alt-screen 进入，首屏看不到这些警告，只有退出后才可能回到原屏幕。
- `--config` 是手写扫描（`__chat__.kl:100-112`），不支持 `--help`、`--version`、`--session-dir`、`--resume`、`--continue`、`--new`。
- 未知参数被静默忽略；`--config` 缺参数才 panic。
- 没有顶层错误处理；任何 `panic` 依赖 readline native shim 恢复终端，退出码也不可控。
- 启动欢迎语在 `run_repl` 内、chrome 创建后（`repl.kl:552`），这点是对的；但坏会话警告更早。

**目标**

- 用 `koala/cli.ArgumentParser` 替代手写参数解析（`libs/koala/cli/parser.kl:76-181`）。
- 启动顺序改为：
  1. 解析参数；
  2. 加载配置（失败显示可读错误 + 退出码 2）；
  3. 解析 key（可取消，退出码 2）；
  4. 创建 SessionManager，但只收集 warnings，不 emit；
  5. 创建 TUI；
  6. 回放 warnings；
  7. 进入 REPL。
- `--help` / `--version` 不加载配置，直接退出 0。
- 顶层 `main` 用 `match`/结果分支处理错误，避免未捕获 panic；无法避免的 panic 也要在 `finally` 语义下 `screen.leave()`（Koala 无 try/finally 时用显式 `leave` 路径 + native shim 兜底）。

**任务**

- `__chat__.kl`：新增 `CliOptions`，使用 `koala/cli`。
- `__chat__.kl`：新增 `startup()` 返回统一结果；`main` 只做退出码映射。
- `sessions.kl`：`SessionManager` 增加 `warnings()`，构造时不再直接 emit。
- `repl.kl`：`run_repl` 接收 warnings 并在 chrome 创建后输出。
- `test_config.kl` / `test_sessions.kl`：启动顺序、warnings、`--help`、未知参数、错误退出码。

**验收**

- 配置文件缺失时，用户看到路径、原因和修复提示，而不是只有 panic 文本。
- 坏会话警告在 TUI 首帧可见。
- `--help`、`--version`、`--config` 行为有测试；未知参数被拒绝。

### 4.8 测试模块：补齐 mock LLM 与真实边界

**现状**

- `test_agent.kl` 主要测 `parse_block`、`action_target`、`run_action`，loop 只做传输失败 smoke（`test_agent.kl:297-314`）；注释说 loop 由 pty E2E 覆盖，但 `test_pty.sh` 只测 `/help`、`/quit`、resize、non-tty（`test_pty.sh:75-150`）。
- `test_session.kl` 的 `ask` 只测连接失败（`test_session.kl:107-119`）。
- `test_sessions.kl` 覆盖 store/manager/restore，但没有 malformed JSON 不 panic、原子写失败、v1 迁移。
- `test_config.kl` 把“缺字段 panic”当作契约（`test_config.kl:125-204`）；改为 `try_load` 后需要调整。
- `test_ui.kl` 覆盖充分，但 native 工具结果、多工具、current+paused 未覆盖。
- 没有 mock OpenAI server，无法断言请求体中的 `messages`、`tools`、`tool_calls`、`tool_call_id`。

**目标**

- 新增 `demo/chat/test/mock_openai.kl`：本地 HTTP/SSE server，按脚本返回预设响应，记录请求体到 `requests.jsonl`。
- 新增 `demo/chat/test/test_agent_loop.sh`：临时 config + mock server + 真实 artifact/测试进程。
- 单元测试覆盖 pure 函数；E2E 覆盖协议；pty 覆盖输入/显示/取消；三者职责分明。

**任务**

- mock server：普通 completion、SSE、tool_calls、多 tool call、malformed JSON、400 context overflow、401、429、超时。
- 测试场景：
  1. `Write → Bash → final`。
  2. 一轮两个 tool calls。
  3. 工具错误 → 模型修正。
  4. 权限拒绝 → 模型换方案。
  5. step cap → `/continue`。
  6. compact 前后 tool 配对完整。
  7. 重启恢复 rich message。
  8. `/file` 路由到 agent。
  9. Ctrl-C 中断后消息序列合法。
- `test_pty.sh` 增加：多步工具、`/sessions`、`/resume`、`/unpause`、Ctrl-C、坏文件警告。
- 所有测试使用 `/tmp` 临时目录，不碰 `demo/chat_sessions/main.json`。

**验收**

- 测试能在没有真实 API key 的情况下跑完整 loop。
- 请求体断言能抓到“工具结果变成 user 消息”这类回归。
- 失败测试会给出请求/响应差异，而不是只报超时。

## 5. 直接依赖模块分析

### 5.1 `libs/koala/json`：safe parse 是硬前置

**现状**

- `parse_str` / `parse_file` 遇到 malformed JSON 直接 `panic`（`libs/koala/json/__json__.kl:14-42`）；`parser.kl` 有大量 `panic` 分支（`parser.kl:45-411`）。
- `Object.get_str` / `get_int` / `get_array` 在缺键或类型不符时 panic（`object.kl:71-90`）。
- 当前 chat、agent、session、config、http 解析都依赖这套 panic 语义。

**目标**

- 新增 `try_parse_str(s) -> ParseResult`（`Value?` + `error` + `offset`）和 `try_parse_file(path)`。
- `Parser` 改为错误状态，不在解析器内部 panic；`parse_str` 保留 panic 包装以兼容旧调用。
- `Object` / `Array` 增加 `try_get_*`，返回 `nil` + 错误，不复制第二套解析器。
- 错误包含行号/列号、附近文本；对 session 文件和 tool arguments 都能给出可操作信息。

**任务**

- `libs/koala/json`：`ParseResult` / `Parser.try_parse` / `try_parse_file`。
- `libs/koala/json/test`：空输入、尾部内容、深层嵌套、非法转义、截断字符串、错误 offset。
- 迁移顺序：`openai` → `sessions` → `config` → `session.list_models` → `compact`。

**验收**

- malformed JSON 不 panic，调用方能区分“文件不存在 / 解析失败 / 根类型不对”。
- 旧 `parse_str` 行为保持不变，现有测试全绿。

### 5.2 `libs/koala/openai`：rich message 与 tool protocol

**现状**

- `CompletionMessage` 只有 `role` / `content`（`types.kl:61-69`）。
- `ChatChunk` 只有文本 `delta`（`types.kl:89-103`），没有 tool call delta。
- `_build_json` 只写 `role` / `content`（`completions.kl:63-76`），不支持 `tool_calls`、`role: tool`、`tool_call_id`、`tools`、`tool_choice`。
- `_parse_completion` 用 `parse_str` + `(as Object)!`（`types.kl:117`），malformed body 会 panic；空 `choices` 由调用方 `[0]` 触发 panic。
- `_extract_error` 也直接 `parse_str`（`types.kl:184-193`）。
- `ChatStream._next_chunk` 直接 `parse_str`（`completions.kl:207`），没有 tool delta；非 JSON 事件被跳过。
- `transport._post` 对 `conn.write` 只调用一次，短写会静默截断请求（`transport.kl:38-51`）；`_read_all` 无上限。
- `Client.__init__` 非法 base_url 静默回落到 OpenAI 官方地址（`client.kl:25-36`）。

**目标**

- 新增 `ToolCall`、`FunctionCall`、`ToolDef`、`FunctionDef`、`ToolChoice`。
- `CompletionMessage` 增加 `tool_calls`、`tool_call_id`、`name`；`content` 允许 `nil`。
- `ChatChunk` 增加 `tool_calls` delta（按 `index` 聚合 `id` / `name` / `arguments`）。
- 新增 `create_tools` / `stream_tools`，保留旧 `create` / `stream` 兼容。
- `_build_json` 支持 `assistant.tool_calls`、`role: tool`、`tool_call_id`、`tools`、`tool_choice: "auto"`。
- 所有响应解析走 safe parse；空 choices、null content、malformed chunk 都返回明确错误。
- `_post` 循环写完整 body；请求体大小上限；response 大小上限。

**任务**

- `openai/types.kl`：rich message、ToolCall、ToolDef、ChatChunk delta。
- `openai/completions.kl`：`create_tools` / `stream_tools`、`_build_json`、流式聚合。
- `openai/transport.kl`：完整写、超时、错误分类。
- `openai/client.kl`：非法 base_url 拒绝。
- `openai/test/test_tools.kl`：请求体、非流式、流式多 tool call、参数跨 chunk、malformed。

**验收**

- 请求体能包含 `tools` 与 `tool_choice`。
- 能解析 `finish_reason: "tool_calls"` 和多个 tool call。
- 流式 tool call 参数跨 chunk 能正确拼接。
- 旧 `create` / `stream` 行为不回归。

### 5.3 `libs/koala/readline`：输入期间的中断与按键轮询

**现状**

- `readline.input` 只在编辑行时处理 Ctrl-C，返回 `""`（`editor.kl:175-191`、`editor.kl:304-313`）。
- `KeyHook` 只在编辑行内调用（`editor.kl:336-345`），请求/工具执行期间没有输入循环。
- `readline` 的终端是阻塞读（`key_reader.kl:1-40`），没有公开的非阻塞/超时读取 API。
- `issue-85` 已记录“读一个键会阻塞线程”等遗留项。

**目标**

- 增加 `readline.poll_key(timeout_ms) -> Key?` 或等价的 stdin 就绪检查，供 agent 在工具/请求期间检查 Ctrl-C。
- 不要求线程；要求能“短轮询 + 可取消”。
- `Esc` 与 `Ctrl-C` 的语义分开：`Esc` 取消当前生成，`Ctrl-C` 第一次中断、第二次退出 loop。

**任务**

- `readline`：新增非阻塞/超时按键读取 API；保留现有 `input`。
- `readline`：Ctrl-C 在编辑行外也能被上层检测（或由 shim 设置取消标志）。
- `repl` / `agent`：在请求 chunk 间、工具执行前、压缩前后轮询取消标志。
- 测试：MemoryTerminal 模拟 Ctrl-C、超时、无输入。

**验收**

- 请求/工具执行中按 Ctrl-C 能在有限时间内停止。
- 输入行内 Ctrl-C 仍然清空当前行，不退出程序。

### 5.4 `libs/koala/tui`：inline 模式与内存上限

**现状**

- `Screen` 默认进入 alternate screen（`screen.kl:44-68`），`leave` 后最后帧留在 scrollback，但历史不可回看。
- `frame_start(clear=false)` 仍会 `ESC[r ESC[H`（`screen.kl:97-106`），普通轮次有重绘成本。
- `WrappedView` 保存全部逻辑行和换行缓存（`wrapped_view.kl:16-60`），长会话内存无上限。
- 终端小于 10 行时整体降级 plain（`repl.kl:542-544`、`repl.kl:558-565`），且不可恢复。

**目标**

- v1.1 增加 `ui_mode=inline`：不使用 alt-screen，直接追加到 scrollback，保留原生滚动。
- 保留 alt-screen 作为可选模式；默认仍可先保持现状，避免一次改太大。
- `WrappedView` 增加最大行数/最大字节数，超出时丢弃最旧行或折叠为“前面还有 N 行”。
- resize 时只重排可见窗口，不重排全部历史（或对历史做惰性缓存）。

**任务**

- `tui`：`Screen` 增加 inline 构造参数；不进入 alt-screen。
- `tui`：`WrappedView` 增加 `max_lines` / `max_bytes` / `trim`。
- `repl`：`ui_mode` 配置与 `/ui` 命令（可选）；plain/alt/inline 三态测试。
- `test_pty.sh`：inline 模式下 `/clear`、resize、长输出、退出后 scrollback。

**验收**

- 退出后仍能在终端 scrollback 看到完整对话（inline 模式）。
- 长会话不会无限增长内存。
- 小终端降级后不会设置非法滚动区。

### 5.5 `libs/koala/http` / `libs/koala/sse`：可取消流与错误区分

**现状**

- `StreamResponse.read` 把读超时、EOF、解析失败都归为 `nil`（`http/stream.kl:73-104`），调用方无法区分“正常结束 / 超时 / 被关闭”。
- `StreamResponse.close` 能释放连接（`http/stream.kl:106-113`），但从同一线程阻塞在 `read` 时无法调用。
- `SSEIter._advance` 直接 `_response.read()`（`sse/event_source.kl:101-140`），阻塞到下一个 chunk。
- `connect_sse` 默认 180s 超时（`sse/__sse__.kl:36-49`），一次超时就会结束整个流。
- `http` 客户端强制 `Connection: close`（`http/client.kl:75-80`），没有 keep-alive；对 chat 可接受，但会增加延迟。

**目标**

- `StreamResponse` 增加 `last_error()` / `is_timeout()`，区分 `eof`、`timeout`、`closed`、`parse_error`。
- 增加“短读超时 + 取消检查”模式：每次 `read` 最多阻塞 N ms，超时返回可重试状态；上层检查取消标志后继续或关闭。
- `close` 可从取消路径调用；如果跨线程不可用，则由同线程轮询实现。
- SSE 解析保持增量，不因一次超时丢弃已收到的 buffer。

**任务**

- `http/stream.kl`：错误状态与 `read_with_timeout`。
- `sse/event_source.kl`：透传 timeout/closed 状态。
- `openai`：`ChatStream` 暴露 `error_kind()`；`stream_tools` 在取消时保留已聚合的 tool calls。
- 测试：mock server 延迟发送、半关闭、超时、取消。

**验收**

- 流式回复中途网络断开时，UI 明确显示“连接中断”，不伪装成正常完成。
- Ctrl-C 关闭流后，不会继续等待 180s。
- 已收到的部分文本/tool call 不会丢。

### 5.6 `libs/std/fs` / `libs/std/os`：已有原语，模块层要用对

**已确认可用**

- `fs.open(path, "x")`（`fs/__fs__.kl:35`）可做独占创建锁。
- `fs.File.write_str` / `sync` / `close`（`fs/file.kl:54-90`）可做临时文件落盘。
- `fs.rename` 覆盖目标文件（`fs/__fs__.kl:153-160`），可用于原子替换。
- `os.getpid` / `os.kill(pid, 0)`（`os/__os__.kl:180-182`、`os/__os__.kl:321-328`）可做陈旧锁检测。
- `Process.communicate(input, timeout)`（`os/process.kl:65-95`）已支持超时，`terminate` / `kill` 可用（`os/process.kl:97-103`）。
- 因此 agent 的 Bash 超时、session 的原子保存、锁文件都**不需要新增 native API**，只需要在 `agent.kl` / `sessions.kl` 正确调用。

**缺口**

- `std/os` 仍不能安装信号处理器（`issue-65`），所以“请求中 Ctrl-C 回调”要靠 readline 轮询或 native 取消标志；不能只等 `os.on_signal`。
- `fs.read_str` 超限 panic（`fs/__fs__.kl:583-585`），模块层必须先 `stat`。
- 没有 `fsync` 目录的封装；rename 后目录项持久化在极端断电下不保证。第一版可接受，文档要写清楚。

**任务**

- 新增共享 `atomic_write(path, data) -> bool`（`session`、`Write` 工具共用）。
- 新增 `FileLock`（`open(path, "x")` + PID + `kill(pid, 0)`），锁冲突给出明确提示。
- `tools.Bash` 使用 `Process.communicate(timeout)`，超时后 `terminate` / `kill` / `wait`。
- 测试：原子写失败保留旧文件、锁冲突、陈旧锁、Bash 超时。

**验收**

- 保存会话和写文件都不会留下半个目标文件。
- 两个进程同时写同一会话时，第二个得到提示。
- Bash 超时不挂住测试进程。

### 5.7 `koala/cli` / `koala/log` / `std/time`

- `koala/cli.ArgumentParser` 已具备 `add_argument`、`parse_args`、`try_parse_args`、`format_help`（`cli/parser.kl:76-181`），应直接用于 `__chat__.kl`，不再手写 `--config`。
- `koala/log` 有 `Logger`、级别、文件 handler；可用于把 tool/agent 诊断写入 `chat.log`，但不要替代 transcript。建议默认关闭，`--debug` 或配置开启。
- `std/time` 的 `Instant` / `Duration` 可用于工具耗时、请求耗时、状态行；`koala/datetime` 用于会话时间戳。统一用 epoch 毫秒存储，显示时再转本地时间。

### 5.8 缺失模块：`tools.kl` 与 rich message 类型

建议新增：

- `demo/chat/tools.kl`：工具定义、dispatch、路径 guard、输出截断。
- `demo/chat/message.kl`（或直接放在 `libs/koala/openai/types.kl`）：rich message 的构造、校验、JSON 编解码。
- `demo/chat/commands.kl`：`CommandSpec` 与 dispatch。
- `demo/chat/app.kl`（可选）：`AppState`、startup、session/agent 路由。

这些模块不是“为了分层而分层”，而是当前三个最大单体的自然拆分点：`agent.kl` 的工具、`repl.kl` 的命令、`session.kl` 的消息。

## 6. 实施阶段

阶段编号用 `C0–C6`，避免和 `agent_loop_plan.md` 的 `M0–M8`、`session_plan.md` 的 `S0–S6` 混淆。

### C0. 基线与契约（0.5 天，现在可做）

- 用 `/tmp` 临时目录建立模块基线：config、repl、agent、compact、ui、sessions。
- 记录当前已知失败/panic 为 reproducer：malformed session、`/file` 大文件、空 choices、mid-stream 断开。
- 明确四个跨模块决策：
  1. 消息类型由 `openai` 定义还是 chat 定义？
  2. 配置错误用结果类型还是 panic 兼容层？
  3. `/reset` 清几个上下文？
  4. 默认 UI 模式 alt-screen 还是 inline？
- 不改生产代码；不碰运行时 `demo/chat_sessions/main.json`。

**验收**：测试只读写 `/tmp`；基线命令全绿；决策记录在文档/issue。

### C1. 基础层：safe JSON + config + OpenAI rich（依赖 M1）

- `json.try_parse_*` 落地；`openai` 改用 safe parse。
- `openai` 增加 rich message / tool protocol；旧 API 兼容。
- `config.try_load`、范围校验、默认值、非法 base_url 拒绝。
- `__chat__` 用 `koala/cli`，启动顺序改为“配置 → key → manager warnings → TUI”。

**验收**：

- malformed JSON 不 panic。
- 配置错误有字段名和修复提示。
- `create_tools` / `stream_tools` 单测通过。
- `--help` / `--version` / `--config` 可用。

### C2. Agent 与 tools（依赖 M2/M3）

- `agent.kl` 改为统一 `run_turn`；fenced fallback 保留。
- 新增 `tools.kl`：`Read/Write/Edit/Bash/Glob/Grep/TodoWrite`。
- Bash 超时、输出截断；Write 原子写；路径 guard 统一。
- `on_checkpoint`、`run_state`、`pending` 接口。
- mock LLM 多步 loop 测试。

**验收**：

- 多 tool call、工具错误修正、权限拒绝、step cap 都能跑。
- Bash 超时不挂测试。
- 工具结果带 `tool_call_id`。

### C3. Compaction（依赖 M6/C2）

- `estimate_tokens` 覆盖 rich message。
- 按 turn group 压缩，保留 tool 配对。
- 请求前预检；工具结果后检查。
- 摘要失败可见；输入长度限制。

**验收**：

- 压缩后无孤立 tool 消息。
- 大工具输出后下一轮不超窗。
- 空 choices / 摘要失败有错误提示。

### C4. 会话持久化（依赖 S1/S2/S3）

- `session_plan.md` 的 v2 schema、原子保存、迁移、`.current`。
- rich message 持久化；checkpoint 保存点。
- `/file` 路由到活动上下文；per-context token。
- `/unpause` 与 `/resume` 语义拆分。

**验收**：

- 重启后 tool 历史完整。
- 坏文件不崩，保存失败保留旧文件。
- chat/agent 状态行 token 正确。

### C5. REPL 命令与应用状态（依赖 C2/C4）

- `CommandSpec` 注册表与 `dispatch`。
- `AppState` / `active_mode` 持久化。
- `InputController` 与取消标志；请求/工具/压缩期间可中断。
- 模型补全缓存失效；保存边界；`/continue`。
- 启动 warnings 在 TUI 后输出。

**验收**：

- 命令表与执行一致；未知命令本地拦截。
- `/model` 后补全更新。
- Ctrl-C 能停止长任务并保持消息合法。
- `/reset` / `/switch` 立即落盘。

### C6. UI、CLI 与 TUI 打磨（P2）

- `ToolCallView` / `ToolResultView`；多工具块；Ctrl-O 多输出。
- `session_lines` 增加 active/updated/corrupt。
- `http_error` 扩展错误表；diff 增加文件头/行号。
- `ui_mode=inline`、`WrappedView` 上限、resize 优化。
- `koala/log` 可选调试日志；`koala/cli` help 文案。

**验收**：

- native 工具结果不再误判失败。
- 80/40 列、plain、CJK、inline 不回归。
- 长会话内存有上限。

## 7. 测试计划

### 7.1 单元测试

| 模块 | 必测 |
|---|---|
| `config` | 默认值、范围、非法 base_url、key 文件、`try_load` 错误、`@should_panic` 兼容层 |
| `json` | malformed、空输入、尾部内容、截断字符串、错误 offset |
| `openai` | tools 请求体、非流式 tool_calls、流式多 tool call、参数跨 chunk、空 choices、null content |
| `agent` | `parse_block` 保留、tool dispatch、路径 guard、Bash 超时、输出截断、step cap、pending |
| `compact` | rich token、turn group、tool 配对、空 summary、超长输入 |
| `ui` | native 工具结果、多工具、current+paused、400/500/0、diff 文件头 |
| `repl` | command table vs dispatch、参数解析、active_mode、cache invalidation、warnings |
| `sessions` | v1 迁移、malformed 不 panic、原子写失败、锁冲突 |

### 7.2 E2E（mock LLM）

1. `Write → Bash → final`，请求体断言 `assistant.tool_calls` 与 `role: tool`。
2. 一轮两个 tool calls，结果都带正确 id。
3. 工具错误 → 模型修正。
4. 权限拒绝 → 模型换方案。
5. step cap → `/continue`。
6. 400 context overflow → compact + retry。
7. endpoint 不支持 tools → fenced fallback。
8. 流式文本 + 流式 tool call。
9. Ctrl-C 中断 → 消息合法、重启可继续。
10. 重启恢复 rich history；坏 session 跳过。

### 7.3 pty

- 多步工具展示、`/sessions`、`/resume`、`/unpause`、`/continue`。
- Ctrl-C 在输入、请求、工具执行三种状态下的行为。
- 坏文件警告、保存失败提示不破坏布局。
- 24/10/9 行、24→9 resize、non-tty、`NO_COLOR`、`TERM=dumb`。
- inline 模式退出后 scrollback 完整（实现 C6 后）。

### 7.4 命令

```bash
source env.sh
cd demo && wombat build chat
cd demo && wombat test chat
bash demo/chat/test/test_pty.sh
# 涉及 libs/koala 或 libs/std 时
./build-std-koala.sh
# 全量回归
./test-debug.sh
```

测试必须使用临时目录；禁止写入仓库内 `demo/chat_sessions/`、`demo/.chat_history`。

## 8. 建议 issue 拆分

当前 `issues/issues.summary.md` 下一个可用编号是 `126`；`agent_loop_plan.md` 已建议占用 `126`–`134`，`session_plan.md` 已建议占用 `135`–`140`。因此本文**不立即新建 issue**，先复用已有拆分：

| 本文主题 | 归入 |
|---|---|
| safe JSON、OpenAI rich/tool | `126` |
| Agent loop 重写 | `127` |
| 核心工具集 / `tools.kl` | `128` |
| mock LLM E2E | `129` |
| 权限与 plan mode | `130` |
| 流式 tool call / interrupt | `131` |
| compaction pair-safe | `132` |
| 会话持久化 / 活动上下文 / 命令 | `133` + `135`–`139` |
| config / CLI / startup | 建议 `141`（等 126–140 落地后再分配） |
| repl 命令注册表 / AppState | 建议 `142` |
| ui 结构化工具事件 | 建议 `143` |
| TUI inline / 内存上限 | 建议 `144` |
| 模块级测试硬化 | 建议 `145` |

这样可以避免同一问题被 `agent_loop_plan`、`session_plan`、本文三处重复建条。只有确认 `126`–`140` 已被占用后，才从 `141` 起新建。

## 9. 风险、假设与待确认决策

### 9.1 风险

1. **safe JSON 是硬前置**。没有它，tool arguments、session、config、HTTP body 的 malformed 输入都可能崩进程；本文 C1 不能跳过。
2. **rich message 改动面大**。`openai`、`agent`、`compact`、`session`、`ui`、`repl`、测试都会受影响；必须在一个里程碑内完成并跑全量回归。
3. **endpoint 差异**。不同 OpenAI-compatible 服务对 `tools`、流式 tool call、历史中的 `role: tool` 接受程度不同；必须保留 fenced fallback 和能力探测。
4. **中断能力受运行时限制**。`std/os` 没有信号处理器（issue-65），当前只能靠 readline 轮询或 native 取消标志；不能承诺“任意阻塞点立即取消”。
5. **原子写不等于目录 fsync**。`fs.rename` 能避免半文件，但极端断电下目录项持久化没有封装；第一版文档要写清边界。
6. **shell 无法完全沙箱化**。`path_blocked` 只能保护文件工具；`Bash` 仍需要权限模式、deny 规则和明确提示。
7. **TUI inline 是较大改动**。alt-screen 目前是布局基础；inline 需要重写绘制、滚动和翻页，不应与 C1–C5 同期做。
8. **配置兼容性**。当前 `Config()` 的 panic 契约被测试固定；改成 `try_load` 时要保留兼容层，避免一次性破坏所有调用点。
9. **写放大**。rich message + 每 tool checkpoint + 全量快照可能让长会话写盘变慢；先做 dirty/去抖，event log 作为后手。
10. **Unicode 宽度**。工具输出、diff、路径都可能包含 CJK/emoji；所有新渲染必须走 `readline.width` / `wrap_text`，不能直接 `len()`。
11. **GC 只能手动触发**。运行时的 `gc_collect` 唯一条入口是 `gc.collect()`，分配超过 `KOALA_GC_MAX`（默认 1 GiB）是 panic 而不是回收；长会话必须自己在“能停下来的地方”收。现状：`agent.settle_heap()` 在每 step 结束、turn 结束、`/cancel`、`/compact` 后以及 REPL 每轮输入循环顶部按 50k 块阈值收集。新增长生命周期缓存时，要同时确认它有对应的 stop point。

### 9.2 待确认决策

1. **消息类型归属**：rich message 放在 `libs/koala/openai/types.kl`，还是 `demo/chat/message.kl`？
   - 建议：协议类型放 `openai`，chat 只做持久化映射；这样其他客户端也能复用。
2. **配置错误模型**：`Config.try_load` 返回结果，还是直接 `panic` + 顶层捕获？
   - 建议：结果类型；Koala 没有 try/catch，panic 无法给出结构化错误。
3. **`/reset` 范围**：清 chat + agent 两个上下文，还是只清当前模式？
   - 建议：清两个（见 `session_plan.md`），但保留 workdir/model/name。
4. **默认 UI 模式**：继续 alt-screen，还是改 inline？
   - 建议：第一阶段保持 alt-screen，C6 增加 `ui_mode=inline` 并默认仍可配置；不要在同一批改动里切换默认。
5. **是否合并 chat/agent conversation**：等 M2/M4 后按 endpoint 实测决定；本文不把合并作为前置。
6. **是否引入 `koala/log` 文件日志**：建议可选 `--debug`，默认不写；不要把 transcript 日志和调试日志混在一起。
7. **工具命名**：默认 Claude 风格 `Read/Write/Edit/Bash/Glob/Grep/TodoWrite`，dispatch 大小写不敏感，保留小写别名。

## 10. 验收清单

- [ ] `json.try_parse_*` 落地，malformed JSON 不再导致 chat/session/config 崩进程。
- [ ] `Config.try_load` 对缺字段、类型错误、范围错误、非法 base_url 给出可读错误。
- [ ] `openai` 支持 `tools`、`tool_calls`、`role: tool`、`tool_call_id`、流式 tool delta。
- [ ] `agent` 支持多 tool call、native 协议、fenced fallback、step cap + `/continue`。
- [ ] `tools.kl` 提供 `Read/Write/Edit/Bash/Glob/Grep/TodoWrite`，有路径 guard、超时、输出上限。
- [ ] `compact` 按 turn group 切分，压缩后无孤立 tool 消息。
- [ ] `session` 持久化 rich message、原子保存、坏文件隔离、`.current`。
- [ ] `repl` 命令表与 dispatch 一致；`/unpause`、`/resume`、`/continue` 语义正确。
- [ ] `repl` 在请求/工具/压缩期间支持 Ctrl-C 中断；消息序列保持合法。
- [ ] `ui` 正确渲染 native 工具结果、多工具块、current+paused、400/500/0 错误。
- [ ] `__chat__` 使用 `koala/cli`，`--help` / `--version` / `--config` 可用，启动 warnings 可见。
- [ ] `test_pty.sh` 覆盖多步工具、会话、Ctrl-C、坏文件警告。
- [ ] mock LLM 测试覆盖 `Write → Bash → final`、多 tool call、工具错误、权限拒绝、compact、重启恢复。
- [ ] `wombat test chat`、`test_pty.sh`、`./test-debug.sh` 全绿。
- [ ] 涉及 `libs/koala` / `libs/std` 的改动额外跑 `./build-std-koala.sh`。
- [ ] 不修改运行时 `demo/chat_sessions/main.json`、`demo/.chat_history` 等仓库内状态文件。

## 11. 与现有计划的关系

| 文档 | 负责 |
|---|---|
| `agent_loop_plan.md` | loop 状态机、native tools、工具集、权限、流式、interrupt、compaction、会话 M7 |
| `session_plan.md` | v2 schema、迁移、原子保存、`.current`、`/resume`、`/continue`、活动上下文 |
| `demo/chat/todo.md` | UI Phase 0–4 已完成项与暂不做项 |
| 本文 | 其余模块的边界、依赖、阶段顺序、测试与 issue 归并 |

**建议执行顺序**：C0 → C1 → C2 → C3 → C4 → C5 → C6。

**最小可交付批次**：C0 + C1 + C2 + C3。完成后应达到：

- 不再因 malformed JSON / 配置错误崩进程；
- native 多步工具调用可运行；
- Bash 有超时和输出上限；
- compaction 不切断工具轮；
- mock LLM 能覆盖核心 loop。

`session` 完整恢复、`/resume`、`/continue`、Ctrl-C、inline TUI 放 C4–C6，避免第一版范围失控。