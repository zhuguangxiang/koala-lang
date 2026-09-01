# Koala 语言设计文档

> 本文档基于 Koala 作者的设计决策与代码库评审整理，覆盖语言核心、标准库、官方库与性能现状。
> 所有结论均为作者已确认的定案，未公开部分（并发细节、JIT）仅标注存在，不做推测。

---

## 1. 三条最高设计原则

Koala 的每一个设计决策都可以向上追溯到这三条原则：

1. **静态强语言中的 Python** —— 语义直觉对齐 Python（插入序 dict、dunder 协议、便利方法），但以静态强类型在编译期守门。
2. **没有秘密隐藏的** —— 语言对用户零暗角：内建类型在标准库源码中声明，编译器行为（slice 补全、自动遵循）在 LRO / 方法表中可见。
3. **简洁是必须的** —— 简洁同时约束 API 表面**和编译器实现**：不为表面的统一引入隐藏机制（豁免检查、桩代码等）。

---

## 2. 类型系统

### 2.1 `any`：空根，完全不透明

- `any` 是空的根 trait，**不声明任何方法**。
- any 型变量**不能执行任何运算**（包括 `==`），必须先窄化（isinstance）后使用。
- 静态类型不同的值比较（如 `1 == "abc"`）是**编译错误**——相等性由编译器全程保证。

### 2.2 根契约：自动遵循 + identity 默认

`Equatable[T]` / `Hashable` / `Printable` 保持**分立**的泛型 trait，普适性在**遵循层**实现：

- 编译器**自动让每个 class 遵循**这三个 trait，并插入 identity 默认实现：
  - `__eq__` = 指针比较
  - `__hash__` = 指针派生
  - `__str__` = `Foo@0x7fXXXX`（Java toString 风格）
- identity 默认使哈希契约自洽：同一性相等 ⟹ 同一指针 ⟹ 同一 hash，零特判。
- 可变容器按 identity 哈希**结构上永远安全**（指针不随内容变化），无需 Python 式的 unhashable panic。容器类型不声明 `__hash__`，走 identity 默认——语义定性为**实际不可哈希**：容器硬做 dict 键时语法放行、运行不 panic，但键是"这个对象"（identity）而非"这份内容"，内容寻址不可达；不做 Python 式的保姆拦截（unhashable TypeError），责任归使用者。
- 内建值类型（int64 / uint64 / float64 / bool / str）覆写三件套为值语义。其中 int / float / bool 声明 `__hash__` 仅为模板可见性（builtin 是用户的代码范本），VM 实际走默认实现——零装箱使同一性即值相等，identity 默认天然正确；str 为真值语义。
- 覆写 `__eq__` 而不覆写 `__hash__` → 编译器警告。
- 自动遵循在 LRO 中可见，无隐藏。

**为什么不合并进 any**：合入非泛型的普适 `any` 会强制 `other any` 签名，需要豁免不变检查 + intf-table 桩代码两个隐藏机制，违背"无隐藏特性"。保持泛型 trait 则每个类型实例化独立槽位（如 `Equatable[int64].__eq__(other int64)`），类型精确、零桩、零豁免。

**根契约的定位**：Java 的问题是把根类型与三大契约焊死在一个具体神类 `Object` 里——equals(Object) 类型擦除、所有类型强制继承绑定。Koala 的解法是拆解：**空 trait `any` + 三个独立的泛型 trait（Equatable[T] / Hashable / Printable）**——any 只做普适父类型，不承载任何方法；三契约分立、类型精确、自动遵循，而默认实现沿用 Object 的 identity 语义（引用相等、指针哈希、`Foo@0x...`）。这就解决了 Java class Object 的全部问题，差别只在容器：Java 集合覆写了 equals/hashCode 为内容语义，Koala 容器保持 identity 语义贯彻到底。

**这一设计根治了 Java 的 equals(Object) 顽疾**：

| Java 的痛 | Koala 的解 |
|-----------|-----------|
| equals 入参被 Object 锁死 | intf-table 槽位类型精确，编译器全程检查 |
| 每个实现手写 instanceof boilerplate | 一行不用写 |
| 普适性靠继承锁死签名 | 自动遵循，签名不损失 |

### 2.3 泛型参数自动获得三契约

`Map[K, V]` 等泛型中的 K / V 因万物自动遵循而**默认可用** `__hash__` / `__eq__` / `__str__`——dict 对键的哈希与相等判定走 K 的 intf-table 槽位，**无需任何约束声明**。

### 2.4 其他类型机制

- **联合类型**：所有成员必须是 final class，不允许 trait 成员。
- **可选类型**：`T?`。
- **构造函数**：偏好联合类型参数的单一 `__init__`，不用静态工厂方法族。
- **带默认值的参数**：省略类型注解。
- **变量声明**：支持默认值与**类型推导**——有初始化表达式即可省略类型注解。
- **泛型自动推导**：泛型类型参数由编译器从上下文推断，调用侧无需手写类型实参。
- **手动泛型展开**：`@specialized` 注解按放置位置分流——挂在具体声明上映射（`list[int64]` → `int64list`），挂在泛型类 / 泛型函数上由编译器生成特化声明；实例化无论显式书写或推断所得，注解即开关，未关联则擦除（见 §7.4）。

### 2.5 Null 安全与值表示

- **无 null**：语言中不存在 null，可选值用 `T?` 表达——从根上消灭 NullPointerException 这一问题类别。
- **分支自动窄化**：if/else 分支中 `T?` 自动窄化为 `T`（flow-sensitive smart cast），无强解仪式。
- **if let / while let 自动解包**：解包即窄化，编译后**不产生多余指令**——无 Rust 式 match 仪式，无强解后缀，零运行时代价。
- **无 box/unbox**：实现层不存在装箱机制——值类型进入普适 `any` / 泛型容器无需变身。对照：Java autoboxing（隐藏分配 + Integer 缓存陷阱）、C# struct 装箱、Kotlin Int-as-Any 装箱、Go interface{} 装箱。"普适 any + 泛型"与"零装箱"通常互斥，Koala 两者兼得，是 VM / 表示层的工程成就。
- 零装箱同时是"无隐藏特性"原则的直接推论（装箱即隐藏分配、隐藏身份变化），也是基准性能的贡献项。

---

## 3. 运算符模型：钩子与契约分轨

Koala 的运算符重载由 **dunder 本身授予（语法钩子），而不是由 trait 声明授予**——实现了 `__add__`，`p + p2` 天然可用，无需任何声明仪式。Koala 存在运算符相关 trait（`Arithmetic` / `BitwiseOperators`，见下文），但它们只是**泛型约束标签，不参与运算符能力的授予**。四条钩子族：

1. **算术 / 位运算符**
2. **下标访问**：`__getitem__` / `__setitem__` / `__getslice__` / `__setslice__`
3. **可调用**：`__call__`（不设 Callable trait——`__call__` 的入参个数不固定，而 Koala 无变参泛型，任何 trait 形状都无法表达“任意参数可调用”，故只留钩子、不设契约）
4. **比较运算符**：全部六个 `__eq__` / `__ne__` / `__lt__` / `__le__` / `__gt__` / `__ge__`

要点：

- 每个类型**自行定义**运算符参数类型，无跨类型签名强制。
- **运算符契约拆成两个 trait**（定义于 `number.kl`），同为**泛型而存在**——`func f[T : Arithmetic]` 凭它约束运算符能力；class 实现了相应 dunder 即天然支持（结构式遵循，无需显式声明）：
  - **Arithmetic[T]**：五件二元算术（`__add__` / `__sub__` / `__mul__` / `__div__` / `__mod__`）+ 一元 `__neg__`；
  - **BitwiseOperators[T]**：五件二元位运算（`__shl__` / `__shr__` / `__bitand__` / `__bitor__` / `__bitxor__`）+ 一元 `__bitnot__`。

  内建遵循：int64 / uint64 两者全遵循（uint64 不声明 `__neg__`——无符号取负无语义，靠部分 trait 实现机制自动 not_impl 占位）；float64 只遵循 Arithmetic（含 `__neg__`）。两个存在理由：其一，让泛型有能力实现运算符；其二，性能——用户类型的二元运算符经 `OP_NUM_*` 协议指令分发，**不创建 call frame**（内建数值走专用指令，同样零帧；只有普通方法调用付帧）。一元协议 op 尚未落地（现 `OP_UNARY_*` 为 IR 层伪指令），泛型一元分发列入后续施工。比较运算符不入这两个 trait，归 Equatable / Comparable 契约轨。

  泛型例子：

  ```kl
  func add_pair[T : Arithmetic](a T, b T) T {
      return a + b
  }

  print(add_pair(3, 4))    // T 推断为 int64
  ```

  函数体内 `a + b` 的操作数类型是未知类型 T，编译器不知道具体实现，但因 `T : Arithmetic` 约束而知道"加法可用"：IR 下降为协议 `add` 指令，后端转为 `num.add`（字节码 `OP_NUM_ADD`，数值协议指令），目标运行时经 Arithmetic intf-table O(1) 分发到 T 的 `__add__`，全程不创建 call frame。当前实现状态（2026-08 实测）：**十六件二元运算符的 IR 下降链路已全部打通**——IR 协议指令（`add/sub/mul/div/mod`、`shl/shr/and/or/xor`、`cmplt/cmple/cmpgt/cmpge/cmpeq/cmpne`）经优化器后保留，isel `num_ops_rules[]` 十六条 `OP_BINARY_* → OP_NUM_*` 映射齐备，寄存器分配后存活到 LIR；**比较族六件与算术族五件 VM handler 均已落地，`test_generic_9` 转绿**。尚不支持：泛型一元 `-` / `~`、泛型复合赋值（`+=` 等）、VM 侧位运算 `OP_NUM_*` handler 族（5 件）——详见 `docs/Koala_TODO.md`。

  **trait 作参数类型（如 `a Arithmetic[int]`，对标 Rust `dyn Trait`）时运算符语法不可用——设计边界，非实现缺口**：trait 运算符方法实例化后是实现侧的具体签名（如 `__add__(int64) int64`），而操作数是 trait 值本身，类型不匹配即编译报错；trait 值与具体类型混算（如 `a + 100`）同样报错。Rust 同理：运算符 trait 族（`std::ops::Add` 等）`add(self, ...)` 按值消费 self，非 dyn-compatible，`dyn Add` 根本写不出来。运算符只存在于具体类型与泛型（`T : Arithmetic`）两条路径；trait 值上保留 `__len__` 等协议钩子调用。
