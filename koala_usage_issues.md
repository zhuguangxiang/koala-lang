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

## 14. 数字字面量后紧跟 `+` / `-`（无空格）词法报错

- 现象：`let x = 2+3` 报 `syntax error`，`let x = 2-3` 同样；加空格 `2 + 3` 正常输出 `5`。`*` 不受影响（`2*3` 正常）。不限于 list 字面量：`(2+3)` 报 `expected ')'`，`[2+3, 4]` 报 `expected ',' or ']'`，报错位置落在表达式起点而非 `+` 处。
- 最小复现：`func main() { let x = 2+3; print(x) }`（写成 `2 + 3` 即通过）。
- 初步分析：数字扫描把紧跟的 `+` / `-` 当成指数符号（`1e+5` 路径）的一部分吞掉，产出非法 token。
- 建议修法：仅在刚吃掉 `e`/`E`（十六进制的 `p`/`P`）之后才接受 `+`/`-` 作为符号，其余位置立即回退，交回主扫描循环作为独立运算符 token。
- 建议测试：`test/test-run/test_lex_num_sign.kl`，覆盖 `2+3`、`2-3`、`[i+1, i-1]`、`(2+3)`、`2*3`，并保证 `1e+5` / `1e-5` / `0x1F` 仍正确。
- 绕过：运算符两侧加空格。
- 状态：2026-11-21 发现于 Phase 2 基准准备阶段，用户确认先记录、待后续处理。
- 补充（2026-11-22，klex/kparse 评审期间）：**标识符后紧跟 `-`/`+` 数字同样中招**，且根因不是指数符号，而是 `koala.l` 的数字规则自带 `[+-]?`（`[+-]?[0-9]([0-9_]*[0-9])?` 等四条）。flex 最长匹配把 `a-1` 切成 `ID(a)` + `INT_LITERAL(-1)`，parser 看到两个相邻操作数即报 `expected an expr list or ')'`。
  - 最小复现：`func main() { let a = 5; print(a-1) }` → `error: expected an expr list or ')'.`；`print(a - 1)` 正常。`x+2`、`arr[i-1]`、`f(n-1)` 同理。
  - 影响面：`libs/koala/klex` 忠实镜像了 `koala.l`，所以 `klex.tokenize("a-1")` 也产出 `ID / INT_LITERAL(-1)`（已实测）。基于 klex 的 kparse 若不做处理，会在同样的输入上失败；而写 koala 代码时也必须给 `+`/`-` 两侧加空格。
  - 候选修法：A）词法层去掉数字规则的 `[+-]?`，符号一律作为独立运算符 token，负数字面量由 parser 的一元 `-` 折叠（需要同步 `ps->sign` / `YY_INT_VALUE` 的符号处理）；B）保留符号规则，但仅当前一个 token 不是「操作数结束符」（klex 的 `_is_ender` 集合：ID/字面量/`)`/`]`/`}`/`?`/`!`/`_` 等）时才吞符号。B 改动小且与 ASI 复用同一判定，倾向 B。
  - 建议测试：`test/test-run/test_lex_num_sign.kl`（同原建议）追加 `a-1`、`a+1`、`arr[i-1]`、`f(n-1)`、`-1`、`a - -1`、`1e-5`、`0x1F`。
  - kparse 侧暂定策略：不改 klex，parser 在「操作数 + 带符号 INT/FLOAT 字面量」相邻时把字面量拆成二元 `+`/`-` 与无符号字面量，保证能解析编译器目前拒绝的写法（超集，不产生误拒）。

## 15. 原生变参 `__init__` 的实参展平只对 list 生效

- 背景：Phase 2 把 list 构造从 `build_intern @list` 改为普通的 `new` + `call @list:__init__`。`__init__(args ...T)` 是原生函数，元素在 call slot 里平铺，因此 `emit_ir_call` 把 parser 为变参打包的元组直接拆开，不再分配那个只用一次的中间元组。
- 现状：展平条件写死为 `type_is_list(callee->ts)`（`irgen.c` `emit_ir_call`）。目前标准库里只有 `list` 和 `tuple` 声明了 `__init__(args ...T)`，tuple 按用户要求仍走 `build_intern @tuple`（不可变、可进常量池），所以展平没有推广到通用路径。
- 影响：将来若新增别的原生变参 `__init__` 类，仍会先建中间元组再拷贝，白白多一次分配；需要把判断改成「callee 的 `__init__` 是原生且形参为单个 `...T`」之类的通用条件。
- 附带：`tuple::__init__`（`tupleobj.c` 的 `_tuple_init`）已随本次回退删除，tuple 不再需要它；`list::__init__` 保留。
- 状态：2026-11-21 记录，待后续决定是否通用化。

## 16. list 构造改为 `new` + `__init__` 的额外开销未量化

- 现象：每个 list 字面量现在比原来多一次方法 dispatch（查 `list::__init__` 的 import slot）和一个 self slot；IR 值 +2、LIR 指令 +5、字节码多 `new`/`call` 两条。
- 已做：20 万次嵌套 list/tuple 构造的 GC 压力烟测，结果正确、无崩溃、无 valgrind 新增报错。
- 未做：没有跑吞吐/延迟基准，无法给出相对 `build_intern` 的开销百分比，也没有确认 JIT 下这次 dispatch 能否被内联或消除。
- 待办：补一个不会被 JIT 优化掉的 list 构造基准（参考 #13 的思路），对比改动前后；若开销显著，考虑让 isel 把「`new` 紧跟 `__init__`」重新折叠成单条指令。
- 状态：2026-11-21 记录，待后续处理。

