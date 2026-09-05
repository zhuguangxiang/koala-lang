# Koala TODO

> 记录未完成实现与待定设计：泛型数值协议（§1–4、§7–8）、语言手册（§5）、
> 版本计划与路线图（§6）、容器协议相关（§9、§11、§12）、Truthiness 设想（§10）。
> 已打通部分见 `Koala_Design_Overview.md` 第 3 节：十六件二元运算符的 IR 下降链路
> （IR 协议指令 → KLR `num.*` → 字节码 `OP_NUM_*`）已实测完整。
>
> 更新日期：2026-09-01

---

## 1. VM 侧 `OP_NUM_*` handler 族

**进展（2026-08-30）**：16 条中已完成 **11 条**——比较族 6 件（EQ/NE/LT/LE/GT/GE）
与算术族 5 件（ADD/SUB/MUL/DIV/MOD）均有 TARGET handler。`test_generic_9`（比较）
与算术分发均转绿。架构变化：`typeslots.c` 已删除，slots 改为 `TypeObject` 上的
`Vector`（`slotid.h` 独立头文件），slotdefs 迁移至 `klc.c`；算术族 TARGET 通过
`kl_slot_call_one_arg` 走 slots 分发。

**未完成**：位运算族 5 件——AND / OR / XOR / SHL / SHR，opcode 与 slot id
（`SLOT_BIT_AND` / `SLOT_BIT_OR` / `SLOT_BIT_XOR` / `SLOT_SHL` / `SLOT_SHR`）
均已定义，缺 VM TARGET handler。

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

**现状**：`koala.y` 已有 `in_expr` 语法规则、AST 已有 `EXPR_IN_KIND`，
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

---

## 5. 语言手册（Koala_Language_Manual.md）后续项

> 2026-08-22 整体 review 产出。P0 已完成（目录、Union Types、Error Handling、Memory Management），
> 以下为剩余项。

**P1 —— 完整性补齐**

1. 表达式与运算符基础章节：基础运算符参考（算术/比较/逻辑/位/赋值/复合赋值）+ 优先级表。
2. Containers 加深：每类容器常用操作示例；set 字面量 `{1,2,3}`；切片 `nums[1:3]` 示例。
3. Strings 加深：切片 `s[0:5]`、拼接、转义序列。
4. 内建函数速查表：`print` / `len` / `enumerate` / `zip` 收拢成表。
5. 词法基础：字面量进制（hex/oct/bin 在 parse_lit_int 有实证）、下划线分隔符待核实。

**P2 —— 可选**

6. 顶层代码说明（模块级语句在 `__init__` 执行、与 `main` 的关系）。
7. Trait 章节补充：`&` 多 trait 基列表、trait 继承（`Comparable : Equatable`）、trait 值作参数。

**优化（事实性/一致性问题）**

8. 重复内容合并：`class Foo[T]` 例子在 Classes and Interfaces 与 Generics 逐字重复；
   `add_pair` 出现 3 次（Functions / Generics / Operator Overloading）。
9. 运算符表 `x in seq` 一行：`in` 语义未落地（见上文第 4 项），需标注或暂时移除。
10. Operator Overloading 的 Point 示例输出注释失实：未实现 `__str__` 时按 Printable identity
    默认打印 `Point@0x7fXXXX`，不是 `Point(4, 6)`；给示例加 `__str__` 或改注释。
11. `guard` 悬空：Booleans 提到 guard 但全文无说明（语法存在、无测试实证）；删除该词或补说明。
12. 章节名 "Functions and Closures" 与正文（闭包不存在）矛盾，可考虑改为 "Functions"。
13. Hello World 的 `koala -c` 与测试体系用的 `koalac` 是否一致，待作者确认。

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

### 7.3 决策点（待作者拍板）

1. @intrinsic 空体的系统性解法：比较族已走 TARGET 快路径；**算术族已确定走
   slots 调用**（`kl_slot_call_one_arg`）；位运算族跟随同一模式。
2. 一元 `-` / `~`（OP_UNARY_* 仅 IR 伪指令）与泛型复合赋值仍是独立项，不在本批。

---

## 8. 泛型体内 T 的点号方法调用（itab 挂接）——作者决定暂时搁置

> 2026-08-22 建立。作者明确：此场景当初未实现是有意为之，复杂度超出表面，
> 重新开启前不推进任何方案。

