# Koala TODO

> 记录未完成实现与待定设计：泛型数值协议（§1–4、§7–8）、语言手册（§5）、
> 版本计划与路线图（§6）、容器协议相关（§9、§11、§12）、Truthiness 设想（§10）、
> 数值 cast 补全（§13，已完成转语义 / 测试基线记录）、from_str 解析 API（§14，设计定案未实现）、
> 测试揭示的 WIP 特性（§15，2026-10-02 复核已消解）、声明-实现欠账清单（§16）、
> 整型提升三处缺陷（§17）、list 接口全面核对与主流语言对齐（§18）、
> 退出码模型（§19，panic 退出码与 main 返回值语义）、float 的 NaN/Inf 构造途径（§20，已修复）、
> `str(float)` 16 位有效数字缺陷（§21，已修复）、数值构造器接收 `str` 的语义（§22，待定，讨论中）、
> datetime 固定偏移时区（§23，待设计）、运算符按操作数类型重载以支持同类相减（§24，编译器级，待讨论）、
> Koala std/能力包 vs Python 标准库的场景×热度对比（§25，现状盘点，非待办）、
> `build_intern` 操作数个数溢出 `nargs:8` 后前端缺诊断（§26，待施工）、
> 全常量 dict 字面量直接作 `print` / `format` 实参时报 `modobj.c: Not Yet Implemented`（§27，待修）、
> os.run 子进程管道大 payload「先写后读」死锁 + Process/Raw* 无 finalizer 致 fd 泄漏与 zombie（§28，待修）、
> `is` / `as` 运算符语义评估与遗留项（§29，2026-10-26 评估）、let-else and `panic()`-as-terminal leftovers (§30, closed)、
> 泛型 bound 实参未转接口：泛型自由函数与泛型类 `__init__`（§34，待修，用例已留档为
> `test/test-run/test_intf_generic_bounds.kl.disabled`）。
> 已打通部分见 `Koala_Design_Overview.md` 第 3 节：十六件二元运算符的 IR 下降链路
> （IR 协议指令 → KLR `num.*` → 字节码 `OP_NUM_*`）已实测完整。
>
> 更新日期：2026-10-30（新增 §34：泛型 bound 实参未转接口——泛型自由函数与泛型类 `__init__`，
> 用例留档为 `test/test-run/test_intf_generic_bounds.kl.disabled`；同批已修的接口槽位三 bug
> 见 §34.4，回归测试为 `test/test-run/test_intf_{ext_inherit,local_inherit,optional}.kl`）; 2026-10-28 (§30 closed: `unreachable()` is terminal; no `unreachable` after `panic()`, to keep code size); 2026-10-27 (added §30: let-else is implemented); 2026-10-26（新增 §29：`is` / `as` 语义评估与遗留项）；2026-10-25（新增 §26：`build_intern` 操作数个数溢出 `nargs:8` 后前端缺诊断，待施工；§27：全常量 dict 字面量直接作实参报 `modobj.c` 未实现，待修；§28：os.run 子进程管道大 payload 先写后读死锁、Process/Raw* 无 finalizer 致 fd 泄漏与 zombie，待修；§20 / §21 均已修复，§22 待定；datetime 包 Date/Time/DateTime 已收工，仅剩时区见 §23；§24 为运算符重载语言待办；§25 为 Koala vs Python 标准库场景×热度对比盘点）

---

## 1. VM 侧 `OP_NUM_*` handler 族

**进展（2026-08-30）**：16 条中已完成 **11 条**——比较族 6 件（EQ/NE/LT/LE/GT/GE）
与算术族 5 件（ADD/SUB/MUL/DIV/MOD）均有 TARGET handler。`test_generic_9`（比较）
与算术分发均转绿。架构变化：`typeslots.c` 已删除，slots 改为 `TypeObject` 上的
`Vector`（`slotid.h` 独立头文件），slotdefs 迁移至 `klc.c`；算术族 TARGET 通过
`kl_slot_call_one_arg` 走 slots 分发。

**未完成**：位运算族 5 件——AND / OR / XOR / SHL / SHR，opcode 与 slot id
（`SLOT_BIT_AND` / `SLOT_BIT_OR` / `SLOT_BIT_XOR` / `SLOT_SHL` / `SLOT_SHR`）
均已定义，缺 VM TARGET handler。（2026-10-02 复核：vm_ops.h 仍只有 OP_NUM_EQ..MOD 共 11 个 TARGET，位运算族维持未实现。）

**施工方向**：按算术族模式补 5 个 TARGET——`kl_slot_call_one_arg(self, arg,
SLOT_BIT_AND)` 等。

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
- 位运算族 5 件（AND/OR/XOR/SHL/SHR）仍未实现；
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

**第二批续：位运算族** ❌ 未实现

- [ ] OP_NUM_AND/OR/XOR/SHL/SHR 五个 TARGET
      （`kl_slot_call_one_arg(self, arg, SLOT_BIT_AND)` 等）
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

- [ ] `number.kl`：int64/uint64/float64 的 `__init__` 去掉 `str`（和 base）；加 `from_str_or` / `try_from_str` static 声明 + doc comment
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

## 17. 整型提升的三处缺陷（rhs 期望类型传播 / 诊断）