## 17. `libs/std/dist/*.klc` 陈旧产物导致运行时 “func not implemented”

- 现象：改完编译器只跑 `./build-debug.sh`，`test-ut` 有 11 个用例报 `Error: func 'tuple::__init__' is not implemented!`。原因是 `libs/std/dist/*.klc` 还是上一版编译器（tuple 也走 `new`+`__init__` 时）编出来的，里面残留 `call @tuple:__init__`，而新编译器已不再提供该原生函数。
- 复现：在 list/tuple 构造路径来回改动后，只 `./build-debug.sh` 不重新 `wombat build`，再跑 `./test-debug.sh`。
- 绕过：`cd libs/std && wombat build`，然后 `./build-debug.sh` 重新安装。
- 建议：`.klc` 里记录编译器版本/ABI 指纹，加载时不匹配就报错或自动重编；至少让 `build-debug.sh` 在编译器源码变动后连带重建标准库产物，避免这类「编译通过、运行才炸」的静默不一致。
- 状态：2026-11-21 发现于 Phase 2 回退 tuple 期间，已用重新构建绕过。

## 18. dict 字面量混用 bool 键与 int 键时断言崩溃

- 现象：`{true: "a", 1: "b"}` 触发 `object.h:519: to_raw_bool: Assertion '((v)->tag > (64 + 1))' failed`，进程 SIGABRT（退出码 134）。同一文件里 `{1: "a", 1.0: "b"}` 输出 `{1: a, 1: b}`、`{0.0: "a", -0.0: "b"}` 输出 `{0: b}`，均正常。
- 最小复现：`func main() { print({true: "a", 1: "b"}) }`。
- 初步分析：bool 与 int 的哈希值相同，两个键落进同一比较分支后，dict 的 key 相等性判断把 int 槽当 bool 解包，`to_raw_bool` 收到 int tag 而断言失败。疑为类型判别缺失，而非断言本身过严。
- 未定性：尚未判定是本轮 dict lowering 改动（`new` + `__init__` + 逐条 `seq.set`）引入，还是既有缺陷。需用隔离 `PATH` + `KOALA_PATH` 的改动前 release 二进制跑同一探针对比才能定性（驱动经 PATH 解析 `koalac`，混用不同 build 会静默串味）。
- 建议修法：定位 `to_raw_bool` 在 dict key eq/hash 路径上的调用链，在比较前补齐键类型校验；不要通过放宽断言掩盖。
- 建议测试：`test/test-run/test_dict_bool_int_key.kl`，覆盖 `{true: 1}`、`{1: true}`、`{true: "a", 1: "b"}`、`{false: "a", 0: "b"}`，锁定预期输出与是否合并。
- 状态：2026-11-21 发现于 Phase 3 收尾核对 `klr_const_map` 语义期间，用户确认先记录、待后续处理。

## 19. dict 常量键应在编译期合并（逻辑已写好但整条链不可达）

- 需求：dict 字面量中重复的**常量**键应在编译期就合并掉，少发 `seq.set`，而不是全部留给运行期 dict 覆盖。
- 现状：编译期完全不合并。Phase 3 后 `{1:"a", 2:"b", 1:"c"}` 发出 3 条 `seq.set`（重复的 `1` 出现两次）；改动前的 `build_intern @map, 1,'a', 2,'b', 1,'c'` 同样是 6 个操作数、不去重。即此行为改动前后一致，不是本轮引入的回退。
- 已有实现：`src/parser/klr/ir.c` 里整套编译期合并逻辑均已写好，但 `klr_const_map` **全仓零调用者**：
  - `klr_const_map`（ir.c:339）只做两件事——调 `klr_const_merged_list`，再把 `which` 从 `CONST_LIST` 改标为 `CONST_MAP`。
  - `klr_const_merged_list`（ir.c:297）是真正的算法：按 `key0,val0,key1,val1,...` 遍历，`out` 为写游标原地压缩；命中重复键时只覆盖 `items[2*dup+1] = val` 并 `continue`（`out` 不前进），即**首次出现占位、最后一个值生效**，与运行期覆盖语义一致。
  - `is_mergeable_key`（ir.c:233）白名单：常量且 `which ∈ {INT, UINT, FLT, BOOL, STR, TUPLE, RANGE}`；排除 list/map，因其可变、运行期可能按身份而非内容比较。
  - `mergeable_key_equal`（ir.c:255）：要求两者 `which` 相同；INT/UINT 比 `len`+`ival`；FLT 直接用 `==` 且故意忽略位宽（`0.0 == -0.0` 合并、`NaN` 永不合并，对齐运行期 `_dict_entry_eq_()`）；STR 先比 intern 指针再回退 `memcmp`；TUPLE/RANGE 递归逐元素，任一元素不可比则整体判不等。设计上保守——判不等只会少合并，不会误合并。
