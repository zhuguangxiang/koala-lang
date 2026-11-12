# Koala TODO

> 记录未完成实现与待定设计：泛型数值协议（§1–4、§7–8）、语言手册（§5）、
> 版本计划与路线图（§6）、容器协议相关（§9、§11、§12）、Truthiness 设想（§10）、
> 数值 cast 补全（§13，已完成转语义 / 测试基线记录）、from_str 解析 API（§14，设计定案未实现）、
> 测试揭示的 WIP 特性（§15，2026-10-02 复核已消解）、声明-实现欠账清单（§16）、
> `parse_binary` 具名常量通道（§17）、list 接口全面核对与主流语言对齐（§18）、
> 退出码模型（§19，panic 退出码与 main 返回值语义）、float 的 NaN/Inf 构造途径（§20，已修复）、
> `str(float)` 16 位有效数字缺陷（§21，已修复）、数值构造器接收 `str` 的语义（§22，待定，讨论中）、
> datetime 固定偏移时区（§23，待设计）、运算符按操作数类型重载以支持同类相减（§24，编译器级，待讨论）、
> Koala std/能力包 vs Python 标准库的场景×热度对比（§25，现状盘点，非待办）、
> `build_intern` 操作数个数溢出 `nargs:8` 后前端缺诊断（§26，待施工）、
> 全常量 dict 字面量直接作 `print` / `format` 实参时报 `modobj.c: Not Yet Implemented`（§27，待修）、
> os.run 子进程管道大 payload「先写后读」死锁 + Process/Raw* 无 finalizer 致 fd 泄漏与 zombie（§28，待修）、
> `is` / `as` 运算符语义评估与遗留项（§29，2026-10-26 评估）、let-else and `panic()`-as-terminal leftovers (§30, closed)、
> 泛型 bound 实参未转接口：泛型自由函数与泛型类 `__init__`（§34，待修，用例已留档为
> `test/test-run/test_intf_generic_bounds.kl.disabled`）、
> mutable literals and the const pool: leftovers (§37, open), compiler issues found while adding
> fs/os APIs (§38, open; its `list.sort()` item is fixed 2026-11-08), signal handlers in std/os (§39, waits for the concurrency API),
> open items of the `@specialized` IR specialization (§40, to fix later), `koala/re` vs Python's
> `re`: what is not aligned (§41, to do / to discuss), indexing an instance of a user class aborts the
> compiler (§42, to fix), `<<=` / `>>=` abort the compiler (§43, fixed), an empty list of tuples
> cannot be created (§44, to fix), string literals have no `\u` / `\U` escapes and keep unknown
> escapes silently (§45, to discuss), language issues met while writing `koala/re` (§46),
> operators on non-number types treated as pure number ops by `normalize` / `lvn` / `dce` (§47,
> fixed; the `lsra` stop-gap for an unused `num.*` is to discuss, unary operators on a user class
> are still rejected, to implement),
> `bytes` / `str` API surface and union literal narrowing: leftovers after the `in` refactor
> (§48, to decide / to implement), side effects of operators and an explicit `pure` declaration
> (§49, to design), a bare `{ ... }` block statement is never executed (§50, fixed),
> `break` / `continue` look for a loop past a function boundary (§51, latent, fix with closures),
> `BufReader.read_line` keeps the '\r' of a CRLF line (§52, to decide), reading an uninitialized
> local gives a garbage value: no definite-assignment check (§53, to fix), class and trait names used
> as values need consistent errors (§54, partially checked, to fix), `sealed trait`: a closed set of implementers
> (§55, to design), returning a value as `any` (§56, fixed), no check that a class method matches its
> trait (§57, postponed), `obj["key"]` on a plain class fails inside the compiler (§58, fixed),
> nullable generic elements do not compile (§59, fixed), a tuple type argument asserts in the compiler
> (§60, fixed), a field named `f` conflicts with the generated `fmt` (§61, fixed), `koala/redis`: tests that wait for a
> mock annotation, TLS and other leftovers (§62, to do; the TLS items fixed 2026-11-08), operators on a trait value: unary operators
> and edge cases (§63, to fix), shift semantics: decided and implemented, leftovers (§64), small
> compiler and runtime issues found on the way (§65, to fix), a narrow number type does not satisfy
> `T: Comparable` when `T` is inferred (§66, fixed), a generic bound specializes to a type with
> no symbol (§67, fixed), a tuple as a type argument in a constructor call (§68, to fix), an
> external class without a public `__init__` (§69, to fix), a repeated call with explicit type
> arguments has no `make_intf` (§71, to fix), `int.hash()` differs through a trait (§72, fixed),
> comparing constants of a narrow float type fails in the backend (§73, to fix), `bfloat16` cannot be
> constructed (§74, to fix), unary operators still promote automatically (§75, to fix), `T` is not
> inferred through a `Box[T?]` parameter (§77, to fix), `enum` as sugar for a final class (§78,
> planned), native handles of `koala/http`, `koala/tls` and `koala/sqlite3` have no finalizer (§79, waits for a
> free path), `koala/http` leftovers (§80, to do), `koala/tls` leftovers (§81, to do), `as!`: a forced cast for a
> value known to have the type (§82, to discuss), `koala/sqlite3` leftovers (§85, to do), a function with a return type can end
> without `return` (§86, fixed), `koala/readline` leftovers (§87, to do).
> 已打通部分见 `Koala_Design_Overview.md` 第 3 节：十六件二元运算符的 IR 下降链路
> （IR 协议指令 → KLR `num.*` → 字节码 `OP_NUM_*`）已实测完整。
>
> 更新日期：2026-11-08 (§87: the signal keys outside the line)；2026-11-08 (§87: Ctrl-Z / Ctrl-\ fixed)；2026-11-08 (added §87: `koala/readline` leftovers; §25.4: the interactive-input row)；2026-11-08 (§86 fixed: missing returns are reported)；2026-11-08 (added §86: no missing-return check)；2026-11-08 (§25.4: `koala/shlex` in the command-line row)；2026-11-08 (added §85: `koala/sqlite3` leftovers; §79: `koala/sqlite3` handles; §25.4: the database row)；2026-11-08 (added §82: `as!`, a forced cast; §62: the TLS items fixed; §25.4: the network and command-line rows
> updated; added §79: native handles without a finalizer; added §80: `koala/http` leftovers; added
> §81: `koala/tls` leftovers)；2026-11-07 (added §78: the plan for `enum`; added §77: `T` is not inferred through `Box[T?]`; §72 fixed; added §75: unary operators still promote; added §73: comparing narrow float constants; added §74: `bfloat16` constructor; §66, §67 fixed; §57: returns are invariant, covariant returns removed; added §71: repeated explicit type arguments; added §72: `int.hash()` through a trait; added §68: a tuple type argument in a constructor call; added §69: an external class without a public `__init__`; added §66: narrow number types and `T: Comparable`; added §67: a generic bound with no symbol; corrected the rules of §56 and §57; §34, §56, §58-§61 fixed)；2026-11-06 (§43 fixed; §1 / §7: `OP_NUM_*` bitwise handlers complete; added §64: shift semantics; added §65: small issues; §19: a test depends on exit code 0; added §63: operators on a trait value, shifts and unary operators; added §62: `koala/redis` tests that wait for a mock annotation, and TLS)；2026-11-04 (added §55: `sealed trait`; added §56: a value converted to `any` has no `make_intf`; added §57: no check that a class method matches its trait; added §58-§61: correct code that fails to compile)；2026-11-03 (added §54: a type name used as a value crashes the backend; fixed alongside: a
> keyword trait type `list[any]()` no longer asserts in `parse_type`)；2026-11-03 (added §53: no definite-assignment check, an uninitialized local reads garbage)；
> 2026-11-03 (added §52: `BufReader.read_line` and the '\r' of a CRLF line, plan recorded, to
> decide)；2026-11-02 (§50 fixed: a bare block statement only opens a scope and emits in place)；2026-11-02 (added §51: `find_loop_scope()` does not stop at a function / anonymous scope, latent
> until closures exist)；2026-11-02 (added §50: a bare `{ ... }` block statement is lowered to an unreachable basic
> block, found while adding "last statement before `}` may drop its `;`")；2026-11-02 (added §49: operators are not assumed side-effect free; direction is a
> compiler-checked `pure` modifier with "removable" / "mergeable" levels, to design)；2026-11-02 (§47 fixed: a type whitelist in `normalize` / `lvn` / `dce`, and `lsra` gives a kept
> `OP_NUM_*` a register, 4 regression tests; §47.3 the `lsra` stop-gap is to discuss, §47.5 unary operators on a user class are left to implement)；2026-11-02（新增 §47：`+` 在 `bytes` / `str` 上被 `normalize` / `lvn` 当作可交换运算——
> `b"ab" + q` 被交换成 `q + b"ab"`，且同一函数内 `a + b` 与 `b + a` 被值编号合并成同一个值；
> 两处根因已定位到 `normalize.c:can_swap()` 与 `lvn.c:lvn_is_commutative()`，用例与修法均已记录，未修；
> 起因是把 `bytes._concat` 换成 `+`，见 `libs/koala/http/request.kl`；同日新增 §48：`bytes` / `str`
> 收尾清单——`in` 支持 `uint8 | bytes` 已落地（`contains_bytes` 不再需要），余下 8 项 API 待定、
> 6 项 union 字面量收窄缺口、3 项待修（含实测：`var` / 字段不能写 union 类型、trait 方法签名不做
> 一致性校验、`str * -1` 断言崩溃），以及 `kl-fmt` 重排注释内代码、`dist/*.klc` 提交策略与提交切分);
> 2026-10-30（新增 §34：泛型 bound 实参未转接口——泛型自由函数与泛型类 `__init__`，
> 用例留档为 `test/test-run/test_intf_generic_bounds.kl.disabled`；同批已修的接口槽位三 bug
> 见 §34.4，回归测试为 `test/test-run/test_intf_{ext_inherit,local_inherit,optional}.kl`）; 2026-10-28 (§30 closed: `unreachable()` is terminal; no `unreachable` after `panic()`, to keep code size); 2026-10-27 (added §30: let-else is implemented); 2026-10-26（新增 §29：`is` / `as` 语义评估与遗留项）；2026-10-25（新增 §26：`build_intern` 操作数个数溢出 `nargs:8` 后前端缺诊断，待施工；§27：全常量 dict 字面量直接作实参报 `modobj.c` 未实现，待修；§28：os.run 子进程管道大 payload 先写后读死锁、Process/Raw* 无 finalizer 致 fd 泄漏与 zombie，待修；§20 / §21 均已修复，§22 待定；datetime 包 Date/Time/DateTime 已收工，仅剩时区见 §23；§24 为运算符重载语言待办；§25 为 Koala vs Python 标准库场景×热度对比盘点）

---

## 1. VM 侧 `OP_NUM_*` handler 族

**进展（2026-08-30）**：16 条中已完成 **11 条**——比较族 6 件（EQ/NE/LT/LE/GT/GE）
与算术族 5 件（ADD/SUB/MUL/DIV/MOD）均有 TARGET handler。`test_generic_9`（比较）
与算术分发均转绿。架构变化：`typeslots.c` 已删除，slots 改为 `TypeObject` 上的
`Vector`（`slotid.h` 独立头文件），slotdefs 迁移至 `klc.c`；算术族 TARGET 通过
`kl_slot_call_one_arg` 走 slots 分发。

**Update (2026-11-06), closed**: all 16 have a `TARGET`. AND / OR / XOR already had one, and SHL / SHR
were added with the shift work (see §64), so the bitwise family works on a trait value.

---

## 2. 泛型一元运算符协议分发（`__neg__` / `__bitnot__`）

**现状**：`-a`（T : Arithmetic）报
`error: unary '-' operator requires int or float type.`；
`~a`（T : BitwiseOperators）报
`error: unary '~' operator requires int type.`
语义层直接拒绝泛型类型的一元运算。

**根因**：`OP_UNARY_*` 目前只是 IR 层伪指令，没有对应的协议下降路径与字节码族
（对标二元的 `OP_NUM_*`，需要一元版本）。

**施工方向**：语义层放开 trait 约束下的一元 `-` / `~`；新增一元协议指令
与对应 `OP_NUM_NEG` / `OP_NUM_BITNOT`（或归入同一族），isel 与 VM handler 同步。

---

## 3. 泛型复合赋值（`+=` / `-=` / `*=` / `/=` / `%=` 及位运算复合赋值）

**现状**：`var s = a; s += b`（T : Arithmetic）报
`error: inplace assignment is not supported for 'T' type.`

**根因**：复合赋值在泛型场景没有走 `__iadd__` 等协议方法的下降路径
（具体类型上的复合赋值工作正常）。

**施工方向**：泛型复合赋值下降为对应 `__i*__` 的协议调用
（或降级为 `s = s + b` 的协议二元运算 + 赋值），与第 1 项的 VM handler 族同步落地。

---

## 4. `in` 表达式的语义下降（`__contains__` 语法糖）

**已完成（2026-10-02 核实，本节关闭）**：全链路已打通——`parse_in`（parser_expr.c 分发表）→ `emit_ir_contains`（irgen.c）→ `OP_CONTAINS` TARGET（vm_ops.h，经 `SLOT_CONTAINS` 槽分发）；test-kl/test_contains.kl 守护类型检查（`100 in "hello"` 报 incompatible type、`1 in 100` 报不支持 `__contains__`）。以下为建立时的原始记录，留档备查：

**现状（原记录，已过时）**：`koala.y` 已有 `in_expr` 语法规则、AST 已有 `EXPR_IN_KIND`，
但 `parser_visit_expr` 与 irgen 的分发表都没有该 kind 的 handler——
写 `5 in r` 编译失败（`koala: compilation failed`，不再段错误）。

**影响**：运算符显式调用禁令（43 项，见设计文档第 3 节）落地后，
`__contains__` 不能显式调用，而 `in` 糖又未实现——
**dict/set 的成员判定目前没有任何可用入口**。

**施工方向**：为 `EXPR_IN_KIND` 补语义处理与 IR 下降，
下降为专用指令 `OP_CONTAINS`（opcode_list.h 已定义，经 `SLOT_CONTAINS`
槽分发；VM TARGET 待实现，见 §12）。

---

## 依赖关系

```
第 2、3 项的运行时部分依赖第 1 项（VM handler 族）；
第 4 项独立，且因 __contains__ 显式调用被禁而成为功能性阻断，建议优先。
```

**Update (2026-10-28)**: item 4 is closed (2026-10-02), so it no longer blocks anything; `in`
works end to end.

---

## 5. 语言手册（Koala_Language_Manual.md）后续项

> 2026-08-22 整体 review 产出。P0 已完成（目录、Union Types、Error Handling、Memory Management），
> 以下为剩余项。

**P1 —— 完整性补齐**

1. 表达式与运算符基础章节：基础运算符参考（算术/比较/逻辑/位/赋值/复合赋值）+ 优先级表。
2. Containers 加深：每类容器常用操作示例；set 字面量 `{1,2,3}`；切片 `nums[1:3]` 示例。
3. Strings 加深：切片 `s[0:5]`、拼接、转义序列。
4. 内建函数速查表：`print` / `enumerate` / `zip` 收拢成表（顶层 `len` / `hash` 已于 2026-10-02 删除，长度 / 哈希只有 `x.len()` / `x.hash()` 方法形态）。
5. 词法基础：字面量进制（hex/oct/bin 在 parse_lit_int 有实证）、下划线分隔符待核实。

**P2 —— 可选**

6. 顶层代码说明（模块级语句在 `__init__` 执行、与 `main` 的关系）。
7. Trait 章节补充：`&` 多 trait 基列表、trait 继承（`Comparable : Equatable`）、trait 值作参数。

**优化（事实性/一致性问题）**

8. ~~重复内容合并：`class Foo[T]` 例子在 Classes and Interfaces 与 Generics 逐字重复；
   `add_pair` 出现 3 次（Functions / Generics / Operator Overloading）。~~
   **Resolved (2026-10-28)**: `class Foo[T]` and `add_pair` are defined once, in Generics.
   Classes and Interfaces uses a plain `Counter` class; Functions and Operator Overloading refer
   to Generics.
9. ~~运算符表 `x in seq` 一行：`in` 语义未落地（见上文第 4 项），需标注或暂时移除。~~
   **Resolved (2026-10-28)**: `in` works now (`OP_CONTAINS` handler in `vm_ops.h`, used by
   `test-ut/test_ut_list.kl`), so the table row stays as it is.
10. ~~Operator Overloading 的 Point 示例输出注释失实：未实现 `__str__` 时按 Printable identity
    默认打印 `Point@0x7fXXXX`，不是 `Point(4, 6)`；给示例加 `__str__` 或改注释。~~
    **Resolved (2026-10-28)**: `Point` implements `Printable.fmt()` (`f.write_fmt(...)`), so the
    output comments hold. The stale `__str__` / `__hash__` / `__len__` names in the manual are
    now `fmt()` / `hash()` / `len()`.
11. ~~`guard` 悬空：Booleans 提到 guard 但全文无说明（语法存在、无测试实证）；删除该词或补说明。~~
    **Resolved (2026-10-27)**: `guard` is replaced by `let-else`; the word is removed from
    Booleans, and Null Safety has a new `let-else` subsection.
12. ~~章节名 "Functions and Closures" 与正文（闭包不存在）矛盾，可考虑改为 "Functions"。~~
    **Resolved (2026-10-28)**: the chapter is renamed "Functions".
13. ~~Hello World 的 `koala -c` 与测试体系用的 `koalac` 是否一致，待作者确认。~~
    **Resolved (2026-10-28)**: both are right. `koala` is the driver that runs `koalac`, and
    `koala -c` (`compile_only` in `args.h`) stops after compiling; the tests call `koalac`
    directly. Hello World now says so.

---

## 6. 版本计划与路线图（合并自根目录 plan.md / think.md / review.md）

> 2026-08-22 由根目录 plan.md、think.md、review.md 合并而来，原文件已删除。

### 6.1 版本计划（原 plan.md）

**Version 0.8.0**

1. 支持非原始类型（用户自定义 class）的数值运算符：`__add__` / `__sub__` / `__mul__` /
   比较与相等（`__lt__` 等）

```swift
let s0 = "hello"
let s1 = "world"
s0 == s1, s0 > s1
```

2. 支持把类型当作普通对象加载，更易用：

```
class A {

}

print(A.methods()) // this is static method call, not type's type method
```

### 6.2 编译器 / 泛型路线图（原 think.md）

**泛型优化**

- ~~泛型 IR 保存到 klc 文件中~~
- ~~泛型展开（单态化）~~ → **已决定：暂不做泛型 AOT 单态化**，继续类型擦除
  （TValue 统一表示）。字段布局不变的前提下，单态化只能省掉 tag 动态分派开销，
  收益有限，且需新增 klc IR 序列化格式、处理跨包重复实例化去重，投入产出比不高。
  后续性能方向改为 JIT（运行时按具体类型 + 热点信息特化）。klc 格式和
  InstanceSymbol（仅服务编译期类型检查）保持不变。
- 用 Self 代替 T 作为 sugar，在 AST 层面转换为 T
  - 注意：应先做“泛型 T 自动推导”，Self 糖依赖推导完成后可能就不需要了；顺序不要搞反。
- 泛型 T 的类型自动推导
  - 部分已实现：`TP_INFER`（symbol.h）+ 构造函数单参数推导（parser_expr.c），
    如 `Foo(1)` 无需写 `Foo[int](1)`。待办：把推导范围从“构造函数单参数”泛化到
    更多场景（普通方法调用、多类型参数等）。
- 有泛型自动推导就不需要 Self 了。
- trait 自动绑定 T（默认绑定为类自身）——未实现，现状要求显式写出，
  如 `class str : Sequence[str]`。（注：裸遵循自类型推导已在 Equatable/Comparable 等
  落地，本条针对 Sequence 等元素类型场景的例外规则。）
- ~~入参 invariant（不变）~~ ——已实现：parser.c 中 generic 类型参数严格不变
  （`List[int32]` 与 `List[int64]` 不兼容）。
- 返回值 covariant（协变）
  - 待确认：若同一方法内 T 既是入参又是返回值类型，不能整体协变（否则不安全）。
  - 需要区分“只读方法（T 仅出现在返回值）可协变”与“读写方法（T 同时在入参）必须
    invariant”，不能一刀切。
- 如果 Sequence 的 T 不是类自身，用户必须显式写出 T（已是现状，后续作为“自动绑定”
  的例外规则）。

**泛型结论**

- 泛型走类型擦除；有性能问题走定制化 OP + CFFI。
- 增加一个 dynamic_upcast op 来支持所有泛型调用方法的例子。
- 先限制 T 的 upbound 只能有一个；限制 T 实例化必须是 class，不能是 trait。
- union 类型限制为 class，不能有 trait：
  - union 成员仅允许 concrete class（可含 builtin class）；
  - 不允许 trait、type parameter、any；
  - 若需要“多接口能力”，用泛型约束或显式 upcast，不用 union 表达。

**语言规则建议（trait 约束省略形式）**

- 禁止 `x: Equatable` 这种 existential 写法（函数入参必须是完整类型）；
- 允许简写：`func foo[T: Equatable](x: T)`，编译期规范化为 `func foo[T: Equatable[T]](x: T)`；
- 只有当 Equatable 是单参 trait 时才允许省略（多参如 `Mapper[K, V]` 必须写全）；
- T 在 `Equatable[...]` 里默认绑定为“当前类型参数自身”（Self-like 绑定）。

### 6.3 语言设计思考（原 think.md“其他的思考”）

- bytes 支持不可修改选项，这样 str.to_bytes 就可以安全地用 view 而不是 copy；
  有了只读 bytes，io 可以不需要支持 string（str.to_bytes 无性能问题）；
- 只需要 bytesio，不需要 stringio；不能合并 bytesio 和 bytebuffer
  （bytebuffer 是可增长缓冲区数据结构，bytesio 是流式游标/Reader-Writer 接口，
  语义不同，类似 Go 的 bytes.Buffer vs io.Reader/Writer）；
- `__str__` vs `__fmt__` 待定；
- 运算符不绑定接口的模型：用户使用运算符只检查 class/trait 是否实现对应运算符方法，
  不检查是否遵循了接口；只有类型被声明为某接口（赋值/入参/返回值）时才检查接口
  方法是否实现；
- 接口方法允许部分实现，未实现方法运行时被执行则抛异常，避免大量 Adapter 类；
- 生成 IR 后插入 transform 把部分 op 转为 call，再交给 opt；
- 逻辑运算符不可重载；`__and__` / `__or__` / `__xor__` 是位运算符（可重载）；
- 自动添加 free 释放 build_intern 之类的 op；
- koala 支持脚本执行，但只编译当前 kl 源文件，依赖包必须事先编译为 klc
  （既保住脚本体验，又是静态类型语言）。

**逃逸分析：先只支持入参，native 由程序员标记**

```
class Foo {

    @noesc(self, name)
    func hello(name str) { ... }
}
```

### 6.4 待办清单（原 think.md 尾部 checklist）

- index access（进行中）
- ☐ slice access
- ☐ `__str__()` vm method binding
- ☐ range / tuple / str / int/float / list / dict / set / bool / NoneType 完善
- ☐ 跨模块、迭代器协议、with 语句支持
- ☐ 函数定义和调用、类定义和实例化、属性访问和方法调用
- ☐ 继承和多态（注：现设计为 class 恒 final + trait 组合，本条按新设计重估）
- ☐ 异常处理（注：现定案为 panic + 返回值，无异常体系，本条按新设计重估）
- ☐ 模块导入和使用、文件操作、正则表达式支持
- ☐ 多线程和多进程支持（注：现定案为无栈协程，待公开细节）
- ☐ 网络编程支持、数据库连接和操作支持
- ☐ 标准库支持、性能优化、内存管理优化（见 Memory Management 未实现标注）
- ☐ 错误处理和调试工具、文档和示例代码

构建标准库命令：

```
koalac --build-stdlib --write-klc libs/std/builtin --package-name=std/builtin
```

### 6.5 外部评审结论存档（原 review.md）

Koala 是完成度极高的原创设计：不是“某语言 + 某特性”的拼凑，而是从三条原则出发
逐层推演的结果。基于可见面给出的“缺口清单”基本全部作废——那些“缺口”要么是
刻意的设计立场（错误处理），要么是看不见的水下机制（overflow、提升、并发、
内存管理）。对“无隐藏特性”原则的注脚：对用户无隐藏，不等于对旁观者无纵深。

**五大世界级设计**

1. **根契约：普适性与类型精度的统一（独一份）**——Java 用 Object 继承换普适性，
   丢了签名精度；Swift/Rust 用 opt-in 保精度，丢了普适性。Koala 的“空 any +
   编译器自动遵循 + 泛型实例化槽位”是两条都要、代价为零的解。
2. **钩子与契约分轨（最干净的一份）**——四类运算符钩子零契约、语义契约零运算符
   捆绑、Comparable 保持经典的 Equatable 继承形态。
3. **panic + GC 的协同（被低估的妙手）**——GC 兜底资源回收，panic 才能真正轻装
   上阵；与 C 式返回值构成“可预期/不可预期”的完美二分。
4. **unsafe-free FFI**——编译器独占桥接，人根本不碰；unsafe 的存在理由被结构性消除。
5. **数值机制**——窄类型 init-only + 自动提升 + overflow trap/wrap 双操作；
   “标准库完全不感知”的隐藏分层比 Swift 更彻底。

**语言坐标系**

| 维度 | Koala 位置 |
|------|-----------|
| 语义直觉 | Python（插入序 dict、dunder、enumerate/zip、class 层命名） |
| 契约机制 | Rust 系（intf-table、trait 约束）但无生命周期/unsafe 负担 |
| 错误处理 | C 的返回值 + 极简 panic，拒 Java try-catch、拒 Go defer |
| 工程布局 | 学 Go（encoding 分包），避 Go 之短（cgo、无重载） |
| 对象模型 | 超越 Java（无 Object 神类）与 Kotlin（自动遵循免声明） |

---

## 7. OP_NUM_* 全特性实现（VM handler 族）

> 2026-08-22 建立。实现完成一项打勾一项，不删除。

### 7.1 进展更新（2026-08-30，取代下述原始快照）

- typeslots.c 已删除，slots 改为 `TypeObject` 上的 `Vector`；slotid.h 独立头文件；
- slotdefs 迁移至 `klc.c`（`SlotDef slotdefs[]` 数组）；
- 比较族 6 件 TARGET 全齐（含 int64/uint64/float64 快路径）；
- 算术族 5 件 TARGET 全齐（ADD/SUB/MUL/DIV/MOD），通过 `kl_slot_call_one_arg` 走 slots 分发；
- printer.c 覆盖度待核实；
- 位运算族 5 件（AND/OR/XOR/SHL/SHR）：done, see the update in §1 (2026-11-06);
- test_generic_9 / test_generic_14 均绿；
- 新增 `@specialized` 泛型函数单态化（commit `3d9ea1a6`，test_specializd.kl 验证通过）。

### 7.1b 原始快照（2026-08-22 建立时点，已部分过时，保留备查）

- opcode_list.h：16 条 OP_NUM_*（算术 ADD/SUB/MUL/DIV/MOD、位运算 AND/OR/XOR/SHL/SHR、
  比较 EQ/NE/LT/LE/GT/GE），全部 FORMAT_RRR；
- isel.c `num_ops_rules`：OP_BINARY_* → OP_NUM_* 16 条映射齐全；lower_operands.c
  `is_binary` 已覆盖 OP_NUM_ADD..OP_NUM_GE 区间；
- vm_ops.h：仅 OP_NUM_EQ 有 TARGET，其余 15 条在非 computed-goto 模式落 eval.c
  UNREACHABLE（test_generic_9 失败的直接原因）；
- typeslots.c：比较六件已挂（slotdefs 全齐，cmp → slot_tp_richcmp）；num_slotdefs
  已存在但 FUNC 全为 NULL、绑定循环是注释占位；bit_slotdefs 不存在；
  TypeObject->arith / ->bit 全库从未被分配或赋值；
- printer.c：只打印 num.eq / num.gt，缺 14 个 case；
- 内建数值类型运行时**没有** cmp / arith / bit 函数：@intrinsic 条目在 klc 里是普通
  空体 CodeObject（write_klc 只认 SYM_FLAGS_NATIVE），slots 动态分发执行空体帧不可用。
  比较族的对策已定为 **TARGET 内建快路径**（OP_NUM_EQ 先行）。

### 7.2 任务清单

**第一批：比较族** ✅ 全部完成

- [x] OP_NUM_EQ 增加 int64 / uint64 / float64 快路径——2026-08-22
- [x] OP_NUM_NE / LT / LE / GT / GE 五个 TARGET——2026-08-22
- [x] uint64 快路径：`(uint64_t)ival` 无符号比较，六条全齐——2026-08-22
- [x] test_generic_9 转绿——2026-08-22

**第二批：算术族** ✅ 全部完成（通过 slots 分发，非 typeslots.c 蹦床）

- [x] OP_NUM_ADD/SUB/MUL/DIV/MOD 五个 TARGET（`kl_slot_call_one_arg`
      走 SLOT_ADD..SLOT_MOD）
- [x] test_generic_14 用户自定义数值类全链路验证通过

**第二批续：位运算族** ✅ TARGET 已完成

- [x] OP_NUM_AND/OR/XOR/SHL/SHR 五个 TARGET（2026-11-06，走 `kl_call_bit_*`，
      `test-run/test_shift_trait.kl` 验证）
- [ ] printer.c 位运算 case 核实与补齐

**第三批：num.* 比较跳转融合（jmp fusion）**

动机（实测 IR，`func max[T : Comparable](a, b) { if a > b { return a } ... }`）：

```
0002:  AE020001   num.gt r2, r0, r1
0003:  54020001   jmp_false r2, 1
0004:  75000000   ret r0
0005:  75000001   ret r1
```

`num.gt` 的结果 r2 仅被 `jmp_false` 消费，两条指令应融合为单条
`jmp_num_gt r0, r1, off`：省一次中间寄存器写读 + 一次 DISPATCH，
且快路径比较与跳转在同一 handler 内完成。

现状对照：单态融合链路已全齐——isel.c `int_cmp_map` / `float_cmp_map`
（+ uint 系）把 OP_INT_*/OP_FLOAT_* 比较 + jmp 融合为 OP_JMP_INT_* /
OP_JMP_UINT_* / OP_JMP_FLOAT_*（含 _IMM 变体，FORMAT_RROff / RImmOff，
printer `print_jmp_cond_fused`、cgen `fused_jmp()` + `lower_fused_jmp` 均就绪）；
**唯独泛型 num.* 比较没有融合族**。

- [ ] opcode_list.h：新增 OP_JMP_NUM_EQ / NE / LT / LE / GT / GE（FORMAT_RROff；
      _IMM 变体视收益再定，比较对象多为泛型变量，初版可不做）
- [ ] isel：OP_BINARY_GT..GE（走 num_ops_rules 的泛型比较）后接
      OP_JMP_FALSE / OP_JMP_TRUE 时融合为 OP_JMP_NUM_*（真值分支取反语义
      与现有单态融合保持一致）
- [ ] vm_ops.h：六个 TARGET，直接复用 OP_NUM_* 比较族的快路径分支
      （int64 有符号 / float64 直比 / uint64 `(uint64_t)` 无符号 / cmp 兜底），
      命中即跳，不再写中间寄存器
- [ ] printer.c / cgen：`print_jmp_cond_fused` 六个 case + FORMAT_RROff 表项
- [ ] 验证：test_generic_9 的 max/min if-branch 反汇编出现融合指令，全量回归无新增

**验证**

- [x] test_generic_9 转绿（六件比较运算符 × int64/float64）——2026-08-22
- [x] test_generic_14 用户自定义数值类全链路 PASS——2026-08-22
- [x] 全量 lit-tests 基线：**136/139 通过**（97.84%），2 Unresolved
  （test_bytes.kl / test_io.kl），1 FAIL（test_pkg_4.kl，LD_LIBRARY_PATH 未设置）
- [x] 2026-10-02 测试基线盘点：lit 发现 **190** 个测试（test-kl 38 / test-ir 36 / test-run 88 / test-ut 28），全部带 RUN 行——§15 的“无 RUN 挂账”全部消解

### 7.3 决策点（待作者拍板）

1. @intrinsic 空体的系统性解法：比较族已走 TARGET 快路径；**算术族已确定走
   slots 调用**（`kl_slot_call_one_arg`）；位运算族跟随同一模式。
2. 一元 `-` / `~`（OP_UNARY_* 仅 IR 伪指令）与泛型复合赋值仍是独立项，不在本批。

---

## 8. 泛型体内 T 的点号方法调用（itab 挂接）——作者决定暂时搁置

> 2026-08-22 建立。作者明确：此场景当初未实现是有意为之，复杂度超出表面，
> 重新开启前不推进任何方案。

**现状**：泛型体内对 `T : Trait` 参数做点号方法调用（如 `a.__str__()`、
`a.hash()`）会撞上 `vm_ops.h:431` 的 `is_intf(callable)` 断言——槽调用
（`itab->methods[index]`）要求接收者携带 itab，而 T 实参是裸值。

**根因链**：irgen.c `_build_obj_intf_upcast` 对 SYM_TYPE_PARAM 目标返回 NULL
（只处理 class → trait 的 make_intf 与 trait → trait 的 upcast_intf），
调用点不发射任何挂接指令 → 裸值进泛型体 → 槽调用断言。

**不受影响的边界**：运算符路径（`==`、`+` 等）走 OP_NUM_* 协议分发，
不需要 itab，继续可用。

**已否决的方向（重开时不必重提）**：

1. 调用点递归挂 bound[0] 的 itab——语义错误：itab 需求归属泛型体内的使用点
   （用了哪个 bound 的方法），不归调用点；body 用第二个 bound 即失效；
   且 `get_intf_index` 按类查 lro，T 无类，索引无法烘焙进指令。
2. 使用点按 trait 名解析（typeof / `itab->tp` 回指恢复具体类型后在
   `tp->itables` 里按名匹配）——原理上可行（IntfTable 回指使包装值可恢复），
   且能结构性消解多 bound 争抢单 itab 槽位的问题，但作者判定为时过早，搁置。

**配套约定**：测试用例保持注释状态（test-run/test_bool_intf_complete.kl 的
`intf_hash` / `intf_str` 及对应 print），待本项重开后启用。

---

## 9. list 的 append/pop 注解升级：@native → @intrinsic

> 2026-08-31 建立。

**现状**：`list.kl` 中 `append(value T)` 和 `pop(index = -1) T` 标记为 `@native`。

**目标**：改为 `@intrinsic`，让编译器直接发射专用 VM 指令：
- `push` → `OP_LIST_PUSH`（IRGen 已直接发射，无需 ISEL 特化）
- `pop` → `OP_LIST_POP`（当前走 OP_CALL + ISEL 特化路径，需 ISEL 支持）

**理由**：`push`/`pop` 是最高频的容器操作（循环构建列表、栈操作），
`@intrinsic` 消除 slots 分发开销，直接操作内部数组 + 容量检查 + write barrier。

**施工方向**：
1. `list.kl`：`@native` → `@intrinsic`（push、pop 两个方法）
2. 确认 IRGen 对 `list.append()` 发射 `OP_LIST_PUSH`（已有逻辑）
3. 确认 ISEL 对 `list.pop()` 的 OP_CALL 特化为 `OP_LIST_POP`（需核实）
4. 全量回归测试

**依赖**：`OP_LIST_POP` 指令是否保留待确认（作者曾讨论移除，改为走 slots 分发）。
若决定保留，则 push/pop 双双升级为 @intrinsic；若移除 OP_LIST_POP，则仅 push 升级。

---

## 10. Truthiness 协议与 all/any 函数（不一定支持）

> 2026-08-31 建立。标注：**不一定支持**，作者未最终拍板。

**设想**：引入 `Truthiness` trait，作为语法钩子支撑 `if obj:` 语法和 `all()`/`any()` 泛型函数。

```koala
pub trait Truthiness {
    func __bool__() bool
}
```

**编译器行为**：`if obj:` → 编译期检查 T 是否实现 Truthiness → 是则下降为 `if obj.__bool__():`，否则编译报错。与现有运算符钩子（`+`/`==`）模式完全对称，无隐式转换。

**实现范围**：
- 基本类型实现：`int`（0 = false）、`str`（空 = false）、`bool`（自身）
- 容器类**不**实现（`if my_list:` 不合法，必须 `if my_list.len() > 0:`）

**下游产物**：`all()`/`any()` 变为可行：

```koala
func all[T: Truthiness](items Iterable[T]) bool { ... }
func any[T: Truthiness](items Iterable[T]) bool { ... }
```

检查的是元素的真假，不是容器的真假。与 Python 语义一致。

**未拍板原因**：引入 Truthiness trait + `if obj:` 语法糖是否符合 Koala 的“简洁”原则，尚需作者最终决定。不做也没有功能缺失——用户可显式写 `if x != 0` / `if x != ""`。

---

## 11. OP_SEQ_GET_IMM / OP_SEQ_SET_IMM 负下标语义未定义

> 2026-09-01 建立。

**现状**：`OP_SEQ_GET_IMM` / `OP_SEQ_SET_IMM` 的 IMM 是 signed 8-bit
（-128..127），但注释未定义负下标的语义。

