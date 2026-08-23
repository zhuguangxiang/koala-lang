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

  函数体内 `a + b` 的操作数类型是未知类型 T，编译器不知道具体实现，但因 `T : Arithmetic` 约束而知道"加法可用"：IR 下降为协议 `add` 指令，后端转为 `num.add`（字节码 `OP_NUM_ADD`，数值协议指令），目标运行时经 Arithmetic intf-table O(1) 分发到 T 的 `__add__`，全程不创建 call frame。当前实现状态（2026-08 实测）：**十六件二元运算符的 IR 下降链路已全部打通**——IR 协议指令（`add/sub/mul/div/mod`、`shl/shr/and/or/xor`、`cmplt/cmple/cmpgt/cmpge/cmpeq/cmpne`）经优化器后保留，isel `num_ops_rules[]` 十六条 `OP_BINARY_* → OP_NUM_*` 映射齐备，寄存器分配后存活到 LIR；**比较族六件 VM handler 已落地，`test_generic_9` 转绿**。尚不支持：泛型一元 `-` / `~`、泛型复合赋值（`+=` 等）、VM 侧算术与位运算 `OP_NUM_*` handler 族——详见 `docs/Koala_TODO.md`。

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

  **禁令对接收者形态无差别**：具体类型、trait 值（如 `Arithmetic[int]`）、泛型类型参数（`T : Arithmetic`）三种形态的显式 dunder 调用报同一条禁令错误。泛型参数的成员解析经 bound 完成——bound 是 T 的成员唯一事实源，运算符下降与点号成员访问共用同一条 bound 查找；正因 bound 承诺了方法存在，T 上的显式调用报的是"禁止"而非"方法不存在"，拒绝理由是规则而不是能力缺失。禁令名单之外的协议钩子（如 `T : Hashable` 的 `__hash__`、`T : Collection` 的 `__len__`）经 bound 解析后按普通成员调用，正常可用。

---

## 4. Trait 与分发机制

- **intf-table**：Rust 风格的每类 trait 表，**O(1) 接口分发、无需查找**——槽位含方法名 + code_index，trait 调用 = 槽位一次间接调用；class 方法调用 = 编译器查参后的**直接调用，零开销**。
- **trait 值表示（intf 值）**：具体类型扩宽为 trait 类型的值（如 `let e Equatable[bool] = false`），TValue 首字携带该类的 intf-table 指针，载荷原样保留。**扩宽是单向的**：primitive（int/float/bool 等）cast 到 trait 后原 tag 即告丢失，不可 downcast 回具体类型（对照：`any` 通道保留 tag，支持 `isinstance` 窄化）。每个 intf-table 带一个回指实现类 TypeObject 的指针，供运行时自描述（`typeof` / `print` / GC 标记）——这是运行时内省设施，不是面向用户的 downcast 通道。
- **部分 trait 实现**：class 遵循 trait 时允许只实现其中一部分方法。未实现的方法在 intf-table 中填入 `not_impl` 占位对象，加载期仅警告、不阻断；只有当未实现的方法被真实调用时抛异常 `Exception("function xxx in xxx is not implemented.")`——语义是"这需要你去实现它"。这一设计直接消灭了 Java 的适配器类生态：Java 强制接口全量实现，迫使生态发明 MouseAdapter / WindowAdapter 之类的空壳抽象类，只为让用户覆写十个方法中的一个；Koala 的答案是声明归声明、实现按需，intf-table 保持 O(1) 静态分发——**契约在类型层面完整，义务在实现层面宽容**。
- **泛型约束**：如 `T : Comparable[T]`；缺少约束只在泛型参数要求时报错。
- **自类型推断**：裸遵循自动推断自类型参数（如 Equatable → Equatable[bool]），无 Self 关键字。
- **LRO**：类型内省可见全部遵循关系与编译器插入的内容。
- **变型**：参数不变（invariant），返回值协变（covariant）。

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

