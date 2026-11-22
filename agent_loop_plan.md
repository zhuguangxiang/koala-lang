# Agent Loop 对齐 Claude Code 实施计划

> 状态：待实施
> 范围：`demo/chat` 的 `Agent.run` agent loop、工具调用协议、压缩、会话语义与测试
> 原则：对齐 Claude Code 可观察的 loop 状态机与工具语义；继续使用 OpenAI-compatible endpoint

## 1. 已确认决策

- 启动默认进入 **chat** 模式；通过 `/agent` 进入 agent 模式。
- `agent_max_steps` 默认 **100**；达到上限后提示 `/continue`，不静默停止。
- `/clear` **只清屏**；`/reset` 继续表示清空当前会话历史。
- 当前 `/resume`（恢复自动 compaction）改名为 **`/unpause`**。
- `/resume` 保留给**历史会话恢复**（列出并选择已保存会话）。
- 允许为 **Ctrl-C interrupt** 修改 `readline` / SSE / HTTP 读取路径，实现真正的取消。
- native 工具名默认采用 Claude Code 风格：`Read`、`Write`、`Edit`、`Bash`、`Glob`、`Grep`、`TodoWrite`；dispatch 同时接受小写别名，降低不同模型偏差。

## 2. 目标架构：只保留一个 loop

目标不是“chat loop + agent loop”两套逻辑，而是一个统一 `run_turn`：

- `tools = []`：模型正常回答，一轮结束，退化为 chat。
- `tools != []`：模型返回 tool calls，执行后把结果喂回，继续下一轮，直到没有 tool calls。
- `done` 只作为不支持 tool calling 的 endpoint 的 fallback，不再作为主协议。

目标伪代码：

```text
run(task):
    append user(task)

    while true:
        cancel_check()
        compact_if_needed()

        resp = request(messages, tools)
        if !resp.ok():
            handle_error()

        append assistant(content, tool_calls)

        if tool_calls.len() == 0:
            finish(content)
            return

        for call in tool_calls:
            validate(call)
            decision = permission(call)

            if decision == deny:
                append tool_result(call.id, "permission denied")
                continue

            result = execute(call)
            append tool_result(call.id, result)

        if steps >= max_steps:
            emit "step limit; /continue"
            return
```

Claude Code 与 OpenAI-compatible 的映射：

| Claude Code | OpenAI-compatible |
|---|---|
| `tool_use` block | `assistant.tool_calls[]` |
| `tool_result` block | `role: "tool"` + `tool_call_id` |
| `stop_reason: tool_use` | `finish_reason: "tool_calls"` |
| `end_turn` | 无 tool calls / `finish_reason: "stop"` |
| `max_turns` | `agent_max_steps` 安全上限，不是正常退出条件 |

## 3. 当前实现差距

### P0：不解决就无法算 Claude Code 式 loop

1. **协议只有 `role/content`**
   - `libs/koala/openai/types.kl:61` 的 `CompletionMessage` 没有 `tool_calls`。
   - `libs/koala/openai/completions.kl` 的 `_build_json` 只发送 `role/content`。
   - 模型无法原生调用工具，只能输出 fenced block。

2. **fenced block 协议有硬伤**
   - `demo/chat/agent.kl:119` 只处理第一个 block。
   - 一次只能执行一个动作。
   - 工具调用没有 id，结果用 `role: "user"` 回传。
   - 文件内容里出现三反引号会截断。
   - 模型格式错误时靠 user 消息纠正，不是结构化错误。

3. **JSON parser 会 panic**
   - `json.parse_str` 遇到 malformed JSON 直接 panic。
   - native tool call 的 `arguments` 是模型生成的 JSON，必须能安全失败。
   - 这是 native tool calling 的前置条件。

4. **loop 语义不符合 Claude Code**
   - `demo/chat/agent.kl:100` 固定最多 20 步，到上限直接停止。
   - 没有“模型不再调用工具就结束”的标准停止条件。
   - 没有多 tool call 支持。
   - 没有流式输出。
   - 没有 interrupt / cancel。
   - 没有 retry / context overflow 处理。

5. **工具结果不可控**
   - `demo/chat/agent.kl:299` 用 `os.output` 捕获完整 shell 输出，无上限、无超时。
   - 大输出可能直接撑爆上下文。
   - 命令卡住会卡死整个 agent。

