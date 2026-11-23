# Agent Loop 对齐 Claude Code 实施计划

> 状态：M0–M7 已完成；M8 待实施
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

> 状态（2026-11-22，M1–M7 落地后）：第 1、2、3、4 项已解决——native tool protocol、多 tool call、标准停止条件、`/continue`、429/5xx 退避、400 overflow→compact 重试、400 其他→fenced 回落；第 5 项已解决（七个工具、超时、输出上限）；第 6 项已解决（四档权限模式、`/permissions`、`/plan`、deny 前缀）；第 7 项已解决（请求前 guard：`上次实际 prompt_tokens + 新增消息估算`，turn group 切分不切断 assistant/tool 配对，超大工具结果按阈值截断头尾）；第 8 项已解决（`estimate_rich_tokens` 计入 tool name/参数/结果，状态行与 `/context` 按模式取 token）；第 9、10、11 项已解决（`/file` 进活动上下文、命令语义、原子保存 + rich 持久化）。第 12 项已补 mock loop 测试。

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

### M0. 建立 mock LLM 测试床 — ✅ 已完成

**目标**：在改 loop 前先有可重复的多步模型响应测试。

任务：

- 新增 `demo/chat/test/support/mock_openai.kl`（放在 `test/support/` 而非 `test/`：wombat 会把 `test/*.kl` 逐个当测试文件跑）
  - 用 `koala/http` 起本地 OpenAI-compatible server。
  - 从脚本文件读取预设响应序列。
  - 支持普通 response 和 SSE stream。
  - 记录每次请求 body 到 `requests.jsonl`，供断言 `role/tool_calls/tool_call_id`。
- 新增 `demo/chat/test/test_agent_loop.kl`（Koala 测试，不用 .sh：这样才进 `wombat test chat`）
  - 以子进程启动 mock server（Koala 无线程，client 与 server 不能同进程）。
  - 写临时 `config.json`，把 `base_url` 指向 mock。
  - 跑真实 `chat.Agent`，断言磁盘效果与请求消息序列。
- 先写一个“现状基线”测试：
  - 模型返回 `write` block → `shell` block → `done`，证明当前 fenced loop 能跑。
  - 后续 M2 把它替换成 native tool call 版本。

已落地：

- `mock_openai.kl`：脚本字段 `content` / `finish_reason` / `stream` / `chunks` / `status` / `error`；`--port-file` 回报端口，`--requests` 记录请求。
- `test_agent_loop.kl`：`test_loop_write_shell_done`（3 次请求、消息序列、文件真实写入）、`test_stream_from_mock`（SSE + usage）、`test_loop_stops_on_http_error`（401 只请求一次、会话回滚）。

验收：

- mock server 可启动、可接收多轮请求。✅
- 测试能断言请求体中的消息序列。✅
- 现有测试全绿（`wombat test chat` 8 passed）。✅

### M1. OpenAI native tool protocol + safe JSON — ✅ 已完成

**目标**：让 `koala/openai` 真正支持 tools，而不是在 agent 里拼字符串。

任务：

1. **safe JSON** — ✅ 已完成
   - 新增 `json.try_parse_str(s) Value?` 或等价 API。✅
   - malformed JSON 返回 nil + 错误信息，绝不 panic。✅
   - 优先改造 `Parser` 为错误状态，避免复制第二套解析器。✅
   - `openai` 解析 HTTP body / SSE chunk 也改用 safe parse。✅
   - `sessions.kl` 的 malformed session 文件也改用它，跳过而不是 panic。✅

   已落地：
   - `Parser` 增加 `failed` / `err_msg` 状态与 `fail()`，全部语法错误改为置错并早返回；`parse()` 仍按原消息 panic，`try_parse()` 返回 `Value?`。
   - `json.try_parse_str` / `json.try_parse_file`；`Parser.is_failed()` / `error_message()`。
   - `libs/koala/json/test/test_parse_safe.kl`（26 个用例，含 panic 消息不变的回归）。
   - `koala/openai`：响应体、SSE chunk、错误体解析改 safe（畸形 chunk 跳过，错误体形状异常回退 `HTTP <status>`），新增 3 个测试。
   - `demo/chat`：`sessions.load` / `config` / `session.list_models` 改 safe parse，坏文件不再 panic。

2. **types** — ✅ 已完成
   - 新增 `ToolCall`：`id`、`type`、`name`、`arguments`。✅ 落在 `FunctionCall` 嵌套上（同 OpenAI 形状），另加 `name()` / `arguments()` / `args()` 便捷方法。
   - 新增 `ToolDef` / `FunctionDef`：名称、描述、JSON Schema。✅ `ToolDef.of(name, description, schema)`。
   - `CompletionMessage` 增加 `tool_calls`。✅ 同时补 `tool_call_id`、`name`、`has_tool_calls()`。
   - `ChatChunk` 增加 tool call delta：`index`、`id`、`name`、`arguments_delta`。✅ 字段名 `tool_index` / `tool_id` / `tool_name` / `tool_args_delta`，旧 5 参构造不变。
   - 保留旧字段，保证现有 `create/stream` 调用不破。✅