- 不可达原因：`const_copy_prop.c:1264` 的常量 call 折叠分支只处理 `list` / `tuple` / `int64`，不走 map；irgen 的 `emit_ir_map` 现在直接发 `new` + `seq.set`，也不经过常量池。
- 语义已核对：`{1:"a", 1.0:"b"}` 运行期输出 `{1: a, 1: b}`（**不合并**），说明 `which` 必须相同这条前置检查与运行期一致 —— koala dict 不像 Python 那样跨类型合并键。
- 待办：决定接入点——在 `emit_ir_map` 里对常量键先去重再发 `seq.set`，或复用/改造 `klr_const_merged_list`。约束：
  1. 只能合并常量键；对象键（自定义 `__hash__`/`__eq__`）必须保留逐条 `seq.set`。
  2. 合并不得吞掉用户 `hash()` 抛出的异常，也不得改变条目求值顺序带来的副作用。
  3. 需先解决 #18 的 bool/int 键语义，否则编译期合并规则无法与运行期对齐。
- 关联：`docs/Koala_TODO.md` §item 3 原计划把 `klr_const_map` / `klr_const_merged_list` / `is_mergeable_key` / `mergeable_key_equal` 当死代码删除（§27）。本条要求改为**保留并接上**，该删除决定需相应撤销。
- 状态：2026-11-21 记录，用户要求后续解决。

## 20. `list[uint8]` 的整数字面量收窄规则前后不一致

- 现象：同样把 `int64` 字面量放进 `uint8` 元素位置，三条路径给出两种结果：
  - `var a = list[uint8](65, 66)` → `error: Incompatible type: expected 'uint8', but got 'int64'.`（每个实参各报一次）
  - `var b list[uint8] = [65, 66]` → `error: Types of two sides are not matched. lhs: list[uint8] =/= rhs: list[int64]`（列表字面量先自行推导成 `list[int64]`，再整体比对，不做逐元素收窄）
  - `var c list[uint8] = []; c.append(65)` → **编译通过且运行正确**（`T` 已绑定为 `uint8` 的方法调用反而接受了 `int64` 字面量）
- 最小复现：`func main() { var a = list[uint8](65, 66); print(a.len()) }` 报错；把同一行换成 `var c list[uint8] = []; c.append(65); print(c.len())` 则输出 `1`。
- 影响：构造 `uint8` 序列（`frozenbytes(data Iterable[uint8])`、`Buffer`、`bytes.fill` 等入口）时，最自然的两种写法都不可用，只能写 `list[uint8](uint8(65), uint8(66))` 或先建空表再 `append`。本轮 `test_frozenbytes.kl` 因此用 `byte_list(s str)` 辅助函数绕开。
- 初步分析：疑为三处的类型检查入口不统一——泛型构造的变长实参走严格 `compatible` 检查；列表字面量走「先推导整体类型再赋值兼容」；而已实例化泛型的方法实参走了允许字面量收窄的另一条路径。需要定位后统一，而不是单独放宽某一处。
- 未定性：尚未判定 `append` 接受 `int64` 是「字面量收窄」的有意设计，还是漏检（若存入的值超出 `uint8` 范围会怎样，未测）。
- 建议测试：`test/test-ut/test_uint8_literal_narrowing.kl`，覆盖上述三条路径 + 越界字面量（`append(256)` / `append(-1)` / `list[uint8](256)`），锁定「哪些位置允许收窄、越界如何报错」。
- 建议修法：先确定语言规则（整数字面量能否隐式收窄到定长整型、在哪些位置），再让三处共用同一判定；若允许收窄，则列表字面量应按声明的元素类型推导而非先推 `int64`。
- 状态：2026-11-21 发现于 frozenbytes 独立类型实现的测试编写期间，已用辅助函数绕过，待讨论。

## 21. koala 驱动器经 PATH 找 koalac，旧安装版会静默遮蔽新构建

- 现象：`./build-debug.sh` 后直接用 `./build/debug/bin/koala x.kl` 验证，执行的仍是 `~/.local/bin/koalac`（旧安装版）：驱动器用 `posix_spawnp("koalac", ...)` 按 PATH 解析编译器，而 `env.sh` 并未把 `build/debug/bin` 加入 PATH。本轮 SSA 修复后曾误判「修复无效」，实为旧编译器在跑。
- 最小复现：改动编译器后 `./build-debug.sh`（不 install），`source env.sh && ./build/debug/bin/koala x.kl`；`which koalac` 指向 `~/.local/bin/koalac`。
- 影响：所有「构建后直接跑 build/debug/bin/koala」的验证都不可信；lit 不受影响（lit.cfg 自行把构建目录置顶 PATH）。
- 建议：驱动器优先按自身可执行文件所在目录解析 koalac（/proc/self/exe 或 argv[0]  dirname），PATH 仅作回退；或启动时比对版本/构建时间并在不一致时告警。短期约定：验证前 `export PATH=$(pwd)/build/debug/bin:$PATH` 或先 `./install-debug.sh`。
- 状态：2026-11-21 记录于 lambda/SSA 修复期间，已用 install-debug.sh 绕过，待讨论。

## 22. 可选函数类型的局部变量判空收窄后调用，parser 阶段 UNREACHABLE 崩溃