**待定案的问题**：
1. Python 式负下标（`a[-1]` 从尾计数）是否支持？
2. 若支持，是编译期展开（重写为 `len - |imm|`）还是运行时解释
   （handler 内做 `if idx < 0 { idx += len }`）？
3. bounds check 是否覆盖负索引场景（编译期展开方案下，重写后的
   索引仍需运行时 bounds check；运行时解释方案下，负索引越界报错
   语义需明确定义）？
4. 与 OP_SEQ_GET（寄存器下标版）的负索引语义是否保持一致？

**建议**：两个指令的负下标语义必须一致；若当前 OP_SEQ_GET 运行时
handler 不处理负索引，IMM 版本也应拒绝负数（编译期报错），避免两套语义。

---

## 12. 容器协议指令族实现（VM handler / printer / isel）

> 2026-09-01 建立。容器类 op 重构（opcode_list.h 1914–2157）后盘点。
> 与 §9（list push/pop 的 @intrinsic 化）相关但正交：§9 是方法内置，
> 本节是语法钩子协议指令。三层对应见 Koala_Design_Overview.md §8.3。

**已完成全链路（isel / printer / VM TARGET）——7 条**（2026-10-02 复核）：

- [x] OP_SEQ_GET（TARGET 经 kl_slot_call 走 SLOT_GET_ITEM）
- [x] OP_SEQ_GET_IMM（isel.c:1051 常数下标特化）
- [x] OP_SEQ_SET（SLOT_SET_ITEM）
- [x] OP_SEQ_SET_IMM（isel.c:1078）
- [x] OP_LEN（2026-10-02 起仅 for-each Sequence 快速路径内部指令：irgen → `klr_build_seq_len` 发射，TARGET 走 SLOT_LEN；原顶层 `len(x)` 的 isel magic 下降分支已删除，用户 `x.len()` 直落 resolved call。`OP_HASH` 已随顶层 `hash()` 退役，零引用）
- [x] OP_CONTAINS（vm_ops.h TARGET 经 SLOT_CONTAINS；`in` 语法全链路见 §4）
- [x] OP_SEQ_GET_SLICE（vm_ops.h TARGET 经 SLOT_GET_SLICE）

**已定义、未实现——5 条**：

- [ ] OP_SEQ_SET_SLICE（SLOT_SET_SLICE）
- [ ] OP_MAP_GET（IR 层已有：insn.c:816 用于成员访问下降；VM TARGET 缺）
- [ ] OP_MAP_SET（IR 层已有：insn.c:842，dce.c 已识别其副作用；VM TARGET 缺）
- [ ] OP_LIST_PUSH（IRGen 发射 + TARGET；见 §9）
- [ ] OP_LIST_POP（OP_CALL + ISEL 特化路径；见 §9）

**每条待办含**：vm_ops.h TARGET（kl_slot_call_* 模式）、printer.c case、
isel 规则（如适用）、回归测试。

**源码注释遗留（顺手修正项）**：
- slotid.h:30 `SLOT_LEN, // __len__` — 注释已换成 dunder 名，但 len 无 dunder（普通方法，2026-10-02 裁定）；绑定表实证 `TPSLOT("len", SLOT_LEN)`（klc.c:462），宜改 `// len`。同族 `// __getitem__` 等对应真 dunder，不受影响
- vm_ops.h:1531 `// TODO: can be negative?` — 即 §11 的负下标问题

---

## 13. 数值类型 cast 补全（int / uint / float 互转）

> 2026-09-04 建立。**2026-09-05 更新：cast 补全已完成**——原 NYI 1 / NYI 2 全部落地，48 组跨族转换均可编译并正确运行。
> 本节由"待实现清单"转为"语义与测试基线记录"。设计层面的完整叙述见 `Koala_Design_Overview.md` §5.1。

### 13.1 构造函数签名

`libs/std/builtin/number.kl` 中**每个**数值类型的 `__init__` 均接受 `int64 | uint64 | float64`；`int64` / `uint64` / `float64` 另接受 `str`。窄类型（int8-32 / uint8-32 / float16-32）依赖隐式提升到 int64 / uint64 / float64 后再截断，提升是编译器机制，标准库源码中不可见。

类型检查已不拒绝任何数值组合，原先阻塞全部 int ↔ float 的 `isel.c` `NYI()` 已移除。

### 13.2 双轨语义：常量编译期判定，变量运行期执行

同一段 `dst(src)` 写法走两条完全不同的路径，**分轨是刻意设计**：`float32(2147483647)` 作为常量编译失败，作为函数参数则得 `2147483648`。

**常量轨——编译器无条件校验，不受 `--int-trap` / `--float-trap` 影响**

- 实现：`check_int_const_cast_valid` / `check_uint_const_cast_valid` / `check_float_const_cast_valid`（`src/parser/opt/const_copy_prop.c`），由 const-copy-prop pass 调用，不读任何 cast 开关。
- 判定用**往返一致性**（`(int64_t)h != val` / `(uint64_t)f != val` / `(double)ival != v`）而非单纯区间比较，因此越界、截断小数、**纯精度丢失**一律编译失败。
- 诊断文本只有两种，CHECK 时按原文写（措辞是 "overflow"，即便实际只是精度丢失）：
  - `constant float overflow in cast: cannot cast from <src> to <dst>`
  - `constant float overflow in cast: cannot cast negative <src> to <dst>`（float → uint 且值为负时优先触发）
- 报错例：`float16(32767)`（折成 32768，往返不一致）、`float32(2147483647)`、`float64(9007199254740993)`、`int8(300)`、`uint8(-1)`、`int64(1.5)`。
- pass 开启条件：`koala` 默认经 `--cgen` 打开；`koalac` 需 `--opt`（或 `--isel` / `--lsra` / `--cgen`）。

**变量轨——降级为 op，由标志位控制**

- `isel_lower_cast`（`src/parser/backend/isel.c`）选四个操作码之一：`OP_INT_CAST` / `OP_FLOAT_CAST` / `OP_FLOAT_TO_INT` / `OP_INT_TO_FLOAT`，目标类型与模式一起编码进 `cast_flag`，由 `include/runtime/do_cast.h` 在 VM 中执行。
- `int_cast_mode()` / `float_cast_mode()`（`include/parser/cmd.h`）是全项目唯一读取这两个开关的地方，只被 isel 使用。
- **mode 0（开关打开，trap）**：越界或不能精确表示即打印 `panic: … cast overflow …` 并 `abort()`。
- **mode 1（默认，wrap / saturate）**：取值见 13.3。

### 13.3 运行期默认取值（mode 1，实测）

| 场景 | 行为 | 实测例 |
|------|------|--------|
| 整型窄化 | 按位回绕 | `uint8(300)` → 44；`int16(40000)` → −25536 |
| float → float 窄化 | 静默舍入 | `float32(3.14159265358979)` → `3.1415927410125732` |
| float → int 在范围内 | 向零截断 | `int64(1.9)` → 1；`int64(-1.7)` → −1 |
| float → uint 且值为负 | 得 0 | `uint8(-1.5)` → 0；`−inf` → uint64 得 0 |
| NaN → int / uint | 得 0 | |
| 超出 64 位边界 | 先饱和 | → `INT64_MAX` / `INT64_MIN` / `UINT64_MAX` |
| 窄目标（int8/16/32, uint8/16/32） | 饱和到 64 位后**再回绕**到目标宽度 | `int8(1000.7)` → −24；`int8(+inf)` → −1 |
| `+inf` → uint64 | `UINT64_MAX`，print 显示为 −1 | 见 13.6 |

float → int **不 trap，而是饱和**——这是 `do_cast.h` mode 1 分支里明确写出的语义，按现状作为规格对待，不要改成 trap。

### 13.4 测试布局

按**源类型**分文件，常量与运行期两套并行：

| 文件 | 内容 | RUN 行 |
|------|------|--------|
| `test-kl/test_const_int_cast_failed.kl` | int 源常量 cast 编译期报错 | `koalac %s --opt 2>&1` |
| `test-kl/test_const_uint_cast_failed.kl` | uint 源同上 | `koalac %s --opt 2>&1` |
| `test-kl/test_const_float_cast_failed.kl` | float 源同上（含 inf / NaN） | `koalac %s --opt --float-trap 2>&1` |
| `test-kl/test_float_cast_fail.kl` | float → float 窄化精度丢失 | `koalac %s --opt --float-trap 2>&1` |
| `test-run/test_cast_int.kl` | int 源运行期取值 | `koala %s` |
| `test-run/test_cast_uint.kl` | uint 源运行期取值 | `koala %s` |
| `test-run/test_cast_float.kl` | float 源运行期取值 | `koala %s` |

约束：

- **运行期用例必须包在函数里、值走参数**，否则被常量折叠，得到的是编译期报错而不是运行期取值。
- 编译期报错文件必须带 `2>&1`；运行期取值文件**不能**带（isel 日志走 stderr，会破坏结尾的 `CHECK-NOT: {{.}}`）。
- NaN 只能由 float64 常量承载：得来自折叠表达式（`1.0e400 - 1.0e400`），绑到 `float32` / `float16` 的 `let` 会报 "Types of two sides are not matched."。字面量在窄宽度下可溢出成 +inf，但永远不会是 NaN。
- 旧的按"源→目标对"分文件（`test_cast_int_int.kl` / `test_cast_uint_uint.kl` / `test_cast_int_uint.kl` / `test_cast_float_float.kl` / `test_cast_int_float.kl` / `test_cast_float_int.kl` / `test_cast_uint_float.kl`）已全部合并删除，**不要重建**；同样不要为一个用例单开一个文件。

### 13.5 已完成 ✅

- [x] **NYI 1**：int / uint → float 转换，经构造函数路径落地为 `OP_INT_TO_FLOAT`
- [x] **NYI 2**：float → int / uint 转换，经构造函数路径落地为 `OP_FLOAT_TO_INT`
- [x] 全部 48 组跨族 cast 编译并运行正确；`isel.c` 中阻塞 int ↔ float 的 `NYI()` 已移除
- [x] 常量 cast 合法性校验：`check_int_const_cast_valid` / `check_uint_const_cast_valid` / `check_float_const_cast_valid` 覆盖 int / uint / float 三种源，越界、截断、精度丢失均报错
- [x] **Bug 1**（2026-09-04）：float widening 在局部变量上返回 0——根因是 `check_float_const_cast_valid()` 只处理窄化，宽化时 `*out` 未赋值；已补 else 分支直传 `c->fval`
- [x] **Bug 2**（2026-09-04）：`float32(3.14) → float64` 得 `3.1400001049041748` 是正确行为，非 Bug——显示的是 float32 的精确值
- [x] 全量回归：156 discovered / 151 passed / 5 unresolved（既有）/ 0 failed

### 13.6 遗留

- [x] **NYI 1b**：`int64.to_float()` / `uint64.to_float()` native 方法——已实现（2026-10-23，`intobj.c` `_int_to_float`，按 self tag 区分符号，`UINT64_MAX.to_float()` 正确得 `1.844674407370955e+19` 而非 −1）
- [x] **NYI 2b**：`float64.to_int()` native 方法——已实现（2026-10-23，`floatobj.c`，向零截断，NaN→0、越界饱和到 INT64_MIN/MAX，与 do_cast.c wrap 模式一致）
- [x] **数值 native 方法族未实现（2026-10-02 src/ 全文 grep 核实；2026-10-23 全部销账）**：~~`int64.pow / abs / to_float`、`uint64.pow / to_float`~~ 已于 2026-10-23 在 `intobj.c` 实现并注册进 `int_methods[]`（pow 用 uint64 二进制快速幂、溢出 wrap 无符号 UB；int64.pow 负指数按数学结果向零截断：base ±1 → ±1，其余 → 0；abs 的 INT64_MIN wrap 到自身）。~~模块级 `pow_int64_int64` / `pow_float64_int64` / `pow_float64_float64`~~ 已于 2026-10-23 在 `src/modules/builtin.c` 实现并注册进 `builtin_functions[]`（int64 版语义与 `int64.pow` 一致；`pow_float64_int64` 平方求幂、负指数取倒数、`0.0^负 → inf`；`pow_float64_float64` 直通 C `pow()`，`(-1.0)^0.5 → NaN`）。更正：`abs_int64` / `abs_float64` 本就是 `__builtin__.kl` 中的**纯 KL 实现**（非 @native），无需 C 实现，原记录有误。~~`float64.abs / ceil / floor / round / sqrt / pow / to_int / is_nan / is_inf`~~ 已于 2026-10-23 在 `floatobj.c` 全部实现并注册进 `float_methods[]`（json.dump 的 NaN/inf panic 路径依赖 `is_nan` / `is_inf`，libs/koala/json 全部测试通过）
- [x] **声明 Bug：`pow_int64_int64` 缺返回类型**（2026-10-23）：`__builtin__.kl` 中声明为 `pub func pow_int64_int64(base int, exp int) {}`（无 `int` 返回类型），导致 `let x = pow_int64_int64(...)` 编译报 `expr has not value`，且作为 print 多参数嵌套调用时触发 VM 断言 `(rs) < max_regs` 崩溃；已补 `int` 返回类型并 `wombat build` 重建 std dist
- [x] **编译器 Bug：常量 cast 报错后 `--write-klc` 段错误 + 诊断被吞**（2026-10-23 修复）：`int64(3.7)` / `uint8(300)` 等常量窄化构造经 `koala` CLI（带 `--write-klc`）编译时 SIGSEGV。根因链：const-copy-prop 检出错误（`klr_error` → stdout printf，重定向时全缓冲）→ `m->errors++` 但管线不检查、继续执行 → codegen 对错误模块不设置 `mach` → `write_to_klc` 只判 `pm->m` 非空即调 `write_rt_data(mach=NULL)` 解引用崩溃 → 崩溃时 stdout 缓冲未 flush，**错误消息随 core 丢失**，用户只见 "compilation failed"。修复（两层）：① `koalac.c` `compile()` 在 `--write-klc` 前检查 `m->errors > 0` 则 return −1（**中间 pass 不早退**——lit 的 `test_const_*_cast_failed` / `test_int_cast_2` 依赖错误后继续 dump IR 且 exit 0）；② `write_klc.c` `write_to_klc` 防御性改判 `pm->m && pm->m->mach`。修复后诊断正常显示（`constant float/integer overflow in cast: ...`），lit 245/245 全绿
- [x] **数值 API 去重决策执行**（2026-10-23）：删除数值类上与顶层函数/构造器重复的方法，**只动 `number.kl` + `intobj.c` + `floatobj.c` 三个文件**：① `int64.pow(x)` / `uint64.pow(x)` / `float64.pow(x)` 方法（KL 声明 + `_int_pow` / `_float_pow` C 实现 + methods 表项）——顶层 `pow(T,U)` 泛型壳路由修复后，`pow_*` 手动实例化是唯一 pow 入口；② `int64.to_float()` / `uint64.to_float()` / `float64.to_int()` 方法（KL 声明 + `_int_to_float` / `_float_to_int` C 实现 + 仅被 `_float_to_int` 使用的 `FLOAT64_UPPER/LOWER_BOUND_FOR_INT64` 宏）——数值构造器 `float64(x)` / `int64(x)` 是唯一转换路径；③ **追加：`int64.abs()` / `float64.abs()` 方法**（KL 声明 + `_int_abs` / `_float_abs` C 实现 + methods 表项）——顶层 `abs[T]` 壳已有纯 KL 手动实例化 `abs_int64` / `abs_float64`（`__builtin__.kl:131/139`，`if a < 0 { return 0 - a }`），路由修复后 `abs(-5)` 即落于此；语义一致性已核（INT64_MIN 情形：C 版特判 wrap 到自身，KL 版 `0 - a` 的 int64 wrap 减法行为相同）；`.abs()` 删前 grep 确认零活跃调用点（仅 test_int64_complete.kl 两处已注释调用）。**明确不动**：`str.to_int/to_float`（json/toml 解析器真实依赖）、`ceil/floor/round/sqrt/is_nan/is_inf` 等其余方法、`pow_int64_int64` / `abs_int64` 等顶层实例化（KL 声明 + builtin.c 实现）、do_cast.c 的字节码 cast。删除后调用点报干净编译错误（`'abs' is not found in class 'int64'`）；验证 `abs(-5)`=5、`abs(-2.5)`=2.5、`pow(2,10)`=1024、`int64(f: float)` 运行时截断=3、str 方法照常；lit 245/245、json 6/6 全绿。注：上文 NYI 1b/2b 及"数值 native 方法族"记录中的 to_float/to_int/pow/abs 方法实现已随本次清理删除，构造器语义（截断/饱和）由 do_cast.c 承担不变
- [x] **~~"Generic Shell Pattern" 编译器路由未实现~~（原记录定性错误，2026-10-23 更正并修复）**：路由**早已实现**——`parser_expr.c` 调用点经 `find_or_add_func_instance`（`parser_generic.c:573`）用 `mangle_func_name(name, tp_args)` 的 mangled 名 `stbl_get` 查现有符号，命中即路由到手动实例化 / `@specialized` 生成函数（abs/min/max 一直正常）。真正的 bug 在 `typespec.c` `mangle_func_name`：**多类型参数之间不写分隔符**，`pow[T,U]@[int64,int64]` mangle 成 `pow_int64int64`，与手写实例化符号 `pow_int64_int64` 不匹配 → 查找 miss → **静默实例化 panic 壳体** → 运行期报 `pow is not implemented for these types`（误导为"类型不支持"）。单类型参数壳体 mangling 恰好正确，掩盖了此缺陷；`pow[T,U]` 是全仓库唯一活跃的多类型参数泛型函数（zip.kl 的 `[T,U]` 均为注释）。修复：循环内在每个类型参数前写 `_`（单参数 mangled 名不变，完全向后兼容；类实例走 `mangle_type_name` 的 `<...>` 约定，不受影响）。验证：`pow(2,10)`→`call @pow_int64_int64`=1024（二进制快速幂）、`pow(2.0,10)`→`@pow_float64_int64`=1024、`pow(2.0,0.5)`→`@pow_float64_float64`=1.414213562373095；不支持组合（如 uint 变量）仍按设计运行期 panic 壳体。回归测试加入 `test_specializd.kl`（IR 路由 CHECK + 运行输出 CHECK），lit 245/245、json 6/6 全绿，`ninja install` 后 CLI 链路验证通过
- [ ] **外部泛型函数显式类型参数断言崩溃**（2026-10-23 pow 路由修复时新发现的**存量 bug**，与 mangling 修复无关）：`abs[int](-5)` / `min[int](3,1)` / `pow[int,int](2,10)` 等对 **std/builtin 外部符号**的显式实例化调用触发 `irgen.c:1217 emit_ir_index` 断言 `exp->ir_val` 失败（SIGABRT）；本地 `@specialized` 函数 `max[int](1,2)` 正常（test_specializd.kl 覆盖）。疑似 IndexExpr 携带显式 tp_args 且命中外部 SYM_FUNC 时 lhs 的 ir_val 未生成。隐式推断路径（`pow(2,10)`）不受影响
- [x] **数值 API test-ut 覆盖 + 测试基础设施修复：坏 RUN 行静默跳过 36 个 ut 文件**（2026-10-23）：① 新增 `test/test-ut/test_number_methods.kl`（16 个 `@test`，实测语义后写期望值）：pow int64 实例化（基本幂/负指数截断语义 `pow(2,-1)`=0、`pow(-1,-3)`=-1 奇偶/uint64 wrap 溢出 `pow(2,63)`=INT64_MIN、`pow(2,64)`=0/经函数参数的运行时路径）、pow float64_int64（`pow(2.0,-2)`=0.25、`pow(0.0,-1)`=inf）、pow float64_float64（`pow(2.0,0.5)`≈√2、`pow(-1.0,0.5)` NaN）、abs 路由（含 `abs(INT64_MIN)`=INT64_MIN wrap 语义、运行时路径）、float64 保留方法（ceil/floor 负数、round 半数远离零 2.5→3 / -2.5→-3、sqrt 负数 NaN、is_nan/is_inf 经 0.0/0.0 与 1.0/0.0、hash 稳定性）。② **发现并修复测试基础设施 bug**：`koala <file> --test`（无 `++` 分隔符）形式**静默不执行任何测试、exit 0**——test-ut 下 36/55 个存量文件的 RUN 行正是此坏形式（`// RUN: koala %s --test`），在 lit 中一直"假通过"（包括带 `@should_panic` 的 test_ut_fail.kl）；批量 sed 修正为 `// RUN: koala --test %s`（用户确认的正确形式：`koala --test <file>` 或 `koala <file> ++ --test`）。修正后抽查确认真实执行（test_ut_assert 3/3、test_ut_fail 7/7、test_str_slice 11/11、test_splitter 23/23，故意失败断言正确报 `1024 != 9999` 且 exit 1），存量 36 文件的测试本身全部健康；lit 246/246 全绿。**遗留注意**：新增 ut 文件 RUN 行务必用 `koala --test %s` 或 `koala %s ++ --test`，切勿写 `koala %s --test`
- [x] **`++` CLI 新语义下 test-ir / test-run / test-kl 全量 RUN 行排查**（2026-10-23）：`++` 分隔符是昨日新增的 CLI 约定（`args.c` `kl_parse_args` 四阶段解析：STAGE 1 只认**文件前**的选项（getopt `+` POSIX 模式）；文件后到 `++` 前一律作为**脚本 argv**；`++` 后再解析 VM 选项且未知选项报错），上述 36 个 ut 坏 RUN 行是历史遗留而非本来错误。逐一排查三目录所有非 koalac 的 RUN 行形式：`koala --dump=... %s` ×3（test_bug_normalize/test_call/test_bool）✓ 有效；`koala %s ex2.toml`（test_valist_for）✓ ex2.toml 是有意的脚本参数；`! koala -c %s`（test_to_str_and_str）✓；`koala %s ++ ...` ×30+ ✓；`koala %s | filecheck %s` ×49 ✓。**发现 2 个静默失效**：`test_misc_2.kl` 的 `koala %s --package-name=foo` 与 `test_sccp.kl` 的 `koala %s --ssa`——选项落入脚本 argv 被忽略（包名未设置 / SSA 管线未启用），因 CHECK 只查程序输出而一直假绿；均改为 `koala %s ++ <opt>` 形式，实测选项生效且 filecheck 通过，lit 246/246 全绿
- [ ] **窄整型字面量丢符号**（不是 cast 问题，但它卡住了两个 cast 用例）：超出 ±2048 的负 int16 / int32 字面量变成无符号值。根因在 `isel.c` 的 `get_const_op`——`[-2048, 2047]` 内发 `OP_LOAD_INT_IMM`，之外走 `OP_LOADK`，而 LOADK 路径没有做符号扩展。
  - 因此 `test_cast_int.kl` 中 `test_i32_to_f32(-100000)` 与 `test_i32_to_f64(-100000)` 仍以注释 + `// expect:` 保留，是仅剩的两个未启用跨族用例；同文件末尾另有一组字面量本身的 worklist。
  - `test_const_int_cast_failed.kl` 里 `let a int32 = -40000` / `-100000` 两处目前"因错误的原因"通过（字面量先变成大正数，再触发常量越界报错）；字面量修好后诊断文本不变，**无需改动**。
- [ ] **高位 uint64 print 成负数**：print 一个最高位为 1 的 uint64 输出有符号值（`UINT64_MAX` → −1）。疑似运行时打印问题，非 cast 问题；`test_cast_uint.kl` / `test_cast_float.kl` 的 CHECK 暂按实测值写。

---

## 14. 数值类型字符串解析 API（from_str 家族）

> 2026-09-12 建立。设计定案（见 `Koala_Design_Overview.md` §5.2 / §6），**实现未开始**。
> 2026-10-24 补充见 §22：ctor 收 `str` 的语义待作者裁定；`int64("100")` 的失败形态已从
> `not_impl` 变为编译期 `irgen.c:442: Not Yet Implemented`（abort），原记录已过时。
>
> **Update (2026-10-31)**: `str` and `base` are removed from the `int64` / `uint64` constructors
> (`number.kl`); `int("12")` / `int64("ff", base = 16)` are now a type error at the call site
> instead of the `emit_int_call()` NYI abort, and `emit_int_call()` only casts a number.
> `float64.__init__` keeps `str`, because `float64("inf")` / `"nan"` / `"3.14"` string literals
> are folded at compile time (the only way to write inf / nan today, see §20 / §22); a runtime str
> already errors with "use 'to_float()'". Still open: `from_str_or` / `try_from_str` (no base-aware
> parsing exists now: `str.to_int()` is base 10 only).

### 14.1 现状（源码核实）

- `int64.__init__(x int64 | uint64 | float64 | str, base = 10)`、`uint64` 同形、`float64.__init__(x int64 | uint64 | float64 | str)`——三个主类型构造器 union 仍含 `str`。
- `intobj.c` 中 `int_init` / `str_to_int` 整段被注释（lines 48-119）——构造器路径上的 str 解析**未实现**，`int64("100")` 当前命中 not_impl。
- `str.to_int` / `to_int_or` / `to_float` / `to_float_or` 在 `strobj.c` 中是活的（`_str_to_int` 等，调用 `str_to_int` helper）——`_or` 家族的既有先例。
- 窄类型（int8/16/32, uint8/16/32, float16/32）的 `__init__` 已是纯数值 union（`@intrinsic`），无需改动。
- src 中不存在任何 `from_str`。

### 14.2 目标形状

每个主数值类（int64 / uint64 / float64）：

```koala
@native
pub func __init__(x int64 | uint64 | float64) {}              // 去掉 str，纯数值 union → cast op

@native
pub static func from_str_or(s str, fallback int64, base = 10) int64 {}

@native
pub static func try_from_str(s str, base = 10) int64? {}
```

- `from_str_or`：主形态，裸返回可内联算术；fallback 必传无隐式默认（nothing hidden）。
- `try_from_str`：检测形态，`T?` + narrowing。
- 无裸 `from_str`；`base ∉ [2,36]` → panic（bug 类）。
- float64 无 base：`from_str_or(s, fallback) float64` / `try_from_str(s) float64?`。

### 14.3 实现清单

- [ ] `number.kl`：int64/uint64/float64 的 `__init__` 去掉 `str`（和 base）（int64 / uint64 done 2026-10-31; float64 kept for the literal fold）；加 `from_str_or` / `try_from_str` static 声明 + doc comment
- [ ] `intobj.c` / `floatobj.c`（uint64 如有独立文件）：实现 `_int64_from_str_or` / `_int64_try_from_str` 等 native（复用 `strobj.c` 的 `str_to_int` / `str_to_float` helper，或抽到公共位置）
- [ ] MethodDef 表注册新 static 方法
- [ ] `intobj.c`：删除注释掉的 `int_init` / `str_to_int` 死代码
- [ ] static 方法的 klc 持久化 / 调用路径验证
- [ ] 测试：`test-run/test_from_str.kl`（from_str_or 各 base、fallback、try_from_str narrowing、base 非法 panic）

### 14.4 与既有 API 的关系

- `str.to_int` / `to_int_or` / `to_float` / `to_float_or`：legacy 冻结不动。将来是否迁移到 `from_str_or` 形态单独立案。
- `int64.to_float()` / `float64.to_int()`（§13.6 NYI 1b/2b）：数值→数值，与 from_str（str→数值）正交，各自独立验证。

### 14.5 设计依据

- minimize-panic 原则（`Koala_Design_Overview.md` §6）
- 显式回退解决安全 vs 易用（§5.2）：fallback 必传 = 失败决策写在调用点 = nothing hidden
- 模式演化：panic+`_or`（旧）→ `_or`+`try_`（新）

---

## 15. 测试文件揭示的 WIP / 未验证特性（2026-10-02 复核：已消解）

> 2026-09-12 建立时的三个挂账文件（test-run/test_nd_list.kl、test-run/test_bytes.kl、test-run/test_io.kl）**已全部删除**，不再存在于 test/。覆盖迁移情况：
>
> - bytes / Buffer 能力 → test-ut/test_bytes.kl、test_buffer.kl（带 RUN 行，`koala --test` 状态码模式）
> - io 流（BytesIO / BufReader / read 协议）→ test-ut/test_bytesio.kl、test_bufio.kl、test_strio.kl 与 test-run/test_io_split.kl
> - 多维下标 `a[i, j]`（原 test_nd_list）→ 用例未保留，特性未落地、不再跟踪
>
> 现状：190 个测试全部带 `RUN:` 行（2026-10-02）；2026-10-03 新增 6 个整型宽度专档后为 196 个，“无 RUN 行 → Unresolved”类挂账仍为零。以下 15.1–15.3 为原记录留档。

### 15.1 多维下标 `a[i, j]`（test-run/test_nd_list.kl）

- 用例：`let a = [[1,2,3],[4,5,6]]; print(a[1,2])`（2×3 嵌套 list 多轴索引）。
- 待验证：是否编译通过；语义是 `a[1][2]` 的糖，还是独立的多轴 `__getitem__`。
- 落地后：补 RUN 行 + CHECK，再考虑写入设计文档 §3 下标族。

### 15.2 bytes 视图与索引（test-run/test_bytes.kl）

- 用例：`bytes(10)`、`len(bs)`、`bs[i] = uint8(96+i)`、`bs.view(3,6)`（切片视图）、`v[0] = 68`（视图写穿原 buf）、`bs.view(0,3).index(90)`（视图内搜索）、`buf.zero()`（清零）。
- §7.2 已记录 test_bytes 为 Unresolved。
- 待验证：view 的共享语义（写穿原 buf）、index 搜索、zero 填充是否全链路通；与设计文档 §8.4「bytes 24 方法；位置式写族」的 view/index 对应关系。

### 15.3 io 模块（test-run/test_io.kl）

- 用例：`import "std/io"`、`io.StringIO(s)`、`io.BufReader(sio)`、`sio.read(buf)` / `br.read(buf)`（读入 bytes 缓冲、返回 int 字节数，`<= 0` 表 EOF）、`while { ... if n <= 0 { break } }`（无条件 while + break 无限循环）。
- §7.2 已记录 test_io 为 Unresolved。
- 待验证：StringIO / BufReader 的 read 协议、与 bytes 缓冲的协作；`while {}` 无限循环形式是否已在其他 passing 测试中实证。

### 15.4 处理结果（2026-10-02）

- 三个文件已删除，无需补 RUN 行；bytes / io 特性以 test-ut 状态码用例重新覆盖，并已入设计文档（§8.4 / §8.9）；
- 多维下标特性未落地，不再跟踪；将来重启时按新用例重开。

---

## 16. 声明-实现欠账清单（@native 无 C 实现 / 类型未注册）

> 2026-10-02 建立。libs/std 全面盘点（40 个 .kl 全文 × src/ 全文 grep 交叉核实）：
> 以下声明已进入 .klc 构建，但运行时侧无实现——调用将命中 `not_impl` 哨兵
> （§7.1：加载期警告 + 调用时 `Exception("... is not implemented.")`）。
> 设计文档只记设计与声明，欠账在此挂账。

### 16.1 容器与类型

- `dict`：25 个方法全部 `@native`；src/ 无 dict 类型实现与注册，且当前 196 个测试中没有任何 dict 用例。
- `hashset` / `TreeSet`：19 / 20 个方法全部 `@native`；无 C 实现、无测试。
- koala.y 仍留 `set_type` 文法残段（行为行已注释），集合字面量语法未启用。

### 16.1b collections 模块实现状态（2026-10-24 全量核对）

> 逐个核对 `libs/std/collections/*.kl` 的 `@native` 声明、`src/objects/collections/*.c`
> 的方法表条数、`src/modules/collections.c` 的 `kl_reg_type` 注册，并实测每个类型能否构造实例。
> `Deque` 本轮已补齐，其余 7 个模块仍是"只有声明、没有运行时"。

**已实现：声明 / C 方法表 / 类型注册 / 测试 四项对齐**

| 模块 | 类型 | @native | C 方法表 | 注册 | 测试 |
| --- | --- | --- | --- | --- | --- |
| `bitset.kl` | `BitSet` | 15 | 15 | ✅ | `test_bitset.kl`（28） |
| `stack.kl` | `Stack[T]` | 8 | 8 | ✅ | `test_stack.kl`（28） |
| `queue.kl` | `Queue[T]` | 8 | 8 | ✅ | `test_queue.kl`（34） |
| `linkedlist.kl` | `LinkedList[T]` + 2 迭代器 | 29 | 29 | ✅ | `test_linkedlist.kl`（50） |
| `deque.kl` | `Deque[T]` + 2 迭代器 | 29 | 29 | ✅ | `test_deque.kl`（67） |

合计 207 用例全通过；`valgrind -q` 下 `test_deque` 零错误；`clang-format` 与编译均无告警。

**未实现：7 个模块 / 85 个 `@native` 方法（C 侧零实现、零注册、零测试）**

| 模块 | 类型（trait） | @native | 实测症状 |
| --- | --- | --- | --- |
| `frozenbytes.kl` | `FrozenBytes : Sequence[uint8]` | 8 | 运行期 `Error: func 'FrozenBytes::__init__' is not implemented!` |
| `frozendict.kl` | `FrozenDict[K, V] : Map[K, V]` | 11 | 运行期同上（`FrozenDict::__init__`） |
| `sorteddict.kl` | `SortedDict[K : Comparable, V] : MutableMap[K, V]` | 19 | 运行期同上（`SortedDict::__init__`） |
| `frozenlist.kl` | `FrozenList[T] : Sequence[T]` | 8 | **编译期崩溃**：`symbol.c:699: Why goes here?` |
| `frozenset.kl` | `FrozenSet[T] : Set[T]` | 11 | 编译期崩溃，同上 |
| `heapq.kl` | `Heap[T : Comparable]` | 8 | 编译期崩溃，同上 |
| `sortedset.kl` | `SortedSet[T : Comparable] : MutableSet[T]` | 20 | 编译期崩溃，同上 |

**顺带发现的编译器缺陷**：实例化"全部方法 `@native` 且 C 侧无实现"的类时，报错方式随
类型参数个数而变——单类型参数的（`Heap` / `SortedSet` / `FrozenList` / `FrozenSet`）在
`src/parser/symbol.c` 的 `UNREACHABLE()` 处崩溃（打印 `Why goes here?` 后 `compilation failed`，
无任何诊断信息）；零参数的 `FrozenBytes` 与双类型参数的 `FrozenDict` / `SortedDict` 则能正常
编译到运行期，调用时命中 `not_impl` 哨兵（§7.1 的预期路径）。即"缺实现"这一种状态有两种报错
方式，单参数泛型那条缺诊断，根因待查（疑在 lro / 接口索引计算）。

**建议实现顺序**（性价比从高到低）

1. `heapq`：8 个方法、无索引访问语义，可独立实现二叉堆；顺带修掉上面那个单参数泛型的编译期崩溃。
2. `frozenbytes` / `frozenlist` / `frozenset` / `frozendict`：只读视图，方法少；但 `FrozenDict`
   依赖 dict 本体（§16.1 仍是欠账），`FrozenSet` 同理依赖 set。
3. `sorteddict` / `sortedset`：39 个方法，需要平衡树或跳表，并依赖 `Comparable` 的跨类型比较分发。

### 16.2 fs / os / 数值方法

- `fs`：C 侧全部已实现——`open` / `read_bytes` / `write_bytes` / `write_str` / `exists` / `is_file` / `is_dir` / `size` / `remove` / `mkdir` / `rmdir` / `list_dir` 与 `File` 方法；`Path` 方法为纯 KL 实现。
- `os`：`system` / `run` + `Process` 8 方法 + `PipeReader` / `PipeWriter` 8 方法——声明活跃、native 全缺（仅 `getenv` / `setenv` / `unsetenv` 已实现）。
- 数值方法族：见 §13.6 新增条目。

### 16.3 命名 / 一致性 WIP（顺手项）

- `MutableSequence.append` 与 `list.append` 已统一。
- `Buffer`（bytebuf.kl，原 `ByteBuf`）已收缩为纯 append 式缓冲、不再声明 `MutableSequence[uint8]`；设计文档 §8.4 已同步。
- seeker.kl 文档提及 `io.StdinReader`——不存在的类（doc-only 残留，示例应改用 `sys.stdin`）。
- `ut.assert_eq[T : Equatable]`（裸约束）与 `assert_ne[T : Equatable[T]]`（完整形参）约束写法不一致，可顺手统一。

---

## 17. The named constant channel of `parse_binary` (to fix)

> Created 2026-10-03, rewritten 2026-11-07. `promote_integer_type` was removed, and with it the wrong
> diagnostic and the wrong log recorded here earlier. The fix is in the design doc **§5.4**. Both
> gaps below are recorded as errors in `test/test-kl/test_number_errors.kl`.

`parse_binary` has two channels that pass the type of `lhs` to `rhs`:

- Literal: `rhs` is a literal and `lhs` is `uint64`.
- Named constant: `rhs` is a constant and `lhs` is not `uint64`; `rhs` is parsed again.

They leave two gaps:

- `x + UINT8_MAX` with `x uint64`: neither channel applies, so the constant stays `int64` and the
  argument of `uint64.__add__` does not match, while `x + 255` is legal.
- `n + INT16_MAX` with `n int8`: the constant is read as `int8` and reported as out of range, while
  `n + 32767` is legal. The type of a right operand is only a hint, so this is too strict.

---

## 18. list 接口全面核对：声明-实现-trait 三方不一致 + 主流语言对齐

