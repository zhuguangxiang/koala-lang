# `is` / `as` 运算符语义评估与遗留项（6 项）

← 返回 [问题列表](issues.summary.md)

- [ ] 部分完成 —— 已定规则均已实现；6 项遗留待拍板 / 待施工

> 来源：`docs/Koala_TODO.md` §29（2026-10-26 评估）

---

### 详细描述

<details>
<summary><b>📌 定位</b></summary>

`as` 是 **downcast**（`any` / trait / union → 具体 class）；`is` 是同一集合上的类型谓词。class 一律 final、没有 class 继承，只有 trait 构成子类型关系。

实现：`src/parser/parser_expr.c`（`check_is_src_type` / `check_as_src_type` / `validate_cast_target` / `validate_union_target` / `resolve_cast_check` / `parse_is` / `parse_as`），IR 在 `src/parser/irgen.c` 的 `emit_ir_is` / `emit_ir_as`。

测试：`test/test-kl/test_is_as_target_errors.kl`、`test_is_as_warnings.kl`、`test/test-ut/test_is_as_*.kl`。

</details>

<details>
<summary><b>✅ 当前已定规则（已实现）</b></summary>

| 项 | `is` | `as` |
|---|---|---|
| 源类型 | 不能是 `T?`；不能是 `...` / 包 / 未解析等无效种类；`any` / trait / union / class / 内建值类型可以 | 同 `is`，另外**目前**一律拒绝内建值类型（见遗留 ①） |
| 目标类型 | 必须是 class；不能是 trait、`T?`、union、`NilType` | 同 `is` |
| 检查顺序 | 先查源（`T?` 等），再解析目标并校验，再校验 union 成员 | 同 `is`；之后同一泛型 class 的不同类型实参（`list[int]` → `list[str]`）**报错** |
| 同一泛型的不同实参（源静态已知） | 不报错：编译期折叠为 `false` 并给 `always false` 警告 | 报错：`cannot cast ... with 'as': type arguments are different.` |
| 结果 | `bool`；编译期可确定时为常量并给 `always true/false` 警告，否则运行时检查 | 恒等为 `T`（警告 `always true`，IR 直通源值）；其余为 `T?` |
| `NilType` 目标 | `target type of 'is' operator cannot be NilType.` | `target type of 'as' operator cannot be NilType.` |

`T?` 作源一律拒绝：判空用 `== nil` / `!`，判类型用 `is` / `as`，两者不混用。

**评估为合理的部分**：`as` 只有 `any` / trait / union 作源才是真正的 downcast；恒等转换给警告而不是报错，与 Kotlin / Swift 一致；目标只能是 class，`as` 结果在可能失败时是 `T?`，可与 `!`、`while let` 直接配合；恒等 `as` 的 IR 不生成任何指令（`ALWAYS_TRUE` 直接复用 `operand`），无运行时开销；作为谓词，`is` 对无关类型返回 `false` 是合法答案，用警告而不是报错合理。

</details>

---

### 修复方案

<details>
<summary><b>1️⃣ `as` 对内建值类型的处理（已定方案，未实现）</b></summary>

同类型（`i as int`）应允许并给 `always true` 警告，IR 直通；不同类型（含不同位宽，如 `int64` → `int8`、`int8` → `int16`）一律报错。

落点：`check_as_src_type` 不再整体拒绝内建值类型；`parse_as` 在目标校验后对「内建源 ≠ 目标」报错；`resolve_cast_check` 对「内建源 == 目标」直接返回 `ALWAYS_TRUE`。`sym_id` 唯一，`emit_ir_as` 的断言成立。

</details>

<details>
<summary><b>2️⃣ 「无关类型」的一致性（暂缓）</b></summary>

不同内建类型要报错，而不同的用户自定义 class（`Dog` → `Cat`）、trait 源且目标 class 未实现该 trait，目前仍只是 `always false` **警告**；union 非成员却是**报错**。

**倾向**：`as` 目标不可能时一律报错，`is` 一律警告。需同步调整 `test_is_as_warnings.kl` 等测试。

</details>

<details>
<summary><b>3️⃣ 泛型实参擦除（仅源为 `any` / trait / union 时）—— 实际类型安全漏洞</b></summary>

源类型静态已知时（`x list[int]` 的 `x is list[str]`）编译期即折叠为 `false`，`as` 直接报错，均无问题。

但源为 `any` 时编译期无法判断，运行时只比较 `list`、**不比较类型实参**：`v any` 的 `v is list[str]` 对 `list[int]` 的值为 `true`（`test_is_as_no_warning.kl` 的 `is_ls` 已有 `TODO: vm erase subtype?`），`v as list[str]` 也会成功，得到类型实参错误的 `list[str]`。同一个值的同一判断，结果取决于静态类型是否为 `any`。

**根因（2026-10-26 实测）**：运行时对象不带泛型类型实参，`is` / `as` 的 `RUNTIME_CHECK` 指令只携带 `target_path` / `target_name`（`list`），VM 只能判断「是不是 `list`」。实测 `is_ls([1,2,3])` → `true`、`as_ls([1,2,3]) == nil` → `false`。

**方向**：VM 运行时保留并比较泛型实参，或对泛型目标的 `any` 源 `is` / `as` 加限制，**需单独评估**。

</details>

<details>
<summary><b>4️⃣ 目标只能是 class</b></summary>

无法用 `is` / `as` 判断或向下转到 trait（含 trait 继承链上的向下转型），表达力受限。

</details>

<details>
<summary><b>5️⃣ 诊断不够具体</b></summary>

- `T?`、`...` 等作源都报 `src of 'is'/'as' operator is not a valid type.`，看不出原因。
- 警告文案里类型名是**内部签名**（如 `Ltest_is_as_warnings.Dog;`、泛型显示为 `list<j>`），不是源码写法。
- `as` 对同一泛型不同实参的报错（`cannot cast 'std/builtin.list<j>' to 'std/builtin.list<u>' with 'as': ...`）同样显示内部签名，应显示 `list[int]` / `list[str]`。

</details>

<details>
<summary><b>6️⃣ 内建类型的 `TypeSpec` 位置共用</b></summary>

`ts->loc` 在内建类型间共享，曾导致报错位置为 `0:0`；现用 `as->loc` / `is->loc`，其他引用 `ts->loc` 的路径需留意。

</details>

<details>
<summary><b>🧪 测试覆盖现状与缺口</b></summary>

`is` 与 `as` 在以下场景成对覆盖（`test_is_as_target_errors.kl`）：trait 目标、带参数 trait 目标、`T?` 目标、union 非成员、`Dog?` / `int?` / `(Dog | Cat)?` 作源、`int?` 目标、`NilType` 目标、`...` 作源；`as` 额外覆盖：内建值类型作源、同一泛型不同实参。

警告（`test_is_as_warnings.kl`）：`is` 覆盖 class / 泛型的 always true/false；`as` 覆盖 class 的 always true/false 与泛型的 always true（泛型 `always false` 已改为报错，在 `test_is_as_target_errors.kl` 的 `as_generic_different_args`）。

**缺口**：`as` 恒等内建类型（待第 1 项实现后补）、`is` 与 `as` 对 trait 源且目标未实现该 trait 的用例。

</details>