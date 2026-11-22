# `+` on `bytes` / `str` treated as commutative：静默错值（已修；47.3 待议、47.5 待实现）

← 返回 [问题列表](issues.summary.md)

- [ ] 部分完成 —— 主体 bug 已于 2026-11-02 修复（47.2）；**47.3 的 `lsra` 兜底方案待议**，**47.5 用户类上的 unary 运算符待实现**

> 来源：`docs/Koala_TODO.md` §47（2026-11-02，fixed；47.3 to discuss, 47.5 to implement）
>
> 起因：新增 `bytes.__add__`（让 `a + b` 能在 `bytes` 上用）并替换 `libs/koala/http/request.kl` 里手写的 `_concat` 时发现。下面两个 bug 都是**静默**的 —— 无诊断、不崩溃，只是值不对 —— 而且都同样命中 `str`。

---

### 详细描述

<details>
<summary><b>🐞 两个静默错值</b></summary>

**1. `normalize` 在左操作数是常量时交换了两个操作数**（期望 `b"abcd"`，实际 `b"cdab"`）：

```kl
func main() {
    let q = b"cd"
    print(b"ab" + q)
}
```

`--dump=no-opt-ir` 是对的（`%0 = add b"ab" std/builtin.bytes, %q`）；`--dump=ir` 显示 `normalize (Iter 1)` 把它改写成 `%0 = add %q, b"ab"`。`src/parser/opt/normalize.c:12` 的 `can_swap()` 只对 `TYPE_STR` 返回 0，所以 `bytes` —— 以及任何自带 `__add__` 的类 —— 落到 `normalize.c:27` 的交换分支。

**2. `lvn` 把 `a + b` 与 `b + a` 编号成同一个值**（期望 `abcd` 然后 `cdab`，实际两次 `abcd`）：

```kl
func main() {
    let p = "ab"
    let q = "cd"
    print(p + q)
    print(q + p)
}
```

`src/parser/opt/lvn.c:154` 的 `lvn_is_commutative()` 包含 `OP_BINARY_ADD`，而 `lvn.c:208` 在不看操作数类型的情况下规范化哈希键的操作数顺序，于是第二个 `add` 被 RAUW 成第一个。`bytes` 操作数同样中招；两个常量任意顺序（`b"ab" + b"cd"` 然后 `b"cd" + b"ab"`）也打印两次 `b"abcd"`；循环里亦然。

两者都**只影响函数内**：让 `+` 经过一次调用边界就得到正确答案，`test/test-ut/test_bytes.kl` 的新测试正是这样绕开的（`add_bytes(a, b)`，原因写在注释里）。`-`、`/`、`%`、`<<`、`>>` 不受影响（`a - b` / `b - a` 与 `a / b` / `b / a` 打印四个不同值）；`koala/http/request.kl` 是安全的，因为它的左操作数是调用（`head.to_bytes()`）而不是常量。

</details>

<details>
<summary><b>🔍 根因（47.1）</b></summary>

`src/parser/irgen.c` 把每个 binary / unary 运算符都下降到同一个 `OP_BINARY_*` / `OP_UNARY_*` 指令，无论操作数类型，而那些 pass 把这些指令当成**纯数值运算**。这只对数字成立。在 `str` / `bytes` / `list` 上 `+` 是拼接（不交换，且 `list` 结果是全新的可变对象），在用户类上运算符是方法调用（`__add__`），可能有副作用。除上面两例外，同一假设还导致：

3. **`lvn` 合并两个同序 `a + b`**：list 上 `l1 = a + b; l2 = a + b; l1.append(9)` 两处都打印 `[1, 2, 3, 9]`；在 `__add__` 自增计数器的用户类上，`c + d` 两次打印 `1 1` 而不是 `1 2`。
4. **`dce` 删掉用户类上未被使用的运算符**：`c + d` 作语句，再 `print(c + d)`，打印 `1` 而不是 `2` —— 第一次 `__add__` 调用被丢掉。

`lvn` 的键 `(op, lhs, rhs)` 对纯运算足够；缺的是**准入检查**，所以下面每个修法都是逐 pass 的类型白名单。

---

### 修复方案

<details>
<summary><b>✅ 修法（2026-11-02 已完成）</b></summary>

- `normalize.c` 的 `can_swap()`：白名单 `TYPE_INT` / `TYPE_FLOAT` / `TYPE_BFLOAT16`，而不是只排除 `TYPE_STR`。
- `lvn.c` 的 `lvn_is_value_insn()`：只有当每个操作数都是 int / float / bfloat16 / bool 时，才给 `OP_BINARY_*` / `OP_UNARY_*` 编号。`str` 被**有意**排除，这样交换律的键重排不需要额外类型检查。
- `dce.c` 的 `is_value_insn()`：在 `has_side_effect()` 里，运算符只在 int / float / bfloat16 / bool / str / optional（`x != nil`）上可删；bytes / list / 用户类上保留。
- `backend/lsra.c` 的 `unused_needs_reg()`（现在在 `backend/liveness.c`，§105）：一个结果未使用但被保留的 `OP_NUM_*` 以前拿不到区间，被发射成 `num.add r255, ...`（VM 断言 `rd < max_regs`）；现在 16 个 `OP_NUM_*` code 都拿到一个单位置区间。调用仍然没有（`vreg -1` = 无返回寄存器，`call flg=0, #1`）。**这一条只是兜底**，见下。

