# demo/chat 界面优化计划（对齐 Claude Code）

状态：已完成（Phase 0–4；tmux 验收跳过）；pty/non-tty E2E 已补 `chat/test/test_pty.sh`
最后更新：2026-11-22
范围：只改界面与交互表现；模型/工具能力缺口另行记录在 `issues/`。

## 1. 目标

让 `demo/chat` 的外观看齐 Claude Code：单列对话流、工具调用块、就地 diff、
行内审批、常驻状态行、明确的忙碌/中断提示；在 80/120 列终端、CJK、管道
输出下都不丢内容、不闪烁、可复制。

验收场景：给定“修复 demo/x 里失败的测试”，用户能一眼看清
`> 输入` → `⏺ 工具` → `⎿ 结果/diff` → `✓ 完成`，失败时能看到关键错误。

## 2. 初始问题（优化前基线）

以下问题均已处理，保留作背景与回归依据。

- 长行被截断：`repl.kl:139` 对每行 `readline.fit_text(lines[i], w)`，
  而 `tui.View` 本身不换行（`view.kl:8`）。启动欢迎语（`repl.kl:243`）
  超过 80 列，大部分命令提示看不见。
- 每轮全屏清空重绘：`screen.frame_start()` 执行 `ESC[2J`（`screen.kl:69-72`），
  闪烁；alt-screen 退出后只剩最后一帧，历史不在 scrollback。
- 工具步骤没有视觉语义：`[step N] info` + 结果首行（`agent.kl:94-95`、
  `step_summary` 在 `agent.kl:161-163`）；失败时只看到 `exit 1`，stderr 被藏掉。
- 没有 diff、没有审批：写文件前无法预览；`agent.kl:183` 直接覆盖。
- 长调用/长命令期间屏幕静止：`agent.kl:70` 非流式请求，`agent.kl:191`
  的 `os.output` 无超时，体感是卡死。
- 状态行只在 agent 模式、且静态：`repl.kl:250-254`、`session.kl:228-235`；
  chat 模式没有 model/tokens。
- 输入框只有上下横线，没有左右边线，内容顶到右边缘（`repl.kl:147-151`）；
  提示词 `[chat] you>` 占宽（`repl.kl:57-62`）。
- 没有 `/help`；未知 `/xxx` 会作为普通问题发给模型（`repl.kl:420-425`）；
  命令表手工维护在 `repl.kl:451`。
- resize 未处理：`Screen` 构造时只读一次尺寸（`screen.kl:42-56`）。

## 3. 设计原则

1. 内容优先：transcript 占 90% 屏幕，常驻 chrome ≤ 3 行（输入框 + 提示/状态）。
2. 单列流：不要侧栏、不要多面板、不要每轮重绘整屏。
3. 一条消息一个视觉块：用户 `>`，工具 `⏺`，结果 `⎿`，错误 `✗`，完成 `✓`。
4. diff 就地显示：写/改文件先给红绿 diff，不让用户自己 `git diff`。
5. 状态可预期：思考/运行/压缩都有单行提示；能中断时明确写 `esc 中断`。
6. 输出可复制：尽量保留 scrollback；不滥用 alt-screen 全屏清空。
7. 颜色只表达语义：成功绿、失败红、警告黄、路径青、次要信息 dim；支持 `NO_COLOR`。
8. 非 tty 降级：纯文本、无 ANSI、无边框、无 spinner 残留。

## 4. 视觉规范

### 4.1 字符

| 用途 | 字符 | 说明 |
|---|---|---|
| 工具调用 | `⏺` | 每个工具一行摘要 |
| 工具结果 | `⎿` | 缩进在工具行下 |
| 成功 / 失败 | `✓` / `✗` | 结果行尾或前缀 |
| 进行中 | `✻` | 思考/运行/压缩 |
| 用户输入 | `>` | transcript 中保留 |
| 暂停会话 | `⏸` | `/sessions` |
| 当前会话 | `●` | `/sessions` |
| 省略 | `…` | `… 18 lines` |

非 tty、`TERM=dumb` 或 `NO_COLOR` 时降级为 `*`、`>`、`+`、`-`，不输出 Unicode 框线。

### 4.2 颜色（16 色安全）

