# demo/chat 会话系统优化计划（对齐 Claude Code）

> 状态：待实施
> 范围：`demo/chat` 的 `Session` / `SessionManager`：持久化、生命周期、活动上下文、恢复与测试
> 关系：本计划细化 `agent_loop_plan.md` 的 **M7**；依赖 M1（safe JSON）与 M2（native tool loop），不替代 M0–M6
> 结论：**需要优化**。当前实现能支撑单进程、短对话、手工恢复，但还不足以支撑 native tool calling、长任务、崩溃恢复和多窗口使用

## 1. 结论与优先级

当前会话系统是一个“最小可用快照存储”：一个会话一个 JSON 文件，启动时全部加载，每轮开头全量覆盖保存。它对普通聊天够用，但在以下三类场景会直接失效：

1. **native tool calling 之后**：消息不再只有 `role/content`，还会包含 `assistant.tool_calls`、`role: tool`、`tool_call_id`、工具名等；当前 `list[dict[str, str]]` 和 `_msgs()` 只序列化两个字段，重启后工具轮会丢失或无法配对。
2. **长任务或异常退出之后**：只在 REPL 每轮开头保存，agent 一次任务中的 assistant/tool 消息可能在进程崩溃、Ctrl-C、断电时全部丢失；直接覆盖写还可能留下半个 JSON，下一次启动直接 panic。
3. **多会话与恢复**：全量加载、用 mtime 猜当前会话、没有稳定 id、没有 pending/checkpoint、`/resume` 仍表示“解除 compaction 暂停”，这些都不符合 Claude Code 的会话生命周期。

### 1.1 P0：必须优先解决

| # | 问题 | 当前证据 | 不解决的后果 |
|---|---|---|---|
| S-P0-1 | 消息 schema 太窄 | `session.kl:25`、`session.kl:392-426`、`_msgs()` 只写 `role/content` | native tool 历史无法持久化，M2 后重启即坏 |
| S-P0-2 | 全量覆盖写，非原子、忽略失败 | `sessions.kl:69-71` 直接 `fs.write_str`；`save_current()` 不返回结果 | 崩溃/磁盘满会截断会话；没有错误提示 |
| S-P0-3 | malformed JSON 直接 panic | `sessions.kl:61-66` 使用 `json.parse_file`；注释也承认会 panic | 一个坏文件导致整个 chat 启动失败 |
| S-P0-4 | chat/agent 双上下文语义不一致 | `session.kl:179-189` 独立 `_agent`；`load_file()` 只写 chat；`status_line()` 永远读 agent token | `/file` 在 agent 模式不可见；chat 状态 token 错误 |
| S-P0-5 | 命令语义冲突 | `repl.kl:651-660` 的 `/resume` 实际是解除 pause；`/reset` 会丢弃 agent | 与 Claude Code 的 resume/reset 语义不一致，用户不可预期 |

### 1.2 P1/P2：应该做，但不阻塞 M1/M2

- P1：启动时全量加载所有会话（`sessions.kl:113-145`），会话越多启动越慢、内存越大。
- P1：当前会话由文件 mtime 推断（`sessions.kl:128-144`），没有显式 current 指针。
- P1：会话名就是文件名，`/rename` 会移动文件；没有稳定 `id`，checkpoint、引用、恢复都缺少可靠主键。
- P1：只有整轮保存，没有 per-tool checkpoint；step cap / interrupt 没有 pending 状态，`/continue` 无法真正续跑。
- P1：`restore_session()` 要求所有字段齐全，旧记录少一个字段就整条丢弃（`session.kl:338-361`、`session.kl:382-427`）。
- P1：`/sessions` 只统计 chat 消息，不显示最近活动、agent 消息数、当前模式。
- P2：没有多进程锁；两个 chat 同时打开同一会话会最后写入者覆盖。
- P2：没有删除、归档、导出、session 级 history；`history_file` 仍是全局 readline 历史。
- P2：没有 schema 版本、标题/摘要、创建/更新时间；无法做按时间排序或列表检索。

