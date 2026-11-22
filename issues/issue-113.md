# 运算符的副作用与显式 `pure` 声明（to design）

← 返回 [问题列表](issues.summary.md)

- [ ] 待设计 —— 方向已倾向（D/Nim 风格 + Julia 两级），6 个设计点未决

> 来源：`docs/Koala_TODO.md` §49（2026-11-02，to design）
>
> 起因是修 §47（`+` 在 `bytes` / `str` 上被当作可交换，已修）时发现的问题。

---

### 详细描述

<details>
<summary><b>📌 起因（49.1）</b></summary>

修 §47 时，`dce` 删掉了一个用户类上未被使用的 `c + d`（其 `__add__` 会自增计数器），而 `lvn` 把两处 `c + d` 合并了（还有两处 list 上的 `a + b` —— 那必须是两个新 list）。

一条出路是声明「运算符必须无副作用」，这样两种改写都合法。**未被采纳**：没有主流语言立这条规矩，而且带副作用的运算符是常见惯用法（C++ 的 `std::cout << x`、管道 / query-builder DSL、计数或记日志的运算符）。

Koala 有 `__shl__`，所以 `log << msg` 一样可能出现；在这种规则下，一个未被使用的 `log << msg` 会**无声消失** —— 与 §47 同类静默错值。

</details>

<details>
<summary><b>🌍 其它语言如何声明纯度（49.2）</b></summary>

**（凭记忆写的，未联网核对；细节可能有出入。）**

运算符重载在 Python（`type(a).__add__(a, b)`，总是调用）、C++（`operator+`）、Rust（`Add::add`）、Swift / Kotlin / C# / Scala（`operator` 函数）里都是**普通调用**：编译器只有在 as-if 规则下（即内联后证明没有可观察行为）才能删掉。唯一「跳过带副作用的用户代码」的标准例外是 C++ 的 copy elision。Haskell 是整门语言纯，不是运算符特例。

显式声明，按**谁做保证**分类：

| 类别 | 语言 | 语法 | 含义 |
|---|---|---|---|
| 编译器检查 | D | `pure int f(int x)` | 无可变全局、只调 `pure` 函数、无 IO；「弱纯」可经指针形参写，「强纯」（不可变形参）才被优化；`debug` 块是逃生口 |
| | Nim | `func f(x: int): int` = `proc` + `{.noSideEffect.}` | 由 effect 系统检查 |
| | Fortran | `PURE FUNCTION`、`ELEMENTAL` | 无 IO、无全局写、函数实参 `INTENT(IN)` |
| | Haskell / Koka | 默认纯；效果在类型里（`IO a`、Koka `console`/`exn`） | 类型系统 |
| 程序员断言（可信，写错即 UB） | GCC / Clang | `__attribute__((pure))` / `((const))` | `pure`：无副作用，可读全局内存；`const`：只依赖实参 |
| | C23 | `[[reproducible]]` / `[[unsequenced]]` | 大致对应 `pure` / `const` |
| | Julia | `Base.@assume_effects :effect_free :consistent :nothrow :terminates_globally ...` | 分得最细：`:consistent` 表示按 `===` identity 结果相同 |
| | Swift（仅 stdlib） | `@_effects(readnone / readonly)` | 内部、带下划线 |
| 仅工具 | C# | `[Pure]` | 文档 / 分析器，编译器不用 |
| | Java | `@Contract(pure = true)`、`@CheckReturnValue` | IDE 警告 |
| | Rust / Kotlin | `#[must_use]` / `@CheckResult` | 反向思路：对未使用的结果告警，而不是删掉调用（Rust 在 1.0 前砍掉了 `pure` 关键字） |

---

### 修复方案

<details>
<summary><b>🎯 对 Koala 要紧的三点（49.3）</b></summary>

1. **「可删」与「可合并」需要不同性质。** `dce` 删除未使用的调用需要：无副作用、不 panic、能终止（删掉一个会在尺寸不匹配时 panic 的 `__add__` 就把错误吞了）。`lvn` / CSE 合并两次调用还要求结果可互换 —— 即 Julia 的 `:consistent`，按 identity 比较 —— 这正是两个 list 上的 `a + b`（两个新的可变 list）**不能**合并的原因。GCC 的 `pure` 只覆盖前半部分；在 C 里这不太要紧，因为结果大多是标量。
2. **默认安全。** 未声明的用户函数或运算符就是普通调用。内建类型对编译器已知，正如今天 §47.2 的白名单。
3. **检查 vs 信任。** D / Nim 风格需要传递性检查（「`pure` 函数只调 `pure` 函数」），所以标准库的 native 函数（`str.__add__` …）必须先标注。GCC / Julia 风格只读一个标志，但写错又是静默错值。

