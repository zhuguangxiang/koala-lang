# Koala 基线 JIT 设计 · Drop Bear（v1）

> 位置：`src/jit/`。本文描述 v1 的设计取舍、执行模型、字节码白名单、
> 两遍汇编与跳转回填、热度分派、内存管理、正确性保证与后续路线。
> 代码入口见 `include/jit/jit.h`，实现见 `src/jit/jit.c` 与 `src/jit/emit.c`。

## 1. 概述

v1 是一个 **Sparkplug 风格的基线（baseline）JIT**：把每条受支持的字节码指令
一对一地 lowering 成一段固定机器码，语义与解释器完全一致，只是去掉了
**取指 dispatch、指令解码、类型检查**，并把字节码跳转直接降为原生跳转。

它不做寄存器分配、不做类型数据流分析、不做内联/逃逸等优化。目标是
**用最小的复杂度、零运行时依赖**，在数值热点函数上拿到稳定的加速比。

### v1 设计不变量（核心约束）

1. **保持 TValue，不拆箱 / 不物化**：寄存器文件就是解释器同款的 `cf->locals`
   （每寄存器 16 字节 `{tag, payload}`），机器码直接在其上 load/store，不拆箱到
   裸 GPR/XMM、也不做逃逸点物化。拆箱 + 机器 RA 属 v2（见 Koala_TODO §100）。
2. **调用都走 `kl_call_code`，self-call 除外**：普通调用（flag=0）经
   `kl_jit_call_helper` 回到运行时的 `kl_call_code`（CFunc 走 `kl_call_cfunc`），
   复用其建帧 / 链帧 / 原生-解释器分派；**不做 JIT→JIT 直调**（v2 再做）。
   唯一例外是纯自递归（`is_self_only`）：直接 near call 本函数原生入口，绕过
   `kl_call_code`（见 §5.3）。

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

- 接口/寄存器调用（`OP_CALL` flag=1/2）、闭包、方法分派
  （flag=0 的普通调用已支持，见 §5.3）。
- 对象 / 字符串等非数值运算（int64/uint64/窄整型/float64/bool 已覆盖，见 §5.4、§5.5）。
- 寄存器分配、栈上寄存器溢出优化、去优化（deopt）与 OSR。
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
代价是每次运算多一次 tag 写入、寄存器仍驻留内存。拆箱与寄存器分配留给 v2。

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
void fn(TValue *regs, TValue *out);
```

- `regs`：调用方的寄存器文件，即 `cf->locals`，含 `nlocals` 个 `TValue`。
- `out`：返回值写回位置（调用方栈上的 `TValue result`）。

入参已由 `kl_call_code` 用 `memcpy` 拷入 `cf->locals`，因此 JIT 无需再搬参数。

### 4.2 TValue 布局与寄存器寻址

`TValue` 为 16 字节：`{ uintptr_t tag; union { int64_t ival; ... }; }`。
约定 **RBX 恒为寄存器文件基址**，则寄存器 `i`：

- tag 位于 `[RBX + i*16]`（`TAG_DISP(i)`）
- payload 位于 `[RBX + i*16 + 8]`（`VAL_DISP(i)`）

相关 tag：`TAG_NIL=0`、`TAG_BOOL=2`、`TAG_INT64=0b1011`。

### 4.3 寄存器使用约定

- **RBX**：寄存器文件基址（callee-saved，序言保存、尾声恢复）。
- **RBP**：帧指针；`[RBP-16]` 保存 `out` 指针。
- **RAX / RCX / RDX**：caller-saved 临时寄存器（scratch），所有运算临时值只用这三个。
- 不使用 RSP/R12 作为内存基址（避免 SIB 字节），编码器只支持 RBX/RBP 基址。

### 4.4 序言 / 尾声

```asm
; prologue: rdi = regs, rsi = out
push rbp
mov  rbp, rsp
push rbx
sub  rsp, 16
mov  [rbp-16], rsi    ; 保存 out 指针
mov  rbx, rdi         ; 寄存器文件基址

; ... body ...