- 强调/路径：cyan
- 成功：green
- 失败：red
- 警告/待确认：yellow
- 次要信息（token、耗时、目录、提示）：dim
- diff：`+` green、`-` red、hunk `@@` cyan
- 不使用背景色块（终端主题差异大）

### 4.3 布局

```
第 1 行      （欢迎语 / 空行）
...          transcript（全宽，左内边距 2）
第 rows-5 行 ╭─ 输入框 ─╮
第 rows-4 行 │ > ...    │
第 rows-3 行 ╰──────────╯
第 rows-2 行 提示：Enter 发送 · Ctrl-J 换行 · / 命令 · @ 文件
第 rows-1 行 auto · ~/proj (main) · 18.2k/131k · 14%
```

建议去掉 transcript 外框（当前上下横线 + 无侧边），只保留输入框；内容多 2–4 列，
视觉噪音更少，也更像 Claude Code。

### 4.4 典型状态

空闲：

```
✻ chat 0.1 · auto · ~/proj
  输入任务，/ 命令，@ 文件，? 帮助

╭──────────────────────────────────────────────╮
│ > 修复 demo/x 里失败的测试                    │
╰──────────────────────────────────────────────╯
  Enter 发送 · Ctrl-J 换行 · Tab 补全
  auto · main · 0/131k · 0%
```

思考 / 运行：

```
> 修复 demo/x 里失败的测试

⏺ 读取 demo/x/test_a.kl (1-80)
⏺ 编辑 demo/x/a.kl  +3 -1
  ⎿ 42  - assert_eq(add(1, 2), 4)
    42  + assert_eq(add(1, 2), 3)
✻ 运行 wombat test chat… 2.4s · esc 中断
```

成功 / 失败：

```
⏺ 运行 wombat test chat
  ⎿ ✓ exit 0 · 12 passed
⏺ 完成
  ⎿ 改了 1 个文件 (+3 -1)

⏺ 运行 wombat test chat
  ⎿ ✗ exit 1 · 12 passed, 1 failed
    demo/x/test_a.kl:42: expected 3, got 4
    … 18 lines omitted
```

审批：

```
⏺ 写入 demo/x/a.kl  +3 -1
  允许？ [y] 本次  [a] 本会话  [n] 拒绝  [d] 看完整 diff
```

错误：

```
✗ 请求失败 [401]
  API key 无效或已过期。检查 OPENAI_COMPAT_API_KEY。
```

帮助：

```
命令
  /agent  /chat  /new  /sessions  /switch  /model  /file  /help
快捷键
  Enter 发送   Ctrl-J 换行   Ctrl-C 清空输入   Ctrl-R 搜索历史
  PgUp/PgDn 翻页   ? 帮助   / 命令   @ 文件
```

## 5. 功能清单

优先级：P0 = 不做界面就不可用；P1 = Claude Code 的关键体验；P2 = 打磨。

