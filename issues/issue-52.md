# 语言手册（Koala_Language_Manual.md）后续项

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— P1 五项、P2 两项未补齐（P0 与优化项已解决）

> 来源：`docs/Koala_TODO.md` §5

---

### 详细描述

<details>
<summary><b>📌 背景</b></summary>

2026-08-22 整体 review 产出。**P0 已完成**（目录、Union Types、Error Handling、Memory Management），以下为剩余项。

</details>

<details>
<summary><b>✅ 已解决的优化项（第 8–13 项）</b></summary>

- **8. 重复内容合并** —— Resolved 2026-10-28：`class Foo[T]` 与 `add_pair` 只在 Generics 定义一次；Classes and Interfaces 改用 `Counter`，Functions / Operator Overloading 改为引用 Generics。
- **9. 运算符表 `x in seq` 一行** —— Resolved 2026-10-28：`in` 已可用（`OP_CONTAINS` handler 在 `vm_ops.h`，被 `test-ut/test_ut_list.kl` 使用），表行保持不变。
- **10. Operator Overloading 的 Point 示例输出注释失实** —— Resolved 2026-10-28：`Point` 实现了 `Printable.fmt()`（`f.write_fmt(...)`），输出注释成立；手册里过时的 `__str__` / `__hash__` / `__len__` 已改为 `fmt()` / `hash()` / `len()`。
- **11. `guard` 悬空** —— Resolved 2026-10-27：`guard` 被 `let-else` 取代，Booleans 中的词已删除，Null Safety 新增 `let-else` 小节。
- **12. 章节名 "Functions and Closures" 与正文矛盾** —— Resolved 2026-10-28：章节已改名 "Functions"。
- **13. Hello World 的 `koala -c` 与测试体系用的 `koalac` 是否一致** —— Resolved 2026-10-28：两者都对。`koala` 是运行 `koalac` 的驱动器，`koala -c`（`args.h` 的 `compile_only`）编译后即停；测试直接调 `koalac`。Hello World 已说明。

</details>

---

### 修复方案

<details>
<summary><b>📋 P1 —— 完整性补齐</b></summary>

1. 表达式与运算符基础章节：基础运算符参考（算术/比较/逻辑/位/赋值/复合赋值）+ 优先级表。
2. Containers 加深：每类容器常用操作示例；set 字面量 `{1,2,3}`；切片 `nums[1:3]` 示例。
3. Strings 加深：切片 `s[0:5]`、拼接、转义序列。
4. 内建函数速查表：`print` / `enumerate` / `zip` 收拢成表（顶层 `len` / `hash` 已于 2026-10-02 删除，长度 / 哈希只有 `x.len()` / `x.hash()` 方法形态）。
5. 词法基础：字面量进制（hex/oct/bin 在 `parse_lit_int` 有实证）、下划线分隔符待核实。

</details>

<details>
<summary><b>📋 P2 —— 可选</b></summary>

6. 顶层代码说明（模块级语句在 `__init__` 执行、与 `main` 的关系）。
7. Trait 章节补充：`&` 多 trait 基列表、trait 继承（`Comparable : Equatable`）、trait 值作参数。

</details>