- 现象：`let a IntFn? = twice; if a != nil { print(a(1)) }` 编译崩溃：`parser_expr.c:2796: Why goes here?`（UNREACHABLE），plain 与 --ssa 均触发。`a!(1)` 同样崩溃。
- 最小复现：`type IntFn = func(int) int` + `func twice(v int) int { return v * 2 }` + `func main() { let a IntFn? = twice; if a != nil { print(a(1)) } }`。
- 初步分析：流敏感收窄把 `a` 的类型改为 IntFn 后，call 的 lhs 符号 kind 不在 parse_call 的分支列表（SYM_FUNC/SYM_INSTANCE_FUNC/...）中，落入 else UNREACHABLE。先 `let b = a!` 再 `b(1)` 可绕过（但会有 "bang on non-nullable" 误报 warning，收窄后 bang 应视为合法）。
- 影响：所有「可选函数值判空后直接调用」的自然写法不可用；与本轮 SSA 修复无关，修复前即存在（git diff 未触及 parser_expr.c）。
- 建议测试：test/test-run/test_optional_func_call.kl，覆盖 if 判空后调用、`!` 后调用、`??` 后调用、跨 phi 的可选函数调用。
- 建议修法：定位收窄后 lhs->sym 的实际 kind，在 parse_call 中按函数类型分派（与 SYM_FUNC 同路径），而非 UNREACHABLE。
- 状态：2026-11-21 发现并修复：parse_call 与 emit_ir_call 接受 SYM_SHADOW_VAR 走函数值路径；isel_lower_cast 对符号同型 cast 降为 load_func/load_type（--ssa 循环内收窄调用）。回归 test_optional_func_call.kl。全局可选变量收窄另见 #23。

## 23. 全局可选变量判空收窄后使用，ir_cast panic（既有缺陷）

- 现象：`var gs str? = nil; func main() { gs = "hi"; if gs != nil { print(gs.len()) } }` 编译崩溃：`panic: 'ir_cast' op requires a reg value or const`，plain 与 --ssa 均触发；与函数值无关，str? 即复现。
- 根因：emit_ir_ident 的 SYM_SHADOW_VAR LOAD 路径（irgen.c 约 580-588 行）对全局变量直接取 origin->ir_val（KLR_VALUE_GLOBAL）送 klr_build_cast，未先 klr_build_get_global 加载；cast 只接受 reg/const。
- 影响：所有「全局可选变量判空后直接使用」的写法不可用（局部变量正常）。与 #22 修复无关，修复前即存在。
- 建议测试：test/test-run/test_global_optional_narrow.kl，覆盖全局 str?/int?/函数? 判空后使用、else 分支收窄、循环内收窄。
- 建议修法：shadow LOAD 对 VAR_SCOPE_GLOBAL 先 klr_build_get_global 再按需 cast（与 SYM_VAR LOAD 的全局路径一致）；STORE/LOAD_STORE 路径同理需检查。
- 状态：2026-11-21 修复 #22 期间探测发现，未修，待讨论。

## 24. SSA 重命名把函数/类符号暴露给比较指令：isel 不物化符号，崩溃或垃圾编码

- 现象（均 --ssa，plain 正确）：
  1. 类符号：`var k any? = nil; while ... { k = Foo; if k != nil { print(k) } }` → 运行时断言 `vm_ops.h:1484: (rs) < max_regs`。code dump：`num.ne r2, r255, r2`（符号无 vreg，-1 编码为 r255）。
  2. 函数符号：循环内 `f = twice; if f != nil { ... }` → 编码为 `jmp_int_eq_imm r255, #-121`（寄存器号与立即数均为垃圾），不崩溃但语义错误，现有 test_optional_func_call.kl 的 loop_call 碰巧通过（r255 读到零值、结果恰好正确）。非 fusion 管线另触发 `check_in_imm8` 断言（nil 为 CONST_NONE）。
- 根因：SSA 重命名把块内「写后读」直接替换为符号值；isel_lower_binary/isel_lower_num_ops 只物化常量（klr_is_const），符号直接取 ->vreg。且 KlrFunc->ts 是返回类型（new_func(ret, name)），find_binary_rule 据此误选 int 规则，nil 被当 int 立即数。
- 关联：#22 的 isel_lower_cast 符号物化分支本身正确（load_type r2,#0 / load_func 编码已验证，同型场景过回归）；any 装箱场景的崩溃根源在本条比较指令，不在 cast。test_optional_func_call.kl 新增 any_class_loop/any_func_loop 因此暂红（lit 623/624）。
- 最小复现：`class Foo { pub func __init__() {} }` + `func t(n int) int { var k any? = nil; var i = 0; while i < n { k = Foo; if k != nil { print(k) }; i = i + 1 }; return 0 }` + `func main() { print(t(2)) }`，`koala x.kl ++ --ssa`。
- 建议修法：isel_lower_binary 开头对 sym-const 的 lhs/rhs 先 lower_function_value/lower_type_value 物化为 local（带 proto/klass 类型）再选规则，函数/类均落入 num 对象比较路径，nil 走 load_tag；单点同治两形态。
- 建议测试：any_class_loop/any_func_loop 已在 test_optional_func_call.kl（双 RUN）；修复后用 --dump=code 确认 loop_call 无 r255 垃圾编码。
- 状态：2026-11-21 记录并修复：isel_lower_binary 开头物化 sym-const 操作数（lower_function_value/lower_type_value，local 带 proto/klass 类型）再选规则。code dump 确认 loop_call 编码为 load_func+load_tag+num.ne，无 r255；any_class_loop/any_func_loop 转绿，lit 626/626、ctest 9/9。

## 25. 全局 trait 类型变量经 `is` 收窄后使用，ir_cast panic（#23 的同族形态）