| ID | 功能 | 现状 | 目标 | 优先级 |
|---|---|---|---|---|
| UI-01 | 长行软换行 | `repl.kl:139` 截断 | 按显示宽度换行，CJK/ANSI 正确，不丢字符 | P0 |
| UI-02 | 停止每轮全屏清空 | `screen.kl:69-72` 每轮 `2J` | 只在 resize/`/clear` 时清；日常只追加/局部重绘 | P0 |
| UI-03 | 工具调用块 | `[step] write x` 纯文本 | `⏺ 动作 + 目标`，执行前显示，完成更新状态 | P0 |
| UI-04 | 工具结果块 | 只显示首行，失败无 stderr | `⎿` + 状态 + 关键输出；失败保留尾部 N 行；`… N lines` | P0 |
| UI-05 | 写文件 diff | 无 | 写/改前显示红绿 diff，新增文件显示 `+N` | P0 |
| UI-06 | 状态行（全模式） | 仅 agent，静态 | cwd + 分支 + model + context 百分比 + 模式；运行时更新 | P0 |
| UI-07 | spinner/忙碌提示 | 无 | 思考/运行/压缩单行提示 + 耗时 + `esc 中断`（有取消能力后） | P0 |
| UI-08 | 错误块 | 纯文本 `Error [..]` | `✗` + 状态码 + 可操作提示；401/429/超时区分 | P0 |
| UI-09 | 欢迎语 | 200+ 字符一行，被截断 | ≤ 2 行：版本、模型、cwd、`/help` | P1 |
| UI-10 | 输入框 | 只有上下线，提示词占宽 | 完整圆角框，`> ` 提示，模式放状态行 | P1 |
| UI-11 | 命令面板 | 只有 Tab 补全 | 输入 `/` 或 `/` + Tab 显示分组命令与说明 | P1 |
| UI-12 | `@` 文件选择 | 只能手输 | 输入 `@` 后列出匹配文件，Tab 选择 | P1 |
| UI-13 | 审批交互 | 无 | 行内 `[y/a/n/d]`，默认拒绝；`d` 看完整 diff | P1 |
| UI-14 | `/help` 与快捷键 | 无 | 分组帮助；未知 `/` 命令本地拦截并提示 | P1 |
| UI-15 | 压缩提示 | `(context compacted)` | `↺ 压缩上下文 18.2k → 4.1k tokens`，dim 单行 | P1 |
| UI-16 | resize | 不支持 | 每帧前/每个 chunk 间检查终端尺寸并重排 | P1 |
| UI-17 | 会话列表 | 单行拼接，易被截断 | 对齐表格：`● 1 main  auto  12 msgs`，当前高亮 | P2 |
| UI-18 | 展开工具输出 | 无 | `Ctrl-O` 展开最近一次完整输出（需按键捕获） | P2 |
| UI-19 | 主题/NO_COLOR | 无 | 自动检测 `NO_COLOR`/`TERM=dumb`；亮/暗终端可读 | P2 |
| UI-20 | todo/plan 面板 | 无 | 任务清单 `☐/☑` 固定区（属于功能，不只是界面） | P2 |
| UI-21 | 鼠标 | 无 | 不做；保留终端原生选择/滚动 | 不做 |

## 6. 设计决策（记录）

以下为本次实现采用或明确暂缓的决策，不作为待办。

| # | 决策 | 建议 | 影响 |
|---|---|---|---|
| 1 | alt-screen 还是 inline scrollback | v1 保留 alt-screen 并修掉每轮清屏；v1.1 增加 `ui_mode=inline`，默认 inline | inline 更接近 Claude Code、保留 scrollback，但需重写绘图/翻页 |
| 2 | transcript 外框是否去掉 | 去掉，只保留输入框 | 内容宽 2–4 列，视觉更干净 |
| 3 | 审批交互 | v1 行内 `[y/a/n/d]`，方向键选择放 v2 | 方向键需要 readline 单键读取 API |
| 4 | 命令面板 | v1 “`/` + Tab 列表”，浮层放 v2 | 浮层需要 readline 重绘/浮层 API |
| 5 | Markdown 范围 | v1 纯文本 + diff + 代码缩进；完整 Markdown/高亮放 P2 | 避免渲染器拖垮进度 |

依赖说明：

- `esc 中断` 依赖库层取消原语（`std/os` 信号标志 + 短轮询，或 readline 原始键捕获）；
  未就绪前不显示该提示，避免承诺做不到的操作。
- 审批与 diff 依赖 agent 层配合（执行前读取旧内容、执行前回调），不是纯 UI 改动。
- 真正的 spinner 动画需要定时器/线程；v1 只做静态提示 + chunk/命令返回时更新耗时。

## 7. 分阶段任务（已完成）

原预估约 2 周（单人）；实际按阶段完成，每阶段结束均跑全量回归。

### Phase 0：可测试的渲染层（0.5–1 天）

- [x] 抽出纯渲染函数 `ui.render(state, cols) -> list[str]`，不直接写终端
- [x] 定义 UI 事件：`turn_start` / `tool_start` / `tool_end` / `diff` / `error` / `status` / `notice`（`turn_start`/`tool_end`/`diff_event`/`error_event`/`status_event`/`notice` 已实现并接入；`tool_start`/`tool_result`/`http_error`/`busy` 原有）
- [x] 用固定 `rows/cols` 做 6 个 golden 场景：空闲、流式、工具成功、工具失败、CJK/长 URL、40 列
- [x] 非 tty / `NO_COLOR` / `TERM=dumb` 降级路径（glyph、prompt、状态行、边框）
- [x] 现有测试全绿

验收：`ui.render` 可在测试里断言完整帧；`Screen` 仍是唯一 I/O 出口。

### Phase 1：内容正确性（2–3 天，最高性价比）

