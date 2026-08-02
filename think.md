
# Road Map

## compiler

## 泛型优化

- 泛型IR保存到klc文件中
- 泛型展开，里面的方法和属性全部展开，不按需展开，但代码生产可以只生成使用的
- 用Self代替T，作为suger，在AST层面转换为T
- 泛型T的类型自动推导
- 有泛型自动推导就不需要Self了
- trait 自动绑定 T（默认绑定为类自身）
- 入参 invariant（不变）
- 返回值 covariant（协变）
- 如果 Sequence 的 T 不是类自身，用户必须显式写出 T

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

```


## optimizer

## standard library


## 其他的思考

- bytes支持不可修改选项，这样str.to_bytes就可以安全地用view，而不是copy
- io属于字节流io，但是，因为string的内部是utf-8的，不存在第2种编码格式，所有string也是字节流，所以io也可以直接读写string
- 有了bytes是只读的了，好像io就可以不需要支持string了，因为str.to_bytes没有性能问题
- 只需要bytesio, 不需要stringio，来表示bytes和str有io能力
- 不能合并bytesio和bytebuffer
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
