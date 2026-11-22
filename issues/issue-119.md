# Tuple 字面量与期望类型：失败用例（to discuss）

← 返回 [问题列表](issues.summary.md)

- [ ] 待讨论 —— 第 1 项已解决；第 2–11 项仍未解（2026-10-28 复核）

> 来源：`docs/Koala_TODO.md` §32（2026-10-28，to discuss）

---

### 详细描述

<details>
<summary><b>📌 现状与已定决策</b></summary>

**Status**：`parse_tuple()` 把期望 tuple 类型的元素类型作为每个元素的 `expected`，并且当元素是 `nil`、或元素恰为 `T` 而期望 `T?` 时，把 `T?` 记进 tuple 类型。所以 `return (nil, 1)` 对 `(int?, int)`、`let t (int8, int?) = (1, nil)`、以及 `x int8` 下的 `(x, y) = (7, nil)` 都能工作。

**Open point**：只把期望类型传给**需要它的元素**（字面量），这样元素永远不会拿左侧的类型来代替解析自己的类型。

**Decided (2026-10-28)**：保持 `parse_tuple()` 当前行为；类型无法解析的元素仍然报自己的错，**永远不会退回到左侧类型**。

下面的用例仍然失败，第 1 项除外（已解决）。记录下来待以后讨论；其余尚未定案。2026-10-28 在第 1 项修完后复核：第 2–7 项仍以所示的错误失败。

</details>

<details>
<summary><b>📦 32.1 与 tuple 相关（1–4）</b></summary>

1. **调用实参里的 `nil`。**
   ```koala
   func take(p (int?, int)) {}
   take((nil, 1))
   // Incompatible type: expected 'std/builtin.tuple<j?j>', but got 'std/builtin.tuple<_?j>'.
   ```
   原因：`nil` 只有 nil 类型 `_?`，没有 `src`，所以 tuple 是 `tuple<_?j>`。`return` 能通过是因为 `parse_tuple()` 会重写 tuple 类型；调用实参没走到那一步（**原因尚未确认**）。

   **Resolved (2026-10-28)**：`type_spec_compatible()` 逐元素比较两个 tuple 实例（`compatible_tuple()`，从 `compatible_klass()` 调用）：元素相同即接受，`nil`（`_?`）可对任何 `T?`，嵌套 tuple 同样递归检查。其它元素类型仍然不变（invariant）。**已知缺口**：值保留自己的运行期类型（`tuple<_?j>`），所以对这种 tuple 做 `is` / `==` 可能看不到声明类型。

2. **用类值填 trait 元素。**
   ```koala
   func one() (Fly, int) {
       return (Bird(), 1)    // return type mismatch in function 'one'
   }
   ```
   原因：`tuple<Bird, int64>` 与 `tuple<Fly, int64>` 不匹配，泛型是**不变**的。trait 值还有另一种表示，所以即使有了协变，irgen 也必须逐个元素上转（`_build_obj_intf_upcast()`，就像 `build_unpack_vars()` 那样）。

3. **`any` 元素。**
   ```koala
   let t (any, int) = (1, 2)
   // lhs: std/builtin.tuple<oj> =/= rhs: std/builtin.tuple<jj>
   ```
   原因：泛型不变，`tuple<jj>` 不是 `tuple<oj>`；而 `var a any = 1` 是可以的。**开放问题**：`tuple` 是否是泛型的特例，可以逐元素兼容？这也能顺便解决第 1 项。

4. **list 字面量里的 tuple。**
   ```koala
   let l list[(int?, int)] = [(nil, 1), (2, 3)]
   // lhs: list<tuple<j?j>> =/= rhs: list<o>
   ```
   原因：泛型不变；且 list 字面量不把期望元素类型传给元素，于是两个 tuple 是 `tuple<_?j>` 与 `tuple<jj>`，它们的 LUB 变成 `list<any>`。**需要 `parse_list()` 传递 `expected`。**

</details>

<details>
<summary><b>🔢 32.2 与 tuple 无关（单个值同样失败，5–7）</b></summary>

5. **`int8` 的二元表达式。**
   ```koala
   let t (int8, int) = (1 + 2, 3)    // var x int8 = 1 + 2 fails too
   ```
   原因：字面量默认是 `int64`；`expected` 没有被传给 `1 + 2` 的操作数，所以结果是 `int64`。**可能的改进**：二元表达式把 `expected` 传给它的字面量操作数。

6. **`int` 当 `float`。**
   ```koala
   let t (float, int) = (1, 2)       // var f float = 1 fails too
   ```
   原因：`int64` 不会隐式转成 `float`，需要 `float(1)`。**这是语言规则，不是 bug。**

