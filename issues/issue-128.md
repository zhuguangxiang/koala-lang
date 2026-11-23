# `koala/json` 对非预期输入 panic：解析错误不可捕获、NaN/Inf 序列化无通道

← 返回 [问题列表](issues.summary.md)

- [ ] 未实现 —— 2 项均于 2026-11-22 以 koala 0.459（build 505）探针复现；对不可信输入（模型生成/网络 JSON）不安全

> 来源：plot hist（#19，NaN bar 跳过序列化）、chat /agent 协议（改用 panic-free 文本协议）期间的实证

---

### 详细描述

<details>
<summary><b>1️⃣ 解析非法输入 panic，且语言无 try/catch（设计已拒），panic 不可捕获</b></summary>

```kl
import "koala/json"
func main() {
    let v = json.parse_str("{bad")
}
```

```text
Failure in func 'Parser$parse_object'
Traceback (most recent call last):
  File "koala/json/parser.kl", line 105, in Parser$parse_object
  ...
Error: Expected a string key in JSON object
```

进程 exit 0（panic 退出码问题见 [#8](issue-8.md) / [#60](issue-60.md)）。错误处理设计已明确拒绝 try-catch（[#123](issue-123.md)「C 的返回值 + 极简 panic」），因此**解析不可信 JSON 会直接杀死程序**。chat /agent 协议被迫改用 fenced-code 文本协议（split/substr/strip，永不 panic）绕开 JSON。

</details>

<details>
<summary><b>2️⃣ `to_json` 序列化 NaN/Inf panic，无 allow_nan 通道</b></summary>

```kl
import "koala/json"
func main() {
    let o = json.Object()
    o["x"] = json.Float(float64("nan"))
    print(o.to_json())
}
```

```text
Failure in func 'format_number'
  File "koala/json/float.kl", line 40, in format_number
  File "koala/json/object.kl", line 151, in Object$to_json
Error: JSON cannot represent NaN
```

Python `json` 默认 `allow_nan=True`（输出 `NaN`/`Infinity`，非标准 JSON 但可互操作）。Koala 侧没有对应通道，持有非有限浮点的对象（如 hist 密度 NaN、除零结果）整个无法序列化 —— plot 渲染层只能**跳过**非有限 bar 绕过。

</details>

---

### 修复方案

<details>
<summary><b>🛠 方向</b></summary>

- 第 1 项：增加**不 panic 的解析入口**（如 `try_parse_str` 返回 optional/错误对象），或先提供 `is_valid` 预检函数；错误对象形态参考 std 现有风格（无异常机制，见 [#123](issue-123.md)）。
- 第 2 项：`to_json` 加 `allow_nan` 参数（默认 panic 保持现状），或提供 `json.FloatNaN/Inf` 显式常量写法；与 [#122](issue-122.md) 的 inf/nan 分层策略保持一致。

</details>

<details>
<summary><b>🔗 关联</b></summary>

- [#8](issue-8.md) / [#60](issue-60.md)：panic 退出码为 0，工具链无法区分崩溃与成功。
- [#122](issue-122.md)：数值构造器与 `str_to_float` 的 inf/nan 分层策略。
- [#123](issue-123.md)：错误处理设计（拒 try-catch）。

</details>
