# 未初始化局部变量的读取：无 definite-assignment 检查

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 编译器**完全没有** definite-assignment 分析；手册已先行描述

> 来源：`docs/Koala_TODO.md` §53（2026-11-03，to fix）
>
> ⚠️ §93（2026-11-12）把同一问题查得更宽（无诊断 + 垃圾值 / nil / **两处段错误**），尚未迁移，范围上以 §93 为准。

---

### 详细描述

<details>
<summary><b>🔁 最小复现（`test2.kl`）—— 静默错值，然后 VM 崩溃</b></summary>

```kl
func test(j int) {
    var i int
    if j > 10 { i = 10 } else { }
    print(i, typeof(i))   // j = 2: prints the module search-path list and <class 'list'>
    print(i + 1)          // VM assertion in vm_ops.h (operand tag check)
}

func t() {
    var i int
    print(i)              // prints 40960
}
```

</details>

<details>
<summary><b>🧠 根因</b></summary>

没有初始化式的局部变量拿到的寄存器仍持有**上一次使用**留下的内容，而读取它被接受。

`docs/Koala_Language_Manual.md`（Constants and Variables）写的却是相反的：「Every variable must be definitely assigned before it is read; the compiler rejects uses of uninitialized values」—— **手册领先于编译器**。

</details>

---

### 修复方案

<details>
<summary><b>🛠 修复方向（原记录标注 not applied）</b></summary>

在检查器（`parser.c` / `parser_expr.c`）里加一趟 **definite-assignment**，对每个函数体逐局部变量跟踪「到达该读取点的每条路径上是否已赋值」：

- `x = ...` / 带初始化式的 `let` / `var` / 元组赋值之后算已赋值。
- `if` 只有在**每个分支**都赋值时才算赋值（以 `return` / `break` / `continue` / `panic` 结尾的分支不必赋值）。
- 循环体里的赋值**不**为循环之后的代码算数。
- 覆盖 let-else、`if let`、`while let`、`for` 变量与 `if` 表达式。
- 在读取点报 `variable 'i' is used before being assigned`。

</details>

<details>
<summary><b>⏳ 过渡选项</b></summary>

在上述完成之前，二选一：给这类局部变量一个**其类型的零值**，或者把手册改成与实现一致。

</details>