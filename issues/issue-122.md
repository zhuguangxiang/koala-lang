# 数值构造器接收 `str` 的语义（`float64("...")` / `int64("...")`）（待定）

← 返回 [问题列表](issues.summary.md)

- [ ] 待裁定 —— float64 的**字面量路径已落地**；integer 侧仍是「放行后 abort」；ctor 收 `str` 的整体语义待作者拍板

> 来源：`docs/Koala_TODO.md` §22（2026-10-24 建立）
>
> 关联：[#101](issue-101.md)（§14 `from_str` 家族）、[#102](issue-102.md)（§29 `is` / `as`）、§13（数值 cast）、§20（NaN/Inf 构造途径）、§21（`str_to_float` 严格策略）。

---

### 详细描述

<details>
<summary><b>🧪 当前实测状态（22.1）</b></summary>

**float64 字面量路径（已实现）**

- `float64("inf")` / `float64("-inf")` / `float64("nan")` / `float64("3.14")` 在编译期校验并**折叠为 float 常量**：非法字面量报编译错误 `invalid float literal '...'`；运行时 `str` 实参报编译错误并提示改用 `to_float()`。inf / -inf / nan 走 interned tag（IR 单条 `ret_tag`），其余走常量池（`ret_const`）。溢出沿用 `strtod` 语义（`"1e400"` → `+inf`），拼写大小写不敏感。
- 落点在 `src/parser/irgen.c`：`parse_str_to_float` + `emit_float_call`（原 `NYI()` 路径已移除）。
- 测试：`test/test-run/test_float_from_string.kl`（IR 折叠 + 运行时输出）、`test/test-kl/test_float_from_string_failed.kl`（4 个负例）；当时全套 lit 253/253 通过。
- 所以 float64 目前只是**半实现**：字面量可用，运行时 `str` 不可用（被刻意拒绝，引导到 `to_float()`）。

**integer 侧（未实现，且语义层放行）**

- `.kl` 侧 `int64.__init__(x int64 | uint64 | float64 | str, base = 10)`、`uint64` 同形（`libs/std/builtin/number.kl`）—— 语义层因 union 含 `str` 而放行。
- 实测 `int64("255")` / `int64("255", base=16)` / `uint64("7")` / `int64("-12")`：编译直接命中 `src/parser/irgen.c:442: Not Yet Implemented`（`emit_int_call` 的 str 分支）—— **签名承诺了 `str`，实现却 abort**，这是缺陷，不只是设计问题。窄类型（int8/16/32 等）union 不含 `str`，语义层就会先拒，无此问题。
- `test/test-run/test_int64_complete.kl:159-161` 的三个用例仍是注释状态。

**已有运行时能力（与 §14 / §21 同源）**

- `str.to_int()` / `to_int_or()` / `to_float()` / `to_float_or()` 可用（`src/objects/strobj.c`），失败时 `raise_exc_str`（`to_int` → `invalid integer literal`，`to_float` → `invalid float literal`）。
- 底层 helper `src/common/mm.c:163` 的 `str_to_int`：**固定十进制**（`strtoll(..., 10)`）、严格全消费、`ERANGE` 即失败；`src/common/mm.c:189` 的 `str_to_float`：额外**拒绝 inf / nan**，`ERANGE` 一律失败（subnormal 也被拒，见 §21 末尾）。`strtoll` / `strtod` 语义还带来**空白不对称**：`" 12"` 通过（前导空白被跳过）、`"12 "` 失败（未全量消费）。
- 于是出现**同一值两种写法两种结果**：`float64("inf")` 编译通过，而 `"inf".to_float()` 与 `float64(s)`（`s = "inf"`）在运行时被拒。

---

### 修复方案

<details>
<summary><b>❓ 需作者裁定的设计点（22.2，11 条，附建议）</b></summary>

1. **定位**：ctor 收 `str` 是「字面量语法糖」（编译期校验、失败 = 编译错误），还是「运行时解析器」（任意 `str` 值、失败 = 运行时 panic）？**建议两者都要**：字面量走折叠（零成本 + 早报错），运行时走解析；否则签名与能力不一致（即当前现状）。
2. **运行时失败语义**：panic（与 `to_int` / `to_float` 一致）／返回 0 / NaN（静默错误，危险）／只保留 `_or` 与 `try_from` 变体。**建议 panic**，另由非 panic 变体承载可恢复场景。
3. **inf / nan 的一致性（本议题最尖锐处）**：`float64("inf")` 编译期通过，但运行时解析路径（`"inf".to_float()`，json / toml 输入校验依赖它）是拒绝的。选项：
   (1) **分层** —— 字面量路径以源字面量语法为准（接受 inf / nan），运行时路径以安全解析为准（拒绝），需要文档说明 + 另开显式入口；
   (2) 运行时也放宽 —— **破坏性**，json / toml 会接受非标准输入；
   (3) 编译期也收紧 —— 推翻本轮需求，不成立。
   **建议 (1)**，但必须明确「字面量 ≡ 源字面量」的定位，并保留一个运行时接受 inf / nan 的显式 API。
4. **`base` 参数**：只允许编译期常量（折叠可覆盖，但 `int64(s, base)` 只完成一半）／允许运行时值（需运行时解析器接收 base：范围 2–36，`base = 0` 表示按 `0x` / `0b` / `0o` 前缀自动识别；非法 base：常量 → 编译错误、变量 → panic）／从 ctor 移除 base、只留 `str.to_int(base)` 形态。**建议允许运行时值 + `base = 0` 前缀语义**；下划线 `1_000` 不进语言层（toml / json 自带的清理层负责），且对非 `str` 实参传 base 应报错（避免 `int64(5, 16)` 被静默接受）。
5. **位宽与范围**：只有 64 位类型收 `str`（窄类型让用户显式套一层，溢出自行负责）／所有数值类型都收（需为每个类型定义越界语义：`int8("300")` 编译期 = 编译错误、运行时 = panic）。**建议先只做 64 位，但接口按全类型设计**（先在 64 位域解析，再做一次「目标位宽范围检查 + 转换」的独立步骤），第二步只是多接一层，无需推翻解析层。
6. **`uint64` 的负号与上界**：**建议 `uint64("-1")` 直接报错**（不做取模回绕，回绕是静默陷阱）；且 `uint64("18446744073709551615")` 必须通过 ⇒ 需要**无符号解析路径**，不能复用有符号解析（否则合法大值被误判溢出）。
7. **严格性与空白**：现状是 `strtoll` / `strtod` 语义（前导空白通过、尾随失败，**不对称**）。**建议统一为**：先 trim 两侧 ASCII 空白，再要求全量消费；空串 / 纯空白失败；整数仅 `[+-]?[0-9]+`（按 base）；浮点 `[+-]?(digits[.digits][e±digits] | inf | nan)`，尾随垃圾（`"12abc"`、`"3.14abc"`）失败。
8. **折叠触发范围**：只折叠**直接字面量**，还是也折叠常量变量 / 常量表达式？**建议只折叠直接字面量** —— 常量传播类折叠收益小，却会把「哪些写法在编译期报错」变成隐式规则。
9. **折叠层与运行时层必须同源**：两层若各写一套解析规则，必然在 `"1e400"` / `"-0"` / 空串 / subnormal 等边界漂移，出现「const 与非 const 结果不同」的最坏情况。**建议**解析规则只有一份事实来源（共享同一段实现），并用同一份边界用例表同时驱动两层，断言 const 与非 const 等价。
10. **与 §14 `from_str` 家族的关系（必须一起裁定）**：[#101](issue-101.md) 的定案是**把 `str` 从 ctor union 移除**，改用 `from_str_or` / `try_from_str`（minimize-panic + fallback 必传）。若采纳 §14，则本节的 ctor 路径最终可能被删掉、只保留「字面量折叠」，甚至折叠也一并迁到 `from_str`；若不采纳 §14，则 ctor 收 `str` 就成为正式入口。**两个方向一起定，否则会做出两套 API。**
11. **别名**：`int` 已被当作 `int64` 使用（`test/test-run/test_float_cast_4.kl:3`、`libs/std/builtin/str.kl:138`）；`float` 是否有同等级别名**未核实**。文档需写明究竟哪几个类型名收 `str`。

</details>

<details>
<summary><b>🧭 候选整体方案（22.3）</b></summary>

| 方案 | 内容 | 代价 |
|---|---|---|
| **A 仅编译期折叠** | 字面量折叠 + 编译期校验；运行时 `str` = 编译错误（= 当前 float64 行为） | ctor 的 `str` 是半成品；`int64(s, base)` 形同虚设；用户被迫改用 `to_int()` / `to_float()` |
| **B 折叠 + 运行时（倾向）** | 字面量折叠；运行时 `str` 走原生解析（panic）；配套 `_or` / `try_from` | 需补运行时解析入口（含 base、无符号）；须解决第 3 / 9 条一致性；若动 std 签名则需重新生成并入库发行产物 |
| C 只走运行时 | 一律运行时解析，取消折叠 | **倒退**：字面量非法从「编译错误」退化为「运行时 panic」；`float64("inf")` 又回到不支持 |
| D 只留 §14 的 from_str 家族 | ctor 去掉 `str`，全部走 `from_str_or` / `try_from_str` | 破坏性（删签名元素）；`float64("inf")` 诉求需换写法 |

</details>

<details>
<summary><b>🚧 若要落地（建议顺序，22.4）</b></summary>

1. 先定规范：解析语法 + 失败语义 + 边界值表（inf / nan、base、空白、溢出、subnormal）。
2. 统一底层解析实现，让同一份用例表同时驱动折叠层与运行时层（第 9 条）。
3. 补运行时入口：整数需要带 base 的能力（现有解析固定十进制）；无符号需独立路径；失败信息与既有 `invalid integer literal` / `invalid float literal` 对齐。
4. 接通 ctor 的运行时分支；`base` 为变量且落在区间外时 panic。
5. 若扩展 `str.to_int(base = 10)` 这类签名，标准库发行产物需重新生成并入库（与 §21 同类成本，注意那是入库的二进制产物）。
6. 测试矩阵：值域边界 × 调用场景（直接调用 / 返回 / 默认参数 / 泛型 / 常量声明）× 两个后端结果一致 × 错误信息文本。
7. 文档：把「字面量等价」与「运行时解析」的差异（尤其 inf / nan）写成用户可见说明。

</details>

<details>
<summary><b>⚖️ 作者裁定（待填，22.5）</b></summary>

- （待讨论：**先定第 1、3、10 三条**，其余可跟随）

</details>
</details>