- 现象：全局变量声明为 trait 类型，在函数里用 `is` 收窄后调用其方法，编译崩溃：`panic: 'ir_cast' op requires a reg value or const`。同一代码把变量改成局部变量则正常；不做 `is` 收窄（直接调用 trait 方法）也正常。
- 最小复现：
  ```
  trait S { func t() str }
  class A : S {
      pub func t() str = "a"
  }
  let g S = A()
  func main() { if g is A { print(g.t()) } }
  main()
  ```
  → `panic: 'ir_cast' op requires a reg value or const`；把 `let g S = A()` 移进 `main()` 即输出 `a`。
- 初步分析：与 #23 同一根因位置——`emit_ir_ident` 的 `SYM_SHADOW_VAR` LOAD 路径对全局变量直接取 `origin->ir_val`（KLR_VALUE_GLOBAL）送 `klr_build_cast`，未先 `klr_build_get_global` 加载。#23 是可选类型判空收窄，本条是 trait 类型 `is` 收窄，触发的是同一条 cast 路径。
- 影响：所有「全局 trait 类型值 + `is` 收窄」的写法不可用。写库时的绕法：把 trait 类型的值放在局部变量或对象字段里（字段访问后收窄已验证正常），不要放在模块级全局。
- 建议测试：`test/test-run/test_global_trait_narrow.kl`，覆盖全局 trait 变量 `is` 收窄后调方法/取字段、`else` 分支、循环内收窄，以及对照组（局部变量、对象字段）。
- 建议修法：与 #23 合并修复——shadow LOAD 对 `VAR_SCOPE_GLOBAL` 先 `klr_build_get_global` 再按需 cast。
- 状态：2026-11-22 发现于 kparse 的 CST 节点模型选型探针（trait 类型 + `is` 分派），未修，待讨论。kparse 已按绕法设计（节点值只存在于局部与字段中）。

## 26. 泛型函数内把 `T`（绑定到具体类）隐式上转为约束 trait，值被破坏，调用即断言崩溃

- 现象：`func f[T : Trait](...) Trait` 用具体类实例化时，返回/传出的 trait 类型值不是合法对象；对它调用任何方法立即断言崩溃。解释器与 JIT 都崩：
  - `KOALA_JIT=0`：`include/runtime/vm_ops.h:477: _eval_frame: Assertion '((callable)->tag > (64 + 1))' failed.`
  - 默认 JIT：`src/runtime/eval.c:436: kl_jit_call_dyn: Assertion '((args)->tag > (64 + 1))' failed.`
- 最小复现（把返回类型 `N` 改成 `T` 就正常）：
  ```
  trait N { func k() int }
  class P : N {
      pub func k() int = 1
  }
  func first[T : N](items list[T]) N {
      return items[0]
  }
  func main() {
      let items = list[P]()
      items.append(P())
      let p = first(items)
      print(p.k())
  }
  main()
  ```
- 对照实验（已实测）：
  1. 返回类型写 `T`（`func first[T : N](items list[T]) T`）→ 正常输出 `1`。
  2. 去掉泛型，形参写具体类（`func first(items list[P]) N`）→ 正常。
  3. 用 trait 自身实例化（调用处 `let items = list[N]()`，即 `T = N`）→ 正常。
  4. 无约束泛型 `func first[T](items list[T]) T` → 正常。
  5. 泛型内部直接对 `T` 值调 trait 方法（`return items[0].k()`，返回 `int`）→ 也崩（`vm_ops.h:547`，`args` 断言），说明不止返回值路径。
- 初步分析：`T` 被约束为 trait 时，泛型特化把 `T` 的运行时表示按「具体类」处理，而 `T -> Trait` 的隐式上转没有做对应的装箱/接口值构造（或反之：调用方按接口胖指针解读了一个裸对象）。于是 trait 方法分派读到非法 tag。对照 1/2/3 可定位到「特化后 T 为具体类」与「目标类型为约束 trait」这两个条件同时成立的那条转换路径。
- 影响：所有「泛型 + trait 约束 + 把元素交给 trait 类型的容器/返回值」的写法都不可用。本轮 `koala/kparse` 的 `add_separated[T : Node](out list[Node], items list[T], seps)` 正是这个形状（把 `list[Param]`/`list[Expr]` 的元素追加进 `list[Node]`），导致任何带参数的函数在遍历 CST 时崩溃。
- 建议测试：`test/test-run/test_generic_trait_upcast.kl`，覆盖上述 5 个对照形态，外加：泛型内 `out.append(items[i])` 后在外部调用、`T` 绑定到 trait 自身、`T` 绑定到具体类、多级 trait 继承（`trait A : B`）下的上转。
- 建议修法：定位泛型特化中 `T -> bound trait` 的转换点，让它与非泛型路径共用同一套接口值构造（itable/装箱），而不是直接复用具体类的值表示；同时检查泛型内部对 `T` 值做 trait 方法分派的那条路径（对照 5）。
- 绕过：把泛型辅助函数改成非泛型（形参直接写 trait 类型，调用方先构造 `list[Trait]`），或按元素类型各写一份小函数。
- 状态：2026-11-22 发现于 kparse 首轮 CST 遍历测试，未修，已停止自动修改等待指示。
- 复验（2026-11-22 09:16）：用户告知已修复后重测，**仍崩**。当前 `~/.local/lib/koala/bin/koalac` 与 `build/debug/bin/koalac` 均为 09:11 构建，晚于最新源码改动（`src/parser/irgen.c` 09:06），即已包含最新构建；`git status` 中 `src/` 无未提交改动。上述最小复现与 kparse 的 `func f(x int) {}` 遍历用例都仍触发同一断言。
- 根因（2026-11-22 定位）：**容器里的 bounded `T` 与形参 `T` 运行时表示不一致**。
  - 一个类型参数至多一个 bound（`parser.c:3582`）。`T : B` 的值在擦除版泛型体里就按 `B` 的接口值使用：调用方 `try_to_build_tp_args_cast`（`irgen.c:1025`）把类型恰为 `T` 的实参统一转成 `B` 的接口值（class → `make_intf`，trait → `upcast_intf`；已是 `T` 或 `B` 的值跳过）。所以 `T → B` 不需要转换，`_build_obj_intf_upcast`（`irgen.c:271`）对泛型变量只发 `cast`、`cgen.c:1370` 降成 `move`，对形参是正确的。
  - 但 `list[P]` 传给 `list[T]`（`T : N`）时，元素仍是裸 `P`：调用方只转换类型恰为 `T` 的形参，容器元素无法逐个装箱。于是 `items[0]` 是裸对象，`return items[0]` 把它当成 `N` 交出去，任何 intf call 都读到非法 tag。
  - 因此这不是"返回值类型检查缺失"，而是嵌套泛型形参的表示问题。对照 5 的 `items[0].k()` 根本不经过返回值，同样崩，可见只靠返回值检查堵不住。