**优化原则**：先把“数据不丢、启动不崩、rich message 能存能读”做扎实，再谈 event sourcing、数据库、跨项目全局索引。第一版不引入数据库，不重写运行时 `demo/chat_sessions/main.json`。

## 2. 现状基线

### 2.1 当前调用链

```text
SessionManager.__init__
  -> SessionStore.list_names()
  -> 逐个 json.parse_file()
  -> restore_session()
     -> Session(messages, _agent)
     -> restore_agent(agent_messages)

REPL 每轮输入前
  -> mgr.save_current()
     -> Session.to_record()
     -> fs.write_str(name.json)
```

关键事实：

- `Session.messages` 是 `list[dict[str, str]]`，只能表达 `role` + `content`（`session.kl:25`）。
- agent 使用另一个独立消息列表 `Agent.messages`（`agent.kl:30-35`），`Session` 只持有 `_agent` 引用。
- 持久化字段固定为 `name/model/workdir/paused/last_tokens/messages/agent_messages`（`session.kl:71-82`）。
- 恢复时字段缺失或类型不符就整体返回 `nil`（`session.kl:338-361`）。
- 保存点在 REPL 顶部（`repl.kl:554`），不是每次消息变化后；`Agent.run()` 内部不触发保存。
- `SessionStore.load()` 用会 panic 的解析器；`save()` 直接覆盖目标文件。
- 当前 `/resume` 是暂停的反操作，不是历史会话恢复；`/clear` 只清屏，`/reset` 清空 chat 并丢弃 agent。

### 2.2 现有测试覆盖到的边界

- `demo/chat/test/test_sessions.kl` 覆盖 store、manager、rename、pause、跳过字段错误的记录、完整记录 roundtrip。
- `test_sessions.kl:282-296` 覆盖“字段不完整/根不是对象”的记录被跳过，但没有覆盖“JSON 本身 malformed 不 panic”。
- `test_sessions.kl:390-427` 把“缺字段必须返回 nil”固定成契约；引入 v2 兼容层后要改成“缺可选字段可迁移，缺关键字段拒绝”。
- `test_repl.kl:90-95` 固定了 `/resume` 出现在 pause/unpause 的命令集合里；改名 `/unpause` 后要同步更新。
- `test_pty.sh` 覆盖 TUI 和 non-tty，但没有覆盖会话创建、重启恢复、坏文件跳过。

## 3. 目标语义

把“会话”定义为一个项目目录下的持久对话单元，包含：

1. **身份**：稳定 `id`（文件名/checkpoint 主键）+ 可改的显示 `name`/`title`。
2. **两个活动上下文**：`chat` 与 `agent` 暂时保留，但每个操作都必须明确作用于“当前活动上下文”；后续再评估是否合并为一个 conversation。
3. **可恢复状态**：模型、工作目录、pause、每个上下文的 token 统计、最后一次活动时间、pending/run 状态。
4. **可靠存储**：原子写、可解析校验、schema 版本、迁移、坏文件隔离、可读的保存错误。
5. **明确生命周期**：新建、切换、重命名、暂停、恢复历史、继续未完成任务、重置；每一步都能安全重启。

### 3.1 暂不合并 chat/agent 的理由

`agent_loop_plan.md` 的 M7 把“单一 conversation（chat = `tools=[]`）”列为可选。本计划建议**分两步**：

- 第一步保留两个上下文，先把 rich message 持久化和 `/file` 路由做对。
- 第二步在 M2/M4 稳定、确认 OpenAI-compatible endpoint 能接受“历史里有 tool 消息但本轮 `tools=[]`”之后再统一。

原因是：一旦同一 conversation 里出现 `role: tool`，后续 chat 请求即使不传 `tools`，部分 OpenAI-compatible 服务也可能拒绝历史中的 tool 消息。先保留双上下文可以避免为了“架构好看”引入兼容性风险。**但所有会话 API 都必须带 active context 参数，禁止再隐式写死 chat 或 agent。**

## 4. 目标数据模型

### 4.1 v2 记录（建议）

