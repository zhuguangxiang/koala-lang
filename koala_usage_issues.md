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
