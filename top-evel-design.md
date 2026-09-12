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
