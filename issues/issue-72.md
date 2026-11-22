# 类型参数值当作自动 trait 传递（to support later）

← 返回 [问题列表](issues.summary.md)

- [ ] 未实现 —— 方法调用可用，**转换**为自动 trait 失败

> 来源：`docs/Koala_TODO.md` §35（2026-10-31，to support later）

---

### 详细描述

<details>
<summary><b>✅ 已支持的部分</b></summary>

类型参数 `T` 无需在 bound 里声明，就已获得自动 trait 的方法：`==` / `!=`（`Equatable`）、`t.hash()` / `hash(t)`（`Hashable`，`OP_HASH`）、`t.fmt(f)`（`Printable`，`OP_FMT`）。

测试：`test/test-run/test_hash_type_param.kl`、`test/test-run/test_fmt_type_param.kl`。

</details>

<details>
<summary><b>❌ 不支持：把 T 值转换为这些 trait</b></summary>

例如传给一个 `Printable` 形参：

```kl
func show[T](t T) str {
    let f = Formatter()
    f.write(t)          // write(value Printable)
    return f.result()
}
// error: Incompatible type: expected 'std/builtin.Printable', but got 'show:T'.
```

同样适用于 `Hashable` / `Equatable` 形参、`let p Printable = t`、返回值、字段以及这些类型的容器元素。

</details>

---

### 修复方案

<details>
<summary><b>1️⃣ 类型检查</b></summary>

`_diff_kind_compatible_check()`（`src/parser/parser.c`）对 `TYPE_GENERIC_VAR` 来源只按**声明过的 bound** 接受。应当额外接受 `any` 那类自动 trait（`Printable`、`Hashable`、`Equatable[T]`），对**每个**类型参数都成立。

</details>

<details>
<summary><b>2️⃣ irgen</b></summary>

`_build_obj_intf_upcast()`（`src/parser/irgen.c`）在来源是类型参数时返回 NULL（不做转换），于是裸值（无约束 `T`）或 bound 的接口值（`T: Shape`）会直接到达被调者，而那里的 `value.fmt(self)` 是接口调用 → 槽位错或 VM 断言 `_eval_frame: Assertion '((callable)->tag > (64 + 1))'`。

**思路**：用自动 trait 的 itable 索引发射 `make_intf`。`OP_MAKE_INTF` 从值的 `kl_typeof()`（对象自身类型，对接口值也一样）取 itable，因此**两种情况都能用** —— 前提是自动 trait 在每个类型里 itable 索引相同（class：`[0] any [1] Printable [2] Hashable [3] Equatable`）。

**依赖校验**：靠它之前需先验证内建类型（`int`、`float`、`str`、`list` …）；否则需要新增一条按 identity 查 trait 的指令。

</details>

<details>
<summary><b>3️⃣ 测试</b></summary>

覆盖：无约束 `T`、`T: Shape`、`T` 分别以 `int` / `str` / 自带与不带 `fmt`/`hash`/`__eq__` 的 class 实例化；`f.write(t)`、`let h Hashable = t`、`list[Printable]` 元素。

</details>