- **语义 trait 保留 dunder 声明**（如 Sequence 声明 `__getitem__` 等）：它们定义概念并只约束遵循者，对标 Python collections.abc。
- **Comparable 保持现状**：`Comparable[T] : Equatable[T]`，声明四个排序方法加继承的 `__eq__` / `__ne__`（曾考虑的单 `cmp` 方案随"合并进 any"动机消失而作废）。
- **运算符只走语法糖，禁止显式函数调用**（学 Swift）：运算符钩子的唯一入口是对应语法，按名字显式调用是编译错误，报错信息直接指向应使用的语法糖。禁止范围（43 个）：
  - 算术与复合赋值：`__add__` / `__sub__` / `__mul__` / `__div__` / `__mod__` / `__neg__`、`__iadd__` / `__isub__` / `__imul__` / `__idiv__` / `__imod__`（走 `+ - * / %` 及 `+= -= *= /= %=`）；
  - 位运算与复合赋值：`__bitand__` / `__bitor__` / `__bitxor__` / `__bitnot__` / `__shl__` / `__shr__`、`__ibitand__` / `__ibitor__` / `__ibitxor__` / `__ishl__` / `__ishr__`（走 `& | ^ ~ << >>` 及对应复合赋值）；
  - 逻辑：`__and__` / `__or__` / `__not__`（走 `&& || !`）；
  - 比较：全部六个（走 `== != < <= > >=`）；
  - 下标：`__getitem__` / `__setitem__` / `__getslice__` / `__setslice__` / `__getsub__` / `__setsub__`（走 `x[i]`、`x[a:b]`、`x[key]`）；
  - 可调用：`__call__`（走 `obj(...)`）；成员判定：`__contains__`（走 `x in seq`，`in` 表达式已有语法、语义下降待实现）；构造：`__init__`（走 `Type(...)`，二次显式构造属于隐患，一并禁止）。

  协议钩子不受此限：`__len__`（`len()`）、`__str__`、`__hash__`、`__iter__` / `__next__` / `__has_next__`（`for` 为主入口，手动迭代允许）。动机：显式调用并不提供超出语法糖的任何能力，反而曾是静默错误入口——`@intrinsic` 方法的空 body 被显式调用时静默返回 none。禁止不影响泛型分发：`max[T: Comparable]` 之类的运算符分发走编译器在运算符 call site 生成的 intf-table，不经用户显式调用路径。

  **禁令对接收者形态无差别**：具体类型、trait 值（如 `Arithmetic[int]`）、泛型类型参数（`T : Arithmetic`）三种形态的显式 dunder 调用报同一条禁令错误。泛型参数的成员解析经 bound 完成——bound 是 T 的成员唯一事实源，运算符下降与点号成员访问共用同一条 bound 查找；正因 bound 承诺了方法存在，T 上的显式调用报的是“禁止”而非“方法不存在”，拒绝理由是规则而不是能力缺失。禁令名单之外的协议钩子（如 `T : Hashable` 的 `__hash__`、`T : Collection` 的 `__len__`）经 bound 解析后按普通成员调用，正常可用。
  
  **禁令按符号划界，不按名字形状**：直接调用禁令的适用对象是特定符号族——内置运算符钩子（上述 43 个）、内置 magic 函数（`len` / `hash` 等 intrinsic）、编译器自动生成的符号（`@specialized` 生成物，见 §7.4）。dunder 名字形状本身不构成禁令依据：**用户自写的 `__foo__` 是合法声明、普通可调用的符号，不做任何拒绝**。

---

## 4. Trait 与分发机制

- **intf-table**：Rust 风格的每类 trait 表，**O(1) 接口分发、无需查找**——槽位含方法名 + code_index，trait 调用 = 槽位一次间接调用；class 方法调用 = 编译器查参后的**直接调用，零开销**。
- **trait 值表示（intf 值）**：具体类型扩宽为 trait 类型的值（如 `let e Equatable[bool] = false`），TValue 首字携带该类的 intf-table 指针，载荷原样保留。**扩宽是单向的**：primitive（int/float/bool 等）cast 到 trait 后原 tag 即告丢失，不可 downcast 回具体类型（对照：`any` 通道保留 tag，支持 `isinstance` 窄化）。每个 intf-table 带一个回指实现类 TypeObject 的指针，供运行时自描述（`typeof` / `print` / GC 标记）——这是运行时内省设施，不是面向用户的 downcast 通道。
- **部分 trait 实现**：class 遵循 trait 时允许只实现其中一部分方法。未实现的方法在 intf-table 中填入 `not_impl` 占位对象，加载期仅警告、不阻断；只有当未实现的方法被真实调用时抛异常 `Exception("function xxx in xxx is not implemented.")`——语义是"这需要你去实现它"。这一设计直接消灭了 Java 的适配器类生态：Java 强制接口全量实现，迫使生态发明 MouseAdapter / WindowAdapter 之类的空壳抽象类，只为让用户覆写十个方法中的一个；Koala 的答案是声明归声明、实现按需，intf-table 保持 O(1) 静态分发——**契约在类型层面完整，义务在实现层面宽容**。
- **泛型约束**：如 `T : Comparable[T]`；缺少约束只在泛型参数要求时报错。
- **自类型推断**：裸遵循自动推断自类型参数（如 Equatable → Equatable[bool]），无 Self 关键字。
- **LRO**：类型内省可见全部遵循关系与编译器插入的内容。
- **变型**：参数不变（invariant），返回值协变（covariant）。

**三轨分发，零运行期名字解析**——Koala 全部调用形态归入三条轨道，没有一轨需要 vtable 式的方法名运行时查找：

| 调用场景 | 分发机制 | 查表开销 |
|---------|---------|---------|
| class 自身方法 | 同模块编译期解析为直接调用（CALL rel32）；跨模块按名查 `tp->members`（wasm import 风格，绑定期完成） | 零（直调）／绑定期一次 |
| trait 泛型分发 | `itables[idx]`（intf-table 按编译期定死的下标） | 一次索引 |
| 协议 dunder（运算符/哈希/下标等） | `slots[id]`（`SlotId` 编译期定死，见 §7.3） | 一次索引 |

`TypeObject` 上因此只有两张分发表：`itables` 与 `slots[]`（热区分发数据，紧邻排布）——不存在也不需要 vtable；方法名字典（`members`）只服务跨模块绑定与内省，不在调用热路径上。

---

## 5. 数值类型

- 类型族：`int`（机器字）、`int8/16/32/64`、`uint8/16/32/64`、`float16/32/64`。
- **窄类型 init-only**：只在初始化时接受窄类型字面量，运算时自动提升：
  - int8/16/32 → int64，uint → uint64，float → float64
  - 提升是**编译器机制**，标准库源码中不可见。
- **溢出**：trap 与 wrap 两种操作，由编译器 / VM 层实现，标准库无需感知。

---

## 6. 错误处理：C 式返回值 + panic

Koala 的错误模型是**刻意设计**，非缺口：

