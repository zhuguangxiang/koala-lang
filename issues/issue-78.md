# 窄浮点类型的常量比较在后端失败

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 正确的代码编译失败（`isel.c:317: Why goes here?`）；根因由读代码推断，未验证

> 来源：`docs/Koala_TODO.md` §73（2026-11-07，to fix）

---

### 详细描述

<details>
<summary><b>🔁 最小复现</b></summary>

```kl
func main() {
    let a = float32(1.5)
    let b = float32(2.5)
    print(a < b)
}
```

```text
isel.c:317: Why goes here?
koala: compilation failed
```

</details>

<details>
<summary><b>🔍 影响范围</b></summary>

- `float32(1.5) > 1.0`、`a > 1.0`，以及同样代码换成 `float16`，**都同样失败**。
- `a == b` 与 `a + b` 可以编译。
- `int8` 没有此问题。
- **与窄类型是否声明 `Comparable` 无关**（§66）：去掉声明后仍同样失败。

</details>

<details>
<summary><b>🧠 推断的根因</b></summary>

出问题的行是 `isel_lower_binary` 的 `c1 && c2` 分支，即**两个操作数都是常量**时到达。此为读代码得出的判断，**尚未验证**。

</details>

---

### 修复方案

<details>
<summary><b>🛠 待补充</b></summary>

原记录未给出方案。可先按推断验证：在 `isel_lower_binary`（`isel.c:317` 附近）的 `c1 && c2` 分支补上窄浮点类型的常量比较下降，或让该分支退回非常量路径。

</details>