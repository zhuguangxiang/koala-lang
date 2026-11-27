# 28. enum 未实现 Equatable/Hashable/Printable，泛型约束检查失败

← 返回 [问题列表](issues.summary.md)

- [x] 已修复 —— 每个 enum 自动实现 `Printable`、`Hashable`、`Equatable[E]`（无 `Comparable`）
- [x] 泛型约束：`[T : Equatable]` / `[T : Hashable]` / `[T : Printable]` 接受本地与跨包 enum
- [x] 值上转：`let p Printable = c`、`let h Hashable = c`、`let e Equatable[C] = c` 通过
- [x] 方法：`c.hash()` / `c.fmt(f)` 复用 int64 实现，`c.name()` 保持原样；打字后仍报错
- [x] 已拍板：Printable 打印整数（方案 a），成员名显式 `c.name()`
- [x] 附带完成：class 手写 `Printable`/`Hashable` 不再被重复追加，`bases` 前两位顺序不变
- [x] 回归：`test/test-run/test_enum_traits.kl`、`test_enum_traits_shadow.kl`、`test_class_root_traits_dedup.kl`，报错用例并入 `test_enum_errors.kl` / `test_enum_name_errors.kl`，新增 `test_enum_base_syntax_error.kl`
- [x] 验证：`./build-debug.sh`、`./test-debug.sh`（639/639）、`libs/std/build-and-test.sh`（含 `KOALA_JIT=0`）

> 已知遗留（另立 issue）：`let x Comparable[int] = C.RED` 仍能通过 → [issue-134](issue-134.md)；
> `Equatable[T]` 类型变量不能参与 `==`（class 同样如此）→ [issue-135](issue-135.md)

---

### 详细描述

<details>
<summary><b>🔍 现象</b></summary>

enum 传给 `ut.assert_eq`（`[T : Equatable]`）报 `type argument 'Proto' does not satisfy constraint on type parameter 'T'`。

gdb 断在 `type_spec_compatible`（parser.c:896）：

- dst = `Lstd/builtin.Equatable<Etest_pkg/enum_cross.Proto;>;`（TYPE_KLASS）
- src = `Etest_pkg/enum_cross.Proto;`（TYPE_ENUM）
- 返回 0

</details>

<details>
<summary><b>🧪 最小复现（test2.kl）</b></summary>

```
import "test_pkg/enum_cross" as zz
import "std/ut"
func main() {
    let p = zz.Proto.TCP
    let q = zz.Proto.TCP
    ut.assert_eq(p, q)
}
```

</details>

<details>
<summary><b>🧠 根因</b></summary>

`check_generic_bound` 的 enum 分支（parser_expr.c:1862）调 `type_spec_compatible(Equatable[Proto], Proto)`；两边 kind 不同走 `_diff_kind_compatible_check`（parser.c:593），**enum 被回退成 int64 再比**，而 int64 的 base 只有 `Equatable[int64]`，不是 `Equatable[Proto]`，失败。

本质是 **`EnumSymbol` 没有 bases**：class 有 `add_root_equatable`/`add_root_traits` 自动补根 trait，enum 没有对应路径。

enum 的 `==`/`!=` 本身**已支持**（parser_expr.c:4500 专门处理），只是泛型约束不认。

</details>

<details>
<summary><b>📊 实测现状（2026-11-22，koala 直接跑）</b></summary>

| 用法 | 结果 |
|---|---|
| `ut.assert_eq(p, q)`（T = Proto） | ❌ 约束不满足 |
| `let e Equatable[C] = C.GREEN` | ❌ 类型不匹配 |
| `c.hash()` | ❌ `enum 'C' has no members` |
| `let p Printable = C.GREEN; print(p)` | ✅ 但打印 `1`（走 enum→int64 回退） |
| `let h Hashable = C.GREEN` | ✅ 同样走 int64 回退 |
| `dict[Proto, int]`、`format("{}", e)`、`func show[T](x T)` | ✅ 打印的都是整数 |

运行时 enum 就是 int64（irgen `klr_lower_type`），int64 已实现这三个 trait，所以**缺的只是编译期**：enum 声明自己实现这些 trait + enum 值能调这些方法。

</details>

---

### 修复方案

<details>
<summary><b>🛠 方案讨论（C 为已确认方向）</b></summary>