3. **completions** — ✅ 已完成
   - 新增 `create_tools(...)` / `stream_tools(...)`，保留旧 `create/stream` 作为兼容层。✅ 旧接口把 `dict[str, str]` 转成 `CompletionMessage` 后调用新接口。
   - `_build_json` 支持 `assistant.tool_calls`、`role: "tool"` + `tool_call_id`、`tools`、`tool_choice: "auto"`。✅ 改为 `_build_body` + `_message_json` / `_tool_json`；assistant 无文本时省略 `content`，不发送 `null`。
   - 解析 `finish_reason: "tool_calls"`。✅ `Choice.finish_reason` 原样透传；新增 `ChatStream.finish_reason()`，记最后一个非空值。
   - 流式解析按 `index` 累积多个 tool call 的 `arguments` 片段。✅ `ChatStream.tool_calls()` 返回组装结果；同一帧里的多个片段也会累积。

4. **错误处理** — ✅ 已完成
   - 空 `choices`、null content、malformed chunk 都不 panic。✅
   - 返回明确的 `ChatCompletion` 错误。✅
   - 附带修复：`_parse_completion` 不再用会 panic 的 `get_array` / `get_object`，改用安全的 `_get_arr` / `_get_obj`。

   已落地（验收）：
   - `libs/koala/openai/test/test_tools.kl`（8 个用例）：rich message 类型与 `args()` 容错；请求体含 `tools` / `tool_choice` / `tool_calls` / `tool_call_id`；`tools=[]` 时不发字段；流式两个 call 且参数跨 chunk；`choices: null` 与 `tool_calls: "oops"` 帧被跳过；空 `choices` 响应不 panic。
   - 非流式解析用子进程客户端 + 父进程 `http.Server` 验证（`create_tools` 阻塞，单线程无法同进程自测）。

   偏差：
   - 未新增 `ToolChoice` 类型，`tool_choice` 是字符串参数（`"auto"` / `"none"` / `"required"` / `""` 省略）。
   - `CompletionMessage.content` 保持 `str`（响应缺 content 时为 `""`），请求侧 assistant 无文本时省略字段。

验收：

- `libs/koala/openai/test/test_tools.kl`：
  - 请求 body 包含 `tools` 和 `tool_choice`。
  - 解析非流式 `tool_calls`。
  - 解析流式分片 tool calls，多个调用、参数跨 chunk。
  - malformed JSON 不 panic。

### M2. 重写 `Agent.run` — ✅ 已完成

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

- mock 场景 1：`Write` → `Bash` → 无 tool call，最终答复，文件真实存在。✅
- mock 场景 2：一轮返回两个 tool calls，两个都执行，结果都带正确 `tool_call_id`。✅
- mock 场景 3：工具返回错误，模型下一轮修正并成功。✅
- mock 场景 4：权限拒绝，工具不执行，模型收到拒绝结果。✅
- mock 场景 5：step cap 提示 `/continue`。✅
- 旧的 fenced fallback 测试仍通过。✅（改为 400 回落用例覆盖）

已落地：

- 新增 `demo/chat/tools.kl`：`agent_tools()` 声明 `Write` / `Bash` 两个 `ToolDef`（JSON Schema + `tool_names()`）；`tool_action(name, call)` 把 native call 归一成既有的 `(info, body)`，复用 `run_action` 的描述/守卫/审批；名字大小写不敏感，缺字段或坏 JSON 返回 nil。
- `agent.kl`：`messages` 改为 `list[openai.CompletionMessage]`；`run()` 记 mark、失败回滚，`resume()` 支撑 `/continue`；`_loop` 用 `cfg.agent_max_steps` 上限且可续跑，429/5xx 退避重试两次，400 + 溢出字样先 `compact_rich_now` 再重试一次，400 其他原因切 fenced 重试一次，401/403 立即中止。
- `_step_native`：追加 assistant（保留 `content` + `tool_calls`），按序执行并追加配对的 `role: "tool"` + `tool_call_id`（未知工具也给配对结果，避免请求非法），无 tool call 即收尾；`finish_reason: "length"` 打提示后按无 tool call 收尾（可用 `/continue` 续），`content_filter` 直接中止。
- `_step_fenced`：保留 `write/shell/done` 降级路径；`_switch_to_fenced()` 换 system prompt 并提示一次“本端点拒绝 tools”。
- `_do_action` 统一 native / fenced 的审批 + 执行；被拒时返回“已被用户拒绝，不要重试”的观察结果。
- prompt：`agent_prompt()` / `fenced_prompt()` 由 `_prompt_with` 组装，注入工作目录、`os.platform()`、no-access 列表与“用 tools、别用文本伪造调用”的约束。
- `compact.kl`：rich 版 `estimate_rich_tokens` / `compact_rich_now`，切分点 `rich_split_index` 避免把 `tool` 结果与所属 assistant 轮拆开（M6 起自动压缩改由请求前 guard `compact_before_request` 驱动）。
- `config.kl`：`agent_max_steps` 默认 20 → 100；`session.kl` / `repl.kl`：`/continue` 命令与帮助条目。
- 测试：`mock_openai.kl` 支持脚本化 `tool_calls`（对象或 JSON 文本，自动补 `finish_reason: "tool_calls"`）；`test_agent_loop.kl` 11 个用例覆盖上述 6 个场景 + `finish_reason: length/content_filter` + 400 overflow → compact → 重试 + SSE + 401；`test_agent.kl` 增加 tools/`tool_action` 单元用例；`test_config.kl` 同步默认步数；`test_session.kl` 覆盖 `/continue` 无任务分支。

