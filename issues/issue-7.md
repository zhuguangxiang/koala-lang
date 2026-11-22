# 7. 单行块里 `let` 解析失败

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 单行块内 `let` 仍报 unexpected end of file

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

`for i in range(2) { let _x = 1 }` 报 `error: unexpected end of file.`（**无行号**）。

</details>

<details>
<summary><b>📊 边界对照</b></summary>

| 写法 | 结果 |
|---|---|
| `for i in range(2) { let _x = 1 }` | ❌ `unexpected end of file.`（无行号） |
| 换成多行块 | ✅ 正常 |
| `{ print(i) }` 单行 | ✅ 正常 |

即：**单行块本身没问题，单行块里的 `let` 才触发**。

</details>

---

### 修复方案

<details>
<summary><b>🛠 建议修法</b></summary>

（原记录未给出方案，待补充。）

</details>