- **可预期的失败 = 返回值错误码**（C 风格，如 int 返回 -1）。
- **不可预期的失败 = 抛异常**：Koala 有异常机制，但非常轻量级——`panic` 函数是抛异常的唯一入口，抛出 `Exception` 对象；异常**不可恢复、不可捕获，一旦抛出必须解决**（panic 即 bug，必须改代码）。
- 明确拒绝：Java try-catch、Go defer（可读性差）、Result 式组合（作者裁定他国无更优特色解法）。
- **与 GC 的协同**是这一模型成立的关键：无析构函数 / 资源泄漏之忧，栈可直接丢弃；无捕获则无展开表，panic 真正轻量。
- bytes 错误模型定案：返回类型的方法用 panic，返回 int 的方法用 -1。
- 可失败转换模式：保留 panic 主方法 + `_or(default)` 变体（如 `to_int()` / `to_int_or(0)`）。
- Map 返回 `T` 的方法必须文档化键缺失时的 panic 行为。

---

## 7. FFI：无 unsafe、零损失

Koala 的 FFI 是独一份的简单与高性能：

```koala
link "koala_ext_native"

@native
pub func pretty(s str) str {}
```

- 模块声明 `link "库名"` + `@native` 函数（普通 Koala 签名），**编译器独占桥接**。
- 类型系统信任声明签名，用户从不触碰桥接代码——`unsafe {}` 存在的理由（人在绕过类型系统）被结构性消除。
- **内存层同样无 unsafe**：shadowstack 将 C / native 代码分配的对象注册为 GC root——native 侧分配的对象与 `.kl` 中分配的命运完全一致，无"记得释放"规则。对照：JNI 局部/全局引用、Python C API 引用计数、Go cgo handle table 均需手动管理。
- 对照：Java JNI（句柄仪式）、Go cgo（栈切换开销）、Rust（强制 unsafe + transmute）、Python ctypes（运行时 marshal）。
- 标准库即活证据：`libs/std/native/*.c` 直接实现 Koala 签名函数。

### 7.1 三种函数：三档绑定规则

Koala 的函数按声明方式分为三种，**声明与实现分离，实现的位置分三处：字节码、C 库、编译规则**。模块加载期的绑定规则：

| 声明 | body | 找到 C 注册实现 | 未找到 | 被调用时抛异常语义 |
|------|------|----------------|--------|--------------------|
| 无注解（普通函数） | 编译为 code 字节码，VM 执行 | C 实现覆写字节码（热路径优化入口） | — | — |
| `@native` | 必须为空 | 绑定 C 实现 | 填 `not_impl` 占位对象，**警告**不阻断 | 抛 `Exception("function xxx in xxx is not implemented.")`——"用户需要实现它"（欠账） |
| `@intrinsic` | 必须为空，调用点由编译器改写为专用指令 | 绑定 C 实现（双栖：泛型/trait 分发的运行时真身） | 填 `intrinsic_not_impl` 哨兵，**静默不警告** | "编译器 bug"（哨兵） |

设计要点：

- **`@intrinsic` 的双重存在**：单态调用（静态类型已知）在编译期被改写为专用指令，函数消失；泛型 / trait 分发（如 `max[T : Comparable]` 内的比较）编译期类型未知，走 intf-table 槽位，需要 C 真身——故 builtin 值类型的比较族等天生是"编译期 intrinsic + 运行时 native"的双栖方法。
- **关键的非对称性**：`@native` 缺失必须警告——欠账要被看见；`@intrinsic` 缺失保持沉默——纯编译期存在是合法形态，不是债。
- **空 body 静默执行被结构性消灭**：任何槽位加载完成后必为可调用的实体（真实现或哨兵），不存在"空字节码悄悄返回垃圾值"的暗道。
- **哨兵与部分 trait 实现共用机制**（见 §4）：`not_impl` 同时承担"native 欠账"与"trait 未实现方法"两种占位，真实调用才 panic。
- **`@intrinsic` 为 builtin 专属**：编译器只认识内建名字的改写规则，非 builtin 模块不得声明；门禁一立，哨兵的"compiler bug"责任归属永远为真。

### 7.2 C 侧实现命名规范

`@native` 方法的 C 实现（注册进类 `MethodDef` 表的函数）统一命名为 `_类名_方法名`——前置下划线 + 类名 + 方法名，dunder 去首尾双下划线（如 `__len__` → `_list_len`），并声明 `static`，可见性限于定义文件。`kl_` 前缀保留给跨文件暴露的公共 API（`kl_new_list` / `kl_free_str` 等），不参与本规范。

样例（`src/objects/listobj.c`）：

```c
static MethodDef list_methods[] = {
    { "append", _list_append }, { "pop", _list_pop },       { "__len__", _list_len },
    { "__str__", _list_str },   { "extend", _list_extend }, { NULL },
};
```

**类型定义统一用 `DEFINE_TYPE` 宏，不手写 `TypeObject` 初始化器**：

```c
DEFINE_TYPE(str, TP_FLAGS_CLASS, 0, _str_methods);
```

展开为 `TypeObject str_type`（name / flags / priv_size / methdefs 一次配齐）。协议 dunder（`__len__` / `__getitem__` / `__contains__` 等）直接作为普通方法注册进 `MethodDef` 表，不再走独立的 SeqMethods 结构。

### 7.3 协议的统一表示：槽（slots）

**协议是概念，结构体只是实现载体之一——Koala 删掉了载体，只留概念。** Hashable / Equatable / Comparable / 算术 / 位运算 / Sequence / Mapping / Printable / Callable 这些协议依然存在，其定义就是槽 id 的命名分区（`SlotId` 枚举：`SLOT_ADD..SLOT_MOD` = 算术、`SLOT_LSHIFT..SLOT_BIT_NOT` = 位运算、`SLOT_LEN..SLOT_SET_SUBSCRIPT` = 序列/映射）加上语言侧的 dunder 名；trait 系统是这些协议的语言层投影。协议的边界本来就是方法签名的集合，用槽分区表达，概念与实现一一对应，无翻译损耗。

**实现上不再为每个协议铸造 C 结构体**：`TypeObject` 上的协议函数指针字段（`hash` / `cmp` / `str` / `call`）与 `ArithmeticMethods` / `BitwiseMethods` / `SeqMethods` / `MapMethods` 全部移除，`slots[SLOT_MAX]`（元素为 `Object*`，即 CFuncObject 或 CodeObject）成为唯一分发源，MethodDef 是唯一注册通道。`__init__` / `__fini__` 同样只是普通方法，无特殊地位。

反面对照：CPython 双轨制——`tp_as_number` / `tp_as_sequence` 等 C 槽结构 + `__dunder__` 方法并存，`typeobject.c` 靠数千行 `slot_tp_*` 蹦床同步两轨（classic classes 遗留 + C API 稳定承诺的产物）。双轨在 Koala 中还意味着按字节偏移回写协议字段的脆弱绑定机制。统一槽 = Lua（metatable 即普通值）/ JVM（vtable 统一分发）模型。槽分发与三档绑定模型自洽：协议函数指针曾是游离其外的隐藏路径，违背 nothing hidden。

**性能**：原路径两层间接（协议字段 → 蹦床 → 重复 `kl_typeof` → slots[] → 实现）变一层（slots[] → Object → 实现），热路径受益最大（dict 的 `__hash__` / `__eq__`、循环的 `__len__`）。若 profile 显示 TValue 打包仍嫌贵，逃生门是第三档 intrinsic（槽打标记、VM 走 C switch），而非退回双轨。

**槽位按热度排布**（与 vm_ops.h 指令分层同一纪律，见 §11.7；分层依据是执行频率，是槽布局自身的设计，不依赖某槽是否已有对应指令）：hot 槽（比较协议 `SLOT_EQ..SLOT_GE` + `SLOT_HASH`）占前 56 字节——一条 cache line 内，dict 探测的 hash + eq 永不越线；warm 槽居第二线：序列协议（`SLOT_LEN` / `SLOT_GET_ITEM` / `SLOT_SET_ITEM` / `SLOT_CONTAINS`）、切片协议（`SLOT_GET_SLICE` / `SLOT_SET_SLICE`）、映射下标（`SLOT_GET_SUB` / `SLOT_SET_SUB`）、算术族中最热的 `SLOT_ADD`；cold 槽殿后：`SLOT_STR`、其余算术（`SLOT_SUB..SLOT_NEG`）、位运算族（`SLOT_SHL` / `SLOT_SHR` / `SLOT_BIT_AND` / `SLOT_BIT_OR` / `SLOT_BIT_XOR` / `SLOT_BIT_NOT`）。`EQ..GE` 连续且 EQ 打头，保留 richcmp 按 `SLOT_EQ + op` 寻址的约定。槽 id 仅运行时按 dunder 名绑定，不进字节码序列化，重排不破坏 .klc 兼容。

**唯一例外**：`gc_mark` 保留为 C 函数指针——纯 GC 引擎内部回调，无 Koala 语义，不进方法表（已作为 `DEFINE_TYPE` 的参数）。C 侧快速分配器（`kl_new_bytes` 等）绕过 `__init__` 直铺内存，属设计内行为，不受影响。

### 7.4 手动泛型展开：`@specialized` 注解与擦除兜底

