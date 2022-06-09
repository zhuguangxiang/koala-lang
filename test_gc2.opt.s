	.text
	.file	"test_gc"
	.globl	foo2                            # -- Begin function foo2
	.p2align	4, 0x90
	.type	foo2,@function
foo2:                                   # @foo2
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movl	$100, %edi
	callq	malloc@PLT
	movb	$100, (%rax)
	movq	%rax, -8(%rbp)
	callq	bar@PLT
.Ltmp0:
	movq	-8(%rbp), %rax
	movb	$120, (%rax)
	addq	$16, %rsp
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Lfunc_end0:
	.size	foo2, .Lfunc_end0-foo2
	.cfi_endproc
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
	.section	.llvm_stackmaps,"aw",@progbits
	.byte	3
	.byte	0
	.short	0
	.long	1
	.long	0
	.long	1
	.quad	foo2
	.quad	24
	.quad	1
	.quad	2882400000
	.long	.Ltmp0-foo2
	.short	0
	.short	5
	.byte	4
	.byte	0
	.short	8
	.short	0
	.short	0
	.long	0
	.byte	4
	.byte	0
	.short	8
	.short	0
	.short	0
	.long	0
	.byte	4
	.byte	0
	.short	8
	.short	0
	.short	0
	.long	0
	.byte	3
	.byte	0
	.short	8
	.short	7
	.short	0
	.long	8
	.byte	3
	.byte	0
	.short	8
	.short	7
	.short	0
	.long	8
	.p2align	3
	.short	0
	.short	0
	.p2align	3
