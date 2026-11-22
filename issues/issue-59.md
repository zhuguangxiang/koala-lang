# 运算符按操作数类型重载（`-` 同类相减）—— 编译器级，待讨论

← 返回 [问题列表](issues.summary.md)

- [ ] 未实现 —— 编译器级改动，待讨论

> 来源：`docs/Koala_TODO.md` §24

---

### 详细描述

<details>
<summary><b>🎯 诉求</b></summary>

Python 的 `datetime - datetime → timedelta` 与 `datetime - timedelta → datetime` 共用 `-` 运算符，靠操作数类型区分。

Koala 目前**不允许同名 dunder 运算符方法按参数类型重载** —— 写两个 `__sub__` 触发 `error: redefinition of '__sub__'`（实证于 `libs/koala/datetime/date.kl`、`datetime.kl`），`a - b` 只能绑**唯一一个**签名。

</details>

<details>
<summary><b>🔧 当前库侧妥协</b></summary>

`-` 绑 `±Duration` 移位，两值求差降级为具名方法 `diff(o) -> Duration`；`Date` / `DateTime` 皆如此。

功能不缺，但与 Python 心智不一致，`diff()` 是明显的**语言限制外溢**。

</details>

---

### 修复方案

<details>
<summary><b>🛠 待讨论</b></summary>

原记录未给出方案，待补充。核心决策点：

1. 是否放开同名 dunder 运算符按参数类型重载（影响语义层的重定义检查）。
2. 若放开，`a - b` 的解析时机（编译期按实参类型选签名 vs 运行期分派）。
3. 放开后 `diff()` 是否保留为别名。

</details>