**泛型默认擦除；想要单态，显式注解——一个注解、两个形态、类与函数两种声明。** `list[T]` 永远是单一 `list_type`、TValue 打包存储、擦除语义，不做任何隐式特化（对照 .NET / Swift 把特化做成用户不可见的优化，违背 nothing hidden）。注解按**放置位置**分流两种行为，类与函数各得其所，合起来是 C++ 特化能力的完整版图（显式实例化 + 全特化 × 类模板 + 函数模板）——但 ODR 陷阱、声明顺序规则一个没带过来。**注解实参只允许基本类型**（int/int8/16/32/64、uint 族、float 族、bool 等基本类型），Ref 类型（用户自定义 class）不允许——特化服务于密集存储，用户类型无密集表示可换，强制走擦除路径。

```koala
@specialized(list[int64])                        // 映射：具体类认领一个实例化
pub class int64list : MutableSequence[int64] { ... }

@specialized(Foo[int], Foo[float64])             // 生成：泛型类列出要物化的实例化
class Foo[T: Arithmetic] { ... }

@specialized(int, uint, float)                    // 生成：泛型函数同理
pub func max[T: Comparable](x T, y T) T { ... }
```

| ↓ | 泛型声明上：**生成** | 具体声明上：**映射** |
|---|---|---|
| **类** | 编译器合成类（C++ 显式实例化 `template class Foo<int>;`） | 手写密集表示（C++ 全特化 `template<> class vector<bool>`） |
| **函数** | 编译器合成函数体（C++ `template int max<int>(int, int);`） | 手写算法（C++ `template<> int max<bool>(bool, bool)`） |

对称破缺一眼可见：**类侧主打映射（换表示），函数侧主打生成（换单态代码）**——函数没有表示可换。

**文法**：`annotation := '@' IDENT | '@' IDENT '(' typespec-list ')'`——逗号分隔、至少一项；单行多个实例化是重复注解的语法糖，关联唯一性按实例化逐条判定（同行重复、跨行重复一概由“第二次关联报错”接住）。实参是真 typespec，由标准解析器解析（与 `var f Foo[int]` 同一条路），编译器零拼接；`max[int64]` 无论符号是类还是函数都长得像实例化，符号种类在注解处理时才分流——文法零分支。

**关联机制（两形态共享）：注解即开关，拼写只是查询触发器；关联点是符号，不是 typespec。** 注解实参 `Foo[int]` 由标准 typespec 解析器解析成一个真实例化——编译器零拼接；注解处理时不设 origin 指针、不做任何 typespec 转化，而是把这个实例化 typespec 的 **sym_id 直接关联到具体类的符号**——Bar 这个符号由此被两个 typespec 关联（它自己的与 `Foo[int]`），二者彻底分离，唯一连接点是符号（一个符号挂多个 typespec 在 Koala 已有先例：KlassSymbol 的 `ts` 与 `instance_ts`）。此后代码中任何 `Foo[int]`（注解位、构造点、嵌套泛型如 `dict[str, list[int64]]`）走既有的 create-or-find 路径命中**同一个符号**——与直呼类名 `int64list(...)` 收敛到同一构造器、同一对象、同一 `typeof`；typespec 本身原样保留，报错与调试显示用户书写的样子。**未关联则照常实例化并擦除，是合法的普通实例化，不是错误**。类型的同一性由符号相等判定——类型系统既有判据，两个 typespec 一个符号即同一类型；决定 `Foo[int]` 含义的是可见的注解集合（可 grep、出处可在报错中指名），不是拼写本身——C++ 显式特化同款语义（`vector<bool>` 的特殊含义来自那条特化声明，而非拼写）。**推断与显式同路**：推断得到的实例化同样经此解析——`list(1, 2, 3)` 推出 T = int64 后构造的 `list[int64]` 走同一条 create-or-find 路径，命中同一符号、得到 int64list。机制对实参来源无感知（显式书写、推断所得、别处传入一律同规则）——区分来源需要额外的 provenance 追踪，违背零机制。由此闭集数值 list 在注解可见处**事实默认密集**（标准库自带注解，用户零拼写成本）；泛型体内的 `list[T]` 恒擦除的正解是：类型变量实参构不成具体实例化、无从关联——分界线是**具体 vs 类型变量**，而非显式 vs 推断。擦除兜底保留给两种情形：实参含类型变量，以及关联不存在（元素类型不在闭集、注解不可见）。

**合法性大多由既有机器代持。** `@specialized(list[int64])` 的实参是一个真 typespec：元数校验、“首个必须是泛型声明”由实例化符号创建路径原生完成；类型变量实参在注解位无作用域，符号解析自然失败。剩余 checker 规则映射形态三条——实参必须是**完整实例化**（裸 `list` 拒绝）、宿主必须是**具体声明**、同一实例化的符号关联只能建立一次（第二次关联即冲突报错并指名两个出处——唯一性检测由符号关联天然承担，无需链接期去重）。映射后实参元组成为惰性标签，成员解析一律以符号为准（具体类、非泛型），不走泛型替换路径。嵌套实参如 `Foo[list[int64]]` 因此合法且良定义：键的解析与代码中任何 typespec 同规则（内层映射先归约，键 = (Foo, int64list)），组合性免费获得。`@specialized` 是 class 上唯一的注解（`@native` 只出现在方法 / 函数上），也出现在泛型 func 声明上；klc 条目存一个可选属性即可，无注解列表机制。**注解实参限基本类型**：`@specialized(list[int64])` 合法，`@specialized(list[MyClass])` 拒绝（MyClass 是 Ref 类型，无密集表示可换，强制走擦除路径）。

**生成形态：编译器替你写 Bar。** 注解挂在泛型声明自身上，编译器对每个列出的实例化做一次 AST 替换（`T := int`），合成出它本该让你手写的具体声明，再走**现成的标准流水线**（check→irgen→isel）：`Foo[T]` 里的 `x + y` 替换后静态解析为 int 加法，isel 直落 `OP_INT_ADD`；约束（`int : Arithmetic`）在同一流水线里自然校验；生成声明显示名诚实（`Foo[int]`，内部命名与可达性见下文）——这是 C++ 显式实例化（`template class Foo<int>;`）的注解版。机制上生成与映射**共享同一台关联机器**：合成出具体类后，实例化符号的 sym_id 照映射形态的规则关联到合成类的符号——两个形态的差别只在具体声明从哪来（手写或合成）。它把“用户要写重复代码”这笔税在**编译器可改写的范围内清零**：一个 20 方法的 `Matrix[T]` 要单态，不再手抄 20 个方法；手写具体类自此只剩一种不可替代的用途——**换物理表示**（注解只做映射，不替你写存储：`int64list` 的裸 `int64_t[]` 仍要手写）。

**泛型函数：矩阵补全，机制账诚实交一笔。** 函数侧主打生成，省的不是代码行数，是 **body 里每条指令的分发**：擦除的泛型体内 `x < y` 是 itable 查找加间接调用，生成的 `min[int64]` 直落 `OP_INT_LT`，调用点本身仍是 rel32 直调——内建实参出类型化指令，用户类型走 slots，**泛型性（itable）一行不剩**。生成体完全具体，因此**不依赖擦除分发轨的成熟度**——@specialized 可作为泛型函数先行落地的路径，不必等擦除轨补齐。映射形态在函数侧退居小众位：给某一实例换算法（C++ 函数模板全特化的经典用途，如 `float32` 走特殊路径的 `max_f32`），外加唯一一条函数侧专属规则——**签名匹配**（具体函数签名 = 泛型签名替换后的形状；类侧无此条，构造器签名天然由 `__init__` 对齐）。机制成本：函数调用不产生 typespec，搭不上实例化符号缓存的便车——需要 **FuncSymbol 上一张特化表**（注解填充、klc 持久化，与既有的注解属性机制同款）加**调用解析里一个查询**：推断实参全具体 → 查表命中则直调特化符号，未命中照旧擦除泛型调用。`max(1, 2)` 推出 T = int64 → 同一查询 → 直调 `max[int64]`——provenance-blind 原样继承。查询必须长在 **checker 的共享调用解析**里（而非源码级钩子）：生成体走标准流水线，体内对其他泛型函数的调用经同一条路解析——`max[int64]` 的 body 内 `x > y` 经 `Comparable` intf-table 解析为 `OP_INT_GT`，**泛型性一行不剩**——若 body 调另一 `@specialized` 泛型函数，实参已具体同样直调特化符号，**“内层先归约”由此从嵌套 typespec 延伸到调用图**。递归自动安全（生成体的递归调用解析回自身，替换已具体）；实例化集合 = 注解集合，**编译时间有界是构造性保证**——C++ 模板爆炸的根源是自动传递物化一切，这里传递的是解析，不是物化。泛型类里的方法随类生成整体覆盖；方法自带类型参数的形态若将来出现，函数规则一字不差。