6. **权限与安全只覆盖很窄**
   - 只有 write/shell 前询问。
   - 没有只读工具、edit 工具、路径统一 guard。
   - shell 只能靠 prompt 提醒，没有 deny 规则或沙箱。

### P1：影响正确性和可用性

7. **compaction 时机错误**
   - `demo/chat/agent.kl:112-117` 只在 assistant 回复后检查。
   - 大工具输出之后不检查，下一轮可能直接超窗。
   - `demo/chat/compact.kl:76` 按“最后 4 条 message”切分，rich message 下会切断 `assistant(tool_calls)` 与 `tool` 结果的配对。

8. **token 统计不准**
   - `estimate_tokens` 只看 `content`，不看 tool arguments / tool result。
   - `demo/chat/session.kl:274-280` 状态行永远读 agent 的 token，chat 模式显示错误。

9. **会话上下文割裂**
   - chat 用 `Session.messages`，agent 用 `Agent.messages`。
   - `/file` 和 `@PATH` 永远追加到 chat 的 `messages`，agent 模式看不到。
   - `/agent` 后上下文不连续。

10. **命令语义不对齐**
    - `/resume` 当前是“恢复自动 compaction”，不是恢复历史会话。
    - `/clear` 只清屏，不是清会话。
    - `/reset` 才是清历史。
    - 与 Claude Code 的命令直觉冲突。

11. **持久化弱**
    - 只在 REPL 每轮开始保存。
    - 保存失败不检查。
    - malformed JSON 直接 panic。
    - 写文件非原子、无锁。

12. **测试覆盖不足**
    - `demo/chat/test/test_agent.kl` 注释声称“loop 由 pty E2E 覆盖”，实际 pty 只测 UI/help/quit。
    - 没有 mock LLM 驱动的多步 loop 测试。

## 4. 分阶段实施计划

### M0. 建立 mock LLM 测试床

**目标**：在改 loop 前先有可重复的多步模型响应测试。

任务：

- 新增 `demo/chat/test/mock_openai.kl`
  - 用 `koala/http` 起本地 OpenAI-compatible server。
  - 从脚本文件读取预设响应序列。
  - 支持普通 response 和 SSE stream。
  - 记录每次请求 body 到 `requests.jsonl`，供断言 `role/tool_calls/tool_call_id`。
- 新增 `demo/chat/test/test_agent_loop.sh`
  - 以子进程启动 mock server。
  - 写临时 `config.json`，把 `base_url` 指向 mock。
  - 跑真实 `chat` artifact 或 `wombat test chat`。
- 先写一个“现状基线”测试：
  - 模型返回 `write` block → `shell` block → `done`，证明当前 fenced loop 能跑。
  - 后续 M2 把它替换成 native tool call 版本。

验收：

- mock server 可启动、可接收多轮请求。
- 测试能断言请求体中的消息序列。
- 现有测试全绿。

### M1. OpenAI native tool protocol + safe JSON

**目标**：让 `koala/openai` 真正支持 tools，而不是在 agent 里拼字符串。

任务：

1. **safe JSON**
   - 新增 `json.try_parse_str(s) Value?` 或等价 API。
   - malformed JSON 返回 nil + 错误信息，绝不 panic。
   - 优先改造 `Parser` 为错误状态，避免复制第二套解析器。
   - `openai` 解析 HTTP body / SSE chunk 也改用 safe parse。
   - `sessions.kl` 的 malformed session 文件也改用它，跳过而不是 panic。

2. **types**
   - 新增 `ToolCall`：`id`、`type`、`name`、`arguments`。
   - 新增 `ToolDef` / `FunctionDef`：名称、描述、JSON Schema。
   - `CompletionMessage` 增加 `tool_calls`。
   - `ChatChunk` 增加 tool call delta：`index`、`id`、`name`、`arguments_delta`。
   - 保留旧字段，保证现有 `create/stream` 调用不破。

3. **completions**
   - 新增 `create_tools(...)` / `stream_tools(...)`，保留旧 `create/stream` 作为兼容层。
   - `_build_json` 支持 `assistant.tool_calls`、`role: "tool"` + `tool_call_id`、`tools`、`tool_choice: "auto"`。
   - 解析 `finish_reason: "tool_calls"`。
   - 流式解析按 `index` 累积多个 tool call 的 `arguments` 片段。

4. **错误处理**
   - 空 `choices`、null content、malformed chunk 都不 panic。
   - 返回明确的 `ChatCompletion` 错误。

