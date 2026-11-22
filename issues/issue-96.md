# `@specialized` 泛型函数：IR 特化的开放项（6 项）

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 6 项开放点，其中 3 项「尚无测试能触及」

> 来源：`docs/Koala_TODO.md` §40（2026-11-01，to fix later）

---

### 详细描述

<details>
<summary><b>✅ 已落地</b></summary>

`@specialized` 函数现在是泛型函数 IR 的克隆（`src/parser/klr/specialize.c`，由 `irgen.c` 的 `emit_specialized_funcs()` 驱动）：所有指令被克隆并替换类型变量，对泛型函数的调用按具体类型实参路由，`T` 接收者上的 intf 调用被去虚化成类方法，`T` 值进入 trait 的地方 irgen 留下 `cast %x T to Trait`，克隆时变成 `make_intf`。

</details>

---

### 修复方案

<details>
<summary><b>1️⃣ 擦除路径仍会在「`T` 值当 intf 值」时崩溃</b></summary>

（§8，作者搁置；§34 关于泛型自由函数的 bound 实参）

调用方传裸值（`call @ts1 [const], 5 int64`），但泛型体对它做 intf 调用（`T : ToString` 时的 `x.to_str()`）→ `vm_ops.h:430` 断言。**只有 `@specialized` 克隆被修好了。**

**§8 重开时的新事实**：现在每个「`T` 值进入 trait」的位置都带标记 `cast %x T to Trait`（isel 下降为 move），所以擦除路径有了**单一的挂接点**可以用来插入转换。

</details>

<details>
<summary><b>2️⃣ 去虚化的回退路径没有测试</b></summary>

当类型实参的类**本身没有定义**该 trait 方法（例如 trait 默认方法）时，`specialize_intf_call()` 用 `make_intf` 包住接收者并保留 intf 调用。

今天没有任何 primitive 类型（`@specialized` 唯一接受的类型）拿到 trait 默认方法，所以**没有测试能到达**这条路径。

</details>

<details>
<summary><b>3️⃣ `T?` → `Trait?` 没有标记</b></summary>

`_build_opt_obj_intf_upcast()` 对类型变量来源提前返回（`_intf_upcast_needed()` 为 false），于是克隆会把裸 optional 值塞进 optional trait 槽位。**尚无代码触及。**

</details>

<details>
<summary><b>4️⃣ `mangle_func_name()` 只处理 primitive、`str`、`bool` 与 class 类型实参</b></summary>

`type_spec_str_func_name()`（`typespec.c`）对其它种类（optional、tuple、proto …）走到 `UNREACHABLE()`，于是路由一个具体类型实参含这些种类的调用会让编译器中止 —— 解析期或克隆中。**读代码发现，尚无测试。**

</details>

<details>
<summary><b>5️⃣ `koala` 驱动掩盖 `koalac` 的崩溃</b></summary>

当 `koalac` 因信号死亡（如 irgen 里的段错误），`koala` 只打印 "koala: compilation failed"。应当报告信号，例如 "koalac crashed (signal 11)" —— 与 §38.2（[#64](issue-64.md) 第 2 项）要求的诊断一致。

</details>

<details>
<summary><b>6️⃣ 泛型体里显式类型实参用类型变量会中止编译器</b></summary>

泛型体中 `twice[T](x)`（而非推断的 `twice(x)`）失败于 `parse_index_func_tp()`（`parser_expr.c`）的断言 `arg_sym && arg_sym->kind == SYM_CLASS`，有无 `@specialized` 都一样。

每个显式类型实参被当成类符号（取其 `instance_ts`）；类型参数（很可能还有 `list[T]` 这样的泛型实例，或 `int?`）未被处理。

**期望**：使用类型实参的 type spec；含类型变量时该调用保持为擦除调用，并把类型实参留在 call 上供克隆使用（与推断调用一样用 `exp->tp_args`）。

</details>