**生成物的命名与可达性：dunder 名 + pub 供链接 + magic 挡名字。** 自动生成的类与函数取名 `__xxx__`、同时 pub——但 **pub 管链接，不管访问**：职责是随模块导出、klc 持久化（magic 标记一并序列化）、跨模块关联解析可寻；挡住源码级按名引用的是 magic——表达式调用位与 typespec 位都不行（`__Foo_int__` 不能出现在类型注解里），**入口只有一个：实例化拼写**。与映射形态构成原则性不对称——映射宿主是手写一等公民（正常名，类名与拼写两个入口），生成物是编译器产物（单一入口）；想要可命名的类就自己写，用户的依赖面收敛为“自己的拼写与自己的注解”，编译器命名方案因此不构成公共 API。两态测试随之更干净：注解关掉时符号根本不存在，开着时不可名——开关任何一态都无法按名依赖生成物。名字从来不是查找键（关联按 sym_id、特化表按（泛型，实参）），**mangling 防碰撞因此不是正确性约束**：两个模块各自的 Foo 各生成 `__Foo_int__` 互不相扰（各自 symtab，永不按名合并）。命名分两层——内部名 `__xxx__`（symtab / klc 标识，裸工具可见，同 nm 看 C++ mangled 名），显示名 = 实例化拼写（`Foo[int]`，typeof 与诊断用，名字诚实）。

**magic 按符号划界，不按名字形状。** 生成物的不可调用来自 magic 标记，而非 dunder 拼写——直接调用禁令的适用对象始终是三类符号：内置运算符钩子（§3 的语法糖专用族）、内置 magic 函数（`len` / `hash` 等 intrinsic）、@specialized 自动生成的类与函数。**用户自写的 `__foo__` 是合法声明、普通可调用的符号，不做任何拒绝**——dunder 名字形状本身不构成 magic。先例同款：JVM synthetic 成员在 class 文件里而不在源语言里，C++ 显式实例化符号有外部链接而 mangled 名用户写不出；Koala 用 dunder 当 synthetic 标记还有一层便宜——`__xxx__` 本就按 IDENT 正常 lex，lexer 零改动。

**密集类是普通类，不是新的语言实体。** 可直接构造（`int64list(1, 2, 3, x)`）、可注解、可导入、`typeof` 诚实返回 `"int64list"`；删掉注解，坏掉的只是 `list[int64]` 这个拼写，类本身毫发无损——部署顺序因此是"类先行，映射后补"。对照：Java `IntStream` 家族是擦除默认 + 手写特化但无映射语法（Valhalla 想补的正是这块）；C++ `vector<bool>` 特化换表示但同名静默坑人——Koala 用显式注解 + 独立类名，替换关系全程可 grep。

**映射随 pub 声明的 import 全局生效。** 凡能见到泛型声明的地方，见到的是同一个映射（依赖模块先行加载，其关联先就位——类的 typespec→符号、函数的特化表同批）；两个 pub 声明 claim 同一实例化在 checker 期即冲突报错并指名两个出处——标准库先占位，对 `list[int64]` 的劫持自动被堵死；第三方给未被认领的泛型做映射合法（Julia 式开放），风险由重复检测兜住。

**兄弟类架构由实例布局钉死，继承被禁止。** C 方法经 `SELF_AS` 直读实例布局，一个 TypeObject 只能对应一种物理表示——裸 `int64_t[]` 与 `TValue[]` 是两种布局，必然两个 TypeObject；若密集类继承 list，list 的 C 方法会把裸数组当 TValue 数组读，内存破坏。互操作只走 trait 边界：两种表示都实现 `MutableSequence[Elem]`，`int64list.extend(list(1, 2))` 在 `Iterable[int64]` 边界上逐元素转换（裸 ↔ TValue——这是位宽转换，非 §2.5 所拒绝的堆装箱：TValue 本就内联携带值，密集类消除的是 16 B → 8 B 的位宽浪费）；泛型代码 `func f[T](l list[T])` 永远只吃擦除的 list，想同时吃两种表示，参数写 `Sequence[T]`。副作用是正面的：API 被推向 trait 参数风格。

**首期三件套**（bytebuf 模式：`DEFINE_TYPE`、gc_mark = NULL、注册 builtin.c 类型表、方法按 trait 声明顺序全量 `@native` 空体声明）：

| 类 | 存储 | 密度 |
|----|------|------|
| `int64list` | 裸 `int64_t[]` | 8 B/元素，TValue 打包的一半 |
| `float64list` | 裸 `double[]` | 8 B/元素 |
| `boollist` | 位压缩 `uint64_t[]` | 1 bit/元素，TValue 打包的 1/128 |

`boollist` 的窗口以比特为单位：start / end 是比特下标，pop-head 依旧 `start++` 不搬移；`__contains__(true)` 可按字 OR 扫描，一次跳 64 个元素。三件套与 `ByteBuf`（`MutableSequence[uint8]`）同属密集容器家族：各成一等类、显式选择存储，`list[T]` 永远是泛型兜底；ndarray 将来是同族成员（自带对齐存储，list 不为 SIMD 预付设计）。闭集按需生长——加一个特化 = 写一个类 + 一行注解，语言核心零改动（`float32` 大概率是第四行）。

**诚实弱点**（接受而非掩盖）：双名同步（注解与类名共同维护，注解可 grep 缓解）；赋值悬崖大幅收缩（推断同路后 `let l list[int64] = list(1, 2, 3)` 两侧落同一符号，合法——残余悬崖只剩未注解空容器：`let l = list()` 无从推断 T，保持擦除，之后再赋给 `list[int64]` 报错；修法是声明处注解，注解驱动的推断同样走关联）；无结构一致性检查（密集类漏声明某 trait 方法，用到才报——v1 靠约定对齐 list 的 API 面，v2 可选 checker pass 比对签名）；生成体的报错映射（编译失败须定位回泛型源码并附 “in instantiation max[int64]” 上下文——C++ 模板报错之痛的微缩版，但有界：只发生在被注解的实例化上，永不扩散到传递闭包）。

落地顺序：注解文法 `@ident(typespec-list)`（实参即实例化 typespec，复用标准解析器，零拼接）→ checker 在注解处理时将实例化 typespec 的 sym_id 关联到宿主符号（映射形态校验：完整实例化、宿主具体声明、关联唯一、实参符号可见性不低于宿主；函数映射形态加签名匹配）→ 生成 pass（泛型宿主：克隆声明 AST、替换类型变量、过标准流水线——类与函数同一个 pass，操作对象从 ClassDecl 换成 FuncDecl；产物 dunder 命名、pub 仅供链接、magic 挡源码引用）→ FuncSymbol 特化表 + 共享调用解析里的查询（传递归约的落点）→ klc 持久化 → 实例化解析处符号即映射（create-or-find 既有路径，零转化；成员解析一律以符号为准）→ 三件套实现 → 两态测试（`test_specialize.kl`：同一文件注解开 / 关都编译通过且行为一致，唯一可观察差异是 `typeof`，含推断用例）。运行期与 VM 零改动——关联完成后 `list[int64]` 的符号就是普通 klass，走现成的类型调用机器；生成的 `max[int64]` 是普通函数，走现成的直调机器。

---

## 8. 标准库设计（libs/std）

### 8.1 透明核心

