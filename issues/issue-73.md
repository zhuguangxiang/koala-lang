# 可变字面量与 const pool：遗留项

← 返回 [问题列表](issues.summary.md)

- [ ] 部分完成 —— list/map/bytes/tuple 字面量已不再共享；Open 4 项中 1 已修，余 3 项 + 1 个根因待决

> 来源：`docs/Koala_TODO.md` §37（2026-11-01）

---

### 详细描述

<details>
<summary><b>✅ 已落地</b></summary>

list 与 map 字面量不再经 const pool 共享。此前全常量的 `[1, 2, 3]`、`{"a": 1}` 或 `list[int](1, 2, 3)` 是 const-pool 对象，`OP_LOADK` / `OP_RET_CONST` 原样交出，于是每次求值返回同一个对象，通过某次结果做的修改会被下一次看到。现在：

- `emit_ir_list`、`emit_ir_map` 与 `emit_list_call` 的常量分支（`irgen.c`）一律发射 `build_intern @list` / `@map`；
- `isel_lower_build_intern`（`isel.c`）不再把全常量 `build_intern` 折叠成常量；
- `emit_ir_tuple` 只在每一项都是不可变常量时才做常量元组（`is_immutable_const`：none、int、uint、float、bool、str，或由它们递归构成的 tuple / range / slice）；否则发射 `build_intern @tuple`。

测试：`test/test-run/test_const_mutable_fresh.kl`（运行期，在旧编译器上失败）与 `test/test-ir/test_const_mutable_ir.kl`（no-opt IR、优化 IR、LIR）。

</details>

---

### 修复方案

<details>
<summary><b>1️⃣ ✅ bytes 字面量仍在共享 —— 已修（见 §83）</b></summary>

irgen 把 `bytes` 字面量包进 `build_intern @bytes`（`INTERN_BYTES`），VM 求值成 pool 数据的一份拷贝，所以 `mk_bytes()[0] = 120` 不再影响下一次 `mk_bytes()` 的返回值；`bytes` 常量读回也一致。

测试：`test/test-run/test_bytes_literal_copy.kl`。

</details>

<details>
<summary><b>2️⃣ `const_copy_prop.c`（OP_IR_CALL 折叠）</b></summary>

对 `list` class 的常量调用仍用 `klr_const_list` 折叠，`tuple(...)` 调用用 `klr_const_tuple`，**都没有不可变性检查**。

已知输入都无法到达（irgen 自己下降 `list[T](...)`，不走调用）；需要决定：在那里改发 `build_intern`，还是断言该路径不可达。

</details>

<details>
<summary><b>3️⃣ 现已无用的路径</b></summary>

`klr_const_list` / `klr_const_map` 只剩第 2 项在用；`is_mergeable_key()`（`klr/ir.c`，编译期合并重复的常量 map 键）已不再使用（重复键由运行期 dict 合并）；loader 的 const list / dict 分支（`modobj.c`、`load_module.c`）与 `KL_MACH_CONST_LIST` / `_DICT` writer 分支对字面量已不可达。

**暂意保留**；第 2 项定案后再删。

</details>

<details>
<summary><b>4️⃣ 空 list 字面量与声明类型</b></summary>

`let l list[int] = []` 失败（`Types of two sides are not matched. lhs: list<j> =/= rhs: list<o>`）：`[]` 没有从声明取元素类型。`list[int]()` 可以。**早于本次改动就存在**（与 [#66](issue-66.md) 的同族问题相关）。

</details>

<details>
<summary><b>🔗 同日修复的相关项 + 仍开放的根因</b></summary>

实例类型（如 `Iterable[str]`）只有一个 interned type spec，但 `find_or_add_instance()` 会**按符号表**创建实例（例如包的 klc 加载时一个、正在编译的模块里一个），`stbl_add_instance()` 把共享 type spec 的 `sym_id` 指向最后一个。于是对 `Iterable[str]` 的 `for` 循环在 irgen 里崩溃（"Why goes here?"）：parser 在另一个实例上实例化了 `iter`。

`get_iter_func()`（`irgen.c`）现在通过 `get_instance_method()` 取方法，按需实例化；回归测试 `test/test-run/test_ext_instance_shared.kl`（配 `test/test_pkg/pkg4`）。

**仍开放**：其它用 `stbl_get(inst->stbl, ...)` 在实例上查方法的地方会遇到同样的不匹配，需审计（如 `irgen.c` 里 `stbl_get(inst_sym->stbl, fn->name)` 附近）。

同日第二例：`std/os` / `std/fs` 有了 `(int, int)?` 返回后，它们的 klc 加载也创建了 `Sequence[int]` 实例，`_add_instance_used_intf_to_trait()` 在模块自己的 `Sequence[int]` 上断言（`test-run/test_intf_ext_generic_inherit.kl`）：`KlrExtTrait` 是按名字找到的，所以两个实例共用它。现在只在 trait 尚未填的槽位上填（`klr_set_ext_intf()`）。

两者都是「一个 interned type spec 配多个实例符号」的症状；**根因修法（一个 type spec 一个实例）仍待决定**。

</details>