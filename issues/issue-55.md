# 泛型体内 T 的点号方法调用（itab 挂接）—— 作者决定暂时搁置

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— **作者决定暂时搁置**，重开前不推进任何方案

> 来源：`docs/Koala_TODO.md` §8（2026-08-22 建立）

---

### 详细描述

<details>
<summary><b>🔍 现状</b></summary>

泛型体内对 `T : Trait` 参数做点号方法调用（如 `a.__str__()`、`a.hash()`）会撞上 `vm_ops.h:431` 的 `is_intf(callable)` 断言 —— 槽调用（`itab->methods[index]`）要求接收者携带 itab，而 `T` 实参是**裸值**。

</details>

<details>
<summary><b>🧠 根因链</b></summary>

`irgen.c` 的 `_build_obj_intf_upcast` 对 `SYM_TYPE_PARAM` 目标返回 **NULL**（只处理 class → trait 的 `make_intf` 与 trait → trait 的 `upcast_intf`），调用点不发射任何挂接指令 → 裸值进泛型体 → 槽调用断言。

</details>

<details>
<summary><b>✅ 不受影响的边界</b></summary>

运算符路径（`==`、`+` 等）走 `OP_NUM_*` 协议分发，**不需要 itab**，继续可用。

</details>

<details>
<summary><b>🚫 已否决的方向（重开时不必重提）</b></summary>

1. **调用点递归挂 bound[0] 的 itab** —— 语义错误：itab 需求归属**泛型体内的使用点**（用了哪个 bound 的方法），不归调用点；body 用第二个 bound 即失效；且 `get_intf_index` 按类查 lro，`T` 无类，索引无法烘焙进指令。
2. **使用点按 trait 名解析**（`typeof` / `itab->tp` 回指恢复具体类型后在 `tp->itables` 里按名匹配）—— 原理上可行（`IntfTable` 回指使包装值可恢复），且能结构性消解多 bound 争抢单 itab 槽位的问题，但**作者判定为时过早，搁置**。

</details>

---

### 修复方案

<details>
<summary><b>⏸ 状态：搁置</b></summary>

此场景当初未实现是**有意为之**，复杂度超出表面。重新开启前不推进任何方案；届时应先从「按 trait 名解析」方向重新评估。

</details>