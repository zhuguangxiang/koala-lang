# 无法创建空的元组列表 `list[(int, int)]`

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 两条路径都失败；`koala/re` 用 `[(a, b)]` 起步后 append 绕过

> 来源：`docs/Koala_TODO.md` §44（2026-11-01，to fix）

---

### 详细描述

<details>
<summary><b>📌 起因</b></summary>

给 `re.Match` 加 `regs` 字段（`list[(int, int)]`）时发现：两条「起一个空元组列表」的路径都失败；**非空字面量可用**，所以 `koala/re` 从 `[(a, b)]` 开始再 append。

</details>

<details>
<summary><b>1️⃣ `list[(int, int)]()` 让编译器 abort（期望 1）</b></summary>

```kl
func main() {
    let xs = list[(int, int)]()
    xs.append((1, 2))
    print(xs.len())
}
```

报 `koalac: parser_expr.c:2622: parse_index_new_type: Assertion 'arg->ts->kind == TYPE_TYPE || arg->ts->kind == TYPE_GENERIC_VAR' failed.`

元组类型两者都不是；而 `list[str]()` 正常。

</details>

<details>
<summary><b>2️⃣ `[]` 不从声明类型取元素类型，且报错是裸露的内部信息（期望 1）</b></summary>

```kl
func main() {
    var xs list[(int, int)] = []
    xs.append((1, 2))
    print(xs.len())
}
```

编译器打印 `lhs: std/builtin.list<Lstd/builtin.tuple<jj>;> =/= rhs: std/builtin.list<o>` 后失败，**没有文件名与行号**：空字面量保持 `list[any]`，没有取声明的元素类型。

</details>

<details>
<summary><b>✅ 可用的变通</b></summary>

`let xs = [(1, 2)]` 后接 `xs.append((3, 4))` 可编译，打印 `2`。

</details>

---

### 修复方案

<details>
<summary><b>🛠 待补充</b></summary>

**原记录未给出方案**，只给出了定位：

- 断言点：`parser_expr.c:2622` 的 `parse_index_new_type`，只接受 `TYPE_TYPE` / `TYPE_GENERIC_VAR`，需放开元组类型。
- 类型推断缺口：`var xs list[(int, int)] = []` 的空字面量没有从声明类型取元素类型（现状落到 `list[any]`）。
- 第 2 条的诊断缺 file:line，需一并修。

</details>