偏差：

- 会话持久化仍是扁平 `{role, content}`：`conversation()` 把 `tool` 消息落成 `user` + `[tool result] ` 前缀，`restore_agent` 再重建 `CompletionMessage`，不保存 rich 字段。
- fenced 路径的 `_request_once` 走 `conversation()`（纯文本），rich 字段在该路径被丢弃，符合 fallback 语义。
- `run_action` 未改名，通过 `_run_call` / `_step_fenced` 包装与 native dispatch 区分。

### M3. 核心工具集 — ✅ 已完成

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

已落地：

- `tools.kl`：`Read` / `Write` / `Edit` / `Bash` / `Glob` / `Grep` / `TodoWrite` 七份 `ToolDef`（JSON Schema 由 `_prop` / `_schema` 组装）；`tool_action` 按名（大小写不敏感）把 native call 归一成既有的 `(info, body)`，不是七个工具之一或必填字段缺失/坏 JSON 时返回 nil，fenced 回落路径不受影响。
- 实现：`read_tool`（行号 + offset/limit、二进制/目录/2MB 上限）、`edit_tool`（唯一匹配，多处需 `replace_all`，返回 `edit_preview` 行）、`glob_tool`（拒绝绝对模式，`fs.walk` + 相对模式，上限 200）、`grep_tool`（`koala/re` + `_grep_file`，`include` / `ignore_case`，上限 200 条 / 5000 文件）、`todo_tool`（内存态，最后一次 TodoWrite 覆盖，非法 status 退化为 `pending`）。
- 正则先经 `_regex_ok` 校验：`re.compile` 对非法模式会 panic，模型给的 pattern 不能直达它。
- 统一路径：`resolve_tool_path` 按 `workdir` 解析并规范化，`path_blocked` 拒绝 no-access 目录；`_display_path` 相对化输出。
- `Bash`：`communicate(nil, timeout)` + `kill()` / `wait()`；`_cap_output` 保留 head/tail 并标注 `output capped at N bytes per stream`，完整输出写入 `fs.make_temp_file` 日志，路径回给模型。
- `agent.kl`：`run_action` 增 `timeout_ms` / `max_bytes` 参数（取 `cfg.agent_tool_timeout_ms` / `cfg.agent_max_tool_output_bytes`），分发七个工具；`_needs_approval` 覆盖 edit；`AGENT_PROMPT` 补齐工具说明；`tool_start` 之后追加 `edit_preview`。
- `config.kl`：`agent_tool_timeout_ms`（默认 120000）、`agent_max_tool_output_bytes`（默认 32000）。
- `session.kl`：状态行追加 `| todo done/total`（列表非空时）。
- 测试：新增 `test_tools.kl`（18 例：Read 行号/范围/截断/错误、Edit 唯一匹配与 preview、Glob 过滤与越界、Grep 匹配/include/ignore_case/非法正则、TodoWrite 覆盖与状态、`tool_action` 七个工具与坏参数、`run_action` 分发、shell timeout 不挂住、输出上限 + 临时日志）；`test_agent.kl` 断言七个工具与必填项；`test_agent_loop.kl` 工具数 2 → 7；`test_session.kl` 覆盖状态行 todo 段。

### M4. 权限模型与 plan mode（✅ 已完成）

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

已落地：

