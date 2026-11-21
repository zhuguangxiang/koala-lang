# koala/ui 设计：纯 Koala 桌面 UI（用户零 HTML/JS/CSS）

状态：设计稿（待批准后实现）。上游依赖 `koala/webview` v1.1（已落地，见
[koala_webview.md](koala_webview.md)）。本文档只覆盖 `koala/ui`；`koala/plot`
是后续 L2 包，不在本文范围。

## 1. 定位与分层

```
L2  koala/plot      图表 API（未来，ML 看板）
L1  koala/ui        纯 Koala widget API —— 用户只写 Koala，本文档
L0  koala/webview   系统 webview 薄包装 —— HTML 是其本质，高级用户直用
```

- 用户写 UI app **只 import `koala/ui`**：widget、布局、事件、主题、定时器
  全是 Koala 对象与方法。HTML/CSS/JS 是 ui 包的**内部实现细节**，用户不可见、
  不需要写、不需要读。
- `koala/webview` 保留不动：它是 L0 逃生舱（自定义页面、navigate 到
  koala/http 服务等），`webview_demo.kl` 是 L0 示例。ui 落地后新增
  `demo/ui_demo.kl` 作为"用户视角"示例，两者并存、定位不同。
- ui 是**纯 Koala 包**（同 math/itertools），无 native 代码、无 .so；
  平台能力全部来自 webview，平台矩阵与 webview 一致（GTK 已实现，
  Windows/macOS 目前是 panic 占位分支）。

## 2. 跨语言先例

| 方案 | 语言侧 API | 渲染后端 | 借鉴/不借鉴 |
|---|---|---|---|
| Python Flet | 纯 Python widget，`on_click=handler(e)` | Flutter 引擎（捆绑） | **事件模型借鉴**（handler+event 对象）；不捆引擎 |
| Python NiceGUI | 纯 Python widget，属性赋值即刷新 | Web（Vue+FastAPI） | **属性 setter 即更新**借鉴；不起 HTTP 服务 |
| Python tkinter | `Button(text=..., command=fn)` | Tk | **构造参数命名 parity**；command 闭包写法受语言限制（§3） |
| Go Fyne | 纯 Go widget | 自绘 OpenGL | 证明"纯语言 widget"路线可行；不自绘 |
| Rust Slint | 专用 DSL | 自绘 | 不引入 DSL，用户只用 Koala |
| pywebview / Wails | 用户写 HTML | 系统 webview | 即我们的 L0，不是 L1 |

结论：**Flet/NiceGUI 式 widget API + 系统 webview 后端**，不捆引擎、不起服务、
不造 DSL。事件回路复用 webview v1.1 已 E2E 验证的 polling bridge
（outbox → eval_sync → Koala → eval），**不需要 native→Koala 回调**。

## 3. 已核实的事实与约束

| 事实 | 出处 | 对设计的影响 |
|---|---|---|
| lambda 不能捕获局部变量/参数/self；无语句体 lambda；无绑定方法值 | 手册匿名函数节（"capturing ... is not implemented yet"） | 事件 handler 只能是**命名函数**，状态走模块级变量或显式 state 对象（§5.4） |
| class 一律 final，不可继承；trait 是接口且无默认实现 | 手册 Classes 节 | 无 Widget 基类继承体系 → **trait Widget + 各 widget 独立类**，公共逻辑放 App/协议层 |
| 函数值一等公民：`type Handler = func(Event)`，可存字段、可传参；但**不能做泛型型参**（`dict[int, Handler]` 报 "type argument must be a class/trait type"，实测） | 手册函数类型节 + U1 实测 | handler 存在 widget 自身（`Button._handler`），不做 App 内注册表（§5.3） |
| 泛型不变：`dict[str,str]` 不 assignable 到 `dict[str,any]` | webview v1.1 实测 | 协议层统一 `json.Value` 承载，不用裸 dict 字面量传参 |
| `koala/json`：`parse_str/loads`、`Object/Array` 构建器、`dumps` | libs/koala/json/__json__.kl | 命令/事件序列化的现成工具 |
| wombat build 只产出 `.klc` 与 `.so`，**无资产拷贝机制** | libs/koala/wombat/build.kl | runtime.html/js/css 无法随 dist 分发为独立文件 |
| 无 `__file__`/模块路径 API，无 include_str 类嵌入语法 | 手册 + src 检索 | 运行时无法定位"包安装目录"→ **runtime 以字符串常量嵌入 .kl**（§4.3） |
| 无线程/协程；webview 页面加载异步；eval_sync 对象结果是 `[object Object]` | webview v1.1 | 单泵循环；mount 后 poll 至就绪；回传一律 `JSON.stringify` |
| `time.Duration/Instant`、`datetime.now()` 可用 | std/time、koala/datetime | 定时器与 demo 时钟不需要 JS |