```json
{
  "version": 2,
  "id": "s-20261122-153045-1",
  "name": "main",
  "title": "修复 demo/chat 会话",
  "created_at": 1790000000,
  "updated_at": 1790000000,
  "last_active_at": 1790000000,
  "model": "auto",
  "workdir": "/home/me/project",
  "paused": false,
  "active_mode": "chat",
  "last_tokens": 42,
  "messages": [],
  "agent_messages": [],
  "chat_last_tokens": 42,
  "agent_last_tokens": 0,
  "pending": null,
  "run_state": "idle",
  "stats": {
    "chat_messages": 0,
    "agent_messages": 0,
    "tool_calls": 0
  }
}
```

兼容策略：

- 没有 `version` 的记录按 **v1** 读取。
- v1 的 `messages` 继续作为 chat 消息；`agent_messages` 继续作为 agent 消息。
- v1 缺失的 `id` 用文件名（去掉 `.json`）补齐；缺失的时间戳用文件 mtime 或 0 补齐。
- v1 缺失 `agent_messages` 视为空列表，不再整条拒绝。
- 只有 `name/model/workdir` 等关键字段缺失或类型错误才拒绝；拒绝时记录警告并跳过，不阻塞启动。
- 新记录写入 v2；旧记录在第一次成功保存时升级，不在只读加载时隐式改写。

### 4.2 消息模型

会话不能继续使用 `list[dict[str, str]]`。建议复用 `agent_loop_plan.md` M1 引入的 rich message 类型，并在持久化层做“无损 JSON”：

- `role`：`system` / `user` / `assistant` / `tool`。
- `content`：允许 `null`（tool_calls-only assistant）。
- `tool_calls`：assistant 消息的工具调用数组，每项含 `id`、`type`、`function.name`、`function.arguments`。
- `tool_call_id`：`role: tool` 消息必填，必须与某个 assistant tool call 配对。
- `name`：可选的工具名/参与者名。
- 未知字段：读取时保留到 `extra`，写回时原样输出，避免未来字段被旧版本静默丢弃。

持久化要求：

- `assistant(tool_calls)` 与对应的 `tool` 结果必须作为一组保存；不允许只保存一半。
- 恢复时校验 tool 配对；发现孤立 `tool` 消息时保留原文并警告，不 panic、不擅自删除。
- `_msgs()` / `_field_msgs()` 不再手工拼 `role/content`，改为 `message.to_json()` / `message_from_json()`。
- `compact.estimate_tokens()` 同步覆盖 `content`、`tool_calls.function.name`、`function.arguments`、tool result。

### 4.3 元数据与列表

新增 `SessionMeta`（轻量，不加载完整消息）：

- `id`、`name`、`title`、`model`、`workdir`、`active_mode`、`paused`。
- `updated_at`、`created_at`。
- `chat_messages`、`agent_messages`、`tool_calls` 数量。
- `last_tokens`（当前活动上下文）。
- `corrupt`/`missing` 状态，用于列表提示。

`/sessions` 从 metadata 读取，不触发全量反序列化；只有 `/switch`、`/resume` 真正选中某个会话时才加载完整记录。

### 4.4 当前会话指针

- 在 session 目录写 `.current`，内容是稳定 `id`（或一行 JSON：`{"id":"..."}`）。
- 切换、新建、重命名后原子更新 `.current`。
- 启动顺序：`--resume ID` > `--continue` > `.current` > 最近 `updated_at` > 新建 `main`。
- 默认启动行为建议保持现有兼容：**没有参数时恢复最近活动会话**；另提供 `--new` 强制新建。若后续要严格对齐 Claude Code，可把默认改成新建，但需要单独确认。

## 5. 存储与可靠性

### 5.1 原子保存

`SessionStore.save()` 从 `fs.write_str` 改为：

1. 写入同目录临时文件 `<id>.json.tmp.<pid>`。
2. 用 `fs.File` 写完整内容，`sync()`，`close()`。
3. `fs.rename(tmp, final)` 覆盖目标。
4. 失败时删除临时文件并返回 `false`；保留旧文件。
5. 可选：重命名前把旧文件复制为 `<id>.json.bak`，恢复时只在主文件坏掉时使用。

`SessionManager.save_current()` 必须返回 `bool`，所有调用点检查并向用户显示 `(save failed: ...)`；不能静默丢数据。

