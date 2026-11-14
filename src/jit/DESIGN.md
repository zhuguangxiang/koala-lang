# Koala 基线 JIT 设计 · Drop Bear（v1）

> 位置：`src/jit/`。本文描述 v1 的设计取舍、执行模型、字节码白名单、
> 两遍汇编与跳转回填、热度分派、内存管理、正确性保证与后续路线。
> 代码入口见 `include/jit/jit.h`，实现见 `src/jit/jit.c` 与 `src/jit/emit.c`。

## 1. 概述

v1 是一个 **Sparkplug 风格的基线（baseline）JIT**：把每条受支持的字节码指令
一对一地 lowering 成一段固定机器码，语义与解释器完全一致，只是去掉了
**取指 dispatch、指令解码、类型检查**，并把字节码跳转直接降为原生跳转。

它不做跨 GC 点的寄存器分配与全局类型数据流分析，不做内联/逃逸等优化（阶段 3 只做
基本块内的局部改进，见 §1 不变量 2）。目标是
**用最小的复杂度、零运行时依赖**，在数值热点函数上拿到稳定的加速比。

### v1 设计不变量（核心约束）

1. **GC 只靠 `KoalaState` 枚举，不需要 stack map**（最高原则）：所有对象引用在每个
   GC 点（helper / 运行时调用）都位于 VM 栈 `[stack_base, stack_top + 顶层
   max_call_args)` 或全局中，规则见 §14.1。下面各条都服从这一条；任何优化（含阶段 3）
   只要保证 GC 点上对象都能经 `KoalaState` 枚举到、GC 能正常工作，即为合法。
2. **保持 TValue 寄存器文件**：寄存器文件就是解释器同款的 `cf->locals`（每寄存器 16
   字节 `{tag, payload}`）。机器码可以把值的副本缓存在物理寄存器里（阶段 3 写穿缓存，
   §15），但在每个 GC 点、基本块边界和出口前，内存中的 TValue 必须与解释器
   语义一致（tag 正确、值已写回）。跨 GC 点的拆箱 + 机器 RA（需 stack map）属 v2
   （见 Koala_TODO §100）。
3. **帧与解释器同构**：每个被调用的函数都有真实 `CallFrame`（入 `ks->cf` 链、locals 在
   VM 栈上），traceback 与解释器逐帧一致。flag=0 调 koala 代码走 JIT→JIT 直调（原生侧
   按 `_new_frame` 建帧，§5.3），其余调用经 helper / `kl_call_code`。唯一例外是纯自递归
   （`is_self_only`）：各层共用外层帧，直接 near call 本函数原生入口（GC 仍靠整段扫
   VM 栈覆盖，§14.1 规则 1）。

### 代号（Naming）

承接 **koala**（语言）/ **wombat**（构建工具）的澳洲动物命名传统，JIT 用澳洲传说神兽：

| 代号 | 版本 | 定位 |
| --- | --- | --- |
| **Drop Bear** | v1 | 基线（本文）：传说里从树上扑下来的凶猛考拉，往热点「空投」原生代码 |
| **Yowie** | v2 | 优化：澳洲雪人，拆箱 + 机器 RA + JIT 直调（见 Koala_TODO §100） |
| **Bunyip** | v3 | 进阶：澳洲沼泽神兽，IC / 内联 / AOT 等 |

koala/wombat 是真实动物，Drop Bear/Yowie/Bunyip 是传说神兽——JIT 是语言家族里「开了挂的传说表亲」。

- 目标平台：x86-64 / System V AMD64 调用约定（Linux）。
- 依赖：仅 libc（`mmap`/`mprotect`/`memcpy`）。不依赖 libgccjit、LLVM 等。
- 实测：`bench/test_arith.kl`（1e8 次纯整数循环）解释器 1.289s → JIT 0.434s，
  约 **3×** 加速（best of 3）。

### 非目标（v1 明确不做）

- 闭包捕获以外的高级特性（接口/寄存器调用、方法、非自调用尾调用已在阶段 1
  支持，见 §5.3）。
- 对象 / 字符串等非数值运算（int64/uint64/窄整型/float64/bool 已覆盖，见 §5.4、§5.5）。
- 跨 GC 点的寄存器分配、去优化（deopt）与 OSR（写穿寄存器缓存见 §15）。
- 跨函数内联、类型特化推测。

遇到上述任何情况，JIT **在编译期整体放弃**（bail-out），该函数永久走解释器，
保证语义正确（详见 §7、§9）。

## 2. 关键取舍：为什么镜像 TValue 而不是拆箱

有两种候选执行模型：

- **方案 B（拆箱 int64 槽）**：把 VM 寄存器映射成裸 `int64` 内存槽，机器码直接
  在整数上运算。速度更快，但需要类型数据流分析来保证每个寄存器在每条指令处
  确实是 int，否则 tag 与 payload 会错位。复杂度与出错面都大。
- **方案 C（镜像 TValue，v1 采用）**：机器码直接操作解释器同款的 `TValue`
  寄存器文件（`cf->locals`），每个寄存器仍是 16 字节 `{tag, payload}`。
  运算只读写 payload（`ival`），并按解释器语义写回 tag。

v1 选 **方案 C**：因为寄存器布局、tag 写回、分支条件都与解释器逐条对齐，
**正确性由构造保证**（correctness-by-construction），无需任何静态分析。
代价是每次运算多一次 tag 写入、寄存器仍驻留内存。阶段 3 在不破坏 §1 不变量 1/2 的
前提下做局部改进（写穿寄存器缓存，§15）；跨 GC 点的拆箱与寄存器分配留给 v2。
省多余 tag 写（前向已知 tag 数据流）曾实现并实测后撤销：release 下仅 loop-sum +25%，
其余 ±3% 噪声内——tag 写是立即数 store，被 store buffer 吸收，真正开销在每条指令的
操作数 load/store，不值得为此增加一套分析。

## 3. 组件与文件

| 文件 | 职责 |
| --- | --- |
| `include/jit/jit.h` | 公共 API：`JitEntry`、`kl_jit_compile`、`kl_jit_free`、开关 |
| `src/jit/emit.h` / `emit.c` | x86-64 机器码编码器子集（REX/ModR/M/rel32 等） |
| `src/jit/jit.c` | 字节码解码、白名单翻译、两遍汇编、W^X 代码缓存 |
| `src/jit/cpu_x86_64_ops.h` | x86-64 指令定义的 X-Macro 真源（仿 `opcode_list.h`）：opcode/mask/编码类/助记符 + 寄存器/条件码/分组名字表 |
| `src/jit/disasm.h` / `disasm.c` | 表驱动反汇编器：消费 `cpu_x86_64_ops.h`，仅含通用解码算法（Intel 语法，与 objdump 逐条对齐） |
| `src/jit/printer.h` / `printer.c` | IR 风格的 trace dump 与 bail 日志（自带 opcode 表，不依赖 `libparser`） |
| `src/runtime/eval.c` | `kl_call_code` 中的热度计数与原生/解释器分派 |
| `include/runtime/object.h` | `CodeObject` 上的 JIT 缓存字段 |
| `src/CMakeLists.txt` | 把 `jit/jit.c`、`jit/emit.c`、`jit/printer.c`、`jit/disasm.c` 编入 `koala_out` |

`CodeObject` 新增字段（`include/runtime/object.h`）：

```c
struct _JitEntry *jit;   /* 已编译原生代码缓存，NULL 表示未编译 */
uint32_t jit_hotness;    /* 调用热度计数 */
uint8_t  jit_tried;      /* 是否已尝试编译（含失败），避免重复尝试 */
```

## 4. 执行模型

### 4.1 原生函数签名

```c
void fn(TValue *regs, TValue *out, KoalaState *ks);
```

- `regs`：调用方的寄存器文件，即 `cf->locals`，含 `nlocals` 个 `TValue`。
- `out`：返回值写回位置（调用方栈上的 `TValue result`），永不为 NULL：
  void 函数也写 `*out`。
- `ks`：当前 `KoalaState`（`__ks()` 是线程局部变量，机器码不便读取，故作参数传入）；
  纯自递归直调用它做栈检查和 helper 前的 `stack_top` 设置（§5.3）。

入参已由 `kl_call_code` 用 `memcpy` 拷入 `cf->locals`，因此 JIT 无需再搬参数。

### 4.2 TValue 布局与寄存器寻址

`TValue` 为 16 字节：`{ uintptr_t tag; union { int64_t ival; ... }; }`。
约定 **RBX 恒为寄存器文件基址**，则寄存器 `i`：

- tag 位于 `[RBX + i*16]`（`TAG_DISP(i)`）
- payload 位于 `[RBX + i*16 + 8]`（`VAL_DISP(i)`）

相关 tag：`TAG_NIL=0`、`TAG_BOOL=2`、`TAG_INT64=0b1011`。

### 4.3 寄存器使用约定

- **RBX**：寄存器文件基址（callee-saved，序言保存、尾声恢复）。
- **RBP**：帧指针；`[RBP-16]` 保存 `out`，`[RBP-24]` 保存 `ks`，
  `[RBP-40, RBP-24)` 是一个 scratch `TValue`（void 自递归直调的返回值）。
- **RAX / RCX / RDX**：caller-saved 临时寄存器（scratch），所有运算临时值只用这三个。
- 不使用 RSP/R12 作为内存基址（避免 SIB 字节），编码器只支持 RBX/RBP 基址。

### 4.4 序言 / 尾声

```asm
; prologue: rdi = regs, rsi = out, rdx = ks
push rbp
mov  rbp, rsp
push rbx
sub  rsp, 32
mov  [rbp-16], rsi    ; 保存 out 指针
mov  [rbp-24], rdx    ; 保存 ks
mov  rbx, rdi         ; 寄存器文件基址

; ... body（rsp ≡ 8 mod 16，helper 调用前 sub rsp,8）...

; epilogue（返回值须已写入 *out）
add  rsp, 32
pop  rbx
pop  rbp
ret
```

`OP_RET*` 先把结果 `TValue` 写入 `*out`（`rcx = [rbp-16]`），再执行尾声。

## 5. 字节码白名单与 lowering

解码宏与 `eval.c` 完全一致：`I_OP(i)=i>>24`，`I_VAL(i,shr,mask)` 取位段，
`sval(val,bits)` 做符号扩展（等价于解释器的 `I_SVAL` / `(int)(inst<<20)>>20`）。

### 5.1 操作数字段位置（务必区分）

| 格式 | 字段 | 用于 |
| --- | --- | --- |
| RRR | rd@16, rs@8, rt@0 | 算术、**产生 bool 的比较** |
| RRImm | rd@16, rs@8, imm@0 | 算术/比较的立即数形式 |
| RROff | **rs@16, rt@8, off@0** | **融合比较跳转** `OP_JMP_INT_*` |
| RImmOff | **rs@16, imm@8, off@0** | 融合比较跳转立即数形式 |
| 12-bit | rd@12(12), rs@0(12) | `OP_MOVE`、`OP_INT_NEG/NOT` |
| RET | rs@0(12) | `OP_RET` |
| LOAD_INT_IMM | rd@16(8), ti@12(4), imm@0(12,signed) | 整数立即数加载 |
| RET_INT_IMM | ti@16(8), imm@0(16,signed) | 返回整数立即数 |
| JMP / JMP_TRUE/FALSE | off@0(16,signed)；JMP_TRUE/FALSE 另有 rs@16(8) | 跳转 |

