# Koala TODO：泛型运算符协议未完成项

> 记录泛型数值协议（Arithmetic / BitwiseOperators / Comparable）相关的未完成实现。
> 已打通部分见 `Koala_Design_Overview.md` 第 3 节：十六件二元运算符的 IR 下降链路
> （IR 协议指令 → KLR `num.*` → 字节码 `OP_NUM_*`）已实测完整。
>
> 更新日期：2026-08-22

---

## 1. VM 侧 `OP_NUM_*` handler 族（优先级最高）

**现状**：`vm_ops.h` 中仅 `OP_NUM_EQ` 有 TARGET handler，其余 15 条
（ADD/SUB/MUL/DIV/MOD、AND/OR/XOR/SHL/SHR、NE/LT/GT/LE/GE）缺失，
执行到时落入 `eval.c:176` 的 UNREACHABLE 分支直接 abort。

**影响**：
- `test_generic_9` 基线失败的直接原因；
- 任何经泛型 trait 约束的数值运算（如 `func f[T : Arithmetic](a T, b T) T { return a + b }`）
  编译正常、运行时崩溃。

**施工方向**：按 `OP_NUM_EQ` 的模式补齐 handler——经当前帧的 intf-table
O(1) 分发到具体类型的 `__add__` 等方法，保持零 call frame 语义。

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
写 `5 in r` 直接**段错误**（NULL handler 跳转地址 0）。

**影响**：运算符显式调用禁令（43 项，见设计文档第 3 节）落地后，
`__contains__` 不能显式调用，而 `in` 糖又未实现——
**dict/set 的成员判定目前没有任何可用入口**。

**施工方向**：为 `EXPR_IN_KIND` 补语义处理与 IR 下降，
下降为对 rhs 类型 `__contains__` 的方法调用即可。

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

### 7.1 现状快照（建立时点，只读核实）

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

**第一批：比较族（零挂接改动，修 test_generic_9）**

- [x] OP_NUM_EQ 增加 int64 / uint64 / float64 快路径（初版两处 bug 已修：
      `==` 笔误改赋值、float 分支补 `DISPATCH()`）——2026-08-22 完成
- [x] OP_NUM_NE / LT / LE / GT / GE 五个 TARGET（EQ 模板 + 同款快路径）——2026-08-22 完成
- [x] uint64 快路径：拆独立分支 + `(uint64_t)ival` 无符号比较，六条全齐
      （EQ/NE/LT/LE/GT/GE），与 OP_UINT_CMPLT 既有约定一致——2026-08-22 完成
- [ ] printer.c 补 num.ne / lt / le / ge 四个 case

**第二批：算术 + 位运算（补 typeslots 脚手架）**

- [ ] typeslots.c：`slot_tp_binary` 公共助手 + 10 个 trampoline
      （ADD/SUB/MUL/DIV/MOD/LSHIFT/RSHIFT/BIT_AND/BIT_OR/BIT_XOR）
- [ ] num_slotdefs 填 FUNC；新增 bit_slotdefs（dunder 名以 number.kl 为准：
      `__shl__` / `__shr__` / `__bitand__` / `__bitor__` / `__bitxor__`）
- [ ] kl_tp_install_slots：补两段绑定循环（首个 dunder 命中时惰性分配
      tp->arith / tp->bit，保留"已实现不覆盖"语义）
- [ ] 10 个 TARGET（tp->arith->xxx / tp->bit->xxx，ASSERT 三件套）
- [ ] 内建类型算术/位运算裁决：TARGET 快路径（同比较族）还是 intrinsic 加载机制
- [ ] printer.c 补 num.add/sub/mul/div/mod/and/or/xor/shl/shr 十个 case

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

- [x] test_generic_9 转绿（六件比较运算符 × int64/float64，24 组输出全对齐）
      ——2026-08-22 lit 单测 PASS
- [ ] uint64 泛型比较测试用例：六条 TARGET 的 uint64 快路径暂无测试覆盖——字面量
      推不出 uint64，需先确认 `uint64(...)` 构造器路径能否参与泛型推导，再补进
      test_generic_9 或另建用例（重点验 LT/GT 的 `(uint64_t)` 无符号比较，
      用高位为 1 的大数如 UINT64_MAX 与小数比较）
- [x] test-run 新增用户自定义数值类用例（真字节码体 dunder，验证 slots 全链路，
      避开 intrinsic 空体问题）——test/test-run/test_generic_14.kl
      （Score 类六件比较 dunder + 泛型 Comparable 函数全六件 + max/min if-branch，
      16 组输出对齐），2026-08-22 lit 单测首跑 PASS（泛型比较用户类
      OP_NUM_* 兜底链路 slot_tp_richcmp → dunder 验证通过）
- [ ] 全量 test-debug.sh 无新增回归

### 7.3 决策点（待作者拍板）

1. @intrinsic 空体的系统性解法：比较族已走 TARGET 快路径；算术族跟随快路径，还是
   走定案中的"klc 增加 intrinsic flag + 加载期 C 实现查找 + sentinel"（klc 格式需动），
   二选一或分阶段。
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
