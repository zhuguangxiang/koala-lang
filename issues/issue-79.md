# `bfloat16` 无法构造

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 构造即 `irgen.c:912: Not Yet Implemented`；其比较与 `max` 也无测试

> 来源：`docs/Koala_TODO.md` §74（2026-11-07，to fix）

---

### 详细描述

<details>
<summary><b>🔁 最小复现</b></summary>

```kl
func main() {
    print(bfloat16(1.5))
}
```

```text
irgen.c:912: Not Yet Implemented
koala: compilation failed
```

出问题的行是 `emit_type_call` 的最后一个分支。`bfloat16(float64(1.5))` 同样失败。

</details>

<details>
<summary><b>📌 连带影响</b></summary>

`bfloat16` 自 §66 起声明了 `Comparable`，但它的比较方法与 `max` **没有任何测试**。

</details>

---

### 修复方案

<details>
<summary><b>🛠 待补充</b></summary>

原记录未给出方案。落点在 `irgen.c` 的 `emit_type_call`：为 `bfloat16` 补上构造下降（与 `float16` / `float32` 的既有分支对齐），随后补比较与 `max` 的测试。

</details>