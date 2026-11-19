/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 *
 * x86-64 System V fiber context switch. Only callee-saved state is kept:
 * rbx, rbp, r12-r15, rsp, MXCSR and the x87 control word. Caller-saved
 * registers are already saved by the C code that called kl_fiber_switch.
 */

    .text

# void kl_fiber_switch(FiberCtx *from, FiberCtx *to)
#   rdi = from, rsi = to; FiberCtx's first field is `void *sp`
    .globl kl_fiber_switch
    .type kl_fiber_switch, @function
    .p2align 4
kl_fiber_switch:
    push    %rbp
    push    %rbx
    push    %r12
    push    %r13
    push    %r14
    push    %r15
    sub     $8, %rsp
    stmxcsr (%rsp)
    fnstcw  4(%rsp)
    mov     %rsp, (%rdi)        # from->sp = rsp
    mov     (%rsi), %rsp        # rsp = to->sp
    ldmxcsr (%rsp)
    fldcw   4(%rsp)
    add     $8, %rsp
    pop     %r15
    pop     %r14
    pop     %r13
    pop     %r12
    pop     %rbx
    pop     %rbp
    ret
    .size kl_fiber_switch, .-kl_fiber_switch

# first entry of a new fiber: r12 = entry function, r13 = its argument
    .globl kl_fiber_trampoline
    .type kl_fiber_trampoline, @function
    .p2align 4
kl_fiber_trampoline:
    mov     %r13, %rdi
    and     $-16, %rsp          # SysV: rsp % 16 == 0 at the call
    call    *%r12
    ud2                         # entry functions never return
    .size kl_fiber_trampoline, .-kl_fiber_trampoline

# no executable stack
    .section .note.GNU-stack,"",@progbits
