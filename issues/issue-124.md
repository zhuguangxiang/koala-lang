# `OP_NUM_*` 全特性实现（VM handler 族）

← 返回 [问题列表](issues.summary.md)

- [ ] 大部分完成 —— 比较族 / 算术族 / 位运算族 TARGET 全齐；**printer 缺 9 条 case、第三批 `num.*` 比较跳转融合未做**

> 来源：`docs/Koala_TODO.md` §7（2026-08-22 建立；实现完成一项打勾一项，**不删除**）

---

### 详细描述

<details>
<summary><b>📈 进展更新（2026-08-30，取代原始快照）</b></summary>

- `typeslots.c` 已删除，slots 改为 `TypeObject` 上的 `Vector`；`slotid.h` 独立头文件；
- `slotdefs` 迁移至 `klc.c`（`SlotDef slotdefs[]` 数组）；
- **比较族 6 件 TARGET 全齐**（含 int64 / uint64 / float64 快路径）；
- **算术族 5 件 TARGET 全齐**（ADD/SUB/MUL/DIV/MOD），通过 `kl_slot_call_one_arg` 走 slots 分发；
- `printer.c` 覆盖度待核实（→ 见下「仍未完成」）；
- **位运算族 5 件**（AND/OR/XOR/SHL/SHR）：done，见 §1 的 2026-11-06 更新；
- `test_generic_9` / `test_generic_14` 均绿；
- 新增 `@specialized` 泛型函数单态化（commit `3d9ea1a6`，`test_specializd.kl` 验证通过）。

</details>

<details>
<summary><b>🗄 原始快照（2026-08-22 建立时点，已部分过时，保留备查）</b></summary>

- `opcode_list.h`：16 条 `OP_NUM_*`（算术 ADD/SUB/MUL/DIV/MOD、位运算 AND/OR/XOR/SHL/SHR、比较 EQ/NE/LT/LE/GT/GE），全部 `FORMAT_RRR`；
- `isel.c` 的 `num_ops_rules`：`OP_BINARY_*` → `OP_NUM_*` 16 条映射齐全；`lower_operands.c` 的 `is_binary` 已覆盖 `OP_NUM_ADD..OP_NUM_GE` 区间；
- `vm_ops.h`：当时仅 `OP_NUM_EQ` 有 TARGET，其余 15 条在非 computed-goto 模式落 `eval.c` 的 `UNREACHABLE`（`test_generic_9` 失败的直接原因）；
- `typeslots.c`：比较六件已挂（`slotdefs` 全齐，`cmp → slot_tp_richcmp`）；`num_slotdefs` 已存在但 FUNC 全为 NULL、绑定循环是注释占位；`bit_slotdefs` 不存在；`TypeObject->arith` / `->bit` 全库从未被分配或赋值；
- `printer.c`：只打印 `num.eq` / `num.gt`，缺 14 个 case；
- 内建数值类型运行时**没有** cmp / arith / bit 函数：`@intrinsic` 条目在 klc 里是普通空体 `CodeObject`（`write_klc` 只认 `SYM_FLAGS_NATIVE`），slots 动态分发执行空体帧不可用。比较族的对策已定为 **TARGET 内建快路径**（`OP_NUM_EQ` 先行）。

---

### 修复方案

<details>
<summary><b>✅ 第一批：比较族（全部完成）</b></summary>

- [x] `OP_NUM_EQ` 增加 int64 / uint64 / float64 快路径 —— 2026-08-22
- [x] `OP_NUM_NE` / `LT` / `LE` / `GT` / `GE` 五个 TARGET —— 2026-08-22
- [x] uint64 快路径：`(uint64_t)ival` 无符号比较，六条全齐 —— 2026-08-22
- [x] `test_generic_9` 转绿 —— 2026-08-22

</details>

<details>
<summary><b>✅ 第二批：算术族（全部完成，通过 slots 分发而非 typeslots.c 蹦床）</b></summary>

- [x] `OP_NUM_ADD/SUB/MUL/DIV/MOD` 五个 TARGET（`kl_slot_call_one_arg` 走 `SLOT_ADD..SLOT_MOD`）
- [x] `test_generic_14` 用户自定义数值类全链路验证通过

</details>

<details>
<summary><b>✅ 第二批续：位运算族（TARGET 已完成，printer 仍缺）</b></summary>

- [x] `OP_NUM_AND/OR/XOR/SHL/SHR` 五个 TARGET（2026-11-06，走 `kl_call_bit_*`，`test-run/test_shift_trait.kl` 验证）
- [ ] **`printer.c` 位运算 case 核实与补齐** —— 2026-11-22 按代码核对：`src/parser/klr/printer.c:1303-1330` 只有 `OP_NUM_EQ/NE/LT/LE/GT/GE` 六个 `print_cmp` 与 `OP_NUM_ADD` 的 `print_binary`，**`SUB/MUL/DIV/MOD` 与五个位运算共 9 条仍无 case**