> **关键陷阱**：产生 bool 的比较（`OP_INT_LT` 等）用 RRR，目的寄存器 rd 在 @16；
> 而融合跳转（`OP_JMP_INT_LT` 等）用 RROff，第一个源寄存器 rs 在 @16、没有 rd。
> 两者位段含义不同，翻译时分别走不同路径（见 `jit.c` 的 default 分支）。

跳转目标统一为 `pc + 1 + off`（解释器取指后 `pc` 已自增，`pc += off`）。

### 5.2 lowering 对照表

| 字节码 | 语义 | 机器码（省略 tag 写回与 mov 细节） |
| --- | --- | --- |
| `OP_MOVE` | rd = rs（16B 整体拷贝） | 拷 tag+payload 两个 8 字节 |
| `OP_LOAD_INT_IMM` | rd = imm，tag=ti | `mov [rd.tag],ti; mov [rd.val],imm` |
| `OP_INT_ADD/SUB/AND/OR/XOR`(+`_IMM`) | rd = rs OP rt/imm | `mov rax,[rs.val]; <alu> rax,[rt.val]/imm; mov [rd.val],rax` |
| `OP_INT_MUL`(+`_IMM`) | rd = rs * rt/imm | `imul rax,[rt.val]` 或 `imul rax,rcx` |
| `OP_INT_DIV/MOD`(+`_IMM`) | rd = rs / 或 % rt/imm（有符号） | `rcx=[rt.val]`；`rcx==0` → `kl_jit_raise_at(DIV_ZERO)` + 错误尾声；`rcx==-1` → 商 `neg rax`（INT64_MIN 回绕）、余 0（`idiv` 会 #DE）；否则 `cqo; idiv rcx`。`_IMM` 形式 imm 为 0/-1 时 bail（isel 不会产生） |
| `OP_INT_SHL_IMM/SHR_IMM` | rd = rs << / >> imm | `shl/sar rax,imm8`（SHR 用算术右移，与 int64 `>>` 一致） |
| `OP_INT_NEG/NOT` | rd = -rs / ~rs | `neg/not rax`；NEG 恒写 INT64，NOT 源 tag 为 UINT64 时写 UINT64、否则 INT64（同解释器） |
| `OP_INT_EQ/NE/LT/LE/GT/GE`(+`_IMM`) | rd = (rs CC rt/imm) → bool | `cmp; setcc al; movzx rax,al; mov [rd.val],rax; tag=BOOL` |
| `OP_JMP` | pc += off | `jmp rel32`（回填） |
| `OP_JMP_TRUE/FALSE` | rs.val != 0 / == 0 则跳 | `cmp [rs.val],0; jne/je rel32` |
| `OP_JMP_INT_*`(+`_IMM`) | rs CC rt/imm 则跳（融合） | `mov rax,[rs.val]; cmp ...; jcc rel32` |
| `OP_RET` | 返回 rs | 写 `*out` = rs 的 tag+payload；尾声 |
| `OP_RET_INT_IMM` | 返回立即数 | 写 `*out` = {ti, imm}；尾声 |
| `OP_RET_VOID` | 返回 nil | 写 `*out` = {NIL, 0}；尾声 |
| `OP_CALL`（flag=0） | rd = fn(args) | helper 调用或自递归直调，见 §5.3 |
| `OP_UINT_ADD/SUB/MUL/AND/OR/XOR`(+`_IMM`) | rd = rs OP rt/imm，tag=UINT64 | 同 INT 族 payload 指令（补码运算与符号无关），tag 写 UINT64 |
| `OP_UINT_DIV/MOD`(+`_IMM`) | 无符号除 / 模 | 除数 0 同 INT 族抛 "division by zero"；`mov rax,[rs.val]; xor edx,edx; div rcx`（`_IMM` 的 imm 0 bail） |
| `OP_UINT_SHL_IMM/SHR_IMM` | rd = rs << / >> imm（逻辑） | `shl/shr rax,imm8`，tag=UINT64 |
| `OP_INT/UINT_SHL/SHR`（寄存器） | rd = rs << / >> rt | 先 `kl_jit_shift_check`（§5.4），再 `shl/sar/shr rax,cl` |
| `OP_UINT_LT/LE/GT/GE`(+`_IMM`)、`OP_UINT_EQ_IMM/NE_IMM` | 无符号比较 → bool | `cmp; setcc`（无符号 cc） |
| `OP_JMP_UINT_LT/LE/GT/GE`(+`_IMM`) | 无符号融合跳转 | `cmp; jcc`（无符号 cc） |
| `OP_LOAD_UINT_IMM` | rd = uimm12，tag=ti | 同 LOAD_INT_IMM，imm 零扩展 |
| `OP_LOADK` | rd = const_pool[idx] | 编译期快照池槽，嵌 tag+payload |
| `OP_LOAD_TAG` | rd = 小常量表[tag] | 编译期取 `kl_jit_tag_value`，嵌 tag+payload |
| `OP_RET_UINT_IMM` | 返回 uimm16 | 写 `*out`={ti,uimm}；尾声 |
| `OP_RET_TAG` / `OP_RET_CONST` | 返回小常量 / 池常量 | 嵌 tag+payload 写 `*out`；尾声 |
| `OP_INT_CAST` | rd = cast(rs, dst_ti, mode) | 调 `do_int_cast`（trap=abort，wrap=截断） |
| `OP_FLOAT_ADD/SUB/MUL/DIV` | rd = rs OP rt（double） | `movsd xmm0,[rs.val]; <sse> xmm0,[rt.val]; movsd [rd.val],xmm0`，tag FLOAT64 |
| `OP_FLOAT_MOD` | rd = fmod(rs, rt) | 调 `fmod`（xmm0/xmm1→xmm0） |
| `OP_FLOAT_NEG` | rd = -rs | GPR 翻转符号位 `xor rax, 0x8000…0`（与 C 取负位等价） |
| `OP_FLOAT_EQ/NE/LT/LE/GT/GE` | rd = (rs CC rt) → bool | 内联 `ucomisd` + setcc（EQ/NE 合并 PF），存 rax + BOOL |
| `OP_JMP_FLOAT_*` | rs CC rt 则跳（融合） | 内联 `ucomisd` + jcc（EQ 先 `jp` 跳过，NE 加 `jp target`） |
| `OP_FLOAT_CAST/FLOAT_TO_INT/INT_TO_FLOAT` | 浮点相关 cast | 调 `do_float_cast`/`do_float_to_int`/`do_int_to_float` |
| `OP_LAND/LOR` | rd = rs && / \|\| rt（bool） | bool payload 0/1，`and/or rax,[rt.val]`，tag BOOL |
| `OP_LNOT` | rd = !rs（bool） | `xor rax,1`，tag BOOL |
| `OP_NUM_*`（16 个） | 泛型数值协议（trait/any 操作数） | 调 `kl_jit_num_arith`/`kl_jit_num_cmp`（slot 分派），见 §5.6 |
| `OP_GET_FIELD/SET_FIELD` | 对象字段读写 | **内联**：obj=payload，`fields[off]` 在 obj+16+off*16，见 §5.7 |
| `OP_GLOBAL_GET/SET` | 全局读写 | **内联**：编译期解析 `&values[idx]`（含外部全局），load/store |
| `OP_NEW` | 建对象 | 调 `kl_jit_new(tp)`（tp 编译期解析） |
| `OP_BUILD_INTERN` | 建 tuple/list/dict 等 | 调 `kl_jit_build_intern(tag,count)`（用 stack_top） |
| `OP_SEQ_GET/SET`（含 `_IMM`/`_SLICE`） | 容器索引/切片 | 调 `kl_jit_slot3`/`kl_jit_*_item_imm`（slot 分派） |
| `OP_LEN/CONTAINS/STR` | 协议方法 | 调 `kl_jit_slot3`（SLOT_LEN/CONTAINS/STR） |
| `OP_HASH/FMT/PRINT` | hash/格式化/打印 | 调 `kl_jit_hash`/`kl_jit_fmt`/`kl_jit_print` |
| `OP_REF_EQ_NULL/NE_NULL`、`OP_JMP_REF_*_NULL` | nil 检查 | **内联**：`cmp tag,0; setcc/jcc` |
| `OP_MOVE_TRUE` | 条件 move | **内联**：`cmp [rs.val],0; je skip; copy` |
| `OP_MAKE_INTF` | 值 → 接口值（itab 取 rs 类型的 `itables[idx]`） | 调 `kl_jit_make_intf(dst,src,idx)` |
| `OP_UPCAST_INTF` | 接口值上转父 trait | **内联**：`rax=[rs.tag]`（itab）`->parents[idx]`，payload 原样 |
| `OP_NIL_CHECK` | `x!` | **内联**：`cmp [rs.tag],TAG_NIL; jne ok`；nil → `kl_jit_raise_at(NIL_UNWRAP)` + 错误尾声；`rd != rs` 时拷贝 |
| `OP_LOAD_FUNC` | rd = 函数对象 | 编译期从 `m->funcs[idx]` 解析，嵌 `{TAG_OBJECT, fn}`；越界/NULL bail |
| `OP_IS/AS` | 类型判断 / 转换 | 调 `kl_jit_is_as(src,tp,is_as)` |
| `OP_CALL`（flag=1/2） | trait 方法 / 函数值调用 | 调 `kl_jit_call_dyn`，见 §5.3 |
| `OP_TAIL_CALL`（flag=8\|x） | 尾调用其他函数 | 调 `kl_jit_tail_call` 后直接返回（trampoline），见 §5.3 |

比较条件码映射：有符号 EQ→`CC_E`、NE→`CC_NE`、LT→`CC_L`、LE→`CC_LE`、
GT→`CC_G`、GE→`CC_GE`；无符号 LT→`CC_B`、LE→`CC_BE`、GT→`CC_A`、
GE→`CC_AE`（EQ/NE 共用 `CC_E`/`CC_NE`）。uint 的 EQ/NE 寄存器形式复用
`OP_INT_EQ/NE`（位比较等价），故只有 `OP_UINT_EQ_IMM/NE_IMM`。

`OP_JMP_TRUE/FALSE` 解释器用 `to_raw_bool`，对 bool 即取 `ival`（0/1），
故机器码直接比较 payload 与 0 等价。

### 5.3 调用与错误传播

**flag=0 普通调用**：被调者是 koala 代码（`IS_CODE`）时走 **JIT→JIT 直调**
（见下文），否则（C 函数，或直调的慢路径）经运行时 helper：

```c
TValue kl_jit_call_helper(TValue *args, TValue *out, Object *fn, int nargs,
                          uint32_t pc);
```

- `fn` 在编译期从 entry table 解析并以 imm64 嵌入；`args = RBX + nlocals*16`
  （VM 滑动窗口约定，等于此处的 `ks->stack_top`）；`out = RBX + rd*16`
  （rd=0xFFF 的 void 调用传 NULL）。
- `pc` 是该 `OP_CALL` 的绝对字节码 pc（`start_pc + 局部 pc`）。helper 先写
  `ks->cf->pc = pc`，等价于解释器的 `SAVE_PC`，供 traceback 定位源码行。
- helper 内部分派与解释器一致：`IS_CFUNC → kl_call_cfunc`，否则
  `kl_call_code`（被调函数已 JIT 则走原生，否则解释器）。
