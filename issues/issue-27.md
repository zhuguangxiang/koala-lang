# 27. 参数列表右括号前换行会被 ASI 插入分号而报错

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 非缺陷而是 ASI 既有规则，已按写法规避，报错文案/文法容忍未改

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

多行写函数签名时，若最后一个形参单独占一行、`)` 另起一行，报 `expected a valid function declaration.`：

```
func f(
    a int, b bool
) {}
```

把 `)` 提到最后一个形参同行（`a int, b bool) {}`）即通过；**`(` 之后换行是允许的**。

</details>

<details>
<summary><b>🧠 根因</b></summary>

`bool`/`int`/`str` 等**类型关键字都在 `koala.l` 的 `semicolon()` 结束符集合里**，形参后的换行触发 ASI，插入的 `;` 落在 `param_list` 与 `)` 之间，文法不接受。

形参类型是**自定义类名（ID）**时同样触发。

</details>

<details>
<summary><b>💥 影响</b></summary>

**不是缺陷而是 ASI 的既有规则**，但写多行签名时很容易踩到，且报错位置指向 `(` 而不是换行处，**不易定位**。

</details>

---

### 修复方案

<details>
<summary><b>🛠 建议修法</b></summary>

报错文案提示「参数列表不能以换行结束」，或让 `param_list` **容忍一个尾随 `;`**。

</details>

<details>
<summary><b>🩹 绕过办法</b></summary>

`)` 与最后一个形参同行。

</details>

<details>
<summary><b>🕐 记录状态</b></summary>

2026-11-22 记录于 kparse 编写期间，已按「`)` 与最后一个形参同行」的写法规避。

</details>