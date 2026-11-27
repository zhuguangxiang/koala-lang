# 134. enum 靠 int64 回退能上转成 `Equatable[int]` / `Comparable[int]`

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复：`let x Comparable[int] = C.RED`、`let x Equatable[int] = C.RED` 仍能通过
- [ ] 待定：堵漏会不会破坏 `let i int = c`（enum → int64 的正常用法）

> 来源：实现 [issue-28](issue-28.md)（enum 自动实现 `Printable`/`Hashable`/`Equatable[E]`）时确认

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

```kl
enum C { RED }

func main() {
    let a Comparable[int] = C.RED   // 通过，且 enum 并没有 Comparable
    let b Equatable[int] = C.RED    // 通过，enum 只有 Equatable[C]
}
```

两者都编译通过，语义上都不成立：

- enum 的 bases 现在固定是 `Printable`、`Hashable`、`Equatable[E]`，**故意没有 `Comparable`**；
- `Equatable[int]` 与 `Equatable[C]` 不是同一个实例。

</details>

<details>
<summary><b>🧠 根因</b></summary>

`_diff_kind_compatible_check()`（`src/parser/parser.c`）在 src 是 `TYPE_ENUM` 时，先按 enum 的 `bases` 匹配，失败后**回退成 int64 再比**：

```c
if (src->kind == TYPE_ENUM) {
    ...
    /* enum -> int is allowed only for int64, its underlying type */
    return type_spec_compatible(dst, int64_type_spec());
}
```

回退不区分目标是 `int64` 还是以 int64 为参数的 trait 实例，所以 `Comparable[int]`、`Equatable[int]` 都被放行。

</details>

<details>
<summary><b>📌 影响范围</b></summary>

- 只影响「把 enum 赋给以 int64 为类型参数的 trait 变量/参数」这一种写法，运行时是 int64 itable，行为与 `int64` 一致，不会崩。
- `let i int = c`、`f(int)` 等正常 int64 回退用法**都依赖同一条路径**。

</details>

---

### 修复方案

<details>
<summary><b>🛠 候选</b></summary>

1. **只在目标不是 trait/实例时回退**：enum → int64 的隐式转换只允许 `dst` 是 `int64`（或 `any` 等既有白名单），目标是 `TYPE_KLASS`（trait 实例）时直接失败，必须先命中 enum 的 `bases`。改动集中在 `_diff_kind_compatible_check` 的回退分支，最小。
2. 维持现状，只在文档里写明「enum 可以当任意 int64 参数的 trait 实例用」。

推荐 1，需先跑全量回归确认没有现有代码依赖 `Comparable[int] = enum` 这类写法。

</details>

<details>
<summary><b>🧪 建议测试</b></summary>

- 报错用例（`test/test-kl/`）：`let a Comparable[int] = C.RED`、`let b Equatable[int] = C.RED` 均报类型不匹配。
- 回归用例（`test/test-run/test_enum_traits.kl` 已有）：`let i int = Color.RED`、`f(int)` 仍通过。

</details>