- [x] `wrap_text`：按显示宽度折行，ANSI 跨行保持样式，CJK/emoji 宽度用 `readline.width`
- [x] tab 展开、超长 URL/无空格路径强制断行、行尾空格不参与宽度
- [x] `draw_transcript` 改用换行后的显示行，不再 `fit_text` 截断（`repl.kl:139`）
- [x] 去掉每轮 `frame_start()` 全屏清空；初始化画一次 chrome，之后只追加/局部重绘
- [x] 仅 `/clear`、启动、resize 和显式重绘时清屏
- [x] `agent.kl` 改为发 `tool_start`/`tool_end` 事件，执行前就显示 `⏺ 动作 + 目标`
- [x] 成功结果一行；失败保留 stderr 尾部 N=20 行，超出显示 `… N lines omitted`
- [x] 结构化错误（status / message / retryable），401/429/超时给可操作提示
- [x] 用户输入在 transcript 中以 `> ` 前缀 + dim 显示
- [x] 测试：80/40 列、200 字符行、CJK、ANSI、长路径、失败 stderr、无 tty（plain 降级用例与真实非 tty E2E 均覆盖，见 `chat/test/test_pty.sh`）

验收：任何一行都不再被截断；80 列下欢迎语和工具结果完整可读；失败能看到关键错误。

### Phase 2：Chrome 与状态（2–3 天）

- [x] 欢迎语 ≤ 2 行：模型、模式、关键命令；长命令清单待 `/help`（`repl.kl:243`）
- [x] 输入框完整边框 + `> ` 提示；模式徽标只在状态行
- [x] 提示行：`Enter 发送 · Ctrl-J 换行 · Tab 补全`；运行中追加 `esc 中断`（取消原语未就绪，暂不显示 esc）
- [x] 状态行全模式：mode + model + work dir + tokens/window
- [x] git 分支只在 cwd 变化时查询并缓存，避免每帧执行
- [x] 忙碌提示：`✻ Thinking…` / `✻ Running <cmd>…` / `✻ Compacting…`，结果返回时恢复状态行
- [x] resize：每帧前和流式 chunk 间轮询 `os.terminal_size`，变化时重排 chrome
- [x] 测试：状态行左侧省略路径、`elide_left`、narrow prompt
- [x] 测试：resize 行布局快照（`ChatRows` 24/12/10/9/8 行），非 tty 边框已冒烟

验收：80/120 列均稳定；运行中不再“静止卡死”；chat 模式也有状态行。

### Phase 3：交互（3–4 天）

- [x] 输入 `/` 或 `/` + Tab 显示分组命令与说明，支持模糊过滤（UI-11）
- [x] 输入 `@` 后列出匹配文件，Tab 选择（UI-12）
- [x] 写文件/执行命令前审批：动作 + diff/命令 + `[y/a/n/d]`，默认拒绝（UI-13）
- [x] `a` 本会话放行；`d` 显示完整 diff；`n` 拒绝并把原因回给模型
- [x] `/help` 分组帮助；未知 `/` 命令本地提示，不再发给模型（UI-14）
- [x] 压缩提示显示前后 token：`↺ 18.2k → 4.1k`（UI-15）
- [x] 测试：面板快照、审批输入、未知命令、压缩提示（审批输入、压缩提示已覆盖）

验收：常见操作无需查文档；危险操作有预览和明确默认值。

### Phase 4：打磨（2 天）

- [x] `/sessions` 对齐表格、当前会话高亮、长名称省略（UI-17）
- [x] `Ctrl-O` 展开最近一次工具完整输出（依赖按键捕获，UI-18）
- [x] `NO_COLOR` / `TERM=dumb` / 非 tty 全链路降级（UI-19）
- [x] 更新 `__chat__.kl` 的 Layout 段与 `/help` 文案
- [x] 验收：80/120 列、管道、`NO_COLOR`、`TERM=dumb`、CJK 会话名、长会话 `PgUp/PgDn`、`Ctrl-O` 无输出提示（伪终端自动冒烟）；tmux 跳过；亮/暗主题仅用 16 色语义，无背景依赖

验收：外观与交互在真实终端环境稳定，无颜色/宽度回归。

## 8. 文件落点