> 2026-10-03 建立。审计方法：对 `libs/std/builtin/list.kl` 声明的 **23 个 `@native` 方法**
> 逐一编写单文件探针编译 + 运行实测，并与 C 侧 `src/objects/listobj.c` 的 `list_methods[]`
> 方法表（:239-249，仅注册 8 个：push / pop / len / __str__ / extend / __getitem__ /
> __setitem__ / __getslice__）交叉核对。
> 关联条目：§7.1（not_impl 哨兵）、§9（push/pop @intrinsic 化）、§12（OP_SEQ_SET_SLICE
> 未实现）、§16.3（append 命名已统一）、§4（`in`/OP_CONTAINS 链路已通但 list 缺方法实体）。
> Update (2026-10-28): `in` on list works now; see the `__contains__` row below.

### 18.1 实测状态总表

**可用（8）**：`__init__` / `len` / `__getitem__` / `__setitem__` / `__getslice__`（`l[a:b]`）/
`append` / `pop` / `extend`。另有 C 侧实现但 list.kl 未声明的 `__str__`（`print(l)` 输出 `[1, 2, 3]`）。

**声明 @native 但无 C 实现——运行时命中 not_impl 哨兵（`func 'X' of 'list' is not implemented!`），共 11 个**：

| 方法 | 实测现象 |
|---|---|
| `empty()` | not implemented |
| `index(v, start, end)` | not implemented |
| `rindex(v, start, end)` | not implemented |
| `count(v, start, end)` | not implemented |
| `insert(i, v)` | not implemented |
| `remove(v)` | not implemented |
| `clear()` | not implemented |
| `reverse()` | not implemented |
| `reversed()` | not implemented |
| `copy(start, end)` | not implemented（含带范围形式） |
| `iter()` | 直接调用 `l.iter()` 编译报 `'iter' is not found in instance of class 'list'`——for-in 走内建字节码协议，声明的 @native `iter()` 无实体；且缺 `step` 形参（见 18.2 第 2 条） |

**编译报错 / 崩溃，共 4 个**：

| 方法 / 语法 | 实测现象 |
|---|---|
| ~~`__contains__`（`v in l`）~~ | ~~编译报 `type 'list<j>' does not support '__contains__' method`——§4 的 OP_CONTAINS 全链路已通，但 list 缺 SLOT_CONTAINS 方法实体，**`in` 运算符对 list 完全不可用**~~ **Resolved (verified 2026-10-28)**: `list.__contains__` is declared (`list.kl:46`) and `v in l` works; `test-ut/test_ut_list.kl` (`test_list_contains`) covers it |
| `__setslice__`（`l[0:2] = xs`） | 编译报 `cannot resolve expr's type`（对应 §12 OP_SEQ_SET_SLICE 未实现） |
| `__add__`（`l + xs`） | **崩溃**：`vm.c:50 kl_not_impl_func` 断言失败（`Assertion '(obj)->_type == &cfunc_type' failed`），进程直接 abort，比 not_impl 异常更严重 |
| `__iadd__`（`l += xs`） | 编译报 `inplace assignment is not supported for 'list' type` |

### 18.2 trait 一致性破损（list vs MutableSequence / Sequence / Iterable）

1. **`append` 接口一致性**（§16.3 已定名）：`MutableSequence`（collection.kl）
   要求 `append(value T)`，list 现已使用 `append`；trait 要求的
   `insert` / `remove` / `clear` 仍未实现——list 实际上不满足其声明实现的 MutableSequence。
2. **`iter` 签名不符**：`Iterable` 要求 `iter(step = 1)`，list 声明无参 `iter()`；
   实测 `l.iter(2)` 与 `l.iter()` 均不可用（编译报 `'iter' is not found`）。
3. **`Sequence.__contains__` / `index` / `count`**：trait 要求的三件全部缺失（见 18.1 两表）。
4. **编译器不强制 trait 一致性**：.kl 中声明存在即可通过编译（即使 @native 无 C 实体、
   trait 方法整体缺席），所有问题推迟到运行时暴露。是否加编译期一致性检查，属设计决策，另行拍板。

### 18.3 for-in 迭代的两个 bug（2026-10-03 复核细分）

**Bug A（运行时）：顶层 `__init__` 中 for-in 遍历 Sequence 后，收尾阶段多做一次越界访问**

```kl
// 模块顶层（无 func 包裹）——报错
let l = list[int](10, 20, 30)
for v in l { print(v) }   // 10 20 30 全部正常打印
print("done")             // 打印 done 后抛 Error: list index out of range
```

- 影响面：**仅模块顶层代码（在 `__init__` 中执行）**对 **list / str / bytes**（Sequence
  快速路径）的 for-in；循环体本身全部正常执行，错误发生在 `__init__` 收尾
  （traceback 指向循环后的最后一条语句或裸 `[pc 0006]`）。
- **不受影响**：普通函数（如 `func main()`）内的 for-in 完全正常；顶层对 `range` 的
  for-in 也正常 → 问题定位在**顶层上下文 + GEN_SEQ 快速路径**的收尾边界
  （`index == len` 时多执行一次带边界检查的取元素）。
- 连续多个顶层 for-in 时各循环输出均正常，仅在 `__init__` 末尾报一次错。

**Bug B（编译器）：for-in 的 iterable 不支持内联构造表达式**

`for v in list[int](1, 2) { ... }` 直接编译失败：`irgen.c:2355: Why goes here?`
（UNREACHABLE），顶层与函数内一致。irgen.c 的 for-each 分派只处理
`klr_is_const(it_val)` 下的 range / seq 两类常量（str 字面量 `"ab"` 可迭代），
构造调用等一般表达式落入 `UNREACHABLE()`。施工方向：先求值 iterable 绑定临时变量
再分派，或至少把 UNREACHABLE 换成明确的编译诊断。

### 18.4 与 Python / Rust / Go 的对齐分析

参照系：
- **Python `list`**：append / extend / insert / remove / pop / clear / index / count / **sort** / reverse / copy，加 `len()` / `in` / 切片 / `+` / `+=`。
- **Rust `Vec`**：push / pop / insert / remove / clear / len / is_empty / contains / **sort / sort_by / dedup / truncate / retain / resize** / reverse / extend / first / last / position。
- **Go `slices` 包**：Append / Insert / Delete / Contains / Index / **Sort** / Reverse / Clone / Equal。

**缺少（建议补）**：
1. `sort_by(cmp)` / `key=`——`sort()`（含 `reverse = true`）已实现（2026-11-08，稳定归并排序，
   见 §38 第 7 条），但比较键（`sort_by` / `key=`）仍缺；
2. `append` 与 `push` 的统一（见 18.2 第 1 条）；
3. `first()` / `last()`——Rust 风格便捷访问，可选；
4. `truncate` / `resize` / `retain` / `dedup` / `position`——Rust 风格，可用推导式替代，优先级低。

**冗余 / 值得商榷**：
1. `copy(start, end)` 与 `__getslice__`（`l[a:b]`）功能重叠——Python 的 `copy()` 是无参整体浅拷贝；
   建议 `copy()` 收敛为无参整体拷贝，范围复制交给切片语法；
2. `empty()` 等价于 `len() == 0`，命名建议对齐 Rust `is_empty()` / Go 惯例。

**合理应保留**：push / pop / extend / insert / remove / clear / index / rindex / count /
reverse / reversed / len / 切片 / `+` / `+=`——方向与主流一致，前提是真正实现。

### 18.5 施工优先级

- **P0（崩溃 / 核心功能不可用）**：
  1. `__add__` 断言崩溃——实现之，或至少降级为 not_impl 异常而非 abort（VM 侧防御）；
  2. for-in 两个 bug（18.3：Bug A 顶层 `__init__` 收尾越界；Bug B 内联 iterable 编译失败）；
  3. 实现 `__contains__`，解锁 `in` 运算符（链路已通，只缺 list 方法实体）。
- **P1（trait 契约兑现）**：实现 18.1 的 11 个 not_impl 方法；`__setslice__`（随 §12
  OP_SEQ_SET_SLICE 一并落地）；`__iadd__`；append/push 定案统一；`iter(step)` 签名修正。
- **P2（能力补齐）**：`sort` ✅（`sort(reverse = false)`，2026-11-08）/ `sort_by` 待做；
  `copy()` 语义收敛；`empty` → `is_empty`；`first` / `last`。

### 18.6 测试现状与挂点

- 现有用例只覆盖可用面：`test/test-run/test_list*.kl`（5 个文件）+ `test/test-kl/test_list.kl`
  仅使用 `push`（21 处）/ `len`（6 处）/ 下标赋值 / 切片。test-ut 侧已有
  `test/test-ut/test_list_sort.kl`（10 个用例：`sort` 的空/单元素/升降序/稳定性/前部 pop 后排序/不可比 panic）。
- 每实现一个欠账方法，应在 `test/test-ut/test_list*.kl` 补用例，
  参照 `test/test-ut/test_bytes.kl` 模式（`@test` + `ut.assert_*`，含范围语义与视图/切片交互）。

---

## 19. 退出码模型（待定）

**现状**：`sys.exit(code = 0)` 已实现（`libs/std/sys/__sys__.kl` + `src/modules/sys.c`），
但退出码通道不完整：

- `kl_run_main`（`src/runtime/vm.c`）在异常路径只打印异常，`koala_run_file` 恒返回 0——
  panic 的 Koala 程序在 shell 看来是成功退出。
- `main` 的返回值被丢弃（`kl_run_main` 拿到 `TValue` 只判 `is_error`），`main` 签名不受约束。

**待定项**：

1. **panic 退出码改为非 0**（如 1）：`kl_run_main` 异常路径返回 1，`koala_run_file` 透传。
   否则工具链分不清「正常退出」与「崩溃」，`sys.exit` 的价值被削弱。
2. **是否支持 `func main() int` 作为退出码通道**：C/Go 血统，与「可预期失败用返回值」一致；
   代价是 parser 需加 main 签名约束。保底方案：明确规定 `main()` 无返回，退出码只有 `sys.exit` 一条通道。

**测试**：退出码无法用现有 `--test` 断言，需 lit 层 `sh` 跑独立进程比对 `$?`。

**Update (2026-11-06)**: `koala x.kl` still exits with 0 after a panic, while `koala x.kl ++ --test`
exits with 1 when a `@test` fails. `test-run/test_shift_traceback.kl` panics at the top level, so its
RUN line has no `!`: when item 1 is done, it needs one.

---

## 20. float 的 NaN / Inf 构造途径（已修复）

**已落地方案（2026-10-24）**：`libs/koala/math/__math__.kl` 提供 `pub let` 常量：

```kl
pub let INF = 1.0 / 0.0  // 第 25 行
pub let NAN = 0.0 / 0.0  // 第 28 行
```

使用 `pub let`（而非 `pub const`）是因为 `const` 不接受表达式（实测报 `error: syntax error`），
只接受字面量。`let` 允许编译期常量表达式，且 `pub` 使其成为模块级公开常量。

**使用方式**：`math.INF` / `math.NAN`，负无穷写作 `-math.INF`。

**历史背景**：本条原记录「标准库能判定不能构造」的缺陷，现已被 math 包的 `pub let` 方案解决。
原记录的三条排除路径（`const` 不接受表达式、词法器不识别 `nan`/`inf` 字面量、函数路线被否决）
均已被更简洁的 `pub let` 方案绕过。

---

## 21. `str(float)` 只有 16 位有效数字，导致 json 浮点往返 panic（已修复）

**现状**（2026-10-24 实测复现，**已修复**，见下文「已落地方案」）：`libs/koala/json/float.kl` 的 `format_number`
用 `str(v)` 序列化，而 `str(float)` 只输出 16 位有效数字——16 位不足以唯一命名每个 double，
需要 17 位。后果是 json 序列化极值浮点后，**产出的文本自己解析不回来**：

```
parse_str("2.2250738585072014e-308")   → 成功（DBL_MIN）
  .to_json()                           → "2.225073858507201e-308"
parse_str(上一行那个文本)               → panic: invalid float literal
```

**根因链**：截断后的值小于 DBL_MIN → `strtod` underflow 置 `ERANGE` → `src/common/mm.c:189`
的 `str_to_float` 命中 `if (errno == ERANGE) return 0.0;` → `ok = 0` → `src/objects/strobj.c:576`
的 `_str_to_float` 抛 `invalid float literal`。DBL_MAX `1.7976931348623157e308` 同理
（16 位四舍五入后变大 → overflow → 同样 panic）。

**旁证**：`str(0.1 + 0.2)` 输出 `0.3`，而 IEEE754 真值是 `0.30000000000000004`；
`str(123456789.123456789)` 输出 `123456789.1234568`。

**作者裁定（2026-10-24）：不改 `str(float)`**。理由是改后 `print(0.1 + 0.2)` 会输出
`0.30000000000000004`，可读性损失太大。所以 `src/objects/floatobj.c` 的 `_float_to_buf`
（第 15-40 行，`TAG_FLOAT64` 分支用 `%.16g`）**维持原样**，print 行为不变。

**已查清的技术约束（决定了 json 层无法在纯 .kl 内自行修复）**：

1. `format` 的浮点路径 `format_float`（`src/runtime/format.c:129-155`）**硬编码 `%f`**
   （定点表示），不支持 `e` / `g` 类型字符；`__kl_format__`（同文件 232 行）只在
   `spec.type == 'f'` 时才走 `format_float`。所以 `format("{:.17f}", DBL_MIN)` 会输出
   308 个零开头的定点数，而且 `char tmp[128]` 装不下（被 snprintf 截断）。json
   需要的是科学计数法，拿不到。
2. shortest-round-trip 必须「输出后 reparse 与原值比对」，但 `str.to_float()` 对 `ERANGE`
   是**直接 panic**（`src/objects/strobj.c:576`）而不是返回错误值，所以 .kl 层无法安全地
   做这个比对——恰好在最需要比对的极值上会炸。

⇒ 选了改 C 侧。

**已落地方案（2026-10-24）：给 `float64` 新增 `to_str_exact()`**，语义是「最短可往返表示」，
json 序列化改用它，`print` / `str()` / `fmt()` 仍走 16 位的 `_float_to_buf`，**print 行为完全不变**
（`print(0.1 + 0.2)` 仍输出 `0.3`）。三处落点：

- `src/objects/floatobj.c`：新增助手 `_float_to_buf_exact`（`ASSERT(tag == TAG_FLOAT64)` +
  `for (prec = 15; prec <= 17; prec++)` 试 `%.*g`，用**裸 `strtod`** 比对——不是 `str_to_float`，
  避开 ERANGE panic——相等即 break）与方法 `_float_to_str_exact`，并在 `float_methods[]`
  注册 `"to_str_exact"`。
- `libs/std/builtin/number.kl`：float64 块 `to_str()` 之后加 `@native pub func to_str_exact() str {}`。
- `libs/koala/json/float.kl`：`format_number` 里 `str(v)` → `v.to_str_exact()`（1 行）。

**命名定为 `to_str_exact` 而非 `repr`**：`repr` 是 Python 惯例，但 Python 的 `str(float)` 本身
就是 shortest-round-trip，Koala 这里是刻意让两者不同；`to_str` + `_exact` 后缀既顺着
`to_int` / `to_int_or` 的「基础名 + 后缀」既有先例，也直接点出与 `to_str()` 的区别。`exact`
只承诺可还原、不承诺位数（实现是 shortest，不是无脑 17 位），比 `full` / `precise` 准确。

**只实现 float64**：`.kl` 侧仅在 float64 块声明，而编译器只认声明过的名字，所以共用
`float_methods[]` 的 float32 / float16 / bfloat16 调不到它，C 侧用 `ASSERT(tag == TAG_FLOAT64)`
收窄即可。这也避开了一个坑：float32 的往返比对必须把 `strtod` 结果窄化回 `float` 再比
（否则 `(double)(float)0.1` 永远不等于 `strtod("0.1")`）；将来要给 float32 加，需单独处理窄化。

**未采纳「接受为已知限制」**：改动面只有约 20 行 C + 7 行 .kl，且是纯新增方法、不改任何
现有行为，没有理由留着这个缺陷。

**附（已处理，2026-10-24）**：`_float_to_buf` 的注释原写「`%.15g` avoids exposing binary
floating-point noise such as 3.1400000000000001」而代码实际是 `%.16g`（疑似从 15 改到 16
时漏改），已把注释里的 `%.15g` 改成 `%.16g`。作者要求**只改这一个字符，不在 C 注释里
附加局限说明**——16 位不保证 round-trip 这类分析留在本文档里就够了。

**测试**：已新建 `test/test-ut/test_float_round_trip.kl`（7 用例，实测 **7 passed / 0 failed**），
覆盖：常见值不增位（`3.5`→`"3.5"`、`1.0`→`"1"`、`1e30`→`"1e+30"`）、15 位不够时退 16 位
（`1.0/3.0`）、DBL_MIN 与 DBL_MAX 的文本级往返、`0.1+0.2` 与 `0.3` 被区分开（`to_str()` 把
两者都映成 `"0.3"`）、非有限值不 abort、跳量级往返。

两条测试设计纪律：**往返断言一律用 `ut.assert_eq` 而非 `assert_float_close`**（默认 eps=1e-9
会把 DBL_MIN 的漂移绿着放过，而这正是该方法存在的唯一理由）；**NaN 不断言精确文本**
（符号位 implementation-defined，glibc 可能输出 `"-nan"`，故只断言 `index("nan") != -1`）。

**刻意未写 subnormal 往返**：最小 subnormal `4.9406564584124654e-324` 的 `to_str_exact()` 能
正确产出 17 位文本，但 `to_float()` 解析它时 glibc 置 `ERANGE`、被 `mm.c:189` 一律拒绝而 panic。
这是 `str_to_float` 的严格策略（上文根因链的第 ③ 因素），不是 `to_str_exact` 的缺陷；
DBL_MIN 是规格化数，解析它不触发 ERANGE，所以那条用例是干净的。

**由此暴露一个独立的待定设计问题**：`str_to_float` 对 `ERANGE` 一律拒绝，**不区分 overflow
与 underflow-to-subnormal**。C 标准里后者置 `ERANGE` 是**可选**的，多数实现接受 subnormal
而不报错，glibc 会报。结果是 Koala 无法通过文本解析得到任何 subnormal 值（`5e-324` 这类
输入直接 panic），而 subnormal 在 IEEE754 里是合法值。是否放宽（underflow 到 subnormal 时
接受而不报错）需作者裁定；若放宽，`to_str_exact` 的 subnormal 往返用例即可补上。

json 侧原先「刻意未写」的 DBL_MIN / DBL_MAX 文本级往返现在**可以补了**（缺陷已修），尚未补。

---

## 22. 数值构造器接收 `str` 的语义（`float64("...")` / `int64("...")`）（待定）

> 2026-10-24 建立。本次讨论记录：float64 的**字面量路径已落地**（编译期折叠，见 22.1），
> integer 侧仍是空实现，且是「放行后崩」；ctor 收 `str` 的整体语义
> （「字面量语法糖」vs「运行时解析器」）待作者裁定。
> 关联 §13（数值 cast）、§14（from_str 家族）、§20（NaN/Inf 构造途径）、§21（`str_to_float` 严格策略）。

### 22.1 当前实测状态（2026-10-24）

**float64 字面量路径（已实现）**：

- `float64("inf")` / `float64("-inf")` / `float64("nan")` / `float64("3.14")` 在编译期校验并
  **折叠为 float 常量**：非法字面量报编译错误 `invalid float literal '...'`；运行时 `str` 实参
  报编译错误并提示改用 `to_float()`。inf / -inf / nan 走 interned tag（IR 单条 `ret_tag`），
  其余走常量池（`ret_const`）。溢出沿用 `strtod` 语义（`"1e400"` → `+inf`），拼写大小写不敏感。
- 落点在 `src/parser/irgen.c`：`parse_str_to_float` + `emit_float_call`（原 `NYI()` 路径已移除）。
- 测试：`test/test-run/test_float_from_string.kl`（IR 折叠 + 运行时输出）、
  `test/test-kl/test_float_from_string_failed.kl`（4 个负例）；全套 lit 253/253 通过。
- 所以 float64 目前只是**半实现**：字面量可用，运行时 `str` 不可用（被刻意拒绝，引导到 `to_float()`）。

**integer 侧（未实现，且语义层放行）**：

- `.kl` 侧 `int64.__init__(x int64 | uint64 | float64 | str, base = 10)`、`uint64` 同形
  （`libs/std/builtin/number.kl`）——语义层因 union 含 `str` 而放行。
- 实测 `int64("255")` / `int64("255", base=16)` / `uint64("7")` / `int64("-12")`：编译直接命中
  `src/parser/irgen.c:442: Not Yet Implemented`（`emit_int_call` 的 str 分支）——**签名承诺了
  `str`，实现却 abort**，这是缺陷，不只是设计问题。窄类型（int8/16/32 等）union 不含 `str`，
  语义层就会先拒，无此问题。
- `test/test-run/test_int64_complete.kl:159-161` 的三个用例仍是注释状态。

**已有运行时能力（与 §14 / §21 同源）**：

- `str.to_int()` / `to_int_or()` / `to_float()` / `to_float_or()` 可用（`src/objects/strobj.c`），
  失败时 `raise_exc_str`（`to_int` → `invalid integer literal`，`to_float` → `invalid float literal`）。
- 底层 helper `src/common/mm.c:163` `str_to_int`：**固定十进制**（`strtoll(..., 10)`）、严格全消费、
  `ERANGE` 即失败；`src/common/mm.c:189` `str_to_float`：额外**拒绝 inf / nan**，`ERANGE` 一律失败
  （subnormal 也被拒，见 §21 末尾）。`strtoll` / `strtod` 语义还带来**空白不对称**：
  `" 12"` 通过（前导空白被跳过）、`"12 "` 失败（未全量消费）。
- 于是出现**同一值两种写法两种结果**：`float64("inf")` 编译通过，而 `"inf".to_float()`
  与 `float64(s)`（`s = "inf"`）在运行时被拒。

### 22.2 需作者裁定的设计点

（每条给出选项与建议，讨论用，均未落地）

1. **定位**：ctor 收 `str` 是「字面量语法糖」（编译期校验、失败 = 编译错误），还是「运行时解析器」
   （任意 `str` 值、失败 = 运行时 panic）？**建议两者都要**：字面量走折叠（零成本 + 早报错），
   运行时走解析；否则签名与能力不一致（即 22.1 的现状）。
2. **运行时失败语义**：panic（与 `to_int` / `to_float` 一致）／返回 0 / NaN（静默错误，危险）／
   只保留 `_or` 与 `try_from` 变体。**建议 panic**，另由非 panic 变体承载可恢复场景。
3. **inf / nan 的一致性（本议题最尖锐处）**：`float64("inf")` 编译期通过，但运行时解析路径
   （`"inf".to_float()`，json / toml 输入校验依赖它）是拒绝的。选项：
   (1) **分层**——字面量路径以源字面量语法为准（接受 inf / nan），运行时路径以安全解析为准
   （拒绝），需要文档说明 + 另开显式入口；
   (2) 运行时也放宽 —— **破坏性**，json / toml 会接受非标准输入；
   (3) 编译期也收紧 —— 推翻本轮需求，不成立。
   **建议 (1)**，但必须明确「字面量 ≡ 源字面量」的定位，并保留一个运行时接受 inf / nan 的显式 API。
4. **`base` 参数**：只允许编译期常量（折叠可覆盖，但 `int64(s, base)` 只完成一半）／
   允许运行时值（需运行时解析器接收 base：范围 2–36，`base = 0` 表示按 `0x` / `0b` / `0o`
   前缀自动识别；非法 base：常量 → 编译错误、变量 → panic）／从 ctor 移除 base、只留
   `str.to_int(base)` 形态。**建议允许运行时值 + `base = 0` 前缀语义**；下划线 `1_000` 不进语言层
   （toml / json 自带的清理层负责），且对非 `str` 实参传 base 应报错（避免 `int64(5, 16)` 被静默接受）。
5. **位宽与范围**：只有 64 位类型收 `str`（窄类型让用户显式套一层，溢出自行负责）／
   所有数值类型都收（需为每个类型定义越界语义：`int8("300")` 编译期 = 编译错误、运行时 = panic）。
   **建议先只做 64 位，但接口按全类型设计**（先在 64 位域解析，再做一次「目标位宽范围检查 +
   转换」的独立步骤），第二步只是多接一层，无需推翻解析层。
6. **`uint64` 的负号与上界**：**建议 `uint64("-1")` 直接报错**（不做取模回绕，回绕是静默陷阱）；
   且 `uint64("18446744073709551615")` 必须通过 ⇒ 需要**无符号解析路径**，
   不能复用有符号解析（否则合法大值被误判溢出）。
7. **严格性与空白**：现状是 `strtoll` / `strtod` 语义（见 22.1，前导空白通过、尾随失败，**不对称**）。
   **建议统一为**：先 trim 两侧 ASCII 空白，再要求全量消费；空串 / 纯空白失败；整数仅
   `[+-]?[0-9]+`（按 base）；浮点 `[+-]?(digits[.digits][e±digits] | inf | nan)`，尾随垃圾
   （`"12abc"`、`"3.14abc"`）失败。
8. **折叠触发范围**：只折叠**直接字面量**，还是也折叠常量变量 / 常量表达式？**建议只折叠直接
   字面量**——常量传播类折叠收益小，却会把「哪些写法在编译期报错」变成隐式规则。
9. **折叠层与运行时层必须同源**：两层若各写一套解析规则，必然在 `"1e400"` / `"-0"` / 空串 /
   subnormal 等边界漂移，出现「const 与非 const 结果不同」的最坏情况。**建议**解析规则只有
   一份事实来源（共享同一段实现），并用同一份边界用例表同时驱动两层，断言 const 与非 const 等价。
10. **与 §14 `from_str` 家族的关系（必须一起裁定）**：§14 的定案是**把 `str` 从 ctor union 移除**，
    改用 `from_str_or` / `try_from_str`（minimize-panic + fallback 必传）。若采纳 §14，则本节的
    ctor 路径最终可能被删掉、只保留「字面量折叠」，甚至折叠也一并迁到 `from_str`；
    若不采纳 §14，则 ctor 收 `str` 就成为正式入口。**两个方向一起定，否则会做出两套 API。**
11. **别名**：`int` 已被当作 `int64` 使用（`test/test-run/test_float_cast_4.kl:3`、
    `libs/std/builtin/str.kl:138`）；`float` 是否有同等级别名**未核实**。文档需写明究竟哪几个
    类型名收 `str`。

### 22.3 候选整体方案

| 方案 | 内容 | 代价 |
|---|---|---|
| **A 仅编译期折叠** | 字面量折叠 + 编译期校验；运行时 `str` = 编译错误（= 当前 float64 行为） | ctor 的 `str` 是半成品；`int64(s, base)` 形同虚设；用户被迫改用 `to_int()` / `to_float()` |
| **B 折叠 + 运行时（倾向）** | 字面量折叠；运行时 `str` 走原生解析（panic）；配套 `_or` / `try_from` | 需补运行时解析入口（含 base、无符号）；须解决第 3 / 9 条一致性；若动 std 签名则需重新生成并入库发行产物 |
| C 只走运行时 | 一律运行时解析，取消折叠 | **倒退**：字面量非法从「编译错误」退化为「运行时 panic」；`float64("inf")` 又回到不支持 |
| D 只留 §14 的 from_str 家族 | ctor 去掉 `str`，全部走 `from_str_or` / `try_from_str` | 破坏性（删签名元素）；`float64("inf")` 诉求需换写法 |

### 22.4 若要落地（建议顺序，供讨论）

1. 先定规范：解析语法 + 失败语义 + 边界值表（inf / nan、base、空白、溢出、subnormal）。
2. 统一底层解析实现，让同一份用例表同时驱动折叠层与运行时层（第 9 条）。
3. 补运行时入口：整数需要带 base 的能力（现有解析固定十进制）；无符号需独立路径；
   失败信息与既有 `invalid integer literal` / `invalid float literal` 对齐。
4. 接通 ctor 的运行时分支；`base` 为变量且落在区间外时 panic。
5. 若扩展 `str.to_int(base = 10)` 这类签名，标准库发行产物需重新生成并入库（与 §21 同类成本，
   注意那是入库的二进制产物）。
6. 测试矩阵：值域边界 × 调用场景（直接调用 / 返回 / 默认参数 / 泛型 / 常量声明）×
   两个后端结果一致 × 错误信息文本。
7. 文档：把「字面量等价」与「运行时解析」的差异（尤其 inf / nan）写成用户可见说明。

### 22.5 作者裁定（待填）

- （待讨论：先定第 1、3、10 三条，其余可跟随）

---

## 23. datetime 固定偏移时区（待设计）

`libs/koala/datetime` 的 `Date` / `Time` / `DateTime` 三类已收工并全绿（构造校验、`to_str`/`fmt`、
`from_str`、比较、`hash`、`+`/`-` 与 `diff`、`Date.today`、`DateTime.combine`/`replace`、
`timestamp` 家族、`now`/`to_instant`/`from_instant`）。**唯一未落地的设计缺口是固定偏移时区**
（作者 2026-10 决定排到最后，届时单独拍板）。

当前 `DateTime` 隐含 UTC，无偏移字段、不输出 `+08:00`/`Z`、无从偏移串解析、无 `astimezone()`。
现状仅靠 `to_instant`/`from_instant` 与 `std/time` 的 realtime(UTC) 时钟桥接。

### 23.1 约束前提

- Koala `Instant`/`sys_clock` 全走 UTC，**无本地时区探测、无命名时区数据库**（`ZoneInfo`/tzdata/DST 出局，已定）。
- 因此只能做「UTC + 显式固定偏移」，对齐 Python `timezone(timedelta(...))`，非 `tzinfo` 全功能。
- 纯 Koala 实现，零新增 C native（沿用「能用 koala 实现就用 koala」约束）。

### 23.2 两个待拍设计点（作者开工前定）

1. **类型形态**：
   - A 新建独立 `Timezone` 类存固定偏移（对齐 Python `tzinfo`）；`DateTime` 加可选 `tz Timezone?` 字段，`nil` = naive。可区分 naive vs UTC-aware。
   - B 直接在 `DateTime` 加 `offset_seconds int`（0=UTC）。更轻，但无法区分 naive 与 UTC-aware。
2. **本轮范围**：
   - 小步：先做建模 + `to_str`/`from_str` 读写偏移（`+08:00`/`Z`）+ 构造带 tz；比较/`astimezone` 留下一轮。
   - 大步：一次做完 aware/naive 比较规则（跨时区按绝对时刻、naive 不与 aware 比）+ `astimezone()` + 全部测试。

### 23.3 备注

- 落 `from_str` 通用 ISO（含 `Z`/`±HH:MM`）与时区解析应与此块合并考虑（当前 `from_str` 定案为严格模式，仅逆向 `to_str`）。
- `Timezone` 类型若新增须遵循包内命名纪律（类名不重复 `datetime` 前缀）与 `pub let` 只读字段约定。

---

## 24. 运算符按操作数类型重载（`-` 同类相减）——编译器级，待讨论

**诉求**：Python 的 `datetime - datetime → timedelta` 与 `datetime - timedelta → datetime` 共用 `-`
运算符，靠操作数类型区分。Koala 目前**不允许同名 dunder 运算符方法按参数类型重载**
（写两个 `__sub__` 触发 `error: redefinition of '__sub__'`，实证于 `libs/koala/datetime/date.kl`、
`datetime.kl`），`a - b` 只能绑**唯一一个**签名。

**当前库侧妥协**：`-` 绑 `±Duration` 移位，两值求差降级为具名方法 `diff(o) -> Duration`；
`Date`/`DateTime` 皆如此。功能不缺，但与 Python 心智不一致，`diff()` 是明显的语言限制外溢。

**为何属编译器级而非库改动**：要让 `dt2 - dt1` 也走 `-`，需 parser/type 层支持「同一运算符方法
按实参类型分派到不同签名」，是语言机制扩展，不是 datetime 能自行解决的。

**状态**：本轮不动，作语言待办登记；是否实现、以何形态实现（重载 vs 多分派表 vs 其它）待后续讨论。
关联：§23 时区落地后若引入 `Timezone`，`DateTime` 上的运算符组合会更复杂，宜一并纳入本节权衡。

---

## 25. Koala 标准库 vs Python 标准库：场景 × 热度对比（现状盘点，非待办）

本节是对 Koala 已交付库面与 Python stdlib 的横向盘点，按「使用场景」分离、按「使用热度」（真实项目里的 import 频率）分档。**收录判据沿用 Koala 哲学**：「能组合就不建包 / 非核心不加」，故大量 Python 模块的缺席是有意取舍，不等同缺口；本节末尾单独区分「真缺口」与「按设计不缺」。

**Koala 库清单基线**（以 `libs/{std,koala}/__ws__.toml` 的 packages 为准）：

- `std/builtin`：str / bytes / ByteBuf / int64·float64·uint64 / bool / list / dict / HashSet / tuple / range / slice / enumerate / zip / Formatter / Exception / TypeObject / Number
- `std/collections`：BitSet / deque / stack / queue / heapq / linkedlist / sorteddict / sortedset / frozenlist·dict·set·bytes
- `std` 系统层：fs（Path·文件）、io（BufReader/BufWriter/BytesIO/StringIO/WordScanner）、os（run/system/env/进程）、sys、time（Instant/Duration/now/sleep）、ut、prelude
- `koala` 能力包：json / toml / math / random / datetime / wombat

### 25.1 超高频 ★★★★★

| 场景 | Python 模块 | Koala 对应 | 覆盖 |
|---|---|---|---|
| 容器/数据结构 | list, dict, set, tuple, collections(deque/Counter/defaultdict/OrderedDict) | list/dict/HashSet/tuple + deque/stack/queue/heapq/linkedlist/sorteddict/sortedset/frozen*/BitSet | **强于** Python std（sorted/frozen/BitSet 内置；Python 需第三方 sortedcontainers/bitarray） |
| 字符串/字节 | str, bytes, string, textwrap | str / bytes / ByteBuf / Formatter | 齐；`in` 代替 contains（作者裁定）、format("{}",x) 承担 f-string 语义 |
| 文件/路径 I/O | open, os.path, pathlib, io, shutil, tempfile | fs.Path·文件、io 读写器/BytesIO/StringIO | 核心齐；shutil/tempfile 判为可组合，不建包 |

### 25.2 高频 ★★★★

| 场景 | Python 模块 | Koala 对应 | 覆盖 |
|---|---|---|---|
| 日期时间 | datetime, time, calendar, zoneinfo | **datetime**(Date/Time/DateTime 全功能) + time(Instant/Duration) | 无时区部分已齐；zoneinfo/calendar 缺（时区见 §23 待设计） |
| 数据序列化 | json, csv, pickle, xml, configparser, tomllib | **json / toml**（typed accessor 已补） | json/toml 齐；csv/xml/pickle/configparser 缺（csv 判可组合，xml/pickle 非核心） |
| 数值/数学 | math, statistics, decimal, fractions, random | **math / random** | 常用数学齐；statistics/decimal/fractions 缺（判第三方/可组合） |
| 正则 | re, difflib | **无** | ⚠️ **真缺口**——超高频、需专用引擎、无法用现有 API 组合 |
| 日志 | logging | **无** | ⚠️ 热度高；是否建包按 Koala 哲学可能倾向第三方 |
| 异常/错误 | try/except, traceback | panic + Exception（**刻意不给 try/catch**） | 设计取舍，非缺口 |

### 25.3 中频 ★★★

| 场景 | Python 模块 | Koala 对应 | 覆盖 |
|---|---|---|---|
| 系统/环境 | os, sys, platform, signal, subprocess | os(run/system/env) / sys | 齐；os.run 直执行不经 shell |
| 迭代/函数式 | itertools, functools, operator | for/range/zip/enumerate（语言级） | 基础齐；itertools/functools/operator 缺（判可组合） |
| 内省/类型 | inspect, types, typing, dataclasses, enum | TypeObject / is / as（静态类型 + 编译期泛型推导） | 静态语言路线，运行时 typing/dataclass 无对应物（范式差异，非缺口） |
| 测试 | unittest, doctest, pdb | **ut**(@test/@should_panic/assert_*) | 单测框架齐；doctest/pdb 缺 |
| 并发/异步 | threading, asyncio, multiprocessing | 运行时有**绿线程调度**，但 std **无并发 API 面** | ⚠️ 半成品：VM 已具备，语言层 API 未暴露 |

### 25.4 低频 / 生态型 ★★

