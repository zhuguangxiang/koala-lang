# `as!`：已知类型时的强制转换（待讨论）

← 返回 [问题列表](issues.summary.md)

- [ ] 未实现 —— 提案已成形，待决定是否接受

> 来源：`docs/Koala_TODO.md` §82（2026-11-08，to discuss）

---

### 详细描述

<details>
<summary><b>🎯 现状与痛点</b></summary>

当代码知道值的类型时，从 `any` / trait / union 里取出它是常见需求：`koala/cli` 的 Namespace 值、json 的 `Value`、`any` 的容器。今天要写成：

```kl
let out = (args.get("output")! as str)!     // get() returns any?
```

第一个 `!` 表示「非 nil」，`as` 给出 `str?`，第二个 `!` 表示「它是 str」。

写法正确、也符合 §29（`as` 之前先解包 optional），但有两个代价：

- 每次这种读取都要写**括号**。
- 转换失败时 panic 成「unwrap nil」，**报错里既没有期望类型也没有实际类型**。

</details>

<details>
<summary><b>💡 提案</b></summary>

`x as! T`，`as` 的强制形式：等同于 `(x as T)!`，结果类型是 `T` 而非 `T?`。转换失败时 panic 并带上两个类型，例如 `cast failed: expected 'str', got 'int64'`。

```kl
let out = args.get("output")! as! str
```

</details>

<details>
<summary><b>📌 提案的性质与已否决项</b></summary>

- 源**仍不能**是 optional：`as!` 不解包 nil，前面那个 `!` 才解。nil 检查与类型检查保持分离，这是 §29 的裁定。
- `!` 本来就表示「我确定，否则 panic」；`as!` 把它用到转换上。Swift 有同样的 `as!` / `as?` 一对。
- **没有隐藏机制**：它就是直接写出的 `(x as T)!`。代价是多一个运算符，与设计原则 3 相悖。
- **已否决**：让 `as!` 同时接受 optional 源、一次做完 nil 检查与类型检查 —— 那会推翻 §29。

</details>

---

### 修复方案

<details>
<summary><b>🛠 若接受</b></summary>

- 检查器给 `as!` 类型 `T`。
- irgen 发射 `as` 加一次 nil 检查，或发射带两个类型的 panic。
- 测试：成功、类型不符、optional 源（应为错误）。

</details>