# `enum` 作为 final class 的语法糖（planned）

← 返回 [问题列表](issues.summary.md)

- [ ] 未实现 —— P0 全 10 项未做；P1 仅 `switch` 语句（phase 1）已完成

> 来源：`docs/Koala_TODO.md` §78（2026-11-07，planned）
>
> `switch` 的完整路线图见 [#112](issue-112.md)。

---

### 详细描述

<details>
<summary><b>📌 定位</b></summary>

enum 是**不带数据**的具名常量封闭集合。case 带数据属于 `sealed trait` 与 class（见 [#71](issue-71.md)）。两者只共享「case 集合封闭」这一检查，`switch` 用它。

```kl
pub enum Color { RED, GREEN, BLUE = 10, ALPHA }

let c = Color.BLUE
print(c)                  // BLUE
print(c.value)            // 10
print(c == Color.BLUE)    // true

let e Color = 10          // error: no implicit conversion
let i int = c             // error: write c.value
let b = c > Color.RED     // error: write c.value > Color.RED.value
Color(0, "RED")           // error: cannot be constructed
Color.RED = Color.BLUE    // error: a member cannot be assigned
```

</details>

<details>
<summary><b>⚙️ 编译器生成的类（伪代码）</b></summary>

```kl
pub class Color : Equatable[Color] {
    pub let value int64
    let name str

    func __init__(v int64, n str) {
        value = v
        name = n
    }

    pub func fmt(f Formatter) { f.write_str(name) }
}

// hidden module-level singletons, created when the module is initialized
let Color.RED = Color(0, "RED")
```

</details>

<details>
<summary><b>✅ 已决定</b></summary>

- 分支 `new_enum`（`47539856`）把 enum 做成带 `TYPE_ENUM` 与 `SYM_ENUM` 的 sealed `int64`。它**落后 34 个提交且未合并**；只手工移植它的文法与成员检查。用 class 可以免费得到 klc 导出、泛型、容器与自动 trait。
- 与 `int` 之间**无隐式转换**，与移除隐式数值提升一致。
- 第一版**没有顺序**。Python 的 `Enum` 也没有，而且按值排序与按声明排序在 `enum E { A = 10, B = 1 }` 上不一样。以后可以加而不破坏代码。

---

### 修复方案

<details>
<summary><b>P0：第一版（10 项）</b></summary>

- ☐ 0. **先核查**：模块如何初始化、跨包的初始化顺序、`static` 是否存在。
- ☐ 1. 文法：`enum` 关键字、只能在顶层、可选 `pub`。错误按 `test/test-kl/test_enum_syntax_errors.kl` 的要求。
- ☐ 2. 生成那个 final class。禁止构造它、禁止给它基类。
- ☐ 3. 成员检查（来自 `new_enum`）：重名、重值、自动递增、溢出。
- ☐ 4. **单例**：在 `parse_dot` 里解析 `Color.RED`，在模块初始化时创建它们，拒绝对成员赋值。**这一项风险最大**，第 0 项就是为早发现问题而设。
- ☐ 5. 方法：`fmt` 打印成员名。`__eq__`、`__ne__`、`hash` 保留默认。
- ☐ 6. `c.value`，以及每种隐式转换的报错。
- ☐ 7. `list[Color]`、`dict[Color, int]`、`Color?`、转成 `any`、泛型 `T`。
- ☐ 8. 测试：改写 `new_enum` 的测试，去掉 `test_enum_syntax*.kl` 的 `SKIP`，第 1–7 项的正确代码与错误都要覆盖。
- ☐ 9. 文档：设计概览、语言手册、§55（[#71](issue-71.md)）。

</details>

<details>
<summary><b>P1：紧随其后（3 项）</b></summary>

- ☐ 10. **跨包**：klc 携带成员表，`pkg.Color.RED` 可用，测试放 `test_pkg`。**在此之前 `pub enum` 无法在别的包里使用。**
- ☐ 11. `Color.from(v)` 返回 `Color?`，非法值给 `nil`。形态取决于第 0 项。
- ☐ 12. `c.name()` 返回成员名。
- ☑ 13. **`switch` 语句（phase 1 已完成，2026-11-19）** —— 见下。

</details>

<details>
<summary><b>☑ 第 13 项详情：`switch` 语句 phase 1</b></summary>

文法（`koala.y`）、AST（`CasePattern` / `CaseClause` / `SwitchStmt`）、语义（`parse_switch`）与 IR（`emit_ir_switch`，一条比较链，**尚无多路终结符**）均已实现并测试（`test/test-run/test_switch_basic.kl`、`test_switch_patterns.kl`、`test/test-kl/test_switch_errors.kl`、`test_switch_syntax_errors.kl`）。

- **主体类型**：int / uint / enum / bool / str / float / class / trait / any / union / 泛型引用。
- **模式**：值（字面量或 const）、`is T`（把变量主体窄化到 `T`）、`in container`、`_`（默认，必须在最后且单独出现）。
- enum / bool / union 主体必须**穷尽**或带 `_`；每个 case 都终止的穷尽 `switch` 自身也是终止的。
- 主体**只求值一次**。
- **optional 主体暂被拒绝**（`switch subject cannot be optional type`），见下方延后设计。

phase 2（多路 IR switch）与第 19 项（`switch` 表达式）仍未完成；详细前瞻计划见 [#112](issue-112.md)。

</details>

<details>
<summary><b>P2：以后再决定（7 项）</b></summary>

- ☐ 14. 列举成员，如 `Color.values()`，及其返回类型。
- ☐ 15. enum 里的方法：语法，以及能否实现 trait。
- ☐ 16. 顺序：用户显式 `Comparable`，还是自动一种方式。
- ☐ 17. 在函数或类里声明 enum：其单例的生命周期与命名。
- ☐ 18. `sealed trait`（[#71](issue-71.md)），与 `switch` 共享封闭集合检查。
- ☐ 19. `switch` 作为表达式。
- ☐ 20. 位标志：是否需要一种 `Flags` 支持。

</details>

<details>
<summary><b>⏸ 延后：optional 的 `switch` 主体（2026-11-19 讨论，未实现）</b></summary>

放开禁令将跟随 Swift：

- `case nil` 匹配 nil；值 / `is T` / `in` 模式是针对 payload 类型 `T` 检查的，**永远不匹配 nil**；`case _` 也覆盖 nil；`case nil, 0:` 可以混写。
- 穷尽性于是要求 nil 加上 `T` 的每个值。
- 普通变量主体在每个 case 里窄化（nil case → 已知为 nil，值 / `is` case → `T`）。

**IR**：先测 nil（`cmpeq subj, none`），把 nil 路由到第一个 `case nil`（或 `_`），非 nil 路径解包进一个 `T` local（与 let-else 一样），再对它跑现有的比较链；nil 模式在链里跳过。

**触点**：`parse_switch`（去掉禁令、按 `st->opt.src` 检查模式、跟踪 `has_nil`、重复 `nil` 报错、用 `stbl_add_shadow_var` 窄化）、`emit_ir_switch`（nil 分流 + 解包）、`switch_check_value`。

`switch let v = expr { ... } else { ... }` 曾被考虑并放弃：带窄化的 optional 主体覆盖了同样的场景，且不需要新语法。

</details>

<details>
<summary><b>📌 第 7 项的注意点</b></summary>

`T` 绑定为 `any` 的泛型函数会在 `typespec.c:1592` 让编译器 abort，有无 enum 都一样，例如 `func gen[T](x list[T])` 的 `gen([1, "a"])`。**尚未修复**（与 [#87](issue-87.md) 第 2 项同源）。

</details>
</details>