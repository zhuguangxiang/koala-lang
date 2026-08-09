
# Road Map

## compiler

## 泛型优化

- ~~泛型IR保存到klc文件中~~
- ~~泛型展开（单态化），里面的方法和属性全部展开，不按需展开，但代码生产可以只生成使用的~~
- 决定：暂不做泛型AOT单态化，继续沿用当前类型擦除（TValue统一表示）方案。
  字段布局不变的前提下，单态化只能省掉tag动态分派的开销，收益有限，
  且需要在klc中新增IR序列化格式（存isel前的泛型IR）、处理跨包重复实例化去重等，
  投入产出比不高。后续性能优化方向改为JIT（运行时按具体类型+热点信息特化），
  不做AOT，因此klc格式和InstanceSymbol（仅服务编译期类型检查）保持不变。
- 用Self代替T，作为suger，在AST层面转换为T
  - 注意：应先做“泛型T自动推导”，Self糖依赖推导完成后可能就不需要了；顺序不要搞反，避免白做。
- 泛型T的类型自动推导
  - 部分已实现：`TP_INFER`（symbol.h）+ 构造函数单参数推导（parser_expr.c），如 `Foo(1)` 无需写`Foo[int](1)`。
    待办是把推导范围从“构造函数单参数”泛化到更多场景（普通方法调用、多类型参数等）。
- 有泛型自动推导就不需要Self了
- trait 自动绑定 T（默认绑定为类自身）
  - 未实现，现状要求显式写出，如 `class str : Sequence[str]`。
- ~~入参 invariant（不变）~~
  - 已实现：parser.c 中 generic 类型参数严格不变（`List[int32]`与`List[int64]`不兼容）。
- 返回值 covariant（协变）
  - 待确认：若同一方法内T既是入参又是返回值类型，不能整体协变（否则不安全）。
  - 需要区分“只读方法（T仅出现在返回值）可协变”与“读写方法（T同时在入参）必须invariant”，不能一刀切。
- 如果 Sequence 的 T 不是类自身，用户必须显式写出 T（已是现状，后续作为“自动绑定”的例外规则）

```
pub class str : Sequence

pub class str : Sequence[str]

trait Equatable[T] {
    func __eq__(other T) bool
}

class Point : Equatable {
    func __eq__(other Point) bool { ... }
}

func foo(x: Equatable) { ... }

func foo[T](x: Equatable[T]) { ... }

func foo[T: Equatable[T]](x: T) { ... }

```
**结论**：
**泛型走类型擦除**
有性能问题走定制化OP+CFFI
增加一个dynamic_upcast op来支持，所有泛型调用方法的例子
先限制T的upbound只能有一个,限制T实例化必须是class，不能是trait，也就是T的类型就是接口类型？
union类型限制为class，不能有trait
- union 成员仅允许 concrete class（可含 builtin class）
- 不允许 trait、type parameter、any
- 若需要“多接口能力”，用泛型约束或显式 upcast，不用 union 表达

泛型T的类型自动推导
如果 Sequence 的 T 不是类自身，用户必须显式写出 T
入参 invariant（不变）
返回值 covariant（协变）

建议定成下面这条语言规则：

- 禁止 x: Equatable 这种 existential 写法（函数入参必须是完整类型）
- 允许写简写：func foo[T: Equatable](x: T)
- 编译期规范化为：func foo[T: Equatable[T]](x: T)
再补两个必要约束，避免歧义：
- 只有当 Equatable 是单参 trait 时才允许这类省略（否则报错要求写全）
- T 在 Equatable[...] 里默认绑定为“当前类型参数自身”，即 Self-like 绑定

例子：

可以省略（单参）
trait Equatable[X] { ... }
T: Equatable → T: Equatable[T]

不能省略（多参）
trait Mapper[K, V] { ... }
T: Mapper ❌
因为不知道该补成 Mapper[T, ?] 还是别的，必须显式写全。

## optimizer

## standard library


## 其他的思考

- bytes支持不可修改选项，这样str.to_bytes就可以安全地用view，而不是copy
- io属于字节流io，但是，因为string的内部是utf-8的，不存在第2种编码格式，所有string也是字节流，所以io也可以直接读写string
- 有了bytes是只读的了，好像io就可以不需要支持string了，因为str.to_bytes没有性能问题
- 只需要bytesio, 不需要stringio，来表示bytes和str有io能力
- 不能合并bytesio和bytebuffer
  - 理由待补充：bytebuffer大概率是可增长的缓冲区数据结构，bytesio是流式游标/Reader-Writer接口，语义不同（类似Go的bytes.Buffer vs io.Reader/Writer），不是同一层概念。
- __str__, vs __fmt__
- 用户使用一个运算符，或者调用一个函数，如果是class则，不检查是否实现了对应的接口
- 如果用户调用了一个接口，则需要检查是否实现
- a > b, 如果a和b都是class，则检查class是否实现了__gt__,但不检查是否实现了Comparable接口
- let x Equatable, let y Equatable, x > y, 则也只检查Equatable中是否有__gt__
- 检查class是否实现接口，只有再赋值或者入参，或者返回值赋值的时候检查
（1）运算符检查：只检查 class/trait 是否实现了对应的运算符方法
（2）接口检查：只有在类型被声明为某接口时才检查接口方法是否实现
- 运算符不绑定接口的模型
- 接口中的方法，类可以实现一部分，但是未实现的方法，再运行时如果被执行了，则抛异常，到时候在补上，避免大量的Adapter的出现
- 在生成IR后，插入一个transform，将一些op转换为call，然后再给opt
- 逻辑运算符（不可重载）位运算符（可重载）__and__, __or__, __xor__就是位运算符
- 自动添加free来释放build_intern这样的op

### koala支持脚本执行，但只会编译当前kl源文件，依赖的包必须事先编译为klc
- 既保住了用户体验，一般用户都是执行一个脚本；又是静态类型的编程语言

## 逃逸分析：先只支持入参，native由程序员标记

```
class Foo {

    @noesc(self, name)
    func hello(name str) { ... }
}
```
index access
☐ slice access
☐ __str__() vm method binding
☐ range 完善
☐ tuple 完善
☐ str 完善
☐ int/float 完善
☐ list 完善
☐ dict 完善
☐ set 完善
☐ bool 完善
☐ NoneType 完善
☐ 跨模块
☐ 迭代器协议
☐ with 语句支持
☐ 函数定义和调用
☐ 类定义和实例化
☐ 属性访问和方法调用
☐ 继承和多态
☐ 异常处理
☐ 模块导入和使用
☐ 文件操作
☐ 正则表达式支持
☐ 多线程和多进程支持
☐ 网络编程支持
☐ 数据库连接和操作支持
☐ 标准库支持
☐ 性能优化
☐ 内存管理优化
☐ 错误处理和调试工具
☐ 文档和示例代码
koalac --build-stdlib --write-klc libs/std/builtin --package-name=std/builtin