### 5.2 安全加载与坏文件隔离

- 依赖 M1 的 `json.try_parse_str` / `try_parse_file`，不复制解析器。
- `SessionStore.load()` 返回 `(record, error)` 或 `nil` + 原因；绝不 panic。
- 解析失败：把文件重命名为 `<id>.corrupt.<timestamp>.json`，尝试 `.bak`；仍失败则跳过并警告。
- 字段校验分层：关键字段失败 → 跳过；可选字段缺失 → 迁移默认值。
- 目录不存在、权限错误、磁盘满都走可读错误路径。

### 5.3 懒加载与 sidecar

第一阶段不要求 event sourcing，但要求启动成本不再随会话正文增长：

- 每个 `<id>.json` 旁边写 `<id>.meta.json`（小文件，原子写）。
- 启动只扫描 `*.meta.json`；没有 sidecar 的旧文件才读取完整 JSON 生成 metadata。
- metadata 与正文不一致时，以正文的 `updated_at` 为准，后台重建 sidecar。
- `SessionManager` 保存 `list[SessionMeta]`，按 `name` 排序；完整 `Session` 按需加载并缓存。

### 5.4 并发与写入频率

- 第一版：同一进程内合并保存（`dirty` 标记 + 每个安全边界一次），避免 REPL 每轮无变化也写盘。
- 第二版：session 目录加 `.lock`，用 `fs.open(lock, "x")` 创建独占锁；写入 PID，启动时用 `os.kill(pid, 0)` 判断陈旧锁。
- 多窗口同时写同一 session 不在第一版承诺；发现锁冲突时明确提示“会话正在被另一个进程使用”，不要静默覆盖。

## 6. 活动上下文与命令语义

### 6.1 上下文路由

`Session` 增加显式的 active context 概念，REPL 不再依赖“当前 mode 变量恰好传对”：

- `active_messages(is_agent bool)` 返回对应列表。
- `load_file(path, is_agent bool)`：chat 模式写 chat messages；agent 模式写 agent messages。
- agent 模式首次 `/file` 且还没有 Agent 时，安静创建一个 Agent（不打印 work dir），把文件内容追加到它的 conversation。
- `ask()` 只操作 chat；`agent()` 只操作 agent；两者不互相清空。
- `reset()` 清空两个上下文和 pending，保留 name/model/workdir。
- `status_line(cols, is_agent bool)` 或 `Session.last_tokens(is_agent)`：chat 显示 chat token，agent 显示 agent token；`show_context` 可同时显示两者。

### 6.2 命令表

| 命令 | 当前 | 目标 |
|---|---|---|
| `/clear` | 只清屏 | 保持：只清屏，不动历史 |
| `/reset` | 清 chat + 丢 agent | 清空当前 session 的 chat 和 agent 历史、pending；保留模型/目录/名称 |
| `/pause [N\|NAME]` | 暂停自动 compaction | 保持 |
| `/unpause [N\|NAME]` | 不存在（当前叫 `/resume`） | 解除暂停；补全与帮助同步 |
| `/resume [ID\|NAME]` | 解除暂停 | 无参数列出历史会话；有参数恢复指定会话；不再表示 unpause |
| `/continue` | 不存在 | 从 `pending`（step cap / interrupt）继续 agent loop；没有 pending 时给 usage |
| `/new [NAME]` | 新建 | 新建稳定 id + 显示名；默认标题取首条用户消息 |
| `/sessions` | 名称/模型/chat 数 | 当前、名称、模型、模式、chat/agent 消息数、最近活动、坏文件提示 |
| `/switch N\|NAME` | 切换 | 保留；切换时懒加载完整 session，更新 `.current` |
| `/rename NAME` | 移动文件 | 只改显示名，不改 `id`/文件名；查重 |
| `/delete [ID\|NAME]` | 不存在 | P2：删除前确认；至少不允许删当前正在运行的 session |

### 6.3 启动参数

```text
--config PATH        现有
--session-dir DIR    新增，默认 chat_sessions（兼容旧行为）
--new                强制新建会话
--continue           恢复最近活动会话（显式写法）
--resume ID          恢复指定会话
```