- 新增 `permission.kl`：单一判定函数 `permission_decision(mode, kind, body, allow, deny, bash_deny) -> (allow|ask|deny, reason)`，native 与 fenced 两条路径共用，fenced 回落不能绕过模式。`tool_kind` 把 `info` 归一成 `read/write/edit/bash/glob/grep/todo/done`（大小写不敏感）；`normalize_permission_mode` 接受 `accept-edits` / `acceptEdits` / `edits` 别名；`bash_deny_prefix` 只做前缀匹配。
- 判定顺序：`agent_deny_tools`（最高，`bypass` 也生效）→ plan 只读上限（`allow_tools` 不能突破）→ `agent_bash_deny` 前缀（`bypass` 也生效）→ 只读工具 / 未知动作放行 → `agent_allow_tools`（跳过询问）→ `bypass` 全放行 → `accept_edits` 放行 write/edit → 其余 `ask`。
- `config.kl`：`agent_permission_mode`（默认 `default`，非法名 panic 并列出合法值）、`agent_allow_tools`、`agent_deny_tools`、`agent_bash_deny`（三者均可选，空列表）。
- `agent.kl`：`_do_action` 按三态分支；`deny` 不执行、不询问，回给模型结构化拒绝（plan 用固定文案 `PLAN_DENIAL`，配置规则用 `permission denied by the harness for ...: <reason>`，都不重试）；`ask` 走注入的 approver，`a` 只记住**当前 kind**（允许 edit 不等于允许 shell）；plan 模式在 system prompt 追加 `PLAN_MODE_NOTE`，切换模式时刷新 `messages[0]`；fenced 回落时若处于 plan 则降级为 `default` 并提示。`_needs_approval` 保留为“这个 ask 是否值得打断用户”（畸形/越界动作交给 `run_action` 报错）。
- `session.kl`：`permission()` / `set_permission_mode()`（同步到已存在的 Agent；新建与恢复 Agent 时注入）；状态行在非 default 时追加 `| perm <mode>`。
- `repl.kl`：`/permissions [MODE]`（无参显示当前模式、四档说明、allow/deny/前缀配置，`*` 标记当前）、`/plan [off]`（记住进入前的模式，退出时恢复）、两条 `CommandInfo`（进入 `/help` 与补全）、`/permissions` 的 mode 补全；审批面板 `[a]` 文案按 kind 显示（all edits / all commands）。
- 明确语义：`a` 不跨会话持久化；权限模式也只存在进程内（不写 session 文件，重启回到 `agent_permission_mode`）；进入 plan 会清掉此前记住的 `a`；shell 黑名单只是前缀护栏，`x && rm -rf /`、`sh -c ...` 不会被拦截，**不是沙箱**。
- 测试：新增 `test_permissions.kl`（7 例：kind 归一、mode 归一/别名、四模式 × 只读/写改执行决策表、deny 列表优先、plan 压过 allow_tools、bash 前缀含 bypass 与绕过局限）；`test_agent_loop.kl` +2（plan 下 Write 被 harness 拒绝且 system prompt 带模式说明；`accept_edits` 下 Write 自动执行、Bash 仍询问）；`test_config.kl` +1（默认值 + 显式字段 + 别名）；`test_session.kl` +1（模式切换与状态行）；`test_repl.kl` +2（命令注册/补全、`/permissions` 报告）；`test_ui.kl` +1（`[a]` 按 kind 的标签）。

### M5. 流式输出与真正的 interrupt（✅ 已完成）

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

已落地：

- `libs/koala/readline`：interrupt 走 SIGINT 标志，不改 raw 模式语义。
  - `native/readline_shim.c` 新增 `tty_interrupt_watch(on)` / `tty_interrupt_taken()`：watch 期间 Ctrl-C 只置一个 `sig_atomic_t` 标志，进程不退出；关闭 watch 时把 SIGINT 交还给原来的 fatal handler（恢复终端再退出），所以编辑器空闲时的 Ctrl-C 行为不变。
  - `native.kl` 声明，`__readline__.kl` 导出 `watch_interrupt(on)` / `interrupt_pending()`（读即清空）。
  - 选择信号而非 stdin 轮询：raw 模式下 Ctrl-C 是字节 3 由编辑器处理，cooked 模式下（agent 在跑、编辑器不在读键）才是 SIGINT，两条路径天然不冲突，无需给编辑器加非阻塞按键轮询。
- `agent.kl`：
  - native 协议改用 `stream_tools`：`_stream_once` 按 chunk 读，文本按行 `emit`（与 chat 模式同形，`bot> ` 只在首行），`_pace(cfg.stream_delay_ms)` 同节奏。
  - tool call 由 `ChatStream.tool_calls()` 按 `index` 累积，流结束后才拿到完整参数并执行，屏幕上看不到半截 JSON；多个 call 仍按顺序执行。
  - `Turn` 记录 status/message/text/calls/finish/prompt_tokens/interrupted；`_stream_native` 对 429/5xx 退避重试两次（与旧的 `_request` 一致），400 overflow → `compact_rich_now` 重试，400 其他 → fenced 回落。
  - `_step_answer(step, text, calls, finish, shown)` 取代 `_step_native(choice, ...)`：`shown` 表示文本已随流显示，避免重复打印；`_run_calls` 在每个 call 前与最后一个 call 后检查中断。
  - 中断语义：流中中断 → 关闭连接，保留已收文本并追加 `[interrupted by the user]`，丢弃半组装的 tool calls（未执行就不能配对）；已声明未执行 → 逐个补 `tool` 结果 `INTERRUPTED_OBSERVATION`，`assistant(tool_calls) -> tool` 始终成对；中断后不自动重试，提示 `/continue` 或 `/cancel`。
  - 工具执行中按 Ctrl-C：子进程与父进程同进程组，`sh -c` 直接收到 SIGINT，`communicate` 的 poll 循环处理 EINTR，本轮结束后由检查点停止。
  - `_drive` 在 `_loop` 前后开关 `watch_interrupt`，失败请求仍回滚整轮；`cancel()`（`/cancel`）丢弃被打断的那一轮（含 user 消息）并对齐 `_sent_len` / `_compacted_len`；`set_interrupt_probe` 让测试无需终端即可在指定检查点触发中断。
  - fenced 协议保持非流式：仅在 native 被拒后使用，逻辑不变。
