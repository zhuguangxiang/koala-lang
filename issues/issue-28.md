# 28. enum 未实现 Equatable/Hashable/Printable，泛型约束检查失败

← 返回 [问题列表](issues.summary.md)

- [ ] 未修复 —— 方案 C 已讨论确认，按用户指示暂不实施，未改任何源码
- [ ] 待拍板：Printable 打印整数还是成员名

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
<summary><b>📋 方案 C 实施步骤（已讨论，未动手）</b></summary>

1. `include/parser/symbol.h`：`EnumSymbol` 加 `Vector bases`；`symbol.c` 的 `stbl_add_enum` 初始化、释放处（~227 行）`vector_fini`。
2. 新增 `enum_add_root_bases(EnumSymbol *, ParserModule *)`：从 `pm->builtin` 取 `Printable`/`Hashable`，用 `find_or_add_instance` 生成 `Equatable[E]` 实例。调用点：`parser.c` `_add_enum` 和 `read_klc.c` `load_enums`。注意 builtin 必须先加载（`init_parser` 里 `load_builtin_module` 先于用户包；编译 builtin 自身时 `pm->builtin = pm->stbl`，需防 trait 尚未声明，找不到就跳过）。`instance_ts` 走 `klass_type_spec` 全局 intern，跨模块指针同一，`type_spec_compatible` 顶部 `dst == src` 即命中。
3. `parser.c` `_diff_kind_compatible_check`：src 是 enum 时先 `_base_compatible(dst, &enum->bases)`，失败再回退 int64。`check_generic_bound` 不用改。值上转 trait 时 IR 源类型已是 int64，走现有 int64 itable 路径（需实测确认）。
4. `parser_expr.c` `parse_dot_inner` enum 分支：除 `name` 外支持 `hash`/`fmt`/`__eq__`/`__ne__`，方法到 int64 类符号表找，接收者按 int64 处理；`__eq__`/`__ne__` 参数必须是同一 enum，挡住 `c.__eq__(5)`。
5. 测试：新增 `test/test-run/test_enum_traits.kl`（本地/跨包 enum 的 `ut.assert_eq`/`assert_ne`、自定义 `[T : Equatable]`/`[T : Hashable]`/`[T : Printable]`、`Equatable[C]`/`Hashable`/`Printable` 变量、`c.hash() == int(c).hash()`，并入 test2.kl 场景）；test-kl 报错用例：`[T : Comparable]` 传 enum、`Equatable[D]` 接收 `C`、`c.__eq__(5)`。
6. 验证：`./build-debug.sh`、`./test-debug.sh`、`./build-std-koala.sh`；文档更新 `docs/Koala_TODO.md` §78、`docs/Koala_Design_Overview.md` §2.6。

</details>

<details>
<summary><b>❓ 待拍板：Printable 打印什么</b></summary>

运行时 enum 是 int64，泛型（类型擦除）、`any`、`Printable` 变量、`format("{}", e)` 拿到的都是 int64，打印整数；`assert_eq` 失败信息也是整数。

- **a（讨论中推荐）**：保持打印整数，与 `test_enum_basic.kl` 现有约定（"print shows the underlying int64 value"）一致，要名字显式 `c.name()`。
- **b**：打印成员名。运行时必须能识别 enum（单独运行时类型或装箱对象），等于回到设计文档「enum 是 final class 单例」方案，涉及 VM/IR/klc，**工作量大一个量级**。

</details>

<details>
<summary><b>⚠️ 既有漏洞（本次不处理，仅记录）</b></summary>

因 int64 回退，`let x Equatable[int] = C.RED`、`let y Comparable[int] = C.RED` 现在**都能通过**。

堵漏需去掉 enum→int64 隐式转换中 trait 目标的路径，会影响 `let i int = c` 等现有用法，**单独讨论**。

</details>

<details>
<summary><b>🕐 记录状态</b></summary>

2026-11-22 发现于 test2.kl（跨包 enum + ut.assert_eq）。方案 C（含 Hashable/Printable）已讨论确认，**按用户指示暂不实施，未改任何源码**，等待明确指令。

</details>