验收：

- `libs/koala/openai/test/test_tools.kl`：
  - 请求 body 包含 `tools` 和 `tool_choice`。
  - 解析非流式 `tool_calls`。
  - 解析流式分片 tool calls，多个调用、参数跨 chunk。
  - malformed JSON 不 panic。

### M2. 重写 `Agent.run`

**目标**：把 `demo/chat/agent.kl:97` 改成 Claude Code 式状态机。

任务：

1. **消息模型**
   - Agent 内部改用 rich message，至少支持：`role`、`content`、`tool_calls`、`tool_call_id`、`name`。
   - 不再把所有观察结果都塞成 `role: "user"`。

2. **主循环**
   - 追加 user task。
   - 请求模型。
   - 追加 assistant 消息，保留 `content + tool_calls`。
   - 没有 `tool_calls`：输出最终答复并结束。
   - 有 `tool_calls`：逐个执行，追加 `role: "tool"` 结果，继续。
   - 支持一次返回多个 tool calls，先串行执行，保证可预测。

3. **停止条件**
   - 无 tool calls / `finish_reason: "stop"`：正常结束。
   - `finish_reason: "length"`：提示被截断，允许模型继续。
   - `finish_reason: "content_filter"`：中止并报告。
   - step cap：默认 100，显式提示 `(step limit reached; send /continue)`，不要静默停。
   - 新增 `/continue`，在保留上下文的前提下继续。

4. **错误处理**
   - 401/403：立即中止，不重试。
   - 429/5xx：有限重试 + 退避。
   - 400 context length：compact 后重试一次。
   - 400 unknown tools / unsupported parameter：切换到 fenced fallback，重试当前 task 一次。
   - tool 执行失败：作为 tool result 返回，让模型自行修复，不 abort。

5. **fallback**
   - 保留现有 `parse_block` / `run_action` 作为 fallback。
   - native 模式不依赖 `done`。
   - fallback 模式仍使用 `write/shell/done`。
   - 把 `run_action` 重命名或包装为 `run_fenced_action`，避免和 native tool dispatch 混淆。

6. **prompt**
   - 重写 `AGENT_PROMPT`：
     - 明确“使用提供的 tools，不要用文本伪造工具调用”。
     - 告知工作目录、平台、no-access dirs。
     - 告知“先读再改”“修改前确认路径”“失败后读错误再修”。
   - fallback prompt 单独保留。

验收：

- mock 场景 1：`Write` → `Bash` → 无 tool call，最终答复，文件真实存在。
- mock 场景 2：一轮返回两个 tool calls，两个都执行，结果都带正确 `tool_call_id`。
- mock 场景 3：工具返回错误，模型下一轮修正并成功。
- mock 场景 4：权限拒绝，工具不执行，模型收到拒绝结果。
- mock 场景 5：step cap 提示 `/continue`。
- 旧的 fenced fallback 测试仍通过。

### M3. 核心工具集

**目标**：让 agent 真正具备 Claude Code 的基本编码能力。

| 工具 | 作用 | 权限 |
|---|---|---|
| `Read` | 按行读取文件，带行号、offset/limit | 自动允许 |
| `Write` | 整文件写入 | 询问 |
| `Edit` | 精确字符串替换，必须唯一匹配 | 询问 |
| `Bash` | 执行 shell，带 timeout | 询问 |
| `Glob` | 文件模式匹配 | 自动允许 |
| `Grep` | 内容搜索，带文件名/行号 | 自动允许 |
| `TodoWrite` | 维护任务清单 | 自动允许 |

任务：

- 新增 `demo/chat/tools.kl`：
  - `ToolSpec`：名称、描述、JSON Schema。
  - `dispatch(name, args_json) -> observation`。
  - 统一路径解析：相对路径基于 `workdir`，规范化后做 no-access 检查。
  - 统一错误格式：`ERROR: ...` 或结构化文本。
  - 名称大小写不敏感，兼容 `read` / `Read`。
- `Read`：行号输出，限制最大行数/字节数；目录、二进制、超限文件返回明确错误。
- `Write`：复用现有 `action_target` / `path_blocked`；返回写入字节数和路径。
- `Edit`：`old_string` 不存在返回错误；多处匹配且未 `replace_all` 返回错误；成功返回简短 diff。
- `Bash`：
  - 用 `os.run` + `Process.communicate(nil, timeout)`，超时 kill。
  - 输出上限 `max_tool_output_bytes`。
  - 超限时保留 head/tail，并写完整日志到临时文件，把日志路径告诉模型。
