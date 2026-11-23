# 解析与诊断问题：空 case 臂 EOF 无行号、case is 换行体须花括号、裸 var 静默失败、方法链断行

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 4 项均于 2026-11-22 以 koala 0.459（build 505）探针复现

> 来源：plot 兼容性运动（colorbar/subplots/tight_layout）期间实测；部分现象与更早会话的「静默解析失败」记录同族

---

### 详细描述

<details>
<summary><b>1️⃣ 空 `case _:` 臂（无语句）→ `unexpected end of file.`，无行列号，exit 255</b></summary>

```kl
func main() {
    let x = 2
    switch x {
    case 1: { print("one") }
    case _:
    }
}
```

```text
p3_case_empty.kl: error: unexpected end of file.
koala: compilation failed
```

exit 255。错误信息有文件名但**没有行列号**；给 default 臂加任意语句即编译通过。规避：default 臂必须有语句，或整臂删除（仅 `case is` 的 switch 合法）。同族见 [#7](issue-7.md)（单行块里 `let` 解析失败，同款 EOF 无行号）。

</details>

<details>
<summary><b>2️⃣ 泛型 `case is list[T]:` 多语句换行体 → `expected 'case' in switch body.`</b></summary>

```kl
func main() {
    var v any = [1, 2]
    switch v {
    case is list[int]:
        print("list")
        print("again")
    case _: { print("other") }
    }
}
```

```text
p4b_case_is.kl:3:14: error: expected 'case' in switch body.
```

多语句体**必须花括号**（`case is list[T]: { ... }` 通过）。探针实测边界：单语句换行体通过；同行单语句体（`case is list[T]: return x`）通过；普通类的 `case is A:` 换行体通过 —— 只有「泛型 + 多语句换行体」报错。

</details>

<details>
<summary><b>3️⃣ 函数体内无类型无初值的 `var y` / `let y` → 完全静默失败</b></summary>

```kl
func main() {
    var y
    print("a")
}
```

```text
koala: compilation failed
```

exit 255，**零诊断**（连错误行都没有）。`let y` 同形同静默；与声明位置无关（块中/块末都静默）。对照：顶层裸 `var y` 有正常诊断 `1:5: error: expected a type or '='.`。

</details>

<details>
<summary><b>4️⃣ 方法链跨行断行 → 误导性连锁错误</b></summary>

```kl
func main() {
    let s = "abc"
    let n = s
        .len()
    print(n)
}
```

```text
p14_chain.kl:4:9: error: expected '}'.
p14_chain.kl:4:10: error: too few arguments in function call.
p14_chain.kl:5:11: error: 'n' is not found
```

换行处被 ASI 断成两条语句，报错位置与真实原因（不支持换行续链）脱节。同族见 [#27](issue-27.md)（右括号前换行被 ASI 插分号）。规避：链保持一行，或先绑中间局部。

</details>

---

### 修复方案

<details>
<summary><b>🛠 方向</b></summary>

- 第 1、3 项：解析失败路径补诊断（至少行列号）；第 3 项函数体裸声明应复用顶层的 `expected a type or '='.` 诊断。
- 第 2 项：泛型 case-is 臂的换行体与新行体在文法上等价（普通类臂已等价），应统一接受；或给出与普通类臂一致的明确报错。
- 第 4 项：ASI 对「行首 `.`」应视为续行（Python/Ruby 同款规则），或报「不支持续行」的定向错误。

</details>

<details>
<summary><b>🔗 关联</b></summary>

- [#7](issue-7.md)：单行块 `let` 解析失败（EOF 无行号，第 1 项同族）。
- [#27](issue-27.md)：ASI 断行问题（第 4 项同族）。
- [#111](issue-111.md) / [#112](issue-112.md)：switch/case 路线图（第 1、2 项所在功能面）。

</details>