- helper **返回被调函数的 TValue**（System V：tag 在 rax、payload 在 rdx）。
  native 侧随后 `cmp rax, TAG_ERROR; je 错误尾声`，对齐解释器 OP_CALL 的
  `if (is_error(&ret)) goto error;`——void 调用也能传播错误。
- 错误时 helper 调用 `trace_here(ks->cf)` 为当前 JIT 帧记录 traceback 条目
  （native 错误尾声做不到这一点），然后错误尾声把 `error_value`
  {TAG_ERROR, -1} 写入 `*out` 并返回，逐层传播与解释器完全一致。

**JIT→JIT 直调**（`emit_direct_call`，阶段 2）：非 self-only 函数里 flag=0 调用
koala 代码时，native 直接建帧并 `call` 被调者的原生入口，绕过 helper 与
`kl_call_code`：

1. **入口守卫**：逐个实参检查 tag（等价 `jit_params_ok`）；不通过走慢路径。
2. **栈检查**：`RBX + (nlocals + cn + cmax)*16 <= ks->stack_limit`（cn/cmax 为被调者
   的 nlocals / max_call_args）且 `rsp > ks->c_stack_limit`，否则抛 "stack overflow"。
3. 运行期读 `co->jit`（NULL 即未编译 → 慢路径；首次调用由慢路径触发编译）。
4. **建帧**（同 `_new_frame` + `_eval_frame` 的链接）：从 `ks->cf_cache` 取帧（空时
   调 `kl_jit_refill_cf` 补一个，深递归不退化到 helper），填 code / module /
   pc=0 / nlocals / tailcalls=0 / ks / `locals = RBX + nlocals*16`（实参已在此），
   `back = ks->cf; ks->cf = 帧`，`stack_top = locals + cn`。
5. `call co->jit->fn(locals, out, ks)`。**`out` 不能别名被调者的帧**：`rd` 在调用区
   （`rd >= nlocals`）时就是被调者的某个实参槽，待处理尾调用的标记会覆盖它，故此时
   `out` 用机器帧 scratch 槽，返回后再拷到 `rd`；void 调用同样用 scratch。
6. 结果为 `JIT_TAG_TAILCALL` 时调 `kl_jit_tail_continue(out)` 跑完 trampoline。
7. **解链**：帧回 `cf_cache`，`ks->cf = back`，`stack_top = RBX + nlocals*16`。
8. 结果为 `TAG_ERROR` 时调 `kl_jit_self_error(pc)` 记本层 traceback，走错误尾声。

每次直调都有真实 `CallFrame` 入帧链、locals 在 VM 栈上，traceback 与解释器逐帧一致，
GC 也能按帧链枚举（§14.1）。`bench` 中 ack/fib/tak 是 self-only，不受影响；
非自身调用为主的代码约 1.75× 加速（§13）。

**纯自递归直调**：若函数内**所有**调用都指向自身（`is_self_only`），
`OP_CALL` 降为直接 near call 到本函数原生入口（偏移 0），绕过
`kl_call_code` 的帧机制；各层共享外层 `CallFrame`（`ks->cf`）。

- **栈检查**：调用前检查新一层的寄存器 + 调用区
  （`RBX + (2*nlocals + max_call_args)*16`）不超过 `ks->stack_limit`，且
  `rsp > ks->c_stack_limit`（线程原生栈底 + 256KB 余量，`kl_new_ks` 用
  `pthread_getattr_np` 求得；NULL 时不检查）。任一越界调
  `kl_jit_raise_at(JIT_RAISE_STACK_OVERFLOW)` 抛 "stack overflow"
  （解释器目前只 ASSERT，见 Koala_TODO）。
- **`stack_top` 惰性维护**：直调不推进 / 恢复 `ks->stack_top`（每次调用省两次
  读写）。self-only 函数的**每个 helper 调用前**（`emit_call_abs`）把它设为本层
  调用区 `RBX + nlocals*16`（只用 rax/r11，不碰已装好的参数），helper 在其上
  建值或为 koala 代码压帧；两个原生入口（`kl_call_code`、`OP_TAIL_CALL`）在
  原生函数返回后把 `stack_top` 复位为 `locals + nlocals`。
- **错误传播**：返回后检查结果 tag，`TAG_ERROR` 时调
  `kl_jit_self_error(pc)`（设 `cf->pc` + `trace_here`），为该层记录一条
  traceback（与解释器逐帧一致），再走错误尾声。
- **void 调用**（rd=0xFFF）：`out` 指向本机器帧的 scratch 槽。

代价：`bench/fib.kl` 约 +3%（0.79s → 0.81s，debug 构建）。

traceback 打印会把连续相同的条目折叠为
`[Previous line repeated N more times]`（最多原样打印 3 条，同 Python）。

**flag=1（trait 方法）/ flag=2（函数值）** 经 `kl_jit_call_dyn` 运行期解析，
与解释器一致：flag=1 取接收者 `args[0].itab->methods[index]`；flag=2 取寄存器中的
函数值，不可调用时抛 "function value is not callable" 并 trace；之后同
`kl_jit_call_helper`。

**尾调用其他函数（`OP_TAIL_CALL` flag=8|x）用 trampoline**，不增长 C 栈
（`test_tailcall_other` 的 1e6 层互递归在 JIT 下仍成立）：

- native 调 `kl_jit_tail_call(args, out, fn, callee, index, site)`
  （`site = pc<<8 | nargs`；x=0 时 fn 编译期解析，x=1/2 同上运行期解析），
  随后检查 TAG_ERROR，否则直接走尾声（`*out` 已由 helper 写好）。
- 被调者为 C 函数：直接调用，结果写 `*out`。
- 被调者为 koala 代码：helper 照解释器 `OP_TAIL_CALL` **复用当前帧**——参数
  `memmove` 到 locals，改 `cf->code/module/nlocals`，`cf->tailcalls++`，复位
  `stack_top`，`*out = {JIT_TAG_TAILCALL, nargs}`。
- 原生入口循环处理该标记：新 `cf->code` 已编译且参数通过守卫则继续调其原生代码；
  否则回落解释器继续同一帧。`kl_call_code` 与 JIT 直调共用
  `kl_jit_tail_continue`（回落时临时解链后 `_eval_frame`，再重新挂回 `ks->cf`
  交给调用方解链）；解释器 `OP_TAIL_CALL` 则 `goto ext_tailcall`。
- 含此类尾调用的函数不是 self-only（自递归直调的各层没有自己的 CallFrame，
  无从复用）。

**方法**：owner 为 TypeObject 时取 `tp->module` 作为字节码与函数表所在模块
（同 `_new_frame` 设 `cf->module`），按普通函数编译。

### 5.4 int 类型规则（与 number.kl 对齐）

- **int64 家族隐式 widening**：int8/16/32、uint8/16/32、int64 做算术都提升为
  int64，走 `OP_INT_*` 族，字节码里**不出现 cast**（payload 存储时已符号/零扩展）。
  窄 tag 只存在于存储/参数上；收窄靠显式 `OP_INT_CAST`（`int8(x)` 构造、`as` 窄化）。
- **uint64 独立**：只有 uint64 走 `OP_UINT_*` 族；uint64 ↔ int64 边界需显式 cast。
  因此「uint 覆盖」实质就是 uint64 覆盖，窄 uint 无需独立 lowering。
- **立即数符号性**：INT 族 `_IMM` 用 `I_SVAL`（符号扩展），UINT 族 `_IMM` 用
  `I_VAL`（**零扩展**）。imm ≥ 128 时两者结果不同，翻译时按 `is_unsigned_op` 区分。
- **结果 tag**：INT 族算术→INT64，UINT 族算术→UINT64，比较→BOOL；`OP_INT_CAST`
  由 `do_int_cast` 写 dst_ti。
- **寄存器移位的 raise 路径**：`_IMM` 移位不校验移位量；寄存器形式
  （`OP_INT/UINT_SHL/SHR`）经 `kl_check_shift_count`，count ≥ 64 时 raise TAG_ERROR。
  JIT 用 `kl_jit_shift_check(count, pc)` 镜像：写 `cf->pc`、调 `kl_check_shift_count`、
  失败 `trace_here`，返回 0 使 native 跳错误尾声。这是 int 覆盖中唯一的非调用 raise 路径。
- **入口守卫**：`jit_params_scalar` 放行 `is_int || is_uint || is_bool || is_float`（解释器标量
  opcode 操作数 assert 的并集），挡掉 float/对象 payload；静态类型保证实际 tag。
- **unary 暂不扩展**：`OP_INT_NEG/NOT` 维持现状，语言侧 unary 语义未整改
  （见 Koala_TODO §94/§75）；NOT 的 tag 分歧待整改后一并镜像。

### 5.5 float / bool 规则

- **float64 是主浮点类型**；float16/32/bfloat16 同窄 int 模式，运算作用于 float64，
  收窄靠 `OP_FLOAT_CAST`。payload 存 `fval`（与 ival 同一 8 字节槽）。
- **算术**（add/sub/mul/div）内联 SSE2（`movsd` + `addsd/subsd/mulsd/divsd`），结果 tag FLOAT64。
  TValue 镜像模型下值在 op 之间都 spill 到内存，XMM0 作 scratch 安全。
- **neg** 用 GPR 翻转符号位（`xor` 0x8000…0），与 C `-fval` 位等价（含 NaN/±0），无需 SSE。
- **mod** 调 libm `fmod`（xmm0/xmm1 入参，xmm0 返回）。
- **比较**（bool 与融合跳转）内联 `ucomisd`，语义同 C 运算符：unordered（NaN）时
  ZF=PF=CF=1。GT/GE 用 `a`/`ae`（unordered 为假）；LT/LE 交换操作数后同样用 `a`/`ae`；
  EQ = ZF && !PF，NE = !ZF || PF。分支形式：EQ 先 `jp` 跳过再 `je target`，NE 为
  `jp target; jne target`（原 `kl_jit_float_cmp` helper 已删除）。
- **bool** payload 恒 0/1：`LAND/LOR` 即位 and/or，`LNOT` 即 `xor 1`，tag BOOL。
- **cast**：`OP_FLOAT_CAST/FLOAT_TO_INT/INT_TO_FLOAT` 调 do_cast.c 的对应 helper
  （与 `OP_INT_CAST` 同 5 参约定，trap 模式 abort）。
- **已知 VM bug（仅镜像不修）**：float 取反分支在 NaN 下语义错误（Koala_TODO §102），
  JIT 差分与解释器一致；测试不钉该 quirks。

### 5.6 泛型数值协议（OP_NUM_*）

操作数是 trait/any 类型（非具体内建）时，isel 发射 `OP_NUM_*`（16 个：算术 10 + 比较 6），
运行期按 slot 分派。JIT 镜像：

- **算术**（add/sub/mul/div/mod/shl/shr/and/or/xor）调 `kl_jit_num_arith(a, b, slot, pc)`
  → `kl_call_slot(args, 2, slot)`，与解释器 `kl_call_add` 等一致。
- **比较**（eq/ne/lt/le/gt/ge）调 `kl_jit_num_cmp(a, b, slot, pc)`，**先镜像解释器的
  primitive 快速路径**（int/float/uint，bool 仅 eq/ne），再回退 slot。快速路径是正确性必需：
  内建类型没有 cmp slot，直接走 slot 会 raise。
- 两个 helper 都写 `cf->pc` 并在出错时 `trace_here`（同 `kl_jit_call_helper`），native 侧
  检查 TAG_ERROR 跳错误尾声。

