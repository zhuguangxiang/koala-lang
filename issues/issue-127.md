# 类型系统限制：union 不能作返回类型、tuple 不隐式上转 any、as any 非法、pub 别名

← 返回 [问题列表](issues.summary.md)

- [ ] 未实现 —— 4 项限制均于 2026-11-22 以 koala 0.459（build 505）探针复现；plot `subplots` 多态返回因此用 `any` 绕过

> 来源：plot 兼容性运动（#25 subplots 数组、#26 tight_layout）期间实证；探针文件 `/tmp/probe_issues/p5_union_ret.kl`、`p10_tuple_any.kl`、`p11_as_any.kl`、`p12_pub_alias.kl`

---

### 详细描述

<details>
<summary><b>1️⃣ union 不能作返回类型</b></summary>

```kl
func f(x int) int | str {
    if x > 0 { return x } else { return "s" }
}
```

```text
p5_union_ret.kl:1:15: error: a union type is not allowed as a return type.
```

`float|str` 先例**仅限参数**。返回值需要多态时只能退回 `any`：`subplots()` 因此返回 `(Figure, any)`，调用端 `case is` 收窄。若放开，需一并回答 union 返回值的类型检查/收窄/klc 序列化语义。

</details>

<details>
<summary><b>2️⃣ tuple 字面量不隐式上转 `any`</b></summary>

```kl
class Ax {}
func g() (int, any) {
    return (1, Ax())
}
```

```text
p10_tuple_any.kl:3:5: error: return type mismatch in function 'g'
```

唯一通道是**先赋给 `any` 局部再打包**：`var ret any = Ax(); return (1, ret)`。

</details>

<details>
<summary><b>3️⃣ `as any` 非法</b></summary>

```kl
var a any = 1
var b = a as any
```

```text
p11_as_any.kl:3:18: error: target type of 'as' operator must be a class type.
```

隐式上转进 any 局部/形参/返回是唯一通道；显式 `as any` 没有对应语法（可能与 [#102](issue-102.md) 的 as 目标类型规则同源）。

</details>

<details>
<summary><b>4️⃣ `pub type` 别名语法错误</b></summary>

```kl
pub type H = func(int) int
```

```text
p12_pub_alias.kl:1:5: error: syntax error
p12_pub_alias.kl:1:14: error: expected an expr.
```

`parse_flags` 已处理 alias flags，但文法拒绝 `pub` 修饰别名。别名只能模块内共享；包测试以外部单文件运行，看不到非 pub 别名（visibility 模型）。

</details>

---

### 修复方案

<details>
<summary><b>🛠 方向</b></summary>

- 第 1 项是 4 项里影响最大的（库作者无法表达「返回 A 或 B」），但语义面最宽 —— 建议与 union 收窄（[#118](issue-118.md) 提到联合收窄缺口）一起设计，短期维持 `any` 约定。
- 第 2、3 项互补：要么允许 tuple 元素位置上转 `any`，要么提供显式 `as any`，二选一即可解除「先赋局部」的绕行。
- 第 4 项工作量小：文法给 alias 声明接 `pub`（flags 通道已就绪）。

</details>

<details>
<summary><b>🔗 关联</b></summary>

- [#102](issue-102.md)：`is` / `as` 语义（第 3 项同族）。
- [#119](issue-119.md)：tuple 字面量与期望类型（第 2 项同族）。
- [#118](issue-118.md)：联合收窄缺口（第 1 项放开后的前提）。

</details>