</details>

<details>
<summary><b>⬜ 第三批：`num.*` 比较跳转融合（jmp fusion）· 未做</b></summary>

**动机**（实测 IR，`func max[T : Comparable](a, b) { if a > b { return a } ... }`）：

```
0002:  AE020001   num.gt r2, r0, r1
0003:  54020001   jmp_false r2, 1
0004:  75000000   ret r0
0005:  75000001   ret r1
```

`num.gt` 的结果 r2 仅被 `jmp_false` 消费，两条指令应融合为单条 `jmp_num_gt r0, r1, off`：省一次中间寄存器写读 + 一次 DISPATCH，且快路径比较与跳转在同一 handler 内完成。

**现状对照**：单态融合链路已全齐 —— `isel.c` 的 `int_cmp_map` / `float_cmp_map`（+ uint 系）把 `OP_INT_*` / `OP_FLOAT_*` 比较 + jmp 融合为 `OP_JMP_INT_*` / `OP_JMP_UINT_*` / `OP_JMP_FLOAT_*`（含 `_IMM` 变体，`FORMAT_RROff` / `RImmOff`，printer 的 `print_jmp_cond_fused`、cgen 的 `fused_jmp()` + `lower_fused_jmp` 均就绪）；**唯独泛型 `num.*` 比较没有融合族**（全仓库 grep 不到 `OP_JMP_NUM`）。

- [ ] `opcode_list.h`：新增 `OP_JMP_NUM_EQ/NE/LT/LE/GT/GE`（`FORMAT_RROff`；`_IMM` 变体视收益再定，比较对象多为泛型变量，初版可不做）
- [ ] isel：`OP_BINARY_GT..GE`（走 `num_ops_rules` 的泛型比较）后接 `OP_JMP_FALSE` / `OP_JMP_TRUE` 时融合为 `OP_JMP_NUM_*`（真值分支取反语义与现有单态融合保持一致）
- [ ] `vm_ops.h`：六个 TARGET，直接复用 `OP_NUM_*` 比较族的快路径分支（int64 有符号 / float64 直比 / uint64 `(uint64_t)` 无符号 / cmp 兜底），命中即跳，不再写中间寄存器
- [ ] `printer.c` / `cgen`：`print_jmp_cond_fused` 六个 case + `FORMAT_RROff` 表项
- [ ] 验证：`test_generic_9` 的 max/min if-branch 反汇编出现融合指令，全量回归无新增

</details>

<details>
<summary><b>🧪 验证基线</b></summary>

- [x] `test_generic_9` 转绿（六件比较运算符 × int64 / float64）—— 2026-08-22
- [x] `test_generic_14` 用户自定义数值类全链路 PASS —— 2026-08-22
- [x] 全量 lit 基线：**136/139 通过**（97.84%），2 Unresolved（`test_bytes.kl` / `test_io.kl`），1 FAIL（`test_pkg_4.kl`，`LD_LIBRARY_PATH` 未设置）
- [x] 2026-10-02 测试基线盘点：lit 发现 **190** 个测试（test-kl 38 / test-ir 36 / test-run 88 / test-ut 28），全部带 RUN 行 —— §15 的「无 RUN 挂账」全部消解

</details>

<details>
<summary><b>⚖️ 决策点（待作者拍板）</b></summary>

1. `@intrinsic` 空体的系统性解法：比较族已走 TARGET 快路径；**算术族已确定走 slots 调用**（`kl_slot_call_one_arg`）；位运算族跟随同一模式。
2. 一元 `-` / `~`（`OP_UNARY_*` 仅 IR 伪指令）与泛型复合赋值仍是独立项，不在本批 —— 见 [#117](issue-117.md)（§47.5）。

</details>

<details>
<summary><b>🔗 关联</b></summary>

- [#123](issue-123.md)（§6）：0.8.0 第 1 项「用户自定义 class 的数值运算符」，其 VM 侧底座就是本节的 `OP_NUM_*` 族。
- [#113](issue-113.md)（§49）：运算符副作用与 `pure`，决定 `num.*` 结果能否被 `lvn` 合并 / `dce` 删除。
- [#117](issue-117.md)（§47）：`+` 在用户类 / `bytes` / `str` 不应被视为可交换，白名单逻辑就在这些 `OP_NUM_*` 判断上。
- §105（LSRA 寄存器分配优化，已完成）：融合指令与 `num.*` 的寄存器需求都由它决定；`unused_needs_reg()` 的兜底见 [#117](issue-117.md) 47.3。

</details>
</details>