**性能结论（实测）**：v1 下 OP_NUM_* 仅 ~1.2× 加速（`test/test-bench/test_num.kl`：
num_add 87.8→73.7ms、num_cmp 97.1→82.6ms）。瓶颈是运行期 slot 分派（`kl_call_slot`），
JIT 与解释器都要付；JIT 只省了字节码 fetch/decode。泛型代码的大幅提速需去虚化 /
inline cache（Yowie/Bunyip）。

**测试注意**：koala 默认开 inline，trait helper 会被内联进调用方；验证 OP_NUM_* 独立 JIT
需 `--no-inline`（见 `test/test-run/test_num_ops.kl` 的 JIT-DAG RUN）。含 trait 局部累加
或调用点 widening 会引入 `make_intf`（B 组，尚未覆盖），bench 用「num 结果立即被比较消费」
的写法避开。

### 5.7 对象 / 容器 / 字符串 / 引用（A 组）

- **入口守卫放宽**：`jit_params_ok` 现接受 scalar + ref + intf（对象参数），否则取对象的
  热函数会被守卫挡回解释器。静态类型保证实参类型，守卫只挡 error/no-value 杂散 tag。
- **字段访问内联**：`to_obj()` 对 ref/intf 都返回 payload（obj 指针），`InstObject.fields[off]`
  在 `obj + 16 + off*16`（OBJECT_HEAD = _type + _nfields 共 16 字节）。GET/SET_FIELD 直接
  load/store，无 call——这是 A 组最大的收益点。
- **全局访问内联**：编译期解析 `&m->values[idx]`（idx≥num_globals 时经 GlobalObject 解析到
  外部模块的 values），嵌地址 load/store。全局数组运行期不 resize，地址稳定。
- **构造**：NEW 调 `kl_jit_new(tp)`（tp 编译期从 types[idx] 解析）；BUILD_INTERN 调
  `kl_jit_build_intern`（用 `ks->stack_top` 上的参数）。
- **容器/协议**：SEQ_*/LEN/CONTAINS/STR/HASH/FMT/PRINT 走 slot 分派 helper（`kl_jit_slot3`
  通用 3 参 + 专用 imm/hash/fmt/print），都带 cf->pc + 出错 trace。
- **nil 检查内联**：REF_EQ_NULL/NE_NULL 即 `cmp tag,0; setcc`；JMP_REF_*_NULL 即 `cmp; jcc`。
  MOVE_TRUE 内联条件 move（本地 fixup 跳过 copy）。
- **排除**：`OP_REF_EQ/NE` 在解释器里是 `OP_NYI`（会 abort），非真实运行 opcode，不覆盖。

**性能结论（实测，`test/test-bench/test_obj.kl`）**：字段循环 72.6→13.5ms（**~5.4×**，内联
字段访问收益大）；list 索引循环 79.6→58.2ms（~1.4×，seq_get 走 slot 分派，收益温和，同
OP_NUM_*——去虚化可进一步提速）。

**测试注意**：可选类型 `return v`（解包）会发 `nil_check`（B 组），故 nil 检查的 JIT 用例
用 `return v == nil`（只发 ref_eq_null，不解包）避开。见 `test/test-run/test_obj_ops.kl`。

## 6. 两遍汇编与跳转回填

单遍发射时跳转目标的原生地址尚不可知，采用 **pc→offset 映射 + fixup 列表**：

1. 发射序言。
2. 顺序翻译每条指令：先记录 `pc_map[pc] = 当前 buffer 偏移`，再发射机器码。
   遇到跳转，发射 `jmp/jcc rel32` 占位（disp=0），并把
   `{patch_off = disp 的 buffer 偏移, target_pc}` 记入 `fixups`。
   错误跳转的 `target_pc` 用哨兵 `JIT_ERR_TARGET`（-1）。
3.  body 之后发射**错误尾声**（写 error_value 到 `*out` + 尾声），记录其
   原生偏移 `err_off`。
4. 回填：`rel32 = pc_map[target_pc] - (patch_off + 4)`
   （+4 因为 rel32 相对的是跳转指令的**下一条**地址）；
   哨兵目标回填为 `err_off`。
5. 校验 `target_pc ∈ [0, n)`，越界则判定失败、放弃编译。

`pc_map` 按指令数 `n` 预分配；`fixups` 初始容量 `n`，按需倍增（一条指令可发射多个
跳转，如直调的栈溢出 / 错误出口）。

## 7. 热度计数与分派

集成点在 `eval.c::kl_call_code`：

```c
if (!co->jit && !co->jit_tried) {
    if (++co->jit_hotness >= kl_jit_threshold()) {
        co->jit = kl_jit_compile(co);
        co->jit_tried = 1;          /* 无论成败，只尝试一次 */
    }
}
TValue result;
if (co->jit && jit_params_ok(cf->locals, nargs)) {
    cf->back = ks->cf;                  /* 链接帧链，与 _eval_frame 一致 */
    ks->cf = cf;
    co->jit->fn(cf->locals, &result, ks); /* 原生路径 */
    ks->stack_top = cf->locals + cf->nlocals; /* 自递归直调不维护 stack_top */
    if (result.tag == JIT_TAG_TAILCALL)  /* 尾调用已改写本帧（§5.3） */
        kl_jit_tail_continue(&result);
    ks->cf = cf->back;
} else {
    result = _eval_frame(ks, cf);       /* 解释器路径 */
}
```

- **帧链链接**：raise 路径（`_raise_exc_fmt` 读 `ks->cf->code`）与
  `kl_call_slot`（读 `ks->cf->code->cs.max_call_args`）都依赖 `ks->cf`
  指向当前帧，故原生路径必须与 `_eval_frame` 一样链接/解链帧。

- **compile-once**：`jit_tried` 闩锁保证每个 `CodeObject` 至多编译一次；
  失败（返回 NULL）后永久走解释器，不反复尝试。
- **入口 tag 守卫**：`jit_params_ok` 要求所有实参都是标量（int/uint/bool/float）、
  引用或接口值，否则即使已编译也走解释器（只挡 error/no-value 等异常 tag）。
- **编译期白名单**：`kl_jit_compile` 接受自由函数与方法（owner 为 TypeObject 时
  取其 module），翻译中遇到任何非白名单 opcode 立即 `failed=1` 放弃。阶段 1 后
  `test/test-run` + `bench` 的 4474 个函数全部可编译（`KOALA_JIT_STATS`，§11）。
- 阈值默认 1（首次调用即编译并在该次调用启用），可用环境变量调整。
- 没有 OSR 时阈值只能是 1：只调用一次、里面跑热循环的函数（`main`）只有在第一次调用
  时编译才能用上 JIT。计划中的 OSR 与回边计数见 §17。

## 8. 内存管理（W^X）

阶段 3 起代码放在共享的分块代码区（§16）：函数按 16 字节对齐顺序分配进 chunk，
写入时只把涉及的页翻成 RW，`memcpy` 后翻回 RX，任何时刻没有页同时可写可执行。
`JitEntry` 的 `mem`/`mem_size` 记录代码在 arena 中的起点与长度；代码进程级存续、
不回收，`kl_jit_free` 只释放 `JitEntry`（与 v1 一样，`CodeObject` 不会被卸载）。

## 9. 正确性保证

1. **逐条对齐解释器**：字段位置、符号扩展、tag 写回、分支条件、跳转目标
   全部与 `vm_ops.h` 核对一致（见 §5）。
2. **有/无符号分离**：uint 的 div/mod/比较/跳转是**独立 opcode**
   （`OP_UINT_*`、`OP_JMP_UINT_*`），分别降为 `div`/无符号 cc（§5.4），
   有符号的 `idiv`/`CC_L` 等绝不会误用于无符号语义。
3. **乘法/溢出**：`OP_INT_MUL` 解释器就是 C int64 相乘（回绕），`imul` 一致。
4. **除法**：`idiv` 向零截断，与 C `/`、`%` 一致；除数 0 抛
   "division by zero"，有符号除数 -1 走 `neg`/0（`INT64_MIN / -1` 不触发 #DE），
   均与解释器一致。
5. **差分测试**（`KOALA_JIT=0` vs 默认）：
   - `bench/jit_diff.kl`：13 个函数覆盖 add/sub/mul、and/or/xor、div/mod、
     移位、neg/not、六种比较（bool 与融合）、循环、`&&`/`||`、三种 return，
     全部编译成功，输出**逐字节一致**。
   - `bench/jit_edge.kl`：负数除法、溢出回绕、大移位结果一致；
     `udiv`/`fadd` 正确 bail，`caller` 经 helper 调用 `helper`，输出仍一致。
6. **错误传播与帧链**：helper 调用后检查 TAG_ERROR 并走错误尾声，
   `cf->pc`/`trace_here` 与解释器逐帧对齐；lit 的 panic/traceback 用例
   （test_ut_fail、test_should_panic_no_msg、test_traceback_* 等）
   在 JIT 开启下全部通过。

## 10. 局限与 v2 路线

- **部分调用仍经 helper**：flag=0 调 koala 代码已是 JIT→JIT 直调（§5.3）；flag=1/2
  调用、调 C 函数、非自调用尾调用仍经运行时 helper（后者经 trampoline）。
- **自递归帧不入帧链**：自递归直调绕过 `kl_call_code`，递归各层共享外层帧
  （`ks->cf`）。各层出错时以 `kl_jit_self_error` 在同一 cf 上逐层 `trace_here`，
  traceback 与解释器一致；但若该 cf 曾被解释器的非自调用尾调用复用
  （`cf->tailcalls > 0`），`(... N tail calls ...)` 标记会随每层重复出现。
  v2 若放宽 self-only 限制，需为递归层补建帧。
- **无寄存器分配**：寄存器驻留内存，每次运算都 load/store。v2 可把热点寄存器
  提升到物理寄存器，并做 int64 拆箱（方案 B）。
- **无 deopt**：v1 用编译期 bail 替代运行期去优化，简单但粒度粗
  （一条不支持即整函数放弃）。v2 可考虑按基本块编译或运行期守卫 + deopt。
- **数值与对象已覆盖**：int64/uint64/窄整型/float64/bool（§5.4–5.6）+ 对象/容器/字符串/
  引用/接口（A/B 组，§5.7）。
- **代码缓存不回收**：进程级常驻，长生命周期多函数场景需 LRU/引用计数。

### 10.1 v1 opcode 覆盖定格（Drop Bear）

以 `vm_ops.h` 的真实 `TARGET` handler 为准（173 个），JIT 覆盖了 VM 支持的**全部可执行
opcode**，仅 3 个例外：

| op | 原因 | 处理 |
| --- | --- | --- |
| `OP_REF_EQ` / `OP_REF_NE` | 解释器里是 `OP_NYI`（abort），非真实运行 op | 不覆盖（VM 未实现） |
| `OP_UNREACHABLE` | 真实 op，panic 抛 "unreachable code executed" | **有意不覆盖**：含它的函数 bail 回解释器，语义无损 |

**与 `opcode_list.h` 全量核对**（排除 IR 伪指令 `OP_IR_*`/`OP_BINARY_*`/`OP_UNARY_*` 及
结构性 `OP_DATA`/`OP_WIDE`）：178 个非 IR opcode 中，VM 自身尚未支持的有——