如果当前目录不是项目根，`--session-dir` 和 `workdir` 分开处理：session 目录存会话，`workdir` 存 agent 工作目录。

## 7. 保存时机与 long-task 恢复

### 7.1 保存边界

把“每轮开头保存”改为“状态变化后的安全边界保存”：

1. 用户消息 append 后。
2. assistant 消息 append 后（尤其是带 `tool_calls` 的那条）。
3. 每个 tool result append 后。
4. compaction 完成后（消息列表被替换）。
5. `/new`、`/switch`、`/rename`、`/pause`、`/unpause`、`/reset` 后。
6. 正常结束、step cap、可恢复错误、Ctrl-C 后。

`Agent` 增加 `on_checkpoint func() bool` 回调（默认 no-op），由 `Session` 注入；只在消息列表真正变化时调用。这样 tool 轮不会等到任务结束才落盘。

### 7.2 pending 与 run_state

```json
"run_state": "idle | running | interrupted",
"pending": {
  "kind": "step_limit | interrupt | tool_boundary",
  "step": 100,
  "created_at": 1790000000
}
```

- 开始一个 agent 任务：`run_state=running`。
- 正常完成：`run_state=idle`、清空 pending。
- step cap：`run_state=idle`、`pending.kind=step_limit`；提示 `(step limit reached; send /continue)`。
- 用户 Ctrl-C：在安全边界保存 `run_state=interrupted`、`pending.kind=interrupt`；消息序列保持合法。
- 启动时发现 `run_state=running`：视为上次异常退出，标记 `interrupted` 并提示 `/continue`。
- `/continue`：
  - `step_limit`：不再追加用户任务，继续同一 conversation 的 loop。
  - `interrupt`：从最后一条完整消息继续；如果中断发生在 tool 执行中，先补一条“工具被中断”的 tool result，保证 tool 配对完整。
- 没有 pending 时 `/continue` 不猜测，直接提示没有可继续的任务。

### 7.3 checkpoint / rewind（P2）

在 S5 之前只做“恢复到最后安全边界”，不做任意回滚。后续可在每次 checkpoint 写一个轻量 snapshot 指针：

- 每个 tool 轮一个 checkpoint id。
- `/rewind` 回到某个 checkpoint；需要同时恢复文件状态或用 diff/undo，属于 M8 backlog。
- 第一版不承诺文件回滚，避免“消息回滚了但磁盘文件没回滚”的假安全。

## 8. 里程碑

### S0. 基线测试与契约确认（0.5 天，可先做）

- 用临时 `session_dir` 锁定当前行为：新建、切换、rename、pause、重启选最近、字段错误跳过。
- 补一个“malformed JSON 当前会 panic”的 reproducer（测试可以先标记为 expected failure，M1 后转正）。
- 明确启动默认、`/reset` 范围、session 目录、是否统一 conversation 四个决策。
- 验收：不改生产代码；测试只在 `/tmp` 下操作，绝不碰 `demo/chat_sessions/main.json`。

### S1. 持久化 v2：安全、原子、可迁移（1–2 天，依赖 M1）

任务：

- 新增/改造 `SessionRecord`（或等价 Object 读写层）：`version`、`id`、时间戳、`title`、可选字段。
- `SessionStore.load()` 改用 safe parse；坏文件隔离；`.bak` 恢复。
- `SessionStore.save()` 改为临时文件 + sync + rename；返回 `bool`；失败显示错误。
- `Session.restore()` 分层校验 + v1→v2 迁移；缺可选字段不再整条丢弃。
- `SessionManager.save_current()` 返回结果；REPL 保存点从“每轮顶部”改为“dirty + 安全边界”。
- `test_sessions.kl` 增加：v1 迁移、缺可选字段、malformed 不 panic、原子写失败保留旧文件、保存失败返回值。

验收：

- 故意写坏一个 session 文件，启动能继续并给出警告。
- 保存过程中模拟 rename 失败，原文件仍完整。
- 旧 v1 记录能加载；新记录为 v2；不修改运行时 `main.json`。