**所有内建类型都在标准库的 `.kl` 源文件中声明**（libs/std/builtin/*.kl）——对比 Go（编译器魔法）、Rust（lang items）、Java（原始类型特殊化），Koala 语言核心零暗角。C 后端实现位于 libs/std/native/。

### 8.2 Trait 层

| Trait | 职责 |
|-------|------|
| `any` | 空根（自动遵循三契约的载体） |
| `Equatable[T]` / `Comparable[T]` / `Hashable` / `Printable` | 值契约 |
| `Arithmetic[T]` / `BitwiseOperators[T]` | 算术 / 位运算能力契约（泛型约束用；结构式遵循；见 §3） |
| `Iterable` / `Iterator` | 迭代协议 |
| `Sequence` / `MutableSequence` | 序列与可变序列协议 |
| `Map` / `Set` | 映射与集合协议 |

命名分层原则：**trait 层用跨语言惯例**（Java/Rust 风格的 `remove` / `remove_or`），**class 便利层对齐 Python**（`setdefault` / `update` / `fromkeys`）。

### 8.3 容器协议

Koala 容器 trait 遵循**有度扁平**（LW-OOP）设计原则：不学 Java/Kotlin/Swift/Python 的 5–6 层深塔，也不学 Go/Rust 的零层次平面。层级结构：

```
Iterable[T]
├── Sequence[T]
│   └── MutableSequence[T]
├── Map[K, V]
└── Set[T]
```

每层有独立语义贡献，无装饰性中间层（如 Java `AbstractList`）。

| Trait | 继承 | 方法数 | 核心语义 |
|-------|------|--------|----------|
| `Iterable[T]` | — | 1 | 遍历（`__iter__()` → `Iterator[T]`） |
| `Sequence[T]` | `Iterable[T]` | 7 | 只读序列：长度、成员判定、下标访问、切片、搜索 |
| `MutableSequence[T]` | `Sequence[T]` | 9 | 可变序列：写入、追加、插入、删除、清空、反转 |
| `Map[K, V]` | `Iterable[(K, V)]` | 10 | 键值映射：下标读写、视图、安全读取、删除 |
| `Set[T]` | `Iterable[T]` | 11 | 数学集合：增删、批量添加、代数运算、子集判定 |

**设计原则**：

- trait 之间可以继承，class 都是 final 的（不可继承覆写）。
- 每层 trait 必须有独立语义贡献，不做纯装饰层。
- 扁平化要有度——介于 Java/Python 式深塔与 Go/Rust 式零层次之间。
- 易用性第一，对齐 Python 语义直觉：所有容器方法都放在 trait 层，用户期望直接能调用。

**Sequence[T]**（7 方法）：

| 方法 | 签名 | 说明 |
|------|------|------|
| `__len__` | `() int` | 元素数量，O(1) |
| `__contains__` | `(item T) bool` | 成员判定，支撑 `in` 语法 |
| `__getitem__` | `(index int) T` | 下标访问，支撑 `a[i]` 语法；越界 panic |
| `__getslice__` | `(r slice) Sequence[T]` | 切片访问，支撑 `a[i:j]` 语法 |
| `index` | `(value T, start=0, end=-1) int` | 首次出现位置，未找到返回 -1 |
| `rindex` | `(value T, start=0, end=-1) int` | 末次出现位置，未找到返回 -1 |
| `count` | `(value T, start=0, end=-1) int` | 出现次数 |

实现者：`str`、`list`、`tuple`、`bytes`、`range`。

**MutableSequence[T]**（9 方法，继承 Sequence 全部方法）：

| 方法 | 签名 | 说明 |
|------|------|------|
| `__setitem__` | `(index int, value T)` | 下标赋值，支撑 `a[i] = v` |
| `__setslice__` | `(r slice, val Iterable[T])` | 切片赋值，支撑 `a[i:j] = [...]` |
| `push` | `(value T)` | 追加到末尾 |
| `extend` | `(items Iterable[T])` | 批量追加 |
| `insert` | `(index int, value T)` | 指定位置插入 |
| `remove` | `(value T)` | 删除首次出现，缺失 panic |
| `pop` | `(index = -1) T` | 按下标弹出并返回，默认末尾 |
| `clear` | `()` | 清空 |
| `reverse` | `()` | 原地反转 |

实现者：`list`、`ByteBuf`。

**Map[K, V]**（10 方法）：

| 方法 | 签名 | 说明 |
|------|------|------|
| `__len__` | `() int` | 键值对数量 |
| `__contains__` | `(key K) bool` | 按键判定（覆写 Iterable 的 `(K, V)` 检查，同 Python `key in dict`） |
| `__getsub__` | `(key K) V` | 按键读取，支撑 `m[key]`；缺失 panic |
| `__setsub__` | `(key K, value V)` | 按键写入，支撑 `m[key] = v` |
| `keys` | `() Sequence[K]` | 键视图 |
| `values` | `() Sequence[V]` | 值视图 |
| `items` | `() Sequence[(K, V)]` | 键值对视图 |
| `clear` | `()` | 清空 |
| `remove` | `(key K) V` | 删键返值，缺失 panic |
| `remove_or` | `(key K, default_value V) V` | 删键返值，缺失返回默认值 |
| `get` | `(key K) V?` | 安全读取，返回可选类型，配合 `if let` |
| `get_or` | `(key K, default_value V) V` | 安全读取，缺失返回默认值 |

实现者：`dict`（插入序哈希映射）。

**Set[T]**（11 方法）：

| 方法 | 签名 | 说明 |
|------|------|------|
| `__len__` | `() int` | 元素数量 |
| `__contains__` | `(v T) bool` | 成员判定，支撑 `in` 语法 |
| `add` | `(v T) bool` | 添加元素，返回 true 表示新插入 |
| `update` | `(items Iterable[T])` | 批量添加 |
| `remove` | `(v T) bool` | 删除元素，返回 true 表示曾存在 |
| `clear` | `()` | 清空 |
| `is_subset` | `(other Iterable[T]) bool` | 子集判定 |
| `is_superset` | `(other Iterable[T]) bool` | 超集判定 |
| `union` | `(other Iterable[T]) Set[T]` | 并集，返回新集合 |
| `intersect` | `(other Iterable[T]) Set[T]` | 交集，返回新集合 |
| `diff` | `(other Iterable[T]) Set[T]` | 差集，返回新集合 |

实现者：`HashSet`（哈希集）、`TreeSet`（有序集）。

### 8.4 类型清单

| 类型 | 要点 |
|------|------|
| `str` | 字符串；`str(obj)` 接受 `any` 经 `__str__()` 做通用字符串转换 |
| `bytes` | 固定长度字节数组，24 方法；位置式写族 |
| `ByteBuf` | 可变字节缓冲，32 方法；append 式写族 |
| `list` | 动态数组（泛型擦除，TValue 打包存储；显式 `list[int64]` 等拼写经 `@specialized` 映射到密集类，见 §7.4） |
| `int64list` / `float64list` / `boollist` | 密集容器三件套（§7.4）：裸 int64 / 裸 double / 位压缩 bool，普通一等类 |
| `dict` | **插入序**哈希映射；Python 便利方法 + Rust 位置族（pop_first / peek_last）+ Java remove 命名 |
| `HashSet` / `TreeSet` | 集合并代数 |
| `tuple` | 不可变元组 |
| `range` | 区间（编译器将 `range(5)` 补全为 `range(0, 5)`） |
| `slice` | 切片（边界由编译器补全为具体 int，统一用 `end` 表排他上界） |

bytes / ByteBuf 的二进制编解码职责已**剥离至官方库 encoding**，本体只保留序列语义。

### 8.5 迭代

- for 循环直接接受 Iterator，支持元组解包。
- **内建容器原生展开**：for 对 range / tuple / list / dict / bytes 等内建类型做原生展开——尤其 `range` 循环**不创建 range 对象、不走 iterator 协议**，编译器直接生成循环体，热路径零分配零分发。
- 顶层组合子：`enumerate` / `zip` 现役；`filter` / `map` / `reduce` 在路线图（注释状态）。
- **明确不做**：`sum` / `any` / `all` / `sorted`（作者定案：简洁优先）。`min` / `max` 已实现为泛型函数 `min[T: Comparable]` / `max[T: Comparable]`，`@specialized(int, uint, float)` 生成单态（见 §7.4）。
- view 语义只用于固定长度类型，动态容器用 copy。

### 8.6 包编程规范

- **简单包用单文件**：包内容简单时，一个 `xxx.kl` 文件即可表示（如 `assert.kl`、`pretty.kl`）。
- **复杂包用目录**：目录名即包名（如 `io/`、`fs/`、`builtin/`），内部结构遵循两条规则：
  - `__<包名>__.kl` 为模块入口，**只放** `link` 语句、全局变量和顶层函数定义，**不放** class / trait 定义。
  - 每个 class / trait 建议单独定义在一个 `xxx.kl` 中，一个类型一个文件（如 `str.kl`、`reader.kl`、`any.kl`）。
- 效果：看文件名即知内容——"没有隐藏"在文件组织层面的投影。

### 8.7 文档注释规范

Koala 文档注释采用 Rust 风格 `///`，由 `tools/kl-doc.py` 提取生成 Markdown API 文档，规范如下：

**语法与绑定**

- `///` 每行一条，紧贴其描述的声明；`//` 为普通注释，`/* */` 为块注释（版权头等），均不被提取。
- 文档注释与声明之间允许 `@native` / `@intrinsic` 等注解行；**空行断开绑定**——注释后不得留空行再写声明。

**覆盖范围**

- **所有符号都要有文档注释**：顶层与成员的 class / trait / func，不论是否 `pub`。
- **所有符号都出现在生成的文档中**，无论是否写了注释——无注释者签名录入、说明为空，工具不隐藏任何符号。

**结构**

- 首行摘要：第三人称、动词开头（Return / Parse / Initialize…）、句号结尾，一句话说清"做什么"。
- 摘要与正文、正文各段之间用空 `///` 行分隔。
- **换行只由空行决定**：连续非空行属同一段落，源码折行无排版语义（工具合并为单行）。

**标签行及特殊渲染**

标签行独占一段（前后空行），首字母大写、冒号结尾：

| 标签 | 用途 | 渲染 |
|------|------|------|
| `Example:` | 用法示例 | 其后必须跟缩进 4 空格的代码行，渲染为独立 `kl` 代码块 |
| `Note:` / `Notes:` | 补充说明 | 独立段落；多条用 `- ` 列表 |
| `Precondition:` | 前置条件 | 独立段落 |
| `Panics:` | panic 条件集中说明 | 独立段落或 `- ` 列表（简单情形可用整句 `Panic if ...` 替代） |
| `See:` | 相关符号交叉引用 | 引用名写在反引号中、逗号分隔；顶层符号自动转锚点链接 |

未定义的标签行按普通段落处理；`Example:` 是唯一强制要求后续代码块的标签。

