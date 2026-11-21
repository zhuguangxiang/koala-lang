# koala/webview 能力补齐方案（v1.1）

范围：**只补 webview 包**。ui/plot 是后续独立的包（ui 完全不感知 HTML/CSS/JS），
本轮不涉及；webview 补齐后将成为它们的底层支撑。
本文档最初基于对 vendored webview 0.12.0 源码的逐项核实；此后上游已整体移除，
窗口引擎改为**自写纯 C 实现**（`native/webview_impl.c`，GTK/WebKitGTK C API），
不引入任何第三方代码。下文第 2 节的上游行号是当时的核实记录，行为语义在自写
引擎中逐项保留。

---

## 1. 现状与缺口

现有表面（`libs/koala/webview/webview.kl` + `native/webview_shim.c`）：

```
Window: __init__(debug) / set_title / set_size(w, h, hints) / navigate /
        set_html / init_js / eval / run / terminate / destroy
模块:   version, HINT_NONE/MIN/MAX/FIXED
```

缺口（对照未来 ui/plot 包装层的需求）：

1. `eval` 发射后不管，**没有 JS→Koala 的数据回传通道**；
2. `run` 是唯一事件循环入口且阻塞至关窗，**无法做 Koala 侧 pump 循环**；
3. HTML 只能以字符串硬编码传入，**没有模板资产化能力**；
4. 窗口管理只有 title/size，缺全屏/尺寸查询等 UI 开发基本件。

## 2. 已核实的实现依据

| 事实 | 位置 |
|---|---|
| GTK 后端 run 循环 = `g_main_context_iteration(nullptr, TRUE)` | webview.h:1952-1956（run_impl） |
| 窗口销毁时引擎置 `m_window=nullptr`；公开 C API `webview_get_window()` 随之返回 NULL → **关窗可检测** | webview.h:1860-1864（on_window_destroy）、webview.h:284 |
| 公开 C API `webview_get_native_handle(w, kind)` 可取 UI_WINDOW（GtkWindow*）/ UI_WIDGET / BROWSER_CONTROLLER（WebKitWebView*） | webview.h:294、枚举 154 行附近 |
| 上游 eval 用 `webkit_web_view_evaluate_javascript`（WebKit≥2.40）/ `webkit_web_view_run_javascript`（旧版），均支持带回调取结果 | webview.h:2001-2016（eval_impl） |
| 页面未加载（URI 为 null）时上游 eval **静默跳过** | webview.h:2002-2005 |
| `destroy()` 必须手动调用（运行时暂不执行 `_free`） | webview_shim.cpp:13-15 |
| 现有 ut 约定 headless 可跑；窗口 lifecycle 走 manual example | test/test_webview.kl 注释 |
| 窗口引擎为自写纯 C 实现，无 vendored 第三方代码 | native/webview_impl.c、native/webview.h |

**关键结论**：`eval_sync` 的结果回调全程停留在 C++ 内，不发生 native→Koala
调用，因此**不触碰 GC rooting 缺口**——bind/dispatch 依然封锁，但数据回传
通道可以先行落地。

## 3. 新增能力

### A. 事件循环控制（pump 架构地基）

| API | 语义 |
|---|---|
| `Window.poll() -> bool` | 非阻塞步进事件循环一次（`g_main_context_iteration(nullptr, FALSE)`）；返回窗口是否仍打开（`webview_get_window() != NULL`）。窗口已销毁时 panic |
| `Window.is_open() -> bool` | 只查询不步进；供 eval 前守卫。窗口已 destroy() 时 panic |

- `run()` 保留不动：阻塞模式与轮询模式并存。
- 典型 pump：`while w.poll() { let evs = w.eval_sync("window.__outbox.splice(0)"); ... }`。
- 平台结构：shim 按上游后端宏三分支（`WEBVIEW_GTK` / `WEBVIEW_EDGE` /
  `WEBVIEW_COCOA`，与 webview.h 内部结构一致）。GTK 已实现；Windows
  (WebView2) / macOS (WKWebView) 分支为 **panic 占位**（注释标明 TODO 实现
  路径），后续补齐。Koala 侧 API 全平台同一套，自写引擎行为与上游语义一致。

### B. JS→Koala 数据回传

| API | 语义 |
|---|---|
| `Window.eval_sync(js str, timeout_ms int = 5000) -> str` | 执行 JS 并取回结果的字符串形式；超时 panic |

实现要点：
- 经 `webview_get_native_handle(BROWSER_CONTROLLER)` 取 WebKitWebView*，
  调上游同款 evaluate API **带 GAsyncReadyCallback**；finish 回调里
  `jsc_value_to_string` 取字符串，嵌套 `g_main_context_iteration(nullptr, TRUE)`
  泵至完成；`g_get_monotonic_time` 计时，超过 timeout_ms panic。
- 兼容上游两个分支：WebKit≥2.40 `evaluate_javascript` / 旧版 `run_javascript`。
- **对象结果约定**：JSC 对对象只给 `[object Object]`，调用侧须
  `JSON.stringify`（文档写明；桥的 outbox 协议照此设计）。