- `/cancel`：`repl.kl` 新增命令与帮助行，`session.cancel_agent()` 无中断时回答 `(nothing to cancel)`。
- 测试：
  - `test_agent_loop.kl` +4：流式分片参数的 tool call 正确组装并执行（写出文件、第二个请求带 `Write` 调用与 `tool` 结果）；第 1 个检查点中断 → 一个请求都不发、任务可 `/cancel` 丢弃；流中中断 → 只保留已到的两个 chunk 并带中断标记；已声明未执行的 call → 补齐 `tool` 结果且文件未创建。
  - `test_pty.sh` 新增 `check_pty_interrupt`：mock 以 `--delay-ms 100` 慢速流式回复，pty 里发送 `\003`，断言出现 `interrupted` 且进程仍回到提示符。
  - mock 增强：请求 body 含 `"stream":true` 就回 SSE（对齐真实 endpoint），`_frames` 先发文本 delta、再按 `index` 分片发 tool call 参数、最后一个 chunk 带 finish/usage，新增 `--delay-ms`。
- 已知边界：`/cancel` 仍不能在 run 进行中键入（REPL 阻塞在 `agent.run`），它用于中断后丢弃残留轮；双击 Ctrl-C 退化为“第一次即停止本轮”，因为单线程下第一次中断已直接回到 REPL。

### M6. 上下文与 compaction（✅ 已完成）

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

已落地：

- `compact.kl`：
  - `estimate_tokens` 计入 `name` 字段；`estimate_rich_message_tokens` 单条估算覆盖 content、`tool_call_id`、`name`、每个 tool call 的 name 与 arguments；`estimate_rich_tokens_from(messages, start)` 只累加“上一次请求之后新增”的消息。
  - `compact_before_request(client, model, cfg, messages, prompt_tokens, sent_len)`：请求前 guard，阈值仍是窗口的 80%。先判断 `estimate = 上次实际 prompt_tokens + 新增消息估算`；超阈值且 `can_compact_rich` 时按 turn group 摘要（`rich_split_index` 落在 `tool` 消息上就回退到发起该调用的 assistant，绝不留下孤立 tool），再对仍超阈值的单个 tool 结果做 `shrink_tool_results`（预算 4000→1333→444→148 字节逐级缩小，`truncate_middle` 保留头尾并写 `N bytes omitted`）。
  - 只有 content 被改写，`role` / `tool_call_id` / `tool_calls` 不动，消息序列始终满足 `assistant(tool_calls) -> tool` 配对。
  - `SUMMARIZE_PROMPT` 要求 summary 保留：当前任务与目标、创建/修改的文件及路径、关键命令与重要错误、未完成事项，并保留精确路径、标识符、报错原文。
  - 摘要失败（HTTP 错误、无 choices、空/空白 summary）一律返回 nil：`maybe_compact` / `compact_rich_now` / `compact_before_request` 显示 `compaction failed: no usable summary; keeping the full history`，历史原样保留，不会被空 summary 覆盖。
  - `can_compact` / `can_compact_rich` 区分“太短没法压缩”和“压缩失败”，避免把 no-op 当失败报错。
- `agent.kl`：
  - 新增 `_sent_len`（上次请求携带的消息数）与 `_compacted_len`（上次压缩后的消息数，防止无新增消息时反复压缩）。
  - `_loop` 每轮请求前调用 `_compact_before_request`，即“本轮所有 tool 结果都已追加、下一次请求尚未发出”的时刻；不再在 turn 结束处压缩，避免中途压缩把同一 assistant turn 的多个 tool 调用拆散。
  - 400 overflow 仍走 `compact_rich_now` 后重试一次，并同步 `_last_tokens` / `_sent_len` / `_compacted_len`；overflow 判定先于“tools 不支持”的 fenced 回落，不会把超窗误判成协议回退。
  - `/compact`（`Agent.compact`）成功或失败都保持游标一致。
- 明确边界：超窗防护针对 native 协议（`role: tool`）的工具结果；fenced 协议的观察结果是 `user` 消息，靠“400 overflow → compact 重试一次”兜底。
- 测试：
  - `test_agent.kl` +4：rich 估算覆盖 tool name/arguments/result 且 `estimate_rich_tokens_from` 只算增量；`rich_split_index` 在边界落在 tool 结果上时回退到 assistant(tool_calls)（`can_compact_rich` 语义）；`truncate_middle` 保留头尾并写省略标记；`shrink_tool_results` 把 40 KB 观察压到阈值内且保留 `tool_call_id`。
  - `test_agent_loop.kl` +2：4 次 Write + 1 次 20 KB Bash 输出、600 token 窗口 → 第 7 个请求带 summary 且只剩 6 条消息（system + summary + 两个完整工具轮），超大观察被截断；空 summary 场景下历史 9 条原样保留、不发第二次请求。

