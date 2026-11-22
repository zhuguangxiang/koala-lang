# class 转 trait 测试发现的问题（崩溃 + 待确认）

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 2 处崩溃 + 4 项待确认 + 5 项泛型场景

> 来源：`docs/Koala_TODO.md` §90（2026-11-10，待修 / 待确认）

---

### 详细描述

<details>
<summary><b>📌 测试基础</b></summary>

覆盖局部变量、参数、返回值、字段、全局变量等 **65 种写法**，通过的场景已固化为 `test-run/test_class_to_trait.kl`（刻意避开下列崩溃写法）。以下问题暂未处理。

</details>

<details>
<summary><b>1️⃣ 编译器中止：`let` 声明的可选 trait 变量再用 `!` 取值</b></summary>

```kl
let a Animal? = Dog()
print(a!.name())        // 或 let b = a!
```

`isel.c:934` `isel_lower_cast` 断言 `ts_symbol_is_trait(src_ts) && ts_symbol_is_klass(dst_ts)` 失败。

**原因**：const-copy-prop 把 `cast %a Animal? to Animal` 中的 `%a` 换成了初值 `%1 = make_intf ... to Animal`，得到 `cast %1 Animal to Animal`（源与目标是同一个 trait），而 isel 只接受 trait → class 的 cast。

`let a Animal? = d`（`d` 是 `Dog`）、`let a Animal? = p`（`p` 是 `Pet`）同样中止。`var` 声明、`if let` 取值正常。

**方向**：isel 把同类型的 cast 当 move，或 const-copy-prop 不产生同类型 cast。

</details>

<details>
<summary><b>2️⃣ VM 崩溃：可变参数 `...Trait` 接收类实例</b></summary>

```kl
func names(xs ...Animal) str {
    var s = ""
    for x in xs { s += x.name() }
    return s
}
names(Dog(), Cat())     // _eval_frame 断言 callable->tag 失败
```

可变参数被打包为 `build_intern @tuple, %0 Dog`，类型 `tuple<Dog>`，元素**没有** `make_intf` 成 `Animal`；体内按 intf 调用 `x.name()` 时崩溃。

只调用 `xs.len()` 不崩；实参已是 `Animal` 值时正常；混合（`names(a, Cat())`）也崩溃。

**方向**：`handle_valist_and_dfl_args()` 构造的 tuple 按可变参数的元素类型转换每个元素（与 list/dict 字面量的 `literal_item()` 相同）。

</details>

<details>
<summary><b>3️⃣ tuple 字面量与 tuple 解构不一致（待讨论）</b></summary>

`let t (Animal, int) = (Dog(), 1)` 编译报错（§89 已记），但 `let (a Animal, n int) = (Dog(), 1)` 与 `(a, n) = (Cat(), 2)` 正常，返回类型 `(Animal, int)` 的函数 `return (Dog(), 1)` 也报错。

**是否让 tuple 字面量按期望类型逐元素转换，待讨论。**

</details>

<details>
<summary><b>4️⃣ trait 值不能用 `==`（待确认）</b></summary>

两个 `Animal` 值 `a == b` 报 "operator '==' is not defined in type 'Animal'"。

设计上每个 class 自动遵循 `Equatable`（identity 默认），但 **trait 类型本身不带 `__eq__`**。是否让所有 trait 值支持 `==` / `!=`（按值自身类型的 `__eq__`），**待确认**。

</details>

<details>
<summary><b>5️⃣ `if` 表达式不能直接作为调用实参（待确认）</b></summary>

`show(if b { Dog() } else { Cat() })` 语法错误（与 trait 无关）；`let x = if ...` 和 `func f() T = if ...` 可用。是否放开，**待确认**。（同 [#63](issue-63.md) 第 5 项。）

</details>

<details>
<summary><b>6️⃣ `?.` 未实现（已知）</b></summary>

`a?.name()` 报 "not supported yet, use 'if let' or '!.'"。

</details>

---

### 修复方案

<details>
<summary><b>🧬 泛型场景（2026-11-10 补测）</b></summary>

覆盖 **80 种**带泛型的写法（泛型函数 / 方法，有无约束；泛型类、泛型 trait 以 trait 实例化；局部变量、参数、返回值、字段、全局变量、容器、可选、嵌套），通过的已固化为 `test-run/test_generic_class_to_trait.kl`。下列问题暂未处理（§89 已记的不重复）。

| # | 写法 | 结果 |
|---|---|---|
| 1 | 可变参数泛型 `func va[T](xs ...T)` 以 `va(1, 2)` 调用 | 编译器中止（类型推断未实现，与 trait 无关；`va[int](1, 2)` 正常） |
| 2 | 带约束的可变参数 `va_bound[Animal](Dog(), Cat())`，函数内调用元素的方法 | VM 崩溃（与不带泛型的 `...Animal` 同源） |
| 3 | `let a Animal? = ident(Dog())` 后接 `a!` | 编译器中止（与不带泛型的 `let` 加 `!` 同源） |
| 4 | `func rl() list[Animal] = [Dog()]`，dict 字面量同理 | 编译报错；`{ return [Dog()] }` 正常，两种函数体不一致 |
| 5 | `enumerate([1, 2])` | 报 "'enumerate' is not found"，手册说 `enumerate` / `zip` 可用 |

</details>

<details>
<summary><b>🧬 泛型场景细节</b></summary>

- **编译器中止：泛型可变参数 `...T` 的类型推断未实现**：`func va[T](xs ...T) int` 以 `va(1, 2)` 调用即在 `parser_generic.c:126` `infer_tp_from_call()` 的 `TYPE_VA_LIST` 分支 `NYI()` 中止（与 trait 无关，普通 int 也中止）。`va[int](1, 2)` 显式写类型实参正常。
- **VM 崩溃：有约束的泛型可变参数接收类实例**：`func va_bound[T : Animal](xs ...T)` 以 `va_bound[Animal](Dog(), Cat())` 调用、体内 `x.name()` 时崩溃。与上面第 2 处同源：打包 tuple 时元素未 `make_intf`。
- **`let x Trait? = <泛型调用>` 后 `x!`** 同样触发 `isel.c:934` 编译器中止：`let a Animal? = ident(Dog())` 再 `a!.name()`，与上面第 1 处同源。
- **`= expr` 函数体不把返回类型传给 list / dict 字面量**：

  ```kl
  func rl() list[Animal] = [Dog()]          // 报错：list<Dog> 不匹配 list<Animal>
  func rd() dict[str, Animal] = {"a": Dog()} // 同样报错
  func rb() list[Animal] { return [Dog()] }  // 正常
  ```

  两种函数体对字面量的期望类型处理不一致；`= expr` 体应与 `return` 一样调用 `parse_literal_as_expected()`。

- **`enumerate` / `zip` 不可用，与手册不符**：手册 Branching and Loops 写 "Top-level iteration combinators `enumerate` and `zip` are available"，实际 `enumerate([1, 2])` 报 "'enumerate' is not found"（`libs/std/builtin/enumerate.kl`、`zip.kl` 中的定义是**注释**）。需实现，或修改手册。

</details>