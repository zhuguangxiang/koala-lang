# `build_intern` 操作数个数溢出 `nargs:8` 上限

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 前端无诊断，溢出 / 截断后的 `nargs` 会直接进字节码

> 来源：`docs/Koala_TODO.md` §26（待施工）

---

### 详细描述

<details>
<summary><b>📐 编码约束</b></summary>

`OP_BUILD_INTERN` 的指令格式为 `| op:8 | rd:8 | tag:8 | nargs:8 |`（见 `include/common/opcode_list.h:1685` 的格式注释），操作数个数只占 8 位，可表示 **0–255**；即 `@map` / `@list` / `@tuple` 一个字面量最多 **255 个元素**。

</details>

<details>
<summary><b>⚙️ VM 侧取用</b></summary>

`include/runtime/vm_ops.h:546` 直接 `imm = I_VAL(inst, 0, 8)` 交给 `do_build_intern()`，**不做范围校验**。

</details>

<details>
<summary><b>🔍 现状</b></summary>

isel 生成路径（`src/parser/backend/isel.c` 的 `isel_lower_build_intern()` 及指令编码处）按 `insn->num_opers` 直接遍历，**未做上限检查**；parser / 诊断层亦无对应报错。属待施工项。

</details>

---

### 修复方案

<details>
<summary><b>🛠 待办</b></summary>

当字面量的元素个数（`build_intern` 的 `num_opers`）越过该字段可表示的范围时，编译器前端应在生成 `build_intern` 指令处**报编译错误**：

- 错误信息需带**字面量位置**与**元素个数**；
- 不得让溢出 / 截断后的 `nargs` 进入字节码交给 VM。

</details>

<details>
<summary><b>🔗 关联</b></summary>

§12（容器协议指令族，见 [#57](issue-57.md)）—— 若后续给 `build_intern` 换用更宽的指令格式，本节随之失效。

</details>