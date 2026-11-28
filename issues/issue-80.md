# 80. union 成员里可以含类型参数

← 返回 [问题列表](issues.summary.md)

- [x] 方案改为**支持**（2026-11-28）：union 成员可以是用了类型参数的泛型 class，如 `G[T] | float64`、`list[T] | bool`；不在声明处报错
- [x] 类方法和 trait 方法：实例化时替换成员里的 `T`
- [x] 泛型函数：从实参所属的成员推导 `T`
- [x] 测试：`test/test-run/test_union_generic_member.kl`；删除原来的预期报错测试 `test/test-kl/test_union_member_type_param.kl`

> 来源：`docs/Koala_TODO.md` §76（2026-11-07，to fix）

---

### 详细描述

<details>
<summary><b>🔁 复现</b></summary>

```kl
trait Shapes[T] { func uni(x list[T] | bool) int }    // no error here

class S : Shapes[str] {
    pub func uni(x list[str] | bool) int { return 2 }
}

func main() {
    let s Shapes[str] = S()
    s.uni(["x"])    // error: expected 'bool | list[Shapes:T]', but got 'list<u>'
}
```

</details>

<details>
<summary><b>🧠 根因</b></summary>

union 是**具体类型**，所以 `T | bool` 会被报成 `Union subtype must be a class`。

但成员**内部**的类型参数没被报出来：`src/parser/parser.c` 的 `check_union_subtypes` 只看成员本身，而 `list[T]` 是某个 class 的实例。

</details>

---

### 修复方案

<details>
<summary><b>🚫 曾否决</b></summary>

原方案是在**声明处**报错（成员里含类型参数就拒绝）。2026-11-28 改为支持：Tensor 需要 `__add__(rhs Tensor[T] | float64)` 这类签名，而成员里的 `T` 只是没有被替换，不是语义问题。

成员本身是类型参数（`T | int`）仍然拒绝：`T` 可以是任意类型，union 无法静态检查。

</details>

<details>
<summary><b>🛠 已实施</b></summary>

1. `src/parser/parser_expr.c` 的 `instance_type_spec`：新增 `TYPE_UNION` 分支，逐个成员替换类（或 trait）的类型参数，写法与 `TYPE_OPTIONAL` / `TYPE_PROTO` 分支一致。类方法和 trait 方法都走这里。
2. `src/parser/parser_generic.c`：新增 `infer_tp_from_union`，`infer_tp_from_type` 和 `infer_tp_from_call` 遇到 union 参数时调用它。规则：找到和实参同一个泛型 class 的成员，只从它推导；实参是具体成员（如 `int`）时什么也不推导，留给其他实参、目标类型或显式类型参数。
3. `test/test-ut/test_trait_impl_match.kl` 里 `Shapes` 的 `uni` 保留，它现在是正向用例。

</details>

<details>
<summary><b>🧪 验证</b></summary>

- `test/test-run/test_union_generic_member.kl`：类方法 `G[T] | float64`、trait 方法 `list[T] | bool`（含泛型 class 实现）、泛型函数推导、成员里的 optional 实参 `G[int?] | str`；JIT 与 `--no-jit` 均通过。
- 报错类用例（成员实参不匹配、推导不出 `T`）只手工验证过，未落地成测试：union 目前只用于函数入参，锁定错误文案意义不大。
- 全量回归 656 个通过，`bootstrap.sh` 的 std 测试通过。

</details>