- `Glob` / `Grep`：基于 `fs.walk` / `koala/re`；结果数量上限，避免刷屏。
- `TodoWrite`：先做内存态 + 状态行展示，持久化后置。

验收：

- 每个工具都有单元测试：正常、路径越界、空参数、超限、失败。
- shell timeout 测试不会挂住测试进程。
- 大输出截断后模型仍能拿到可操作的信息。

### M4. 权限模型与 plan mode

**目标**：从“y/a/n”升级为 Claude Code 式权限语义。

任务：

- 权限模式：
  - `default`：读自动，写/改/执行询问。
  - `accept_edits`：写/改自动，执行仍询问。
  - `plan`：只允许读工具，禁止写/改/执行。
  - `bypass`：全部自动，仅用于受控环境。
- 配置项：`agent_permission_mode`、`agent_tool_timeout_ms`、`agent_max_tool_output_bytes`、可选 `allow_tools` / `deny_tools`。
- `/permissions` 查看和切换。
- `/plan` 进入计划模式，模型只能读；退出后再执行。
- `a` 仍然表示“本会话后续全部允许”，但不跨会话持久化。
- shell deny 规则：至少支持命令前缀/正则黑名单；明确说明 shell 不能完全沙箱化。

验收：

- plan mode 下 `Write/Edit/Bash` 被拒绝且不执行。
- `accept_edits` 下写自动、shell 仍询问。
- deny 规则命中时返回结构化拒绝结果。
- 所有文件工具都经过 no-access 检查。

### M5. 流式输出与真正的 interrupt

**目标**：接近 Claude Code 的实时体验，并允许用户随时停止。

任务：

- Agent 使用 `stream_tools`：
  - 文本 delta 实时显示。
  - tool call 参数在完整后展示，不显示半截 JSON。
- 多 tool call 的流式累积按 `index` 处理。
- 为 `readline` 增加非阻塞按键轮询，或提供 `select` 等价的 stdin 就绪检查。
- SSE / HTTP 读取增加可中断路径：关闭 response 后读取循环立即退出。
- Ctrl-C 语义：
  - 流式响应中：停止读取并关闭连接；保留已收到的部分 assistant 文本，追加中断标记。
  - 工具执行中：终止子进程/进程组，工具结果返回 interrupted。
  - 若 assistant 已声明 tool calls 但尚未执行：为每个 call 补一条 `tool` 结果（interrupted），保持消息协议完整，再退出本轮。
  - 第二次 Ctrl-C：退出当前 agent loop，回到 REPL。
- 新增 `/cancel` 作为显式取消命令，便于 pty 与脚本测试。
- 中断后不自动重试当前请求；由用户 `/continue` 继续。

验收：

- mock SSE 流能实时显示文本。
- 流式 tool call 能正确组装并执行。
- pty 测试能发送 Ctrl-C，agent 在合理时间内停止，不写坏文件。
- 中断后消息序列仍满足 `assistant(tool_calls) -> tool` 配对要求。

### M6. 上下文与 compaction

**目标**：不再出现“工具输出后刚好超窗”。

任务：

- `estimate_tokens` 覆盖：content、tool name、tool arguments、tool result。
- 请求前检查：`last_prompt_tokens + 新增消息估算`。
- 工具结果后立即检查，特别是大输出。
- compaction 按 turn group 切分：
  - `assistant(tool_calls)` 与对应 `tool` 结果必须一起保留或一起压缩。
  - 不允许留下孤立的 `tool` 消息。
- summary 包含：当前任务、已改文件、关键命令/错误、未完成事项。
- 400 context overflow：compact 后重试一次。
- compaction 失败必须显示错误，不能静默替换成空 summary。

验收：

- 大工具输出后触发 compact，下一轮请求不超窗。
- compact 后最后工具轮仍完整。
- token 估算测试覆盖 rich messages。

### M7. 会话、状态与持久化

**目标**：修复当前会话语义，支撑长任务。

任务：

- rich message 持久化：`tool_calls`、`tool_call_id`、`name`；旧记录只有 role/content 仍可加载。
- `/file` / `@PATH` 注入当前活动上下文：
  - chat 模式进 chat messages。
  - agent 模式进 agent conversation。