; epilogue（返回值须已写入 *out）
add  rsp, 16
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
| `OP_INT_DIV/MOD`(+`_IMM`) | rd = rs / 或 % rt/imm（有符号） | `mov rax,[rs.val]; cqo; idiv [rt.val]/rcx; mov [rd.val],rax(商)/rdx(余)` |
| `OP_INT_SHL_IMM/SHR_IMM` | rd = rs << / >> imm | `shl/sar rax,imm8`（SHR 用算术右移，与 int64 `>>` 一致） |
| `OP_INT_NEG/NOT` | rd = -rs / ~rs | `neg/not rax` |
| `OP_INT_EQ/NE/LT/LE/GT/GE`(+`_IMM`) | rd = (rs CC rt/imm) → bool | `cmp; setcc al; movzx rax,al; mov [rd.val],rax; tag=BOOL` |
| `OP_JMP` | pc += off | `jmp rel32`（回填） |
| `OP_JMP_TRUE/FALSE` | rs.val != 0 / == 0 则跳 | `cmp [rs.val],0; jne/je rel32` |
| `OP_JMP_INT_*`(+`_IMM`) | rs CC rt/imm 则跳（融合） | `mov rax,[rs.val]; cmp ...; jcc rel32` |
| `OP_RET` | 返回 rs | 写 `*out` = rs 的 tag+payload；尾声 |
| `OP_RET_INT_IMM` | 返回立即数 | 写 `*out` = {ti, imm}；尾声 |
| `OP_RET_VOID` | 返回 nil | 写 `*out` = {NIL, 0}；尾声 |
| `OP_CALL`（flag=0） | rd = fn(args) | helper 调用或自递归直调，见 §5.3 |
| `OP_UINT_ADD/SUB/MUL/AND/OR/XOR`(+`_IMM`) | rd = rs OP rt/imm，tag=UINT64 | 同 INT 族 payload 指令（补码运算与符号无关），tag 写 UINT64 |
| `OP_UINT_DIV/MOD`(+`_IMM`) | 无符号除 / 模 | `mov rax,[rs.val]; xor edx,edx; div [rt.val]/rcx` |
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
| `OP_FLOAT_EQ/NE/LT/LE/GT/GE` | rd = (rs CC rt) → bool | 调 `kl_jit_float_cmp`（C 运算符，含 NaN），存 rax + BOOL |
| `OP_JMP_FLOAT_*` | rs CC rt 则跳（融合） | 调 `kl_jit_float_cmp`；`cmp rax,0; jne` |
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

比较条件码映射：有符号 EQ→`CC_E`、NE→`CC_NE`、LT→`CC_L`、LE→`CC_LE`、
GT→`CC_G`、GE→`CC_GE`；无符号 LT→`CC_B`、LE→`CC_BE`、GT→`CC_A`、
GE→`CC_AE`（EQ/NE 共用 `CC_E`/`CC_NE`）。uint 的 EQ/NE 寄存器形式复用
`OP_INT_EQ/NE`（位比较等价），故只有 `OP_UINT_EQ_IMM/NE_IMM`。

`OP_JMP_TRUE/FALSE` 解释器用 `to_raw_bool`，对 bool 即取 `ival`（0/1），
故机器码直接比较 payload 与 0 等价。

### 5.3 调用与错误传播

**flag=0 普通调用**经运行时 helper：

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

**纯自递归直调**：若函数内**所有**调用都指向自身（`is_self_only`），
`OP_CALL` 降为直接 near call 到本函数原生入口（偏移 0），绕过
`kl_call_code` 的帧机制，递归期间 `ks->stack_top` 不被读取，无需推进。
自递归函数不含 helper 调用，唯一失败模式是 SIGFPE（与解释器同样终止进程），
故无需错误检查。

**flag=1（接口）/ flag=2（寄存器 callable）** 动态解析，v1 整体 bail。

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
- **比较**（bool 与融合跳转）调 `kl_jit_float_cmp(a, b, op)`，直接用解释器同款 C 运算符，
  NaN/unordered 语义 by-construction 一致。内联 `ucomisd`（需处理 PF）留 v2。
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

