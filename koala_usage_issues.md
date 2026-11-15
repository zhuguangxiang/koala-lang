# Koala 使用问题记录

使用 koala 写脚本/测试过程中遇到的问题、不便和改进建议。

## 1. `[T]` 写列表类型，报错不友好

- 现象：`func sub(path str, pairs [str]) {}` 报一串 `expected a valid function declaration.`、`expected key:value pairs.`、`expected ']'` 等，没指出真正原因。
- 正确写法：类型 `list[str]`，构造 `list[str]()`。
- 建议：类型位置支持语法糖 `[T]`（等价 `list[T]`），bison 验证无冲突，风格同元组类型 `(A, B)`。
- 待定：表达式里 `[str]()` 能否作为构造。`[str]` 在表达式中是列表字面量（类型也是值，`[int]` 是合法的“类型列表”），解析期无法区分；`[str]()` 只能在语义阶段特殊改写（callee 为单元素列表字面量、元素是类型、无参数），但会造成 `[str]()` 与 `let t = [str]` 含义不一致。

## 2. 没有内置 `assert`

- 现象：脚本里 `assert(cond)` 报 `'assert' is not found`。

## 3. `str` 没有 `find`

- 现象：`t.find(s)` 报 `'find' is not found in class 'str'`，只有 `index`。其他语言习惯用 `find`。

## 4. `fs.read_str` 默认 `max = 65_536`

- 现象：读大文件（如 `src/parser/koala.y`）需显式传 `max`，否则可能被截断（未确认是截断还是报错），容易踩坑。

## 5. `-e` 模式下模块名显示为临时文件名

- 建议显示为 `__main__`。

## 6. `panic` 写 stdout 时消息丢失

## 7. 单行块里 `let` 解析失败

- 现象：`for i in range(2) { let _x = 1 }` 报 `error: unexpected end of file.`（无行号）；换成多行块正常，`{ print(i) }` 单行也正常。

## 8. 未捕获的运行时错误退出码为 0

- 现象：`print(l[-4])` 报 `Error: list index out of range` 并打印 traceback，但 `koala` 进程退出码是 0，lit 里无法用 `not koala ...` 断言失败。

## 9. 误写 `p.0` 访问元组元素时，koalac 段错误

- 现象：元组下标应写 `p[0]`；误写成 `p.0`（如 `for p in [("x", "y")] { print(p.0) }`）时 `koalac` 直接 Segmentation fault，`koala` 只打印 `koala: compilation failed`，应给出语法/类型错误提示。

## 10. `ut.assert_eq` 传入可选类型时 koalac 断言崩溃

- 现象：`ut.assert_eq(dq.iter().next(), "v")`（`next()` 返回 `str?`）时 koalac 触发 `check_generic_bound: Assertion 'ts_sym && ts_sym->ts && ts_sym->ts->kind == TYPE_TYPE' failed`（`parser_expr.c:1847`），应报类型错误。
- 绕过：改用 `next_strict()`。

## 11. 原生代码连续分配多个对象时，C 局部变量不是 GC root

- 现象：`os.run()`、`os.pipe()` 等原生函数先创建 Raw 对象并存放在 C 局部变量中，再创建 Process/tuple 把它们挂上去。若中间某次分配达到阈值触发 GC，尚未挂上的对象会被误回收。原生代码中途回调用户代码（`__hash__`、`__eq__`、`next()` 等）时，C 局部变量持有的新对象同样不受保护（如 dict `|` 的结果、hashset `is_subset` 的临时表中的元素）。
- 待办：需要原生代码的临时 root 机制（例如 push/pop 到 KoalaState 的 root 栈），或保证这些区间不触发 GC。

## 12. GC 扫描 VM 栈时可能把残留值当 root

- 现象：`stack_base`（`vm.c:51`，`aligned_alloc`）未初始化，`_new_frame`（`eval.c:250`）与 JIT 直调建帧都不清零新帧 locals。函数返回后其槽位落在扫描上界外，引用的对象被回收；之后新帧复用这些槽位，未写过的寄存器残留悬空指针，若此时触发 GC 会 mark 已释放内存。valgrind 报的 `gc_mark_roots`（`gc.c:250`）`uninitialised value` 即扫到从未写过的栈槽。`src/jit/DESIGN.md` §14.1 规则 3 推荐的“每次按规则 2 的上界扫”不充分，因为上界随调用深度伸缩。
- 状态：构造用例未能复现悬空指针（解释器/JIT 下 valgrind 0 错误）。
- 候选修法：A）`stack_base` 初始化为 nil，`_new_frame` 与 JIT 直调建帧时清零 locals；B）整段栈初始化为 nil，GC 以历史最高水位为扫描上界（不改 JIT，略保守）。倾向 B。

## 13. JIT 模式下 GC 覆盖不足

- 现象：`test_gc_*.kl` 去掉 `--no-jit` 全过，但耗时极短，churn 循环可能被 JIT 优化掉，JIT 帧/直调路径下的 root 正确性没有被真正压测。
- 待办：设计不会被优化掉的 JIT 场景 GC 测试（如自递归直调深处触发 GC）。
