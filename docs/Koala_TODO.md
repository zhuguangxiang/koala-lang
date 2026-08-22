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

