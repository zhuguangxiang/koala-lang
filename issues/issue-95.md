# unary 语义未整改：uint64 `~x` 得 int64、NEG/NOT tag 不一致、JIT 镜像分歧

← 返回 [问题列表](issues.summary.md)

- [ ] 部分完成 —— 语言侧已修（2026-11-14）；**JIT 侧镜像未做**

> 来源：`docs/Koala_TODO.md` §94（2026-11-13，语言侧已修 2026-11-14）
>
> JIT int 类型覆盖设计时排查 unary，确认 §75（unary 仍自动提升）至今未整改，并顺带核出若干精确机制问题。本条汇总存档；**JIT 暂不扩展 unary 覆盖**，待语言侧整改后再镜像。

---

### 详细描述

<details>
<summary><b>1️⃣ 实测症状（2026-11-13，debug 构建，解释器路径）</b></summary>

```kl
func f(x uint64) uint64 { return ~x }
func main() {
    let r = f(uint64(5))
    print(r, typeof(r))    // 实测：-6 <class 'int64'>
}
```

期望按 `number.kl` 声明 `uint64.__bitnot__() uint64`：应为 `18446744073709551610 <class 'uint64'>`。

实测 payload 是 `-6`（~5 的补码，正确），但**类型谎报为 int64**。与 §75 的 uint8 例子同源。

</details>

<details>
<summary><b>2️⃣ 精确机制（已核实，纠正一处早期误判）</b></summary>

- **`OP_INT_NOT` 保留源 tag**：`include/runtime/vm_ops.h:2660` `regs[rd].tag = regs[rs].tag;` —— **不是**写 INT64。早期「OP_INT_NOT 写 TAG_INT64」的判断有误，特此纠正。
- **`OP_INT_NEG` 强制 INT64**：`include/runtime/vm_ops.h:2674` `regs[rd].tag = TAG_INT64;` —— 与 NOT 不对称：NEG 抹掉源 tag，NOT 保留。
- **uint64 `~x` 得 int64 的根因不在 isel 选 opcode，而在前端结果类型**：§75 已记录 `parse_unary` 把 unary 结果类型硬编码为 int64/uint64/float64，不按 `number.kl` 声明走；`emit_ir_unary` 也不对操作数/结果做 cast。isel 的 `isel_lower_unary_bit_not`（`src/parser/backend/isel.c:1480`）/ `isel_lower_unary_neg`（`:1466`）只按 `TYPE_INT`/`TYPE_FLOAT` 分流、不区分符号，是上游硬编码的下游表现，**不是独立根因**。
- **声明侧**：`libs/std/builtin/number.kl` 中 `uint64` 声明 `__bitnot__() uint64`（:717），**未声明 `__neg__`**；窄类型（int8/16/32、uint8/16/32）两者都不声明（§75）。

</details>

<details>
<summary><b>3️⃣ JIT 侧镜像分歧（待 unary 整改后一并处理）</b></summary>

- 现 JIT 白名单含 `OP_INT_NEG/NOT`，lowering 统一用 `emit_store_int64`（写死 TAG_INT64）。对 NEG 与解释器一致；**对 NOT 不一致** —— 解释器保留源 tag，JIT 写 INT64。
- 当前被 int64-only 测试面掩盖（`bench/jit_diff.kl` 的 `f_unary` 结果随后参与算术，tag 被洗成 INT64，差分不可见）。一旦覆盖窄类型/uint 且 `~x` 结果直接被 `typeof`/`print` 观察，分歧即显现。
- **处理方向**：JIT 的 NOT 改为镜像「保留源 tag」（读 `[rs.tag]` 写回 `[rd.tag]`），或随语言侧 unary 整改统一结果类型后再定。**本阶段不动 JIT 代码。**

</details>

<details>
<summary><b>4️⃣ 解释器里窄类型 `~x` 的 tag 错误（2026-11-14 发现，已修）</b></summary>

irgen 的 `cast int8 → int64` 在运行期是空操作（§5「扩宽后的值保留自身类型」），值仍带窄 tag；`OP_INT_NOT` 再原样复制源 tag，于是结果没有变成声明的 int64：

```kl
func f(a int8)  { print(~a, typeof(~a)) }   // 解释器：-6 <class 'int8'>，应为 int64
func g(a uint8) { print(~a, typeof(~a)) }   // 解释器：18446744073709551610 <class 'uint8'>，应为 -6 int64
```

- JIT 打开时（默认阈值 1）结果正确，因为 JIT 的 NOT 写死 INT64，所以 `test_unary.kl` 没测出来；`KOALA_JIT=0` 可复现。NEG 写死 INT64，不受影响。
- **修复**：`OP_INT_NOT`（`include/runtime/vm_ops.h`）的结果 tag 为：源是 `TAG_UINT64` 时为 `TAG_UINT64`，其余一律 `TAG_INT64`，与声明一致。解释器与 JIT 结果相同（JIT 对 uint64 `~x` 仍写 INT64，见上面第 3 条）。
- 测试：`test/test-run/test_unary.kl` 加 `env KOALA_JIT=0 koala %s ++ --no-inline` RUN（要关内联，否则实参是常量，`~` 被折叠，运行期不执行 NOT）。

</details>

---

### 修复方案

<details>
<summary><b>✅ 已完成</b></summary>

- [x] 语言侧按 §75 整改：窄类型声明 `__neg__`/`__bitnot__`，unary 结果类型与操作数 cast 跟随声明。
- [x] uint64 不声明 `__neg__`（无符号没有负数），`-x` 编译报错。`~uint64(5)` 现为 `18446744073709551610 <class 'uint64'>`。
- [x] ~~NOT 保留的源 tag 就是结果类型~~：此判断有误，已修，见第 4 条。
- [x] 回归 `test/test-run/test_unary.kl`，补 uint64 `~x` 用例和 `test/test-kl/test_unary_neg_uint64.kl`。

</details>

<details>
<summary><b>🛠 未做</b></summary>

- [ ] JIT 侧按第 3 条镜像 NOT 的 tag 行为，再纳入 unary 覆盖。

</details>

<details>
<summary><b>🔗 关联</b></summary>

- §75（unary 仍自动提升）是本条的**上游根因**；§63（[#75](issue-75.md) trait 值上的运算符、移位与 unary）、§47.5（用户类上的 unary 待实现）同属 unary 整改面。
- 可能与记录在案的「uint64 高位打印为有符号」相关（tag 谎报为 int64 后按有符号打印），待验证。

</details>