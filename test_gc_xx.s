	.text
	.file	"test_gc"
	.section	.init_array.00102,"ax",@progbits
	.globl	foo                             # -- Begin function foo
	.p2align	4, 0x90
	.type	foo,@function
foo:                                    # @foo
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rbx
	.cfi_def_cfa_offset 16
	.cfi_offset %rbx, -16
	movl	$100, %edi
	callq	malloc@PLT
	movq	%rax, %rbx
	movb	$10, (%rax)
	callq	bar@PLT
	movb	$20, (%rbx)
	popq	%rbx
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end0:
	.size	foo, .Lfunc_end0-foo
	.cfi_endproc
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
