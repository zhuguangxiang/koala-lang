# 用户类实例的索引会让编译器 abort

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— `x[i]` / `x[i] = v` 命中 `irgen.c:1490: Why goes here?`；已有可行方案未实施

> 来源：`docs/Koala_TODO.md` §42（2026-11-01，to fix）

---

### 详细描述

<details>
<summary><b>🧠 根因</b></summary>

任何定义了 `__getitem__` / `__setitem__` 的用户类实例上的 `x[i]` 与 `x[i] = v` 都会让编译器 abort：`irgen.c:1490: Why goes here?`。

parser 已经解析并检查了这次调用（`parser_expr.c` 的 `parse_index_load()` / `parse_index_store()`），但 `emit_ir_index()`（`irgen.c`）只下降序列（`KLR_SEQ_SET` / `KLR_SLICE_SET`）与 map，其它情况走到 `UNREACHABLE()`。

它也会命中 std 里带 `__getitem__` 的类（`SortedDict`、`LinkedList`、`FrozenBytes`），并挡住 `re.Match` 上的 `m[g]`（[#109](issue-109.md) 第 3 项）。

</details>

<details>
<summary><b>🔁 失败用例</b></summary>

**读**（`irgen.c:1490: Why goes here?`；期望 `4`）：

```kl
class C {
    pub func __getitem__(i int) int = i * 2
}

func main() {
    let c = C()
    print(c[2])
}
```

**写**（同样 abort；期望 `7`）：

```kl
class G {
    var _c list[int]

    pub func __init__() { _c = [0] }

    pub func __getitem__(i int) int = _c[i]

    pub func __setitem__(i int, v int) { _c[i] = v }
}

func main() {
    let g = G()
    g[0] = 7
    print(g[0])
}
```

**索引上的复合赋值即使在 list 上也失败**，报 `error: cannot resolve expr's type.`（期望 `7`）；类实例上同理：

```kl
func main() {
    let l = [1, 2]
    l[1] += 5
    print(l[1])
}
```

更完整的回归测试（含 union key —— `__getitem__(key int | str)`，用 `is` 窄化 —— 与泛型类 `Box[T]`）保留为 `test/test-run/test_class_index.kl.disabled`；修好后重命名为 `.kl`。

</details>

---

### 修复方案

<details>
<summary><b>❌ 已否决的尝试（2026-11-01）</b></summary>

在 `KlrIndexInfo`（`ir.h`）里加第三种索引种类 `KLR_METHOD_SET`，把 `__setitem__` 的 callable 与返回类型带到 `emit_ir_simple_assignment()`，并在 `emit_ir_index()` 里直接调 `__getitem__`。

它通过了上面的测试，但**被回退**：

- `KLR_SEQ_SET` / `KLR_SLICE_SET` 代表真实的 IR 指令；`KLR_METHOD_SET` 不代表任何指令，只是借共享的 IR 结构夹带 irgen 数据。
- 它需要自己复制一份 `emit_ir_call()` 的实参处理（接口上转、trait 约束的类型参数做 cast），却仍漏掉普通方法调用会处理的东西：默认参数、其它模块的类、trait 接收者、继承来的方法。
- 复合赋值（`x[i] += v`）仍不支持。

</details>

<details>
<summary><b>✅ 方案 A（首选）：在 parser 里做语法糖</b></summary>

当下标对象是用户类时，把 `x[i]` 改写成调用 `x.__getitem__(i)`，把 `x[i] = v` 改写成调用语句 `x.__setitem__(i, v)`。

`parse_index_load()` / `parse_index_store()` 已经有方法符号并检查了实参，所以 irgen **无需改动**，方法调用的每一种情形都与今天一致。

`x[i] += v` 后来可以变成 `x.__setitem__(i, x.__getitem__(i) + v)`（`x` 与 `i` 只求值一次）。

</details>

<details>
<summary><b>方案 B（改动更小）：只改 irgen，不动 IR 结构</b></summary>

加载仍保留直接调 `__getitem__`；存储时让 `emit_ir_index()` 只求值 `x` 与 `i`，再由 `emit_ir_simple_assignment()` 用 IndexExpr 的子节点构造 `__setitem__` 调用。

**缺点**：仍会复制调用实参的处理逻辑。

</details>