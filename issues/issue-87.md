# 泛型规范审查发现的不一致（12 项）

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 5 项不一致 / 缺陷 + 7 项后续方向；本轮修复尝试**已回退**

> 来源：`docs/Koala_TODO.md` §89（2026-11-10，待修）

---

### 详细描述

<details>
<summary><b>📖 规范依据</b></summary>

`Koala_Design_Overview.md` §4（泛型实参不变、只转换值）与手册 Generics 章节。以下是审查中**实测**到的、与规范不一致或有缺陷的行为。

</details>

<details>
<summary><b>📊 不一致与缺陷总表（暂缓处理）</b></summary>

| # | 问题 | 严重程度 |
|---|---|---|
| 1 | `func names[T : N](xs list[T])` 以 `names([L()])` 调用时 VM 崩溃；`dict[str, T]` 参数、构造函数的 `list[T]` 参数同样受影响。 | VM 崩溃 |
| 2 | 类型参数推断或显式指定为 `any` 时编译器中止，报内部错误 "Why goes here?"，如 `first([1, 2.5])`、`pick[any](1, "a")`；对应 §40.4。 | 编译器中止 |
| 3 | `T?` 参数不能从 `opt(1)` 推断出 `T = int`；报错文案把函数称为 "class"。 | 缺陷 |
| 4 | 字面量取公共类型（`[1, nil]` 为 `list[int?]`），泛型函数的实参推断未做同样的合并（`pick(1, nil)` 报错）。 | 待讨论 |
| 5 | 求公共类型时不找共同的 trait：`[Dog(), Cat()]` 推断为 `list[any]` 而不是 `list[Animal]`。 | 待讨论 |

</details>

---

### 修复方案

<details>
<summary><b>1️⃣ VM 崩溃：有约束的 `T` 嵌在容器参数里，推断为类</b></summary>

**（擦除路径，与 §8、§34、§40.1 同源）**

`func names[T : N](xs list[T])` 以 `names([L()])` 调用，或 `dict[str, T]` 参数、`class Holder[T : N]` 的 `__init__(_xs list[T])`，运行时 `_eval_frame` 断言失败。

泛型体按约束的 intf-table 调用 `x.name()`，但容器里是**原始的类值**；`try_to_build_tp_args_cast()` 只转换类型恰为 `T` 的参数。显式写 `names[N](...)` / `Holder[N](...)` 正常。

**方向**：擦除体内对 `T` 值的约束方法调用按值自身的类型查表（规范所说的「由值的类型在运行时分发」），或推断出类时拒绝该调用。

</details>

<details>
<summary><b>2️⃣ 编译器中止：泛型函数的类型实参推断为 `any`</b></summary>

**（§40.4 的复现）**

`first([1, 2.5])`、`lst([1, "a"])`、`pick[any](1, "a")` 在 `mangle_func_name()` → `type_spec_str_func_name()`（`typespec.c`）走到 `UNREACHABLE()`（"Why goes here?"）。

**方向**：类型实参无法命名时应跳过特化路由，调用擦除的泛型函数。

</details>

<details>
<summary><b>3️⃣ `T?` 参数不能从非空实参推断 `T`</b></summary>

`func opt[T](x T?)` 的 `opt(1)` 报 "cannot infer type parameter 'T' for class 'opt'"（`opt[int](1)` 正常）。

应推断 `T = int`；报错文案把函数称为 class。（与 [#81](issue-81.md) 的 `Box[T?]` 同族。）

</details>

<details>
<summary><b>4️⃣ 推断规则两套</b></summary>

字面量取 lub（`[1, nil]` 是 `list[int?]`），泛型函数实参要求完全相同（`pick(1, nil)` 报错，需 `pick[int?](1, nil)`）。当前按规范记录为现状，**是否统一待讨论**。

</details>

<details>
<summary><b>5️⃣ lub 不找公共 trait</b></summary>

`[Dog(), Cat()]` 是 `list[any]`；`let n Animal = Dog()` 后 `[n, Cat()]` 也是 `list[any]`（`Cat : Animal`）。因此 `names([Dog(), Cat()])` 报约束不满足。

**是否让 lub 取公共 trait 待讨论**；目前需给字面量写类型。

</details>

<details>
<summary><b>6️⃣ 多约束不支持</b></summary>

`T : A & B` 报 "can only have one bound"。规范已按现状记录；**是否支持待讨论**。

</details>

<details>
<summary><b>7️⃣ `@specialized` 在类上的映射 / 生成形态未实现</b></summary>

`Koala_Design_Overview.md` §7.4 前半部分按设计描述了 `@specialized(list[int64]) class int64list` 与 `@specialized(Foo[int]) class Foo[T]`，实现中类上的注解被**警告并忽略**（`test-kl/test_specialized_ignored_warnings.kl`）。

§7.4 末尾已注明，阅读时以手册为准。（相关：§40 `@specialized` 泛型函数，尚未迁移。）

</details>