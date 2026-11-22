# 9. 误写 `p.0` 访问元组元素时，koalac 段错误

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 仍段错误，未给出语法/类型错误提示

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

元组下标应写 `p[0]`；误写成 `p.0` 时 `koalac` 直接 **Segmentation fault**，`koala` 只打印 `koala: compilation failed`。

应给出语法/类型错误提示，而不是崩掉。

</details>

<details>
<summary><b>🧪 最小复现</b></summary>

```
for p in [("x", "y")] { print(p.0) }
```

</details>

---

### 修复方案

<details>
<summary><b>🛠 建议修法</b></summary>

（原记录未给出方案，待补充。目标是把段错误换成正常的语法/类型错误诊断。）

</details>