### M7. 会话、状态与持久化（✅ 已完成）

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
- 可选：单一 conversation，把 chat 视为 `tools=[]` 的 agent turn；这是彻底消除上下文割裂的方案，可放 M7 后段。（未做，留到 M8）

验收：

- agent 任务后 `/file` 的内容在下一轮 agent 请求中可见。
- 重启后 tool_calls/tool 结果能恢复。
- 状态行在 chat/agent 模式都显示正确 token。
- malformed session 文件不会导致启动崩溃。

已落地：

- rich message 持久化：
  - `koala/openai` 根模块新增 `message_to_object` / `message_from_object` / `tool_call_to_object` / `tool_call_from_object`：`role`/`content` 加上 `tool_calls`、`tool_call_id`、`name`；`content` 为空且只有工具调用时省略该字段（与 wire 一致）。`completions.kl` 的 `_message_json` 改为复用同一个函数，请求体与会话记录不再有两套形状。
  - 解析永不 panic：缺 `role` 或 `role` 非字符串返回 nil；其余字段缺失/类型错只损失该字段；没有函数名的 tool call 被丢弃。
  - `Agent.conversation_json()` 输出完整结构；`restore_agent(client, cfg, workdir, Array, paused)` 直接读会话记录里的 JSON 数组，旧记录（仅 `{role, content}`）同样能载入，非对象元素跳过。
  - `Session.to_record()` 写 `conversation_json()`；`restore_session` 的 `agent_messages` 只校验是数组，元素交给 `message_from_object`，因此旧存档可读、坏元素不会连累整个会话。
- 原子保存：
  - `SessionStore.save` 先写 `<name>.json.tmp` 再 rename；任一步失败返回 false 并清掉临时文件。
  - `SessionManager._save` 统一检查返回值，失败时按“连续失败”只警告一次（REPL 每轮都保存，不能让坏盘刷屏）；`rename` 在写旧记录失败时直接返回 false。
- `/file` / `@PATH` 进当前活动上下文：
  - `Session.load_file(path, is_agent)`：chat 模式进 `Session.messages`，agent 模式通过新的 `Agent.add_context` 追加 `role: user` 消息（不触发请求），下一次 agent 任务就能看到。
  - `Session._ensure_agent()` 统一创建 Agent 并注入 approver / 权限模式 / paused，`/file` 提前建 agent 也不会绕过权限。
- 状态行与 `/context` 按模式取 token：新增 `Session.active_tokens(is_agent)`，`status_line(cols, is_agent)` 与 `show_context` 都走它，`repl` 按 `_status_mode` 传入；chat 模式不再显示 agent 的 0。
- `/sessions`：
  - 新增 `Session.message_counts()`（chat/agent 条数）与 `SessionStore.mtime(name)`；行里显示 `12+34 msgs`（有 agent 时）与 `MM-DD HH:MM` 的最近写入时间。
  - `SessionRow` 增加 `agent_msgs` / `active`（带默认值，旧构造调用不变）；`session_lines` 先丢 active 列、再丢 model、最后丢 msgs，窄终端仍然每行不超宽。
- 命令语义：
  - `/pause` 冻结自动 compaction，恢复改用 **`/unpause`**（原 `/resume` 的行为）。
  - `/resume [N|NAME]` 现在是恢复历史会话：无参数先列表并给出用法，带参数切换并报告条数与模型。
  - 启动参数 `--continue`：恢复 `.current` 指向的会话（指针缺失/失效时退回最近写入），并在欢迎卡后说明恢复了哪个会话（模式、条数）。
  - 稳定 id 与 `.current`：记录新增 `version: 2` 与 `id`（= 文件名）；`SessionStore.read_current/write_current` 原子维护 `.current`；`/rename` 只改显示名；`--new` 强制新建、`--resume NAME` 按 name/id 指定会话。
  - `SessionManager.broken()` 记录加载失败的 id 与原因，`/sessions`（以及无参数的 `/resume`）在表末打印 `! id (原因)`；`.current` 指向已删除会话时启动打印警告并修复指针。
  - 跨重启 pending：`Session.to_record()` 写 `run_state`（`idle|pending`）与 `pending{mark, reason}`，`restore_session` 通过 `Agent.restore_pending()` 重新武装；`/continue`、`/cancel` 重启后仍有效；`repl` 在 `run` / `resume` / `cancel` / 普通一轮后立即 `save_current()`，避免中断后直接退出丢状态。
  - Tab 补全、`/help` 面板与模块文档同步。