`fixups` 与 `pc_map` 均按指令数 `n` 预分配（每条指令至多一个跳转）。

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
if (co->jit && jit_params_scalar(cf->locals, nargs)) {
    cf->back = ks->cf;                  /* 链接帧链，与 _eval_frame 一致 */
    ks->cf = cf;
    co->jit->fn(cf->locals, &result);   /* 原生路径 */
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
- **入口 tag 守卫**：`jit_params_scalar` 要求所有实参都是 `is_int || is_uint || is_bool || is_float`
  （整数 payload 都在 ival），否则即使已编译也走解释器，挡掉 float/对象入参。
- **编译期白名单**：`kl_jit_compile` 仅接受 module 拥有的自由函数
  （`IS_MODULE(owner)`），且翻译中遇到任何非白名单 opcode 立即 `failed=1` 放弃。
- 阈值默认 1（首次调用即编译并在该次调用启用），可用环境变量调整。

## 8. 内存管理（W^X）

```c
mem = mmap(NULL, sz, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
memcpy(mem, buf.data, sz);
mprotect(mem, sz, PROT_READ|PROT_EXEC);   /* 翻转为可执行、不可写 */
```

先写后执行，满足 W^X。`JitEntry` 记录 `mem`/`mem_size`，`kl_jit_free` 用
`munmap` 释放。v1 中代码缓存随 `CodeObject` 生命周期存续（进程级，暂不回收）。

## 9. 正确性保证

1. **逐条对齐解释器**：字段位置、符号扩展、tag 写回、分支条件、跳转目标
   全部与 `vm_ops.h` 核对一致（见 §5）。
2. **有符号特化 + 保守 bail**：uint 的 div/mod/比较/跳转是**独立 opcode**
   （`OP_UINT_*`、`OP_JMP_UINT_*`），不在白名单内 → 整体放弃 → 解释器。
   因此白名单里的有符号 `idiv`/`CC_L` 等绝不会误用于无符号语义。
3. **乘法/溢出**：`OP_INT_MUL` 解释器就是 C int64 相乘（回绕），`imul` 一致。
4. **除法**：`idiv` 向零截断，与 C `/`、`%` 一致；除零两者同样触发 SIGFPE。
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

- **接口/寄存器调用不支持**：`OP_CALL` flag=1/2 整体 bail；
  v2 需处理 itab/callable 动态解析。
- **自递归帧不入帧链**：自递归直调绕过 `kl_call_code`，递归各层共享外层帧。
  self-only 函数无 raise 路径（仅 SIGFPE），traceback 不受影响；
  v2 若放宽 self-only 限制，需为递归层补建帧。
- **无寄存器分配**：寄存器驻留内存，每次运算都 load/store。v2 可把热点寄存器
  提升到物理寄存器，并做 int64 拆箱（方案 B）。
- **无 deopt**：v1 用编译期 bail 替代运行期去优化，简单但粒度粗
  （一条不支持即整函数放弃）。v2 可考虑按基本块编译或运行期守卫 + deopt。
- **数值已覆盖 int64/uint64/窄整型/float64/bool**：对象/字符串未支持。
- **代码缓存不回收**：进程级常驻，长生命周期多函数场景需 LRU/引用计数。

## 11. 环境变量

| 变量 | 作用 | 默认 |
| --- | --- | --- |
| `KOALA_JIT=0` | 关闭 JIT（全部走解释器） | 开启 |
| `KOALA_JIT_THRESHOLD=<n>` | 热度阈值，调用 n 次后编译 | 1 |
| `KOALA_JIT_TRACE=1` | 开启 trace（见下）：编译成功打印 IR 风格 dump，编译失败/跳过打印 bail 原因 | 关闭 |

三个变量均在进程内首次读取后缓存（`static int cached = -1` 惰性初始化），
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

### 13.1 落后归因 → Yowie(v2) 靶子

| 落后项 | 根因（v1 限制） | Yowie 对策 |
| --- | --- | --- |
| test_float_logic（4.46×） | 浮点比较 `if res>=50.0` 每迭代调 `kl_jit_float_cmp` C helper | 内联 `ucomisd`（处理 PF/NaN） |
| test_float_arith（3.88×） | TValue 镜像：每迭代 movsd 读内存→addsd→写内存+写 tag | 拆箱 + RA，值留 XMM |
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
2. VM 栈：把 [ks->stack_base, ks->stack_top) 整段当 TValue[] 扫，is_ref / is_intf 即 root。
```

依据：
- `_new_frame` 中 `cf->locals = stack_top; stack_top += nlocals`，各帧 locals + 调用参数窗口
  在 VM 栈上连续排布，活跃区 `[stack_base, stack_top)` 恰好覆盖所有帧的活值。
- **JIT 原生栈（rbp/rsp、返回地址）里没有对象引用**——值都在 VM 栈的 `cf->locals`。
  故 GC **不扫原生栈**，只扫 VM 栈。这是镜像模型最大的便利。
- 帧链（`cf->back` / `ks->cf` / `cf->pc`，见 §7）已就位：可按 cf 链逐帧扫 `nlocals`
  （精确到帧），或直接扫整段栈（更简单）。两者皆可。

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