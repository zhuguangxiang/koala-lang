# 15. 原生变参 `__init__` 的实参展平只对 list 生效

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 展平条件仍写死 `type_is_list`，待决定是否通用化

---

### 详细描述

<details>
<summary><b>📖 背景</b></summary>

Phase 2 把 list 构造从 `build_intern @list` 改为普通的 `new` + `call @list:__init__`。

`__init__(args ...T)` 是原生函数，元素在 call slot 里平铺，因此 `emit_ir_call` 把 parser 为变参打包的元组**直接拆开**，不再分配那个只用一次的中间元组。

</details>

<details>
<summary><b>🔍 现状</b></summary>

展平条件**写死为 `type_is_list(callee->ts)`**（`irgen.c` 的 `emit_ir_call`）。

目前标准库里只有 `list` 和 `tuple` 声明了 `__init__(args ...T)`，tuple 按用户要求仍走 `build_intern @tuple`（不可变、可进常量池），所以展平**没有推广到通用路径**。

</details>

<details>
<summary><b>💥 影响</b></summary>

将来若新增别的原生变参 `__init__` 类，仍会先建中间元组再拷贝，**白白多一次分配**。

</details>

<details>
<summary><b>📎 附带变更</b></summary>

`tuple::__init__`（`tupleobj.c` 的 `_tuple_init`）已随本次回退删除，tuple 不再需要它；`list::__init__` 保留。

</details>

---

### 修复方案

<details>
<summary><b>🛠 建议修法</b></summary>

把判断改成「callee 的 `__init__` 是原生且形参为单个 `...T`」之类的**通用条件**。

</details>

<details>
<summary><b>🕐 记录状态</b></summary>

2026-11-21 记录，待后续决定是否通用化。

</details>