- 测试：
  - `test_sessions.kl` +6：rich 消息 save/restore 往返（含 tool_calls / tool_call_id）、`message_from_object` 局部容错、原子保存（无 `.tmp` 残留、目录占位失败返回 false）、`mtime`、chat/agent 条数、状态行按模式取 token；会话身份 +6：`test_manager_rename_branches`（只改显示名、文件不动、可回改到自己的 id）、`test_rename_keeps_the_record_and_the_pointer`（记录/指针一起更新）、`test_current_pointer_wins_over_recency`、`test_current_pointer_skips_a_stale_id`、`test_store_current_pointer_branches`（坏/空指针、原子写、不进会话列表）、`test_broken_session_files_are_listed_not_fatal`（坏文件进 `broken()`，健康会话照常加载）；`test_ui.kl` +1：`broken_session_line` 的 `!` 前缀；pending +2：`test_pending_turn_survives_a_record`（idle/pending 记录往返、缺 pending 对象退回 idle）、`test_restore_interrupted_turn_can_still_cancel`（重启后 `/cancel` 仍能截断）。
  - `test_agent_loop.kl` +1：恢复的 `assistant(tool_calls) -> tool` 历史在下一次请求里原样回传（`tool_calls` 的 id/name 与 `tool_call_id` 都在）。
  - `test_session.kl` +1：agent 模式 `/file` 只进 agent 会话、chat 模式仍进 chat；`test_ui.kl` +1：`12+34 msgs` 与最近活动列、窄行降级；`test_repl.kl` +1：`/unpause` 注册与 `/resume` 说明。
  - `test_pty.sh` 全绿，`wombat test chat` 10/10 文件通过。
- 未做（可选，留到 M8）：把 chat 视为 `tools=[]` 的 agent turn，合并成单一 conversation。当前两套历史仍分开持久化（`messages` / `agent_messages`），只是 `/file`、状态行与 `/sessions` 都按模式正确选择。

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
| Native tool calling | ✅ 已实现（M1） | 无法真正对齐 Claude loop | P0 | M1 |
| safe JSON parse | ✅ 已实现（M1） | 模型参数错误会崩进程 | P0 | M1 |
| 多 tool call | ✅ 已支持（M2） | 一次只能一个动作 | P0 | M2 |
| 标准停止条件 | ✅ 已支持（M2，含 `/continue`） | loop 语义不对 | P0 | M2 |
| Read/Edit/Glob/Grep | ✅ 已实现（M3） | 不能读代码、不能精确改 | P0 | M3 |
| Bash timeout/输出上限 | ✅ 已实现（M3） | 卡死/撑爆上下文 | P0 | M3 |
| 权限模式 | ✅ 已实现（M4，四档） | 不能 plan/accept-edits | P1 | M4 |
| plan mode | ✅ 已实现（M4，`/plan`） | 不能先规划后执行 | P1 | M4 |
| 流式 tool call | ✅ 已实现（M5） | 体验差，无法实时 | P1 | M5 |
| interrupt/cancel | ✅ Ctrl-C 标志 + `/cancel`（M5） | 长任务无法停止 | P1 | M5 |
| compaction 时机 | ✅ 请求前 guard（M6） | 工具输出后易超窗 | P1 | M6 |
| tool 配对保留 | ✅ turn group 切分（M6） | compaction 可能切断工具轮 | P1 | M6 |
| 超大工具输出 | ✅ 阈值内截断头尾（M6） | 单条观察撑爆窗口 | P1 | M6 |
| 空/失败 summary | ✅ 报错并保留历史（M6） | 静默丢上下文 | P1 | M6 |
| rich message 持久化 | ✅ `message_to_object/from_object`（M7） | 重启丢 tool 历史 | P1 | M7 |
| `/file` 活动上下文 | ✅ 按模式进 chat/agent（M7） | agent 看不到 | P1 | M7 |
| 会话 resume | ✅ `/resume` + `/unpause` + `--continue`（M7） | 命令语义错误 | P2 | M7 |
| 原子保存 | ✅ tmp + rename + 失败警告（M7） | 可能写坏 session | P2 | M7 |
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
| `demo/chat/permission.kl` | 权限判定表（四档模式 + deny/allow/前缀规则） |
| `demo/chat/compact.kl` | rich message token 估算、pair-safe compaction |
| `demo/chat/session.kl` | 活动上下文、状态行、rich message 持久化 |
| `demo/chat/sessions.kl` | 原子保存、malformed 跳过、resume 选择 |
| `demo/chat/config.kl` | 新增 agent 配置项，默认 step 100 |
| `demo/chat/repl.kl` | `/continue`、`/permissions`、`/plan`、`/cancel`、`/unpause`、`/resume` |
| `demo/chat/ui.kl` | 多工具/流式展示适配 |
| `libs/koala/readline/*` | SIGINT watch（`tty_interrupt_watch/taken`），支撑 Ctrl-C |
| `libs/koala/http/*` / `libs/koala/sse/*` | 无需改动：`ChatStream.close()` 已是提前退出路径，中断在 chunk 边界检查 |
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
- sessions：
  - rich 消息 save/restore 往返，旧 `{role, content}` 记录仍可加载。✅ `test_rich_agent_messages_survive_a_record`、`test_rich_messages_parse_partially`
  - 原子保存：无 `.tmp` 残留，失败返回 false。✅ `test_store_save_is_atomic`
  - 状态行按模式取 token、`/file` 进活动上下文、`/sessions` 的 chat+agent 条数。✅ `test_status_line_reads_the_active_conversation_tokens`、`test_load_file_agent_mode_goes_to_the_agent`、`test_message_counts_chat_and_agent`

