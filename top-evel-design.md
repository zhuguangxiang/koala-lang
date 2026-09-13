Magic function

Magic function 是编译器内部的语言协议入口，不是普通方法。

语言语法
    ↓
编译器识别
    ↓
__xxx__()

因此用户不能通过 dot 访问：

obj.__xxx__()    // 非法

编译器会直接阻止这种访问。

普通 method

普通方法才属于对象 API，可以直接 dot 链式调用：

obj.foo()
obj.foo().bar()
obj.len()
obj.iter()
obj.next()
Top-level mapping

高频普通 method 可以提供 top-level shortcut：

len(x)  → x.len()
iter(x) → x.iter()
next(x) → x.next()

所以 mapping 和 magic 是完全不同的两套机制：

Magic:
语言语法 → 编译器 → __xxx__()
                  ↑
             用户不能 dot 调用

Mapping:
top-level → 普通 method
                ↑
          用户可以 dot 调用

这样 Koala 的边界就非常清楚：

__xxx__ 只存在于语言协议层，普通用户 API 永远不需要通过 magic function 暴露。

而且既然编译器已经把 magic function 的 dot 访问干掉了，就不需要依赖文档约定来防止用户使用，语言层面本身就是封闭的。

核心保留：对象元数据与基础转换（Koala 已有）这三个函数是绝对的刚需，它们让不同类型（字符串、数组、字典）的通用基础操作高度一致。

len(x) ➔ 映射到 __len__。绝大多数语言的通识。
str(x) ➔ 映射到 __str__。任何东西都需要快速方便地转成打印字符串。
hash(x) ➔ 映射到 __hash__。内建哈希表（Map/Set）的基石。

📊 2. 建议引入：数学与比较（Math & Comparison）这几个函数作为全局函数，阅读体验远超面向对象方法。
比如 max(a, b) 显然比 a.max(b) 更加符合人类的数学直觉。

abs(x) ➔ 绝对值。映射到 __abs__。max(...) / min(...) ➔ 极值。支持传入两个参数或一个可迭代对象（Iterable）
。pow(base, exp) ➔ 幂运算（通常比直接提供 ** 运算符更易于在某些强类型语言里做重载）。
🛠️ 3. 建议引入：函数式核心（Functional Core）如果 Koala 支持 Lambda/闭包，这三个全局函数配合 for...in 循环，
将直接让语言拥有极其强大的现代函数式编程体验（且它们返回的都是惰性迭代器，零拷贝）。

range(start, stop, step) ➔ 循环的绝对绝对主力（如 for i in range(0, 10)）
。enumerate(iterable) ➔ 同时循环索引和内容（如 for i, item in enumerate(list)），解决 for 循环拿不到索引的痛点。
zip(*iterables) ➔ 同时打包循环多个序列（如 for name, age in zip(names, ages)）。🛑

坚决从 Python 过滤掉（剔除）的函数以下 Python 函数在现代编译型语言中不合理，
应坚决降级为“普通方法”或由编译器关键字替代：
Python 全局函数Koala 应该怎么处理？剔除原因
id(obj)降级为指针/引用比较（如 === 或 &obj）暴露底层内存地址应该由特定的低级语法或反射提供，不配占用 Top-level。
type(obj)改为 obj.class 或关键字 typeof静态/渐进式类型语言中，类型反射应该属于反射库（Reflection）或专门的关键字。
reversed(seq)降级为方法：seq.reversed()正如我们前面推导的，s.reversed() 更利于 IDE 自动补全和链式调用。
sorted(seq)降级为方法：seq.sorted()同上，作为容器对象的方法，能更干净地进行就地排序（sort）和返回新容器排序（sorted）的隔离。
map() / filter()降级为方法：seq.map().filter()Python 的 map(fn, seq) 导致了括号嵌套地狱。现代语言一致认为链式方法体验更好。
any() / all()降级为方法：seq.any() / seq.all()属于集合的属性，面向对象写法更直观。

📝 最终建议的 Koala 全局函数完美清单rust

// 1. 元数据
len(x), hash(x), str(x), iter(x)

// 2. 数学
abs(x), min(a, b), max(a, b), pow(x, y)

// 3. 流程控制与迭代构造
range(stop), enumerate(seq), zip(a, b)


这个清单极其克制（一共只有 10 个左右全局函数），
既保留了 Python 表达力最强、写起来最爽的精华（如 enumerate 和 range），
又彻底甩掉了 Python 的历史包袱，维持了全局命名空间的纯净。
你觉得这个过滤后的清单符合 Koala 的定位吗？
我们可以接着确定 range 和 enumerate 返回的迭代器是否也适用我们刚刚敲定的非 magic 的 has_next / next 驱动机制。


已确定保留

abs
format
hash
len
max
min
panic
pow
print
str
typeof

已确定考虑增加
next       // iterator -> T?，耗尽返回 nil
enumerate  // 倾向 class/type，不是普通函数
zip        // 倾向 class/type，不是普通函数

待定，方案未确定
all
any
sum
sorted

