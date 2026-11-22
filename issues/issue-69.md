# 类名 / trait 名作值使用需要一致的报错

← 返回 [问题列表](issues.summary.md)

- [ ] 部分修复 —— 元组构造（含 `print` 变参）已报 irgen 错，未覆盖全部语境与 trait 名

> 来源：`docs/Koala_TODO.md` §54（2026-11-04，partially checked, to fix）
>
> 与 §53（未初始化局部变量的读取）一起在 `test2.kl` 里发现 —— 见 [#70](issue-70.md)。

---

### 详细描述

<details>
<summary><b>🔁 最小复现</b></summary>

```kl
class P {}
print(int)
print(P)
print(ToString)
let t = int
```

</details>

<details>
<summary><b>🔍 现状</b></summary>

值位置上的类名可以到达 irgen 并成为类引用（`KLR_VALUE_KLASS`），但后端**没有把它载入寄存器的指令**。trait 名同样缺一条完整的值路径。

元组构造中的类对象（包括 `print` 的变参）现在会得到 irgen 错误；但这**未覆盖全部值语境，也未覆盖 trait 名**。

</details>

<details>
<summary><b>✅ 附带修复（2026-11-03）</b></summary>

`list[any]()` 曾在 `parse_type`（`parser_expr.c`）断言 —— 那里期望一个 class，而关键字类型 `any` 是 trait（`libs/std/builtin/any.kl`）。现在接受 class 或 trait，其它一律报错。

测试：`test/test-run/test_generic_any_arg.kl`。

</details>

---

### 修复方案

<details>
<summary><b>🛠 语义分析处统一报清晰错误</b></summary>

在类名 / trait 名尚未成为一等值之前，只要它们被用作**值**，就在语义分析阶段报清晰的编译错误，覆盖：

- `let` / `var` 初始化式
- 赋值
- 调用实参
- 返回值
- 元组及其它容器元素
- 嵌套表达式

并覆盖局部 / 外部 / 内建 / 泛型四种类型名，同时测优化与非优化编译。适当时保留既有更具体的类型错误；**不允许非法输入走到编译器断言**。

</details>

<details>
<summary><b>⚠️ 需区分「类型名表达式」与「实例表达式」</b></summary>

一个表达式的符号本身也可能指向某个实例的类。以下语境必须保持原样：

- 类型标注
- 构造与转换调用（`int(x)`、`P()`）
- 泛型类型实参（`list[int]`）
- 静态成员访问（`bytes.from_hex`）
- class / trait 的实例值（含 trait 类型的变量、参数与返回值）
- `typeof` 运行期返回的值

</details>