### E2E（mock LLM）

1. `Write` → `Bash` → final。✅ `test_native_write_bash_answer`
2. 一轮两个 tool calls。✅ `test_native_two_calls_one_turn`
3. 工具错误 → 模型修正。✅ `test_native_tool_error_then_fix`
4. 权限拒绝 → 模型收到拒绝并换方案。✅ `test_native_permission_denied`（用户拒绝）、`test_plan_mode_denies_changes_without_asking`（plan 模式）、`test_accept_edits_runs_write_but_still_asks_bash`（accept_edits）
5. step cap → `/continue`。✅ `test_native_step_limit_then_continue`
6. context overflow → compact + retry。✅ `test_overflow_compacts_then_retries`
7. 不支持 tools 的 endpoint → fenced fallback。✅ `test_fenced_fallback_on_400`
8. 流式文本 + 流式 tool call。✅ 流式文本 `test_stream_from_mock`；流式 tool call 由 `test_streamed_tool_call_arguments_are_assembled` 端到端覆盖（mock 分片参数 → 组装 → 执行）
9. Ctrl-C 中断，消息序列仍合法。✅ `test_interrupt_before_request_sends_nothing`、`test_interrupt_mid_stream_keeps_partial_text`、`test_interrupt_pairs_declared_calls`、pty `check_pty_interrupt`

### pty

- 多步 agent loop 的 tool block 展示。
- Ctrl-C 中断。✅ `check_pty_interrupt`
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

M1/M2 的实现与测试随里程碑落地，未单独立 issue（记录在本页 §4 的「已落地 / 偏差」）。后续 M3–M8 需要新建条目时，按 `issues/issues.summary.md` 页首的「下一个可用编号」顺序分配；本文早期草稿里的 `126`–`134` 已被实际缺陷与任务条目占用，不要再沿用。

可参考的拆分顺序（编号开工时再取）：

- 核心工具集（Read/Write/Edit/Bash/Glob/Grep、输出上限与超时）— M3
- 权限模式与 plan mode — M4
- 流式 tool call、interrupt/cancel — M5
- rich message compaction 精化、token 统计对齐 — M6
- 会话持久化（rich 字段）、`/file` 活动上下文、命令语义 — M7
- 高级能力 backlog（memory/todo/subagent/hooks/MCP）— M8

## 9. 风险与假设

- **safe JSON 是硬前置**：不先解决，native tool arguments 随时可能崩进程。
- **endpoint 差异**：不同 OpenAI-compatible 服务对 `tools`、流式 tool call 支持不一致；必须保留 fallback 与能力探测。
- **interrupt 涉及 runtime**：需要改 `readline`/SSE/HTTP；已确认允许，但改动面要单独测试。
- **shell 安全边界有限**：无法仅靠 harness 完全沙箱化；第一版以权限询问 + deny 规则为主。
- **rich message 迁移**：旧 session 只有 role/content，加载时必须兼容。
- **默认 chat**：agent 是显式模式，避免普通问答被工具 loop 干扰。
- **工具命名**：默认 Claude 风格，dispatch 大小写不敏感；若特定模型只接受小写，可在 M1 增加别名映射。

## 10. 建议执行顺序

1. **M0** mock 测试床 ✅
2. **M1** safe JSON + OpenAI tools ✅
3. **M2** Agent loop 重写 ✅
4. **M3** 核心工具集 ✅
5. **M4** 权限/plan ✅
6. **M6** compaction ✅
7. **M5** 流式/中断 ✅
8. **M7** 会话/状态/持久化 ✅
9. **M8** 高级能力

第一版可交付范围是 **M0–M4**：能原生多步调用工具、能读写改代码、能跑命令、有 mock E2E，并有权限模式与 plan mode。M6 已补齐上下文压缩，M5 已补齐流式输出与 Ctrl-C 中断，M7 已补齐 rich message 持久化、`/file` 活动上下文、命令语义与原子保存（剩余未落地项见 `session_plan.md` §13.1），剩下的是 M8 高级能力。

## 11. 实施前检查

- 不改动运行时 `demo/chat_sessions/main.json`。
- 每个里程碑结束运行 `wombat test chat` 与相关单测。
- 涉及 `libs/koala` 的改动额外运行 `./build-std-koala.sh`。
- 每个里程碑完成后更新对应 issue 状态，全部完成后再归档到 `issues/issues.done.md`。
- 旧 openai 测试全绿。