| 场景 | Python 模块 | Koala | 说明 |
|---|---|---|---|
| 网络 | socket, http, urllib, ssl | `std/socket`（TCP / UDP / raw）、`koala/tls`（基于 OpenSSL 的 TLS 客户端）、`koala/http`（HTTP/1.1 客户端，报文解析用 vendor 的 llhttp，URL 解析只有它的 `Url`） | 2026-11-08 更新：无服务端 TLS / HTTP，遗留见 §80、§81 |
| 加密/编码/压缩 | hashlib, hmac, base64, zlib, gzip, codecs | **无**（`.hash()` 是 FNV-1a 内部哈希，非加密） | base64/zlib 实用但非语言核心 |
| 命令行解析 | argparse, getopt, shlex | `koala/cli`：argparse 的 API（`ArgumentParser` / `add_argument` / `add_subparsers` / `add_parser` / `parse_args`，Python 的帮助与报错格式），取值 `(args.get("x")! as str)!`；`type_="int"`（`type` 是关键字），无 `store_const`；`koala/shlex`：shlex 的 `split` / `join` / `quote` 与 `shlex` 词法器，结果与 CPython 3.14 一致，出错返回 nil | 2026-11-08 更新，取值的易用性见 §82 |
| Interactive input | readline, getpass, curses | `koala/readline`: the API of Python's `readline` (`input`, the history and its files, `set_completer` with a `Completer` object, `parse_and_bind` / inputrc, the hooks), written in Koala over a raw-mode shim, checked against GNU readline 8.3; `input` returns nil at the end of the input and `""` on Ctrl-C instead of raising; no vi mode, no getpass, no curses | added 2026-11-08, leftovers in §87 |
| 数据库 | sqlite3, dbm | `koala/sqlite3`：sqlite3 的 API（`connect` / `Connection` / `Cursor` / `Row`，`?` 与命名参数、`executemany` / `executescript`、隐式 `BEGIN`、语句缓存），vendor 的 SQLite 3.54.1；出错返回 nil + `sqlite_errorcode()` 而非异常，行是 `Row`；无 dbm、无 MySQL 等服务端驱动 | 2026-11-08 新增，遗留见 §85 |
| 结构化配置 | enum, dataclasses | 无 | 范式差异 |

### 25.5 综合判断

**真缺口（三类）**：

1. **正则 `re`** — 超高频、需专用引擎、不可组合替代，最该补。
2. **时区（zoneinfo 对应物）** — datetime 已列 §23 待设计（受限于无 tzdata/DST 基建）。
3. **并发 API 面** — 运行时绿线程已就绪，缺语言层暴露（threading/asyncio 对应物），属「半成品」而非「没做」。

**已强于 Python std 的地方**：sorted/frozen 系列、BitSet、heapq 全部内置；datetime 单个纯 .kl 包即覆盖 Date/Time/DateTime/from_str/replace/timestamp/hash。

**按设计不缺（不得当缺陷）**：无 try/catch（刻意）、无运行时 typing/dataclass（静态类型 + 编译期泛型已覆盖）、csv/itertools/statistics/socket/argparse/base64（可组合或第三方生态承担）。

**一句话**：Koala std 在「数据结构 + 文件 I/O + 序列化 + 日期 + 数学」几条超高频线上已达甚至超过 Python std；短板集中在正则、并发 API、网络/加密生态——前两者是语言层真待补，后者按哲学本应留给第三方。

---

## 26. `build_intern` 操作数个数溢出 `nargs:8` 上限——前端缺诊断，待施工

**编码约束**：`OP_BUILD_INTERN` 的指令格式为 `| op:8 | rd:8 | tag:8 | nargs:8 |`
（见 `include/common/opcode_list.h:1685` 的格式注释），操作数个数只占 8 位，
可表示 0–255；即 `@map` / `@list` / `@tuple` 一个字面量最多 255 个元素。

**VM 侧取用**：`include/runtime/vm_ops.h:546` 直接 `imm = I_VAL(inst, 0, 8)`
交给 `do_build_intern()`，不做范围校验。

**待办**：当字面量的元素个数（`build_intern` 的 `num_opers`）越过该字段可表示的
范围时，编译器前端应在生成 `build_intern` 指令处报编译错误，错误信息需带字面量
位置与元素个数；不得让溢出 / 截断后的 `nargs` 进入字节码交给 VM。

**现状**：isel 生成路径（`src/parser/backend/isel.c` 的 `isel_lower_build_intern()`
及指令编码处）按 `insn->num_opers` 直接遍历，未做上限检查；parser / 诊断层亦无
对应报错。属待施工项。

**关联**：§12（容器协议指令族）——若后续给 `build_intern` 换用更宽的指令格式，
本节随之失效。

---

## 27. 全常量 dict 字面量直接作实参时报 `modobj.c: Not Yet Implemented`

**Status (2026-11-01)**: resolved. A map literal is now always `build_intern @map` (§37), never a
const-pool dict, so `print({1: 2})` and `print(format("{}", {"a": [1]}))` work. The "bind with `let`
first" note in `test/test-ut/test_ut_dict_literal.kl` is no longer needed.

**现象**（2026-10-25 实测，Debug 构建，flag `++`）：

| 写法 | 结果 |
|---|---|
| `print(format("{}", {1: 2}))` | `modobj.c:269: Not Yet Implemented`，程序中止 |
| `print({1: 2})` | 同上 |
| `let d = {1: 2}; print(format("{}", d))` | 正常，输出 `{1: 2}` |
| `let x = 2; print(format("{}", {1: x}))` | 正常，输出 `{1: 2}` |
| `func keep(d dict[int, int]) dict[int, int] { return d }` 后 `print(format("{}", keep({1: 2})))` | 正常，输出 `{1: 2}` |
| `print(format("{}", [1, 2]))` | 正常，输出 `[1, 2]` |

**范围**：`print` 与 `format` 都受影响。受影响的写法都是「全常量 dict 字面量直接出现在实参
位置」；含变量的 dict 字面量、先绑定到 `let` / `var` 的、经一层函数调用传入的均正常，list
字面量直传也正常。

**影响**：dict 字面量不能直接当实参用，必须先绑定局部变量再传。`test/test-ut/test_ut_dict_literal.kl`
的头部注释已按此约定记录，用例全部走「先 `let` 再传」。

**待办**：修掉该路径，使 dict 字面量在实参位置与先绑定后的行为一致；修好后上面两行报错的写法
应能直接通过。

**关联**：§26（`build_intern` 操作数个数上限）同属 `build_intern` / const-pool 折叠区域。

---

## 28. os.run 子进程管道：大 payload「先写后读」死锁 + 无 finalizer 致 fd 泄漏与 zombie

**Status (2026-10-31)**: partly fixed; the `communicate()` and SIGPIPE-restore tests pass.

- Item 1 is done: `Process.communicate(input bytes? = nil) (bytes, bytes)` (`src/modules/os.c`) runs one
  `poll()` loop over stdin / stdout / stderr, closes all three parent-side ends and reaps the child.
  `os_native_lib_init` now ignores SIGPIPE (as `socket` already did), so a child that stops reading makes
  the write fail with EPIPE instead of killing the process. An ignored signal stays ignored across exec,
  so `os.run` (`posix_spawnp`) and `os.system` (now `posix_spawn` of `/bin/sh -c` instead of system(3),
  still returning the raw wait status) pass `POSIX_SPAWN_SETSIGDEF` for SIGPIPE (`init_spawn_attr`), as
  Python's `restore_signals` does. Unlike system(3), `os.system` does not ignore SIGINT / SIGQUIT in the
  parent while the shell runs. New tests are in `test/test-ut/test_run_pipe.kl`
  (`test_communicate_*`; `test_communicate_large` sends 300000 bytes; `test_*_sigpipe_default`, `test_system_exit_code`). `test_run_read_all_large` still uses
  write-all-then-`read_all()` and stays `@skip`: its "zombie" note does not match the 17000-byte payload
  (below the deadlock limit); find out why before removing the skip.
  **Found (2026-11-01)**: the limit is not always 2 x 64 KiB. Once a user's pipes use more than
  `/proc/sys/fs/pipe-user-pages-soft` (16384 pages), Linux gives new pipes only 8 KiB (measured with
  `F_GETPIPE_SZ` while 40 tests ran in parallel), so the two pipes hold 16384 bytes and 17000 bytes
  written before reading deadlock: 1 hang in 40 parallel runs of `test_write_chain.kl`, which was
  `// SKIP` for this. It now sends its 17000 bytes with `communicate()` and runs again (120 parallel
  runs, no hang). `test_run_read_all_large` had the same pattern and the same cause; it now has the
  child make its 17000 bytes (`yes ... | head -n 1000`) and only reads them with `read_all()`, and
  runs again (no `@skip`; 120 parallel runs of `test_run_pipe.kl`, no hang).
- Item 2 is wired but cannot run yet: RawReader / RawWriter have `_free = raw_free` (closes the fd if
  `close()` was never called) and Process has `_free = process_free` (`waitpid(WNOHANG)`: reaps a child that
  has already exited, never blocks on a running one). `kl_free_instance()` calls `tp->_free` before `mm_free`,
  and `TypeObject._free` is documented as a finalizer that does not free the memory. But nothing frees
  objects at run time today (no explicit `free`, no compiler-inserted free, the CMS GC is off; §12 of the
  Design Overview), so these finalizers never fire. Until a free path exists, leaks ① / ② remain for code
  that does not call `close()` / `wait()`; `communicate()` is the leak-free path.
- Left: `Process` holds its three pipe objects as raw `Object *` with `_nfields = 0` and no `_mark`, so a
  tracing GC would not see them; add a `_mark` when the GC is turned on. A still-running child whose
  Process is freed is not reaped (it needs SIGCHLD handling or a reaper, still to be decided).
- Same for `fs.File` (`src/modules/fs.c`, `file_type`): no `_free`, so a File that is never `close()`d
  leaks its fd. Add a `_free` that closes the fd, together with the free path above.
- All memory / finalizer items of this section wait for a run-time free path (explicit `free`,
  compiler-inserted free, or the GC); they are tracked here only, not in work plans.

**现象一（死锁）**：对 `os.run` 返回的 Process，若「先把 stdin 全部写完，再 `read_all()` 读 stdout」，
当 payload 超过管道容量时死锁——父进程阻塞在 `write(stdin)`、子进程阻塞在 `write(stdout)`，两侧永久睡眠
（`ps` TIME=00:00:00，wchan 分别为 `anon_pipe_write` / `pipe_wait_writable`）。
`test/test-ut/test_run_pipe.kl:130` 的 `test_run_read_all_large` 已因此被 `@skip`（头部注 `// TODO: cause zombie process`）。

**阈值**：`src/modules/os.c:422` 的 `kl_pipe_cloexec` 用裸 `pipe()`（全仓无 `F_SETPIPE_SZ`），容量为 Linux 默认 64KB。
子进程为 echo（如 `cat`）时：stdout 管道灌满 64KB → 子进程停读 stdin → stdin 管道再灌满 64KB → 父进程停写，
故 payload 越过 ~128KB（2×64KB）即死锁；≤128KB 则写完 close、`read_all` 抽干后子进程退出，正常。
当前 `test_run_read_all_large` 为 1000×17=17000 字节，远低于阈值，单跑可过（死锁需更大 payload）。

**现象二（fd 泄漏 / zombie）**：`process_type`（os.c:626）、`raw_reader_type`（os.c:273）、`raw_writer_type`（os.c:400）
均未注册 `_free` finalizer；运行时也从不调用 `_free`（`kl_free_instance` 仅 `mm_free`，`src/objects/typeobj.c:118`），且无 SIGCHLD 处理。后果：
① `read_all()`（os.c:205）读完不关 fd，测试一般只 `close` stdin，故每个 os.run 泄漏 stdout/stderr 两个读端 fd；
② 未显式 `wait()` 的子进程永不回收 → zombie。

**待办**：
1. 给 Process 增 `communicate()` 式 API：用 `poll`/`select` 非阻塞地边写 stdin 边抽 stdout/stderr（对标 Python `subprocess.communicate`），从根上消除大 payload 死锁；随后解除 `test_run_read_all_large` 的 `@skip`。
2. 为 Process/RawReader/RawWriter 注册 `_free`（关闭各自 fd；Process 额外回收子进程），并把运行时对象释放路径接上 `_free` 调用。

**关联**：finalizer 机制在 `TypeObject` 已声明 `_free`/`_mark`（`include/runtime/object.h:261-263`）但全仓未接线；亦见 §16（声明-实现欠账）。

---

## 29. `is` / `as` 运算符语义评估与遗留项（2026-10-26 评估）

`as` 的定位是 **downcast**（`any` / trait / union → 具体 class）；`is` 是同一集合上的类型谓词。
class 一律 final、没有 class 继承，只有 trait 构成子类型关系。实现在 `src/parser/parser_expr.c`
（`check_is_src_type` / `check_as_src_type` / `validate_cast_target` / `validate_union_target` /
`resolve_cast_check` / `parse_is` / `parse_as`），IR 在 `src/parser/irgen.c` 的 `emit_ir_is` / `emit_ir_as`。
测试见 `test/test-kl/test_is_as_target_errors.kl`、`test_is_as_warnings.kl`、`test/test-ut/test_is_as_*.kl`。

### 29.1 当前已定规则（已实现）

| 项 | `is` | `as` |
|----|------|------|
| 源类型 | 不能是 `T?`；不能是 `...` / 包 / 未解析等无效种类；`any` / trait / union / class / 内建值类型可以 | 同 `is`，另外**目前**一律拒绝内建值类型（见 29.3-①） |
| 目标类型 | 必须是 class；不能是 trait、`T?`、union、`NilType` | 同 `is` |
| 检查顺序 | 先查源（`T?` 等），再解析目标并校验，再校验 union 成员 | 同 `is`；之后同一泛型 class 的不同类型实参（`list[int]` → `list[str]`）**报错** |
| 同一泛型的不同实参（源静态已知） | 不报错：编译期折叠为 `false` 并给 `always false` 警告（`x list[int]` 的 `x is list[str]`） | 报错：`cannot cast ... with 'as': type arguments are different.`（`as` 是转换，不可能成功） |
| 结果 | `bool`；编译期可确定时为常量并给 `always true/false` 警告，否则运行时检查 | 恒等为 `T`（警告 `always true`，IR 直通源值）；其余为 `T?` |
| `NilType` 目标报错 | `target type of 'is' operator cannot be NilType.` | `target type of 'as' operator cannot be NilType.` |

`T?` 作源一律拒绝：判空用 `== nil` / `!`，判类型用 `is` / `as`，两者不混用。

### 29.2 评估：合理的部分

- `as` 只有 `any` / trait / union 作源才是真正的 downcast；恒等转换给警告而不是报错，与 Kotlin / Swift 一致。
- 目标只能是 class，`as` 结果在可能失败时是 `T?`，可与 `!`、`while let` 直接配合。
- 恒等 `as` 的 IR 不生成任何指令（`ALWAYS_TRUE` 直接复用 `operand`），无运行时开销。
- 作为谓词，`is` 对无关类型返回 `false` 是合法答案，用警告而不是报错合理。

### 29.3 遗留项（待作者拍板 / 待施工）

1. **`as` 对内建值类型的处理（已定方案，未实现）**：同类型（`i as int`）应允许并给 `always true` 警告，IR 直通；
   不同类型（含不同位宽，如 `int64` → `int8`、`int8` → `int16`）一律报错。
   落点：`check_as_src_type` 不再整体拒绝内建值类型；`parse_as` 在目标校验后对「内建源 ≠ 目标」报错；
   `resolve_cast_check` 对「内建源 == 目标」直接返回 `ALWAYS_TRUE`。`sym_id` 唯一，`emit_ir_as` 的断言成立。
2. **「无关类型」的一致性（暂缓）**：不同内建类型要报错，而不同的用户自定义 class（`Dog` → `Cat`）、
   trait 源且目标 class 未实现该 trait，目前仍只是 `always false` 警告；union 非成员却是报错。
   倾向：`as` 目标不可能时一律报错，`is` 一律警告。需要同步调整 `test_is_as_warnings.kl` 等测试。
3. **泛型实参擦除（仅源为 `any` / trait / union 时）**：源类型静态已知时（`x list[int]` 的 `x is list[str]`）
   编译期即折叠为 `false`，`as` 直接报错，均无问题。但源为 `any` 时编译期无法判断，运行时只比较 `list`、
   不比较类型实参：`v any` 的 `v is list[str]` 对 `list[int]` 的值为 `true`（`test_is_as_no_warning.kl` 的 `is_ls` 已有
   `TODO: vm erase subtype?`），`v as list[str]` 也会成功，得到类型实参错误的 `list[str]`。同一个值的同一判断，
   结果取决于静态类型是否为 `any`，是实际的类型安全漏洞。
   **根因（2026-10-26 实测）**：运行时对象不带泛型类型实参，`is` / `as` 的 `RUNTIME_CHECK` 指令只携带
   `target_path` / `target_name`（`list`），VM 只能判断「是不是 `list`」。实测
   `is_ls([1,2,3])` → `true`、`as_ls([1,2,3]) == nil` → `false`。解决方向：VM 运行时保留并比较泛型实参，或对泛型目标的 `any` 源
   `is` / `as` 加限制，需单独评估。
4. **目标只能是 class**：无法用 `is` / `as` 判断或向下转到 trait（含 trait 继承链上的向下转型），表达力受限。
5. **诊断不够具体**：`T?`、`...` 等作源都报 `src of 'is'/'as' operator is not a valid type.`，看不出原因；
   警告文案里类型名是内部签名（如 `Ltest_is_as_warnings.Dog;`、泛型显示为 `list<j>`），不是源码写法；
   `as` 对同一泛型不同实参的报错（`cannot cast 'std/builtin.list<j>' to 'std/builtin.list<u>' with 'as': ...`）同样显示内部签名，应显示 `list[int]` / `list[str]`。
6. **内建类型的 `TypeSpec` 位置共用**：`ts->loc` 在内建类型间共享，曾导致报错位置为 `0:0`；现用 `as->loc` / `is->loc`，其他引用 `ts->loc` 的路径需留意。

### 29.4 测试覆盖现状

`is` 与 `as` 在以下场景成对覆盖（`test_is_as_target_errors.kl`）：trait 目标、带参数 trait 目标、`T?` 目标、union 非成员、
`Dog?` / `int?` / `(Dog | Cat)?` 作源、`int?` 目标、`NilType` 目标、`...` 作源；`as` 额外覆盖：内建值类型作源、
同一泛型不同实参。警告（`test_is_as_warnings.kl`）：`is` 覆盖 class / 泛型的 always true/false；`as` 覆盖 class 的 always true/false 与泛型的 always true（泛型 `always false` 已改为报错，在 `test_is_as_target_errors.kl` 的 `as_generic_different_args`）。
缺口：`as` 恒等内建类型（待 29.3-① 实现后补）、`is` 与 `as` 对 trait 源且目标未实现该 trait 的用例。

---

## 30. `let-else` and `panic()` as a terminal statement: leftovers (2026-10-27)

**Status**: `let v = expr else { ... }` is implemented (single value only, no destructuring). The
grammar desugars it in `let_else_stmts()` (`koala.y`) into two sibling statements, `let v = expr`
and `if v == nil { ... }` (`IfStmt.is_let_else = 1`), reusing the early-return shadow narrowing.
`parse_if()` runs two checks on `is_let_else`: `v` must be optional (checked before the generated
`v == nil` is visited), and the else block must exit (return / break / continue / panic).
`panic()` counts as terminal in `is_terminal()` through `is_panic_call()`, which matches the
resolved `std/builtin.panic` symbol.

**Pending**:

1. **irgen: no fall-through after `panic()`.** `panic` is an ordinary `@native` function, so the
   block holding the call still has an edge into the following code (never taken at runtime: the
   VM has stopped). For let-else / early-return narrowing, that fake edge is an IR path where `v`
   is nil yet flows into a non-null use, and the optimizer still reasons about it and keeps the
   merge point.
   **Fix**: in `emit_ir_expr()`, after a call where `is_panic_call(exp)` is true, emit
   `klr_build_unreachable` (already used for native function bodies in `irgen.c`) and treat the
   current block like return / break / continue, with no fall-through jump.
   **Decided not to do (2026-10-28)**: the extra `unreachable` instruction only increases the code
   size; the fall-through edge is never taken at runtime, so it stays.
2. ~~**`unreachable()` should also be terminal.** `std/builtin.unreachable()` never returns either,
   but `is_terminal()` doesn't recognise it, so `let v = e else { unreachable() }` reports "block
   must exit".~~
   **Resolved (2026-10-27)**: `is_panic_call()` became `is_noreturn_call()`, matching both `panic`
   and `unreachable` in `std/builtin`; it is used by `is_terminal()` and by the return-type check.

---

## 31. Destructuring, `for` and narrowing: deferred tasks (2026-10-28)

**Status**: `for (x, y) in ...` / `for x, y in ...` with `_`, local `let (a, b) = t` /
`var (a, b) = t` (optional per-field types), and quiet error recovery for a bad `for` header are
done. The tasks below are not started; planned for about a month later (around 2026-11-28).

1. **`enumerate` / `zip`** (size M, design first). Goal: `for (i, x) in enumerate(xs)` and
   `for (a, b) in zip(xs, ys)`.
   Open questions: free functions in `std/builtin` (Python style) or iterator methods
   (`xs.iter().enumerate()`, Rust style); the produced type (`Iterator[(int, T)]` /
   `Iterator[(A, B)]` classes, e.g. `EnumerateIter[T]` / `ZipIter[A, B]`); whether `zip` stops at
   the shorter input; whether they take anything with `__iter__` or only sequences.
   Files: `libs/std/builtin/*.kl` (or native C), and the 3 docs. Risk: may expose gaps in generic
   iterator inference.
   Checked (2026-10-28): not solved. `libs/std/builtin/enumerate.kl` and `zip.kl` only hold
   commented-out stubs (`Enumerate[T] : Iterator[(int, T)]`, `Zip[T, U] : Iterator[(T, U)]`),
   so `enumerate(xs)` / `zip(xs, ys)` report "'enumerate' is not found". The §25 inventory lists
   them under `std/builtin`, but they are not implemented yet.

2. **Destructuring at top level and in class fields** (size M–L). Goal: `let (a, b) = f()` at
   module level, maybe `var (x, y) = (0, 0)` in a class body.
   Work: allow `tuple_decl` in the top-level (and class-member) grammar; `parse_tuple_decl` only
   handles `VAR_SCOPE_LOCAL` today and needs global / field variants with the global-init order;
   irgen must store into globals in the module init (or fields in `__init__`); each name must be
   exported to `.klc` like a normal `let` / `var`.
   Open question: are class fields worth it (Rust / Go don't allow it)? Suggestion: top level only.
   Files: `koala.y`, `parser.c`, `irgen.c`, maybe `write_klc.c`.

3. **Nested destructuring** (size M). Goal: `let ((a, b), c) = t` and `for (k, (x, y)) in m`.
   Work: parenthesised items inside `tuple_decl_list` / `id_list`; the `IdentType` items become a
   pattern tree in the AST; recursive type / count checks; recursive `build_unpack_vars`.
   Open question: typed items at inner levels (suggestion: yes, same as the top level).
   Files: `koala.y`, `ast.h` / `ast.c`, `parser.c`, `irgen.c`. Risk: changes the AST shape shared
   by `for` and `tuple_decl`.

4. **Narrowing through `&&` / `||` / `!`** (size M).
   Level 1: `if x != nil && y != nil { x + y }` narrows both in the then-block.
   Level 2: `if x == nil || y == nil { return }` narrows both after the `if` (and in the else
   block); a real `!` flips the facts; any nesting depth.
   Constraint: change only the narrowing code; no detached symbols (`stbl == NULL`), no refactor
   of `parse_if` / `parse_block` / `symbol.c` without agreement first.
   Open design point: how to carry the else-only facts of `||` to the else block and to the
   early-return path without detached symbols.
   Tests: a test-run case for the narrowed results; a test-kl case for the non-narrowed ones
   (`||` in the then-block, `&&` in the else block, `&&` + early return).
   Note (2026-11-03): an `if` expression narrows its branches through the same `unwrap_for_if()` /
   `inherit_if_shadows()` as `parse_if` (`parse_if_expr()` in `parser_expr.c`), so items 4 and 5
   apply to `let n = if v != nil && v.len() > 1 { ... } else { ... }` too; add if-expression cases
   to the tests then.

5. **Narrowing inside the right operand (level 3)** (size S–M, after item 4). Goal:
   `if s != nil && s.len() > 0`, where `s` is already narrowed while `s.len()` is checked.
   Work: when visiting `A && B`, visit `B` in a temporary scope holding the true facts of `A`
   (for `A || B`, the false facts); this happens during the expression visit (`parser_expr.c`).
   Risk: the IR of `B` must use the narrowed symbol.

6. **`if let (a, b) = t` / `while let (a, b) = t`：可选元组的解构**（规模 M，暂不实现，2026-10-30 记录）。
   目标：`t` 是可选元组 `(T1, T2)?`，不为 `nil` 时解构并进入 then 块（或循环体），规则与元组
   let-else 相同：支持 `_`、名字可带类型、名字个数必须与元组大小一致。
   ```koala
   if let (host, port) = split_host_port(url) {
       print(host, port)
   }
   ```
   **为什么暂不实现**：
   - 常见的需求是"取到值，否则提前退出"，已经由元组 let-else（`let (a, b) = t else { ... }`）覆盖；
     `if let` 用于"只有存在时才做某事"，配合元组的场景较少。
   - 现有写法只多一行，可以接受：`if let t = f() { let (a, b) = t ... }`。
   - 实现不是零成本：需要新增 `IF LET '(' tuple_decl_list ')' ...` 和 `WHILE LET ...` 语法规则；
     `parse_if_let()` / `parse_while_let()` 现在只绑定一个符号，要改为绑定多个名字；irgen 要在 then 块
     （或循环体）开头用 `build_unpack_vars()` 解构。
   **为什么以后可能要支持**：
   - 补齐一致性：`let (a, b) = t`、`let (a, b) = t else { }`、`for (a, b) in ...` 都已支持解构，
     `if let` / `while let` 是剩下的地方。
   - 其他语言都有：Rust `if let Some((a, b)) = f() { }`，Swift `if let (a, b) = f() { }`。
   **注意区分**：这不是之前已经放弃的"多值 if let / while let"（8 条 NYI 规则，同时要求多个可选值都不为
   `nil`，例如 `if let a = x, b = y`）。这里只有一个可选元组，语义与元组 let-else 一致，不会推翻那个决定。
   等实际代码需要时再实现。

---

## 32. Tuple literals and expected types: failing cases (2026-10-28, to discuss)

**Status**: `parse_tuple()` gives each element the element type of an expected tuple type as
its `expected`, and records `T?` in the tuple type when an element is `nil` or exactly `T`
and `T?` is expected. So `return (nil, 1)` for `(int?, int)`, `let t (int8, int?) = (1, nil)`
and `(x, y) = (7, nil)` with `x int8` work. Open point: pass the expected type only to the
elements that need it (literals), so an element never takes the type of the left side
instead of resolving its own.
**Decided (2026-10-28)**: keep the current `parse_tuple()` behaviour; an element whose type
cannot be resolved still reports its own error, it never falls back to the left side's type.

The cases below still fail, except item 1 (resolved). They are recorded to discuss later;
nothing else is decided yet. Re-checked on 2026-10-28 after item 1: items 2–7 still fail with
the errors shown.

### 32.1 Tuple related

1. **`nil` in a call argument.**
   ```koala
   func take(p (int?, int)) {}
   take((nil, 1))
   // Incompatible type: expected 'std/builtin.tuple<j?j>', but got 'std/builtin.tuple<_?j>'.
   ```
   Cause: `nil` has only the nil type `_?`, no `src`, so the tuple is `tuple<_?j>`. A
   `return` passes because `parse_tuple()` rewrites the tuple type; a call argument does not
   get there (why is not confirmed yet).
   **Resolved (2026-10-28)**: `type_spec_compatible()` compares two tuple instances element by
   element (`compatible_tuple()`, called from `compatible_klass()`): an element is accepted if
   it is the same type, `nil` (`_?`) for any `T?`, or a nested tuple checked the same way.
   Other element types stay invariant. Known gap: the value keeps its own runtime type
   (`tuple<_?j>`), so `is` / `==` on such a tuple may not see the declared type.

2. **A class value for a trait element.**
   ```koala
   func one() (Fly, int) {
       return (Bird(), 1)    // return type mismatch in function 'one'
   }
   ```
   Cause: `tuple<Bird, int64>` does not match `tuple<Fly, int64>`, generics are invariant.
   A trait value also has another representation, so even with covariance irgen must upcast
   each element (`_build_obj_intf_upcast()`, as `build_unpack_vars()` does).

3. **An `any` element.**
   ```koala
   let t (any, int) = (1, 2)
   // lhs: std/builtin.tuple<oj> =/= rhs: std/builtin.tuple<jj>
   ```
   Cause: generics are invariant, `tuple<jj>` is not `tuple<oj>`; `var a any = 1` works.
   Open question: is `tuple` a special case of generics, compatible element by element?
   This would also solve item 1.

4. **Tuples in a list literal.**
   ```koala
   let l list[(int?, int)] = [(nil, 1), (2, 3)]
   // lhs: list<tuple<j?j>> =/= rhs: list<o>
   ```
   Cause: generics are invariant; and the list literal does not pass the expected element
   type to its elements, so the tuples are `tuple<_?j>` and `tuple<jj>`, and their LUB makes
   `list<any>`. Needs `parse_list()` to pass `expected`.

### 32.2 Not tuple related (a single value fails the same way)

5. **A binary expression for `int8`.**
   ```koala
   let t (int8, int) = (1 + 2, 3)    // var x int8 = 1 + 2 fails too
   ```
   Cause: a literal is `int64` by default; `expected` is not passed to the operands of
   `1 + 2`, so the result is `int64`. Possible improvement: a binary expression passes
   `expected` to its literal operands.

6. **`int` for `float`.**
   ```koala
   let t (float, int) = (1, 2)       // var f float = 1 fails too
   ```
   Cause: `int64` is not converted to `float` implicitly, `float(1)` is needed. This is the
   language rule, not a bug.

7. **An empty list.**
   ```koala
   func g() (list[int], int) {
       return ([], 1)                // 'return []' for 'list[int]' fails too
   }
   ```
   Cause: `[]` is `list<any>` (`list[o]`), generics are invariant, it does not match
   `list[int]`. Possible improvement: an empty `[]` takes its element type from `expected`.

### 32.3 Runtime type of a tuple

8. **A tuple's runtime type may differ from its declared type (`is` / `==`).**
   ```koala
   func take(p (int?, int)) {
       print(p is (int?, int))   // the value was created as tuple<_?j>
   }
   take((nil, 1))
   ```
   Cause: since item 1, `type_spec_compatible()` accepts `tuple<_?j>` where `tuple<j?j>` is
   expected, but the value keeps the instance it was created with (`tuple<_?j>`). The static
   type says `(int?, int)`, the runtime type says `(nil, int)`. So a runtime type check (`is`,
   `as`) or an operator that compares the two types (`==` between a `(nil, 1)` value and an
   `(int?, int)` value) may give a wrong result or an error. `return (nil, 1)` is not affected:
   `parse_tuple()` already builds that value as `tuple<j?j>`.
   To check: what `is` / `as` / `==` do on such a tuple today. Possible fixes: build the value
   with the declared type when it is passed (an argument, an assignment), or compare tuple
   instances element by element at runtime too.
   Checked (2026-10-28): not observed yet.
   - `p is (int?, int)` / `p as (int?, int)` on the declared type are decided at compile time
     ("'is' check is always true"), the runtime type is not looked at.
   - A runtime check through `any` (`let a any = p` in `take((nil, 1))`, then
     `a is (int?, int)`) prints `true`.
   - `==` cannot show it: `==` between two tuples does not compile at all (item 9).
   Keep this item open until `==` on tuples works and the runtime `is` on tuples is understood.

### 32.4 Found while checking the cases above

9. **`==` between two tuples does not compile.**
   ```koala
   let a (int, int) = (1, 2)
   let b (int, int) = (1, 2)
   print(a == b)   // argument type mismatch for operator '==' of type 'tuple<jj>'.
   ```
   Not related to optional elements: plain `(int, int)` fails the same way.

10. **A runtime `is` on an `any` value held by a `let` local crashes the VM.**
    ```koala
    let b any = 1
    print(b is int)
    // koala: include/runtime/vm_ops.h:2287: _eval_frame: Assertion `(rs) < max_regs' failed.
    ```
    The same at top level and inside a function, for an `int` or a tuple value. A parameter
    assigned to an `any` local (`let a any = p`) works.

11. **`v = nil` inside `if v != nil { ... }` fails in isel.**
    ```koala
    func narrowed() {
        var v int? = 1
        if v != nil {
            v = nil
        }
        print(v)
    }
    // isel.c:317: Why goes here?
    ```
    Both operands of a binary op reach `isel_lower_binary()` as constants, and its constant
    folding branch is only `UNREACHABLE()` (`src/parser/backend/isel.c:316`).

---

## 33. Bugs found on 2026-10-28 (to fix)

Bugs seen during the `koala.y` recovery / memory work and the tuple work. The tuple-related
bugs are in §32.4 (items 9–11: `==` on tuples, the VM crash on `is` with an `any` local, the
isel failure for `v = nil` under `if v != nil`).

1. **`a ?: b` removed (2026-10-29).** The `?:` token (`OPT_DEF`) and the `or_expr OPT_DEF or_expr`
   rule were deleted from `koala.l` / `koala.y`; the rule used to silently evaluate to `a`.
   `?:` conflicts with the Elvis meaning in other languages. A null-coalescing `a ?? b` may be
   added later.

2. **Memory leaks on every compile, outside the parser.** `valgrind --leak-check=full koalac`
   on a valid program reports about 22–29 KB "definitely lost". The parser and the AST frees
   leak nothing after the `%destructor` work and the tuple / map free fixes. The rest:
   - `.klc` loading: `read_intf_table()` (`klc.c:1460`, about 36 KB in 3 blocks) and
     `klc_const_to_literal()` (`read_klc.c:310`, about 5 KB);
   - semantic pass: `handle_valist_and_dfl_args()` (`parser_expr.c`, the old `KeyWordExpr`
     nodes are dropped, see its `// TODO: free old args`), `parse_tuple()` (the `tp_args`
     vector), `find_or_add_instance()` (`parser_generic.c:511`), `stbl_new()`, `str_ndup()`.

3. **Annotation values can leak after a syntax error.** `koala.y` has no `%destructor` for
   `<prefix_flags>` / `<annot_param>` (annotations and prefix flags) and for the nested type-list
   vectors `specialized_types_list` / `atom_primitive_type_lists`, so their values leak when
   bison discards them during recovery. They were left out on purpose: some of their rules may
   not fully initialise the struct, and a destructor on garbage would crash. Fix: make every
   rule set the whole value first, then add the destructors.

4. **Tuples with generic type arguments are still compared invariantly.** The element-wise
   check (§32 item 1, `compatible_tuple()`) only covers concrete tuple instances (`TYPE_KLASS`).
   A tuple type whose elements contain a type parameter is a `TYPE_GENERIC_REF` and goes through
   `compatible_generic_ref()`, which needs every argument to be strictly equal. So in
   `func f[T](x T) (T?, int)`, `return (nil, 1)` may still fail.

5. **Noisy recovery after `tuple = x`.** Since the tuple-assignment rule spells out
   `'(' expr_list ',' expr ')'`, `tuple = x` (the `tuple` keyword on the left) is a syntax
   error, but the recovery adds follow-on errors: "expected '}'." and "'x' is not found".