## 4. 架构

### 4.1 数据流

```
Koala 侧                                页面侧（ui 包内置 runtime）
─────────────────────────────          ─────────────────────────────
widget 树（App.add / 容器嵌套）
  │ 序列化为 mount 命令（json）
  ▼
webview.set_html(RUNTIME)  ──────────►  runtime.js 建 DOM、注册元素表
  │ poll 至就绪
  ▼
app.run() 泵循环：
  1. eval_sync 排空 outbox  ◄──────────  用户点击/输入 → push 事件 JSON
  2. json 解析 → 分发 handler
  3. handler 内 setter 入队增量命令
  4. 到期 timer 触发 handler
  5. eval 批量下发增量命令  ──────────►  runtime.js 按 id 更新 DOM
  6. sleep ~10ms，回到 1
```

- **没有任何 native→Koala 调用**：handler 由 Koala 泵循环自己调用，
  GC rooting 缺口不被触碰。
- 增量命令**按泵迭代批量下发**（一次 eval 一个命令数组），handler 里连续
  set 多个属性只产生一次页面往返。

### 4.2 包结构

```
libs/koala/ui/
  __pkg__.toml          dependencies = ["koala/webview", "koala/json"]
  __ui__.kl             模块文档 + 公开常量（THEME_LIGHT/DARK 等）
  app.kl                App：窗口持有、泵循环、事件分发、命令队列、定时器
  widget.kl             trait Widget + Label/Button/TextInput/CheckBox/Slider
  layout.kl             Row/Column 容器
  event.kl              Event 类 + type Handler = func(Event)
  protocol.kl           命令/事件 JSON 编解码（内部，pub 仅供测试）
  runtime.kl            RUNTIME_HTML/CSS/JS 字符串常量（内部资产，§4.3）
  test/test_ui.kl       headless ut
```

多文件合并为一个包模块（与 koala/json 同机制），用户侧只见 `ui.X`。

### 4.3 runtime 资产：嵌入而非外部文件（v1 决定）

wombat 无资产分发、运行时无模块路径 API（§3），所以 v1 把 runtime 的
HTML/CSS/JS 作为**字符串常量写在 `runtime.kl`**，随 .klc 一起分发——单文件
artifact，零新基建，任何安装方式下都能工作。

- 这不违背"HTML 不硬编码"约定：该约定针对**用户内容**（L0 已提供
  set_html_file 模板资产化）；runtime 是 ui 包自己的代码，等价于 .so 里的
  C 源码，用户不碰。
- 可维护性：runtime 三段各自一个常量、注释分隔；JS 保持 vanilla、无依赖、
  目标 < 300 行。
- v2 升级路径：wombat 增加 assets 拷贝 + 运行时模块路径 API 后，可改为外部
  文件加载，API 不变。

### 4.4 runtime.js 职责（页面侧，用户不可见）

- 接收命令：`mount(tree)` 建整棵 DOM；`set(id, prop, value)` 增量更新；
  `add(parent, node)` / `remove(id)` 动态增删。
- 元素表 `Map<id, element>`；每个可交互元素挂监听，事件 push 进
  `window.__outbox`（JSON 字符串）。
- **XSS 防线**：一切用户文本走 `textContent` / `value` 赋值，命令传输走
  `JSON.stringify/parse`；runtime 不用 innerHTML 插入用户数据。
- 主题：CSS 变量两套（light/dark），`set("app","theme",...)` 切换根类名。
- 就绪信号：`window.__ui_ready = true`，Koala 侧 poll + eval_sync 等待。

## 5. API 设计（用户视角）

### 5.1 App

```koala
let app = ui.App(title str, width int = 900, height int = 640, state any = nil)
```

| 方法/属性 | 语义 |
|---|---|
| `add(w Widget) Widget` | 挂到根布局（默认 Column）；分配 id、绑定到 App（handler 在 widget 自身）；run 前后都可用（run 后走 add 命令） |
| `run()` | 阻塞泵循环直到窗口关闭或 `close()`；内部完成 mount/就绪等待/分发/刷新 |
| `poll() bool` | 高级用户自驱循环（等价 webview.poll + 一轮分发刷新）；run 即 `while poll() {}` |
| `close()` | 主动关窗退出 run |
| `set_theme(name str)` | `ui.THEME_LIGHT` / `ui.THEME_DARK` |
| `set_fullscreen(on bool)` / `get_size() (int,int)` | 透传 webview（平台矩阵同 L0） |
| `set_title(title str)` / `set_size(w,h)` | 透传 webview |
| `every(ms int, fn Handler) Timer` | 周期定时器；`Timer.cancel()` 取消 |
| `state any` | 用户状态对象透传给 handler（`e.state`），避免模块全局变量的多窗口污染 |

