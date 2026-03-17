AST
  ↓
Koala IR（唯一 IR = VM Machine IR）
  ↓
isel.c（在 IR 上做指令选择 + 融合 + 拆分 + legalization）
  ↓
lsra.c（在 IR 上做寄存器分配：vreg → VM slot）
  ↓
codegen/（线性化 → LowerInsn → encode）
  ↓
VM 执行

klr/
    ir.h
    ir.c
    isel.c        // 指令选择（融合 + 拆分 + legalization）
    lsra.c        // 寄存器分配（vreg → VM slot）
    passes.c      // IR-level pass 管理（可选）

codegen/
    codegen.c     // IR → LowerInsn（线性化）
    lower_insn.h  // LowerInsn 结构
    encode.c      // LowerInsn → bytecode
    vm_opcode.h   // VM opcode 表（ABC/ABx/Ax）

vm/
    vm.c          // VM 执行器
    vm_state.h

我们需要将通用的运算，比如：

Binary的运算，转化为INT或者float或者保留通用运算；

将有const的参与的操作进行优化；

op_ir_local转化为move或者const；

cond_jmp好像不能在isel中进行改变；

这个思路非常清晰，这正是 Legalization（合法化） 和 Strength Reduction（强度折减） 的核心工作。既然 KLR 是 VM 的 Machine IR，我们在 klr/isel.c 中需要完成从“语义抽象”到“机器限制”的最后跳跃。
以下是针对你提到的几个点的具体实现方案：

1. 运算的特化与类型降级 (Type Specialization)
通用运算（如 OP_BINARY_ADD）在高级 IR 中可能处理多种类型，但在后端我们需要根据类型信息将其特化：
INT/FLOAT 特化：检查操作数的 ts (TypeSpec)。如果左右都是 int64，将 OP_BINARY_ADD 替换为更高效的物理指令 VM_OP_ADD_INT；如果是 float，则替换为 VM_OP_ADD_FLOAT。
保留通用运算：如果类型是 object 或动态类型，则保留为 OP_BINARY_ADD（映射到 VM 的通用分发函数）。
2. 常量参与的操作优化 (Constant Folding & Immediates)
这是减少寄存器压力的关键。
计算折叠：如果 ADD 5, 10，直接在 isel.c 中将其替换为 MOVE 15。
立即数转换：如果 ADD %r1, 10，且 VM 支持 ADDI（带有立即数的加法），则将 OP_BINARY_ADD 转换为 VM_OP_ADDI。这样可以少占用一个寄存器 Slot（不用把 10 先 MOVE 到寄存器）。
3. OP_IR_LOCAL 的消除与转化
OP_IR_LOCAL 在前端代表声明，在后端它必须变成具体的物理操作：
转化为 MOVE：如果 local 有初始值，直接变为 MOVE 到对应的 vreg。
转化为 CONST：如果这个 local 在整个生命周期内其实是个常量，直接将其所有引用点替换为常量，并删除该指令。
寄存器占位：如果没有初值，它仅仅是给 LSRA 划定一个 vreg 的起点，不产生实际代码。
4. OP_IR_JMP_COND 的特殊处理
你提到的“不能在 ISEL 中改变”非常正确，因为跳转涉及到 CFG（控制流图） 的结构：
不改变结构：我们不删除 JMP_COND 节点，也不改变它指向的 BB。
改变“形态”：我们只在 Fusion（融合） 时改变它的 code。
例如：CMP %0, %1 + JMP_COND %0。
在 isel.c 中，我们把 JMP_COND 的 code 修改为物理跳转指令 VM_OP_BLT (Branch if Less Than)，并让它直接引用 %0 和 %1 的 vreg。
这样，原本的 CMP 指令就变成了“无用指令”（因为结果 %0 不再被需要），在随后的 LSRA 或 DCE 中会被自动抹除。

流程总览
IR → isel（语义选择，非控制流）

    保留：
        OP_JMP_COND cond, true_bb, false_bb（双 target terminator，不动）

    处理：

        算术/逻辑/比较：选具体 VM 变体（int/float/imm 等）

        常量折叠 / 立即数形式

        local / 参数访问的规范化

    不做：

        不改 terminator 结构

        不插入/删除跳转

        不拆/合并 BB

SA / RA（在 isel 后的 IR 上跑）

    CFG 基于 OP_JMP_COND 的两个 target，语义稳定

    live range、支配关系等都在这套 IR 上分析

BB 排序（layout）

    在 codegen 前/中决定 block 顺序

    目标：尽量让 false_bb 成为 fallthrough，减少额外 JMP

codegen（线性化 + cond_jmp lowering）

    对每个 OP_JMP_COND cond, true_bb, false_bb：

        看当前 block 的 layout：

            如果 false_bb 是 fallthrough：

                生成一条：JMP_IF cond, true_bb

            否则：

                生成：

                JMP_IF cond, true_bb

                JMP false_bb

        可选：匹配前面的 cmp，做 cmp+branch 融合：

            cmp_lt a, b + OP_JMP_COND t, BB1, BB2

                → JMP_LT a, b, BB1 (+ 可选 JMP BB2)