| 文件 | 改动 |
|---|---|
| `demo/chat/repl.kl` | `_Chrome`、`emit`、`draw_transcript`、`draw_input_box`、状态行、欢迎语、命令分发、补全 |
| `demo/chat/agent.kl` | `emit("[step] ...")`/`step_summary` 改为 UI 事件；执行前发 `tool_start`；写前读旧内容用于 diff |
| `demo/chat/session.kl` | `ask` 错误结构化、usage/耗时、`status_line` 扩展；去掉 TUI 里的 `bot>` 前缀 |
| `demo/chat/compact.kl` | 压缩前后 token 事件 |
| `demo/chat/ui.kl`（新增） | 事件类型、纯渲染函数、颜色/glyph token |
| `libs/koala/tui/screen.kl` | 局部重绘、`erase_below`、光标保存/恢复、resize 支持 |
| `libs/koala/tui/view.kl` | 按显示行换行与滚动，或新增 `WrappedView`（不破坏现有 API） |
| `libs/koala/readline` | `wrap_text` 辅助；后期再加单键读取/浮层 API |
| `demo/chat/test/` | UI golden 测试；现有用例保持通过 |

## 9. 测试计划（UI 专属）

- [x] 纯渲染快照：固定 `cols=80/120/40`，断言完整帧（ANSI 归一化；40 已有，80/120 新增）
- [x] 换行正确性：CJK、emoji、组合字符、ANSI 跨行、tab、长 URL、超长无空格路径
- [x] 不丢内容：`render` 后拼接（去 padding/ANSI）等于原文，作为 UI-01 硬验收
- [x] 非 tty/`TERM=dumb`：无 ANSI、无边框、无 spinner；`NO_COLOR`：ASCII 边框、无 ANSI、无 Unicode 字形
- [x] 错误展示：stderr 尾部 N 行、`… N lines`、exit code（现有用例覆盖）
- [x] 状态行：长路径左侧省略、token 百分比、分支缺失时降级（现有用例覆盖）
- [x] resize：`ChatRows` 纯函数重排；小于 10 行（含 9/8 行）自动降级 plain，不再设置非法滚动区
- [x] pty E2E：`chat/test/test_pty.sh` 覆盖 24/10/9 行、<10 行 plain 降级、24→9 行 resize 回退、非 tty 无 ANSI、`/help` 与 `/quit`；已接入 `demo/build-and-test.sh`

## 10. 验收标准（DoD）

- 80 列终端下，启动语、工具结果、错误信息完整可读，无静默截断。
- 一次任务全程：`> 输入` → `⏺ 工具` → `⎿ 结果/diff` → `✓ 完成`，角色一眼可分。
- 失败时屏幕能看到关键 stderr，而不是只有 `exit 1`。
- 状态行在 chat/agent 都显示 model、cwd、上下文占用；运行时更新。
- 运行中有明确忙碌提示；取消能力就绪后显示 `esc 中断`。
- 欢迎语 ≤ 2 行；`/help` 可查全部命令与快捷键；未知 `/` 命令不再发给模型。
- 非 tty 输出纯文本；`NO_COLOR` 下无颜色。
- 现有测试 + 新增 UI 测试全部通过；无回归。
- 文档更新完成（`__chat__.kl` Layout、`/help` 文案）。

## 11. 暂不做

- 鼠标操作
- v1 的浮层菜单/方向键选择（先做行内列表与行输入）
- 完整 Markdown 渲染器、语法高亮引擎
- 主题市场、动画、背景色块
- 侧栏、多面板、图片渲染
- todo/plan 面板（属于功能，放 P2）
- Web 访问、MCP、provider 扩展（属于能力，另立 issue）

## 12. 执行顺序（已完成）

以下为实际执行顺序，已全部完成：

1. Phase 0：纯渲染层 + 6 个 golden 快照（不改行为）。
2. UI-01：软换行 + “渲染不丢字符”测试。
3. UI-03/04/08：工具块、失败尾部、错误块 + 测试。
4. UI-02：去掉每轮全屏清空 + 测试。
5. UI-09/10/06/07：欢迎语、输入框、状态行、spinner。

Phase 3 的审批/命令面板和 Phase 4 的打磨也已完成；tmux 实机验收跳过，
由 pty 冒烟替代。inline scrollback 按第 6 节第 1 条留作后续版本。

---

本文档记录已完成的界面优化；后续能力缺口与新任务统一记录在 `issues/`。