- 状态行按当前模式显示 token。
- `/sessions` 统计 chat + agent 消息，显示最近活动。
- 命令语义：
  - `/clear` 只清屏。
  - `/reset` 清空当前会话历史。
  - `/unpause` 恢复自动 compaction。
  - `/resume` 列出并恢复历史会话；新增 `--continue` 启动参数恢复最近会话。
- 保存：
  - 临时文件 + rename 原子写。
  - 检查返回值，失败提示。
  - malformed JSON 跳过并警告，不 panic。
- 可选：单一 conversation，把 chat 视为 `tools=[]` 的 agent turn；这是彻底消除上下文割裂的方案，可放 M7 后段。

验收：

- agent 任务后 `/file` 的内容在下一轮 agent 请求中可见。
- 重启后 tool_calls/tool 结果能恢复。
- 状态行在 chat/agent 模式都显示正确 token。
- malformed session 文件不会导致启动崩溃。

### M8. 高级能力 backlog

这些不阻塞第一版 loop，但属于 Claude Code 能力差距：

- 项目记忆：`KOALA.md` / `.koala/AGENTS.md` 自动注入。
- Todo 持久化与 UI。
- checkpoint / rewind / undo。
- 子 agent / `Task` 工具。
- hooks（工具前后、提交前）。
- MCP client。
- skills / 自定义命令。
- WebFetch / WebSearch。
- 图片、Notebook、PDF 读取。
- 后台 Bash / 长任务管理。
- 多 agent 并行（依赖 Koala 运行时任务/线程）。
- Anthropic API provider adapter（如果以后直接接 Claude）。

## 5. 能力缺失清单

| 能力 | 当前状态 | 影响 | 优先级 | 阶段 |
|---|---|---|---|---|
| Native tool calling | 无 | 无法真正对齐 Claude loop | P0 | M1 |
| safe JSON parse | 无，malformed 会 panic | 模型参数错误会崩进程 | P0 | M1 |
| 多 tool call | 无 | 一次只能一个动作 | P0 | M2 |
| 标准停止条件 | 无，只有 done/step cap | loop 语义不对 | P0 | M2 |
| Read/Edit/Glob/Grep | 无 | 不能读代码、不能精确改 | P0 | M3 |
| Bash timeout/输出上限 | 无 | 卡死/撑爆上下文 | P0 | M3 |
| 权限模式 | 只有 y/a/n | 不能 plan/accept-edits | P1 | M4 |
| plan mode | 无 | 不能先规划后执行 | P1 | M4 |
| 流式 tool call | 无 | 体验差，无法实时 | P1 | M5 |
| interrupt/cancel | 无 | 长任务无法停止 | P1 | M5 |
| compaction 时机 | 只在 assistant 后 | 工具输出后易超窗 | P1 | M6 |
| tool 配对保留 | 无 | compaction 可能切断工具轮 | P1 | M6 |
| rich message 持久化 | 无 | 重启丢 tool 历史 | P1 | M7 |
| `/file` 活动上下文 | 固定进 chat | agent 看不到 | P1 | M7 |
| 会话 resume | 当前是 unpause | 命令语义错误 | P2 | M7 |
| 原子保存 | 无 | 可能写坏 session | P2 | M7 |
| 项目记忆 | 无 | 每次都要重复说明 | P2 | M8 |
| Todo/checkpoint | 无 | 长任务管理弱 | P2 | M8 |
| 子 agent/hooks/MCP | 无 | 生态能力缺失 | P3 | M8 |

## 6. 预计文件改动

| 文件 | 改动 |
|---|---|
| `libs/koala/json/*` | safe parse / try_parse |
| `libs/koala/openai/types.kl` | `ToolCall`、`ToolDef`、rich message、tool chunk |
| `libs/koala/openai/completions.kl` | `create_tools` / `stream_tools`、tools 请求体、tool_calls 解析 |
| `libs/koala/openai/test/*` | tool protocol 单元测试 |
| `demo/chat/agent.kl` | 重写 `Agent.run`，native loop + fallback |
| `demo/chat/tools.kl` | 新增核心工具实现与 schema |
| `demo/chat/compact.kl` | rich message token 估算、pair-safe compaction |
| `demo/chat/session.kl` | 活动上下文、状态行、rich message 持久化 |
| `demo/chat/sessions.kl` | 原子保存、malformed 跳过、resume 选择 |
| `demo/chat/config.kl` | 新增 agent 配置项，默认 step 100 |
| `demo/chat/repl.kl` | `/continue`、`/permissions`、`/plan`、`/cancel`、`/unpause`、`/resume` |
| `demo/chat/ui.kl` | 多工具/流式展示适配 |
| `libs/koala/readline/*` | 非阻塞按键轮询，支撑 Ctrl-C |
| `libs/koala/http/*` / `libs/koala/sse/*` | 可中断读取路径 |
| `demo/chat/test/*` | mock server、loop E2E、工具/压缩/权限/中断测试 |