- 类型系统信任声明签名，用户从不触碰桥接代码——`unsafe {}` 存在的理由（人在绕过类型系统）被结构性消除。
- **内存层同样无 unsafe**：shadowstack 将 C / native 代码分配的对象注册为 GC root——native 侧分配的对象与 `.kl` 中分配的命运完全一致，无"记得释放"规则。对照：JNI 局部/全局引用、Python C API 引用计数、Go cgo handle table 均需手动管理。
- 对照：Java JNI（句柄仪式）、Go cgo（栈切换开销）、Rust（强制 unsafe + transmute）、Python ctypes（运行时 marshal）。
- 标准库即活证据：`libs/std/native/*.c` 直接实现 Koala 签名函数。

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
| `Collection` / `Sequence` / `MutableSequence` | 容器语义（保留 dunder 声明） |
| `Map` / `Set` | 映射与集合语义 |

命名分层原则：**trait 层用跨语言惯例**（Java/Rust 风格的 `remove` / `remove_or`），**class 便利层对齐 Python**（`setdefault` / `update` / `fromkeys`）。

### 8.3 类型清单

| 类型 | 要点 |
|------|------|
| `str` | 字符串 |
| `bytes` | 固定长度字节数组，24 方法；位置式写族 |
| `ByteBuf` | 可变字节缓冲，32 方法；append 式写族 |
| `list` | 动态数组 |
| `dict` | **插入序**哈希映射；Python 便利方法 + Rust 位置族（pop_first / peek_last）+ Java remove 命名 |
| `HashSet` / `TreeSet` | 集合并代数 |
| `tuple` | 不可变元组 |
| `range` | 区间（编译器将 `range(5)` 补全为 `range(0, 5)`） |
| `slice` | 切片（边界由编译器补全为具体 int，统一用 `end` 表排他上界） |

bytes / ByteBuf 的二进制编解码职责已**剥离至官方库 encoding**，本体只保留序列语义。

### 8.4 迭代

- for 循环直接接受 Iterator，支持元组解包。
- **内建容器原生展开**：for 对 range / tuple / list / dict / bytes 等内建类型做原生展开——尤其 `range` 循环**不创建 range 对象、不走 iterator 协议**，编译器直接生成循环体，热路径零分配零分发。
- 顶层组合子：`enumerate` / `zip` 现役；`filter` / `map` / `reduce` 在路线图（注释状态）。
- **明确不做**：`sum` / `any` / `all` / `sorted` / `min` / `max`（作者定案：简洁优先）。
- view 语义只用于固定长度类型，动态容器用 copy。

### 8.5 包编程规范

- **简单包用单文件**：包内容简单时，一个 `xxx.kl` 文件即可表示（如 `assert.kl`、`pretty.kl`）。
- **复杂包用目录**：目录名即包名（如 `io/`、`fs/`、`builtin/`），内部结构遵循两条规则：
  - `__<包名>__.kl` 为模块入口，**只放** `link` 语句、全局变量和顶层函数定义，**不放** class / trait 定义。
  - 每个 class / trait 建议单独定义在一个 `xxx.kl` 中，一个类型一个文件（如 `str.kl`、`reader.kl`、`any.kl`）。
- 效果：看文件名即知内容——"没有隐藏"在文件组织层面的投影。

### 8.6 文档注释规范

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

- **基建**：采用 LLVM 项目的 lit + FileCheck 工业标准（lit.cfg / ShTest / RUN 指令），test/ 下 127 个测试文件、2600+ 行 CHECK 断言。
- **三层金字塔**：
  - `test-kl`（30）——前端语言特性（cast、slice、if-let、包管理…）
  - `test-ir`（35）——优化器逐 pass 验证（ssa、sccp、isel、lsra、fusion、tailcall…）
  - `test-run`（64）——端到端运行（泛型、链表/树数据结构、LRO、intf 调用、IO…）
- **观察面全覆盖**：RUN 行覆盖 no-opt-ir / ssa / ir / lir / vreg / code / itable 全部 7 个 dump 阶段——"无隐藏特性"的工程回响：每个可观察阶段都有断言守护。
- **负向断言文化**：优化器测试大量使用 CHECK-NOT 守护"不过度优化"（如不同常量的 phi 必须保留、死分支常量必须清除），测试的是正确性边界而非仅优化效果——LLVM 测试文化的核心实践。
- **诊断与开关回归**：编译器错误消息有专门的 `2>&1` 回归测试；--int-trap / --float-trap / --fusion / --tail-call 每个编译开关均有对应测试。

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