6. **A trait-typed global is stored without `make_intf` (found 2026-10-30).**
   ```koala
   let l = [1, 2, 3]
   let s Sequence[int] = l
   print(s.len())
   // koala: include/runtime/vm_ops.h:430: _eval_frame: Assertion `((callable)->tag > (64 + 1))' failed.
   ```
   The module `__init__` emits `global_set @s std/builtin.Sequence<j>, %0 std/builtin.list<j>`:
   the bare object is stored, not an interface value, so every method call through the global
   dispatches on a non-interface object. A local trait fails the same way (`let n Named = W()`
   at top level). The same code inside a function is fine (`make_intf ... to
   std/builtin.Sequence`). Fix: convert the initial value with `_build_obj_intf_upcast()`
   before the `global_set` (and on later assignments to such a global), as the local path
   does. `test/test-run/test_intf_ext_generic_inherit.kl` keeps its values in locals until then.
   **Resolved (2026-10-31)**: the global branch of `emit_ir_var_decl()` converts the initial
   value with `_build_obj_intf_upcast()` before `global_set`; later assignments are covered by
   item 7. Tested by `test/test-run/test_intf_assign.kl`.

7. **Assigning a class value to an existing trait-typed variable skips `make_intf` (found
   2026-10-31).**
   ```koala
   var x Fly = Bird()   // fine: make_intf ... to Fly
   x = Bird()           // move %x Fly, %2 Bird -- the bare object is stored
   print(x.fly())       // koala: vm_ops.h:430: Assertion `((callable)->tag > (64 + 1))' failed.
   ```
   The declaration converts the value, a later plain assignment does not:
   `emit_ir_simple_assignment()` moves `rhs->ir_val` as is. The same happens for a name bound
   by `var (f Fly, n) = ...` and then assigned. Fix: convert with `_build_obj_intf_upcast()`
   before the store in the assignment path (locals, fields and globals), as the declaration
   does. Likely the same root as item 6.
   **Resolved (2026-10-31)**: `emit_ir_simple_assignment()` converts the value to the target's
   type first, for locals, globals, fields and list elements. For an index store the target
   type is `__setitem__`'s value parameter, kept in the new `IndexExpr.value_ts` by
   `parse_index_store()` (the index expr's own type is the index's). A trait value of the same
   trait is stored as is; a derived trait is upcast to the base. Tested by
   `test/test-run/test_intf_assign.kl`.

8. **`A?` -> `C?` between optional traits is not upcast, a wrong method is called (found
   2026-10-31).**
   ```koala
   trait B { func b1() str }
   trait C { func c1() str }
   trait A : B & C { func a1() str }
   // Foo : A implements all three
   let a A? = Foo()
   let c C? = a
   print(c!.c1())    // prints "b1": B's method
   ```
   The IR is `move %c C?, %a A?`: no `upcast_intf`. `_build_obj_intf_upcast()` finds no symbol
   for the optional source type `A?` and returns without a conversion (its step 5). The value
   keeps A's itable, and the call takes C's slot 0 in it, which is A's `b1`. With a single
   base the bug is hidden, because a base's methods are always a prefix of the derived
   trait's table. The non-optional `let c C = a` is correct (`upcast_intf ... to C`).
   Fix: unwrap an optional source too, and emit the upcast under a nil check (nil stays nil),
   for declarations, assignments, arguments and returns.
   **Resolved (2026-10-31)**: `_build_obj_intf_upcast()` hands an optional source to the new
   `_build_opt_obj_intf_upcast()`: when `_intf_upcast_needed()` says the inner type needs a
   conversion, it emits a result local (`nil`), a nil check, and on the not-nil path
   `cast A? to A` + `upcast_intf` (`make_intf` for a class) into the local. Every caller goes
   through it (declarations, globals, assignments, arguments, returns, tuple elements, fields);
   the same trait (`A?` -> `A?`) is still a plain move. Tested by
   `test/test-run/test_intf_optional_upcast.kl`.

9. **Optional chaining with a method call aborts in irgen (found 2026-10-31).**
   ```koala
   let c C? = C()
   print(c?.m())     // koalac: irgen.c:668: update_call_args:
                     //         Assertion `proto_ts->kind == TYPE_PROTO' failed.
   ```
   The same for a trait (`a?.hello()`) and for `str?` (`s?.len()`); `c?.v` on a field also
   fails (`koala` reports "compilation failed" while `koalac` exits 0, not looked into). The
   Manual documents `b?.get()` as supported (Optional chaining). No test uses `?.` with a
   call yet.
   **Disabled (2026-10-31)**: `parse_dot()` reports "'?.' is not supported yet, use 'if let' or
   '!.'." instead of letting irgen abort; the grammar rule, `DOT_OPTIONAL` and its type
   inference are kept, and the Manual says it is not supported yet. To implement later: the
   method call and the field access, with the nil short-circuit of a whole chain, then remove
   the error. Tested by `test/test-kl/test_opt_dot_disabled.kl`.

10. **Too many arguments in a call are not reported (found 2026-10-31).**
    ```koala
    func one(x int) int = x
    one(1, 2)          // compiles without an error
    typeof(1, 2)       // the same for builtins
    hash(1, 2)
    ```
    Too few arguments are reported ("too few arguments in function call."), extra ones are
    accepted silently, for user functions and builtins alike.
    **Resolved (2026-10-31)**: `check_call_args()` reports "too many arguments in function
    call." at the first extra argument. Enabling it showed that class field initializers were
    visited before the signatures they call were parsed (before later classes' methods and before
    all top-level functions), so their arguments were never checked. Fields now have their own
    pass, `kl_parse_ast_klass_field_meta()` (`koalac.c`), after the method and top-level function
    signatures and before the bodies. Tested by `test/test-kl/test_call_too_many_args.kl`,
    `test/test-kl/test_field_init_call_errors.kl` and `test/test-run/test_field_init_call.kl`;
    `test/test-kl/test_call.kl` now also expects its three real extra-argument errors.

---

## 34. 泛型 bound 实参未转接口：泛型自由函数与泛型类 `__init__`（2026-10-30 发现，fixed 2026-11-07）

> Fixed (2026-11-07): `try_to_build_tp_args_cast()` now also runs for a generic free function and for
> `emit_type_call()`, and it skips an argument that already is an interface value. Tests:
> `test/test-run/test_bound_arg_free_func.kl`, `test_bound_arg_init.kl`. Left to do: enable
> `test/test-run/test_intf_generic_bounds.kl.disabled`, which has not been tried.

> 背景：为接口 IR 的三个槽位 bug（外模块 trait 继承取到祖先 trait 的槽、`A?` / 泛型接口 ts
> 未归一到 origin trait、`get_intf_index()` 返回 -1 时直接 abort）补回归测试时发现的两处**同源**
> 缺口。那三个 bug 已修，回归测试为 `test/test-run/test_intf_ext_inherit.kl`、
> `test_intf_local_inherit.kl`、`test_intf_optional.kl`（三者同时校验 no-opt-ir / lir 与运行输出，
> 全量 lit 298 个用例通过）。
> 本节两个缺口的用例已写好留档：**`test/test-run/test_intf_generic_bounds.kl.disabled`**
> （lit 只收集 `.kl` / `.test`，见 `test/lit.cfg` 的 `config.suffixes`，故当前不会被执行；
> 修复后去掉 `.disabled` 即启用）。

### 34.1 现状

两处症状相同：被调函数体内按 bound 接口调用（`call @Shape:area [intf-index = 0], %v wrap:T`），
但实参是裸对象、没有 `make_intf` 包装 → VM 断言 abort（进程直接死，不是可捕获的 error）：

```
koala: include/runtime/vm_ops.h:430: _eval_frame: Assertion `((callable)->tag > (64 + 1))' failed.
```

1. **泛型自由函数**：`func wrap[T: Shape](v T)` + `wrap(Rect())` → **必崩**。

   ```
   %0 = call @Shape:area [intf-index = 0], %v wrap:T     ← 体内按接口调用
   %1 = call @wrap, %1 wrap.Rect                         ← 实参裸对象，缺 make_intf
   ```

2. **泛型类 `__init__`**：`Doubler[Rect](Rect())` 的构造实参不转换。

   ```
   %1 = call @Shape:area [intf-index = 0], %0 Doubler:T
   call @Doubler:__init__, %1 Doubler<L Rect;>, %0 Doubler.Rect    ← 缺 make_intf
   ```

   **间歇性**（比第 1 条更难定位）：只有直接使用 `__init__` 存下的字段才崩（`peek()`）；
   若该字段后来被实例方法写过一次（`store()`），那次写入走了转换、字段被替换成接口值，
   程序就输出正确结果——实测 `print(d.store(Rect()))` → `10`。即崩溃与否取决于
   "字段是否被非 `__init__` 路径写过"，会随调用顺序时隐时现。

   对照（已正确，`test_intf_optional.kl` 已固化该 IR）：

   ```
   %34 = make_intf %33 test_intf_optional.Rect [intf-table-index = 4] to Doubler:T
   call @Doubler:store, %35 Doubler<L Rect;>, %34 Doubler:T
   ```

### 34.2 根因

`try_to_build_tp_args_cast()`（`src/parser/irgen.c:738`）只有**一个**调用点（`irgen.c:900`），
前置条件是 `lhs_sym->origin != NULL` 且 `ASSERT(_parent && _parent->kind == SYM_INSTANCE)`
（`irgen.c:896-899`）——即"泛型**类实例**的方法调用"。

- 泛型自由函数调用没有 `origin`，`parent` 是 `SYM_PACKAGE` → 进不了这条分支；
- 类构造 `Doubler[Rect](...)` 走 `emit_type_call()`（`irgen.c:858`）→ 同样绕过。

### 34.3 施工方向

把"形参声明类型是开放泛型 `T`（带 bound）→ 对实参发 `make_intf`"从实例方法路径里抽出来，
改为按**被调函数的形参类型**统一处理，建议落在 `update_call_args()`（`irgen.c:889`）内或紧随其后，
一次覆盖三条路径：普通/泛型自由函数调用、`emit_type_call()` 构造调用、现有实例方法调用。

注意点：

- 形参类型是 `T?`、`list[T]` 等**含 T 的复合类型**时的逐层转换（现有 helper 只处理裸 `T`）；
- itable index 来自 `get_intf_index(obj_sym, bound)`，为 -1 时必须报 `_intf_index_error()`
  而不是继续（本次已在 `try_to_build_tp_args_cast()` 与 `_build_obj_intf_upcast()` 两处补上）；
- 修复后启用 `test/test-run/test_intf_generic_bounds.kl.disabled`（预期输出 `10 / 20 / 20`），
  并把 `test_intf_optional.kl` 中"绕开该缺口"的注释与 `Doubler` 用例改回用 `__init__` 直接填充。

### 34.4 同批已修（交叉参考，非待办）

1. **外模块 trait 的继承方法取错槽**：`get_callable()` 现接受 `SYM_INHERITED` 并以 `sym->parent`
   （= 接收者的静态类型）为 owner；`_get_ext_trait()` 按需建槽表；`_add_all_intf_to_trait()`
   跳过 `SYM_INHERITED` 项。例：`Conn:write_str` 用 `intf-index = 3`（Conn 的槽表），
   `io.Writer:write_str` 用 1（Writer 自己的槽表）——同一方法两个索引。此 bug 曾表现为
   `libs/std/socket/conn.kl` 的 `TcpConn.write_str` 运行时打到错误槽位。
2. **`A?` / 泛型接口 ts 未归一**：`_build_obj_intf_upcast()` 先用新增的 `intf_canonical_ts()`
   （`src/parser/symbol.c`）把 `Getter[int]?` / `Getter[T]` 归一到 origin trait 的规范 ts，
   否则 `get_intf_index()` / `get_intf_table_index()` 的 lro 线性查找失配。连带修掉：只增加可空性
   （`Getter[int]` → `Getter[int]?`）时**不得**发 `upcast_intf`（`ts_sym == obj_sym` 短路），
   否则会把 itable 换成某个父槽 → `vm_ops.h:1848 Assertion 'idx < itab->num_parents'`。
3. **`get_intf_index()` 返回 -1 无诊断**：新增 `_intf_index_error()`，两处调用点改为报
   `'%s' does not implement interface '%s'` 而非 abort。该路径目前无法从用户代码触达
   （`let s Shape = Nope()` 被 "Types of two sides are not matched" 拦下、`Doubler[Nope]` 被
   "generic bound not satisfied" 拦下、`n as Shape` 被 "target type of 'as' operator must be a
   class type" 拦下），属防御性改动。


## 35. A type parameter value passed as an automatic trait (2026-10-31, to support later)

A type parameter `T` already gets the methods of the automatic traits without declaring them in
its bound: `==` / `!=` (`Equatable`), `t.hash()` / `hash(t)` (`Hashable`, `OP_HASH`) and
`t.fmt(f)` (`Printable`, `OP_FMT`). Tests: `test/test-run/test_hash_type_param.kl`,
`test/test-run/test_fmt_type_param.kl`.

Not supported yet: **converting** a `T` value to one of these traits, e.g. passing it to a
`Printable` parameter:

```kl
func show[T](t T) str {
    let f = Formatter()
    f.write(t)          // write(value Printable)
    return f.result()
}
// error: Incompatible type: expected 'std/builtin.Printable', but got 'show:T'.
```

The same applies to `Hashable` / `Equatable` parameters, `let p Printable = t`, returns, fields
and container elements of these types.

Where it fails / what is needed:

1. **Type check**: `_diff_kind_compatible_check()` (`src/parser/parser.c`) accepts a
   `TYPE_GENERIC_VAR` source only through its declared bounds. It should also accept `any`-like
   automatic traits (`Printable`, `Hashable`, `Equatable[T]`) for every type parameter.
