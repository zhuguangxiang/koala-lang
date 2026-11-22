# `sealed trait`：封闭的实现者集合

← 返回 [问题列表](issues.summary.md)

- [ ] 未实现 —— 待设计（与 enum、`match` 一起定）

> 来源：`docs/Koala_TODO.md` §55（2026-11-04，to design）

---

### 详细描述

<details>
<summary><b>🎯 动机与现状</b></summary>

像 `koala/toml` 的根 trait `Value`（`Integer`、`String`、`Table` …）这样，可以在不用 `any` 的前提下给一个类型一组有限的 case，读取经 `as` 完成。

但**集合并不封闭**：别的包可以写 `class Foo : toml.Value`。风险不大，因为 `as` 是带检查的转换 —— 最坏结果是 panic 或走默认分支，不会得到垃圾值。

```kl
pub sealed trait Value {
    func to_toml() str
}
```

</details>

<details>
<summary><b>📌 动机来源</b></summary>

`libs/koala/redis_refactor/result.kl` 的 `Result.val` 是 `any` 加一个 `NilValue` 标记。今天就能改走 `koala/toml` 那套模式，**不需要新语法**。

</details>

---

### 修复方案

<details>
<summary><b>✅ 已达成一致</b></summary>

- `sealed trait` 只能由**同一个包**内的 class 实现，跟随 Kotlin。界限是包而非文件，因为 `koala/toml` 一个类型一个文件。
- 别的包仍可使用它：声明变量、调它的方法、用 `as` 转换。
- 别的包里的 trait **不能继承**它，否则那条 trait 就成了绕过限制的通道。
- enum 是一种 sealed 类型，其 case 在自己的定义里创建。

</details>

<details>
<summary><b>❓ 设计时需决定（与 enum、`match` 一起）</b></summary>

- enum 是否是 `sealed trait` 的语法糖，从而共用一份 narrowing 与 `match` 的实现。
- `match` 是否必须穷尽所有 case。
- `permits`（列出已有类型如 `int`、`str`）**不打算做**。像 `koala/toml` 那样包一层可以接受。它只对**没有方法**的 trait 才可能成立，因为内建类型的接口表里不含别的包的 trait。

</details>