7. **空的 list 字面量。**
   ```koala
   func g() (list[int], int) {
       return ([], 1)                // 'return []' for 'list[int]' fails too
   }
   ```
   原因：`[]` 是 `list<any>`（`list[o]`），泛型不变，不匹配 `list[int]`。**可能的改进**：空 `[]` 从 `expected` 取元素类型。

</details>

<details>
<summary><b>🏷 32.3 tuple 的运行期类型（8）</b></summary>

8. **tuple 的运行期类型可能与声明类型不同（`is` / `==`）。**
   ```koala
   func take(p (int?, int)) {
       print(p is (int?, int))   // the value was created as tuple<_?j>
   }
   take((nil, 1))
   ```
   原因：自第 1 项起，`type_spec_compatible()` 接受 `tuple<_?j>` 顶替 `tuple<j?j>`，但值保留创建时的实例（`tuple<_?j>`）。静态类型说 `(int?, int)`，运行期类型说 `(nil, int)`。于是运行期类型检查（`is`、`as`）或比较两个类型的运算符（`(nil, 1)` 值与 `(int?, int)` 值之间的 `==`）可能给出错误结果或报错。`return (nil, 1)` 不受影响：`parse_tuple()` 已经把它构造成 `tuple<j?j>`。

   **待查**：今天 `is` / `as` / `==` 在这种 tuple 上到底怎样。**可能的修法**：值被传递（实参、赋值）时按声明类型构造，或在运行期也逐元素比较 tuple 实例。

   **Checked (2026-10-28)：尚未观察到。**
   - 对声明类型做 `p is (int?, int)` / `p as (int?, int)` 在编译期就定案（"'is' check is always true"），不看运行期类型。
   - 经 `any` 的运行期检查（`take((nil, 1))` 里 `let a any = p`，然后 `a is (int?, int)`）打印 `true`。
   - `==` 无法暴露：两个 tuple 之间的 `==` **根本编不过**（第 9 项）。

   保持开着，直到 tuple 上的 `==` 可用、且 tuple 的运行期 `is` 被搞清。

</details>

<details>
<summary><b>🐞 32.4 检查上述用例时发现（9–11）</b></summary>

9. **两个 tuple 之间的 `==` 编不过。**
   ```koala
   let a (int, int) = (1, 2)
   let b (int, int) = (1, 2)
   print(a == b)   // argument type mismatch for operator '==' of type 'tuple<jj>'.
   ```
   与 optional 元素无关：普通 `(int, int)` 一样失败。

10. **`let` 局部持有的 `any` 值做运行期 `is` 会崩 VM。**
    ```koala
    let b any = 1
    print(b is int)
    // koala: include/runtime/vm_ops.h:2287: _eval_frame: Assertion `(rs) < max_regs' failed.
    ```
    顶层与函数内、`int` 或 tuple 值都一样。形参赋给 `any` 局部（`let a any = p`）是好的。

11. **`if v != nil { ... }` 里的 `v = nil` 在 isel 失败。**
    ```koala
    func narrowed() {
        var v int? = 1
        if v != nil {
            v = nil
        }
        print(v)
    }
    // isel.c:317: Why goes here?
    ```
    二元运算的两个操作数都作为常量到达 `isel_lower_binary()`，而它的常量折叠分支只有 `UNREACHABLE()`（`src/parser/backend/isel.c:316`）。

</details>

<details>
<summary><b>🔗 关联</b></summary>

- [#120](issue-120.md)（§33）：同日发现的其他 bug，其中第 4 项（带泛型实参的 tuple 仍按不变比较）是本节第 2/3 项的延伸。
- [#78](issue-78.md)：同一个 `isel_lower_binary()` 的 `c1 && c2`（两操作数都是常量）分支 `UNREACHABLE()` 路径（第 11 项）。
- [#106](issue-106.md)（§16）：`Heap[T : Comparable]` 一类泛型类 bound 崩溃，泛型不变性的邻近问题。

---

### 修复方案

<details>
<summary><b>❓ 待讨论方向：需要拍板的四个问题</b></summary>

1. **`tuple` 是否作为泛型的特例，逐元素兼容？**（第 3 项）—— 若成立，第 1 / 3 / 4 项一并解决。
2. **`parse_list()` 是否把 `expected` 传给元素？**（第 4 项）—— 空 `[]` 与 list 里的 tuple 都依赖它。
3. **二元表达式是否把 `expected` 传给字面量操作数？**（第 5 项）—— `var x int8 = 1 + 2` 与 tuple 里的 `1 + 2` 同源。
4. **tuple 的运行期类型是否跟随声明类型？**（第 8 项）—— 需要先让第 9 项（tuple 的 `==`）可用，才能验证。

已定案的两条不要重复讨论：`parse_tuple()` 的现有行为保留；元素类型解析失败时报自己的错，**不退回到左侧类型**。第 6 项（`int` 不隐式转 `float`）是语言规则，不是 bug。

</details>

</details>