- 实测（2026-11-22，临时屏蔽编译期检查后跑，JIT 与 `KOALA_JIT=0` 结果一致）：
  | 写法 | 结果 |
  |---|---|
  | `func upcast[T:N](x T) N { return x }` | ✅ 形参已是 `N` 接口值 |
  | `func opt_upcast[T:N](x T) N? { return x }` | ✅ |
  | `func bound_to_trait[T:N](x T) N = x`（表达式体） | ✅ |
  | `let n N = P(); upcast(n)`（`T = N` 自身） | ✅ |
  | `func first[T:N](items list[T]) N { return items[0] }` | ❌ `eval.c:436` / `vm_ops.h:477` 断言 |
- 为什么不加编译期检查（讨论结论）：
  1. 只看类型无法区分 `return x`（合法）与 `return items[0]`（崩溃），两者类型都是 `T`。
  2. 按表达式来源区分（只放行形参标识符）规则过于复杂，且 `let y = x; return y` 这类同样合法的写法会被误禁。
  3. 利用"单 bound"简化成"`T` 只能去 `T`/`T?`/`any`/`B`/`B?`"是可行的，但按第 1 条它同样放行 `return items[0]`，对本 issue 无效。
  4. 从源头禁止"bound 为 trait 的 `T` 出现在嵌套形参类型里（`list[T]` 等）"能堵住，但会禁掉 `func max_of[T:Comparable](xs list[T]) T` 这类常见写法，代价过大。
  5. 不新增运行时 op（不做 `OP_MAKE_INTF_DYN`）。
- 决策（方案 C）：**不加任何编译期检查，代码保持修复前状态**，本条只作记录。曾实现过 `check_generic_invariant()`（只在 `parse_return` 检查 `T`/`T?`）并全量回归通过，但它会误禁上面表格中 4 种合法写法，已全部撤回，未新增测试用例。
- 待议修法方向：让容器里的 bounded `T` 与形参 `T` 表示一致（例如泛型容器按 bound 存接口值，或在取元素时按 bound 构造接口值），或让泛型体内的 intf call 走动态 itab 查找。两者都涉及运行时表示，需要单独设计。
- 状态：2026-11-22 未修，仅记录。`src/`、`include/`、`test/` 均无改动，全量回归 632 通过、`build-std-koala.sh` 109 通过。绕过办法见上一条。
- kparse 规避（2026-11-22）：`add_separated[T : Node]` 已删除，23 个调用点改为在各自非泛型 `slots()` 方法内联交织循环（上转发生在非泛型上下文，实测正常）。实测确认：无约束泛型 `func add[T](out list[N], items list[T])` 编译不过（`T` 不能转 `N`），显式局部上转 `let n N = items[i]` 在泛型内同样崩，故内联是唯一可靠规避。

## 27. 参数列表右括号前换行会被 ASI 插入分号而报错

- 现象：多行写函数签名时，若最后一个形参单独占一行、`)` 另起一行，报 `expected a valid function declaration.`：
  ```
  func f(
      a int, b bool
  ) {}
  ```
  把 `)` 提到最后一个形参同行（`a int, b bool) {}`）即通过；`(` 之后换行是允许的。
- 根因：`bool`/`int`/`str` 等类型关键字都在 `koala.l` 的 `semicolon()` 结束符集合里，形参后的换行触发 ASI，插入的 `;` 落在 `param_list` 与 `)` 之间，文法不接受。形参类型是自定义类名（ID）时同样触发。
- 影响：不是缺陷而是 ASI 的既有规则，但写多行签名时很容易踩到，且报错位置指向 `(` 而不是换行处，不易定位。
- 建议：报错文案提示「参数列表不能以换行结束」，或让 `param_list` 容忍一个尾随 `;`。
- 状态：2026-11-22 记录于 kparse 编写期间，已按「`)` 与最后一个形参同行」的写法规避。

## 28. enum 未实现 Equatable/Hashable/Printable，泛型约束检查失败（方案已讨论，未实施）