2. **irgen**: `_build_obj_intf_upcast()` (`src/parser/irgen.c`) returns NULL (no conversion) when
   the source is a type parameter, so the raw value (unbounded `T`) or an interface value of the
   bound (`T: Shape`) would reach the callee, whose `value.fmt(self)` is an interface call → wrong
   slot or the VM assertion `_eval_frame: Assertion '((callable)->tag > (64 + 1))'`.
   Idea: emit `make_intf` with the automatic trait's itable index. `OP_MAKE_INTF` takes the
   itable from `kl_typeof()` of the value (the object's own type, also for an interface value),
   so it works for both cases **if** the automatic traits have the same itable index in every
   type (classes: `[0] any [1] Printable [2] Hashable [3] Equatable`). To verify for the builtin
   types (`int`, `float`, `str`, `list`, ...) before relying on it; otherwise a new op that looks
   the trait up by identity is needed.
3. Tests: unbounded `T`, `T: Shape`, `T` instantiated with `int` / `str` / a class with and
   without its own `fmt` / `hash` / `__eq__`; `f.write(t)`, `let h Hashable = t`, `list[Printable]`
   element.

## 36. `if` expression: later extensions (2026-10-31, to discuss)

`if c { a } else { b }` is a value in `let` / `var` initializers, `=`, `return` and `= expr`
function bodies (Manual "Branching and Loops"; `test/test-run/test_if_expr.kl`). Left for later:

1. **Narrowing in the branches.** `parse_if()` narrows for `x != nil` / `x is T` in the then/else
   blocks; `parse_if_expr()` does not, so `let n = if x != nil { x! } else { 0 }` needs `!`
   (or `x ?? 0`). Needs the shadow-variable scopes of `parse_if()` around each branch and the
   matching irgen side.
2. **`if let` form**: `let n = if let v = x { v } else { 0 }`.
3. **Expected type for `x = if ...`**: `parse_assign()` visits the right side before the left,
   so the branches get no expected type and must agree by themselves
   (`f = if c { Bird() } else { Plane() }` with `f Fly` fails; a typed `let` works).
4. **A `panic()` / `unreachable()` branch**: its type is no-type, so it does not fit the other
   branch; it could be allowed as "never returns".
5. **Other positions**: arguments, operands, `x += if ...`. Kept out on purpose, following Swift
   (decided 2026-10-31, also for call arguments: bind the value with `let` first); revisit only
   with a concrete need.
6. **`match` as an expression** in the same positions.

---

## 37. Mutable literals and the const pool: leftovers (2026-11-01)

**Status**: list and map literals are no longer shared through the const pool. Before, an
all-constant `[1, 2, 3]`, `{"a": 1}` or `list[int](1, 2, 3)` was a const-pool object that
`OP_LOADK` / `OP_RET_CONST` hand out as is, so every evaluation returned the same object and a
change made through one result was seen by the next. Now:

- `emit_ir_list`, `emit_ir_map` and the constant branch of `emit_list_call` (`irgen.c`) always emit
  `build_intern @list` / `@map`;
- `isel_lower_build_intern` (`isel.c`) no longer folds an all-constant `build_intern` into a const;
- `emit_ir_tuple` makes a constant tuple only when every item is an immutable constant
  (`is_immutable_const`: none, int, uint, float, bool, str, or a tuple / range / slice of those,
  checked recursively); otherwise it emits `build_intern @tuple`.

Tests: `test/test-run/test_const_mutable_fresh.kl` (runtime, fails on the old compiler) and
`test/test-ir/test_const_mutable_ir.kl` (no-opt IR, optimized IR, LIR).

**Open**:

1. **bytes literals are still shared.** `b"ab"` is a const-pool bytes object loaded with `OP_LOADK`;
   `bytes` is writable, so `mk_bytes()[0] = 120` changes what the next `mk_bytes()` returns
   (`b"xb"`). A tuple holding bytes is now built at run time, but the bytes item inside is still the
   shared one. To decide: make `bytes` immutable, as in Python (with a mutable counterpart for
   writes), or build a bytes literal afresh (e.g. an `INTERN_BYTES` that copies the pool data).
2. **`const_copy_prop.c` (OP_IR_CALL fold).** A constant call to the `list` class is still folded
   with `klr_const_list`, and a `tuple(...)` call with `klr_const_tuple` without the immutability
   check. No known input reaches it (irgen lowers `list[T](...)` itself, without a call); decide
   whether to emit `build_intern` there or assert the path is unreachable.
3. **Now unused paths.** `klr_const_list` / `klr_const_map` are only used by item 2;
   `is_mergeable_key()` (compile-time merge of duplicated constant map keys, `klr/ir.c`) is no
   longer used (duplicated keys are merged by the runtime dict); the loader's const list / dict
   cases (`modobj.c`, `load_module.c`) and the `KL_MACH_CONST_LIST` / `_DICT` writer cases are no
   longer reached for literals. Kept on purpose for now; remove once item 2 is decided.
4. **Empty list literal and the declared type.** `let l list[int] = []` fails ("Types of two sides
   are not matched. lhs: list<j> =/= rhs: list<o>"): `[]` does not take its element type from the
   declaration. `list[int]()` works. Older than this change.

**Related, fixed the same day**: an instance type such as `Iterable[str]` has one interned type
spec, but `find_or_add_instance()` creates an instance per symbol table (e.g. one while a package's
klc is loaded, one in the module being compiled), and `stbl_add_instance()` points the shared
type spec's `sym_id` at the last one. A `for` loop over an `Iterable[str]` then crashed in irgen
("Why goes here?"): the parser had instantiated `iter` on the other instance. `get_iter_func()`
(`irgen.c`) now gets the method through `get_instance_method()`, which instantiates it on demand;
regression test `test/test-run/test_ext_instance_shared.kl` (with `test/test_pkg/pkg4`). Still
open: other places that look a method up on an instance with a plain `stbl_get(inst->stbl, ...)`
can meet the same mismatch; audit them (e.g. `irgen.c` around `stbl_get(inst_sym->stbl, fn->name)`).
A second case was hit the same day: once `std/os` / `std/fs` had `(int, int)?` returns, their klc
loads created a `Sequence[int]` instance too, and `_add_instance_used_intf_to_trait()` asserted on
the module's own `Sequence[int]` (`test-run/test_intf_ext_generic_inherit.kl`): the `KlrExtTrait` is
found by name, so both instances share it. It now fills only the still empty slots when the trait
already has them (`klr_set_ext_intf()`). Both are symptoms of one interned type spec with several
instance symbols; a fix at the root (one instance per type spec) is still to be decided.

---

## 38. Compiler and tooling issues found while adding fs / os APIs (2026-11-01, to discuss)

1. **`in` binds looser than `||` / `&&`.** In `koala.y`, `in_expr` is `or_expr IN or_expr` at the
   top of `expr`, so `a || "/" in s` parses as `(a || "/") in s` and fails type checking. Python
   puts `in` at the comparison level, above `and` / `or`. `std/fs/path.kl` (`Path.with_name`) uses
   parentheses for now.
2. **A compile error without a diagnostic.** `if new_name.empty() || "/" in new_name || "\\" in new_name { ... }`
   in a method only prints "koala: compilation failed"; the shorter
   `x.empty() || "/" in s` reports "argument type mismatch for operator '||'". Some error path
   fails without calling `kl_error`.
3. **A keyword used as a name.** `let link = "..."` reports only "expected an identifier." (plus
   cascading errors); it should say that `link` is a keyword.
4. **`tools/kl-doc.py` anchors collide.** The `Stat` class and the `stat` function both get the
   anchor `#stat` in `docs/std_fs_api.md`, so in-page links clash (anchors are lowercased names).
5. **SIGPIPE is ignored in two places.** `os_native_lib_init` and `socket_native_lib_init` both call
   `signal(SIGPIPE, SIG_IGN)`; harmless, but it could be one call at runtime start-up. Children of
   `os.run` / `os.system` get the default action back (`init_spawn_attr`, §28).
6. **§25.4 is out of date**: it says the network area is empty, but `std/socket` exists.

7. **~~`list.sort()` is declared but not implemented~~ — fixed 2026-11-08.** It panicked with
   "func 'list::sort' is not implemented!" (found 2026-11-01 in `fs.glob`). The runtime now has
   `_list_sort` in `src/objects/listobj.c`: a stable bottom-up merge sort over the list's
   `TValue`s, a `sort(reverse = false)` signature, and ordering through `SLOT_LT` (`__lt__`) so
   user classes sort. `fs.glob` itself still returns `**` matches in `walk` order (that is now
   documented as its contract, not a symptom of the missing `sort`); callers who want a strict
   order call `sort()` on the result. Tests: `test/test-run/test_list_sort.kl`,
   `test/test-ut/test_list_sort.kl`. Still missing: `sort_by(cmp)` and `key=`.

---

## 39. Signal handlers in `std/os` (2026-11-01, waits for the concurrency API)

`std/os` can send signals (`os.kill(pid, sig)`, `Process.terminate` / `kill`, the `SIGHUP` ..
`SIGTERM` constants) but a program cannot handle one: there is no `signal.signal` counterpart, so
SIGINT / SIGTERM end the process with the default action, and the only signal setting the runtime
makes is ignoring SIGPIPE (§28, §38.5).

**Why it waits**: a C signal handler runs asynchronously, on whichever thread the kernel picks, in
the middle of any VM instruction; it can only do async-signal-safe work, so it cannot run Koala
code. The handler must therefore only record the signal (e.g. a self-pipe or `signalfd`), and a
Koala-level handler has to run later on a normal thread of execution. Where and when that is (a
dedicated task, the scheduler's event loop, a channel the program reads) depends on the
concurrency API, which the runtime's green threads have but std does not expose yet (§25.3).

**To decide when it is designed**:

1. The API shape: a callback (`os.on_signal(sig, func)`, like Python), a channel / iterator of
   received signals (like Go's `signal.Notify`), or both.
2. Which signals may be handled: not SIGKILL / SIGSTOP; SIGSEGV / SIGBUS / SIGFPE are better left to
   the runtime (panic, traceback).
3. Restoring the default action and ignoring a signal (`SIG_DFL` / `SIG_IGN`), and that children of
   `os.run` / `os.system` get the default actions back (`init_spawn_attr`, §28).
4. How a handled SIGINT interacts with blocking calls (`Process.wait`, `communicate`, socket reads):
   EINTR is retried today, so a handler must still get to run.
5. Platform names and numbers beyond the five portable constants (e.g. SIGUSR1 is 10 on Linux and
   30 on macOS).

---

## 40. `@specialized` generic functions: open items of the IR specialization (2026-11-01, to fix later)

A `@specialized` function is now a clone of the generic function's IR (`src/parser/klr/specialize.c`,
driven by `emit_specialized_funcs()` in `irgen.c`): all instructions are cloned with the type
variables replaced, calls to generic functions are routed with the concrete type args, an intf
call on a `T` receiver is devirtualized to the class method, and where a `T` value goes into a
trait irgen leaves `cast %x T to Trait`, which the clone turns into a `make_intf`. Left open:

1. **The erased track still crashes on a `T` value used as an intf value** (§8, shelved by the
   author; §34 for the bound args of generic free functions): callers pass raw values (`call @ts1 [const], 5 int64`), but the generic body makes an
   intf call on it (`x.to_str()` with `T : ToString`) → `vm_ops.h:430` assertion. Only the
   `@specialized` clones are fixed. New fact for when §8 is reopened: every place a `T` value goes
   into a trait now carries the marker `cast %x T to Trait` (lowered to a move by isel), so the
   erased track has a single point to attach a conversion to.
2. **Devirtualization fallback is untested**: when the class of the type argument does not define
   the trait method itself (e.g. a trait default method), `specialize_intf_call()` wraps the
   receiver with `make_intf` and keeps the intf call. No primitive type (the only types
   `@specialized` accepts) gets a trait default method today, so no test reaches it.
3. **`T?` → `Trait?` has no marker**: `_build_opt_obj_intf_upcast()` returns early for a type
   variable source (`_intf_upcast_needed()` is false), so a clone would pass the raw optional
   value into an optional trait slot. Not hit by any code yet.
4. **`mangle_func_name()` handles only primitive, `str`, `bool` and class type args**:
   `type_spec_str_func_name()` (`typespec.c`) ends in `UNREACHABLE()` for other kinds (optional,
   tuple, proto, ...), so routing a call whose concrete type args include one of them aborts the
   compiler, at parse time or in a clone. Seen in the code, no test yet.
5. **The `koala` driver hides a `koalac` crash**: when `koalac` dies by a signal (e.g. a segfault in
   irgen), `koala` only prints "koala: compilation failed". It should report the signal, e.g.
   "koalac crashed (signal 11)", as §38.2 also asks for a diagnostic.
6. **Explicit type args with a type variable abort the compiler**: in a generic body,
   `twice[T](x)` (instead of the inferred `twice(x)`) fails the assertion
   `arg_sym && arg_sym->kind == SYM_CLASS` in `parse_index_func_tp()` (`parser_expr.c`), with or
   without `@specialized`. Each explicit type arg is taken as a class symbol (its `instance_ts`);
   a type parameter (and likely a generic instance such as `list[T]`, or `int?`) is not handled.
   Expected: the type arg's type spec is used, and with type variables the call stays an erased
   call whose type args are kept on the call for the clones (as for an inferred call,
   `exp->tp_args`).

## 41. `koala/re` vs Python's `re`: what is not aligned (2026-11-01, to do / to discuss)

The API of `libs/koala/re` follows Python's `re` (checked against Python 3.14); there is no
Koala-only or Go-only API left, except `SEARCH` / `MATCH` / `FULL_MATCH` with `Pattern.exec` and
the public constructors, kept on purpose. Aligned on 2026-11-01: `compile` / `search` / `match` /
`fullmatch` / `findall` / `finditer` / `sub` / `split` / `escape` / `purge`; the flags `NOFLAG` /
`I` / `IGNORECASE` / `M` / `MULTILINE` / `S` / `DOTALL` / `A` / `ASCII` with Python's values;
`Pattern.pattern` / `flags` / `groups` / `groupindex` and its methods with `pos` / `endpos`;
`Match.re` / `string` / `regs` / `pos` / `endpos` / `group` / `start` / `end` / `span` (by number
or name) / `groups` / `groupdict` / `expand`; Python's replacement templates (`\1`, `\g<name>`,
`\0`, octal, `\n` ...) and errors; Python's `count`; `split` inserts the groups; the printed form
of `Pattern` and `Match`. Also aligned on 2026-11-01, second round: `subn`; `findall` with one
group gives group 1; `\d \w \s \b` are Unicode unless `ASCII` (then ASCII only, and `IGNORECASE`
folds only ASCII letters; `\d \w \s` are never case-folded, as in Python); `\Z`, `\uXXXX`,
`\UXXXXXXXX`, `(?a)`, `(?u)`; and Python's empty-match rule (3.7+) in `findall` / `finditer` /
`sub` / `split`, with `split` pieces as Python cuts them. Removed then: `UNGREEDY`, `is_match`, `replace` (now `sub`),
`replace_literal`, `replace_with` / `Replacer`, `Pattern.group_name` / `group_index` /
`group_names`, `Match.regex()` / `input()` / `indexes()` (now `re` / `string` / `regs`) /
`matched` / `named` / `submatches`, and the `n` limit of `findall`.

### 41.1 In Python, missing in Koala (not added: no new API in that refactor)

1. ~~`subn`~~: done 2026-11-01.
2. `sub` with a function as `repl`: Koala has no function-type parameters yet.
3. `m[g]` (`__getitem__`): blocked by §42 (indexing a user class aborts irgen).
4. `group(1, 2)` returning several groups: needs variadic parameters and a tuple return.
5. `Match.lastindex` / `lastgroup`: the engine would have to record the order in which the groups
   close; the offsets cannot tell `((a))` from `(a)(b?)`.
6. `error` / `PatternError` (with `msg`, `pattern`, `pos`, `lineno`, `colno`), `RegexFlag`,
   `Scanner` and `Pattern.scanner`: errors are panics, flags are ints.
7. `Pattern.__eq__` / `__hash__` (equal when pattern and flags are), `__copy__` / `__deepcopy__`
   on `Pattern` and `Match`.
8. The flags `U` / `UNICODE`, `L` / `LOCALE`, `X` / `VERBOSE`, `DEBUG`.

### 41.2 Not aligned: behaviour

1. **A group that took no part gives `""`, not `None`**: in `group()`, `groups()`, `groupdict()`
   and the groups `split` inserts. `None` needs `str?` returns.
2. **`findall` with 2 groups or more panics** ("use finditer"): Python returns tuples then, which
   a `list[str]` cannot hold. With 0 or 1 group it is Python's.
3. ~~Empty matches follow Go~~: done 2026-11-01 (the C iterator has Python's `must_advance`).
4. **Syntax extras kept from RE2**: `\p{..}` / `\pL`, `\Q...\E`, `[[:alpha:]]`, `\x{...}` and the
   inline `(?U)` are accepted (Python has none of them). Compile errors use Go's wording
   ("missing closing )"). Python's `(?x)`, `(?L)` and the rule that global inline flags must come
   first are not supported.
5. **Offsets are byte offsets**, also in error positions of templates and in the printed span;
   Python's are character indexes. Kept: Koala's `str` indexes bytes.
6. **`Pattern.flags` does not include inline flags** such as `(?i)`, nor Python's default
   `UNICODE` (32).
7. **Template group names**: any non-ASCII character counts as a letter, where Python asks for
   `str.isidentifier()`.
8. **Printed form**: non-printable characters outside ASCII are not escaped (Python writes
   `'\u200b'`).
9. **Keyword arguments**: Koala passes only parameters with a default by keyword, so the required
   ones keep short names (`s`, not `string`).
10. **Unicode version**: the tables are Unicode 15.0 (generated from Go), Python 3.14 has 16.0;
    the 5004 `\w` and 80 `\d` characters added in 16.0 do not match. Checked over every code
    point: on the characters of 15.0, `\w \d \s \W \S \D`, `(?i)` on literals and classes, and
    the `ASCII` variants all agree with Python, except item 11.
11. **`(?i)` and the Turkish i**: Python's `(?i)[a-z]` also matches U+0130 `İ` and U+0131 `ı`,
    which Unicode simple folding (ours) keeps apart.
12. **`$` without `MULTILINE`** matches only at the very end; Python's also matches just before a
    final `\n` (`re.search('a$', 'a\n')` matches in Python, not here). `\Z` is the very end in
    both.

### 41.3 Compiler issues met on the way

- `x as str` on an `int | str` holding an int yields nil while its static type is `str` (the
  compiler warns that `!` on it is redundant); `koala/re` narrows with `is` only. Related to §29.
- Indexing a user class: §42. `<<=` / `>>=`: §43. An empty list of tuples: §44.

### 41.4 Next steps (2026-11-01, in this order)

1. **`$` as in Python** (41.2, item 12): without `MULTILINE`, `$` also matches just before a final
   `\n`. Lines read from a file keep their `\n`, so `re.search('x$', line)` is common and fails
   silently today. Small: a new assertion kind in the parser (`re_core.c`), `\Z` stays the very end;
   check against Python as for the other assertions.
2. **API doc `docs/koala_re_api.md`**, in the style of the other package docs: the API, the syntax
   (Python's, without backreferences and lookaround, plus the RE2 extras of 41.2 item 4), Unicode
   and `ASCII`, the templates of `sub`, byte offsets, the linear-time guarantee, and the
   differences of 41.2.
3. **Performance of `findall` / `finditer` / `sub` / `split`**: every `kl_re_iter_next` call goes
   through `kl_re_exec`, which allocates and frees its thread lists, stack and capture buffers;
   a find-all loop over a long text with many matches pays that per match. Reuse them across the
   loop (for example a scratch area kept in `ReIter` or passed by the caller).

## 42. Indexing an instance of a user class aborts the compiler (2026-11-01, to fix)

`x[i]` and `x[i] = v` on an instance of any user class that defines `__getitem__` /
`__setitem__` abort the compiler with `irgen.c:1490: Why goes here?`. The parser already resolves
and checks the call (`parse_index_load()` / `parse_index_store()` in `parser_expr.c`), but
`emit_ir_index()` (`irgen.c`) only lowers sequences (`KLR_SEQ_SET` / `KLR_SLICE_SET`) and maps,
and ends in `UNREACHABLE()` for anything else. It also hits the std classes with `__getitem__`
(`SortedDict`, `LinkedList`, `FrozenBytes`) and blocks `m[g]` on `re.Match` (§41.1, item 2).

### 42.1 Failing cases

Read (`irgen.c:1490: Why goes here?`; expected `4`):

```kl
class C {
    pub func __getitem__(i int) int = i * 2
}

func main() {
    let c = C()
    print(c[2])
}
```

Write (same abort; expected `7`):

```kl
class G {
    var _c list[int]

    pub func __init__() { _c = [0] }

    pub func __getitem__(i int) int = _c[i]

    pub func __setitem__(i int, v int) { _c[i] = v }
}

func main() {
    let g = G()
    g[0] = 7
    print(g[0])
}
```

Compound assignment on an index fails even on a list, with `error: cannot resolve expr's type.`
(expected `7`); the same on a class instance:

```kl
func main() {
    let l = [1, 2]
    l[1] += 5
    print(l[1])
}
```

A fuller regression test, with a union key (`__getitem__(key int | str)`, narrowed with `is`) and
a generic class (`Box[T]`), is kept as `test/test-run/test_class_index.kl.disabled`; rename it to
`.kl` once fixed.

### 42.2 Rejected attempt (2026-11-01)

A third index kind `KLR_METHOD_SET` in `KlrIndexInfo` (`ir.h`), carrying the `__setitem__`
callable and its return type to `emit_ir_simple_assignment()`, plus a direct `__getitem__` call in
`emit_ir_index()`. It passed the test above but was reverted:

- `KLR_SEQ_SET` / `KLR_SLICE_SET` stand for real IR instructions; `KLR_METHOD_SET` stands for none
  and only smuggles irgen data through a shared IR struct.
- It needed its own copy of the argument handling of `emit_ir_call()` (interface upcasts, casts
  for type parameters bound by a trait) and still missed what normal method calls handle:
  default parameters, classes of other modules, trait receivers, inherited methods.
- Compound assignment (`x[i] += v`) stays unsupported.

### 42.3 Proposed fixes

- **A (preferred)**: desugar in the parser. When the subscripted value is a user class, rewrite
  `x[i]` into the call `x.__getitem__(i)` and `x[i] = v` into the call statement
  `x.__setitem__(i, v)`; `parse_index_load()` / `parse_index_store()` already have the method
  symbol and check the arguments. irgen then needs no change, and every case of a method call
  works as it does today. `x[i] += v` can later become `x.__setitem__(i, x.__getitem__(i) + v)`
  (evaluating `x` and `i` once).
- **B (smaller)**: irgen only, no IR struct change. Keep the direct `__getitem__` call for loads;
  for stores, let `emit_ir_index()` only evaluate `x` and `i`, and have
  `emit_ir_simple_assignment()` build the `__setitem__` call from the IndexExpr's children. Still
  duplicates the call argument handling.

## 43. `<<=` / `>>=` abort the compiler (2026-11-01, fixed 2026-11-06)

**Fixed**: the compound assignment was not the cause. `var b = 6; b = b << 2` aborted too: constant
propagation made both sides constant, and `const_copy_prop.c` did not fold shifts, so isel saw a
shift of two constants. Constant folding now handles shifts (see §64), and
`test-run/test_shift.kl` covers both. The original record follows.

The shift compound assignments abort the backend with `isel.c:317: Why goes here?`. The other
compound assignments on the same local (`|=`, `&=`, `^=`, and `+=` / `-=` / `*=` / `/=` / `%=`)
compile. Failing case (expected `bits 12`, then `bits 6`):

```kl
func main() {
    var b = 3
    b <<= 2
    print("bits", b)
    b >>= 1
    print("bits", b)
}
```

Found while checking compound assignments for `koala/re`; located by the author: with the two shift
lines commented out, the rest of that check compiled. Not yet traced in `isel.c`.

## 44. An empty list of tuples cannot be created (2026-11-01, to fix)

Found while giving `re.Match` its `regs` field (`list[(int, int)]`). Both ways to start an empty
list of tuples fail; a non-empty literal works, so `koala/re` starts from `[(a, b)]` and appends.

1. **`list[(int, int)]()` aborts the compiler** (expected `1`):

   ```kl
   func main() {
       let xs = list[(int, int)]()
       xs.append((1, 2))
       print(xs.len())
   }
   ```

   `koalac: parser_expr.c:2622: parse_index_new_type: Assertion 'arg->ts->kind == TYPE_TYPE ||
   arg->ts->kind == TYPE_GENERIC_VAR' failed.` A tuple type argument is neither; `list[str]()`
   works.

2. **`[]` is not typed from the declared type, and the error is a raw internal message**
   (expected `1`):

   ```kl
   func main() {
       var xs list[(int, int)] = []
       xs.append((1, 2))
       print(xs.len())
   }
   ```

   The compiler prints `lhs: std/builtin.list<Lstd/builtin.tuple<jj>;> =/= rhs:
   std/builtin.list<o>` and fails, with no file or line: the empty literal stays `list[any]`
   instead of taking the declared element type.

`let xs = [(1, 2)]` followed by `xs.append((3, 4))` compiles and prints `2`.

## 45. String literals: no `\u` / `\U` escapes, unknown escapes kept silently (2026-11-01, to discuss)

Found while writing a `koala/re` test: `"1٣"` was meant as `1` and ARABIC-INDIC DIGIT THREE,
but Koala has no `\u` escape. A backslash before a letter that is not a known escape stays in the
string as it is, with no error or warning:

```kl
func main() {
    let a = "1٣"
    print(a.len(), a)      // 7 1٣
    let b = "\U0001F600"
    print(b.len(), b)      // 10 \U0001F600
    let c = "\x41"
    print(c.len(), c)      // 1 A
    let d = "\q"
    print(d.len(), d)      // 2 \q
}
```

The test then searched `1٣` with `\d` and found five digits instead of two; it now writes the
character itself (`"1٣"`). The manual does not list the string escapes.

To discuss:

1. Add `\uXXXX` and `\UXXXXXXXX` (as Python, Go, C), so a non-ASCII character can be written in
   ASCII source.
2. Make an unknown escape such as `\q` a compile error (Go, Rust) or at least a warning (Python
   3.12+ warns), instead of keeping it silently.
3. Document the escapes in the manual (Strings and Characters).

## 46. Language issues met while writing `koala/re` (2026-11-01, to fix / to discuss)

Each case was run on 2026-11-01; `koala/re` and its tests work around all of them.

1. **`ut.assert_eq` on a `str?` aborts the compiler** (to fix; expected `ok`):

   ```kl
   import "std/ut"

   func main() {
       let a str? = "y"
       ut.assert_eq(a, "y")
       print("ok")
   }
   ```

   `typespec.c:1575: Why goes here?`. The `re` tests unwrap with `!` first.

2. **`==` on lists compares identity** (to discuss): `print([1, 2] == [1, 2])` prints `false`.
   Python, Go (for arrays) and Rust compare the elements. The `re` tests compare lists element by
   element with small helpers (`assert_ints`, `assert_strs`).

3. **No tuple element access** (to discuss): `let t = (1, 2)` then `print(t.0)` fails with
   `error: expected an identifer.` (also a typo: "identifier"). Only destructuring works,
   `let (a, b) = t`.

4. **A parameter may not have the name of a field** (to discuss): with `let x int` in the class,
   `pub func __init__(x int) {}` fails with `parameter 'x' conflicts with field name in class 'P'`.
   It forced `re.Pattern(text, bits)` (the fields are `pattern` and `flags`) and
   `re.Match(pattern, text, ...)` (the fields are `re` and `string`); Python, Go and Rust allow it
   (`self.x = x`).

5. **No function-type parameters** (to do): a parameter such as `fn func(Match) str` does not
   compile (`zip.kl` keeps such code commented out). It blocks Python's `re.sub(func, s)`
   (41.1, item 2).

6. **`uint8` has no `__add__` / `__iadd__`** (to do, not a bug): `var u = uint8(250)` then
   `u += uint8(10)` fails with `neither '__iadd__' nor '__add__' is defined for class 'uint8'`,
   as the rule says; the operators are just not implemented for the small integer types yet.

Also for `koala/re`: its C macros keep the `RE_*` prefix (`RE_FLAG_*`, `RE_ERR_*`, `RE_MODE_*`,
`RE_MATCH`) while the functions are `kl_re_*`. glibc's `<regex.h>` defines other `RE_*` macros
(`RE_ICASE`, `RE_DUP_MAX`, `RE_SYNTAX_*`); none clashes today, but a source file that includes
both headers could. Renaming them to `KL_RE_*` was left for later on purpose.

## 47. `+` on `bytes` / `str` is treated as commutative: silent wrong values (2026-11-02, fixed; 47.3 to discuss, 47.5 to implement)

Found while adding `bytes.__add__` (so that `a + b` works on `bytes`) and replacing the hand-written
`_concat` helper in `libs/koala/http/request.kl`. Both bugs below are **silent** — no diagnostic, no
crash, just the wrong value — and both hit `str` as well as `bytes`.

1. **`normalize` swaps the operands when the left one is a constant** (expected `b"abcd"`, actual
   `b"cdab"`):

   ```kl
   func main() {
       let q = b"cd"
       print(b"ab" + q)
   }
   ```

   `--dump=no-opt-ir` is correct (`%0 = add b"ab" std/builtin.bytes, %q`); `--dump=ir` shows
   `normalize (Iter 1)` rewriting it to `%0 = add %q, b"ab"`. `src/parser/opt/normalize.c:12`
   `can_swap()` only returns 0 for `TYPE_STR`, so `bytes` — and any class with its own `__add__` —
   falls through to the swap done by `normalize.c:27`.

2. **`lvn` numbers `a + b` and `b + a` as one value** (expected `abcd` then `cdab`, actual `abcd`
   twice):

   ```kl
   func main() {
       let p = "ab"
       let q = "cd"
       print(p + q)
       print(q + p)
   }
   ```

   `src/parser/opt/lvn.c:154` `lvn_is_commutative()` includes `OP_BINARY_ADD`, and `lvn.c:208`
   canonicalizes the operand order of the hash key without looking at the operand types, so the
   second `add` is RAUW'd to the first one. The same happens with `bytes` operands, with two
   constants in either order (`b"ab" + b"cd"` then `b"cd" + b"ab"` prints `b"abcd"` twice), and
   inside a loop.

Both are intra-function only: routing the `+` through a call boundary returns the right answers,
which is how the new tests in `test/test-ut/test_bytes.kl` work around it (`add_bytes(a, b)`, with
the reason written in a comment there). `-`, `/`, `%`, `<<`, `>>` are not affected (`a - b` /
`b - a` and `a / b` / `b / a` print their four distinct values); `koala/http/request.kl` is safe
because its left operand is a call (`head.to_bytes()`), not a constant.

### 47.1 Root cause

`src/parser/irgen.c` lowers every binary / unary operator to the same `OP_BINARY_*` / `OP_UNARY_*`
insn whatever the operand type, but the passes treated these insns as pure number ops. That only
holds on numbers. On `str` / `bytes` / `list` `+` is concatenation (not commutative, and a `list`
result is a fresh mutable object), and on a user class an operator is a method call (`__add__`)
that may have side effects. Besides the two cases above, the same assumption gave:

3. **`lvn` merges two same-order `a + b`**: on lists `l1 = a + b; l2 = a + b; l1.append(9)` printed
   `[1, 2, 3, 9]` for both; on a user class whose `__add__` bumps a counter, `c + d` twice printed
   `1 1` instead of `1 2`.
4. **`dce` deletes an unused operator on a user class**: `c + d` as a statement, then
   `print(c + d)`, printed `1` instead of `2` — the first `__add__` call was dropped.

The key `(op, lhs, rhs)` of `lvn` is enough for pure ops; what was missing is the admission check,
so every fix below is a per-pass type whitelist.

### 47.2 Fix (2026-11-02)

- `normalize.c` `can_swap()`: whitelist `TYPE_INT` / `TYPE_FLOAT` / `TYPE_BFLOAT16` instead of
  excluding only `TYPE_STR`.
- `lvn.c` `lvn_is_value_insn()`: an `OP_BINARY_*` / `OP_UNARY_*` insn is numbered only when every
  operand is int / float / bfloat16 / bool. `str` is left out on purpose, so the commutative key
  reordering needs no extra type check.
- `dce.c` `is_value_insn()`: in `has_side_effect()`, an operator is removable only on int / float /
  bfloat16 / bool / str / optional (`x != nil`); on bytes / list / user class it is kept.
- `backend/lsra.c` `unused_needs_reg()`: a kept `OP_NUM_*` with an unused result used to get no
  interval and was emitted as `num.add r255, ...` (VM assertion `rd < max_regs`); the 16
  `OP_NUM_*` codes now get a one-position interval. Calls still get none (`vreg -1` = no return
  register, `call flg=0, #1`). This one is a stop-gap, see 47.3.

Cost: `str` operators are no longer value-numbered, and unused operators on bytes / list are no
longer deleted. Regression tests: `test/test-run/test_normalize_add_order.kl`,
`test_lvn_add_order.kl`, `test_lvn_add_object.kl`, `test_dce_klass_op.kl`. The `add_bytes(a, b)`
work-around in `test/test-ut/test_bytes.kl` can now go back to a plain `+`.

### 47.3 `lsra` register for an unused `OP_NUM_*` (stop-gap, to discuss)

The `lsra.c` `unused_needs_reg()` change in 47.2 works but is not a good design; revisit later.

Why a register is needed at all: every `OP_NUM_*` handler in `include/runtime/vm_ops.h` decodes `rd`,
checks it with `CHECK_REG_ID(rd)` (`src/runtime/eval.c:110`, `ASSERT((id) < max_regs)`) and writes
`regs[rd] = ret` unconditionally. An unallocated value has `vreg -1`, encoded as `rd = 255`:

- debug build: the assertion fails (`_eval_frame: Assertion '(rd) < max_regs' failed`);
- release build: `ASSERT` is a no-op under `NDEBUG` (`include/common/common.h:84`), so the VM writes
  `regs[255]` past the frame's registers — silent memory corruption.

`OP_CALL` does not have this problem because it has a real "no result" encoding: cgen prints
`call flg=0, #1` for `vreg -1`, and the VM only writes when `rd != 0xFFF`.

Why the stop-gap is not satisfying:

- the whitelist of 16 opcodes has to follow every opcode that a user-class operator may lower to
  (it misses the future object unary op of 47.5, for instance);
- it spends a register (and maybe one more `nlocals`) on a value nobody reads;
- the rule "a value with no use gets no register" now has an exception that lives in `lsra`, far
  from the `dce` decision that caused it.

Options to discuss:

1. a "discard result" encoding for `num.*`, like `call` (e.g. `rd == 255` skips the write): cgen
   plus the 16 `OP_NUM_*` handlers, and one more test on a hot path;
2. lower an operator on a user class to a real method call in `isel` (or `irgen`), so the unused
   case reuses the no-result `call` form; this also matches the "operator is a method call" view
   of 47.1;
3. keep the stop-gap but derive it from the type, not the opcode.

Option 2 is the one §49 leans to: it is also what lets a `pure` declaration on `__add__` decide
whether the call may be deleted or merged.

### 47.4 Note on `dce` vs `lvn` whitelists

They differ on purpose: `dce` only needs "no side effect", so `str` / optional are removable; `lvn`
also needs "same inputs give the same, interchangeable value", which rules out `str` (`+` is not
commutative) and any object result.

### 47.5 Unary operators on a user class (to implement)

`__neg__` / `__bitnot__` / `__not__` are in the dunder table of `parser_expr.c` (`-c`, `~c`, `!c`),
but the checker rejects them on a user class:

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

reports `unary '-' operator requires int or float type.` (`parser_expr.c`, unary check; it carries
a `// TODO: check _neg__ exists or not`). `+`, `~`, `!` have the same int / float / bool-only checks.
When this is implemented, the unary path must match the binary one end to end:

- `irgen.c` lowers `-x` to `OP_UNARY_NEG`, `~x` to `OP_UNARY_NOT`, `!x` to `OP_LNOT`, and `+x` to
  nothing at all (no-op, there is no `__pos__`).
- `backend/isel.c`: `isel_lower_unary_neg()` hits `UNREACHABLE()` on any type other than int / float,
  and `isel_lower_unary_bit_not()` asserts int; both need an object path (an `OP_NUM_NEG`-like insn
  or a call).
- `lvn` / `dce`: `OP_UNARY_NEG` / `OP_UNARY_NOT` already go through `lvn_is_value_insn()` /
  `is_value_insn()`, so a user-class operand is neither numbered nor deleted. `OP_LNOT` does not:
  `dce` `has_side_effect()` returns 0 for it, so an unused `!c` on a user class would be deleted —
  it needs the same whitelist case.
- `lsra.c` `unused_needs_reg()`: add the object unary code, or a kept unused `-c` writes `r255`.
- Test: the case above in `test_dce_klass_op.kl` (unused `-c`, then `print(-c)` prints `20`).

---

## 48. `bytes` / `str` leftovers after the `in` refactor (2026-11-02, to decide / to implement)

**What landed (not a todo).** `bytes.__contains__` now takes `uint8 | bytes`, and `in` reads a
`bytes` operand as a sub-sequence search (`src/objects/bytesobj.c:295`), so a separate
`contains_bytes` method is not needed. To make that union parameter usable, an integer or float
literal now narrows to the **first union member that can hold it** (`narrow_lit_int`,
`src/parser/parser_expr.c:178`); a literal no integer member can hold is reported once, against the
first integer member (`300 in bs` → `Value 300 overflows 'uint8' range`). The rule is documented in
`docs/Koala_Language_Manual.md:557` (Union Types). Tests: `test/test-ut/test_bytes.kl`
(`test_bytes_contains`, `test_bytes_contains_view`) and `test/test-kl/test_bytes_in_range.kl`.

### 48.1 `bytes` API surface: to decide

1. **Splitting on whitespace.** `split(sep bytes, maxsplit = -1)` / `rsplit` require a separator,
   so `b"a b  c".split()` does not exist. Two shapes: a separate
   `split_whitespace(maxsplit = -1)` (no signature change, no optional-argument work) or
   `split(sep bytes? = nil, maxsplit = -1)` (closer to Python, needs a `nil` default and a `nil`
   branch in C). `str.split` has the same gap — its default is `sep = " "`, so `"a  b".split()`
   yields empty parts instead of splitting on a whitespace run.

2. **`strip` / `lstrip` / `rstrip` accept no character set** — only the fixed whitespace set in
   `bytes_is_space` (`src/objects/bytesobj.c:809`: space, `\t`, `\n`, `\r`). Two things to decide:
   add `chars bytes? = nil`, and whether the whitespace set should match Python's `\t\n\v\f\r `
   (we currently miss `\v` 0x0B and `\f` 0x0C, so `b"a\x0c".strip()` keeps the form feed).

3. **`splitlines()` has no `keepends`.** The Python default matches ours; `keepends = true` is
   missing.

4. **`is_ascii()` is missing on both `bytes` and `str`.** `str` has `isalpha` / `isdigit` /
   `isalnum` / `isspace` / `islower` / `isupper`; `bytes` has none of them (see item 7).

5. **`count_bytes(sub, start, end)` is missing.** `count(value uint8, ...)` counts single bytes
   only, while `index_bytes` / `rindex_bytes` already search sub-sequences, so a sub-sequence count
   has no equivalent: `b"aaaa".count_bytes(b"aa")` (Python: 2, non-overlapping) is rejected with
   `'count_bytes' is not found in class 'bytes'` (verified 2026-11-02).

6. **`__eq__` / `__ne__` / `hash` are implemented and registered in C but not declared in
   `bytes.kl`** (`src/objects/bytesobj.c:991-997`), and the class header is
   `bytes : Sequence[uint8]` where `str` is `str : Sequence & Comparable & ToString`. Nothing breaks
   today — trait bounds and method signatures are not checked (48.2 item 6), so `b"a" == b"a"`,
   `b"a" < b"b"`, `hashset[bytes]`, `dict[bytes, int]` and even a `[T : Comparable]` generic called
   with two `bytes` all run correctly (verified 2026-11-02). The question is whether the header
   should tell the truth: declare the three natives plus `& Comparable & Hashable` (and use
   `Comparable` instead of the hand-written `__lt__`…`__ge__` block), or record that `bytes` exposes
   equality and ordering only through natives, without claiming the traits. Either way it is a
   documentation/consistency decision until conformance checking exists.

7. **`str` → `bytes` parity.** Comparing the two APIs today (`pub func` names in `bytes.kl` vs
   `str.kl`):

   | | names |
   | --- | --- |
   | `str` has, `bytes` lacks | `capitalize`, `center`, `ljust`, `rjust`, `zfill`, `lower`, `upper`, `isalpha`, `isdigit`, `isalnum`, `isspace`, `islower`, `isupper`, `remove_prefix`, `remove_suffix`, `substr`, `to_bytes`, `to_int`, `to_int_or`, `to_float`, `to_float_or`, and `__eq__` / `__ne__` (item 6) |
   | `bytes` has, `str` lacks (by design) | `copy`, `copy_str`, `fill`, `zero`, `__setitem__`, `__setslice__`, `index_bytes`, `rindex_bytes` |

   Both declare `__add__`, `__contains__`, `__getitem__`, `__getslice__`, `__lt__`…`__ge__`,
   `__init__`, `__mul__`, `count`, `empty`, `ends_with`, `fmt`, `has_next`, `index`, `iter`, `join`,
   `len`, `lstrip`, `next`, `next_strict`, `replace`, `reversed`, `rindex`, `rsplit`, `rstrip`,
   `split`, `splitlines`, `starts_with`, `strip`, `to_str`, `view`.

   Which of these belong on `bytes` at all? A byte array is a container, so `substr` /
   `to_int` / `to_float` could stay off the API with `to_str()` as the bridge (`str.to_bytes` already
   covers the other direction); the plausible additions are the byte-level case fold
   (`lower` / `upper`) and the `is*` predicates, each needing an ASCII-only definition written down
   (a byte array has no encoding).

8. **Deliberately absent — record the decision so it is not re-litigated:**
   `translate` / `maketrans`, `partition` / `rpartition`, `expandtabs`, printf-style `%` formatting,
   and `decode(encoding)` (only UTF-8 via `to_str()`, which panics on invalid input — a
   non-panicking `to_str_or` / `validate` is a separate gap).

   Hex was on this list and was reversed on 2026-11-03: `bytes.to_hex(sep = "", bytes_per_sep = 1)`
   and the static `bytes.from_hex(s)` now carry the hexadecimal text of a byte array (`sep` is any
   string, wider than Python's single character; separator groups counted from the right for a
   positive `bytes_per_sep`, from the left for a negative one;
   `from_hex` skips ASCII whitespace between bytes and panics with the position of a bad digit).
   The `koala/hex` package is removed and `koala/crypto` uses `to_hex()`; `koala/json/string.hex_byte`
   still formats its single `\u00XX` byte by hand. Tests: `test/test-ut/test_bytes_hex.kl`.

### 48.2 Union types and literal narrowing: remaining gaps

1. **A union type is only accepted in a parameter position.** A `var` declaration or a class field
   fails to parse (verified 2026-11-02):

   ```kl
   var v int | str = 0          // error: expected ';' or '='
   var v (int | str) = 0        // error: expected a type or '='
   class C { var v int | str }  // error: expected ';' or '=', then: expected field-decl or method-decl
   ```

   `docs/Koala_Language_Manual.md` shows exactly the `var v int | str = 0` example under Union
   Types, so the manual is ahead of the parser: either teach the var-decl / field type-spec path to
   take a union, or drop the example from the manual.

2. **Narrowing is literal-only, by design.** `let i = 97; i in bs` is a compile error (an `int`
   variable is not implicitly narrowed, matching the "no implicit narrowing" rule); `uint8(i) in bs`
   works and truncates a runtime value, while a constant out of range is rejected
   (`uint8(300)` → `constant integer overflow in cast`). `(i as uint8) in bs` fails with
   `src of 'as' operator is not a valid type`, which looks like a separate `as` defect worth its own
   report.

3. **Float literal narrowing does not range-check.** The first float member is taken as is
   (`src/parser/parser_expr.c:302-312`), so a `float16` member would accept `1e40` unchanged. No
   such member exists in `std` today, so nothing breaks yet.

4. **Several integer members: first-that-fits, no ambiguity diagnostic.** With
   `func f(v uint8 | int)`, `f(7)` receives a `uint8` and `f(300)` an `int` (verified). The rule is
   documented in the manual; add a warning only if the silent choice proves to confuse.

5. **Only `TYPE_INT` / `TYPE_FLOAT` members participate.** Enums, aliases and `bool` members never
   take a literal, so a union such as `uint8 | bool` keeps `true` on the `bool` member (correct) but
   would never narrow an integer literal to an enum member.

6. **Trait conformance does not check method signatures, and generic bounds are not checked
   either.** A class declared as `Foo : Sequence[uint8]` with `__contains__(item str) bool` compiles
   and runs (verified: `"x" in f` prints `false`), while
   `libs/std/builtin/collection.kl:55` declares `func __contains__(item T) bool`; likewise
   `func maxi[T : Comparable](a T, b T)` accepts two `bytes` although `bytes` does not declare
   `Comparable` (verified). So the widened `bytes.__contains__(uint8 | bytes)` is accepted by
   accident rather than by a contravariance rule. If conformance checking is ever tightened,
   parameter widening (contravariant, sound) must become an explicit rule, otherwise `bytes` stops
   conforming to `Sequence[uint8]`. The same hole applies to every trait method and bound in `std`.

### 48.3 To fix in `str` / `bytes`

1. **`str.__mul__` asserts on a non-positive count.** `"ab" * -1` aborts the process
   (`strobj.c:336: Assertion 'n >= 0' failed`), while its own doc says "Return an empty string if
   `n` is not positive" (`libs/std/builtin/str.kl:161`) and `bytes.__mul__` already returns the
   shared empty array for `n <= 0`. Fix: drop the `ASSERT` and return the empty string, mirroring
   `_bytes_mul`.

2. **The whitespace set is narrower than Python's** (see 48.1 item 2): `\v` and `\f` are not
   stripped, and the doc comments on `strip` / `lstrip` / `rstrip` enumerate the same four
   characters, so doc and C agree with each other but not with Python.

3. **`test/test-ut/test_bytes.kl` formatting leftovers.** Three stray double blank lines (around
   lines 387, 635 and 782) that `kl-fmt` would delete, plus the comment block at 1180-1196 (see
   48.4 item 1).

### 48.4 Tooling, artifacts and commit scope

1. **`kl-fmt` rewrites code inside comments.** `test/test-ut/test_bytes.kl:1180-1196` keeps verbatim
   compiler diagnostics as documentation
   (`// print(uint8(300)) // error: constant integer overflow in cast: cannot cast from int64 to uint8`);
   `python3 tools/kl-fmt.py --diff` wants to wrap those lines at 100 columns and re-indent them.
   Until this is settled the file can never be `--check`-clean. Decide: skip `//`-only blocks, add a
   `// kl-fmt: off` / `on` directive, or accept the reflow.

2. **`dist/std/*.klc` are build outputs that the compiler loads.** Every `.kl` edit needs
   `libs/std/build-and-test.sh` (or `wombat`) re-run and the regenerated
   `libs/std/dist/std/builtin.klc` committed together with the source — otherwise a fresh checkout
   compiles against stale bytecode. To decide: always include the regenerated `.klc` in the commit
   (recommended, since the compiler reads bytecode), and whether a CI check should verify that the
   committed `.klc` matches a fresh build.

3. **`libs/std/dist/std/fs.klc` is rewritten by every build although its source is untouched** —
   same size, different bytes; it had to be `git checkout`-ed back several times on 2026-11-02. Find
   the nondeterminism (timestamp / hash-map iteration order in the serializer?) or ignore the file
   in git.

4. **Commit split for the current working tree** (nothing is committed yet). Suggested:
   (a) the `bytes` / `str` operator and method batch — `__mul__`, `__lt__`…`__ge__`, `join`,
   `replace(count)`, the empty-`bytes` singleton, `test_empty_bytes_stable`, and the `str.join`
   refactor (`strobj.c`, `str.kl`, `test_str.kl`);
   (b) `bytes.__contains__(uint8 | bytes)` + union literal narrowing (`parser_expr.c`) + manual +
   tests;
   (c) `libs/koala/http/response.kl` (+ its `.klc`).
   To decide as well: whether the staged binaries under `libs/koala/dist/**/*.so`
   (`libkoala_crypto.so`, `libkoala_tls.so`) belong in the repository at all.

### 48.5 Cross-references

- Mutable `bytes` literals sharing the const pool: §37 "Open" item 1 (`docs/Koala_TODO.md:2072`) —
  still open, unchanged by this refactor.
- `+` on `bytes` / `str` treated as commutative by `normalize` / `lvn`: §47 — fixed on 2026-11-02
  (47.2); the `lsra` stop-gap (47.3) is still to discuss.
- Single-type-parameter generic **classes** whose type parameter has a trait bound crash the
  compiler: §16.1b (`Heap[T : Comparable]`, `SortedSet[T : Comparable]`, …). Free generic functions
  are unaffected (`maxi[T : Comparable]` works), so this does not block 48.1 item 6; it does block
  the `sortedset` / `heapq` family that would use a `Comparable` `bytes`.
- `is` / `as` operator leftovers: §29 — see 48.2 item 2 for the `i as uint8` failure.

---

## 49. Side effects of operators and an explicit `pure` declaration (2026-11-02, to design)

### 49.1 Where it comes from

While fixing §47, `dce` deleted an unused `c + d` on a user class whose `__add__` bumps a counter, and
`lvn` merged two `c + d` (and two `a + b` on lists, which must be two fresh lists). One way out was
to declare that operators must be side-effect free, so that both rewrites become legal. **Not
adopted**: no mainstream language makes that rule, and side-effecting operators are an ordinary
idiom (C++ `std::cout << x`, pipe / query-builder DSLs, counting or logging operators). Koala has
`__shl__`, so `log << msg` is just as likely; under such a rule an unused `log << msg` would vanish
without any diagnostic — the same kind of silent wrong value as §47.

### 49.2 How other languages declare purity

Written from memory, not checked online; details may be off.

Operator overloads are plain calls in Python (`type(a).__add__(a, b)`, always called), C++
(`operator+`), Rust (`Add::add`), Swift / Kotlin / C# / Scala (`operator` functions): the compiler
may drop one only under the as-if rule, i.e. after inlining proves nothing observable happens. The
one standard exception that skips user code with side effects is C++ copy elision. Haskell is pure
as a whole language, not as an operator special case.

Explicit declarations, by who guarantees them:

| Kind | Language | Syntax | Meaning |
|---|---|---|---|
| compiler-checked | D | `pure int f(int x)` | no mutable globals, only calls `pure` functions, no IO; "weakly pure" may write through pointer params, only "strongly pure" (immutable params) is optimized; `debug` blocks are an escape hatch |
| | Nim | `func f(x: int): int` = `proc` + `{.noSideEffect.}` | checked by the effect system |
| | Fortran | `PURE FUNCTION`, `ELEMENTAL` | no IO, no global writes, function args `INTENT(IN)` |
| | Haskell / Koka | default pure; effects in the type (`IO a`, Koka `console` / `exn`) | type system |
| programmer-asserted (trusted, wrong = UB) | GCC / Clang | `__attribute__((pure))` / `__attribute__((const))` | `pure`: no side effects, may read global memory; `const`: depends only on args |
| | C23 | `[[reproducible]]` / `[[unsequenced]]` | roughly `pure` / `const` |
| | Julia | `Base.@assume_effects :effect_free :consistent :nothrow :terminates_globally ...` | finest split: `:consistent` means same result by `===` identity |
| | Swift (stdlib only) | `@_effects(readnone / readonly)` | internal, underscored |
| tooling only | C# | `[Pure]` | docs / analyzers, not used by the compiler |
| | Java | `@Contract(pure = true)`, `@CheckReturnValue` | IDE warnings |
| | Rust / Kotlin | `#[must_use]` / `@CheckResult` | the opposite tack: warn on an unused result instead of deleting the call (Rust dropped a `pure` keyword before 1.0) |

### 49.3 What matters for Koala

1. **"Removable" and "mergeable" need different properties.** `dce` deleting an unused call needs
   no side effects, no panic, and termination (deleting an `__add__` that panics on mismatched sizes
   swallows the error). `lvn` / CSE merging two calls also needs the results to be interchangeable —
   Julia's `:consistent`, compared by identity — which is exactly why two `a + b` on lists (two
   fresh mutable lists) cannot be merged. GCC's `pure` only covers the first part; in C it hardly
   matters because results are mostly scalars.
2. **Safe by default.** An undeclared user function or operator is an ordinary call. Built-in types
   are known to the compiler, as the §47.2 whitelists do today.
3. **Checked vs trusted.** D / Nim style needs a transitive check ("a `pure` function only calls
   `pure` functions"), so the native functions of the standard library (`str.__add__`, ...) must be
   annotated first. GCC / Julia style only reads a flag, but a wrong declaration is again a silent
   wrong value.

### 49.4 Direction (leaning, not decided)

- A compiler-checked `pure` modifier in the D / Nim spirit, with Julia's split into at least two
  levels: "removable" (unused call may be deleted) and "mergeable" (same-argument calls may be
  merged; implies removable, and the result must be interchangeable).
- For operators, together with §47.3 option 2: lower an operator on a user class to a real method
  call, so `dce` / `lvn` / `lsra` all see a call, and the `pure` level of `__add__` decides whether
  it may be deleted or merged. The §47.2 whitelists in `dce` / `lsra` could then go.

### 49.5 What the compiler has today (survey, 2026-11-02)

- **Modifiers / annotations**: the only modifier keywords are `pub` and `static`. Annotations are
  `@name`, `@name(...)` on their own line, at most one per declaration (`koala.y` `prefix` /
  `annotation`); `check_func_prefix()` in `parser.c` accepts `intrinsic`, `native`, `specialized`,
  `test`, `should_panic`, `skip`, `bench`, anything else is "unknown annotation". Trait methods
  (`trait_method`) take no annotation at all.
- **Symbol flags**: `SYM_FLAGS_*` in `include/parser/symbol.h` (MUTABLE, CONST, PUBLIC, EXT,
  GENERIC, NATIVE, STATIC), bit 7 and up free.
- **`.klc`**: `KLC_FLAGS_*` in `include/common/klc.h` (bit 7 and up free); `write_meta_func()` in
  `write_klc.c` writes only PUB / STATIC / METH to the meta `KlcFunc`, `read_klc.c`
  `to_symbol_flags()` maps them back. Annotations are written but not read back into `FuncSymbol`
  (only the test runner reads them), so a purity flag must be a real meta flag.
- **Native methods**: `@native` / `@intrinsic` with an empty body, bound by name at load time
  (`load_module.c`, `kl_get_native`); there is no purity metadata anywhere.
- **No notion of a pure function** exists. `KLR_INSN_FLAGS_CONST` on a call (`[const]` in the IR
  dump) only means every argument is a literal (`klr_build_call` in `klr/insn.c`). `dce.c` treats a
  `[const]` call whose callee is a `KLR_VALUE_KLASS` (a local class constructor) as side-effect free,
  which is unsound when `__init__` has side effects — the same assumption problem as §47.
- **Operators**: the checker finds `__add__` on a user class (`parser_expr.c`) but does not attach
  the `FuncSymbol` to the IR; `emit_ir_binary()` always emits `OP_BINARY_*`; the VM dispatches
  `OP_NUM_ADD` through `kl_call_slot(..., SLOT_ADD)`. No operator is lowered to a method call today.
- **Traits**: `build_class_intf_table()` in `symbol.c` matches trait methods to impls by name only —
  no signature or flag check — so "`pure` on a trait method binds every impl" needs a new check
  there (and the flag must survive the `.klc` round trip for imported traits).

### 49.6 To design

- Syntax: a new modifier keyword (like `pub`) or an annotation (`@pure`); note an annotation today
  is one-per-declaration, on its own line, and not allowed on trait methods. Names of the two levels.
- Check rules: transitivity; what a `pure` body may read (fields of `self`? globals declared
  `let`?); allocation of a fresh object (fine for "removable", not for "mergeable"); panics.
- Native functions: how a C-implemented method declares its level.
- Traits: whether `pure` on a trait method (e.g. `Number.__add__`) binds every implementation.
- Serialization: how the flag travels in `.klc` so an importing module can use it.
- An escape hatch for debug logging inside a `pure` function (D's `debug` blocks).

---

## 50. A bare `{ ... }` block statement is never executed (2026-11-02, fixed 2026-11-02)

Found while testing "the `;` of the last statement before `}` may be left out" (Manual, Branching and
Loops). Unrelated to that change: it fails the same way with explicit `;`. **Silent**: no
diagnostic, the statements in the block just never run.

```kl
func main() {
    var x = 0
    { x = 1; }
    print(x)                  // expected 1, prints 0
    { print("a"); print("b"); }   // prints nothing
    if true { print("d") }    // fine
}
```

`--dump=no-opt-ir` shows each bare block as its own basic block with `;; No preds!`, after the
`print(x)` of the entry block. Cause, `src/parser/irgen.c` `emit_ir_block()`:

- it appends a new basic block (`klr_append_block(fn, "block")`) and emits the statements there,
  but never emits a `jmp` from the current block into it;
- it never emits a `jmp` out of it either, and after `exit_scope()` the statements that follow go
  on in the old block (`ps->scope->bb` is unchanged), so the new block is dead and later passes
  drop it.

Fix (2026-11-02): `emit_ir_block()` no longer creates a basic block. A block statement only opens a
scope: its statements go on in the current block (`sc->bb = ps->scope->bb` before entering), and the
block where they end — moved on by a nested `if` / `while` — is written back to the outer scope after
`exit_scope()`. A body ending in `return` / `break` / `continue` needs no special case: its last block
already has a terminator. The other option (keep the block and add `jmp` in / `jmp` out) gives a
block with one unconditional predecessor and one successor, which `cfg_merge_block_pass` would merge
back anyway.

Tests: `test/test-run/test_bare_block.kl` (runs with `--dump=no-opt-ir` and checks no `(block)`
basic block is left; covers empty `{}`, nested empty and nested blocks, a block entered from the
entry block and from the join block of an `if` / a `while`, blocks inside branches and loop bodies,
scoping, a `while` inside a block, `return` / `continue` inside a block);
`test/test-run/test_block_last_stmt.kl` is back to bare blocks.

---

## 51. `break` / `continue` look for a loop past a function boundary (2026-11-02, latent)

Found while discussing statement blocks in `if` expressions (not adopted: an `if` expression stays
one value per branch, it only replaces `?:`). Read from the code, not reproducible today.

`find_loop_scope()` (`src/parser/parser.c`), used by `parse_break()` / `parse_continue()`, walks
`ps->scope` outwards until it meets a `WHILE_BLOCK` / `WHILE_LET_BLOCK` / `FOR_BLOCK`, but it never
stops at a `SCOPE_FUNC` or `SCOPE_ANONY` scope. A `break` in a function body nested inside a loop
would therefore find the outer function's loop and pass the check, and irgen would jump to a block of
another function.

It cannot happen today: anonymous functions and closures are not part of the language (Manual,
Functions), and a `func` declared inside a function body is a syntax error. Fix it together with
closures (or local functions): stop the walk at `SCOPE_FUNC` / `SCOPE_ANONY` and report "break
statement must be inside a loop." there; `return` already resolves through `get_current_function()`,
which stops at the nearest `SCOPE_FUNC` (check that it also stops at `SCOPE_ANONY` then).

---

## 52. `BufReader.read_line` keeps the '\r' of a CRLF line (2026-11-03, to decide)

`read_line` (`libs/std/io/bufio.kl`) is `read_until(out, '\n')`: the '\n' is consumed and not
appended, but the '\r' before it is kept, so `"a\r\nb"` reads `"a\r"`. `Lines` strips both
(`libs/std/io/splitter.kl`), so line iteration and `read_line` disagree, and every CRLF protocol has
to strip it by hand (`koala/redis/protocol.kl` does `line.to_str().remove_suffix("\r")`).
`test/test-ut/test_read_line_byte.kl` (`test_read_line_keeps_cr`, and its header comment) pins the
current behavior.

Other languages: a call that drops the '\n' also drops the '\r' (Go `bufio.Reader.ReadLine` and
`Scanner`, Java `BufferedReader.readLine`, C# `StreamReader.ReadLine`, Rust `lines()`); a call that
keeps the '\n' keeps the '\r' (Go `ReadString('\n')`, Rust `read_line`, Python binary
`readline`). Koala's `read_line` is in between.

Plan (not applied, open to adjustment):

- Semantics: drop the one '\r' right before the '\n'; keep a '\r' inside a line (a lone '\r' is not
  a line end); drop a trailing '\r' of a last line without '\n' (as Go's `Scanner`); the returned
  count excludes the dropped '\r'. `read_until`, `Lines` and `Splitter` are unchanged; raw bytes
  stay available through `read_until(out, '\n')`.
- Implementation, `bufio.kl` only: `read_line` gets its own loop with a local "held-back '\r'"
  flag, since the '\r' may end one chunk and the '\n' start the next. When a chunk without '\n'
  ends in '\r', that byte is not written and the flag is set; the next chunk drops it if it starts
  the line end, otherwise writes it first; at EOF it is dropped. A flag rather than leaving the '\r'
  unconsumed in the read buffer: with capacity 1 the held byte would fill the buffer and `fill()`
  could never read again. No `Buffer.truncate` is needed. The parameter is renamed `_buf` -> `out`
  as in `read_until`.
- Docs: the `read_line` doc comment and its entry in `docs/std_io_api.md`.
- Tests: change `test_read_line_keeps_cr` to expect `1` / `"a"` and fix the header comment (an
  existing test, to be agreed first); add: '\r' and '\n' in two chunks (capacity 2), a '\r' inside a
  line, a held '\r' followed by another byte, an empty CRLF line, a '\r' at EOF without '\n',
  capacity 1 with CRLF lines, CRLF lines interleaved with `read_byte`.
- Open points: whether a lone '\r' should also end a line (Java does); whether to add a variant
  that keeps the line ending.

---

## 53. Reading an uninitialized local gives a garbage value: no definite-assignment check (2026-11-03, to fix)

Found in `test2.kl`. **Silent** wrong values, then a VM crash:

```kl
func test(j int) {
    var i int
    if j > 10 { i = 10 } else { }
    print(i, typeof(i))   // j = 2: prints the module search-path list and <class 'list'>
    print(i + 1)          // VM assertion in vm_ops.h (operand tag check)
}

func t() {
    var i int
    print(i)              // prints 40960
}
```

The compiler has no definite-assignment analysis at all: a local declared without an initializer
gets a register that still holds whatever an earlier use left there, and reading it is accepted.
`docs/Koala_Language_Manual.md` (Constants and Variables) states the opposite: "Every variable must
be definitely assigned before it is read; the compiler rejects uses of uninitialized values" — the
manual is ahead of the compiler.

Fix direction (not applied): a definite-assignment pass in the checker (`parser.c` /
`parser_expr.c`) over each function body, tracking per local whether it is assigned on every path
reaching a read: assigned after `x = ...` / `let` / `var` with an initializer / tuple assignment;
an `if` assigns only when every branch assigns (a branch that ends in `return` / `break` /
`continue` / `panic` does not need to); a loop body does not assign for the code after the loop;
let-else, `if let`, `while let`, `for` variables and `if` expressions included; report
"variable 'i' is used before being assigned" at the read. Until then, either give such locals a
zero value of their type or keep the manual honest.

---

## 54. Class and trait names used as values need consistent errors (partially checked, to fix)

Found together with §53 in `test2.kl`.

```kl
class P {}
print(int)
print(P)
print(ToString)
let t = int
```

A class name in a value position can reach irgen as a class reference (`KLR_VALUE_KLASS`), but
the backend has no instruction that loads it into a register. Trait names also lack a complete
value path. Class objects in tuple construction, including `print`'s variadic arguments, now
receive an irgen error; this does not cover all value contexts or trait names.

Until class and trait names are supported as first-class values, report a clear compile error in
semantic analysis wherever they are used as values: `let` / `var` initializers, assignments, call
arguments, returns, tuple and other container elements, and nested expressions. Cover local,
external, builtin and generic type names, and test optimized and non-optimized compilation.
Keep existing more specific type errors where appropriate; do not allow invalid input to reach
compiler assertions.

Distinguish a type-name expression from an instance expression: an expression's symbol alone can
also refer to the class of an instance. Preserve type annotations, constructor and conversion
calls (`int(x)`, `P()`), generic type arguments (`list[int]`), static member access
(`bytes.from_hex`), class and trait instance values (including trait-typed variables, parameters
and returns), and runtime values returned by `typeof`.

Fixed alongside (2026-11-03): `list[any]()` asserted in `parse_type` (`parser_expr.c`), which
expected a class while the keyword type `any` is a trait (`libs/std/builtin/any.kl`); it now accepts
a class or a trait and reports anything else. Test: `test/test-run/test_generic_any_arg.kl`.

---

## 55. `sealed trait`: a closed set of implementers (2026-11-04, to design)

A root trait such as `Value` in `koala/toml` (`Integer`, `String`, `Table`, ...) gives a type a
limited set of cases without `any`, and reads go through `as`. The set is not closed: another
package can write `class Foo : toml.Value`. The risk is small, because `as` is a checked conversion:
the worst result is a panic or a default branch, not a garbage value.

```kl
pub sealed trait Value {
    func to_toml() str
}
```

Agreed so far:

- A `sealed trait` can only be implemented by a class in the same package, following Kotlin. The
  limit is the package and not the file, since `koala/toml` keeps one type per file.
- Another package can still use it: declare variables, call its methods and convert with `as`.
- A trait in another package cannot inherit it, or that trait would be a way around the limit.
- An enum is a kind of sealed type, whose cases are created in its own definition.

To decide when this is designed, together with enum and `match`:

- Whether enum is sugar over `sealed trait`, sharing one implementation of narrowing and `match`.
- Whether `match` must handle every case.
- `permits`, listing existing types such as `int` and `str`, is not planned. Wrapping them as
  `koala/toml` does is acceptable. It could only work for a trait without methods, because the
  interface tables of builtin types do not include a trait from another package.

Motivation: `Result.val` in `libs/koala/redis_refactor/result.kl` is `any` with a `NilValue` marker.
It can follow the `koala/toml` pattern today, with no new syntax.

---

## 56. Returning a value as `any` (2026-11-04, fixed)

Assigning anything to `any` or `any?` is a plain move, with no `make_intf` and no `cast`, and `any`
keeps the value's own representation. `T` -> `T?` needs no op either.

`build_return` (`irgen.c`) added a `cast` for a return to `any`, which failed to compile or lost the
type of a literal. It no longer does. Tests: `test/test-run/test_return_any.kl`,
`test/test-ir/test_ir_return_any.kl`.

---

## 57. Check that a class method matches its trait (2026-11-07, done)

`check_intf_methods` (`src/parser/parser_generic.c`) runs after `build_intf_table` and before IR
generation. It walks the interface table of each class, so the automatic `Printable`, `Hashable` and
`Equatable` are checked too. Tests: `test/test-kl/test_trait_impl_mismatch.kl` and
`test/test-ut/test_trait_impl_match.kl`.

The rules, with `T` bound to the actual type:

- A method the trait declares and the class implements is checked. A method the class does not
  implement passes.
- The number of parameters must be the same. Parameter names and default values are not compared.
- Parameters, the return type and generic type arguments are invariant: `Set[T]` -> `hashset[T]`,
  `T?` -> `T`, `any` -> `int` and `int` -> `int8` are not allowed.
- A class parameter that is a union is checked on its own: it is valid when it takes everything the
  trait's parameter declares. `bytes.__contains__(value uint8 | bytes)` matches `item uint8`, and
  `x int | str | bool` matches `x int | str`.
- A method with a type parameter of its own, such as `tuple.__getitem__[infer U]`, is not checked.
- A member of that name that is not a method, or a static method, is an error.
- A class implements a generic trait with one list of type arguments, directly or through another
  trait: `class C : G[int] & G[str]` is an error, and `L[int] & R[int]` with `L[T] : G[T]` and
  `R[T] : G[T]` is not. The interface table has one entry for `G`.

The set methods of `hashset`, `FrozenSet` and `SortedSet` return `Set[T]` as the trait declares. A
native method returns a plain object, not a trait value, so `union` is a Koala method: it is
`self | other` in `hashset` and `SortedSet`, and calls a private native `_union` in `FrozenSet`, which
has no operators. The same holds for `intersect` and `diff`.

---

## 58. `obj["key"]` on a plain class fails inside the compiler (2026-11-04, fixed)

`t["amy"]` with `__getitem__(name str)` on a plain class compiles and runs. Checked on 2026-11-06.

---

## 59. Nullable generic elements do not compile (2026-11-04, fixed)

`list[str?]`, `list[int?]` and a user class `Box[str?]` compile and run. Checked on 2026-11-06.

Left to check: the native paths that loop on `kl_call_next` and stop at nil, such as the iterator
branch of `_list_extend`, cut the elements at the first nil. `for` loops use `has_next` /
`next_strict` and do not have this problem.

---

## 60. A tuple type argument asserts in the compiler (2026-11-04, fixed)

`list[(str, int)]()` compiles and runs. Checked on 2026-11-06.

---

## 61. A field named `f` conflicts with the generated `fmt` (2026-11-04, fixed)

`class Box { var f any = 1 }` compiles and runs. Checked on 2026-11-06.

---

## 62. `koala/redis`: tests that wait for a mock annotation, TLS and other leftovers (2026-11-06, to do)

**Tests that wait for a mock annotation.** These paths need a socket that fails on demand.

- `Conn.receive`: a reset peer gives `ERR_IO`, an expired read timeout gives `ERR_TIMEOUT`, and both
  mark the Conn broken.
- Writing to a closed connection returns a short count and does not panic (`TcpConn.write`).
- `Pipeline`: after a wire-level failure nothing more is read, and the remaining commands share the
  failed Result.
- `SingleTcpPool`: a broken Conn is discarded, and the next acquire reconnects.
- `Conn.setup`: a rejected `Options.name` closes the Conn, and acquire returns nil.
- `TcpConn.read` returns -2 on a timeout. Whether the VM maps `SO_RCVTIMEO` to `IO_TIMEDOUT` or
  `IO_WOULDBLOCK` is not checked yet.

Tests against a real server check that it is up first, and return without panicking when it is not.

**TLS (fixed 2026-11-08).**

- ~~`SingleTlsPool` has no `_in_use` flag~~: it lends its Conn exclusively, as `SingleTcpPool` does
  (`test_tls_pool_lends_exclusively`).
- ~~`tls.dial` takes no options~~: `Options.connect_timeout_ms` bounds both the TCP connect
  (`so.DialOptions`) and the TLS handshake (`tls.TlsOptions`).
- `connect_tls` and `SingleTlsPool` were run by hand against a local TLS-only redis (a throwaway CA
  through `SSL_CERT_FILE`): ping / set / get work, by name and by IP. There is still no automated
  test, see §81.

**Other leftovers.**

- A failed write is not seen: `BufWriter` ignores a short write, so `Conn` only finds out from a later
  read or read timeout. The old client checked `io.write_all`.
- A non-UTF-8 bulk value panics in `decode` (`bytes.to_str`), so a binary value kills the program, and
  the API has no `bytes` form.
- An array reply with a server error keeps only the first error, because `Reply` has no error type.
  It matters for `EXEC`; decide with the transaction work.
- Not supported: `AUTH` / `SELECT` in `Options` (lost on a reconnect otherwise), transactions, Pub/Sub,
  RESP3, Unix sockets, connection URIs, `SCAN`.
- `redis` is not in `libs/koala/__ws__.toml`, so `wombat build koala` / `wombat test koala` skip it.
- `libs/koala/redis_old` can be deleted: nothing refers to it, and git has the old files.

---

## 63. Operators on a trait value: unary operators and edge cases (2026-11-06, to fix)

`intobj.c` and `floatobj.c` register the operators, so `+ - * / %`, the comparisons and
`& | ^ << >>` work on a trait value such as `Arithmetic[int]`.

**1. A unary operator does not compile.** `-a` and `~b` on a trait value report
`unary '-' operator requires int or float type.` (see §2, which needs a unary protocol
instruction). `_int_neg` / `_int_bitnot` / `_float_neg` have not run yet.

**2. `uint64` has no `__neg__`, by design, but `Arithmetic` requires one.** `uint64` implements
`Arithmetic`, and nothing reports the missing method (see §57). `intobj.c` registers `__neg__` only
because `int64` and `uint64` share a method table; `uint64` code never reaches it.

**3. To decide: division differs on the two paths.** The natives raise `division by zero` and wrap
`INT64_MIN / -1` to `INT64_MIN`. `OP_INT_DIV` / `OP_INT_MOD` check nothing, so the same operations in
statically typed code are a `SIGFPE`. Shifts no longer differ, see §64.

**4. Regression tests.** Only shifts on a trait value have one (`test-run/test_shift_trait.kl`); the
arithmetic, comparison and `& | ^` operators do not.

---

## 64. Shift semantics (2026-11-06, implemented; leftovers to do)

Decided: the Rust way. Integers are promoted to int64 / uint64 before a shift, so the width is 64.

| Count | Checked by | Result |
|---|---|---|
| literal or `const`, negative or 64 and up | parser (`check_shift_count`) | error |
| literal or `const` 0 | parser | warning |
| constant after propagation, negative or 64 and up | `const_copy_prop.c` | error |
| known at run time, negative or 64 and up | VM (`kl_check_shift_count`) | panic |

Only built-in integers are checked: a class may give `__shl__` its own meaning. Tests:
`test-kl/test_shift_count_const.kl`, `test-kl/test_shift_count_fold.kl`, `test-run/test_shift.kl`,
`test-run/test_shift_trait.kl`, `test-run/test_shift_traceback.kl`, `test-ut/test_shift_panic.kl`.

Left to do:

- The Language Manual does not describe these semantics yet.
- No test shows that a class's `__shl__` is left unchecked (`stream << 100`).
- `let n = 0; x << n` reports no warning: the optimizer checks only for errors, because a literal 0
  would then be reported twice.
- Without `--opt`, a bad count constant only after propagation is not an error. isel relies on the
  optimizer, so such a count would be encoded into an `_IMM` instruction, which has no check. `koala`
  always runs the optimizer; `koalac` alone does not reach isel without it.
- Constant `int8(1) << 40` (both sides constant, narrow left side) has no dedicated test.

---

## 65. Small compiler and runtime issues (2026-11-06, to fix or decide)

- An empty list literal does not take its type from the parameter: `encode("PING", [])` with
  `args list[str]` reports `expected 'std/builtin.list<u>', but got 'std/builtin.list<o>'`.
- The type of `nil` prints as `_?` in errors: `lhs: any =/= rhs: _?`.
- `koala -c <dir> -o <file>` ignores `-o` when the options follow the directory, and writes the klc
  next to the directory; `koala -c -o <file> <dir>` works.
- `io.Reader` says "Never return a negative value", but `TcpConn.read` returns -1 / -2 and
  `TlsConn.read` returns a negative value. `BufReader` and `io.copy` handle it; the contract text does
  not say so.
- `TcpConn.write` reports failure with a short count, and UDP / raw sockets still panic on an I/O
  failure.
- Outside a wombat project, a native library is found only through `LD_LIBRARY_PATH`, so a script that
  imports `koala/tls` prints `Failed to load libkoala_tls.so` (`source env.sh` first). Under wombat
  this is handled. Loading it from the klc's directory was tried and reverted: the path belongs to
  wombat.
- A narrowed variable in an `is` check has no test for indexing: `test-run/test_narrow_index.kl` covers
  only optional narrowing.

---

## 66. A narrow number type does not satisfy `T: Comparable` when `T` is inferred (2026-11-07, fixed)

```kl
print(max(float32(1.5), float32(2.5)))
// error: type argument 'float32' does not satisfy constraint on type parameter 'T'
```

`std/builtin.max` is `max[T : Comparable](x T, y T) T`, and the narrow number types only declared
`ToString`. It compiled while inferred type arguments had no bound check.

Fixed in two parts. In `libs/std/builtin/number.kl`, the narrow integers declare `Comparable[int64]` and
the narrow floats `Comparable[float64]`, with their comparison and arithmetic methods. In
`check_generic_bound`, a number type satisfies a generic bound through a declared base with a compatible
type argument (`number_base_satisfies_bound`). So `T` stays `float32`, and `max` returns a `float32`. Other
types stay invariant.

---

## 67. A generic bound specializes to a type with no symbol (2026-11-07, fixed)

Passing a type that does not declare a generic bound, such as `float32` to `T: Comparable`, logged
`compatible_klass: unresolved TypeSpec` with `sym_id=-1` for every base of the type, before the
expected error.

The old `type_spec_specialize(ts, tp_args)` only built a klass type, and the symbol comes from
`find_or_add_instance`. So `Comparable<f>` had no symbol unless a class had already declared it, which
is why `class Score : Comparable` was compared properly.

Fixed by creating the instance when it is first needed, as generic instantiation does everywhere else:
`check_generic_bound` now uses the function that was named `inst_type_spec`. It was renamed to
`type_spec_specialize` and the old function was removed.

---

## 68. A tuple as a type argument in a constructor call is read as a value (2026-11-07, fixed)

Correct code fails to compile:

```kl
func main() {
    let l = list[(str, int)]()
    l.append(("a", 1))
}
```

```text
error: Incompatible type: expected 'std/builtin.tuple<Lstd/builtin.type;Lstd/builtin.type;>',
but got 'std/builtin.tuple<uj>'.
```

The element type became a tuple of two `type` values: `(str, int)` was treated as a tuple expression
whose items are type names, not as a tuple type. The same type in a declaration worked:
`let l list[(str, int)] = [("a", 1)]`.

Fixed by `type_arg_of_expr` in `src/parser/parser_expr.c`, which turns a tuple in this position into a
tuple type, and reports a literal: `list[1]()` and `list[(1, 2)]()` compiled before, with the literal
read as the class of its value. The type arguments of a generic function go through
`parse_index_func_tp` and do not take a tuple. Tests: `test/test-run/test_tuple_type_arg.kl` and
`test/test-kl/test_tuple_type_arg_errors.kl`.

---

## 69. An external class without a public `__init__` (2026-11-07, fixed)

Constructing a class of another package that declares no public `__init__`:

```kl
// test_pkg/pkg
pub class Plain {}
pub class Generic[T] {}

// the importing file
pkg.Plain()         // error
pkg.Generic[int]()  // compiler crash
```

- The non-generic class gets the error "class 'Plain' is external, but has no public '__init__()'."
- The generic class crashes the compiler instead of reaching that error:

```text
koalac: src/parser/parser_expr.c:1243: get___init___instance: Assertion '_sym' failed
```

`get___init___instance` asserted that the origin class has `__init__` in its symbol table, which an
external class without a public `__init__` does not.

Fixed: the constructor call of an instance checks this first, and reports the same error as the
non-generic class. Test: `test/test-kl/test_external_class_no_init.kl`, with the package
`test/test_pkg/pkg_no_init`.

---

## 71. A repeated call with explicit type arguments has no `make_intf` (2026-11-07, to fix)

Correct code crashes at runtime on the second call:

```kl
trait Maker { func make() Sized }

class Thing : Sized { pub func len() int { return 7 } }

class Narrower : Maker { pub func make() Sized { return Thing() } }

func via_bound[T: Maker](m T) int { return m.make().len() }

func main() {
    print(via_bound[Narrower](Narrower()))
    print(via_bound[Narrower](Narrower()))
}
```

```text
vm_ops.h:430: _eval_frame: Assertion '((callable)->tag > (64 + 1))' failed.
```

The IR of the second call is `call @via_bound, %4 Narrower`, with no `make_intf` before it. An explicit call fails when any call of the function comes before it,
inferred or explicit. Only the first explicit call and inferred calls get `make_intf`. The cause has not
been traced.

---

## 72. `int.hash()` differs through a trait (2026-11-07, fixed)

```kl
func main() {
    print(5.hash())       // the same on both paths now
    let h Hashable = 5
    print(h.hash())
}
```

Integers had no `hash` method, so they used `_default_hash`, which hashes the whole `TValue` with its
tag. An interface value has its interface table there, so the two calls differed. A narrow value in a
wide container keeps its own tag, so `int8(1)` and `1` differed too: `hashset[int64]` kept them as two
elements and `int8(1) in d` was false, although `==` says they are equal.

Fixed by `_int_hash` in `src/objects/intobj.c`, which hashes only the value, through `_get_raw_bits`.
Test: `test/test-run/test_number.kl`.

---

## 73. Comparing constants of a narrow float type fails in the backend (2026-11-07, to fix)

Correct code fails to compile:

```kl
func main() {
    let a = float32(1.5)
    let b = float32(2.5)
    print(a < b)
}
```

```text
isel.c:317: Why goes here?
koala: compilation failed
```

`float32(1.5) > 1.0`, `a > 1.0` and the same code with `float16` fail the same way. `a == b` and
`a + b` compile, and `int8` has no such failure. It does not depend on the narrow types declaring
`Comparable` (§66): it fails the same way without that.

The line is the `c1 && c2` branch of `isel_lower_binary`, reached when both operands are constants. That
this is the cause comes from reading the code and has not been traced.

---

## 74. `bfloat16` cannot be constructed (2026-11-07, to fix)

```kl
func main() {
    print(bfloat16(1.5))
}
```

```text
irgen.c:912: Not Yet Implemented
koala: compilation failed
```

The line is the last branch of `emit_type_call`. `bfloat16(float64(1.5))` fails the same way. So
`bfloat16`, which declares `Comparable` since §66, has no test for its comparison methods or for `max`.

---

## 75. Unary operators still promote automatically (2026-11-07, to fix)

The binary promotion was removed: a narrow number type's methods in `libs/std/builtin/number.kl` declare
what it promotes to. `-a` and `~a` do not follow that yet. `parse_unary` hard-codes the result type as
`int64`, `uint64` or `float64`, and the narrow types declare neither `__neg__` nor `__bitnot__`.

```kl
func f(a uint8) uint64 { return ~a }

func main() {
    let r = f(uint8(5))
    print(r, typeof(r))    // 18446744073709551610 <class 'uint8'>
}
```

To do: declare `__neg__` and `__bitnot__` on the narrow types, and make the unary operators follow the
declaration as the binary operators do. `emit_ir_unary` does not cast its operand, so the tag above
stays `uint8`; that has to follow the declaration too.

---

## 76. A union member can contain a type parameter (2026-11-07, to fix)

A union is concrete, so `T | bool` is reported as `Union subtype must be a class`. The same parameter
inside a member is not reported, because `check_union_subtypes` in `src/parser/parser.c` only looks
at the member itself: `list[T]` is an instance of a class.

```kl
trait Shapes[T] { func uni(x list[T] | bool) int }    // no error here

class S : Shapes[str] {
    pub func uni(x list[str] | bool) int { return 2 }
}

func main() {
    let s Shapes[str] = S()
    s.uni(["x"])    // error: expected 'bool | list[Shapes:T]', but got 'list<u>'
}
```

To do: report the declaration, by looking for a type parameter inside each member. Then remove `uni`
from `Shapes` in `test/test-ut/test_trait_impl_match.kl`, and add the error to a test in
`test/test-kl`.

## 77. `T` is not inferred through a `Box[T?]` parameter (2026-11-07, to fix)

A type parameter inside an optional type argument cannot be inferred from the call argument. Creating
the object and returning `Box[T?]` both work, see `test/test-ut/test_optional_type_arg_new.kl`.

```kl
class Box[T] {
    var v T
    func __init__(value T) { v = value }
    func get() T { return v }
}

func same[T](b Box[T?]) Box[T?] { return b }

func main() {
    let a Box[int?] = Box[int?](1)
    let c = same(a)    // error: cannot infer type parameter
    print(c.get()!)
}
```

To do: match `T?` against `int?` when inferring from a generic instance, then add this call to the
test above.

## 78. `enum` as sugar for a final class (2026-11-07, planned)

An enum is a closed set of named constants without data. Data on the cases belongs to `sealed trait`
and classes (§55). The two only share the check that the set of cases is closed, used by `switch`.

```kl
pub enum Color { RED, GREEN, BLUE = 10, ALPHA }

let c = Color.BLUE
print(c)                  // BLUE
print(c.value)            // 10
print(c == Color.BLUE)    // true

let e Color = 10          // error: no implicit conversion
let i int = c             // error: write c.value
let b = c > Color.RED     // error: write c.value > Color.RED.value
Color(0, "RED")           // error: cannot be constructed
Color.RED = Color.BLUE    // error: a member cannot be assigned
```

The compiler generates the class, in pseudo code:

```kl
pub class Color : Equatable[Color] {
    pub let value int64
    let name str

    func __init__(v int64, n str) {
        value = v
        name = n
    }

    pub func fmt(f Formatter) { f.write_str(name) }
}

// hidden module-level singletons, created when the module is initialized
let Color.RED = Color(0, "RED")
```

Decided:

- Branch `new_enum` (`47539856`) makes enum a sealed `int64` with `TYPE_ENUM` and `SYM_ENUM`. It is
  34 commits behind and is not merged. Only its grammar and member checks are ported by hand. A class
  gets `klc` export, generics, containers and the auto traits from the existing code.
- No implicit conversion to or from `int`, in line with the removal of implicit number promotion.
- No ordering in the first version. Python's `Enum` has none, and order by value and order by
  declaration differ for `enum E { A = 10, B = 1 }`. It can be added later without breaking code.

### P0: the first version

- ☐ 0. Check first: how a module is initialized, the order across packages, whether `static` exists.
- ☐ 1. Grammar: the `enum` keyword, top level only, optional `pub`. Errors as required by
  `test/test-kl/test_enum_syntax_errors.kl`.
- ☐ 2. Generate the final class. Forbid constructing it and giving it bases.
- ☐ 3. Member checks, from `new_enum`: duplicate name, duplicate value, auto increment, overflow.
- ☐ 4. Singletons: resolve `Color.RED` in `parse_dot`, create them at module initialization, reject
  assigning to a member. This is the largest risk, item 0 is there to find problems early.
- ☐ 5. Methods: `fmt` prints the member name. `__eq__`, `__ne__` and `hash` keep the defaults.
- ☐ 6. `c.value`, and errors for every implicit conversion.
- ☐ 7. `list[Color]`, `dict[Color, int]`, `Color?`, conversion to `any`, a generic `T`.
- ☐ 8. Tests: rewrite the tests of `new_enum`, remove `SKIP` from `test_enum_syntax*.kl`, cover items
  1 to 7 for both correct code and errors.
- ☐ 9. Docs: design overview, language manual, §55.

### P1: right after

- ☐ 10. Across packages: `klc` carries the member table, `pkg.Color.RED` works, tests in `test_pkg`.
  Until then a `pub enum` cannot be used from another package.
- ☐ 11. `Color.from(v)` returns `Color?`, `nil` for an invalid value. Its form depends on item 0.
- ☐ 12. `c.name()` returns the member name.
- ☐ 13. `switch`: the grammar exists, `STMT_MATCH_KIND` has no semantics or IR yet. Support enum first.
  It must handle every case or have a `_` branch.

### P2: to decide later

- ☐ 14. Listing the members, e.g. `Color.values()`, and its return type.
- ☐ 15. Methods in an enum: the syntax, and whether it can implement a trait.
- ☐ 16. Ordering: an explicit `Comparable` by the user, or an automatic way.
- ☐ 17. Declaring an enum in a function or class: the lifetime and naming of its singletons.
- ☐ 18. `sealed trait` (§55), sharing the closed-set check with `switch`.
- ☐ 19. `switch` as an expression.
- ☐ 20. Bit flags: whether a `Flags` kind of support is needed.

Note for item 7: a generic function with `T` bound to `any` aborts the compiler at `typespec.c:1592`,
with or without an enum, e.g. `gen([1, "a"])` for `func gen[T](x list[T])`. It is not fixed yet.

---

## 79. Native handles of `koala/http`, `koala/tls` and `koala/sqlite3` have no finalizer (2026-11-08, waits for a free path)

- `koala/http`: `_Parser` (`parser.kl`) wraps an `HttpParser` that `http_parser_new` allocates with
  `calloc`. Only `close()` frees it. `Response.parse`, `ResponseHead.parse` and the client close it
  on every path; a `_Parser` dropped without `close()` leaks.
- `koala/tls`: `TlsConn` owns an `SSL *`. Only `close()` frees it (`SSL_shutdown` + `SSL_free`) and
  closes the TCP fd; a `TlsConn` dropped without `close()` leaks both.
- `koala/sqlite3`: `Connection` owns a `sqlite3 *` and its cached statements, and a `Cursor` the
  statement it is running. Only `Connection.close()` frees them: it closes its cursors, finalizes
  the cache and calls `sqlite3_close_v2`. A connection dropped without `close()` leaks the
  database handle, its statements and, for a file, keeps the file open; an open transaction stays
  uncommitted until the process exits. Python closes on garbage collection.

The same kind of leak as §28: give both a `_free` once the run time has a free path.

---

## 80. `koala/http` leftovers (2026-11-08, to do)

- HEAD: the native `http_parser_skip_body` exists, but the client never calls it, and `Request` has
  only `get` / `post`. A hand-built `Request("HEAD", ...)` whose response has a `Content-Length`
  waits for EOF, then returns nil.
- 1xx responses, to check: llhttp completes a `100 Continue` as a message of its own, and the
  client stops at the first complete message, so it may return the 100 as the response. Not
  verified.
- Keep-alive: the client forces `Connection: close`. The parser already has `keep_alive` and
  `reset`. Keep a connection pool inside `koala/http` until a second real pool exists, then decide
  on a shared `koala/pool` (discussed 2026-11-08: the shared core is thin, when a connection can be
  reused is decided by each protocol, and without a concurrency API a pool gives little more than one
  reused connection per host).
- Parse errors are not reported: `http_parser_error` is unused, and `Response.parse` returns nil
  without a reason.
- Request parsing, for a server: the parser accepts requests, nothing in Koala uses it yet.

---

## 81. `koala/tls` leftovers (2026-11-08, to do)

Fixed on 2026-11-08: the certificate is checked against the host name (`SSL_set1_host`) or, for an
IP literal, against its IP entries with no SNI sent; a close without `close_notify` is EOF
(`SSL_OP_IGNORE_UNEXPECTED_EOF`); `close()` twice is safe; a failed handshake closes the TCP
connection. `dial(url, options?, net_options?)` and `TlsOptions` (handshake timeout,
`server_name`, `skip_verify`) were added.

**No automated test.**

- The IP literal path: checked by hand against a local TLS redis whose certificate has an IP
  entry. From this machine `1.1.1.1` and `8.8.8.8` port 443 cannot be reached, so a test against a
  public server hangs.
- Unexpected EOF: no public server reliably closes without `close_notify`, so
  `test_tls_read_to_end` does not reproduce the old failure.
- `redis.connect_tls` / `SingleTlsPool` against a server (by hand only, §62).
- The network tests use example.com and badssl.com, and return early when offline.

**Not supported.**

- `ca_file` (custom root CAs): there is one process-wide `SSL_CTX`, a `SSL_CTX` per configuration
  is needed.
- A minimum protocol version, ALPN, client certificates.
- A server side (`TlsListener`, server handshake).
- The connection state: negotiated version, cipher, peer certificate.
- Failure reasons: `dial` and `wrap` return nil for DNS, TCP, handshake and verification failures
  alike.

**Behavior to know.** With a handshake timeout, `wrap` clears the read and write timeouts of the
TCP connection afterwards. This also clears the `SO_SNDTIMEO` that `so.dial` leaves set for its
connect timeout.

---

## 82. `as!`: a forced cast for a value known to have the type (2026-11-08, to discuss)

Taking a value out of an `any` / trait / union when the code knows its type is common: the values
of a `koala/cli` Namespace, a json `Value`, a container of `any`. Today it is written:

```kl
let out = (args.get("output")! as str)!     // get() returns any?
```

The first `!` says "not nil", the `as` gives `str?`, the second `!` says "it is a str". It is
correct and follows §29 (an optional is unwrapped before `as`), but has two costs:

- The parentheses, at every such read.
- A failed cast panics as an unwrap of nil: the message says neither the expected nor the actual
  type.

**Proposal.** `x as! T`, the forced form of `as`: the same as `(x as T)!`, with the result type `T`
instead of `T?`. A failed cast panics with both types, e.g. `cast failed: expected 'str', got
'int64'`.

```kl
let out = args.get("output")! as! str
```

- The source still cannot be optional: `as!` does not unwrap nil, the `!` before it does. Checking
  for nil and checking the type stay separate, as §29 decides.
- `!` already means "I am sure, panic otherwise"; `as!` applies it to a cast. Swift has the same
  `as!` / `as?` pair.
- No hidden mechanism: it is `(x as T)!` written directly. The cost is one more operator, against
  design principle 3.
- Rejected: an `as!` that takes an optional source and checks for nil and the type at once. It
  would undo §29.

To decide. If accepted: the checker gives `as!` the type `T`, irgen emits `as` plus a nil check, or
a panic that carries the two types; tests for success, a wrong type, and an optional source (an
error).

## 83. A `const` cannot hold `bytes`, and reads it back wrong at run time (2026-11-08, to fix)

Found while writing the tests for `koala/zipfile`.

A `const` whose value is a `bytes` literal compiles, then aborts the process the first time it is
read:

```kl
const DIGITS = b"0123456789"

func main() {
    print(DIGITS.len())
}
```

```
koala: include/runtime/vm_ops.h:2435: _eval_frame:
       Assertion `obj && ((((obj))->_type) == &global_type)' failed.
Aborted (core dumped)
```

A `str` constant of the same shape is fine (`pub const RFC_4122 = "specified in RFC 4122"` in
`koala/uuid` has worked since that package landed), and a module-level `let` holding the same
`bytes` works as well, so the fault is in how a `bytes` constant is materialised into the module
globals and then loaded from them: the loader expects a `global_type` object where it finds
something else.

Two smaller findings from the same session, both worth a decision rather than a fix:

- `const` values must be literals, so a long literal cannot be split across lines with `+`:
  `const PY_ZIP = "504b..." + "0304..."` is rejected with `const value must be a literal`. The
  workaround used in `koala/zipfile`'s test is one long line, which is what the file carries.
- An untyped parameter whose default is `nil` crashes the compiler instead of being rejected: the
  manual says a parameter type may be omitted when a default pins it down, but `nil` pins nothing
  and the checker dereferences a null `src`:

  ```kl
  func t(x = nil) { if x == nil { print("none") } }

  func main() { t(); t(b"abc") }
  ```

  ```
  koalac: src/parser/parser.c:558: _optional_compatible_check:
          Assertion `dst->opt.src != ((void *)0)' failed.
  ```

  So a `bytes? = nil` parameter has to spell its type out (`zdict bytes? = nil` in `koala/zlib`),
  which is why those annotations survive the type-omission cleanup.
- `as` on a union is not uniform: `x as bytes` after `if x is bytes` narrows to a non-optional
  `bytes`, so `(x as bytes)!` is rejected in one place ("types of two sides are not matched") while
  `(y as str)!` is required in another (`str?` from `str | bytes`), and `source as str` inside
  `if source is str` is rejected with "src of `as` operator is not a valid type". Each call site in
  `koala/zipfile` now carries the form the checker accepts, with no rule behind the choice. This is
  the case §82's `as!` would tidy up.

Until the `const` crash is fixed, the rule for library code is: **hold module-level bytes in a
`let`, or return them from a function**; never in a `const`.

---

## 84. Two `std` gaps that `koala/tarfile` had to work around (2026-11-08, to do)

`koala/tarfile` is a 1:1 port of CPython 3.14's `tarfile`; two of its features have no Koala
counterpart, and both would be small additions to `std`.

1. **`std/fs` has no `mkfifo()` / `mknod()`.** `TarFile.makefifo()` and `TarFile.makedev()` are
   therefore stubs that panic ("Koala has no mkfifo"), so a FIFO or a device member can only be
   *listed* and *extracted* never, even though `dir()`/`chmod()`/`chown()`/`set_times()` are all
   there. One `mkfifo(2)` wrapper in `libs/std/fs/` closes the FIFO half; `mknod(2)` on a
   non-Linux needs `<sys/mknod.h>` juggling, so it can be Linux-only.
2. **`std/time` has no calendar functions.** It offers `Duration`/`Instant` only - no
   `localtime()`, no `gmtime()`, no `strftime()`. `TarFile.print_list()` therefore prints its time
   column in **UTC**, where Python prints local time, and the formatting is done by hand
   (`utc_datetime()`: a civil-from-days conversion over an epoch second count). Any program that
   wants `ls -l` timestamps needs the same hand-written code, so a `time.gmtime()` returning a
   broken-down time would be worth having on its own.

Both were found while porting; the workarounds are recorded in `docs/koala_tarfile_api.md`
("Differences from Python").

---

## 85. `koala/sqlite3` leftovers (2026-11-08, to do)

`koala/sqlite3` follows CPython's `sqlite3` over the vendored SQLite 3.54.1. Where it differs on
purpose, the package docs say so: errors are a nil (or an error code) plus
`sqlite_errorcode()` / `sqlite_errorname()` / `errmsg()` instead of exceptions; programming errors
panic; a row is always a `Row`; `named=` carries the dict of named parameters; `in_transaction`,
`total_changes` and `isolation_level` are methods. Not done yet:

- **Iterating a cursor**: Python's `for row in cur.execute(...)` has no counterpart; `Cursor` is
  not an `Iterable[Row]`. `while let row = cur.fetchone()` works.
- **Module attributes**: `apilevel`, `paramstyle`, `threadsafety`, `complete_statement()`.
- **Type adaptation**: `detect_types`, `PARSE_DECLTYPES` / `PARSE_COLNAMES`, `register_adapter`,
  `register_converter`, `text_factory`, and a `row_factory` other than `Row`.
- **Callbacks into Koala**: `create_function`, `create_aggregate`, `create_window_function`,
  `create_collation`, `set_authorizer`, `set_progress_handler`, `set_trace_callback`. Native code
  cannot call back into Koala yet (see `koala/http`'s parser, §80).
- **Other `Connection` methods**: `interrupt`, `backup`, `iterdump`, `blobopen`, `getlimit` /
  `setlimit`, `serialize` / `deserialize`, and the `autocommit` attribute of Python 3.12.
- **`Row` equality and hash**: Python compares two rows by their columns and values.
- **A context manager**: Python's `with conn:` commits or rolls back. Waits for `with`.
- **Blocking**: every call blocks the thread; a long query would stall a scheduler once Koala has a
  concurrency API.
- **`fetchmany` / `fetchall` on an error** return nil, so the rows read before the error are lost.
  Python raises and loses them too; to decide whether to keep them.

---

## 86. A function with a return type can end without `return` (2026-11-08, fixed)

Nothing checked that every path of a function with a return type ends in `return`: the body
compiled and the missing value was nil, so an `int` function could return nil.

```kl
func noisy() int {
    print("x")
}           // error: missing return in function 'noisy', which returns 'int64'.
```

Since only `= expr` returns its value (decided 2026-11-08), a block of one expression,
`func triple(a int) int { a * 3 }`, is such a body too; before, it returned `a * 3`.

Fixed 2026-11-08 in `emit_ir_func_decl()`: when the function's last block has no terminator, the
error is reported if that block can be reached from the entry. `block_is_reachable()` walks the
CFG edges, follows only the taken edge of a branch on a constant (so `while true { }` and
`loop { }` do not reach their end unless they `break`), and stops at a block that calls
std/builtin's `panic()` / `unreachable()`. Both branches of an `if` returning, `if let`,
`let-else` and a `for` that may run zero times are all handled by the walk itself; `parse_block`'s
`has_terminal` was not enough. `koalac` now stops after irgen when it reported errors (before, it
went on to codegen). No library, demo or tool code had a missing return. Cases:
`test/test-kl/test_missing_return_errors.kl`.

---

## 87. `koala/readline` leftovers (2026-11-08, to do)

`koala/readline` follows CPython's `readline` module, and the editor follows GNU readline 8.3's
emacs keymap; `docs/koala_readline_api.md` lists where it differs on purpose (`input` returns nil
or `""` instead of raising, the callbacks are trait objects, the file functions return false).

Fixed 2026-11-08: Ctrl-Z and Ctrl-\ did nothing, because raw mode clears `ISIG` and the editor
dropped the bytes. The editor now handles the terminal's own suspend and quit keys (read from its
termios, so `stty susp` and `stty -isig` are followed): out of raw mode it sends SIGTSTP or
SIGQUIT to the process group through `Terminal.suspend()` / `quit()`, then goes back to raw mode
and edits on, drawing nothing, as GNU readline does. Ctrl-C stays a key, so that `input` can
return `""`. Cases: `test_editor.kl` on a `MemoryTerminal`.

Not done yet:

- **The suspend and quit keys outside the line**: they are caught only while the line is edited.
  During an incremental search, a quoted-insert, the completion `y or n` query and the `--More--`
  pager, Ctrl-Z and Ctrl-\ are read as other keys are; with GNU readline the terminal driver
  sends the signal there too.
- **vi mode**: `editing-mode vi` and the vi keymap; the line is accepted and ignored.
- **Numeric arguments and the region**: `digit-argument` (M-0 ... M-9, M--), `universal-argument`,
  `set-mark`, `exchange-point-and-mark`, `kill-region`, `copy-region-as-kill`.
- **Other GNU commands**: `menu-complete`, `history-search-backward` / `-forward`,
  `non-incremental-*-search-history`, `character-search`, `overwrite-mode`, keyboard macros
  (C-x ( / C-x ) / C-x e), `dump-functions` / `dump-variables`, `tilde-expand`.
- **Long lines**: a line wider than the screen scrolls sideways on one row, as linenoise does; GNU
  readline wraps it over several rows.
- **`bell-style visible`** is silent; GNU readline flashes the screen.
- **`readline.backend`** (Python 3.13) is not provided.
- **Blocking**: reading a key blocks the thread; it would stall a scheduler once Koala has a
  concurrency API.
- **Platforms**: the shim uses POSIX termios; there is no Windows console support.

---

## 88. enum 后续 review 场景（2026-11-10，待检查）

以下是待补充 review / 回归验证的场景，不是已确认缺陷。

- [ ] **跨包同名 enum**：两个包分别定义同名公开 enum，确认赋值、比较和泛型实例
  保持类型隔离；泛型函数名 mangling 不因 enum 同名而冲突。
- [ ] **enum 作为 dict key**：检查 hash、相等性、插入、覆盖、查询和删除的一致性，
  并确认语义阶段保留 enum 身份，IR 阶段按 int64 表示。
- [ ] **KLC int64 边界值往返**：公开 enum 成员包含 int64 最小值、最大值和负数，
  写入 KLC 后由另一包读取，确认成员值完整保留。
- [ ] **类型与值的错误路径**：enum 类型名允许作为泛型类型实参，但裸 enum 不能作为值，
  enum 成员（如 `A.X`）也不能作为类型实参；覆盖本地与包限定写法。

`match` 和 display / `fmt` 属于暂缓讨论的功能，不纳入当前 enum 支持的完成范围。

## 89. 泛型规范审查发现的不一致（2026-11-10，待修）

规范见 `Koala_Design_Overview.md` §4（泛型实参不变、只转换值）与手册 Generics 章节。以下是审查中
实测到的、与规范不一致或有缺陷的行为。

### 不一致与缺陷（暂缓处理）

以下 5 项保留为待办。本轮针对第 1、2 项的修复尝试及新增测试已回退，尚未修复；
后续需先确认处理方案，再补回归测试。

| # | 问题 | 严重程度 |
|---|---|---|
| 1 | `func names[T : N](xs list[T])` 以 `names([L()])` 调用时 VM 崩溃；`dict[str, T]` 参数、构造函数的 `list[T]` 参数同样受影响。 | VM 崩溃 |
| 2 | 类型参数推断或显式指定为 `any` 时编译器中止，报内部错误 "Why goes here?"，如 `first([1, 2.5])`、`pick[any](1, "a")`；对应 §40.4。 | 编译器中止 |
| 3 | `T?` 参数不能从 `opt(1)` 推断出 `T = int`；报错文案把函数称为 "class"。 | 缺陷 |
| 4 | 字面量取公共类型（`[1, nil]` 为 `list[int?]`），泛型函数的实参推断未做同样的合并（`pick(1, nil)` 报错）。 | 待讨论 |
| 5 | 求公共类型时不找共同的 trait：`[Dog(), Cat()]` 推断为 `list[any]` 而不是 `list[Animal]`。 | 待讨论 |

### 复现细节与后续方向

- [ ] **VM 崩溃：有约束的 `T` 嵌在容器参数里，推断为类**（擦除路径，与 §8、§34、§40.1 同源）：
  `func names[T : N](xs list[T])` 以 `names([L()])` 调用，或 `dict[str, T]` 参数、
  `class Holder[T : N]` 的 `__init__(_xs list[T])`，运行时 `_eval_frame` 断言失败。泛型体按约束的
  intf-table 调用 `x.name()`，但容器里是原始的类值；`try_to_build_tp_args_cast()` 只转换类型恰为
  `T` 的参数。显式写 `names[N](...)` / `Holder[N](...)` 正常。方向：擦除体内对 `T` 值的约束方法调用
  按值自身的类型查表（规范所说的“由值的类型在运行时分发”），或推断出类时拒绝该调用。
- [ ] **编译器中止：泛型函数的类型实参推断为 `any`**（§40.4 的复现）：`first([1, 2.5])`、
  `lst([1, "a"])`、`pick[any](1, "a")` 在 `mangle_func_name()` → `type_spec_str_func_name()`
  （`typespec.c`）走到 `UNREACHABLE()`（"Why goes here?"）。类型实参无法命名时应跳过特化路由，
  调用擦除的泛型函数。
- [ ] **`T?` 参数不能从非空实参推断 `T`**：`func opt[T](x T?)` 的 `opt(1)` 报
  "cannot infer type parameter 'T' for class 'opt'"（`opt[int](1)` 正常）。应推断 `T = int`；
  报错文案把函数称为 class。
- [ ] **推断规则两套**：字面量取 lub（`[1, nil]` 是 `list[int?]`），泛型函数实参要求完全相同
  （`pick(1, nil)` 报错，需 `pick[int?](1, nil)`）。当前按规范记录为现状，是否统一待讨论。
- [ ] **lub 不找公共 trait**：`[Dog(), Cat()]` 是 `list[any]`；`let n Animal = Dog()` 后 `[n, Cat()]`
  也是 `list[any]`（`Cat : Animal`）。因此 `names([Dog(), Cat()])` 报约束不满足。是否让 lub 取
  公共 trait 待讨论；目前需给字面量写类型。
- [ ] **多约束不支持**：`T : A & B` 报 "can only have one bound"。规范已按现状记录；是否支持待讨论。
- [ ] **`@specialized` 在类上的映射 / 生成形态未实现**：`Koala_Design_Overview.md` §7.4 前半部分按
  设计描述了 `@specialized(list[int64]) class int64list` 与 `@specialized(Foo[int]) class Foo[T]`，
  实现中类上的注解被警告并忽略（`test-kl/test_specialized_ignored_warnings.kl`）。§7.4 末尾已注明，
  阅读时以手册为准。

## 90. class 转 trait（不含泛型）测试发现的问题（2026-11-10，待修 / 待确认）

覆盖局部变量、参数、返回值、字段、全局变量等 65 种写法，通过的场景已固化为
`test-run/test_class_to_trait.kl`（刻意避开下列崩溃写法）。以下问题暂未处理。

### 崩溃（待修）

- [ ] **编译器中止：`let` 声明的可选 trait 变量再用 `!` 取值**：
  ```kl
  let a Animal? = Dog()
  print(a!.name())        // 或 let b = a!
  ```
  `isel.c:934` `isel_lower_cast` 断言 `ts_symbol_is_trait(src_ts) && ts_symbol_is_klass(dst_ts)`
  失败。原因：const-copy-prop 把 `cast %a Animal? to Animal` 中的 `%a` 换成了初值
  `%1 = make_intf ... to Animal`，得到 `cast %1 Animal to Animal`（源和目标是同一个 trait），
  isel 只接受 trait → class 的 cast。`let a Animal? = d`（`d` 是 `Dog`）、`let a Animal? = p`
  （`p` 是 `Pet`）同样中止。`var` 声明、`if let` 取值正常。方向：isel 把同类型的 cast 当 move，
  或 const-copy-prop 不产生同类型 cast。
- [ ] **VM 崩溃：可变参数 `...Trait` 接收类实例**：
  ```kl
  func names(xs ...Animal) str {
      var s = ""
      for x in xs { s += x.name() }
      return s
  }
  names(Dog(), Cat())     // _eval_frame 断言 callable->tag 失败
  ```
  可变参数被打包为 `build_intern @tuple, %0 Dog`，类型 `tuple<Dog>`，元素没有 `make_intf` 成
  `Animal`；体内按 intf 调用 `x.name()` 时崩溃。只调用 `xs.len()` 不崩；实参已是 `Animal` 值时
  正常；混合（`names(a, Cat())`）也崩溃。方向：`handle_valist_and_dfl_args()` 构造的 tuple
  按可变参数的元素类型转换每个元素（与 list/dict 字面量的 `literal_item()` 相同）。

### 不一致 / 待确认

- [ ] **tuple 字面量与 tuple 解构不一致**：`let t (Animal, int) = (Dog(), 1)` 编译报错
  （§89 已记），但 `let (a Animal, n int) = (Dog(), 1)` 与 `(a, n) = (Cat(), 2)` 正常，
  返回类型 `(Animal, int)` 的函数 `return (Dog(), 1)` 也报错。是否让 tuple 字面量按期望类型
  逐元素转换，待讨论。
- [ ] **trait 值不能用 `==`**：两个 `Animal` 值 `a == b` 报 "operator '==' is not defined in type
  'Animal'"。设计上每个 class 自动遵循 `Equatable`（identity 默认），但 trait 类型本身不带
  `__eq__`。是否让所有 trait 值支持 `==` / `!=`（按值自身类型的 `__eq__`），待确认。
- [ ] **if 表达式不能直接作为调用实参**：`show(if b { Dog() } else { Cat() })` 语法错误
  （与 trait 无关）；`let x = if ...` 和 `func f() T = if ...` 可用。是否放开，待确认。
- [ ] **`?.` 未实现**：`a?.name()` 报 "not supported yet, use 'if let' or '!.'"（已知）。

### 泛型场景（2026-11-10 补测）

覆盖 80 种带泛型的写法（泛型函数 / 方法，有无约束；泛型类、泛型 trait 以 trait 实例化；局部变量、
参数、返回值、字段、全局变量、容器、可选、嵌套），通过的已固化为
`test-run/test_generic_class_to_trait.kl`。下列问题暂未处理（§89 已记的不重复）。

| # | 写法 | 结果 |
|---|---|---|
| 1 | 可变参数泛型 `func va[T](xs ...T)` 以 `va(1, 2)` 调用 | 编译器中止（类型推断未实现，与 trait 无关；`va[int](1, 2)` 正常） |
| 2 | 带约束的可变参数 `va_bound[Animal](Dog(), Cat())`，函数内调用元素的方法 | VM 崩溃（与不带泛型的 `...Animal` 同源） |
| 3 | `let a Animal? = ident(Dog())` 后接 `a!` | 编译器中止（与不带泛型的 `let` 加 `!` 同源） |
| 4 | `func rl() list[Animal] = [Dog()]`，dict 字面量同理 | 编译报错；`{ return [Dog()] }` 正常，两种函数体不一致 |
| 5 | `enumerate([1, 2])` | 报 "'enumerate' is not found"，手册说 `enumerate` / `zip` 可用 |

详细说明：

- [ ] **编译器中止：泛型可变参数 `...T` 的类型推断未实现**：`func va[T](xs ...T) int` 以
  `va(1, 2)` 调用即在 `parser_generic.c:126` `infer_tp_from_call()` 的 `TYPE_VA_LIST` 分支
  `NYI()` 中止（与 trait 无关，普通 int 也中止）。`va[int](1, 2)` 显式写类型实参正常。
- [ ] **VM 崩溃：有约束的泛型可变参数接收类实例**：`func va_bound[T : Animal](xs ...T)` 以
  `va_bound[Animal](Dog(), Cat())` 调用、体内 `x.name()` 时崩溃。与上文“可变参数 `...Trait`
  接收类实例”同源：打包 tuple 时元素未 `make_intf`。
- [ ] **`let x Trait? = <泛型调用>` 后 `x!` 同样触发 `isel.c:934` 编译器中止**：
  `let a Animal? = ident(Dog())` 再 `a!.name()`，与上文 `let a Animal? = Dog()` 同源。
- [ ] **`= expr` 函数体不把返回类型传给 list / dict 字面量**：
  ```kl
  func rl() list[Animal] = [Dog()]          // 报错：list<Dog> 不匹配 list<Animal>
  func rd() dict[str, Animal] = {"a": Dog()} // 同样报错
  func rb() list[Animal] { return [Dog()] }  // 正常
  ```
  两种函数体对字面量的期望类型处理不一致；`= expr` 体应与 `return` 一样调用
  `parse_literal_as_expected()`。
- [ ] **`enumerate` / `zip` 不可用，与手册不符**：手册 Branching and Loops 写
  “Top-level iteration combinators `enumerate` and `zip` are available”，实际 `enumerate([1, 2])`
  报 "'enumerate' is not found"（`libs/std/builtin/enumerate.kl`、`zip.kl` 中的定义是注释）。
  需实现，或修改手册。

## 91. 变长参数的打包 / 解包：实现现状与缺陷（2026-11-12，待修 / 待设计）

起因是修「变长参数静态下标 ≥ 1 报越界」时顺带挖出的转发与 `!` 解包问题。本节先把当前实现
的模型写清楚，再列缺陷；**设计未定案，未施工**。

### 91.1 当前模型：变长槽 = 一个位置实参，持有一个 tuple 对象

- 打包在 `handle_valist_and_dfl_args()`（`src/parser/parser_expr.c:1967`）：把第
  `valist_index` 个及之后的位置实参收进 `vec`，再 `expr_from_tuple(vec)` 造成**一个** tuple
  表达式塞进 `real_args`（`:2035`）。零变长实参时也补一个空 tuple（`:2047`），注释
  （`:2042-2045`）说明理由：变长形参恒占一个位置槽，实参个数必须与 prototype 对齐。
- 因此被调方看到的 `xs` 就是一个普通 tuple 值，`xs.len()` / `for x in xs` / `xs[i]` 全部走
  tuple 的方法，运行期没有独立的 va_list 概念。
- 实测（根目录 `test22.kl`）：

  ```kl
  func pass(xs ...int) int {
      print(xs)                  // → 1 2 3
      print(xs, typeof(xs))      // → (1, 2, 3) <class 'tuple'>
      return 0
  }
  ```

  同一个 `xs`，独占变长槽时被摊开，和别人并列时是一个 tuple 元素。差异来自下面的直通路径。

### 91.2 已存在的「直通」路径 `skip_tuple`

`parser_expr.c:2011-2025`：当变长槽**只收到 1 个**实参、且该实参的 `ts` 本身是
`TYPE_VA_LIST` 时，不再包 tuple，直接透传（`skip_tuple = 1`）。这就是 `print(xs)` 输出
`1 2 3` 的原因。两个限制：

- 只在独占整个变长槽时生效；`print(xs, typeof(xs))` 有 2 个实参 → 走打包 → `xs` 成为新 tuple
  的一个元素。
- 完全不检查元素类型，只看「是不是 va_list」。

**关键结论：打包层已经支持「把一个变长值原样交给另一个变长形参」，类型检查层不认这件事。**
下面 A / B 两个缺陷都是这两层不一致的表现。

### 91.3 缺陷 A：转发给带类型的变长形参被类型检查拒绝（待修）

- [ ] ```kl
  func second(xs ...int) int { return xs[1] }
  func pass(xs ...int) int { return second(xs) }   // 报错
  ```
  `Incompatible type: expected 'int64', but got '...int64'.`

  位置：`check_valist_arg()`（`parser_expr.c:1073`）在 `:1098` 直接拿**元素类型**与实参类型
  比兼容性；它只有 `!` 解包分支（`:1093`），没有「实参本身是一个 va_list 值」的分支。

  实测矩阵（失败只取决于**被调方**的变长形参有没有类型）：

  | 调用方 | 被调方 | 结果 |
  |---|---|---|
  | `...int` | `...int` | 报错 `expected 'int64', but got '...int64'` |
  | `...`（无类型） | `...int` | 报错 `expected 'int64', but got '...any'` |
  | `...int` | `...`（无类型） | 通过 |
  | `...` | `...` | 通过（`test-run/test_valist_zero_args.kl` 即此形态） |

  后两行通过**不是**因为有特判，而是被调方元素类型为 `any`，
  `type_spec_compatible(any, ...int64)` 恒真，把整个 tuple 吞掉了。

  方向：让 `check_valist_arg` 承认「实参是 va_list / tuple 值且独占变长槽」，与 `skip_tuple`
  对齐，同时校验元素类型兼容（`...int` → `...str` 应报错）。

### 91.4 缺陷 B：`!` 对 Sequence 只做了类型层，codegen 是空操作（待修 / 待设计）

- [ ] 类型层：`parse_bang()`（`parser_expr.c:4068`）在 `:4098-4103` 用
  `match_sequence(e->ts, &it_ts, NULL)` 把静态类型改成 Sequence 实例类型（`it_ts` 出参语义见
  `typespec.c:845`）；`check_valist_arg:1093` 再用 `arg_ts` 出参取**元素类型**，于是检查通过。
- [ ] 代码生成层：`emit_ir_bang()`（`src/parser/irgen.c:2278-2300`）在非 optional 时直接
  `exp->ir_val = e->ir_val` 返回（`:2290`，注释写的是 shadow variable 的场景），**没有任何摊开
  动作**。KLR 指令集、`isel.c`、VM 里也都没有用于**调用实参摊开**的指令（`irgen.c:3369`
  的 `build_unpack_vars` 是 `let (x, y) = ...` 的解构，与此无关）。即 `!` 的 Sequence 语义
  目前只是一个类型标注。
- [ ] 实测后果（静默算错，比崩溃更危险）：

  ```kl
  func n(xs ...int) int { return xs.len() }
  func count_bang(xs ...int) int { return n(xs!) }
  func second(xs ...int) int { return xs[1] }
  func pass_bang(xs ...int) int { return second(xs!) }
  ```
  `count_bang(10, 20, 30)` 得 **1**（应为 3）；`pass_bang(10, 20, 30)` 运行时抛
  `Error: tuple index out of range`。`typeof(xs!)` 仍是 `<class 'tuple'>`，值就是原 tuple。

  链路：`xs!` 的 `ts` 已被 `parse_bang` 改成 Sequence 而非 VA_LIST → `skip_tuple` 不触发 →
  整个 tuple 被当成**一个元素**再包一层 → 被调方 `len()` 为 1。

  方向：`!` 需要真正的下降。打包时把 sequence 的元素展开进 tuple（运行期长度未知，需要
  runtime 侧的 tuple 拼接 / 扩展支持），或引入显式 spread 指令。属设计题，不是补一个分支。

### 91.5 缺陷 C：`xs!` 能否编译取决于编译顺序（偶然而非设计，待修）

- [ ] `match_type_spec()`（`typespec.c:829-832`）靠 `get_symbol_by_id(ts->sym_id)` 找符号；
  va_list 的 `sym_id` 初值是 -1（`va_list_type_spec()`，`typespec.c:732`），由
  `get_type_symbol()`（`parser.c:437`）**惰性**填写。于是 `xs!` 是否被认成 Sequence，取决于
  之前有没有别的代码碰过同一个 va_list TypeSpec。

  实测三个最小形态（同一函数体，只差前置语句）：

  ```kl
  // w1：xs! 是第一次使用 → 编译报错
  func pass(xs ...int) int { print(xs!, typeof(xs!))  return 0 }
  // error: only optional type/Sequence's subtype can use bang operator.

  // w2：先 xs.len() → 通过，且 xs! 的值仍是整个 tuple
  func pass(xs ...int) int { print(xs.len())  print(xs!, typeof(xs!))  return 0 }
  // → 3 / (1, 2, 3) <class 'tuple'>

  // w3：先 xs[0] → 同 w2
  func pass(xs ...int) int { print(xs[0])  print(xs!, typeof(xs!))  return 0 }
  // → 1 / (1, 2, 3) <class 'tuple'>
  ```

  方向：va_list 的类型符号应在 `resolve_type` 阶段就确定，不要依赖惰性填 `sym_id` 的副作用。

### 91.6 缺陷 D：`tuple.__getslice__` 未实现（2026-11-12 作者决定：暂不解决）

- [ ] 任何 tuple 切片都在运行时抛 `func 'tuple::__getslice__' is not implemented!`，
  **不限于变长参数**：

  ```kl
  let t = (1, 2, 3)
  print(t[1:])        // Error: func 'tuple::__getslice__' is not implemented!

  func tail(xs ...int) int {
      var s = 0
      for x in xs[1:] { s = s + x }   // 同样抛错
      return s
  }
  ```

  `tuple_methods[]`（`src/objects/tupleobj.c:60-66`）只注册了 `to_list` / `fmt` / `len` /
  `__getitem__`，没有 `__getslice__`。前端能过（`parse_slice_load` 认 tuple 是 Sequence），
  所以是纯运行期缺口，编译期无诊断。

  注：`demo/func_type.kl` 的 `reduce` 用 `xs[1:]` 能跑，是因为那里的 `xs` 是 `list[int]`；
  变长槽是 tuple，同样的写法就炸。`for x in xs` 直接迭代 tuple 是正常的，只有切片不行。

  方向（未施工）：给 tuple 补 `__getslice__`，返回新 tuple；需先定下越界 / 负下标 / 空切片的
  语义，与 §11（`OP_SEQ_GET_IMM` / `OP_SEQ_SET_IMM` 负下标语义未定义）一并考虑。

### 91.7 设计前必须先看：既有负向用例把「`!` 不摊开」钉住了

- [ ] `test/test-kl/test_list.kl:54,57-58`：`let l6_1 = list(100, l2!, 200)` 之后
  `l6_1[1][0] = 100` 期望报 `type 'int64' is not subscriptable (missing __setitem__)`。
- [ ] `test/test-kl/test_infer_new.kl:66-68`：`A(100, 200, "hello", l!, v5 = 1, v6 = false)`
  之后 `a6.f2[0] = 100` 期望报同样的错。

  这两条都建立在「`l!` 只贡献**一个**元素」的现状上。真要 implement spread，必须先决定这两个
  用例怎么改，不能直接改实现。

### 91.8 已修 / 已固化的部分

- 变长参数**静态下标 ≥ 1** 已修（原 C1）。根因：`get_valist_symbol()`（`parser.c:421`）把变长
  参数映射成 `tuple[元素类型]`，只有 1 个类型实参，而 `infer_tuple___getitem__` 拿字面量下标去
  和 `vector_size(inst_sym->tp_args)`（恒为 1）比。修法：`infer_func_tp` 增加 `recv_ts` 参数
  （`include/parser/parser.h:230`），`parser.c:53` 对 `TYPE_VA_LIST` 直接返回 `va_list.src`，
  不做 arity 检查（长度只有调用点才知道）。
  不能给 instance 打标记：`tuple[int64]` 与真实一元组 `(1,)` 是同一个缓存符号
  （`find_or_add_instance` 按 mangled name 缓存），打标记会破坏真实元组的越界检查。
- 用例：`test/test-run/test_vararg_index.kl`（纯类型 `...int` / `...str` / `...float` / 无类型
  `...` / 泛型 `...T`，覆盖下标 0/1/2、方法、泛型函数、泛型类方法，并保留动态下标、常量折叠
  下标、for-each、`len()` 作回归护栏；两条 RUN：默认 + `--ssa`）。
- **该用例目前故意留红**：按作者要求保留了 `pass`（缺陷 A）与 `pass_bang`（缺陷 B）两条待修
  用例，等设计定案后再动。全量 491/492，唯一红点就是它。注意文件里缺陷 B 被缺陷 A 遮住
  （`pass` 编译不过，lit 跑不到运行时），两个缺陷要分开修、分开验。

### 91.9 关联条目

§90 已记的「可变参数 `...Trait` 接收类实例时 VM 崩溃」（打包 tuple 时元素未 `make_intf`）与
本节同源，都在 `handle_valist_and_dfl_args()` 的打包处，宜一并设计。§89 / §90 已记的
「泛型可变参数 `...T` 推断 `NYI()`」（`parser_generic.c:126` 的 `TYPE_VA_LIST` 分支）也在同一
条链路上。

## 92. phi coalescing 把常量当空区间，导致两个循环的累加器被合并到同一个 local（2026-11-12，待修）

发现于「kw 传位置参数」的回归验证，**与 kw 无关**：全部复现都只用纯位置实参和字面量。
只在 `--ssa` 下出现，默认 VM 路径全部正确。

### 92.1 症状：同一函数里两个 for-each 循环，第一个累加器的值被清零

- [ ] 最小复现（`a`、`b` 都以 `0` 初始化）：

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

- [ ] **不是 range 的问题**：把两个 `range(3)` 都换成 `[0, 1, 2]` 一样错；只换其中一个也错。
  最初误以为是「连续 range 循环」，实测与迭代器种类无关。

### 92.2 触发条件矩阵（VM 全部正确，下表只列 SSA）

| 变体 | 第一个累加器初值 | 第二个初值 | 循环数 | SSA 输出 | 结论 |
| --- | --- | --- | --- | --- | --- |
| 基准 | `0` | `0` | 2 | `0 3` | 错 |
| 两个 `var` 都提到最前 | `0` | `0` | 2 | `0 3` | 错，与 `var` 位置无关 |
| 只改第二个初值 | `0` | `7` | 2 | `0 10` | 错，**只有第一个初值起作用** |
| 只改第一个初值 | `5` | `0` | 2 | `8 3` | 对 |
| 两个都非 0 | `5` | `7` | 2 | `8 10` | 对 |
| 同一个累加器贯穿两个循环 | `0` | — | 2 | `3 9` | 对 |
| 两个循环共用一个累加器、末尾才 print | `0` | — | 2 | `6` | 对 |
| 换成两个 `while` 手写循环 | `0` | `0` | 2 | `3 3` | 对 |
| 三个循环三个累加器（list 字面量） | `0` | `0` | 3 | `3 3 3` | 对，块布局变了 |

- [ ] 更硬的症状：第一个循环用 list、第二个用 range 时**直接编译崩溃**：

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

  ```
  cgen.c:1426: Why goes here?      // fill_mach_insn() 的 default: UNREACHABLE()
  ```

### 92.3 直接证据：de-ssa 之后 `b` 的 local 声明消失，`b` 的 phi copy 落在 print 之前

`koalac --ssa --dump=ssa` 的 de-ssa 输出（节选，基准复现）：

```
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

SSA 层本身是**对的**（`%a.phi.0` 与 `%b.phi.0` 是两个不同的 phi，各自 `phi 0, %backedge`）。
错在 de-ssa / coalescing：`b` 被并进 `a` 的 congruence class。

对照：把初值改成 `5` / `7`（正确的那一档），de-ssa 输出里 `%lc.a.phi.0` 与 `%lc.b.phi.0`
**是两个独立 local**，`move %lc.b.phi.0, 7` 同样落在 `%bb3`，但因为不共享槽位所以无害。

### 92.4 根因推断（证据充分，未施工验证）

- [ ] `src/parser/opt/phi_coalescing.c:108` `_build_interval()` 给常量 `start = 0`，
  `:77` `_build_interval_end()` 对常量直接 `return 0` —— 常量的活跃区间是**空区间 `[0, 0)`**。
- [ ] `:181` `_intervals_overlap()` 是 `a->start < b->end && b->start < a->end`。
  代入常量（`start == end == 0`）恒为 false，**常量与任何区间都不干涉**。
- [ ] `:386` `klr_coalesce_run_analysis()` 对每个 phi 的**每个** incoming operand 都调
  `_try_merge()`，常量 operand 也不例外。于是「所有以同一个常量对象为 incoming 的 phi」
  都并到那个常量的 class 里。`0` 是共享的同一个 `KlrValue`，所以 `a.phi.0` 和 `b.phi.0`
  被判定可合并 —— 而它们的真实活跃范围在 `%bb3` 的 print 处是重叠的（`b` 定义时 `a` 还活着）。
- [ ] 这正好解释 §92.2 的全部现象：初值不同 → 不同常量对象 → 不合并 → 正确；
  只有第一个初值为 0 也照样错（`b` 并进零 class 即可）；三个循环时块布局变化使线性区间真的重叠，
  反而被拦住。
- [ ] 附带问题：线性区间模型 `[def_pos, last_use_pos)` 按指令序号铺平，**不考虑 CFG**，
  也不把「phi copy 实际插在前驱块的什么位置」计入 incoming 值的活跃范围。
  即使修掉常量这一条，循环出口块里带其他代码时仍可能有别的漏判。

  方向（未定）：要么给常量算出真实区间（延伸到每个 phi copy 的落点，即 `pred->last_pos`），
  要么干脆不让常量参与 coalescing —— 常量的 phi copy 本来就该是一条立即数 `move`，
  不需要和 phi 共享槽位。后者改动面更小。

### 92.5 现状与规避

- 全量 497/497 通过，现有用例没有覆盖到这个形状。
- `test/test-run/test_kw_range.kl` 因此**每个函数只放一个 range 循环**，文件头注释记了原因。
  这是规避，不是修复；本节修好后可以把那些循环合回一个函数。
- 修好后应新增正向用例，至少覆盖 §92.2 矩阵里「错」的三档 + 崩溃那一档，两条 RUN（默认 + `--ssa`）。

### 92.6 关联条目

同在 `phi_coalescing.c` 的已修问题：嵌套 for-each + if 的 `move` 放错块（本轮之前已修）。
另有一条待议的 de-ssa critical-edge 问题，与本节同属「phi copy 落点」这一类，宜一并考虑。

## 93. 未初始化变量的读取：无诊断 + 垃圾值 / nil / 两处段错误（2026-11-12，待修）

**编译器对「声明了但没赋初值就读取」完全不报错。** 全仓库 grep 不到 `uninitialized` /
`not initialized` / `used before` 之类诊断，`VarSymbol`（`include/parser/symbol.h:82`）里
也没有任何「已赋值」状态位。顺着查出来 4 个独立问题，其中 2 个是段错误。

### 93.1 现状矩阵

| 写法 | koalac | VM | SSA |
| --- | --- | --- | --- |
| 局部 `var a int` 后 `print(a)` | exit 0，无诊断 | 打印 **40960**（垃圾） | **段错误** |
| 局部 `var s str` 后 `print(s)` | exit 0，无诊断 | 打印 **40960**（垃圾当指针） | 未测 |
| 全局 `var g int` 后 `print(g)` | exit 0，无诊断 | 打印 **`nil`** | 打印 **`nil`** |
| 全局 `var g int` 后 `print(g + 1)` | exit 0，无诊断 | **运行时断言崩溃** | **运行时断言崩溃** |
| 全局 `let c int`（无 `=`） | 已报错 `expected '='` | — | — |
| 局部 `let c int`（无 `=`） | **段错误** | **段错误** | **段错误** |

全局算术崩溃的断言：

```
include/runtime/vm_ops.h:85: _eval_frame: Assertion
  `((((regs + rs)->tag) & ~(uintptr_t)0b11) == 0b1000) || ... == 0b1100)' failed.
```

即一个声明为 `int` 的全局槽里装着 `nil`，是**类型谎言**，不只是「值为 0」的问题。

### 93.2 缺陷 A：局部 var 无初值被读取，无诊断（待修）

- [ ] 最小复现：

  ```kl
  func main() {
      var a int
      print(a)      // VM: 40960    SSA: 段错误
  }
  ```

- [ ] 前端根因：`src/parser/parser.c:1392` `parse_var_decl()`，`!exp && ts` 分支只设
  `sym->ts` 就 `sym->status = SYM_RESOLVED` 返回，不记录「有没有初值」。
- [ ] 后端根因：`src/parser/irgen.c:2711` `emit_ir_var_decl()`，局部变量只
  `klr_build_local_var()` 建槽，`exp` 为空就不发 `move`，槽里是未初始化内存。
- [ ] SSA 崩溃栈（`koalac --ssa`，gdb）：

  ```
  find_entry            src/common/hashmap.c:163
  hashmap_get           src/common/hashmap.c:177
  read_variable         src/parser/klr/ssa.c:85
  read_variable_recursive src/parser/klr/ssa.c:243
  read_variable         src/parser/klr/ssa.c:93
  build_ssa             src/parser/klr/ssa.c:318
  ```

  `read_variable_recursive()` 一路回溯到入口仍找不到该变量的任何定义。注意
  `build_ssa()` Step 2（`ssa.c:273`）已给**所有**块初始化过 `current_defs`，
  所以不是「map 没初始化」那么简单，回溯 path 上的 `pred` 有问题，具体机制未查清。
  前端加上诊断后合法程序不会再走到这里，但 SSA 侧本身是否该有兜底另议。

### 93.3 缺陷 B：全局 var 无初值，槽里是 nil（待修）

- [ ] 最小复现：

  ```kl
  var g int
  func main() { print(g) }      // nil
  ```

  ```kl
  var g int
  func main() { print(g + 1) }  // 运行时断言崩溃，见 §93.1
  ```

- [ ] 全局 `let` / `const` 无初值**已经**被文法拦住（`src/parser/koala.y:1524` 起的
  `let_decl` 所有非错误产生式都要求 `'='`），只有 `var` 开着这个口子。

### 93.4 缺陷 C：局部 `let` 无初值 → 编译器段错误（待修，独立小 bug）

- [ ] 最小复现：

  ```kl
  func main() {
      let c int
      print(c)
  }
  ```

  `koalac` 直接 SIGSEGV，**连那条 `expected '='` 都没打出来**。

- [ ] 根因：`src/parser/koala.y:1554` 的错误产生式 `LET ID optional_type error`
  报完 `expected '='` 后置 `$$ = NULL`；`koala.y:2757` 的 `local: let_decl semi`
  动作里 `var_set_where($$, VAR_LOCAL)` 是个裸宏
  （`include/parser/ast.h:537`：`((VarDeclStmt *)stmt)->where = _where;`），**不判 NULL**。
- [ ] 旁证：紧邻的 `local: const_decl semi`（`koala.y:2752`）里同一行
  **已经被注释掉了** —— 这个坑之前有人踩过，但只绕开了 const，没修 let / var。
- [ ] `var_decl` 的错误产生式（`koala.y:1741` 起）同样置 `$$ = NULL`，
  `koala.y:2762` 同样无保护。实测 `var a int =` 走到别的产生式没崩，
  但这条路径的 NULL 风险仍在，宜一并加保护。

### 93.5 必须保留的合法写法（实测 VM / SSA 都正确）

- [ ] 局部「先声明后赋值」是合法且常用的，修复**不能**简单地「没初始化式就报错」：

  ```kl
  var a int; a = 5; print(a)                    // 5
  var a int; if true { a = 1 } else { a = 2 }   // 1
  var a int; for i in range(3) { a = i }        // 2
  ```

- [ ] 全局可选类型的懒初始化，**全仓库唯一的无初值全局**，在 std 里：

  `libs/koala/log/default.kl:75` `var _default Logger?`，靠
  `if _default == nil { _default = Logger(...) }` 懒建（`:80`）。
  对 `T?` 来说未初始化就是 nil，语义自洽，**不能打断**。

### 93.6 拟定规则（未施工，待作者确认）

- [ ] 1. **全局非可选**必须有初始化式，否则报错 —— 堵住 §93.3 的类型谎言。
- [ ] 2. **全局可选 `T?`** 允许省略初值，等价于隐式 `= nil`，std 不动。
- [ ] 3. **局部 `var`**：给 `VarSymbol` 加「已赋值」标记，赋值语句置位；
  **整个函数里从未被赋值过**的变量被读取时报错。
  这是小步方案，漏判 `if c { a = 1 } print(a)` 这类部分路径未赋值；
  完整的必经赋值分析（definite assignment）要新增流分析 pass，且会拒掉
  §93.5 里「循环内赋值、循环外读取」这种现在能跑的写法，暂不做。
- [ ] 4. 顺手修 §93.4 的 NULL 解引用（`var_set_where` 加保护，或错误产生式不返回 NULL）。

### 93.7 施工代价与影响面

- `test/test-kl/test_call.kl:59,61` 有两个 `var l2 List[Bar];` / `var l2_1 List[Baz];`，
  **声明后从未使用**（遗留装饰）。规则 1 会让它们各多出一条错误，
  打乱该负向用例钉住的报错序列，需要补初始化式或删掉这两行。
- 除此之外 `libs/` / `test/` / `demo/` 里没有别的无初值全局。
- 按 TDD：先写用例（局部未赋值读取、全局非可选无初值、局部 let 无初值不崩、
  §93.5 三种合法写法 + std 懒初始化仍通过），再实现。

### 93.8 关联条目

§92 也是「读到一个不该读的值」，但那个是 SSA 优化把两个变量合并所致，与本节无关：
本节在**前端就没有任何检查**，两条后端都受影响。