- **对象同一性比较**：`OP_REF_EQ`/`OP_REF_NE`（NYI）+ `OP_JMP_REF_EQ`/`OP_JMP_REF_NE`
  （无 handler）。即对象 `==`/`!=` 按引用比较这一语言特性 VM 还没实现，JIT 自然不覆盖。
- `OP_DOWNCAST_INTF`：已定义但编译器未发射、VM 无 handler（保留 op）。
- `OP_DATA`（调用 payload 数据槽）/`OP_WIDE`（宽操作数前缀）：结构性，非可执行指令，无需 handler。

结论：**JIT 没有落后于 VM**——VM 能跑的 opcode JIT 都能跑（UNREACHABLE 有意除外）；
剩余缺口是 VM/语言层面的对象同一性比较，属 VM 待实现特性，非 JIT 欠账。

## 11. 环境变量

| 变量 | 作用 | 默认 |
| --- | --- | --- |
| `KOALA_JIT=0` | 关闭 JIT（全部走解释器） | 开启 |
| `KOALA_JIT_THRESHOLD=<n>` | 热度阈值，调用 n 次后编译 | 1 |
| `KOALA_JIT_TRACE=1` | 开启 trace（见下）：编译成功打印 IR 风格 dump，编译失败/跳过打印 bail 原因 | 关闭 |
| `KOALA_JIT_STATS=1` | 进程退出时打印 `[jit stats] compiled N, not compiled M` 及按原因（`op NAME`、`op call flg=N` 等）排序的 bail 计数，以及 `[jit stats] code B bytes, P pages, C chunks`（§16） | 关闭 |
| `KOALA_JIT_REGCACHE=0` | 关闭写穿寄存器缓存（§15） | 开启 |
| `KOALA_JIT_CHECK_CACHE=1` | 每次命中寄存器副本前与内存比对，不符 `ud2`（§15.7） | 关闭 |
| `KOALA_JIT_CODE_CHUNK=<bytes>` | 代码区 chunk 大小（向上取整到页），测试用小值强制跨 chunk | 1MB |

这些变量均在进程内首次读取后缓存（`static int cached = -1` 惰性初始化），
运行期修改环境变量不生效。`KOALA_JIT` 仅当首字符为 `'0'` 时关闭，其余（含未设置）一律开启；
`KOALA_JIT_TRACE` 为 0/1 开关且默认关闭：未设置或首字符为 `'0'` 时关闭，其余（如 `1`）开启。

### 11.1 Trace 输出（`printer.c`）

trace 全部写往 **stderr**，不污染 stdout（差分测试仍可直接 `diff`）。

- **编译成功**：`jit_print_code` 按编译器 KLR 的 IR 打印风格输出整个函数——
  函数头 `[jit] func @name {  ;; N insns, M bytes, prologue KB`、
  基本块标号 `  %bbN:`（由 pc0 / 跳转目标 / 跳转或返回后的落点推导 leader）、
  8 空格缩进的逐条指令，以及尾部 `[jit] }`。

  每条**字节码 op** 先打印一行表头（解码后的操作数 + 原生偏移与总字节数），
  随后把该 op 发射出的原生代码**逐条反汇编**，每条原生指令单独一行，
  显示其 **hex 机器码 + x86-64 汇编**（Intel 语法），并缩进对齐到表头的 `;;` 列：

  ```
          0001: r3 = int.gt_imm r0, #0          ;; +0x023 30B
                                                ;;   48 8b 43 08              mov rax, [rbx+0x8]
                                                ;;   48 81 f8 00 00 00 00     cmp rax, 0x0
                                                ;;   0f 9f c0                 setg al
                                                ;;   48 0f b6 c0              movzx rax, al
                                                ;;   48 89 43 38              mov [rbx+0x38], rax
                                                ;;   48 c7 43 30 02 00 00 00  mov qword [rbx+0x30], 0x2
  ```

  左列为字节码 `pc` 与操作数（寄存器 `rN`、立即数 `#N`、跳转 `label %bbN`）；
  右侧每条原生指令一行「二进制 + 汇编」，与左侧 op 一一对应。`jmp`/`jcc`
  的原生目标偏移还会回映射到 IR 块标号（如 `je 0x8d  ; -> %bb6`）。
  反汇编由 `disasm.c` 完成，只覆盖 `emit.c` 产生的指令子集，**与 objdump
  逐条对齐**（差分基准的 20 个函数、536 条原生指令零差异）。x86-64 的
  opcode/mask/编码类/助记符及寄存器、条件码、分组名字表全部定义在
  `cpu_x86_64_ops.h`（仿 `opcode_list.h` 的 X-Macro 真源），`disasm.c` 用
  X-macro 展开成表后仅保留通用的「掩码匹配 + 按编码类格式化」算法，不含任何
  硬编码指令值。字节码 opcode 名字/格式表则由 `printer.c` 用 X-macro 从
  `opcode_list.h` 自带生成。二者均**不链接 `libparser`**（`__op_names`/
  `bytecode_print` 只进 `koalac`，运行时不可用）。

- **编译失败/跳过**：`jit_print_bail` 打印一行原因，含函数名、失败 `pc`、
  opcode 名与原始指令字，例如：

  ```
  [jit] bail @udiv: unsupported opcode at pc 0 (op 'uint.div', insn 0x2e000001)
  [jit] bail @BufReader$__init__: not a module free function
  ```

  覆盖所有放弃路径：非模块自由函数、无代码、空代码、不支持的 opcode、
  跳转目标越界、`mmap`/`mprotect` 失败。

## 12. 复现验证

```bash
source env.sh
export PATH=$(pwd)/build/release/bin:$PATH
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/build/release/lib

# 差分正确性
KOALA_JIT=0        koala bench/jit_diff.kl > /tmp/off.txt
KOALA_JIT_TRACE=1  koala bench/jit_diff.kl > /tmp/on.txt
diff /tmp/off.txt /tmp/on.txt        # 应无差异

# 性能
KOALA_JIT=0 koala bench/test_arith.kl   # ~1.29s
KOALA_JIT=1 koala bench/test_arith.kl   # ~0.43s
```

## 13. 性能基线（vs LuaJIT）与 Yowie 优化靶子

实测环境：release 构建，hyperfine（warmup 2 / min-runs 5），2026-11-14。
单位 ms（均值）。`vs luajit` 列 >1 表示 luajit 更快。**所有落后项的热点函数均已 JIT**
（trace 确认无 bail），落后是 v1 代码质量问题，非回退解释器。

| bench | koala JIT | koala 解释 | luajit | luajit -joff | JIT 加速 | vs luajit |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| tak | 136 | 533 | 147 | 348 | 3.9× | **koala 胜 1.08×** |
| fib | 786 | 3256 | 686 | 3265 | 4.1× | 平手（luajit 1.14×） |
| loop-sum | 68 | 290 | 46 | 247 | 4.3× | luajit 1.47× |
| test_logic_chain | 438 | 1310 | 340 | 1241 | 3.0× | luajit 1.29× |
| sum | 96 | 569 | 46 | 952 | 5.9× | luajit 2.07× |
| shuffle | 42 | 136 | 20 | 456 | 3.2× | luajit 2.12× |
| ack | 233 | 600 | 78 | 378 | 2.6× | luajit 2.99× |
| test_float_arith | 179 | 260 | 46 | 337 | 1.5× | luajit 3.88× |
| test_float_logic | 284 | 576 | 64 | 577 | 2.0× | luajit 4.46× |
| test_arith | 484 | 1418 | — | — | 2.9× | N/A（Lua 5.4 位运算语法） |
| test_bitwise | 206 | 924 | — | — | 4.5× | N/A |
| test_uint_arith | 463 | 1355 | — | — | 2.9× | N/A |

> 3 个位运算基准的 `.lua` 用 Lua 5.4 位运算语法（`|`/`~`/`<<`），LuaJIT（5.1 基）不支持，
> 不做模拟对照。koala 解释器 ≈ luajit -joff（fib 3.26s vs 3.27s），差距全在 JIT 质量。

**阶段 2（JIT→JIT 直调）**：调用密集基准（`run_loop` 内 1e8 次调用
`add1` + 互递归 `fa`/`fb` fib(32)，`++ --no-inline`，debug 构建）0.89s → 0.51s
（解释器 1.64s）；fib / ack / tak 为 self-only，不变（0.79 / 0.24 / 0.14s）。

**阶段 3（写穿寄存器缓存，§15）**：test_float_arith 190 → 54ms（luajit 46）、
test_float_logic 233 → 110ms、loop-sum 75 → 54、sum 104 → 77、test_bitwise 215 → 163、
shuffle 86 → 76、tak 153 → 138；fib / ack / `idiv` 主导的基准不变。完整对比见 §15.8
（上表为 11-14 基线，未重测）。

### 13.1 落后归因 → Yowie(v2) 靶子

| 落后项 | 根因（v1 限制） | Yowie 对策 |
| --- | --- | --- |
| test_float_logic（4.46×） | 浮点比较 `if res>=50.0` 每迭代调 `kl_jit_float_cmp` C helper | 阶段 3 已内联 `ucomisd`（处理 PF/NaN） |
| test_float_arith（3.88×） | TValue 镜像：每迭代 movsd 读内存→addsd→写内存+写 tag | 阶段 3 写穿寄存器缓存：190→54ms（§15.8） |
| ack（2.99×） | 嵌套非尾调用每次 helper→`kl_call_code` 建帧开销 | JIT→JIT 直调 |
| sum / shuffle（~2×） | 尾循环 TValue 每迭代 load/store + tag 写 | 拆箱 + RA |
| loop-sum / logic_chain（1.3–1.5×） | TValue 镜像 load/store + tag 写（已较接近） | 拆箱 + RA |

结论：v1 已把可 JIT 的都 JIT 了，剩余差距集中在**代码质量**而非覆盖，对应 Yowie 三项
（拆箱+RA、内联浮点比较、JIT 直调，见 Koala_TODO §100）。此表作为 Yowie 的优化对照基线。

> 注：shuffle 的结果受尾调用编译器 bug 影响（Koala_TODO §104，40M 输入恰好掩盖），
> 性能数仍有效（同样跑 40M 次循环）。

## 14. GC root 枚举（前瞻）

GC 尚未落地（半自动内存模型是设计目标）。本节锁定 JIT 侧的 root 枚举约定，供 GC 实现时遵循。
**核心结论：v1 的 TValue 镜像让 root 枚举几乎白送——JIT 帧与解释器帧对 GC 完全同构。**

### 14.1 v1：保守扫 VM 栈 + 全局

值始终驻留 `cf->locals`（VM 栈上的 TValue 寄存器文件），GC 无需区分原生帧/解释帧，只扫两块：

```
1. 全局：遍历每个 module->values[0..num_values]，is_ref / is_intf 即 root。
2. VM 栈：把 [ks->stack_base, scan_top) 整段当 TValue[] 扫，is_ref / is_intf 即 root，
   scan_top = ks->stack_top + ks->cf->code->cs.max_call_args（ks->cf 为 NULL 时即 stack_top）。
```

**枚举规则（GC 实现必须遵守）**：

1. **只能整段扫 VM 栈，不能只走帧链**。纯自递归直调（§5.3）的各层共用外层 `cf`，
   深层寄存器 `[RBX_k, RBX_k + nlocals)` 不属于任何 `CallFrame`；按 `cf->locals +
   nlocals` 逐帧扫会漏掉它们。整段扫则必然覆盖（各层在 VM 栈上连续）。