- 守卫：窗口已销毁 → panic；页面未加载（URI null）→ **显式 panic**
  （修正上游静默跳过的行为，错误信息说明需先 set_html/navigate 并 poll 至加载完成）。
- `eval(js)`（fire-and-forget）保留，两者并存。

### C. 模板资产化（HTML 不再硬编码）

| API | 语义 |
|---|---|
| `render_template(text str, vars dict[str, any]) -> str` | 模块级纯 Koala 函数；`{{key}}` 占位替换为 `format("{}", vars[key])`；缺 key/非法占位 panic |
| `load_template(path str, vars dict[str, any]? = nil) -> str` | 模块级；fs 读文件 → render_template；文件不存在/不可读 panic |
| `Window.set_html_file(path str, vars dict[str, any]? = nil)` | load_template → set_html |

- 替换语义 v1 为**原样替换**：本地桌面应用、数据属调用者，且桥注入 JSON
  需要原样；HTML 转义责任在调用者，文档写明。
- 不做循环/条件/转义开关等高级语法，不引 Jinja 类引擎。
- 资产约定：HTML/JS 以独立文件存放（用户项目文件或包内资产），运行时读取；
  后续桥的 runtime.js 遵循同一约定。
- `{{key}}` 匹配规则：key 为标识符字符集；未闭合/非法占位符按原文保留还是
  panic，实现时以"缺 key panic、非法语法 panic"为准，ut 固化。

### D. 窗口管理增强

| API | 语义 |
|---|---|
| `Window.set_fullscreen(on bool)` | 经 UI_WINDOW handle 调 `gtk_window_fullscreen/unfullscreen` |
| `Window.get_size() -> (int, int)` | 经 UI_WINDOW handle 调 `gtk_window_get_size`；响应式布局/canvas 尺寸需要 |

- 目前仅 GTK 分支实现；Windows/macOS 分支 panic 占位（与 A/B 相同的三分支
  结构，见 A 节平台结构说明）。
- handle 为 NULL（窗口已关）→ panic。

### E. 明确不做（v1.1）

- **bind/dispatch**（native→Koala 回调）：仍被 GC rooting 缺口阻塞；A+B 已覆盖
  未来桥的需求，留待运行时回调机制落地后升级。
- 文件对话框、剪贴板、多窗口管理、下载、打印：v1.2 候选。
- `set_icon`：GTK4 无对应窗口 API，不做。
- 模板高级语法、`eval` 改为同步：不做（保持向后兼容）。

## 4. API 汇总（补齐后）

```
Window:
  __init__(debug bool = false)
  set_title(title str) / set_size(w int, h int, hints int = HINT_NONE)
  set_fullscreen(on bool)                    [新 D]
  get_size() -> (int, int)                   [新 D]
  navigate(url str) / set_html(html str)
  set_html_file(path str, vars dict[str, any]? = nil)   [新 C]
  init_js(js str)
  eval(js str)                               [现状，fire-and-forget]
  eval_sync(js str, timeout_ms int = 5000) -> str   [新 B]
  poll() -> bool                             [新 A]
  is_open() -> bool                          [新 A]
  run() / terminate() / destroy()            [现状]
模块:
  version, HINT_*                            [现状]
  render_template(text str, vars dict[str, any]) -> str      [新 C]
  load_template(path str, vars dict[str, any]? = nil) -> str [新 C]
```

## 5. 测试与验收

- **headless ut**（包内 `test/`，`@test`，双模式 JIT/--no-jit 全过）：
  - render_template：正常替换、多 key、重复 key、缺 key panic、非法占位 panic、
    空 vars/空文本边界；
  - load_template：读文件往返、默认 vars、文件不存在 panic；
  - 现有 version/constants 用例保留。
- **manual example**（沿用现有 headless 约定，lifecycle 不进 ut）：
  counter demo 一条链路验证 A+B——按钮点击 → JS outbox → `poll()` 步进 →
  `eval_sync` 取回事件 → `eval` 更新标签；关窗时 poll 返回 false 退出循环。
- 每步编译 + 包 runner（build-and-test.sh）全过；D 的全屏/尺寸在 demo 中目视验证。

**验收结果（2026-11-21）**：headless ut 19/19，JIT 与 --no-jit 双模式全过；
自动化 E2E（真实 demo 模板 + 模拟点击）验证 点击→outbox→eval_sync→Koala 计数→
eval 回推 全链路（计数 3、全屏切换正常）；eval_sync 错误分支（未加载页面、
JS 异常、同步死循环超时、窗口关闭）均实测触发预期 panic。
demo：`demo/webview_demo.kl` + `demo/webview_demo.html`（模板资产化示例）。

## 6. 实现顺序

A（poll/is_open）→ B（eval_sync）→ C（模板）→ D（窗口管理）→ 验收 demo。
每个能力落地即编译验证，测试随做随加。