**内容要求**

- 错误语义必须写明：panic 条件、返回 `-1` / `null` 的含义、`_or(default)` 变体的区别。
- 区间语义写明半开区间 `[start, end)` 与 `end = -1` 的含义。
- 必要时标注复杂度（O(1) / O(n)）。

**行内格式**

- 标识符、参数名、类型、方法名、运算符表达式用反引号（`` `sep` ``、`` `__str__` ``、``(`self == other`)``）。
- 引用参数一律用反引号名字，不用位置性描述。

**工具链**

- `tools/kl-doc.py` 按本规范提取生成 Markdown：标签行加粗，`Example:` 代码独立成块，`See:` 顶层符号自动链接；生成文档按 Traits / Classes / Functions 分类，Contents 表格含名称与摘要。
- 写注释不得依赖本规范之外的排版技巧。

### 8.8 与 Python 内置函数 / 类对照

以 Python `builtins` 为基准逐项比对，标注 Koala 现状与决策。

**已有**

| Python | Koala | 备注 |
|--------|-------|------|
| `print()` | `print()` | `@native`，支持 `sep` / `end` |
| `len()` | `len()` | `@intrinsic` |
| `hash()` | `hash()` | `@intrinsic` |
| `str()` | `str(any)` | 经 `__str__()` 通用字符串转换 |
| `type()` | `typeof()` | 改名避免冲突 |
| `min()` / `max()` | `min()` / `max()` | `@specialized` 泛型 |
| `format()` | `format()` | `@native` |
| `range()` | `range` 类 | 编译器补全 |
| `slice()` | `slice` 类 | 编译器补全 |
| `set()` | `HashSet` / `TreeSet` | — |
| `list()` | `list(args ...T)` | 变长参数构造，不支持 `list(iterable)` 形式 |
| `dict()` | `dict()` | — |
| `tuple()` | `tuple(args ...T)` | — |
| `classmethod()` / `staticmethod()` | `static` 关键字 | 语言级替代 |
| `open()` | `fs.open` | 归 fs 包 |
| `object` | `any` trait | 拆解为 any + 三契约 |
| `Exception` 体系 | 单一 `Exception` | panic 不可捕获，无需层级 |
| `abs()` | `int64.abs()` / `float64.abs()` | 方法而非顶层函数 |
| `round()` | `float64.round()` | 方法而非顶层函数 |
| `pow()` | `int64.pow()` / `uint64.pow()` / `float64.pow()` | 方法而非顶层函数 |

**明确要补**

| 函数 | 说明 |
|------|------|
| `enumerate()` | 已规划，for 循环高频用法 |
| `zip()` | 已规划，多序列并行迭代 |
| `hex()` / `oct()` / `bin()` | 整数进制格式化 |
| `input()` | 标准输入读取 |
| `callable()` | 判断对象是否可调用（反射 API） |

**暂时没有（反射 / 运行时编译，将来设计）**

| 类别 | 项目 |
|------|------|
| 反射 | `globals()` / `locals()` / `dir()` / `vars()` |
| 反射 | `isinstance()` / `issubclass()` |
| 反射 | `hasattr()` / `getattr()` / `setattr()` / `delattr()` |
| 运行时编译 | `exec()` / `eval()` / `compile()` |
| 动态导入 | `__import__()` |

**明确不要**

| 项目 | 理由 |
|------|------|
| `repr()` | 不区分 repr/str，`__str__()` 统一承担 |
| `property()` | `pub let` / `pub func` 直截了当 |
| `memoryview` | view 只用于固定长度类型，动态容器用 copy |
| `frozenset()` | 无 hashable 容器需求 |
| `super()` | Koala 不需要显式调用父类方法语法 |
| `breakpoint()` | 调试器入口，Koala 无此机制 |
| `help()` | 交互式文档，Koala 用生成式文档工具 |
| `any()` / `all()` | 依赖隐式 truthiness 判定，Koala 不支持 bool 运算符重载（`__bool__`），无法泛型化；主流静态语言均不提供 |
| `bool()` | 同上，隐式 truthiness 转换机制不存在 |

**不确定**

| 项目 | Python 用途 |
|------|-------------|
| `sum()` / `sorted()` | 集合聚合 / 排序 |
| `reduce()` | 累积归约 |
| `map()` / `filter()` | 函数式组合子 |
| `divmod()` | 返回 (商, 余) 元组 |
| `reversed()` 顶层 | 各类型已有 `.reversed()` 方法，顶层是否冗余 |
| `id()` | 返回对象内存地址（身份标识） |
| `chr()` / `ord()` | 字符 ↔ code point 互转 |
| `complex` | 复数类型 |
| `iter()` / `next()` 顶层 | 手动迭代协议 |
| `aiter()` / `anext()` | 异步迭代协议，Koala 暂无 async 迭代 |
| `ascii()` | 返回 ASCII 可打印表示，非 ASCII 转义 |
| `bytes()` | 不可变字节构造，Koala `bytes` 类尚无通用构造器 |

**已有对应（方法形式）**

| Python 构造器 | Koala 方法 | 备注 |
|---|---|---|
| `int(x)` | `str.to_int()` / `float64.to_int()` | 方法而非构造器 |
| `float(x)` | `str.to_float()` / `int64.to_float()` | 方法而非构造器 |
| `bytearray()` | `ByteBuf` | 可变字节缓冲 |

---

## 9. 官方发布库（libs/koala）

区别于标准库的官方发布库，编码编解码的归属地：

| 模块 | 内容 |
|------|------|
| `encoding/bin` | 52 个函数：be/le 全展开的 u16/u32/u64/i16/i32/i64/f32/f64 读写三族（read_* / write_* / buf_write_*）+ LEB128（read_leb_u64 等，i64 用 zigzag） |
| `encoding/base64` | RFC 4648 标准与 URL 变体，encode / decode / encode_url / decode_url |
| `pretty` | 美化输出 |

设计取舍：学 Go 的 encoding 分包布局，但拒绝 Go 的格式串 DSL（静态类型下正确取舍）；拒绝 base32/ascii85/pem/asn1/gob（用得少）；hex 为下一个自然成员。

---

## 10. 性能现状

**解释器裸跑**（JIT 未上线）即取得以下战绩（hyperfine，mean ± σ）：

| 基准 | Koala | Lua | LuaJIT(-joff) | vs Lua | vs LuaJIT |
|------|-------|-----|---------------|--------|-----------|
| sum | **565.6ms** | 1074ms | 956.4ms | 1.90× | **1.69×** |
| loop-sum | 270.5ms | 298.2ms | 247.7ms | 1.10× | 0.92× |
| fib | **2.927s** | 4.208s | 3.242s | 1.44× | **1.11×** |
| ack | 556.8ms | 551.6ms | 385.5ms | 持平 | 0.69× |
| tak | 413.7ms | 581.5ms | 356.1ms | 1.41× | 0.86× |
| shuffle | **140.7ms** | 653.5ms | 461.6ms | **4.64×** | **3.28×** |
| logic_chain | **1.114s** | 1.857s | 1.222s | 1.67× | **1.10×** |
| float_arith | **254.5ms** | 539.5ms | 343.0ms | **2.12×** | **1.35×** |

11 项基准赢 9 项，其中 6 项连 LuaJIT 的解释器内核一起赢。静态类型是未来 JIT 的免费弹药：类型特化、去虚化（intf-table 槽位内联）、越界检查消除无需猜测与守卫——性能天花板在结构上高于动态语言的 JIT。

---

## 11. 编译器与优化器实现

编译器 `koalac` 是一个完整的、LLVM 架构风格的多阶段编译器（C 实现），不是"AST 直出字节码"的脚本编译器。

### 11.1 编译流水线

```
.kl 源码
  → 词法/语法分析（flex + bison）          → AST
  → 符号/类型检查（symbol.c / typespec.c）
  → intf-table 构建                        → trait 分发表
  → IR 生成（irgen.c）                     → KLR IR（类型化、use-def 链、CFG）
  → 优化（opt.c，PassManager）             → 常量/复制传播、CFG 清理、DCE
  → SSA 构造 → SCCP → SSA 析构（phi 合并） → SSA 级优化
  → 指令选择（isel.c）                     → 类型特化操作码（reg-reg / reg-imm 双形态）
  → 线性扫描寄存器分配（lsra.c，LIR）
  → 代码生成（cgen.c）
  → .klc 产出（write_klc.c；read_klc.c 反向加载）
```

每个阶段可用命令行独立开关启用（--irgen / --ssa / --opt / --isel / --lsra / --cgen），并提供逐级 dump（no-opt-ir / ssa / ir / lir / vreg / code / itable）——与"无隐藏特性"原则一脉相承：**编译器的每一步中间产物都可观察**。

### 11.2 IR 设计（KLR）

- 每个值携带 TypeSpec 类型、use/def 双链、虚拟寄存器号、源码位置（诊断回溯）。
- 基本块 + 边构成 CFG，函数级独立优化。
- 值种类覆盖常量、全局、函数、块、参数、指令、类、字段、trait、intf、外部模块符号（FFI 对接）。

