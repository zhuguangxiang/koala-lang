# 8. 未捕获的运行时错误退出码为 0

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 运行时错误仍以退出码 0 结束

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

`print(l[-4])` 报 `Error: list index out of range` 并打印 traceback，但 `koala` 进程**退出码是 0**。

</details>

<details>
<summary><b>💥 影响</b></summary>

lit 里无法用 `not koala ...` 断言失败 —— 测试框架依赖退出码判定，退出码 0 会让「本该失败」的用例被记为通过。

</details>

---

### 修复方案

<details>
<summary><b>🛠 建议修法</b></summary>

（原记录未给出方案，待补充。）

</details>