### 5.2 Widget（v1 清单）

trait `Widget`（final 类约束下的公共接口，§3）：`wid() int`、
`to_json() json.Value`。各 widget 独立类，setter 即入队增量命令
（NiceGUI 式）：

| Widget | 构造 | 可写属性（setter） | 事件 |
|---|---|---|---|
| `Label` | `Label(text str)` | `set_text(text)` | — |
| `Button` | `Button(text str, on_click Handler? = nil)` | `set_text`、`set_enabled(bool)` | click |
| `TextInput` | `TextInput(placeholder str = "", on_change Handler? = nil, on_submit Handler? = nil)` | `set_text`、`set_placeholder` | change（每次输入）、submit（回车） |
| `CheckBox` | `CheckBox(text str, checked bool = false, on_change Handler? = nil)` | `set_checked`、`set_text` | change |
| `Slider` | `Slider(min float, max float, value float = min, on_change Handler? = nil)` | `set_value` | change |
| `Row` / `Column` | `Row(*children Widget)` / `Column(*children Widget)`（变参） | `add(w)`、`set_gap(px)`、`set_padding(px)`、`set_align(...)` | — |

- 布局 v1 = 盒模型 Row/Column 嵌套 + gap/padding/align，映射 CSS flex；
  不做 grid/绝对定位（§10）。
- 构造参数命名向 tkinter/Flet 靠拢（text/checked/value/on_click），
  降低学习成本。
- handler 也可构造后绑定：`btn.on_click(fn)`（与构造参数等价，后绑覆盖）。

### 5.3 Event 与 Handler

```koala
type Handler = func(Event)        // 命名函数，无捕获（§3 语言约束）

class Event {
    kind str          // "click" | "change" | "submit" | "timer"
    widget Widget?    // 触发源（timer 事件为 nil）
    app App?          // 反向引用，handler 内可继续操作 UI
    state any?        // App 构造时传入的用户状态对象
    value json.Value? // 事件携带的原始 JSON 值（如有）
    func value_str() str      // change/submit 的输入值（U1）
    func value_bool() bool    // CheckBox（U2）
    func value_float() float  // Slider（U2）
}
```

- 分发：handler **存在 widget 自身**（`Button._handler Handler?`），不是
  App 内的注册表——函数类型不能做泛型型参，`dict[int, Handler]` 不可行。
  泵循环解析 outbox JSON 后按 id 查 widget，调 `Widget._handle(e Event)`，
  由各 widget 处理自己拥有的事件种类（Button 只在 click 时调 `_handler`）。
  **未知 id 的事件静默丢弃**（过期页面事件，非错误）；未绑 handler 的按钮
  可点但无动作，与 tkinter 一致。
- handler 内抛 panic → 沿用 Koala 语义直接终止（无 try/catch，不吞异常）。

### 5.4 状态管理（语言约束下的诚实方案）

lambda 无捕获、class final，所以 v1 不提供"闭包按钮"。两种官方写法：

```koala
// 写法一：模块级状态 + 模块级 widget 引用（小 app，最简）
var count = 0
var counter = ui.Label("0")
func on_inc(e ui.Event) {
    count = count + 1
    counter.set_text(format("{}", count))
}

// 写法二：显式 state 对象（多窗口/可测试）
class CounterState { var count int = 0 }
func on_inc(e ui.Event) {
    if let s = e.state as CounterState {
        s.count = s.count + 1
        // 通过模块级 widget 引用或 e.widget 更新界面
    }
}
let app = ui.App("Demo", state = CounterState())
```

闭包捕获落地后（语言层演进）可平滑增加 `on_click { || ... }` 便捷写法，
现有 API 不破坏。

### 5.5 完整示例（demo/ui_demo.kl 目标形态，零 HTML/JS/CSS）

```koala
import "koala/ui"
import "koala/datetime"

var count = 0
var fullscreen = false
var clock = ui.Label("...")        // 模块级 widget 引用，main 里 add
var counter = ui.Label("0")

func on_inc(e ui.Event) {
    count = count + 1
    counter.set_text(format("{}", count))
}

func on_theme(e ui.Event) {
    // CheckBox change：e.value_bool() 取勾选态
    if e.value_bool() { e.app.set_theme(ui.THEME_DARK) }
    else { e.app.set_theme(ui.THEME_LIGHT) }
}

func on_fs(e ui.Event) {
    fullscreen = !fullscreen
    e.app.set_fullscreen(fullscreen)
}

func on_tick(e ui.Event) {
    clock.set_text(datetime.DateTime.now().to_str())
}

func main() {
    let app = ui.App("Koala Desktop App", 900, 640)
    app.add(ui.Row(clock))
    app.add(ui.Row(counter, ui.Button("+1", on_inc)))
    app.add(ui.Row(ui.CheckBox("Dark mode", false, on_theme),
                   ui.Button("Fullscreen", on_fs)))
    app.every(1000, on_tick)
    app.run()
}
```