### S2. rich message 与 tool 历史持久化（1–2 天，依赖 M1/M2）

任务：

- `Session.messages` / `Agent.messages` 改为 rich message 列表（复用 M1 的 `ToolCall` / `Message` 类型）。
- `to_record()` / `restore()` / `_msgs()` / `_field_msgs()` 全部改为 rich JSON；保留 v1 兼容。
- 校验 `assistant(tool_calls)` 与 `role: tool` 配对；恢复时给孤立 tool 消息警告。
- `compact.estimate_tokens()` 覆盖 tool_calls 参数与 tool result。
- `Agent` 注入 checkpoint 回调；每个 assistant/tool 消息后保存。
- 增加 roundtrip 测试：多 tool call、空 content、tool result、name、未知字段。

验收：

- 一个 `Write → Bash → final` 的会话重启后，tool_calls/tool 结果完整，不会把工具历史当普通 user 文本。
- compact 前后 tool 轮不被切断。

### S3. 活动上下文与命令语义（1 天）

任务：

- `load_file(path, is_agent)`、`active_messages`、per-context token 统计。
- `/file` / `@PATH` 路由到当前模式；agent 无实例时安静创建。
- `status_line` 按模式显示 token；`/context` 显示当前上下文，可附带另一侧摘要。
- `/reset` 清两个上下文和 pending；`/clear` 保持只清屏。
- `/pause` / `/unpause`；`/resume` 改名为历史恢复；更新命令表、帮助、补全、测试。
- `/sessions` 统计 chat + agent 消息，显示最近活动与当前标记。

验收：

- agent 模式 `/file` 后，下一轮 agent 请求能看到文件内容。
- chat 模式状态行显示 chat token，不再显示 agent 的 token。
- `/help`、Tab 补全、`/sessions` 与命令实现一致。

### S4. 会话生命周期与历史恢复（1–2 天）

任务：

- 稳定 `id`（文件主键）与可改 `name`；`/rename` 不再移动文件。
- `.current` 指针；`--new` / `--continue` / `--resume ID`。
- `SessionMeta` sidecar；启动懒加载；`/sessions` 读 metadata。
- `/resume` 选择器（无参数列表 + 编号/名称参数）；`/switch` 保留直达。
- `/delete`（可选，P2）：确认、拒绝删当前运行中会话、清理 sidecar/temp/bak。
- 旧文件没有 sidecar 时按需生成；不自动批量重写。

验收：

- 创建多个会话后重启，当前会话与 `.current` 一致；`/resume` 能选旧会话。
- 只改显示名不会导致旧引用/checkpoint 失效。
- 启动不加载所有正文（通过测试计数或大文件启动时间验证）。

### S5. 长任务恢复、并发与性能（1–2 天）

任务：

- `run_state` / `pending` 写入与恢复；`/continue`。
- Ctrl-C 在安全边界落盘；中断后消息序列合法。
- 写入合并/去抖：同一轮多个 checkpoint 不重复写同一内容。
- 可选 `.lock`：单写者；陈旧锁检测；冲突提示。
- 可选 event log：如果全量快照在长会话中明显变慢，再评估 JSONL append + snapshot；不在第一版强行引入。

验收：

- 模拟进程在 tool 中途退出，重启提示 interrupted，`/continue` 能继续且不产生孤立 tool 消息。
- 两个进程同时打开同一会话，第二个得到明确提示而不是静默覆盖。
- 长会话连续 100 次 checkpoint 的写放大可接受（目标：每轮只写变化后的快照，不重复写不变内容）。

### S6. 测试与硬化（贯穿）

- 单元：迁移、safe parse、原子写、rich roundtrip、metadata、稳定 id、current 指针、命令表。
- E2E：mock LLM 下 `Write → Bash → final` 后重启恢复；`/resume`；`/continue`；坏文件启动；Ctrl-C 后重启。
- pty：`/sessions`、`/resume`、`/unpause` 的显示与输入；窄终端/plain 不崩。
- 回归：`cd demo && wombat test chat`；涉及 `libs/koala` 的 JSON/openai 改动后跑 `./build-std-koala.sh`；全量 `./test-debug.sh`。

