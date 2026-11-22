# `--inline` 扩大泛型支持（暂缓）

← 返回 [问题列表](issues.summary.md)

- [ ] 未实现（暂缓）—— 3 项限制：指令白名单、类型替换、`tp_args`

> 来源：`docs/Koala_TODO.md` §97（2026-11-13，暂缓）

---

### 详细描述

<details>
<summary><b>🔍 现状（`src/parser/opt/inline.c`）</b></summary>

目前只内联「泛型类的方法 + 接收者类型参数已确定」（如 `Box[int]` 上的方法，`init_type_subst`），且：

- **指令白名单**：`is_generic_safe_op` 只允许 `get_field` / `set_field` / `local` / `move` / `ret`。泛型类的 get/set 能内联，方法里有运算、调用、`!` 等就放弃。
- **类型替换**：`subst_type` 只替换 `T`、`T?` 和 self 类型；`list[T]`、`Box[T]` 这类嵌套泛型类型放弃。
- **带类型实参的指令**：`insn->tp_args` 非空（如 `foo[T](x)`）一律不内联。

泛型函数（非方法）有特化版本，特化后是普通函数，走普通路径，**不在本条范围**。

</details>

---

### 修复方案

<details>
<summary><b>🛠 方向</b></summary>

- [ ] 放开运算类指令：类型替换为具体类型后由 isel 按具体类型选指令，**逐个指令验证**。
- [ ] `subst_type` 支持嵌套泛型类型（实例化后的 instance 类型）。
- [ ] 支持 `tp_args` 的替换。
- [ ] 测试：`Box[int]` / `Box[str]` / `Box[T?]` 上带运算、调用、`!` 的方法，`--dump=inline-ir` 加运行结果。

</details>

<details>
<summary><b>⚠️ 风险</b></summary>

类型替换出错会产生类型不对的 IR，isel / 运行期才暴露；**每放开一类指令都要有对应测试**。

</details>

<details>
<summary><b>🔗 关联</b></summary>

[#90](issue-90.md)（§96 多 bb）：解析出 callee 后仍受那条限制。默认开启见 §98（已完成）。

</details>