对比 `webview_demo.kl`：无模板文件、无 outbox 协议、无 eval 字符串、
无就绪等待——全部沉到 ui 包内部。

## 6. 线协议（内部，ut 固化）

命令（Koala → 页面，一次 eval 下发数组）：

```json
[{"cmd":"mount","root":{"id":1,"type":"column","children":[
    {"id":2,"type":"label","text":"0"},
    {"id":3,"type":"button","text":"+1"}]}},
 {"cmd":"set","id":2,"prop":"text","value":"3"},
 {"cmd":"set","id":0,"prop":"theme","value":"dark"}]
```

事件（页面 → outbox，每条一个 JSON 字符串）：

```json
{"id":3,"kind":"click"}
{"id":5,"kind":"change","value":"hello"}
{"id":7,"kind":"change","value":true}
```

- id 由 App 单调分配（0 保留给 app 级命令）；widget 对象持有自己的 id。
- 值类型：str/int/float/bool，json.Value 承载；页面侧 `JSON.stringify`，
  Koala 侧 `json.parse_str` + 类型化 getter（失败 panic，消息含原始串）。
- 协议版本字段 `v:1` 挂在 mount 上，runtime 校验不匹配即报错（防新旧混装）。

## 7. 错误处理（全部 panic，消息精确，ut @should_panic 固化）

| 场景 | 消息（示意，实现时固化） |
|---|---|
| run 后窗口创建失败/无显示 | 透传 webview panic |
| mount 就绪等待超时 | `ui: runtime not ready (timeout)` |
| outbox 事件 JSON 解析失败 | `ui: bad event json: <raw>` |
| 事件 id 无注册 widget | 静默丢弃（页面陈旧事件，非错误） |
| value_* 类型不匹配 | `ui: event value is not <type>` |
| set_theme 非法值 | `ui: unknown theme '<name>'` |
| widget 未 add 就 setter | `ui: widget not added to an app` |
| Win/mac 平台 | 透传 webview 的 not-implemented panic |

## 8. 测试与验收

- **headless ut**（包内 test/，双模式 JIT/--no-jit）：不创建窗口即可测——
  - widget 树 → mount JSON（嵌套容器、id 分配、各 widget 字段）；
  - 命令队列：setter 入队、批量 flush 编码、未 add  panic；
  - 事件解析分发：注入伪造 outbox 串 → handler 被调、值 getter、
    未知 id 丢弃、坏 JSON panic、类型不匹配 panic；
  - 定时器调度逻辑（到期判定用注入时钟，不真 sleep）；
  - 主题/协议版本校验。
- **live E2E**（沿用 webview v1.1 验收法）：demo 模板 + eval 模拟点击，
  断言计数/主题/全屏链路；headless ut 不含窗口 lifecycle。
- **验收**：`demo/ui_demo.kl` 功能对齐 webview_demo（时钟/计数/主题/全屏），
  用户目视确认；包测试双模式全过。

## 9. 实现顺序（每步编译 + ut 随做随加）

1. **U1 骨架**：runtime.kl（mount/set/click）+ protocol + App(run/poll/close)
   + Label/Button + click 分发 → headless ut + 最小 live 探针。
2. **U2 输入**：TextInput/CheckBox/Slider + change/submit + 值 getter + state。
3. **U3 布局与窗口**：Row/Column 嵌套 + gap/padding/align + theme +
   fullscreen/get_size 透传。
4. **U4 定时器与验收**：every/Timer.cancel + demo/ui_demo.kl + E2E + 双模式全量。

## 10. 明确不做（v1）与 v2 候选

**不做**：
- plot/图表（koala/plot 独立包，L2）；
- 对话框/文件选择/剪贴板/多窗口管理（webview v1.2 候选联动）；
- grid/绝对定位/动画/自定义 CSS 注入（破坏"零 CSS"承诺的口子不开）；
- 闭包 handler（语言捕获未落地）；widget 继承体系（class final）；
- native→Koala 回调（GC rooting 缺口未补，polling bridge 已够用）；
- runtime 外部资产文件（无分发机制，§4.3）。

**v2 候选**：Image/Select/ProgressBar/TextArea widget、键盘快捷键、
wombat assets + 模块路径 API 后 runtime 外置、闭包捕获落地后的便捷 handler、
Windows/macOS 后端随 webview 补齐自动获得。