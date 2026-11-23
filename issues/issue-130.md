# 130. `in` 配合 `!` 与 `||`：编译断言崩溃或条件恒假

← 返回 [问题列表](issues.summary.md)

- [x] 已修复 —— `parse_in` 对 optional 容器报错而非断言崩溃（2026-11-22）；两个回归用例落地
- [x] 崩溃形态：`str?` / `Object?` 作容器时 `koalac` abort，现为类型错误
- [x] 正确写法确认：`"a" in s!`（或先窄化）
- [x] 回归测试：语义用例 + 报错用例各一

> 来源：实现 native tool-call 参数校验（`demo/chat/tools.kl`）时发现

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

`if !("path" in a) || !("content" in a) { ... }` 有两种错误表现，取决于 `a` 的静态类型来源：

1. **编译崩溃（容器为 optional）**：
```text
koalac: /home/james/Codes/koala-lang/src/parser/parser_expr.c:5089: parse_in: Assertion `container_ts_sym' failed.
koala: compilation failed
```
2. **静默恒假（`a` 由 `Object?` 窄化而来）**：能编译，但缺字段时条件仍为 `false`，分支不进入。

同一表达式拆成两个独立 `if` 后行为正确。

</details>

<details>
<summary><b>🧠 根因</b></summary>

`optional_type_spec_intern()`（`src/parser/typespec.c`）给 optional 类型的 TypeSpec 显式置 `sym_id = -1`，而 `parse_in()` 拿容器类型后直接查符号：

```c
Symbol *container_ts_sym = get_symbol_by_id(container->ts->sym_id);
ASSERT(container_ts_sym);
```

于是任何 `T?` 容器都必然查不到符号，debug 构建 abort，release 构建则退化为空指针解引用。`in` 需要具体容器类型来解析 `__contains__`，optional 容器本身不是合法容器。

</details>

<details>
<summary><b>🧪 最小复现</b></summary>

崩溃版：
```koala
from "koala/json" import parse_str, Object

func main() {
    let a = parse_str("{\"path\": \"a\"}") as Object
    if !("path" in a) || !("content" in a) { print("nil") } else { print("ok") }
}
```
`func check(s str?) bool { return "a" in s }` 同样崩溃（与 `!`、`||` 无关）。

修复后的正确写法：
```koala
func check(s str?) bool { return "a" in s! }
```

</details>

<details>
<summary><b>📌 影响范围</b></summary>

- 语法/类型推断层：`in` 运算符的容器类型检查。
- 危害最大的是「能编译但恒假」形态：语义错误且无任何诊断。
- 现有代码库未使用该组合，属潜在缺陷。

</details>

---

### 修复方案

<details>
<summary><b>🛠 已实施</b></summary>

- `parse_in()` 在访问容器后新增 optional 检查并报错，不再走到断言：

```c
if (type_is_optional(container->ts)) {
    kl_error(in->op_loc, "optional container type is not allowed in 'in' operator.");
    return;
}
```

- 新增 `test/test-run/test_in_not_or.kl`（语义）：
  - `dict[str, str]`、带 `__contains__` 的 `Object`、`Object?` 窄化结果、`v!` 解包四种来源；
  - 缺键时 `!(a in x) || !(b in x)` 为 `true`，两键齐备为 `false`。
- 新增 `test/test-kl/test_in_optional_container.kl`（报错）：
  - `str?` 形参、`Object?` 形参、`as Object` 结果三处都必须报 `optional container type is not allowed in 'in' operator.`；
  - 正例覆盖 `"a" in s!`、`"path" in o!` 与 nil 判断窄化后使用。

</details>

<details>
<summary><b>✅ 验证</b></summary>

- `lit --filter='test_in_not_or|test_in_optional_container'` → 2/2 PASS（Total Discovered 635）。
- `koala test/test-run/test_in_not_or.kl` → `true / true / false / true false / true false`，exit 0，无 warning。
- `koalac test/test-kl/test_in_optional_container.kl` → 3 条 error，exit 255，无断言崩溃。

</details>

<details>
<summary><b>📝 备注</b></summary>

- 「恒假」形态未能复现：修复前后在 dict / `Object.__contains__` / `Object?` 窄化 / `v!` 四种来源下 `||` 语义均正确，判定为旧构建产物（stale `.klc`）造成的假象，不是独立缺陷。
- `demo/chat/tools.kl` 已改回等价的 `!("path" in a) || !("content" in a)` 写法（issue 修复后回改，`wombat test chat` 已验证）。

</details>