- 现象：enum 传给 `ut.assert_eq`（`[T : Equatable]`）报 `type argument 'Proto' does not satisfy constraint on type parameter 'T'`。gdb 断在 `type_spec_compatible`（parser.c:896）：dst = `Lstd/builtin.Equatable<Etest_pkg/enum_cross.Proto;>;`（TYPE_KLASS），src = `Etest_pkg/enum_cross.Proto;`（TYPE_ENUM），返回 0。
- 最小复现（test2.kl）：
  ```
  import "test_pkg/enum_cross" as zz
  import "std/ut"
  func main() {
      let p = zz.Proto.TCP
      let q = zz.Proto.TCP
      ut.assert_eq(p, q)
  }
  ```
- 根因：`check_generic_bound` 的 enum 分支（parser_expr.c:1862）调 `type_spec_compatible(Equatable[Proto], Proto)`；两边 kind 不同走 `_diff_kind_compatible_check`（parser.c:593），enum 被回退成 int64 再比，而 int64 的 base 只有 `Equatable[int64]`，不是 `Equatable[Proto]`，失败。本质是 `EnumSymbol` 没有 bases：class 有 `add_root_equatable`/`add_root_traits` 自动补根 trait，enum 没有对应路径。enum 的 `==`/`!=` 本身已支持（parser_expr.c:4500 专门处理），只是泛型约束不认。
- 实测现状（2026-11-22，koala 直接跑）：
  | 用法 | 结果 |
  |---|---|
  | `ut.assert_eq(p, q)`（T = Proto） | ❌ 约束不满足 |
  | `let e Equatable[C] = C.GREEN` | ❌ 类型不匹配 |
  | `c.hash()` | ❌ `enum 'C' has no members` |
  | `let p Printable = C.GREEN; print(p)` | ✅ 但打印 `1`（走 enum→int64 回退） |
  | `let h Hashable = C.GREEN` | ✅ 同样走 int64 回退 |
  | `dict[Proto, int]`、`format("{}", e)`、`func show[T](x T)` | ✅ 打印的都是整数 |
  运行时 enum 就是 int64（irgen `klr_lower_type`），int64 已实现这三个 trait，所以缺的只是编译期：enum 声明自己实现这些 trait + enum 值能调这些方法。
- 方案讨论：
  - A：只在 `check_generic_bound` 的 enum 分支补规则（std/builtin 的 `Equatable[E]`/`Hashable`/`Printable` 视为满足）。改动最小，但只覆盖泛型约束，值上转和方法调用仍缺。
  - B：在 `_diff_kind_compatible_check` 放行 enum→trait。会同时允许 `let x Equatable[Proto] = p` 这类值上转，IR 里 enum 是 int64，trait 值需要装箱/itable，没有对应实现，容易运行时崩溃。
  - C（已确认方向，含 Hashable/Printable）：编译期给 enum 固定 bases `Printable`、`Hashable`、`Equatable[E]`（与 class 自动根 trait 一致；故意不加 `Comparable`，enum 仍不能比大小），运行时方法全部复用 int64 实现。bases 由 enum 自身决定，不写进 klc，本地和导入 enum 各自在创建时生成。klc 格式和 VM 不用改。
- 方案 C 实施步骤（已讨论，未动手）：
  1. `include/parser/symbol.h`：`EnumSymbol` 加 `Vector bases`；`symbol.c` 的 `stbl_add_enum` 初始化、释放处（~227 行）`vector_fini`。
  2. 新增 `enum_add_root_bases(EnumSymbol *, ParserModule *)`：从 `pm->builtin` 取 `Printable`/`Hashable`，用 `find_or_add_instance` 生成 `Equatable[E]` 实例。调用点：`parser.c` `_add_enum` 和 `read_klc.c` `load_enums`。注意 builtin 必须先加载（`init_parser` 里 `load_builtin_module` 先于用户包；编译 builtin 自身时 `pm->builtin = pm->stbl`，需防 trait 尚未声明，找不到就跳过）。`instance_ts` 走 `klass_type_spec` 全局 intern，跨模块指针同一，`type_spec_compatible` 顶部 `dst == src` 即命中。
  3. `parser.c` `_diff_kind_compatible_check`：src 是 enum 时先 `_base_compatible(dst, &enum->bases)`，失败再回退 int64。`check_generic_bound` 不用改。值上转 trait 时 IR 源类型已是 int64，走现有 int64 itable 路径（需实测确认）。
  4. `parser_expr.c` `parse_dot_inner` enum 分支：除 `name` 外支持 `hash`/`fmt`/`__eq__`/`__ne__`，方法到 int64 类符号表找，接收者按 int64 处理；`__eq__`/`__ne__` 参数必须是同一 enum，挡住 `c.__eq__(5)`。
  5. 测试：新增 `test/test-run/test_enum_traits.kl`（本地/跨包 enum 的 `ut.assert_eq`/`assert_ne`、自定义 `[T : Equatable]`/`[T : Hashable]`/`[T : Printable]`、`Equatable[C]`/`Hashable`/`Printable` 变量、`c.hash() == int(c).hash()`，并入 test2.kl 场景）；test-kl 报错用例：`[T : Comparable]` 传 enum、`Equatable[D]` 接收 `C`、`c.__eq__(5)`。
  6. 验证：`./build-debug.sh`、`./test-debug.sh`、`./build-std-koala.sh`；文档更新 `docs/Koala_TODO.md` §78、`docs/Koala_Design_Overview.md` §2.6。