- **A**：只在 `check_generic_bound` 的 enum 分支补规则（std/builtin 的 `Equatable[E]`/`Hashable`/`Printable` 视为满足）。改动最小，但**只覆盖泛型约束**，值上转和方法调用仍缺。
- **B**：在 `_diff_kind_compatible_check` 放行 enum→trait。会同时允许 `let x Equatable[Proto] = p` 这类值上转，IR 里 enum 是 int64，trait 值需要装箱/itable，**没有对应实现，容易运行时崩溃**。
- **C（已确认方向，含 Hashable/Printable）**：编译期给 enum 固定 bases `Printable`、`Hashable`、`Equatable[E]`（与 class 自动根 trait 一致；**故意不加 `Comparable`**，enum 仍不能比大小），运行时方法全部复用 int64 实现。bases 由 enum 自身决定，**不写进 klc**，本地和导入 enum 各自在创建时生成。**klc 格式和 VM 不用改。**

</details>

<details>
<summary><b>📋 方案 C 实施步骤（已实施）</b></summary>

1. `include/parser/symbol.h`：`EnumSymbol` 加 `Vector bases`；`symbol.c` 的 `stbl_add_enum` 里 `vector_init_ptr`，`SYM_ENUM` 释放处 `vector_fini`。
2. 新增 `enum_add_root_bases(EnumSymbol *, HashMap *stbl, ParserModule *pm)`（`parser.c`，紧接 `add_root_equatable`）：从 `pm->builtin` **直接取符号**（不按名字在当前作用域查找，所以用户同名 trait 遮不住），`Printable`/`Hashable` 取 `instance_ts`；`Equatable` 用 `find_or_add_instance(stbl, Equatable, [sym->ts], pm)` 生成 `Equatable[E]`。顺序固定 `Printable, Hashable, Equatable[E]`。`pm->builtin == NULL` 时直接返回（编译 std/builtin 自身、trait 尚未声明）。调用点：`_add_enum` 末尾、`read_klc.c` 的 `load_enums` 末尾。
3. `parser.c` `_diff_kind_compatible_check`：src 是 `TYPE_ENUM` 时先 `_base_compatible(dst, &enum->bases)`，失败再回退 int64。`check_generic_bound` 未改。
4. `parser_expr.c` `parse_dot_inner` 的两个 enum 分支：新增 `parse_enum_root_method()`，`hash`/`fmt` 到 int64 类符号表取方法符号后**保留 dot**（`exp->sym`/`exp->ts` 换成 int64 的实现），由 `emit_ir_dot` 把接收者作为 `self` 交给 `emit_ir_call`，不再改写实参列表。`__eq__`/`__ne__` 无需处理：显式调用本来就被 dunder 规则挡住。
5. 附带修：`add_root_traits` 增加 `take_declared_base()`，class 手写的 `Printable`/`Hashable` 从原位取出后放回队首，`bases` 仍是 `[Printable, Hashable, ...用户 trait..., Equatable[K]]`（`compute_pip` 走 `bases[0]`、`add_root_equatable` 用 `root_traits = 2` 的假设不变）。klc 里写出的 bases 因此不再有重复项，**std 库需要重编**。
6. 测试与验证见上方清单；`docs/Koala_Design_Overview.md` §2.6 补了「已实现」说明。

</details>

<details>
<summary><b>❓ Printable 打印什么（已拍板：方案 a）</b></summary>

运行时 enum 是 int64，泛型（类型擦除）、`any`、`Printable` 变量、`format("{}", e)` 拿到的都是 int64，打印整数；`assert_eq` 失败信息也是整数。

- **a（已采用）**：打印整数，与 `test_enum_basic.kl` 现有约定（"print shows the underlying int64 value"）一致，要名字显式 `c.name()`。
- **b（未采用）**：打印成员名。运行时必须能识别 enum（单独运行时类型或装箱对象），等于回到设计文档「enum 是 final class 单例」方案，涉及 VM/IR/klc，**工作量大一个量级**。

</details>

<details>
<summary><b>⚠️ 既有漏洞（本次不处理，另立 issue）</b></summary>

因 int64 回退，`let x Equatable[int] = C.RED`、`let y Comparable[int] = C.RED` 现在**都能通过**。

堵漏需去掉 enum→int64 隐式转换中 trait 目标的路径，会影响 `let i int = c` 等现有用法，**单独讨论** → [issue-134](issue-134.md)。

</details>

<details>
<summary><b>🕐 记录状态</b></summary>

2026-11-22 发现于 test2.kl（跨包 enum + ut.assert_eq）。方案 C（含 Hashable/Printable）讨论确认。

2026-11-27 实施完成：enum 的 bases 由编译器固定生成（`Printable, Hashable, Equatable[E]`，不写 klc、导入时重生成）、`_diff_kind_compatible_check` 认 enum 的 bases、`hash`/`fmt` 复用 int64 实现；顺带修掉 class 重复追加 `Printable`/`Hashable` 的问题。全量回归 639/639，std 库（含 `KOALA_JIT=0`）全通。

</details>