## 9. 测试计划

### 9.1 单元测试

`demo/chat/test/test_sessions.kl`：

- `test_restore_v1_missing_optional_fields`：缺 `agent_messages`、`id`、时间戳仍可加载。
- `test_restore_v2_rich_messages`：assistant tool_calls + tool result + name 完整 roundtrip。
- `test_load_malformed_does_not_panic`：坏 JSON 被跳过，其他会话仍加载。
- `test_save_atomic_failure_keeps_old`：目标不可写/rename 失败时旧文件不变，临时文件清理。
- `test_meta_sidecar_and_current_pointer`：metadata 列表、`.current` 切换。
- `test_rename_keeps_id`：只改显示名，文件名/id 不变。
- `test_schema_rejects_invalid_role_or_tool_pair`：非法 role 拒绝；孤立 tool 保留并警告。

`demo/chat/test/test_session.kl`：

- `test_status_tokens_follow_mode`：chat/agent 分别显示。
- `test_load_file_routes_to_active_context`：chat 与 agent 各写各的。
- `test_reset_clears_both_contexts`：保留 model/workdir/name，清 pending。

`demo/chat/test/test_repl.kl`：

- 命令表包含 `/unpause`、`/resume`、`/continue`，不再把 `/resume` 归到 pause 组。
- 帮助/补全/unknown-command 与新语义一致。
- `/sessions` 文案包含 chat/agent 计数和最近活动。

### 9.2 E2E（mock LLM）

在 M0 的 mock server 基础上增加：

1. 运行一个 `Write → Bash → final` 的 agent 任务；退出；重启；`/context` 和下一轮 agent 请求仍含完整 tool 历史。
2. 创建 `a`、`b` 两个会话，重启后 `/resume` 选择 `a`，消息和 workdir 正确。
3. `/file` 在 agent 模式注入成功后，下一轮 mock 请求体可见文件内容。
4. 写坏一个 session JSON，启动不崩；`/sessions` 标记坏文件；其他会话可切换。
5. step cap 后进程退出；重启 `/continue` 继续，不重复追加原任务。
6. Ctrl-C 中断后重启，消息序列合法，`/continue` 可从安全边界继续。
7. `/clear` 不清历史；`/reset` 清历史；两者分别验证。

### 9.3 pty

- `/sessions` 在 80/40 列不截断关键字段。
- `/resume` 无参数列表、按编号选择、`/unpause` 补全。
- 坏文件警告在 TUI 和 plain 下都可读。
- 保存失败提示不会破坏输入框布局。

### 9.4 命令

```bash
source env.sh
cd demo && wombat build chat
cd demo && wombat test chat
bash demo/chat/test/test_pty.sh
# 涉及 libs/koala 的 JSON/openai 改动后
./build-std-koala.sh
# 全量回归
./test-debug.sh
```

## 10. 建议 issue 拆分

`agent_loop_plan.md` 已建议占用 `126`–`134`。本计划的条目建议从 `135` 开始，等 agent loop issue 确认后再正式建：

- `135` 会话 v2 schema、v1 迁移、safe load、原子保存、坏文件隔离
- `136` rich message / tool 历史持久化与 checkpoint 回调
- `137` 活动上下文：`/file` 路由、per-context token、`/reset` 语义
- `138` 会话生命周期：稳定 id、`.current`、metadata sidecar、`/resume`、`--continue`
- `139` 长任务恢复：`run_state`/`pending`、`/continue`、锁与写放大
- `140`（可选）会话删除/导出、event log、session 级 history

建议先建 `135`；`136` 必须等 M1/M2 的类型稳定，避免基于旧 `dict[str,str]` 写两遍。

## 11. 建议执行顺序与最小交付

1. **S0** 基线测试与契约确认（现在可做，不改生产代码）。
2. **M1** safe JSON + openai rich types（来自 `agent_loop_plan.md`）。
3. **S1** 持久化 v2 + 原子写 + 迁移。
4. **M2** Agent loop 重写。
5. **S2** rich message 持久化 + checkpoint。
6. **S3** 活动上下文与命令语义。
7. **S4** 稳定性生命周期与 `/resume`。
8. **S5** `/continue`、中断恢复、锁。
9. **S6** 测试硬化。

