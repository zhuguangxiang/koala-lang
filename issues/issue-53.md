# list 的 append/pop 注解升级：@native → @intrinsic

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— `append` / `pop` 仍是 `@native`，依赖 `OP_LIST_POP` 去留待确认

> 来源：`docs/Koala_TODO.md` §9

---

### 详细描述

<details>
<summary><b>🔍 现状</b></summary>

`list.kl` 中 `append(value T)` 和 `pop(index = -1) T` 标记为 `@native`。

</details>

<details>
<summary><b>🎯 目标</b></summary>

改为 `@intrinsic`，让编译器直接发射专用 VM 指令：

- `push` → `OP_LIST_PUSH`（IRGen 已直接发射，无需 ISEL 特化）
- `pop` → `OP_LIST_POP`（当前走 `OP_CALL` + ISEL 特化路径，**需 ISEL 支持**）

</details>

<details>
<summary><b>💡 理由</b></summary>

`push`/`pop` 是最高频的容器操作（循环构建列表、栈操作），`@intrinsic` 消除 slots 分发开销，直接操作内部数组 + 容量检查 + write barrier。

</details>

---

### 修复方案

<details>
<summary><b>🛠 施工方向</b></summary>

1. `list.kl`：`@native` → `@intrinsic`（push、pop 两个方法）
2. 确认 IRGen 对 `list.append()` 发射 `OP_LIST_PUSH`（已有逻辑）
3. 确认 ISEL 对 `list.pop()` 的 `OP_CALL` 特化为 `OP_LIST_POP`（**需核实**）
4. 全量回归测试

</details>

<details>
<summary><b>⚠️ 依赖 / 未决</b></summary>

`OP_LIST_POP` 指令**是否保留待确认**（作者曾讨论移除，改为走 slots 分发）。

- 若决定保留 → push / pop 双双升级为 `@intrinsic`。
- 若移除 `OP_LIST_POP` → 仅 `push` 升级。

</details>