> 2026-10-03 建立。源码分析 `src/parser/parser_expr.c` 的 `promote_integer_type` / `parse_binary`
> （提升规则与两阶段实现已入设计文档 §5、§5.3）时发现的三处问题，**均未修复**。
> 三个修法的决策完备方案见设计文档 **§5.4**，此处只挂账问题本身。

### 17.1 孤岛规则的错误消息文本不准

- 现状：`parse_binary:2812-2816` 写死 `"cannot do binary operation on int64 and uint64 with different signs."`。
- 问题：实际触发条件是 **uint64 与任意宽度的有符号类型**（`type_is_int` 只判 `TYPE_INT && sign == 1`，不含宽度），因此 `int8 + uint64`、`int32 * uint64` 都会报出“int64 and uint64”这样的假信息，也不告知用户可用的显式 cast 出路。
- 影响：诊断误导。196 个测试无一覆盖此错误路径（全是正例）。
- 修法：设计文档 §5.4 修法一（带上两侧真实 `signature` + 两条 cast 出路）。

### 17.2 提升日志永远打印目标类型，起不到诊断作用

- 现状：`promote_integer_type:2766-2774` 的四条 `log_info` 都在**赋值之后**读 `lhs->ts->int_flt_info.width / sign`，此时已被覆写为 int64 / uint64，故日志恒为 `from int64 to int64` / `from uint64 to uint64`。
- 函数开头（`:2754-2755`）已把原始类型存进 `lhs_ts` / `rhs_ts`，但四条日志都没用它们。
- 对照组：同文件 `parse_unary:2703-2736` 用 `orig_ts` 打印，写法正确；`parse_binary:2810` 的 `orig_ts` 只在 float 分支用到。
- 修法：设计文档 §5.4 修法二（先打印后赋值，或直接引用 `lhs_ts` / `rhs_ts`）。

### 17.3 uint64 左操作数 + 具名常量：期望类型传播的不对称缺口

- 现状：`parse_binary` 有两条把 lhs 类型传给 rhs 的通道，二者对 uint64 的处理互补而不重合：
  - 字面量通道（`:2791`）：rhs 是 `EXPR_LITERAL_KIND` 且 **lhs 是 uint64** → `rhs->expected = lhs->ts`
  - 具名常量通道（`:2800`）：rhs 是 `EXPR_ID_KIND` 带 `SYM_FLAGS_CONST` 且 **lhs 不是 uint64** → 设 expected 并重新解析
- 主缺口：**lhs 是 uint64 且 rhs 是具名常量**时两条都不生效。`x + UINT8_MAX`（x uint64）里 UINT8_MAX 保持自身推断的有符号类型 → 撞孤岛报错；而同一表达式的字面量形式 `x + 255` 合法。**此路径现有测试未覆盖，属源码推演结论、未实测。**
- 连带误报（同源）：具名常量通道对**窄 lhs + 更宽常量**会报错。`check_literal_as_expected` 超范围时先 `kl_error("Literal value out of range for intN")` 再返回 0（`include/parser/literal_as_expected.h`），故 `n + INT16_MAX`（n int8）报错，而字面量形式 `n + 32767` 因通道不触发而合法。二元运算里的 rhs 类型只是“选公共类型的提示”，不是用户声明的目标类型，报错过严。
- 字面量 vs 具名常量的结果类型不一致：`u + 100`（u uint8）得 `int`，`u + UINT8_MAX` 得 `uint64`——同量级的值、两种写法，结果类型不同。属同一不对称的另一面。
- 对照组：函数实参通道（`:597-616`）对字面量与具名常量一视同仁、且不排除 uint64，这正是 `test_u64_add_u8(UINT64_MAX, UINT8_MAX)` 能通过的原因——说明该机制在 uint64 下本就可用。
- 修法：设计文档 §5.4 修法三（新增不报错的 `int_value_fits` 适配判定 + 两条通道统一条件）。其中“字面量通道放宽到任意窄 lhs”会使 `u + 100` 的结果从 `int` 变 `uint64`，是本修法唯一的语义变化，**需作者确认后再实施**（保守变体见 §5.4 末段）。

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
1. `sort()` / `sort_by(cmp)`——三语言全都有，koala 只声明了 `reverse`（且未实现），最显眼的缺失；
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
- **P2（能力补齐）**：`sort` / `sort_by`；`copy()` 语义收敛；`empty` → `is_empty`；`first` / `last`。

### 18.6 测试现状与挂点

- 现有用例只覆盖可用面：`test/test-run/test_list*.kl`（5 个文件）+ `test/test-kl/test_list.kl`
  仅使用 `push`（21 处）/ `len`（6 处）/ 下标赋值 / 切片；**test-ut 无 list 用例**。
- 每实现一个欠账方法，应在 `test/test-ut/test_list.kl`（待新建）补用例，
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
| 网络 | socket, http, urllib, ssl | **全无** | 最大空白区，属重生态，通常第三方承担 |
| 加密/编码/压缩 | hashlib, hmac, base64, zlib, gzip, codecs | **无**（`.hash()` 是 FNV-1a 内部哈希，非加密） | base64/zlib 实用但非语言核心 |
| 命令行解析 | argparse, getopt, shlex | **无**（wombat 是构建工具，非 CLI 解析器） | 判第三方 |
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

## 34. 泛型 bound 实参未转接口：泛型自由函数与泛型类 `__init__`（2026-10-30 发现，待修）

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