代价：`str` 运算符不再被值编号，bytes / list 上未使用的运算符不再被删除。回归测试：`test/test-run/test_normalize_add_order.kl`、`test_lvn_add_order.kl`、`test_lvn_add_object.kl`、`test_dce_klass_op.kl`。`test/test-ut/test_bytes.kl` 里的 `add_bytes(a, b)` 绕过写法现在可以改回普通 `+`。

</details>

<details>
<summary><b>⚠️ 47.3 `lsra` 为未使用的 `OP_NUM_*` 分配寄存器（兜底，待议）</b></summary>

上面 `unused_needs_reg()` 的改动能用，但**不是好设计**，以后要重看。

**为什么非要寄存器**：`include/runtime/vm_ops.h` 里每个 `OP_NUM_*` handler 都解码 `rd`，用 `CHECK_REG_ID(rd)`（`src/runtime/eval.c:110`，`ASSERT((id) < max_regs)`）检查，并无条件写 `regs[rd] = ret`。未分配的值 `vreg -1`，编码为 `rd = 255`：

- debug 构建：断言失败（`_eval_frame: Assertion '(rd) < max_regs' failed`）；
- release 构建：`NDEBUG` 下 `ASSERT` 是空操作（`include/common/common.h:84`），于是 VM 写到帧寄存器之外的 `regs[255]` —— **静默内存破坏**。

`OP_CALL` 没这个问题，因为它有真正的「无结果」编码：cgen 对 `vreg -1` 打印 `call flg=0, #1`，VM 只在 `rd != 0xFFF` 时写。

**兜底不令人满意的原因**：

- 那 16 个 opcode 的白名单必须跟随用户类运算符可能下降到的每个 opcode（比如漏了 47.5 未来的对象 unary op）；
- 它为一个没人读的值花掉一个寄存器（可能还多一个 `nlocals`）；
- 「没有使用的值不分配寄存器」这条规则现在有了例外，而且例外住在 `lsra` 里，离造成它的 `dce` 决策很远。

**待议选项**：

1. 给 `num.*` 一个「丢弃结果」编码，像 `call` 一样（如 `rd == 255` 跳过写入）：cgen 加 16 个 `OP_NUM_*` handler，并在热路径上多一次测试；
2. 在 `isel`（或 `irgen`）把用户类上的运算符下降成**真正的方法调用**，这样未使用的情形复用无结果的 `call` 形式；这也契合 47.1 的「运算符就是方法调用」观点；
3. 保留兜底，但让它从**类型**推导而不是从 opcode。

**选项 2 是 [#113](issue-113.md)（§49）倾向的**：它也正是让 `__add__` 上的 `pure` 声明能决定调用可否被删除或合并的前提。

</details>

<details>
<summary><b>📝 47.4 `dce` 与 `lvn` 白名单为何不同</b></summary>

两者**有意**不同：`dce` 只需要「无副作用」，所以 `str` / optional 可删；`lvn` 还需要「相同输入给出相同、可互换的值」，这就排除了 `str`（`+` 不交换）和任何对象结果。

</details>

<details>
<summary><b>⬜ 47.5 用户类上的 unary 运算符（待实现）</b></summary>

`__neg__` / `__bitnot__` / `__not__` 都在 `parser_expr.c` 的 dunder 表里（`-c`、`~c`、`!c`），但检查器在用户类上拒绝它们：

```kl
class Counter {
    var n int
    func __init__() { n = 0 }
    pub func __neg__() int {
        n = n + 10
        return n
    }
}

func main() {
    let c = Counter()
    -c
    print(-c)    // expected 20
}
```

报 `unary '-' operator requires int or float type.`（`parser_expr.c` 的 unary 检查；那里带着一句 `// TODO: check _neg__ exists or not`）。`+`、`~`、`!` 有同样的 int / float / bool-only 检查。实现时 unary 路径必须端到端对齐 binary 路径：

- `irgen.c` 把 `-x` 下降为 `OP_UNARY_NEG`，`~x` 为 `OP_UNARY_NOT`，`!x` 为 `OP_LNOT`，`+x` 什么都不发（no-op，没有 `__pos__`）。
- `backend/isel.c`：`isel_lower_unary_neg()` 在非 int / float 类型上直接 `UNREACHABLE()`，`isel_lower_unary_bit_not()` 断言 int；两者都需要对象路径（一个类似 `OP_NUM_NEG` 的指令，或者一次调用）。
- `lvn` / `dce`：`OP_UNARY_NEG` / `OP_UNARY_NOT` 已经走 `lvn_is_value_insn()` / `is_value_insn()`，所以用户类操作数既不被编号也不被删除。**`OP_LNOT` 没有**：`dce` 的 `has_side_effect()` 对它有返回 0，于是用户类上未使用的 `!c` 会被删掉 —— 需要同样的白名单分支。
- `liveness.c` 的 `unused_needs_reg()`：加上对象 unary 的 code，否则被保留未使用的 `-c` 会写 `r255`。
- 测试：上面的例子放进 `test_dce_klass_op.kl`（未使用的 `-c`，然后 `print(-c)` 打印 `20`）。

</details>

<details>
<summary><b>🔗 关联</b></summary>

- [#113](issue-113.md)（§49）：运算符副作用与 `pure` 声明，倾向选项 2（运算符下降为方法调用）。
- [#73](issue-73.md)（§37）：可变字面量与 const pool 的遗留项。
- §75（unary 仍自动提升）已修，与 47.5 同一条 unary 链路。
- §105 `backend/liveness.c` 的重构（`lsra.c` 里的 `unused_needs_reg()` 已迁址）。

</details>
</details>