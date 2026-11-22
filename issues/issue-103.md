# 写 `koala/re` 时遇到的语言问题（6 项 + 1 项命名隐患）

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 6 项语言问题 + `RE_*` 宏前缀隐患；`koala/re` 与测试已全部绕过

> 来源：`docs/Koala_TODO.md` §46（2026-11-01，to fix / to discuss）
>
> 每个用例都在 2026-11-01 实测。

---

### 详细描述

<details>
<summary><b>1️⃣ `ut.assert_eq` 传 `str?` 让编译器 abort（to fix，期望 `ok`）</b></summary>

```kl
import "std/ut"

func main() {
    let a str? = "y"
    ut.assert_eq(a, "y")
    print("ok")
}
```

`typespec.c:1575: Why goes here?`。`re` 的测试改为先用 `!` 解包。

</details>

<details>
<summary><b>2️⃣ list 上的 `==` 比较的是 identity（to discuss）</b></summary>

`print([1, 2] == [1, 2])` 打印 `false`。Python、Go（数组）、Rust 都比较元素。

`re` 的测试用小的辅助函数逐元素比较（`assert_ints`、`assert_strs`）。

</details>

<details>
<summary><b>3️⃣ 没有元组元素访问（to discuss）</b></summary>

`let t = (1, 2)` 后 `print(t.0)` 报 `error: expected an identifer.`（顺带一个拼写错误："identifer"）。

只有解构可用：`let (a, b) = t`。

</details>

<details>
<summary><b>4️⃣ 形参不能与字段同名（to discuss）</b></summary>

类里有 `let x int` 时，`pub func __init__(x int) {}` 报 `parameter 'x' conflicts with field name in class 'P'`。

于是被迫写成 `re.Pattern(text, bits)`（字段是 `pattern` 与 `flags`）、`re.Match(pattern, text, ...)`（字段是 `re` 与 `string`）；Python、Go、Rust 都允许同名（`self.x = x`）。

</details>

<details>
<summary><b>5️⃣ 不支持函数类型形参（to do）</b></summary>

`fn func(Match) str` 这样的形参编译不过（`zip.kl` 里这段代码保持注释）。

它挡住了 Python 的 `re.sub(func, s)`（§41.1 第 2 项）。

</details>

<details>
<summary><b>6️⃣ `uint8` 没有 `__add__` / `__iadd__`（to do，非 bug）</b></summary>

`var u = uint8(250)` 后 `u += uint8(10)` 报 `neither '__iadd__' nor '__add__' is defined for class 'uint8'` —— 符合规则，只是小整数类型的运算符还没实现。

</details>

---

### 修复方案

<details>
<summary><b>🛠 逐项落点</b></summary>

- 第 1 项：`typespec.c:1575` 的 `UNREACHABLE()`，需处理 optional 类型实参（与 [#87](issue-87.md) 第 2 项、[#96](issue-96.md) 第 4 项同族：类型 spec 命名走到 UNREACHABLE）。
- 第 2 项：**待讨论** —— 让 list 的 `==` 比较元素（需决定是否统一到 `__eq__` 协议）。
- 第 3 项：**待讨论** —— 支持 `t.0` 元素访问；顺带修 "identifer" 拼写。
- 第 4 项：**待讨论** —— 允许形参与字段同名（`self.x = x` 语义）。
- 第 5 项：实现函数类型形参（与 §41.1 第 2 项、§49 相关）。
- 第 6 项：给小整数类型补 `__add__` / `__iadd__` 等运算符。

</details>

<details>
<summary><b>⚠️ 另一项：C 宏前缀隐患</b></summary>

`koala/re` 的 C 宏保留 `RE_*` 前缀（`RE_FLAG_*`、`RE_ERR_*`、`RE_MODE_*`、`RE_MATCH`），而函数是 `kl_re_*`。

glibc 的 `<regex.h>` 定义了其它 `RE_*` 宏（`RE_ICASE`、`RE_DUP_MAX`、`RE_SYNTAX_*`）；今天没有冲突，但一个同时包含两个头的源文件可能会冲突。改名为 `KL_RE_*` **有意留到以后**。

</details>