2. **上界要含顶层帧的调用区**。实参先写进调用区 `[stack_top, stack_top +
   max_call_args)` 再调用（VM 约定，解释器同）：`f(A(), B())` 在构造 B 时 A 已在调用区；
   调 C 函数时实参也在 `stack_top` 之上。故扫到 `stack_top + 顶层 max_call_args`。
   非顶层帧的调用区就是下一帧的 locals，已在 `stack_top` 之下。
3. **扫描上界必须单调一致，或清零越界槽**。保守扫描把死寄存器残值也当 root；
   若某槽曾在上界之外（其指向的对象可能已被回收）后又被纳入扫描，会把悬空指针当 root。
   做法二选一：GC 每次都按规则 2 的上界扫（推荐），或 `_new_frame` / JIT 直调建帧时
   把新帧 locals 清为 nil。
4. **`stack_top` 只在 GC 点可信**。self-only 函数惰性维护 `stack_top`，仅在每个 helper
   调用前（`emit_call_abs`）设为本层 `RBX + nlocals*16`；GC 只在 helper / 运行时调用内
   触发（safepoint = 调用点），此时规则 2 的上界正确（同一函数的各层 max_call_args 相同）。
5. **原生侧无 root**：原生栈（rbp/rsp、返回地址、scratch 槽）与 rax/rdx 里的对象引用
   都只在两次 GC 点之间短暂存在（helper 返回值立即写回寄存器文件），GC 不扫原生栈。
6. **`KoalaState` 即全部入口**：VM 栈（`stack_base` / `stack_top`）、帧链（`cf`）都在
   `KoalaState` 中，JIT 不另开栈内存；`cf_cache` 里的空闲帧不是 root。

依据：
- `_new_frame` 中 `cf->locals = stack_top; stack_top += nlocals`，各帧 locals + 调用参数窗口
  在 VM 栈上连续排布，活跃区 `[stack_base, stack_top)` 恰好覆盖所有帧的活值。
- **JIT 原生栈（rbp/rsp、返回地址）里没有对象引用**——值都在 VM 栈的 `cf->locals`。
  故 GC **不扫原生栈**，只扫 VM 栈。这是镜像模型最大的便利。
- 帧链（`cf->back` / `ks->cf` / `cf->pc`，见 §7）已就位：可按 cf 链逐帧扫 `nlocals`
  （精确到帧），或直接扫整段栈（更简单）。两者皆可。
- **JIT 直调同构**（§5.3）：直调帧与 `_new_frame` 建的帧完全一样（帧取自 / 还回
  `ks->cf_cache`，`locals` 在 VM 栈上，`nlocals` 正确，入 `ks->cf` 链），JIT 不另开
  栈内存，`KoalaState` 无需新增字段。不变量：**每个 helper 调用点（即 GC 点）
  `ks->stack_top` 覆盖所有活寄存器**——非 self-only 函数的 helper 调用处
  `stack_top == cf->locals + cf->nlocals`（`kl_jit_call_helper` 中 ASSERT）；
  self-only 函数由 `emit_call_abs` 在每个 helper 前设置。

**保守 vs 精确**：v1 用**保守扫描**——`nlocals` 个槽全当潜在 root（含已死寄存器残值）。
代价是少量漂浮垃圾（对象多活一会儿），但绝对安全、零额外元数据。精确扫描需每 PC 的活跃
槽位图（LSRA 已有 liveness，可选导出），v1 不必。

**Safepoint = 调用点**：GC 只在 call 处触发（此时 `cf->pc` 已设、值已 spill 到 `cf->locals`
与参数窗口）。镜像模型下值本就在内存，任何调用点状态都一致，无需额外 spill。

### 14.2 v2（拆箱）：stack map

拆箱后活引用可能留在机器寄存器/原生栈，不在 `cf->locals`，扫 VM 栈会漏根。两条路：

| 方案 | 做法 | 取舍 |
| --- | --- | --- |
| safepoint spill | 每个调用点把活引用 spill 回 `cf->locals` | GC 扫描保持统一简单；每 call 多 spill 开销 |
| **stack map** | 每个 safepoint 发射元数据：哪些寄存器/栈槽是 ref | 高效无多余 spill；需生成/维护 stack map，GC 解析原生帧 |

LuaJIT/V8 走 stack map。建议 v2 用 **stack map**（性能优先），但**现在就锁定 safepoint 约定**
（call 点 `cf->pc` 有效、活引用位置可枚举）。v1 先按"全在 cf->locals"实现 GC，v2 再扩展。

### 14.3 C 扩展侧 root

C 扩展的 root 用设计文档的 TraceStack 机制（与 JIT 无关，另行处理）；JIT  helper 调用
C 函数期间，活值已在 VM 栈/参数窗口，C 侧临时 root 由 TraceStack 覆盖。

### 14.4 落地建议

- v1 GC 直接保守扫 VM 栈 + 全局，**零 JIT 侧改动**（镜像模型已就位），先跑通半自动内存模型。
- 现在锁定 safepoint 约定，为 v2 stack map 留接口。
- 关联：Koala_TODO §100（Yowie 拆箱）、内存管理设计目标（Koala_Language_Manual.md）。
## 15. 阶段 3：写穿寄存器缓存（Write-Through Register Cache）

> 状态：**已实现（2026-11-15）**，默认开启，`KOALA_JIT_REGCACHE=0` 关闭。在**不改变内存
> 中 TValue 语义的任何一刻**的前提下，把热点 VM 寄存器的 payload 副本留在物理寄存器里，
> 消掉每条指令的操作数 load。实现细节与设计稿的差异见 15.9，实测见 15.8。

### 15.1 现状与瓶颈（实测 trace）

v1 每条算术指令都是「load 操作数 → 运算 → store payload → store tag」四步，例如
`bench/test_float_arith.kl` 的循环体（单个 BB `%bb4`）：

```asm
; 0004: r1 = float.add r1, r2
movsd xmm0, [rbx+0x18]        ; load r1  ← 上一轮刚 store 的值
addsd xmm0, [rbx+0x28]        ; load r2
movsd [rbx+0x18], xmm0        ; store r1
mov   qword [rbx+0x10], 0x13  ; store tag
; 0005: r3 = int.add_imm r3, #1
mov   rax, [rbx+0x38]         ; load r3  ← 上一轮刚 store 的值
add   rax, 0x1
mov   [rbx+0x38], rax
mov   qword [rbx+0x30], 0xb
; 0006: jmp_int_lt r3, r0, %bb4
mov   rax, [rbx+0x38]         ; 又 load r3（上一条刚 store）
cmp   rax, [rbx+0x8]
jl    %bb4
```

**为什么慢**：`r1 = r1 + r2` 是循环携带依赖链（loop-carried）。每一轮的 `addsd` 必须等
上一轮的 `movsd [rbx+0x18], xmm0` 经 **store-to-load forwarding** 转发回来，这一步在
Intel 上约 4–5 周期，与 `addsd` 的 4 周期**串行相加**：每轮 ≈ 9 周期，而纯寄存器链只需 4
周期。整数链更夸张：`add` 1 周期，但加上转发后是 ≈ 6 周期。tag 写是立即数 store，
不在任何依赖链上，会被 store buffer 吸收——这也是之前「省 tag 写」实测无收益的原因（§2）。

结论：**真正的成本是依赖链上经过内存的那一跳**，消掉它要让值在寄存器里跨过回边。

### 15.2 为什么不做「纯基本块内缓存」

最初设想是 BB-local：块内缓存，块尾全部写回、块首全部作废。但 trace 显示热点循环体
大多**只有一个或两个 BB**，依赖链恰好跨越回边：

| bench | 循环结构 | BB-local 能否消掉链上的 load |
| --- | --- | --- |
| test_float_arith | 单 BB 自环 `%bb4` | 否：块首作废 → 每轮重 load r1/r3 |
| loop-sum | `%bb2`（判断）+ `%bb3`（体）| 否：每个 BB 内每寄存器只用一次 |
| sum / shuffle | 自尾调用 `tail_call` → 跳回 pc 0 | 否：跳回即块边界 |
| test_arith | 单 BB，r1 上 8 步链 | 部分：块内 7 次 load 可省，回边那次不行 |

即 BB-local 方案对多数基准**几乎无收益**。因此改为：**内存永远是真值（写穿），寄存器只
是可随时丢弃的副本**，副本的有效性用一个跨 BB 的前向 must 数据流决定，可以跨过回边。

### 15.3 核心原理：写穿 + 副本有效性

- **写穿（write-through）**：任何写 VM 寄存器 `rN` 的指令，照旧把 payload 和 tag
  写回 `[rbx+N*16]`。**内存中的 TValue 在每条字节码指令边界都与 v1 完全一致**。
  不变量 1（GC 枚举）、不变量 2（TValue 文件）、错误尾声、traceback、`kl_jit_refill_cf`、
  JIT→JIT 直调、自递归直调全部**零改动**、天然成立。
- **副本**：一部分 VM 寄存器（「提升寄存器」）在编译期被分到一个固定物理寄存器
  `P(rN)`。在某个程序点，若 `valid(rN)` 为真，则保证 `P(rN) == [rbx+N*16+8]`。
- **读**：缓存感知的指令读 `rN` 时，`valid` 则直接用 `P(rN)`（**省掉 load，也断开了
  经过内存的依赖链**）；否则从内存 load 进 `P(rN)` 并置 `valid`（load-on-miss）。
- **写**：结果算在 `P(rd)` 里，然后 store 回内存（store 不在依赖链上，被 store buffer
  吸收），置 `valid(rd)`。
- **作废**：副本只可能因两类事件失真——物理寄存器被破坏，或内存被「旁路」写入：
  1. **GC 点 / 任何 helper、运行时、直调、自递归 near call**：caller-saved 物理
     寄存器被破坏，且 helper 可能写任意 VM 寄存器、将来 GC 可能移动对象 → **全部作废**。
  2. **非缓存感知的指令**（如 inline 的 `get_field`、`make_intf` 等）照旧经内存读写；
     它读内存总是对的（写穿保证），但它的写不会更新副本 → **全部作废**（保守；以后可
     细化为只作废其 def）。
  3. 缓存感知指令写 `rd` 但结果在另一类寄存器（如 float 结果、`rd` 是 GPR 类）→ 实现中
     用一条 `movq` 跨类拷贝，副本**仍有效**（payload 只是 64 位，类别只影响放在哪）。

由于内存永远是真值，**作废永远是安全的**：最坏情况只是退化成 v1 的 load。唯一的
正确性风险是「该作废时没作废」（副本陈旧），15.7 用运行期自检专门覆盖它。

### 15.4 物理寄存器池与提升选择

- **GPR 池**：`RSI, RDI, R8, R9, R10`（5 个）。它们在 v1 中只出现在 helper 调用的参数
  序列里，而调用点本来就全部作废，不冲突。`RAX/RCX/RDX` 仍是运算 scratch
  （`idiv`/`cqo`/移位计数要用），`R11` 留给直调序列，`RBX/RBP` 不变。
- **XMM 池**：`XMM1–XMM7`（7 个），`XMM0` 仍是 scratch（`fmod` 等 helper 调用点本就作废）。
- 全部是 caller-saved：**序言 / 尾声不变**，不需要额外 push/pop，栈对齐不变。
- **类别**：每个提升寄存器只有一个类别（int → GPR，float → XMM），按它在缓存感知指令
  中被当作 int 还是 float 访问的加权次数取多数（`move` 不计票）。类别不符的读：副本有效
  时 `movq` 跨类取，否则走内存；类别不符的写：`movq` 跨类刷新。
