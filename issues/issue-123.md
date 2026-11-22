# 版本计划与路线图（合并自根目录 plan.md / think.md / review.md）

← 返回 [问题列表](issues.summary.md)

- [ ] 路线图存档 —— `0.8.0` 第 1 项已落地、第 2 项仍缺；泛型 / 语言设计 / 待办清单各条按需推进

> 来源：`docs/Koala_TODO.md` §6（2026-08-22 由根目录 `plan.md`、`think.md`、`review.md` 合并而来，原文件已删除）
>
> 本节是**路线图与设计结论存档**，不是一个可勾完的缺陷条目。已落地的结论保留（不要重复讨论），未落地的逐条列出。

---

### 详细描述

<details>
<summary><b>📦 6.1 版本计划（原 plan.md）· 2026-11-22 复测</b></summary>

**Version 0.8.0**

1. **支持非原始类型（用户自定义 class）的数值运算符**：`__add__` / `__sub__` / `__mul__` / 比较与相等（`__lt__` 等）—— ✅ **已可用**（2026-11-22 实测）：

   ```kl
   let s0 = "hello"
   let s1 = "world"
   print(s0 == s1, s0 > s1)      // false true
   ```

   ```kl
   class V {
       var n int = 0
       func __init__(x int) { n = x }
       pub func __add__(o V) V { return V(n + o.n) }
       pub func __lt__(o V) bool { return n < o.n }
       pub func __eq__(o V) bool { return n == o.n }
   }
   func main() {
       let a = V(3)
       let b = V(4)
       print((a + b).n, a < b, a == b)   // 7 true false
   }
   ```

   注：unary 运算符（`-c` / `~c` / `!c`）在用户类上**仍不支持**，见 [#117](issue-117.md)（§47.5）。

2. **支持把类型当作普通对象加载**，更易用：

   ```
   class A { }
   print(A.methods())   // this is static method call, not type's type method
   ```

   ❌ **仍未实现**（2026-11-22 实测）：`error: static method 'methods' not found`。

</details>

<details>
<summary><b>🧬 6.2 编译器 / 泛型路线图（原 think.md）</b></summary>

**泛型优化**

- ~~泛型 IR 保存到 klc 文件中~~
- ~~泛型展开（单态化）~~ → **已决定：暂不做泛型 AOT 单态化**，继续类型擦除（TValue 统一表示）。字段布局不变的前提下，单态化只能省掉 tag 动态分派开销，收益有限，且需新增 klc IR 序列化格式、处理跨包重复实例化去重，投入产出比不高。后续性能方向改为 **JIT**（运行时按具体类型 + 热点信息特化）。klc 格式和 InstanceSymbol（仅服务编译期类型检查）保持不变。
- 用 `Self` 代替 `T` 作为 sugar，在 AST 层面转换为 T
  - **注意**：应先做「泛型 T 自动推导」，`Self` 糖依赖推导完成后可能就不需要了；**顺序不要搞反**。
- 泛型 `T` 的类型自动推导
  - 部分已实现：`TP_INFER`（`symbol.h`）+ 构造函数单参数推导（`parser_expr.c`），如 `Foo(1)` 无需写 `Foo[int](1)`。**待办**：把推导范围从「构造函数单参数」泛化到更多场景（普通方法调用、多类型参数等）。
- 有泛型自动推导就不需要 `Self` 了。
- **trait 自动绑定 T**（默认绑定为类自身）—— 未实现，现状要求显式写出，如 `class str : Sequence[str]`。（注：裸遵循自类型推导已在 `Equatable` / `Comparable` 等落地，本条针对 `Sequence` 等元素类型场景的例外规则。）
- ~~入参 invariant（不变）~~ —— **已实现**：`parser.c` 中 generic 类型参数严格不变（`List[int32]` 与 `List[int64]` 不兼容）。
- **返回值 covariant（协变）**
  - 待确认：若同一方法内 `T` 既是入参又是返回值类型，不能整体协变（否则不安全）。
  - 需要区分「只读方法（`T` 仅出现在返回值）可协变」与「读写方法（`T` 同时在入参）必须 invariant」，**不能一刀切**。
- 如果 `Sequence` 的 `T` 不是类自身，用户必须显式写出 `T`（已是现状，后续作为「自动绑定」的例外规则）。

**泛型结论**

- 泛型走类型擦除；有性能问题走定制化 OP + CFFI。
- 增加一个 `dynamic_upcast` op 来支持所有泛型调用方法的例子。
- 先限制 `T` 的 upbound 只能有一个；限制 `T` 实例化必须是 class，不能是 trait。
- union 类型限制为 class，不能有 trait：
  - union 成员仅允许 concrete class（可含 builtin class）；
  - 不允许 trait、type parameter、`any`；
  - 若需要「多接口能力」，用泛型约束或显式 upcast，不用 union 表达。

**语言规则建议（trait 约束省略形式）**

- 禁止 `x: Equatable` 这种 existential 写法（函数入参必须是完整类型）；
- 允许简写：`func foo[T: Equatable](x: T)`，编译期规范化为 `func foo[T: Equatable[T]](x: T)`；
- 只有当 `Equatable` 是单参 trait 时才允许省略（多参如 `Mapper[K, V]` 必须写全）；
- `T` 在 `Equatable[...]` 里默认绑定为「当前类型参数自身」（Self-like 绑定）。

---

### 修复方案

<details>
<summary><b>🎨 6.3 语言设计思考（原 think.md「其他的思考」）</b></summary>

- **bytes 支持不可修改选项**，这样 `str.to_bytes` 就可以安全地用 view 而不是 copy；有了只读 bytes，io 可以不需要支持 string（`str.to_bytes` 无性能问题）；
- **只需要 bytesio，不需要 stringio**；不能合并 bytesio 和 bytebuffer（bytebuffer 是可增长缓冲区数据结构，bytesio 是流式游标 / Reader-Writer 接口，语义不同，类似 Go 的 `bytes.Buffer` vs `io.Reader/Writer`）；
- `__str__` vs `__fmt__` 待定；
- **运算符不绑定接口的模型**：用户使用运算符只检查 class / trait 是否实现对应运算符方法，不检查是否遵循了接口；只有类型被声明为某接口（赋值 / 入参 / 返回值）时才检查接口方法是否实现；
- **接口方法允许部分实现**，未实现方法运行时被执行则抛异常，避免大量 Adapter 类（现状：trait 一致性连签名都不查，见 [#118](issue-118.md) 48.2 第 6 项）；
- 生成 IR 后插入 transform 把部分 op 转为 call，再交给 opt（与 [#113](issue-113.md) §49 / [#117](issue-117.md) §47.3 选项 2 同一方向）；
- 逻辑运算符不可重载；`__and__` / `__or__` / `__xor__` 是位运算符（可重载）；
- 自动添加 free 释放 `build_intern` 之类的 op；
- koala 支持脚本执行，但只编译当前 kl 源文件，依赖包必须事先编译为 klc（既保住脚本体验，又是静态类型语言）。

**逃逸分析：先只支持入参，native 由程序员标记**

```
class Foo {
    @noesc(self, name)
    func hello(name str) { ... }
}
```

</details>

<details>
<summary><b>☑️ 6.4 待办清单（原 think.md 尾部 checklist）· 现状标注</b></summary>

- index access（进行中）—— 现已可用
- ☐ slice access —— 现已可用（`__getslice__`；tuple 切片仍缺，见 [#115](issue-115.md) 缺陷 D）
- ☐ `__str__()` vm method binding —— 已可用（`fmt` / `print`）
- ☐ range / tuple / str / int/float / list / dict / set / bool / NoneType 完善 —— 持续
- ☐ 跨模块、迭代器协议、with 语句支持 —— 跨模块与 `iter` 已可用
- ☐ 函数定义和调用、类定义和实例化、属性访问和方法调用 —— 已可用
- ☐ 继承和多态（**注：现设计为 class 恒 final + trait 组合，本条按新设计重估**）
- ☐ 异常处理（**注：现定案为 panic + 返回值，无异常体系，本条按新设计重估**）
- ☐ 模块导入和使用、文件操作、正则表达式支持 —— `koala/re` 已落地，见 [#109](issue-109.md)
- ☐ 多线程和多进程支持（**注：现定案为无栈协程，待公开细节**）
- ☐ 网络编程支持、数据库连接和操作支持
- ☐ 标准库支持、性能优化、内存管理优化（见 Memory Management 未实现标注）
- ☐ 错误处理和调试工具、文档和示例代码

构建标准库命令：

```
koalac --build-stdlib --write-klc libs/std/builtin --package-name=std/builtin
```

</details>

<details>
<summary><b>🏆 6.5 外部评审结论存档（原 review.md，仅存档不施工）</b></summary>

Koala 是完成度极高的原创设计：不是「某语言 + 某特性」的拼凑，而是从三条原则出发逐层推演的结果。基于可见面给出的「缺口清单」基本全部作废 —— 那些「缺口」要么是刻意的设计立场（错误处理），要么是看不见的水下机制（overflow、提升、并发、内存管理）。对「无隐藏特性」原则的注脚：对用户无隐藏，不等于对旁观者无纵深。

**五大世界级设计**

1. **根契约：普适性与类型精度的统一（独一份）** —— Java 用 Object 继承换普适性，丢了签名精度；Swift/Rust 用 opt-in 保精度，丢了普适性。Koala 的「空 `any` + 编译器自动遵循 + 泛型实例化槽位」是两条都要、代价为零的解。
2. **钩子与契约分轨（最干净的一份）** —— 四类运算符钩子零契约、语义契约零运算符捆绑、`Comparable` 保持经典的 `Equatable` 继承形态。
3. **panic + GC 的协同（被低估的妙手）** —— GC 兜底资源回收，panic 才能真正轻装上阵；与 C 式返回值构成「可预期 / 不可预期」的完美二分。
4. **unsafe-free FFI** —— 编译器独占桥接，人根本不碰；`unsafe` 的存在理由被结构性消除。
5. **数值机制** —— 窄类型 init-only + 自动提升 + overflow trap/wrap 双操作；「标准库完全不感知」的隐藏分层比 Swift 更彻底。

**语言坐标系**

| 维度 | Koala 位置 |
|------|-----------|
| 语义直觉 | Python（插入序 dict、dunder、enumerate/zip、class 层命名） |
| 契约机制 | Rust 系（intf-table、trait 约束）但无生命周期 / unsafe 负担 |
| 错误处理 | C 的返回值 + 极简 panic，拒 Java try-catch、拒 Go defer |
| 工程布局 | 学 Go（encoding 分包），避 Go 之短（cgo、无重载） |
| 对象模型 | 超越 Java（无 Object 神类）与 Kotlin（自动遵循免声明） |

</details>

<details>
<summary><b>🔗 关联</b></summary>

- [#111](issue-111.md)（§78）/ [#112](issue-112.md)（§112）：`enum` 与 `switch` 路线图，属 6.2 之后的新增语言能力。
- [#114](issue-114.md)（§103）：支配树 / GVN 演化路径，对应 6.2 里「性能方向改为 JIT」的另一半（编译期优化）。
- [#113](issue-113.md)（§49）：运算符副作用与 `pure`，对应 6.3 的两条（运算符不绑定接口、op 转 call 的 transform）。
- [#124](issue-124.md)（§7）：`OP_NUM_*` 族，是 0.8.0 第 1 项（用户类数值运算符）的 VM 侧底座。

</details>
</details>