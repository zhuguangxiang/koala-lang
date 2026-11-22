# phi coalescing 把常量当空区间，导致两个循环的累加器被合并到同一个 local

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— **只在 `--ssa` 下出现**；默认 VM 路径全部正确

> 来源：`docs/Koala_TODO.md` §92（2026-11-12，待修）
>
> 发现于「kw 传位置参数」的回归验证，**与 kw 无关**：全部复现都只用纯位置实参和字面量。

---

### 详细描述

<details>
<summary><b>1️⃣ 症状：同一函数里两个 for-each 循环，第一个累加器的值被清零</b></summary>

最小复现（`a`、`b` 都以 `0` 初始化）：

```kl
func main() {
    var a = 0
    for i in range(3) { a = a + i }
    print(a)          // VM: 3    SSA: 0   ← 错
    var b = 0
    for i in range(3) { b = b + i }
    print(b)          // VM: 3    SSA: 3
}
```

**不是 range 的问题**：把两个 `range(3)` 都换成 `[0, 1, 2]` 一样错；只换其中一个也错。最初误以为是「连续 range 循环」，实测与迭代器种类无关。

</details>

<details>
<summary><b>2️⃣ 触发条件矩阵（VM 全部正确，下表只列 SSA）</b></summary>

| 变体 | 第一个累加器初值 | 第二个初值 | 循环数 | SSA 输出 | 结论 |
|---|---|---|---|---|---|
| 基准 | `0` | `0` | 2 | `0 3` | 错 |
| 两个 `var` 都提到最前 | `0` | `0` | 2 | `0 3` | 错，与 `var` 位置无关 |
| 只改第二个初值 | `0` | `7` | 2 | `0 10` | 错，**只有第一个初值起作用** |
| 只改第一个初值 | `5` | `0` | 2 | `8 3` | 对 |
| 两个都非 0 | `5` | `7` | 2 | `8 10` | 对 |
| 同一个累加器贯穿两个循环 | `0` | — | 2 | `3 9` | 对 |
| 两个循环共用一个累加器、末尾才 print | `0` | — | 2 | `6` | 对 |
| 换成两个 `while` 手写循环 | `0` | `0` | 2 | `3 3` | 对 |
| 三个循环三个累加器（list 字面量） | `0` | `0` | 3 | `3 3 3` | 对，块布局变了 |

**更硬的症状**：第一个循环用 list、第二个用 range 时**直接编译崩溃**：

```kl
func main() {
    var a = 0
    for i in [0, 1, 2] { a = a + i }
    print(a)
    var b = 0
    for i in range(3) { b = b + i }
    print(b)
}
```

```text
cgen.c:1426: Why goes here?      // fill_mach_insn() 的 default: UNREACHABLE()
```

</details>

<details>
<summary><b>3️⃣ 直接证据：de-ssa 之后 `b` 的 local 声明消失，`b` 的 phi copy 落在 print 之前</b></summary>

`koalac --ssa --dump=ssa` 的 de-ssa 输出（节选，基准复现）：

```text
%bb0(entry):
      %lc.i.phi.0 = local int64
      %lc.i.phi.0 = local int64
      move %lc.i.phi.0 int64, 0 int64
      %lc.a.phi.0 = local int64
      move %lc.a.phi.0 int64, 0 int64      // 只有 a 的 local，b 的没有了
      jmp label %bb1
...
%bb3(loop-end):                            // 第一个循环的出口
      move %lc.i.phi.0 int64, 0 int64
      move %lc.a.phi.0 int64, 0 int64      // ← b 的 phi copy，把 a 清零了
      %3 = build_intern @tuple, %lc.a.phi.0 int64
      call @print ...                    // 于是打印 0
      jmp label %bb4
...
%bb5(loop-body):
      %5 = add %lc.a.phi.0 int64, %lc.i.phi.0 int64   // 第二个循环也在用 a 的 local
```

SSA 层本身是**对的**（`%a.phi.0` 与 `%b.phi.0` 是两个不同的 phi，各自 `phi 0, %backedge`）。**错在 de-ssa / coalescing**：`b` 被并进 `a` 的 congruence class。

对照：把初值改成 `5` / `7`（正确的那一档），de-ssa 输出里 `%lc.a.phi.0` 与 `%lc.b.phi.0` 是**两个独立 local**，`move %lc.b.phi.0, 7` 同样落在 `%bb3`，但因为不共享槽位所以无害。

</details>

---

### 修复方案

<details>
<summary><b>🧠 根因推断（证据充分，未施工验证）</b></summary>

- `src/parser/opt/phi_coalescing.c:108` 的 `_build_interval()` 给常量 `start = 0`，`:77` 的 `_build_interval_end()` 对常量直接 `return 0` —— 常量的活跃区间是**空区间 `[0, 0)`**。
- `:181` 的 `_intervals_overlap()` 是 `a->start < b->end && b->start < a->end`。代入常量（`start == end == 0`）恒为 false，**常量与任何区间都不干涉**。
- `:386` 的 `klr_coalesce_run_analysis()` 对每个 phi 的**每个** incoming operand 都调 `_try_merge()`，常量 operand 也不例外。于是「所有以同一个常量对象为 incoming 的 phi」都并到那个常量的 class 里。`0` 是共享的同一个 `KlrValue`，所以 `a.phi.0` 和 `b.phi.0` 被判定可合并 —— 而它们的真实活跃范围在 `%bb3` 的 print 处是重叠的（`b` 定义时 `a` 还活着）。
- 这正好解释上面矩阵的全部现象：初值不同 → 不同常量对象 → 不合并 → 正确；只有第一个初值为 0 也照样错（`b` 并进零 class 即可）；三个循环时块布局变化使线性区间真的重叠，反而被拦住。
- **附带问题**：线性区间模型 `[def_pos, last_use_pos)` 按指令序号铺平，**不考虑 CFG**，也不把「phi copy 实际插在前驱块的什么位置」计入 incoming 值的活跃范围。即使修掉常量这一条，循环出口块里带其他代码时仍可能有别的漏判。

</details>

<details>
<summary><b>🛠 方向（未定，二选一）</b></summary>

- 给常量算出真实区间（延伸到每个 phi copy 的落点，即 `pred->last_pos`）；
- **干脆不让常量参与 coalescing** —— 常量的 phi copy 本来就该是一条立即数 `move`，不需要和 phi 共享槽位。**后者改动面更小。**

</details>

<details>
<summary><b>📌 现状与规避</b></summary>

- 全量 **497/497 通过**，现有用例没有覆盖到这个形状。
- `test/test-run/test_kw_range.kl` 因此**每个函数只放一个 range 循环**，文件头注释记了原因。这是**规避，不是修复**；本节修好后可以把那些循环合回一个函数。
- 修好后应新增正向用例，至少覆盖上面矩阵里「错」的三档 + 崩溃那一档，两条 RUN（默认 + `--ssa`）。

</details>

<details>
<summary><b>🔗 关联</b></summary>

同在 `phi_coalescing.c` 的已修问题：嵌套 for-each + if 的 `move` 放错块（本轮之前已修）。

另有一条待议的 de-ssa critical-edge 问题，与本节同属「phi copy 落点」这一类，宜一并考虑。

</details>