- **选择**：对每个 VM 寄存器按「读次数 × 8^循环深度」打分（深度 = 覆盖该 pc 的后向跳转
  `[target, src]` 区间数，封顶 3；含自尾调用跳回 pc 0），分数 ≥ 8（循环内读一次，或直线
  代码读 8 次）才提升，各池取分数最高者。

### 15.5 跨 BB 有效性：前向 must 数据流

- 状态：一个位集 `valid`（提升寄存器 ≤ 12 个，`uint32_t` 足够）。
- **发射器就是转移函数**（实现与设计稿的主要差异）：不另写 `cache_effect` 表，而是
  **多遍发射**。每遍完整发射一次函数体，发射过程中维护 `rc_valid`：`emit_jump` 把当前
  状态记到目标 pc 的边上，顺序落入（前一条不是 `jmp`/`ret*`/`tail_call`）也记一条边；
  跳转目标处把状态设为上一遍算出的 `in[pc]`。一遍结束后 `in' = ∩ 边状态`，与本遍所用
  的 `in` 相同即不动点，本遍的机器码就是结果。规则只存在于发射器一处，不可能漂移。
- 第 0 遍不带缓存：收集跳转目标、后向边、操作数投票（15.4），无提升寄存器则直接用这遍
  的输出（与 v1 逐字节相同）。之后从「每个目标全有效」的乐观初值开始迭代；状态只减不增，
  实测 1–3 遍收敛，上限 64 遍，超限回退到无缓存发射。
- **非感知指令**：发射前、发射后都把 `rc_valid` 清零（发射前清零保证它内部发出的跳转边
  带空状态；两条单独去掉都是等价变异，同时去掉则测试失败）。
- **入口预装**：仅当 pc 0 是跳转目标（自尾调用）时，在序言之后、pc 0 标号之前把全部
  副本 load 一次（未初始化的 local 也无妨：副本只承诺「等于内存 payload」）。外部调用、
  JIT 直调、自递归 near call 都从偏移 0 进入，经过预装；自尾调用 `jmp pc0` 跳过预装，
  作为 pc 0 的前驱参与汇合，所以 `sum`/`shuffle`/`tail_sum` 的副本能跨过尾调用回边。
  pc 0 不是目标时入口状态为空，靠 load-on-miss。

### 15.6 发射示意（test_float_arith，设 r1→XMM1、r2→XMM2、r3→RSI、r0→RDI）

```asm
; 入口预装（一次）
movsd xmm1, [rbx+0x18] ; movsd xmm2, [rbx+0x28] ; mov rsi, [rbx+0x38] ; mov rdi, [rbx+0x8]
...
%bb4:                         ; in = {r0,r1,r2,r3}（前驱 bb0 与回边都有效）
addsd xmm1, xmm2              ; r1 += r2   ← 链：仅 addsd 4 周期
movsd [rbx+0x18], xmm1        ; 写穿（不在链上）
mov   qword [rbx+0x10], 0x13
add   rsi, 1                  ; r3 += 1    ← 链：1 周期
mov   [rbx+0x38], rsi
mov   qword [rbx+0x30], 0xb
cmp   rsi, rdi                ; 无 load
jl    %bb4
```

每轮 load 从 3 次降到 0；store 数不变（4 次，吞吐 ≥1/周期，不是瓶颈）。

**就地计算**（实测后补上）：第一版结果先算在 scratch（`mov rax, rsi; add rax, 10;
mov [mem], rax; mov rsi, rax`），`test_arith` 反而慢 6%——本机（Meteor Lake）对「store
后紧跟同址 load」转发很快，而两条 reg-reg mov 都落在依赖链上。现在 `rc_dst`：若 `rd`
已提升且同类、且不是第二操作数，就直接在 `P(rd)` 里算（`add rsi, 10; mov [mem], rsi`），
链上只剩运算本身。除法仍走 `RAX/RDX`（`idiv` 固定寄存器），比较的 `setcc` 仍走 `AL`。

### 15.7 为什么保持 §1 不变量（及其证明义务）

- 不变量 1/2：写穿保证每条指令边界的内存状态与 v1 **逐字节相同**，GC 点上对象位置、
  `stack_top`、tag 全部不变，§14.1 的枚举规则原样成立。物理寄存器里的副本**从不是
  唯一持有者**，GC 不需要看到它；GC 点作废全部副本，所以将来即使是移动式 GC
  也不会让副本里的旧指针被继续使用。
- 不变量 3：帧结构、调用路径不变。
- 唯一新增的风险是**副本陈旧**。对策：
  1. 规则单一真源：发射器本身就是数据流转移函数（15.5），不存在两份规则。
     缓存感知的指令必须在**任何内部分支 / raise 之前**取完全部操作数（如除法先取被除数
     和除数再判零），保证一条指令只有一个缓存状态。
  2. **运行期自检**：环境变量 `KOALA_JIT_CHECK_CACHE=1`（任何构建）时，每次命中副本前
     发射 `cmp P(rN), [rbx+N*16+8]; je ok; ud2`（XMM 先 `movq r11, xmmN` 再按位比较，
     NaN 也能比对）。全量 lit 在「自检 + 阈值 1」下通过；人为制造漏作废时进程以
     SIGILL 崩在具体指令上（已验证）。
  3. 回归测试 `test/test-run/test_jit_regcache.kl`：跨回边的 int/float 链、循环中夹
     `get_field`（非感知指令写提升寄存器，且写之前副本有效）、循环中夹调用、汇合点一侧
     经调用重定义、寄存器移位 helper + 除法（RCX/RDX）、自尾调用（入口预装）、浮点比较 /
     融合浮点跳转。五种模式：JIT / `KOALA_JIT=0` / 阈值 1 / 阈值 1 + 自检 / trace
     （断言 `isum`/`fsum`/`tail_sum` 确有提升）。
  4. 变异验证（全部使测试失败）：`get_field` 当感知、寄存器移位当感知、`call` 当感知、
     去掉入口预装、汇合不取交、`rc_def` 不刷新副本、去掉非感知指令前后两处作废。
     等价变异：只去掉前 / 后一处作废、不可达 pc（终结指令之后且非跳转目标）的状态清零。

### 15.8 收益：预估 vs 实测（Core Ultra 7 155H，release）

实测：同一 release 二进制，`KOALA_JIT_REGCACHE=0` 对比默认，hyperfine `-N -w 2 -m 8`，
单位 ms（均值）。

| bench | 链上变化 | 预估 | 关 | 开 | 实测 |
| --- | --- | --- | ---: | ---: | ---: |
| test_float_arith | 9c → 4c（addsd） | ~1.8–2× | 190 | 54 | **3.51×** |
| test_float_logic | 浮点比较 + 计数链进寄存器 | ~1.1–1.3× | 233 | 110 | **2.11×** |
| loop-sum | 6c → 1–2c | ~1.5–2× | 75 | 54 | 1.38× |
| sum | 自尾调用链经寄存器 | ~1.4–1.8× | 104 | 77 | 1.35× |
| test_bitwise | 位运算链 | — | 215 | 163 | 1.32× |
| shuffle | store 吞吐瓶颈 | ~1.1–1.3× | 86 | 76 | 1.14× |
| tak | 比较 / 参数读 | ≈ 0 | 153 | 138 | 1.10× |
| test_arith / test_uint_arith | `idiv` 主导 | ~1.2–1.4× | 507 / 475 | 502 / 470 | 1.01× |
| test_logic_chain | 两次 `idiv` + 分支预测失败主导 | ~1.1–1.3× | 448 | 449 | 1.00× |
| fib / ack | 调用密集，调用点全部作废 | ≈ 0 | 815 / 260 | 819 / 263 | ≈1.0× |

test_float_arith 从 190ms 到 54ms，已接近 luajit（46ms）。整数除法类基准受 `idiv`
（~15–40 周期）支配，链上省下的 4–5 周期占比很小。

若 shuffle 类测得确为 store 吞吐瓶颈，后续可加「15.9 延迟写回」：dirty 副本只在 GC 点 /
出口 / 非感知指令前写回（仍满足不变量 1，但内存不再逐条与 v1 一致，需改写不变量 2 的措辞，
且错误路径要先写回）。**先做写穿版，用实测决定是否值得。**

### 15.9 实现记录（`src/jit/jit.c`）

- `rc_*` 一组函数：`rc_gpr`/`rc_xmm`（取操作数副本，miss 时 load）、`rc_load_gpr`/
  `rc_load_xmm`（把操作数搬到指定寄存器，含跨类 `movq`）、`rc_alu`/`rc_sse`（与内存或
  副本运算）、`rc_dst`（就地计算的目标）、`rc_def`/`rc_def_imm`（写穿后刷新副本）、
  `rc_emit_preload`。`emit_jump` 记边。
- 缓存感知（`rc_aware`）：`move`、`load_int_imm`/`load_uint_imm`/`loadk`/`load_tag`、
  int/uint 全部 ALU / 比较 / 融合跳转（含 `_imm`、立即数移位、寄存器与立即数除模）、
  `int.neg`/`int.not`、float 四则 / `neg` / 比较 / 融合跳转、`land`/`lor`/`lnot`、
  `jmp`/`jmp_true`/`jmp_false`/`jmp_ref_*_null`、`ret*`、自尾调用。其余（调用、寄存器移位、
  `float.mod`、类型转换、字段、`new` 等）为非感知。
- 不提升的情形：没有寄存器得分 ≥ 8 时第 0 遍输出即最终输出，与 v1 一致。
- trace：有提升时在函数头前打印 `[jit] regcache @f: r1=rsi r2=xmm1 ... (N passes)`。
- 编码器新增 `movaps`/`movq xmm↔gpr`/SSE reg-reg/`ucomisd reg-reg`/`ud2`，反汇编同步。
- 后续可选：「延迟写回」（见上文）、更细的非感知指令作废（只作废其 def）。

## 16. 阶段 3：分块代码区（Chunked Code Arena）

> 状态：**已实现（2026-11-14）**，`src/jit/jit.c` `jit_code_alloc`。

### 16.1 问题：每个函数一个 mmap

v1 每编译一个函数就 `mmap` 一块 RW、拷入、`mprotect` 成 RX。`mmap` 以页（4KB）为单位，
而 JIT 函数普遍很小：bench 程序平均约 800 字节（含 std 的 `__init__`），get/set 这类
方法只有 100 多字节。于是：

- **物理内存浪费**：每个函数至少独占一页。83 字节的函数也占 4KB，浪费约 98%。
- **iTLB / L1i 局部性差**：每个函数在不同的页上，调用链 `main → all → f0 → f1 …` 每跳
  都可能换页，一个 iTLB 项只覆盖一个函数。
- **地址不可控**：各函数映射散落在地址空间里，将来想用 `call rel32` 互调（5 字节，
  对比现在的 `mov r11, imm64; call r11` 13 字节）没有 ±2GB 保证。

### 16.2 为什么不用 `mremap` 扩大一块大映射

`mremap(MREMAP_MAYMOVE)` 在原地放不下时会**搬走整块**。已编译代码里有绝对地址
（`JitEntry.fn` 被缓存、返回地址在 C 栈上、自递归 `call rel32` 指向自身），搬走后全部
失效；不带 `MAYMOVE` 则随时可能失败。所以用**多个固定 chunk**，每个 chunk 一旦映射
地址就不变，满了再开新的。