**最小可交付（建议）**：S0 + S1 + S2 + S3。完成后应达到：

- 坏 session 不崩；
- 保存失败不丢旧数据；
- 重启后 tool 历史完整；
- `/file` 在 agent 模式生效；
- chat/agent token 正确；
- `/unpause` 与 `/resume` 语义不再冲突。

`/resume` 完整选择器、`/continue`、多进程锁可以放下一批。

## 12. 风险、假设与待确认决策

### 12.1 风险

- **safe JSON 是硬前置**：S1 不能在 M1 之前落地，否则仍然会 panic。
- **rich message 类型变更面大**：`Session.messages`、`Agent.messages`、`compact`、`session` 持久化、测试都受影响；必须在一个里程碑内完成并跑全量回归。
- **checkpoint 写放大**：全量快照在每个 tool result 后写，长对话可能慢；先做去抖/脏标记，event log 作为后手。
- **双上下文仍是过渡态**：`/file` 路由只是修语义，不解决“chat 与 agent 各自一半历史”的长期问题；统一 conversation 要等 endpoint 兼容性验证。
- **并发锁**：Koala 标准库没有 `flock`，只能用 `open("x")` + PID 检测；陈锁误判需要保守处理。
- **时间与排序**：UTC 时间字符串/epoch 的精度与本地时区显示要统一；排序以 `updated_at` 为准，不以文件名或 mtime。

### 12.2 待确认决策

1. **启动默认**：保持“恢复最近会话”（兼容现状），还是改成“默认新建，`--continue` 恢复最近”（更接近 Claude Code）？本文建议前者，同时提供 `--new`。
2. **`/reset` 范围**：清 chat + agent 两个上下文（本文建议），还是只清当前模式？
3. **session 目录**：继续 `chat_sessions/`（cwd），还是改为项目级/用户级目录？本文建议先用 `--session-dir` 可配置，默认不变。
4. **是否合并 conversation**：本文建议暂缓，等 M2/M4 后按 endpoint 实测决定。
5. **是否做 event log**：第一版不做；只有全量快照被证明是瓶颈时才做 JSONL append + snapshot。

## 13. 验收清单

- [ ] malformed session 文件不会导致启动 panic，其他会话正常可用。
- [ ] 会话保存是原子写；保存失败有提示且旧文件完好。
- [ ] v1 会话可加载并迁移到 v2；旧运行时文件不被自动改写。
- [ ] rich message（tool_calls / tool_call_id / name）重启后完整。
- [ ] agent 模式 `/file` 的内容下一轮 agent 请求可见。
- [ ] chat/agent 状态行 token 分别正确。
- [ ] `/clear` 只清屏；`/reset` 清当前 session 历史；`/unpause` 解除 pause；`/resume` 恢复历史。
- [ ] `/sessions` 统计 chat + agent 消息并显示最近活动。
- [ ] 稳定 id：`/rename` 不改文件名；`.current` 恢复正确。
- [ ] step cap / Ctrl-C 后重启可 `/continue`，消息序列合法。
- [ ] `wombat test chat`、`test_pty.sh`、`./test-debug.sh` 全绿。
- [ ] 不修改 `demo/chat_sessions/main.json` 等运行时文件。

## 14. 与 `agent_loop_plan.md` 的对应关系

| 本计划 | agent_loop_plan | 说明 |
|---|---|---|
| S0 | M0 | 复用 mock LLM 测试床；S0 只加会话基线 |
| S1 | M1 + M7 保存 | safe JSON 先由 M1 提供 |
| S2 | M2 + M7 rich message | `tool_calls` 类型由 M1/M2 提供 |
| S3 | M7 活动上下文/命令 | `/file`、token、`/reset`、`/unpause` |
| S4 | M7 `/resume` | 稳定 id、current、metadata |
| S5 | M5 + M7 | interrupt、`/continue`、恢复 |
| S6 | M0 + 全量测试 | mock E2E、pty、回归 |