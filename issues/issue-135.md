# 135. `Equatable[T]` 类型的变量不能参与 `==` / `!=`

← 返回 [问题列表](issues.summary.md)

- [x] 已定位：`Equatable[T].__eq__(other T)` 的实参是 `T`；`e == e` 的 rhs 静态类型是 `Equatable[P]`，而 `Equatable[P]` 不是 `P` 的子类型，拒绝是**对的**（不是 issue 初判的「trait 参数展开不匹配 / intern 指针不等」）
- [x] 已定语义：trait 值上的 `==` / `!=` 走 itable 的动态分派（`__eq__` / `__ne__` 槽），rhs 必须是具体 `T`，与 Rust「operator trait 不是 dyn-compatible」一致
- [x] 已修复：报错追加期望/实际类型 → `argument type mismatch ... expected 'P', but got 'Equatable[P]'`
- [x] 正例已可用：`e == P()` / `e != P()` / `e == p` 以及 `Q : Equatable[P]` 的动态分派
- [ ] 待确认：enum 分支（`Equatable[C]` 上 `e == e`）是否同因，按要求暂不看

> 来源：实现 [issue-28](issue-28.md) 时写 `let e Equatable[C] = C.RED` 的用例发现，class 同样复现

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

```kl
class P {}

enum C { A }

func main() {
    let e1 Equatable[P] = P()
    print(e1 == e1)          // error: argument type mismatch for operator '==' of type 'Equatable[P]'

    let e2 Equatable[C] = C.A
    print(e2 == e2)          // error: argument type mismatch for operator '==' of type 'Equatable[C]'
}
```

连两边静态类型完全相同（`e1 == e1`）也报错，说明不是 enum 的问题，trait 实例类型上的 `==` 整体不可用。

</details>

<details>
<summary><b>🧠 根因初判（已排除，见下一节）</b></summary>

`==` 被解析成 `Equatable[T]` 实例的 `__eq__`/`__ne__` 调用，实参类型检查（`src/parser/parser_expr.c` 里生成 `argument type mismatch for operator ...` 的两处）要求实参正好是 `Equatable[T]` 本身，而 `e1` 的静态类型没有按 trait 参数展开匹配，于是两边都判不匹配。

具体是「实参类型没做 `Equatable[P]` 内部的 `P` 对齐」还是「接口变量不接受值实参」，需要再定位。

</details>

<details>
<summary><b>🧠 根因定位（已排除初判）</b></summary>

`libs/std/builtin/any.kl:28` 声明：

```kl
pub trait Equatable[T] {
    func __eq__(other T) bool
    func __ne__(other T) bool
}
```

`Equatable[T]` 的语义是「自身能和 `T` 比较」，不是「能和自己比较」。`src/parser/parser_expr.c` 的 `parse_binary`：

- `lhs_op_ts->sym_id` 指向的 symbol 是 `SYM_INSTANCE`，走 `get_instance_method(...)` 得到 `Equatable[P]` 的 `__eq__(other P)`（parser_expr.c:4707 附近）；
- 实参检查取 `arg_info->ts`（实例化后是 `P`）与 rhs 的 `rhs_op_ts`（`Equatable[P]`）做 `type_spec_compatible`，`P` 与 `Equatable[P]` 不兼容 → 报错。

所以两边静态类型相同（`e1`/`e1`）也照样报错，因为被比较的对象不是 lhs 类型，而是 lhs 的 trait 参数。issue 初判的「两个 `Equatable[P]` 是不同 intern 实例，指针不相等」不成立，类型系统从未拿 `Equatable[P]` 和 `Equatable[P]` 相比。

同一原因解释了 `test/test-kl/test_operator_ban.kl` 里早已记录的 `a + b`（`Arithmetic[int]`）被拒——那是设计上的选择，不是缺陷。

</details>

<details>
<summary><b>✅ 实测（koala）</b></summary>

| 用例 | 结果 |
| --- | --- |
| `let e1 Equatable[P] = P(); e1 == e1` | ❌ `expected 'P', but got 'Equatable[P]'`（修复前只有一层 `argument type mismatch`） |
| `e1 == P()` / `e1 != P()` | ✅ `false` / `true`（class 内置 `Equatable[P]` 按引用比较） |
| `let p = P(); let e2 Equatable[P] = p; e2 == p` | ✅ `true` |
| `class Q : Equatable[P] {...}; let e Equatable[P] = Q(); e == P()` | ✅ `true`，动态分派到 `Q.__eq__` |

结论：接口变量上的 `==` 已经走 `__eq__` 槽动态分派，不是按引用比较（issue 里「未确认」项可关闭）。拒绝 `e1 == e1` 是类型安全的必要条件：`Equatable[P]` 背后可以是任意 `Q`，而 `Q.__eq__` 只接受 `P`。

</details>

<details>
<summary><b>📌 影响范围</b></summary>

- 只影响静态类型是 trait **实例**（`Equatable[X]`）的变量/参数；`c == c`（class、enum 值）不受影响。
- 泛型 `[T : Equatable]` 内部的 `a == b` 正常。
- 绕开写法：比较前先窄化回具体类型，或直接比较原值。

</details>

---

### 修复方案

<details>
<summary><b>🛠 已实施</b></summary>

不改语义（现状是正确的），只把诊断说清楚：`src/parser/parser_expr.c` 的 `parse_binary` 两处 `argument type mismatch` 现在追加期望/实际类型。

- 实参检查（trait 实例 / class / enum 的 `__op__` 分支）：
  `argument type mismatch for operator '==' of type 'Equatable[P]'. expected 'P', but got 'Equatable[P]'.`
- 自动 trait（`[T : Equatable]` 的 `is_auto_trait_func` 分支）同样追加 `expected '<lhs>', but got '<rhs>'.`
- 类型名用 `type_spec_print`，保留包限定（`std/builtin.Equatable[P]`），避免跨包同名歧义。
- 旧消息作为子串保留，`test/test-kl/` 里 25 处既有 `// CHECK` 无需改动。

</details>

<details>
<summary><b>🧪 测试</b></summary>

- 新增 `test/test-run/test_trait_equatable_operand.kl`：`Equatable[P]` 变量上的 `e == p` / `e != p`，以及 `Q : Equatable[P]` 走 itable 动态分派。
- 新增 `test/test-kl/test_trait_instance_operand.kl`：`e == e` / `e != e` 必须报带 `expected ... but got ...` 的新消息。
- 既有 `test/test-kl/test_operator_ban.kl`、`test_number_errors.kl`、`test_func.kl`、`test_tuple.kl`、`test_infer_new.kl` 保持不变（子串匹配仍通过）。

</details>

<details>
<summary><b>📌 遗留</b></summary>

- enum 分支（`let e Equatable[C] = C.A; e == e`）按用户要求暂不跟进；预期与 class 完全同因，属于同一处实参检查。
- 若将来想支持「trait 值之间比较」，需要引入 `Equatable[Self]` 之类的新语义或反射/`any` 装箱，属于语言设计变更，不在本 issue 范围。

</details>