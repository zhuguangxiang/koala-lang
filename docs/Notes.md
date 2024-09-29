
# 随手笔记
## 计算机语言

参考：python/javajava/kotlin/go/rust/kotlipython/swift
overloading: 重载函数, 变量支持枚举类型?
override: 重写

变量定义:

```
var msg : string = "hello"
var val = 100
let msg : bool = True
let flag = 0b0101
var (code, msg) = (200, "OK")
var ret = (404, "Not Found")
let arr : [int8] = [1,2,3]
let m : [string : string] = {"name" : "Tump", ...}
let l = list()
// list[int8](), list(iterable), list((1,2,3))
let t = tuple(1,2,3), tuple([1,2,3])
let m = map[int, int]()
let r = range(1,2,-1)
let s = slice(1,2,-1)
x := 100
```

函数定义:

```
func foo(v1 : int, v2 : string) : string {
	return v2 * v1
}
```

运算符:

- +, -, *, /, %,
- .
- []
- :=
- ==, >, <, <=, >=, !=
- in, as, is
- not, and, or
- &, |, ^,<<, >>, >>>

逻辑控制:

- if/switch
- while/for
- guard x = ... else { ... }  // x can be used after guard and cannot used in {...}
- with file = open(...) { ... } // __fini__, file cannot be used after with

类型定义:

```
class Foo[T: Int|Slice, V: Bar&Baz[T]] : Object, Map, Seq {
... ...
}
```

类: 定义一种数据类型和操作集合

特征: 定义一组意义相关的接口的集合, 类可以实现部分接口,不需要实现全部接口.这一点和大多数OOP语言不同. 如果调用了一个没有实现的接口, 虚拟机运行时会抛异常, 由于异常是不可以恢复的, 用户需要处理这样的异常. 这样将强制用户实现具体的接口操作.

- 内置接口, 有动态指令的接口, 直接调用运行时slots接口执行

- 通用接口, 动态查找执行

```
trait Foo {
	func name() : string
	func age() : int {
		return 0
	}
}

class Bar : Foo {
	func age() : int {
		return 100
	}
}

let foo : Foo = Bar()
if foo.name is not None {
	print(foo.name())
}

bar := Bar()
if bar.name is not None { ... } // compile error? or o
```



`内置接口`

​	Number, Map, Sequence, Iterator, Dynamic-lookup等



## 语言虚拟机

### 指令集

- 具体指令: 类型确定的指令，比如：Int, Float, Array, Map对象
- 栈操作指令: PUSH指令, KW指令
- 函数调用指令: CALL指令, CALL_METHOD, CALL_DYNAMIC
- 函数返回指令: RETURN, RET_NONE,  RETURN_FROM_CALL
- 逻辑跳转指令: JUMP, JUMP_IF_TRUE, JUMP_IF_FALSE, JUMP_IF_GT
- 抽象指令: Number(+), Map([]), Sequence(in, *, []), Iterator(next)

### 执行状态机
### 对象模型
对象分为managed对象和非managed对象, 前者由垃圾回收算法分配和释放, 用户无需关心其生命周期; 后者为用户自己管理

## 内存管理
### 垃圾回收算法

## Linux-64地址空间

支持46位(64TB)的物理地址空间和47位(128T)的进程虚拟地址空间, 高18位没有使用
[Using the extra 16 bits in 64-bit pointers](https://stackoverflow.com/questions/16198700/using-the-extra-16-bits-in-64-bit-pointers)

## JVM中的G1算法
使用46位中的高4位, 低42位支持4TB地址空间.


## JVM中的ZGC算法

[理解并应用JVM垃圾收集器-ZGC](https://zhuanlan.zhihu.com/p/105921339 "理解并应用JVM垃圾收集器")

## 疑问

### 为什么需要2个survivor区域?

## 资料
- [Koka](https://koka-lang.github.io/koka/doc/index.html, "A Functional Language with Effect Types and Handlers")
- [mimalloc](https://github.com/microsoft/mimalloc)
- handle SIGSEGV nostop noprint

LISP有哪些抽象概念

字符串模式匹配算法

什么是接口?

- golang
- rust
- Java
- Python

Null问题：

很多语言使用option类型来解决null的运行时异常，从语言层面分析，因为语言没有null的概念，所以用户无法写出带null的代码，这是一种语言层面的解决方案。这种方案不需要编译器参与分析和支持，但牺牲了如下的考虑：

- 语言的易用性降低，对语言来说这是一个仅次于安全的非常重要的需要考虑的因素。Rust就是这样的例子，极大提高安全性和极大降低易用性，把rust变成了专家级语言，脱离了普通程序员。
- 会给运行时带来可观的开销，会生成box和unbox的代码，会进行校验。
- 易用性，性能和安全，是编程语言需要考虑的铁三角，是需要作平衡的。

`思考`：为什么需要非常强的逻辑去解决这样的问题呢？最近在实现CMS算法，发现一个值得思考的问题，我一直想做到Mutator和Collector的状态强一致性，但是最后失败了，系统进入死锁状态。后来我想了一下，并发系统一定要强一致性吗？最终一致性或者弱一致性不好吗？最后把系统改为弱一致性（允许在某个时间点上或者某段时间上状态不一致，其实也就是系统要有容错机制），一切工作正常了。我好像又开始悟了，薛宝钗说贾宝玉悟了😄

回到编程语言的思考上来，带了null的语言就不安全吗？有没有带null或者隐式带null的语言，从语言层面看，从系统层面看，都是安全的，而且运行时也没有额外的检查的开销也不会有异常产生呢？答案是肯定的，但是这需要编译器支持，但运行时没有额外开销。何乐不为呢? swift语言的`if let`和`guard let ... else`，特别是`guard`。我们可以使用`if let`解决资源不能及时释放问题，我们可以用`guard let`解决null问题。类型为`Type?`的变量不能直接使用需要在`if let`或者`guard let`中重新定义一个变量使用，这样就是安全的，编译器会作检查，而且这种检查是很容易实现的。







开发必备：

- Java
- clang/llvm
- gcc/gdb/cmake
- flex bison
- git
-  pip install -U cmake ninja clang-format lit FileCheck
- tmux
  - Ctrl + b
- python
  - python3 -m venv .venv
  - source .venv/bin/activate
  - pip install pysocks
  - deactivate
- proxy
  - v2raya v2ray
<!--stackedit_data:
eyJoaXN0b3J5IjpbMjM2ODg5NzQ1XX0=
-->