明确抛弃
ascii
bool
breakpoint
callable
classmethod
compile
complex
delattr
dir
eval
exec
getattr
globals
hasattr
help
id
input
isinstance
issubclass
iter
locals
memoryview
object
open
repr
reversed
setattr
staticmethod
super
type
vars

我会按 Koala 的 top-level function 是否有独立价值 来分。

1. 明确应该保留
abs
abs(x)

普通 top-level function。

这是非常高频的数学操作，而且 abs(x) 比 x.abs() 更自然。保留。

format
format(value, spec)

普通 top-level function。

格式化本身是一个通用操作，不应该要求用户知道具体类型的方法。保留。

hash
hash(x)

@intrinsic mapping：

hash(x) → x.hash()

保留。

len
len(x)

@intrinsic mapping：

len(x) → x.len()

保留。

iter
iter(x)

@intrinsic mapping：

iter(x) → x.iter()

保留。

reversed
reversed(x)

也是 @intrinsic mapping：

reversed(x) → x.iter(reversed = true)

保留。

它和 iter() 共用一个 iterator implementation。

next

普通 top-level function：

next(iterator) T?

保留。

它和 Iterator.next() T 的语义不同：top-level next() 是安全接口，耗尽返回 nil。

max
max(...)

普通 top-level function。

这是对多个值/序列进行选择的通用操作，不是某个对象的 property。保留。

min

同上：

min(...)

保留。

pow
pow(x, y)

普通 top-level function。

数学二元操作，用 top-level 表达自然。保留。

print
print(...)

普通 top-level function。

这是语言级 I/O 操作，没有合理的 receiver。保留。

str
str(x)

普通 top-level function。

它是通用转换/字符串化入口，保留。

typeof
typeof(x)

Koala 自己的类型查询操作，保留。

2. enumerate

我仍然不建议做普通 top-level function。

它不是简单的：

enumerate(x) → x.enumerate()

而是创建一个带状态的迭代对象。

所以更合适的是：

enumerate → class/type

也就是说它仍然可以有非常自然的：

enumerate(xs)

构造形式，但语义上是调用一个类型/构造器，而不是 builtin function。

保留能力，但不归入 builtin function。

3. zip

和 enumerate 一样。

zip(a, b)

本质是创建一个组合迭代器/iterable。

因此：

zip → class/type

而不是专门设计一个 compiler builtin function。

保留能力，但不归入 builtin function。

4. all

暂定。

all(items)

它确实有 top-level 表达的价值，因为它是对 iterable 的聚合判断，而不是某个对象本身的简单属性。

但 Koala 还没确定：

泛型约束
什么类型可以作为 predicate result
空 iterable 的语义
是否只接受 Iterable[bool]
是否需要 predicate 版本

所以：

all → 待定
5. any

和 all 一样：

any → 待定

语义上值得考虑，但具体 API 还没定。

6. sum

继续待定。

sum() 看起来简单，但实际上涉及：

sum(Iterable[T])

到底要求什么：

T 必须支持 +？
是否需要 zero/identity？
整数、浮点、Decimal 等怎么处理？
空序列返回什么？
是否允许指定初始值？

所以：

sum → 待定
7. sorted

继续待定，而且我认为它比 all/any/sum 更值得谨慎。

需要先确定：

返回 Array[T] 还是 Sequence[T]
输入 Iterable[T] 还是 Sequence[T]
比较协议
是否支持 key
是否支持 comparator
稳定性
是否允许自定义排序方向

所以：

sorted → 待定
8. map / filter

不需要 top-level builtin。

如果以后 Iterable 提供：

items.map(f)
items.filter(pred)

那么 top-level：

map(f, items)
filter(pred, items)

没有足够的额外价值。

所以：

map    → 普通 method
filter → 普通 method

而不是 builtin。

9. 明确不要

这些继续全部抛弃：

ascii
bool
breakpoint
callable
classmethod
compile
complex
delattr
dir
eval
exec
getattr
globals
hasattr
help
id
input
isinstance
issubclass
locals
memoryview
object
open
repr
setattr
staticmethod
super
type
vars

其中 repr 继续坚决不要。

最终重新整理
已确定的 Koala top-level functions
@intrinsic mapping
hash
len
iter
reversed

对应：

hash(x)       → x.hash()
len(x)        → x.len()
iter(x)       → x.iter()
reversed(x)   → x.iter(reversed = true)
普通 top-level functions
abs
format
max
min
next
pow
print
str
typeof

其中：

next(iterator) → T?

是普通函数，不是 mapping。

考虑作为 class/type
enumerate
zip
待定
all
any
sum
sorted
不需要 top-level function
map
filter

它们如果存在，应当是普通 method。

明确抛弃
ascii
bool
breakpoint
callable
classmethod
compile
complex
delattr
dir
eval
exec
getattr
globals
hasattr
help
id
input
isinstance
issubclass
locals
memoryview
object
open
repr
setattr
staticmethod
super
type
vars

我认为这版已经和你现在定下来的核心原则一致了：

Top-level function 是精选出来的高频、普遍、自然的语言/API入口；mapping 只是其中一类，普通 top-level function 同样合法。没有必要为了 API 对称性给所有 method 都制造 top-level function。
