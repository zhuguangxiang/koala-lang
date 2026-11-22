# 4. `fs.read_str` 默认 `max = 65_536`

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 默认上限仍在，且截断/报错行为未确认

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

读大文件（如 `src/parser/koala.y`）需显式传 `max`，否则可能被截断（**未确认是截断还是报错**），容易踩坑。

</details>

---

### 修复方案

<details>
<summary><b>🛠 建议修法</b></summary>

（原记录未给出方案，待补充。）

</details>