</details>

<details>
<summary><b>➡️ 方向（倾向，未决定，49.4）</b></summary>

- 一个**编译器检查的 `pure` 修饰符**，D / Nim 的精神，并采用 Julia 的**至少两级**划分：「可删（removable，未使用的调用可删）」与「可合并（mergeable，同实参调用可合并；蕴含可删，且结果须可互换）」。
- 对运算符，与 §47.3 的选项 2 一起做：把用户类上的运算符下降为**真正的方法调用**，这样 `dce` / `lvn` / `lsra` 看到的都是调用，由 `__add__` 的 `pure` 级别决定能否删或合并。届时 `dce` / `lsra` 里的 §47.2 白名单就可以去掉。

</details>

<details>
<summary><b>🔍 编译器今天有什么（49.5 盘点，2026-11-02）</b></summary>

- **修饰符 / 注解**：修饰符关键字只有 `pub` 与 `static`。注解是 `@name` / `@name(...)`，独占一行，每个声明至多一个（`koala.y` 的 `prefix` / `annotation`）；`parser.c` 的 `check_func_prefix()` 接受 `intrinsic`、`native`、`specialized`、`test`、`should_panic`、`skip`、`bench`，其余报 "unknown annotation"。trait 方法（`trait_method`）**根本不接受注解**。
- **符号标志**：`include/parser/symbol.h` 的 `SYM_FLAGS_*`（MUTABLE、CONST、PUBLIC、EXT、GENERIC、NATIVE、STATIC），第 7 位及以上空闲。
- **`.klc`**：`include/common/klc.h` 的 `KLC_FLAGS_*`（第 7 位及以上空闲）；`write_klc.c` 的 `write_meta_func()` 只把 PUB / STATIC / METH 写进 meta `KlcFunc`，`read_klc.c` 的 `to_symbol_flags()` 映射回来。注解会被写出但**不会读回 `FuncSymbol`**（只有 test runner 读它们），所以纯度标志必须是一个真正的 meta 标志。
- **native 方法**：`@native` / `@intrinsic` 配空函数体，加载期按名字绑定（`load_module.c`、`kl_get_native`）；**任何地方都没有纯度元数据**。
- **不存在「纯函数」概念**。调用上的 `KLR_INSN_FLAGS_CONST`（IR dump 里的 `[const]`）只表示每个实参都是字面量（`klr/insn.c` 的 `klr_build_call`）。`dce.c` 把 callee 是 `KLR_VALUE_KLASS`（本地类构造器）的 `[const]` 调用当作无副作用 —— **当 `__init__` 有副作用时不成立**，与 §47 同类假设问题。
- **运算符**：检查器能在用户类上找到 `__add__`（`parser_expr.c`）但**不把 `FuncSymbol` 挂到 IR 上**；`emit_ir_binary()` 永远发射 `OP_BINARY_*`；VM 通过 `kl_call_slot(..., SLOT_ADD)` 派发 `OP_NUM_ADD`。今天**没有任何运算符被下降为方法调用**。
- **trait**：`symbol.c` 的 `build_class_intf_table()` 仅按**名字**匹配 trait 方法与实现 —— 不检查签名或标志 —— 所以「trait 方法上的 `pure` 约束每个实现」需要在那里加新检查（且该标志必须能跨 `.klc` 往返，以支持导入的 trait）。

</details>

<details>
<summary><b>❓ 待设计（49.6，6 项）</b></summary>

- **语法**：新修饰符关键字（像 `pub`）还是注解（`@pure`）？注意今天的注解是「每声明一个、独占一行、trait 方法上不允许」。两级的命名。
- **检查规则**：传递性；`pure` 体可以读什么（`self` 的字段？声明为 `let` 的全局？）；分配新对象（对「可删」可以，对「可合并」不行）；panic。
- **native 函数**：C 实现的方法如何声明它的级别。
- **trait**：trait 方法上的 `pure`（如 `Number.__add__`）是否约束每个实现。
- **序列化**：标志如何在 `.klc` 里传递，使导入模块能用它。
- **逃生口**：`pure` 函数内部的调试日志（D 的 `debug` 块）。

</details>

<details>
<summary><b>🔗 关联</b></summary>

- §47（`+` 在 `bytes` / `str` 上被当作可交换，已修）及其 47.2 白名单、47.3 选项 2。
- 运算符在 trait 值上的情况：[#75](issue-75.md)（§63）。
- unary 的上游根因：[#95](issue-95.md)（§94）与 §75。

</details>
</details>