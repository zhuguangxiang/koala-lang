# 容器协议指令族实现（VM handler / printer / isel）

← 返回 [问题列表](issues.summary.md)

- [ ] 部分完成（7/12）—— 5 条「已定义、未实现」

> 来源：`docs/Koala_TODO.md` §12（2026-09-01 建立，2026-10-02 复核）
>
> 与 §9（list push/pop 的 `@intrinsic` 化，见 [#53](issue-53.md)）相关但**正交**：§9 是方法内置，本节是语法钩子协议指令。三层对应见 `Koala_Design_Overview.md` §8.3。

---

### 详细描述

<details>
<summary><b>✅ 已完成全链路（isel / printer / VM TARGET）—— 7 条</b></summary>

- [x] `OP_SEQ_GET`（TARGET 经 `kl_slot_call` 走 `SLOT_GET_ITEM`）
- [x] `OP_SEQ_GET_IMM`（`isel.c:1051` 常数下标特化）
- [x] `OP_SEQ_SET`（`SLOT_SET_ITEM`）
- [x] `OP_SEQ_SET_IMM`（`isel.c:1078`）
- [x] `OP_LEN`（2026-10-02 起仅 for-each `Sequence` 快速路径内部指令：irgen → `klr_build_seq_len` 发射，TARGET 走 `SLOT_LEN`；原顶层 `len(x)` 的 isel magic 下降分支已删除，用户 `x.len()` 直落 resolved call。`OP_HASH` 已随顶层 `hash()` 退役，零引用）
- [x] `OP_CONTAINS`（`vm_ops.h` TARGET 经 `SLOT_CONTAINS`；`in` 语法全链路见 §4）
- [x] `OP_SEQ_GET_SLICE`（`vm_ops.h` TARGET 经 `SLOT_GET_SLICE`）

</details>

<details>
<summary><b>❌ 已定义、未实现 —— 5 条</b></summary>

- [ ] `OP_SEQ_SET_SLICE`（`SLOT_SET_SLICE`）
- [ ] `OP_MAP_GET`（IR 层已有：`insn.c:816` 用于成员访问下降；VM TARGET 缺）
- [ ] `OP_MAP_SET`（IR 层已有：`insn.c:842`，`dce.c` 已识别其副作用；VM TARGET 缺）
- [ ] `OP_LIST_PUSH`（IRGen 发射 + TARGET；见 §9）
- [ ] `OP_LIST_POP`（`OP_CALL` + ISEL 特化路径；见 §9）

</details>

---

### 修复方案

<details>
<summary><b>🛠 每条待办含</b></summary>

`vm_ops.h` TARGET（`kl_slot_call_*` 模式）、`printer.c` case、isel 规则（如适用）、回归测试。

</details>

<details>
<summary><b>🧹 源码注释遗留（顺手修正项）</b></summary>

- `slotid.h:30` `SLOT_LEN, // __len__` —— 注释已换成 dunder 名，但 `len` 无 dunder（普通方法，2026-10-02 裁定）；绑定表实证 `TPSLOT("len", SLOT_LEN)`（`klc.c:462`），宜改 `// len`。同族 `// __getitem__` 等对应真 dunder，不受影响。
- `vm_ops.h:1531` `// TODO: can be negative?` —— 即 §11 的负下标问题，见 [#54](issue-54.md)。

</details>