### 11.3 优化器

- **PassManager 框架**：pass 可嵌套成组（嵌套 PM 作为一个 pass），不动点迭代直至无变化。
- **经典优化全家桶**：
  - 常量与复制传播（const-copy-prop）
  - CFG 优化组：删除仅跳转块、分支折叠、删除无用块、块合并
  - 死代码消除（DCE）
  - **SSA 构造 + SCCP**（稀疏条件常量传播：TOP/常量/BOTTOM 三值格 + meet 运算 + 双 worklist，教科书级实现）+ SSA 析构（phi 合并）
- **指令选择**：表驱动规则把泛型二元运算（OP_BINARY_*）降级为类型特化操作码（OP_INT_ADD / OP_UINT_DIV …），并支持 reg-imm 立即数形态与交换律换序——与 CPython 3.11 特化自适应解释器殊途同归，而 Koala 在编译期一次完成。
- **寄存器分配**：线性扫描（LSRA）——活跃区间分析 + 空闲寄存器位集，另有 simple regalloc 后备路径。
- **IR 级另有三招**：指令融合（--fusion）、尾调用优化（--tail-call）、窥孔优化（peephole）——均带编译开关与对应回归测试。

### 11.4 与语言设计的呼应

- **trap/wrap 溢出机制**直接暴露为编译开关（--int-trap / --float-trap）。
- **intf-table** 在编译期构建（--dump=itable 可观察），支撑运行时的零查找 trait 分发。
- 类型化 IR 让静态类型的信息一路保留到操作码选择——**这正是未来 JIT 的免费弹药**（类型特化、去虚化在编译期已有先例）。

### 11.5 实现体量

编译器核心约 3.1 万行 C（parser/irgen/opt/backend 合计），含 SCCP、SSA、LSRA 这类通常只出现在 LLVM/专业 JIT 项目中的组件——以个人项目论，这是专业编译器团队级的规格。

### 11.6 测试体系

- **基建**：采用 LLVM 项目的 lit + FileCheck 工业标准（lit.cfg / ShTest / RUN 指令），test/ 下 139 个测试、2800+ 行 CHECK 断言。
- **三层金字塔**：
  - `test-kl`（30）——前端语言特性（cast、slice、if-let、包管理…）
  - `test-ir`（35）——优化器逐 pass 验证（ssa、sccp、isel、lsra、fusion、tailcall…）
  - `test-run`（73）——端到端运行（泛型、链表/树数据结构、LRO、intf 调用、IO…）
- **观察面全覆盖**：RUN 行覆盖 no-opt-ir / ssa / ir / lir / vreg / code / itable 全部 7 个 dump 阶段——"无隐藏特性"的工程回响：每个可观察阶段都有断言守护。
- **负向断言文化**：优化器测试大量使用 CHECK-NOT 守护"不过度优化"（如不同常量的 phi 必须保留、死分支常量必须清除），测试的是正确性边界而非仅优化效果——LLVM 测试文化的核心实践。
- **诊断与开关回归**：编译器错误消息有专门的 `2>&1` 回归测试；--int-trap / --float-trap / --fusion / --tail-call 每个编译开关均有对应测试。

### 11.7 VM 指令布局：按执行频率物理排序

`vm_ops.h` 的指令 TARGET 不按语义分组、按频率分层物理排布，源码注释即分层标记：

- **hot**（文件头）：`OP_MOVE` / `OP_LOADK` / `OP_LOAD_INT_IMM`、整数 `ADD`/`SUB`（含 IMM 与 uint 变体）、整数序比较跳转 `OP_JMP_INT_LE/GT/LT/GE`、`OP_RET` 族——寄存器搬运、循环计数算术、循环回边分支。
- **warm**：逻辑分支（EQ/NE、ref-null 判断）、`OP_CALL` / `OP_TAIL_CALL`、字段存取、`OP_NEW`、int 全序比较、位运算与逻辑短路、复杂整数算术（MUL/DIV/MOD）、`OP_NUM_*` 泛型数值、`OP_SEQ_*`、float 基本运算与跳转、intf 构造/上转。
- **cold**（文件尾）：uint 完整族、global 存取、float 复杂族（DIV/MOD/CMP）、misc（NEG/NOT/LOAD_TAG）、类型转换、`OP_NOP`。

同一语义被热度拆开：`OP_INT_ADD` 在 hot 段、`OP_INT_MUL/DIV` 降到 warm、`OP_INT_NEG` 落进 misc——排布依据是 profile 频率，不是指令族谱。收益：handler 代码热段聚拢，提升 icache 命中；computed-goto 跳转表目标地址集中，利于分支预测器与取指预取；源码注释（hot/warm/cold）让分层意图可审计、可重排。

同一纪律推广到运行时分发表：`TypeObject.slots[]` 的 `SlotId` 同样按热度分区（见 §7.3）——指令布局与槽布局共用一条设计原则：**把执行频率最高的入口放进第一条 cache line**。

---

## 12. 半自动内存管理（Semi-automatic Memory Management）

作者正式公布的内存管理模型，名为**半自动**——确定性释放为主，收集器备而不用：

- **显式 `free`**：程序员可对对象调用，在生命周期已知时主动提前回收。
- **编译器自动插入 free**：生命周期不言自明处由编译器代劳——
  - for 循环内产生的短命临时对象；
  - 变参入参自动装箱成的 tuple 对象，调用结束后自动释放（隐藏分配与隐藏释放**对称配对**，程序员永不需要跟踪）。
- **逃逸分析 + `noesc` 标记**：控制对象的堆逃逸，未逃逸对象免于 GC 管辖。
- **CMS 收集器备而不用**：并发标记清除实现已在仓库中但**未启用**——主通道是确定性释放，GC 作为逃生阀存在，且其状态公开透明。
- **设计动机**：热循环中大量短命对象**当场回收**，不堆积、不停顿——GC 压力在源头溶解。对比 GC 语言热循环里短命对象只能攒够压力再回收并付停顿，Koala 是"平稳低水位"对"锯齿状压力曲线"。
- **坐标系定位**：Rust/C++ 有确定性但仪式重（所有权/析构），Go/Java 零仪式但非确定（STW、堆积），Swift ARC 居中但有引用计数开销与循环问题；Koala 落在**确定性 + 低仪式**的无人区——编译器把 free 插在不需思考的地方，程序员只在想提前回收时出手。
- 与其他机制的咬合：无栈协程的帧是堆上精确对象、生命周期清晰，走半自动通道；shadowstack 兜住 FFI 侧的 C 对象根系。

---

## 13. 未公开 / 进行中

作者确认存在且独一份，细节未完全公开，本文档不做推测：

- **并发**——核心已揭晓：**无栈协程**（编译器状态机变换，非 Go 式有栈协程；GC 使其免除 Rust 式 Pin 仪式），调度与 API 细节待公开
- **JIT**——尚未上线，编译器后端的类型化 IR 已备好弹药

---

## 附录：跨语言定位

| 维度 | Koala 位置 |
|------|-----------|
| 语义直觉 | Python（插入序 dict、dunder、enumerate/zip、便利方法层） |
| 契约机制 | Rust 系（intf-table、trait 约束），无生命周期 / unsafe 负担 |
| 泛型表示 | 擦除兜底 + `@specialized` 手动展开（映射换表示 / 生成换单态，类与函数双声明）：C++ 特化的完整版图、Java 擦除拿不到的出口；无 .NET / Valhalla 运行期泛型机器——生成只是同一份源码按注解再过一遍标准流水线 |
| 错误处理 | C 的返回值 + 极简 panic；拒 Java try-catch、拒 Go defer |
| 工程布局 | 学 Go（encoding 分包），避 Go 之短（cgo、无重载） |
| 对象模型 | 超越 Java（无 Object 神类，根契约零 boilerplate）与 Kotlin（自动遵循免声明） |
| 数值 | trap/wrap 溢出与 Swift `&+` 系、Zig 殊途同归 |
| Null 安全与值表示 | T? 可选 + 分支窄化（Kotlin/Swift 同级）；**零装箱在"普适 any + 泛型"语言中稀缺** |
| 内存管理 | 半自动：free + 编译器自动插入 + noesc，CMS 备而不用；落在"确定性 + 低仪式"无人区 |
| 并发 | 无栈协程（编译器变换，GC 免 Pin），非 Go 有栈路线 |
| FFI | 无 unsafe 双层兑现：类型层编译器桥接 + 内存层 shadowstack 纳管 C 对象 |
| 执行速度 | 解释器越过 Lua 全场、多数越过 LuaJIT(-joff)，JIT 未发力 |

**一句话总结**：Koala 用白板设计的自由，把 Python 的语义、Rust 的契约、C 的错误哲学、半自动的内存管理焊成了零后门、零仪式的整体，并以解释器裸跑的性能数据证明了这条路线的执行力。
