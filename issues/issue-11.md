# 11. 原生代码连续分配多个对象时，C 局部变量不是 GC root

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 缺少原生代码临时 root 机制

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

`os.run()`、`os.pipe()` 等原生函数先创建 Raw 对象并存放在 **C 局部变量**中，再创建 Process/tuple 把它们挂上去。若中间某次分配达到阈值触发 GC，**尚未挂上的对象会被误回收**。

原生代码中途回调用户代码（`__hash__`、`__eq__`、`next()` 等）时，C 局部变量持有的新对象同样不受保护（如 dict `|` 的结果、hashset `is_subset` 的临时表中的元素）。

</details>

---

### 修复方案

<details>
<summary><b>🛠 待办 / 候选方向</b></summary>

需要原生代码的**临时 root 机制**（例如 push/pop 到 `KoalaState` 的 root 栈），或保证这些区间不触发 GC。

</details>