- 待拍板：Printable 打印什么。运行时 enum 是 int64，泛型（类型擦除）、`any`、`Printable` 变量、`format("{}", e)` 拿到的都是 int64，打印整数；`assert_eq` 失败信息也是整数。
  - a（讨论中推荐）：保持打印整数，与 `test_enum_basic.kl` 现有约定（"print shows the underlying int64 value"）一致，要名字显式 `c.name()`。
  - b：打印成员名。运行时必须能识别 enum（单独运行时类型或装箱对象），等于回到设计文档"enum 是 final class 单例"方案，涉及 VM/IR/klc，工作量大一个量级。
- 既有漏洞（本次不处理，仅记录）：因 int64 回退，`let x Equatable[int] = C.RED`、`let y Comparable[int] = C.RED` 现在都能通过。堵漏需去掉 enum→int64 隐式转换中 trait 目标的路径，会影响 `let i int = c` 等现有用法，单独讨论。
- 状态：2026-11-22 发现于 test2.kl（跨包 enum + ut.assert_eq）。方案 C（含 Hashable/Printable）已讨论确认，按用户指示暂不实施，未改任何源码，等待明确指令。

## 29. pub 函数签名里出现跨包 enum，导入方读 klc 断言崩溃（read_klc.c:68）

- 现象：包 A 导出一个签名中含**另一个包的 enum** 的 pub 函数/方法，包 B 导入 A 时 koalac 直接断言退出：
  `koalac: src/parser/read_klc.c:68: ts_need_fixup: Assertion 'ts->sym_id >= 0' failed.`
- 最小复现：包 `kpmin`（`__pkg__.toml` 里 `dependencies = ["koala/klex"]`）只有一个文件：
  ```
  import "koala/klex"
  pub func f(k klex.TokenKind) bool = true
  ```
  `wombat build kpmin` 成功；使用方：
  ```
  import "kpmin"
  import "koala/klex"
  print(kpmin.f(klex.TokenKind.ID))
  ```
  → 上述断言。
- 对照实验（已实测）：
  1. 去掉 `pub`（同签名私有函数）→ 正常。
  2. enum 只作局部变量（`pub func g() bool { let k = klex.TokenKind.ID; return true }`）→ 正常。
  3. 签名里是**本包**的 enum（`pub enum E` + `pub func f() E`、pub 方法、trait 方法签名）→ 全部正常。
  4. 形参换成跨包的 **class**（`klex.Token`）→ 正常。
  5. `switch` 不是必要条件，只要签名提到跨包 enum 就崩。
- 根因（已定位，2026-11-22 复核）：**klc 加载顺序依赖**。enum 的 TypeSpec 按签名全局 intern（`typespec.c` `__to_enum_type` → `type_spec_intern`，初始 `sym_id = -1`）；`read_klc.c` `load_enums`（~662 行）创建 enum 符号时用 `enum_type_spec(pkg, name)` 命中同一个 intern 对象并填上 `sym_id`。于是：
  - enum 定义在**导出包自己**的 klc 里（如 `test_pkg/enum_cross` 的 `make_holder(p Proto)`）：加载该 klc 时 `load_enums` 先把 `sym_id` 填好，同 klc 内签名再引用时已 >= 0，`ASSERT` 通过 → **这就是 bf56f9e7 的测试能过的原因，与 #29 同根因但不同 case**。
  - enum 来自**第三方包**（kpmin 导出 `klex.TokenKind`）：若 kpmin.klc 先于 klex.klc 加载，intern 对象 `sym_id` 仍是 -1 → `ts_need_fixup` 的 `ASSERT` 崩。实测把 `import "koala/klex"` 放到 `import "kpmin"` **之前**即正常。
- 与 bf56f9e7 / 44b1c754 的关系：bf56f9e7（00:13）把 `TYPE_ENUM` 分支改成 `return ts->sym_id < 0`（登记 fixup，消除顺序依赖，`read_klc.c:184` 的 enum fixup 处理代码仍在）；**44b1c754（09:12，plot 主题提交）把这 8 行回退成了旧 ASSERT**，顺序依赖随之恢复。疑似误回退，需确认。
- 影响：任何「导出接口里用别人家的 enum」的库，其可用性取决于使用方的 import 顺序。`koala/kparse` 的 `Cursor.check(k klex.TokenKind)`、`SyntaxToken.token_kind() klex.TokenKind` 正是这个形状；使用方先 `import "koala/klex"` 再 `import "koala/kparse"` 可绕过（已实测 parse 可用）。
- 建议修法：`TYPE_ENUM` 与 `TYPE_KLASS` 同等对待——`sym_id < 0` 时返回 1 并登记 fixup，在加载期按包名+enum 名解析；或在写 klc 阶段就为跨包 enum 解析出 sym_id。
- 建议测试：`test/test-run/test_cross_pkg_enum_sig.kl` + 配套 test_pkg 模块，覆盖 pub 函数形参、pub 函数返回值、pub 方法形参/返回值、trait 方法签名、pub 类字段为跨包 enum，以及私有函数/局部变量的对照组。
- 状态：2026-11-22 记录。**已修复**（staged：`ts_need_fixup` 恢复 `return ts->sym_id < 0`，新增 `test_pkg/enum_cross_indirect` 间接包 + `test_pkg_enum_cross.kl` 的 `zz.fn(yy.Proto.TCP)` 用例，import 顺序故意间接包在前）。09:54 构建后实测：staged 测试全过、kpmin 优先 import 的最小复现正常、kparse 无需顺序技巧直接导入可用。
