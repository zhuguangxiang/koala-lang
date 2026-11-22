# switch/case 完整路线图（phase 1 + 2a 已完成；phase 3+ 规划）

← 返回 [问题列表](issues.summary.md)

- [ ] 进行中 —— phase 1 与 phase 2a 已验收；phase 2b（暂不支持）、phase 3（optional）、#19（表达式）待做

> 来源：`docs/Koala_TODO.md` §112（2026-11-20）
>
> **已定决策**：optional subject 暂不放开（设计归档于 §78 第 13 项末尾，实现见本文 3）；**不做** `switch let`。enum 本身见 [#111](issue-111.md)。

---

### 详细描述

<details>
<summary><b>📊 现状（112.0）</b></summary>

- 语法 / AST / 语义 / irgen / 测试见 §78 第 13 项。dispatch：整数 / enum 常量 switch（≥4 键）走 **O(log n) 二分分支树**（已实现）；其余仍是 O(n) 比较链 —— 每个 value case 一条 `cmpeq` + `OP_IR_JMP_COND`，`is T` 走 `OP_IS`，`in c` 走 `__contains__`，`_` 为 default。
- subject 只求值一次（非 NAME 时 move 到合成 local）；optional 在 `parse_switch`（parser.c:2902）早期拒绝。
- IR 终结指令：`OP_RET` / `OP_RET_VOID` / `OP_IR_JMP_COND`（branch，4 operand：cond/reserved/then/else）/ `OP_JMP`（1 operand：target）；`insn_is_terminator()`（ir.h:968）列举。边由 `klr_link_edge(src,dst)`（ir.c:675）维护 in/out_edges；phi 用 `phi_preds[]` 按边匹配。
- opt 管线（opt.c）：`kl_optimize` = const-copy-prop → normalize → lvn → cfg_bb_opt(remove-only-jump / branch-folding / remove-unused / merge-block) → dce；`kl_ssa_opt` = normalize → cfg_bb_opt → dce → `kl_do_ssa` → `kl_do_sccp` → `kl_exit_ssa` → 再跑管线。sccp `evaluate_terminator`（sccp.c:493）按终结指令 code 推可达边；isel `isel_lower_jmp_cond`（isel.c:1221）把 cmp+branch 融合成 `OP_JMP_INT_*` 等。

</details>

<details>
<summary><b>✅ Phase 2a — 整数 / enum 二分 dispatch（已完成，2026-11-20）</b></summary>

**状态：已完成并验收**（LIT 602/602、CTEST 9/9、valgrind 干净）。

**适用范围**：subject 为 `int` / `uint` / `enum`（底层 int64），且**所有非 default clause 的 pattern 都是编译期常量整数**（`CasePattern.has_const`，由 `parse_switch` 用 `switch_pattern_const` 标记），常量键数 ≥ `SWITCH_BINARY_MIN_KEYS`（=4）。

- **dense 与 sparse 统一走二分 O(log n)**：二分查找不要求值域连续，连续 / 稀疏 / 负数 / 大间隔都覆盖。值域密度只影响是否值得跳转表（2b，暂不支持）。
- **保持比较链**：`bool`（最多 2 键，trivial）、`str` / `float`（相等比较）、`is T`、`in c`、非常量 value case、键数 < 4。含任一不受益 pattern 时**整体回退**比较链（首版从简）。

**实际实现（直接 irgen lowering，取代原 Approach B）**：原计划新增 `OP_IR_SWITCH` + `switch_lower` pass；实施时发现 `koalac.c` 中 `kl_inline` 先于 `kl_optimize` 运行，高层 switch 会泄漏进 inline 克隆。故改为**直接在 `emit_ir_switch` 发射二分分支树** —— 不新增 opcode / printer / verifier / pass，IR 自始至终只含标准分支，inline / opt / ssa / isel / cgen / jit **全部零改动**，爆炸半径最小。

- `irgen.c`：`switch_cmp`（由 `build_switch_eq` 重构，EQ/LT 共用扩宽逻辑）；`switch_emit_binary`（eligibility 检查 + 收集 typed key operand + 排序 + 建树）；`switch_build_bst`（递归）；`switch_emit_chain`（原比较链抽出的回退函数）。
- **lower-bound 二分树**：键升序排序，内部节点一次 `subject < keys[mid]` 导航（`mid=(lo+hi+1)/2` 保证右半平衡），叶子一次 `subject == keys[lo]` 验证；命中跳 clause body，未命中跳 `fall_bb`（有 default → default body；否则 end / 穷尽无 default 时 unreachable）。最坏 ⌈log₂m⌉+1 次比较，m ≥ 4 时严格优于比较链。
- **类型处理**：int/uint 在入口一次性扩宽到 int64/uint64（`klr_build_cast`），key operand 同扩宽；enum 保持原类型 + int64 成员值，走通用 `OP_NUM_*` 比较（不 cast，规避 `isel_lower_cast` 无 enum→int64 分支的风险）；uint 的 `OP_BINARY_LT` 由 isel 按操作数类型 lower 成无符号 `OP_UINT_LT`。
- subject 只求值一次，二分树各块均被 entry 支配，复用同一 `subj` 值。
- **后端零改动**：isel `isel_lower_jmp_cond` 自动把 `lt`/`eq`+branch 融合成 `jmp_int_lt_imm` / `jmp_int_eq_imm`（enum 走 num 融合）。
- AST：`CasePattern` 增 `has_const` / `const_val`（`parse_switch` 填充），irgen 直接读取，避免暴露 static `switch_pattern_const`。

**测试**：`test/test-ir/test_ir_switch.kl`（FileCheck 验证 `jmp_int_lt_imm` 导航 + `jmp_int_eq_imm` 叶子）、`test/test-run/test_switch_jump.kl`（dense / sparse / multi-pattern / 负数 / uint64 大值 / enum 穷尽无 default / 含 default，逐例运行结果正确）；现有 4 个 switch 测试与 optional 禁令保持绿。

---

### 修复方案

<details>
<summary><b>🚫 Phase 2b — dense 跳转表（暂不支持，2026-11-20 决定）</b></summary>

**用户决策：暂不支持跳转表**，通用二分（2a）收益已足够、风险更低。以下仅作未来归档。

仅当值域紧凑（`(max-min+1)` 与 case 数之比小于阈值，如 ≤ 2~3 倍）且 case 数足够多时启用。需后端支持：

- 新 VM op（如 `OP_SWITCH_INT`）：subject 减 min 得索引，范围检查后查表间接跳转；cgen 发射跳转表（block 偏移数组）+ 间接 jmp；JIT（jit.c）镜像或回落解释器。
- isel 把 dense 的 switch lower 成该 op；sparse 仍走 2a 二分。
- **风险**：跳转表重定位、JIT 代码区 W^X、fixup 表（多出口）。收益需基准实测（分支数大才显著），故后置。

</details>

<details>
<summary><b>⏸ Phase 3 — optional subject（设计已归档，待启动，7 步）</b></summary>

设计见 §78 第 13 项末尾 *Deferred: optional switch subjects*。实现要点（`T?` subject）：

1. `parse_switch`：移除 parser.c:2902 的拒绝；记录 `st->opt.src`（底层 T）。
2. `switch_check_value`：pattern 对 `st->opt.src`（T）检查，而非 `T?`（沿用 let-else 语义）。
3. `emit_ir_switch`：nil-first 分裂 —— `cmpeq subj, none` → nil 臂（nil pattern / 含 nil 的 `_`）；non-nil 臂把 subj 解包成 T local（同 let-else / `!`），对 T 复用比较链（跳过 nil pattern）。**IR 约束**：仅 move 与 nil 比较可接 `T?`（ir.h:121），解包后操作数均为 T。
4. narrowing：命中臂用 `stbl_add_shadow_var` 把 subject 收窄为 T。
5. 穷尽：nil + 全部 T 成员（enum / int / bool）；`_` 覆盖 nil+T。
6. **限制**：optional 首版仅 value / nil / `_`，不支持 `is` / `in`（union / any 另议）。
7. 测试：`test_optional_switch.kl`（nil / 非 nil / 穷尽 / narrowing / 单次求值）+ 错误测试。

注意：phase 2 的整数多路优化作用于**解包后**的 T 比较链；optional 本身仍先做 nil 分裂。

</details>

<details>
<summary><b>🔜 #19 — `switch` 作为表达式（设计阶段）</b></summary>

形态（待定）：`let x = switch subj { case p1: e1, case p2: e2, _: e3 }`，每个臂是**表达式**而非语句块。

- **语义**：各臂表达式类型须可统一（公共类型 / 泛型推断）；**必须穷尽**（无 fall-through，缺臂报错）；无 terminal 语句要求（臂即值）。
- **语法**：新增表达式产生式（或复用 switch 块在表达式上下文）；`:` 后接 expr，臂间用 `,` 或换行分隔（需消解与语句 switch 的歧义）。
- **IR**：各臂求值到临时值，merge block 用 `OP_IR_PHI` 合并；subject 单次求值；`is` pattern 的 narrowing 在臂内生效。
- **触点**：`koala.y`、ast、`parse_switch`（表达式变体）、`emit_ir_switch`（phi 合并）、类型统一。
- **先出设计稿讨论，再实现**；与语句 switch 共享 pattern 校验。

</details>

<details>
<summary><b>🧪 测试、风险与里程碑（112.5）</b></summary>

- **构建 / 测试**：始终 `./build-debug.sh` + `./test-debug.sh`（安装到 `~/.local`；ninja-only 会因 stale release runtime 产生假 lit 失败）；valgrind 抽查。
- **错误测试约定**：用 void 函数（`parse_switch` 早退使 `is_terminal=0` 会级联 missing return）；FileCheck 用 `{{.*}}` 匹配 ANSI 颜色。
- **风险**：临界边（多 case 共享 body → 二分树天然每 case 独立边，规避）；enum 底层类型一致性；subject 单次求值；JIT 对 switch 的回落（2a 无新 op，JIT 自动支持分支）。
- **里程碑顺序**：Phase 2a（二分，✅ 已完成）→ Phase 2b（跳转表，暂不支持）→ Phase 3（optional）→ #19（表达式）。各阶段独立验收，保持现有 4 个 switch 测试与 optional 禁令（直到 Phase 3）始终绿。

</details>
</details>