### 16.3 实现原理

```
chunk（mmap 1MB，PROT_NONE + MAP_NORESERVE，只占虚拟地址，不占物理页）
┌────────────┬──┬──────────┬──┬────────────┬───────────── … ─────┐
│ f0 (RX)    │gap│ f1 (RX)  │gap│ f2 (RX)   │  未用（PROT_NONE）   │
└────────────┴──┴──────────┴──┴────────────┴───────────── … ─────┘
                                            ↑ used（下一个从 align16(used) 开始）
```

分配 `sz` 字节（`jit_code_alloc`）：

1. `off = align16(used)`。若当前 chunk 放不下，新映射一个 chunk，大小为
   `max(chunk, 页对齐(sz))`（超大函数独占一个）。旧 chunk 剩余部分保持 PROT_NONE，
   不占物理内存。
2. 只对代码覆盖的页 `[页下界(off), 页上界(off+sz))` 执行 `mprotect(RW)`，`memcpy`，
   再执行 `mprotect(RX)`。首页可能还有之前的函数，它在这一瞬间不可执行；但编译是
   **本线程同步**进行的，期间不会有原生代码在跑（可能有 JIT 帧挂在 C 栈上等 helper
   返回，翻回 RX 之后才恢复执行），所以安全。翻回 RX 失败则 `abort`，因为旧代码已经
   不可执行，没有安全的退路。
3. 对齐空隙不填充：新映射的页本来是零，空隙也永远不会执行。没有 int3 填充，也就不会
   去写前一页。

**W^X 不变**：任何页在任何时刻不同时可写可执行。x86 的指令缓存与数据写一致，
不需要显式 flush（移植到 AArch64 时需要 `__builtin___clear_cache`）。

**为什么不用双映射（memfd 映射两次：一个 RW 视图、一个 RX 视图）**：写入时不用翻页
权限，多线程也安全；但 RW 视图一直存在，W^X 实际被削弱。更严重的是 `MAP_SHARED` 在
`fork` 后父子进程共享同一份物理页：子进程再编译会写坏父进程的代码。当前运行时是单线程
编译，翻页方案更简单也更安全。

**将来多线程**：需要给 arena 加锁；并且翻页期间其他线程可能正在执行首页上的代码，
那时应改成每线程一个 arena，或者每个函数从新页开始（只在多线程时这样做）。

### 16.4 收益（实测，`KOALA_JIT_STATS=1`，`KOALA_JIT_THRESHOLD=1`）

「旧页数」= 各函数 `⌈sz/4096⌉` 之和，「新页数」= arena 实际提交的页。

| 程序 | 函数数 | 代码字节 | 旧页数 | 新页数 | 节省 |
| --- | ---: | ---: | ---: | ---: | ---: |
| bench/fib.kl（及同类 bench） | 14 | 11554 | 14 | 3 | 4.7× |
| bench/jit_diff.kl | 21 | 33323 | 26 | 9 | 2.9× |
| test_lambda_infer.kl | 57 | 60728 | 68 | 15 | 4.5× |
| test_int_mix.kl | 69 | 141552 | 82 | 35 | 2.3× |
| test_with.kl | 62 | 184395 | 85 | 46 | 1.8× |

函数越小越省：get/set 内联前的小方法每页能放下 30 个左右。系统调用次数与旧方案持平
（旧：每个函数 1 次 `mmap` + 1 次 `mprotect`；新：每个函数 2 次 `mprotect`，每个 chunk
1 次 `mmap`）。运行时间：fib / ack / tak / jit_diff / test_with / test_int_mix 的差异都在
hyperfine 噪声内（±2%）。这些程序的热代码只有几页，iTLB 本来就不是瓶颈；局部性的收益
要等函数数到几百上千（标准库、大应用）才会显现。另外 arena 给后续 `call rel32` 互调
（同一 chunk 内保证在 ±2GB 内）打好了基础。

### 16.5 测试

`test/test-run/test_jit_code_arena.kl`：40 个小函数、一个约 31KB 的 `big`、一个先编译且走
错误尾声的 `fdiv`。六种运行：JIT、解释器、阈值 1、4KB chunk（强制跨 chunk、`big`
独占一个 chunk）、`PACK`（断言 40+ 函数不到 40 页、只用 1 个 chunk）、`SPLIT`（断言多个
chunk）。全量 lit 在 `KOALA_JIT_CODE_CHUNK=4096 KOALA_JIT_THRESHOLD=1` 下也通过。

变异（全部使测试失败）：不翻回 RX、只翻最后一页、起点向下对齐（覆盖前一个函数的错误
尾声）、`used` 不前进、不换新 chunk、超大函数不独占 chunk、页上界向下取整。

## 17. 计划：OSR（栈上替换）与回边计数

> 状态：**设计稿，待实现**。目标是把编译阈值从 1 提高，同时不丢掉热循环。

### 17.1 问题：没有 OSR，阈值只能是 1

现在函数只能在**调用入口**（`kl_call_code`）切换到原生代码。实测（release，ms）：

| 程序 | 解释器 | 阈值 1 | 阈值 2 | 阈值 10 | 阈值 100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| loop-sum | 299 | **58** | 297 | 300 | 301 |
| test_float_arith | 268 | **59** | 267 | 266 | 268 |
| fib | 3601 | 820 | 821 | 826 | 815 |
| test_with（62 个函数） | 24.4 | 24.1 | 23.6 | 23.5 | 23.3 |
| test_int_mix（69 个函数） | 16.2 | 16.8 | 16.4 | 15.9 | 14.8 |

- 阈值 ≥ 2 时，「只调用一次的 `main` 里跑热循环」永远不会被 JIT，退回解释器速度（5×）。
- 阈值 1 的代价：每个只跑一次的函数（模块 `__init__`、一次性的 helper）也要编译，
  约 30µs/个，短程序多花 1–2ms（约 10%），代码区也被冷代码占满。

### 17.2 方案：热度 = 调用 + 回边；回边处 OSR 进入原生代码

**热度计数**（`CodeObject.jit_hotness`，单个计数器）：

- 每次调用 +1（`kl_call_code`，现状）。
- 解释器每次**向后跳转**（`off < 0`，所有跳转类指令：`jmp`、融合比较跳转、
  `jmp_true/false`、`jmp_ref_*_null`）+1。改成统一的 `JUMP(off)` 宏，宏内
  `if (off < 0) { 计数; 若已编译则尝试 OSR }`，只在被采纳的后向跳转上多一次比较。
- 解释器的**自尾调用**（`local_tailcall`）也算一次回边，目标 pc 0。
- 达到阈值（`KOALA_JIT_THRESHOLD`，默认值实现后实测再定，候选 100–1000）就编译，
  `jit_tried` 闩锁不变。

**OSR 入口**：编译时为每个**回边目标**（第 0 遍已收集的 `backs`）在函数末尾额外发一段桩：

```
osr_T:  序言（与正常入口相同：push rbp/rbx，保存 out/ks，rbx = regs）
        按 rc_in[T] 从内存装载寄存器缓存副本（§15，与入口预装同一函数）
        jmp pc_map[T]
```

`JitEntry` 新增 `osr[] = {pc, 桩偏移}` 表，按 pc 排序，二分查找。
目标为 pc 0（自尾调用）时直接用正常入口 `fn`。

**解释器侧**（`_eval_frame`，帧此时已链接 `ks->cf = cf`）：

```c
/* 后向跳转之后，pc 指向循环头 T */
if (co->jit && (stub = jit_osr_lookup(co->jit, T)) && jit_params_ok(regs, nparams)) {
    stub(regs, &result, ks);
    ks->stack_top = cf->locals + cf->nlocals;   /* 同 kl_call_code */
    if (result.tag == JIT_TAG_TAILCALL) kl_jit_tail_continue(&result);
    goto done;      /* 不走 error:，原生代码已经记过 traceback */
}
```

原生代码一直跑到函数返回，把结果写到 `result`，解释器直接以这个结果结束本帧，和
`OP_RET` 的效果一样。**没有反向切换（deopt）**：一旦进入原生代码，就不会再回到这一帧
的解释器。

### 17.3 为什么在 v1 里 OSR 很简单（不变量保持）

一般 JIT 做 OSR 难在要把解释器状态（栈槽、拆箱值）**转换**成编译代码的布局。v1 不存在
这个问题：解释器和原生代码共用同一份 TValue 寄存器文件（`cf->locals`），内存就是真相
（§1）。在后向跳转的那一刻，内存里已经是完整状态，原生代码在 `pc_map[T]` 处需要的只有：

1. `rbx = regs`、帧槽里的 `out`/`ks`：序言建立，与正常入口相同。
2. 寄存器缓存副本 `rc_in[T]`：桩从内存装载，和入口预装的道理一样（副本只承诺
   「等于内存 payload」）。
3. `ks->stack_top`、`ks->cf`：解释器执行到回边时与正常入口时相同（`_eval_frame` 在
   调用前后会恢复 `stack_top`，帧已经链接）。

所以 OSR 不需要状态转换表、stack map 或 deopt，GC 枚举也不受影响（帧和 VM 栈不变）。

### 17.4 预期收益

- loop-sum / test_float_arith 在阈值 100 下仍是 ~58ms（循环 100 次后 OSR 进入）。
- 短程序：只编译真正热的函数，test_int_mix 16.8 → ~14.8ms，代码页更少。
- fib / tak / ack 这类调用密集的程序：前 N 次调用走解释器，之后一样，差别可以忽略。
- 解释器（`KOALA_JIT=0`）的开销：每个被采纳的后向跳转多一次比较、计数加一，预计 <1%，
  实现后用 `KOALA_JIT=0` 的 bench 验证。

### 17.5 测试与证明义务

- `test/test-run/test_jit_osr.kl`，在默认阈值下运行，并与 `KOALA_JIT=0`、阈值 1 对比
  输出：`main` 中的 int / float 循环（寄存器缓存副本在循环头有效）、嵌套循环（先从
  内层头进入）、`while` 条件在底部的循环（后向条件跳转）、OSR 之后循环中抛错（traceback
  只记一次、与解释器一致）、自尾调用函数只调用一次、OSR 之后发生尾调用（`TAILCALL`）。
- trace 新增 `[jit] osr @f pc=T`，测试断言确实发生了 OSR；`KOALA_JIT_STATS` 增加
  OSR 次数。
- 全量 lit 在 `KOALA_JIT_CHECK_CACHE=1` + 默认阈值下跑：桩少装一个副本会当场 `ud2`。
- 变异：桩不装副本、跳到错误的 `pc_map`、OSR 后不复位 `stack_top`、出错后走 `error:`
  （重复 traceback）、不处理 `TAILCALL`、回边不计数（性能测试 / trace 断言失败）。
- 测试里已有的 `KOALA_JIT_THRESHOLD=1` RUN 保留原义（首次调用即编译）。

### 17.6 落地步骤

1. `JitEntry.osr` 表 + 桩发射（复用 `rc_emit_preload`，增加按 `rc_in[T]` 装载的版本）。
2. `vm_ops.h` 跳转统一用 `JUMP(off)`；`_eval_frame` 中加入回边计数与 OSR 分派；自尾调用
   计一次回边。
3. trace / stats / 测试 / 变异；用 bench 与短程序扫描阈值，决定默认值，更新 §7、§11、§13。