**现状**：泛型体内对 `T : Trait` 参数做点号方法调用（如 `a.__str__()`、
`a.__hash__()`）会撞上 `vm_ops.h:431` 的 `is_intf(callable)` 断言——槽调用
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

## 9. list 的 push/pop 注解升级：@native → @intrinsic

> 2026-08-31 建立。

**现状**：`list.kl` 中 `push(value T)` 和 `pop(index = -1) T` 标记为 `@native`。

**目标**：改为 `@intrinsic`，让编译器直接发射专用 VM 指令：
- `push` → `OP_LIST_PUSH`（IRGen 已直接发射，无需 ISEL 特化）
- `pop` → `OP_LIST_POP`（当前走 OP_CALL + ISEL 特化路径，需 ISEL 支持）

**理由**：`push`/`pop` 是最高频的容器操作（循环构建列表、栈操作），
`@intrinsic` 消除 slots 分发开销，直接操作内部数组 + 容量检查 + write barrier。

**施工方向**：
1. `list.kl`：`@native` → `@intrinsic`（push、pop 两个方法）
2. 确认 IRGen 对 `list.push()` 发射 `OP_LIST_PUSH`（已有逻辑）
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
- 容器类**不**实现（`if my_list:` 不合法，必须 `if len(my_list) > 0:`）

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

**已完成全链路（isel / printer / VM TARGET）——5 条**：

- [x] OP_SEQ_GET（TARGET 经 kl_slot_call 走 SLOT_GET_ITEM）
- [x] OP_SEQ_GET_IMM（isel.c:1051 常数下标特化）
- [x] OP_SEQ_SET（SLOT_SET_ITEM）
- [x] OP_SEQ_SET_IMM（isel.c:1078）
- [x] OP_LEN（isel.c:622/632 下降；TARGET 走 SLOT_LEN）

**已定义、未实现——7 条**：

- [ ] OP_SEQ_GET_SLICE（SLOT_GET_SLICE）
- [ ] OP_SEQ_SET_SLICE（SLOT_SET_SLICE）
- [ ] OP_MAP_GET（IR 层已有：insn.c:816 用于成员访问下降；VM TARGET 缺）
- [ ] OP_MAP_SET（IR 层已有：insn.c:842，dce.c 已识别其副作用；VM TARGET 缺）
- [ ] OP_CONTAINS（SLOT_CONTAINS；依赖 §4 的 in 表达式语义下降）
- [ ] OP_LIST_PUSH（IRGen 发射 + TARGET；见 §9）
- [ ] OP_LIST_POP（OP_CALL + ISEL 特化路径；见 §9）

**每条待办含**：vm_ops.h TARGET（kl_slot_call_* 模式）、printer.c case、
isel 规则（如适用）、回归测试。

**源码注释遗留（顺手修正项）**：
- slotid.h:30 `SLOT_LEN, // OP_SEQ_LEN` — 注释里的指令名过时（现为 OP_LEN）
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

- [ ] **NYI 1b**：`int64.to_float()` / `uint64.to_float()` native 方法——**未验证**，构造函数路径通了不代表这两个方法通了
- [ ] **NYI 2b**：`float64.to_int()` native 方法——同上，未验证
- [ ] **窄整型字面量丢符号**（不是 cast 问题，但它卡住了两个 cast 用例）：超出 ±2048 的负 int16 / int32 字面量变成无符号值。根因在 `isel.c` 的 `get_const_op`——`[-2048, 2047]` 内发 `OP_LOAD_INT_IMM`，之外走 `OP_LOADK`，而 LOADK 路径没有做符号扩展。
  - 因此 `test_cast_int.kl` 中 `test_i32_to_f32(-100000)` 与 `test_i32_to_f64(-100000)` 仍以注释 + `// expect:` 保留，是仅剩的两个未启用跨族用例；同文件末尾另有一组字面量本身的 worklist。
  - `test_const_int_cast_failed.kl` 里 `let a int32 = -40000` / `-100000` 两处目前"因错误的原因"通过（字面量先变成大正数，再触发常量越界报错）；字面量修好后诊断文本不变，**无需改动**。
- [ ] **高位 uint64 print 成负数**：print 一个最高位为 1 的 uint64 输出有符号值（`UINT64_MAX` → −1）。疑似运行时打印问题，非 cast 问题；`test_cast_uint.kl` / `test_cast_float.kl` 的 CHECK 暂按实测值写。