## 7. 测试与验收

### 单元测试

- `json.try_parse_str`：malformed、trailing content、空输入，均不 panic。
- openai：
  - 请求体含 `tools`。
  - 解析非流式 tool_calls。
  - 解析流式多 tool call、分片参数。
  - 空 choices / null content 不 panic。
- tools：
  - Read/Write/Edit/Glob/Grep/Bash 正常路径。
  - 路径越界、缺失文件、二进制、超限。
  - Bash timeout、输出截断。
- compaction：
  - rich token 估算。
  - tool 轮配对不被切断。
  - 大工具输出后触发压缩。

### E2E（mock LLM）

1. `Write` → `Bash` → final。
2. 一轮两个 tool calls。
3. 工具错误 → 模型修正。
4. 权限拒绝 → 模型收到拒绝并换方案。
5. step cap → `/continue`。
6. context overflow → compact + retry。
7. 不支持 tools 的 endpoint → fenced fallback。
8. 流式文本 + 流式 tool call。
9. Ctrl-C 中断，消息序列仍合法。

### pty

- 多步 agent loop 的 tool block 展示。
- Ctrl-C 中断。
- 窄终端/plain 模式不崩。

### 命令

```bash
source env.sh
cd demo && wombat build chat
cd demo && wombat test chat
bash demo/chat/test/test_pty.sh
./demo/build-and-test.sh
./test-debug.sh
```

## 8. 建议的 issue 拆分

当前 `issues/issues.summary.md` 下一个编号是 `126`，建议按以下顺序建条目：

- `126` safe JSON + OpenAI native tool protocol
- `127` Agent loop 重写（native + fallback）
- `128` 核心工具集（Read/Write/Edit/Bash/Glob/Grep）
- `129` mock LLM 多步 E2E 测试床
- `130` 权限模式与 plan mode
- `131` 流式 tool call 与 interrupt
- `132` rich message compaction / token 估算
- `133` 会话持久化、`/file` 活动上下文、命令语义
- `134` 高级能力 backlog（memory/todo/subagent/hooks/MCP）

## 9. 风险与假设

- **safe JSON 是硬前置**：不先解决，native tool arguments 随时可能崩进程。
- **endpoint 差异**：不同 OpenAI-compatible 服务对 `tools`、流式 tool call 支持不一致；必须保留 fallback 与能力探测。
- **interrupt 涉及 runtime**：需要改 `readline`/SSE/HTTP；已确认允许，但改动面要单独测试。
- **shell 安全边界有限**：无法仅靠 harness 完全沙箱化；第一版以权限询问 + deny 规则为主。
- **rich message 迁移**：旧 session 只有 role/content，加载时必须兼容。
- **默认 chat**：agent 是显式模式，避免普通问答被工具 loop 干扰。
- **工具命名**：默认 Claude 风格，dispatch 大小写不敏感；若特定模型只接受小写，可在 M1 增加别名映射。

## 10. 建议执行顺序

1. **M0** mock 测试床
2. **M1** safe JSON + OpenAI tools
3. **M2** Agent loop 重写
4. **M3** 核心工具集
5. **M4** 权限/plan
6. **M6** compaction
7. **M5** 流式/中断
8. **M7** 会话/状态/持久化
9. **M8** 高级能力

第一版可交付范围建议是 **M0–M3**：能原生多步调用工具、能读写改代码、能跑命令、有 mock E2E。之后再补权限、压缩、流式和会话。

## 11. 实施前检查

- 不改动运行时 `demo/chat_sessions/main.json`。
- 每个里程碑结束运行 `wombat test chat` 与相关单测。
- 涉及 `libs/koala` 的改动额外运行 `./build-std-koala.sh`。
- 每个里程碑完成后更新对应 issue 状态，全部完成后再归档到 `issues/issues.done.md`。
- 旧 openai 测试全绿。