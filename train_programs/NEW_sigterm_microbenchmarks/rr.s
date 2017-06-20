	.file	"rr.c"
	.globl	KB
	.section	.rodata
	.align 4
	.type	KB, @object
	.size	KB, 4
KB:
	.long	1024
	.globl	MB
	.align 8
	.type	MB, @object
	.size	MB, 8
MB:
	.quad	1048576
	.globl	GB
	.align 8
	.type	GB, @object
	.size	GB, 8
GB:
	.quad	1073741824
	.comm	number_of_threads,4,4
	.comm	pin,4,4
	.comm	pol,4,4
	.comm	loops_per_thread,3200,32
.LC1:
	.string	"before ... bzero"
.LC3:
	.string	"Never reaches this point!\n"
.LC4:
	.string	"%c\n"
	.text
	.globl	inc
	.type	inc, @function
inc:
.LFB24:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$160, %rsp
	movq	%rdi, -152(%rbp)
	movq	%fs:40, %rax
	movq	%rax, -8(%rbp)
	xorl	%eax, %eax
	movq	-152(%rbp), %rax
	movq	%rax, -104(%rbp)
	movl	$1073741824, %edx
	movq	%rdx, %rax
	salq	$2, %rax
	addq	%rdx, %rax
	movq	%rax, -96(%rbp)
	movl	$1000000, -140(%rbp)
	movq	-104(%rbp), %rax
	movl	8(%rax), %eax
	movl	%eax, -136(%rbp)
	leaq	-48(%rbp), %rax
	movq	%rax, %rsi
	movl	$1, %edi
	call	clock_gettime
	movq	-96(%rbp), %rax
	movl	-136(%rbp), %edx
	movl	%edx, %esi
	movq	%rax, %rdi
	call	numa_alloc_onnode
	movq	%rax, -88(%rbp)
	movl	$.LC1, %edi
	call	puts
	movq	-96(%rbp), %rdx
	movq	-88(%rbp), %rax
	movl	$0, %esi
	movq	%rax, %rdi
	call	memset
	leaq	-32(%rbp), %rax
	movq	%rax, %rsi
	movl	$1, %edi
	call	clock_gettime
	movq	-32(%rbp), %rdx
	movq	-48(%rbp), %rax
	subq	%rax, %rdx
	movq	%rdx, %rax
	pxor	%xmm0, %xmm0
	cvtsi2sdq	%rax, %xmm0
	movsd	%xmm0, -80(%rbp)
	movsd	.LC2(%rip), %xmm0
	ucomisd	-80(%rbp), %xmm0
	jbe	.L2
	movsd	.LC2(%rip), %xmm0
	subsd	-80(%rbp), %xmm0
	cvttsd2siq	%xmm0, %rax
	movl	%eax, %edi
	call	sleep
.L2:
	movb	$0, -141(%rbp)
	movq	-104(%rbp), %rax
	movl	(%rax), %eax
	movl	%eax, -132(%rbp)
	movq	$0, -72(%rbp)
	movq	$16384, -64(%rbp)
	movq	$0, -128(%rbp)
	jmp	.L4
.L9:
	movq	-96(%rbp), %rax
	leaq	63(%rax), %rdx
	testq	%rax, %rax
	cmovs	%rdx, %rax
	sarq	$6, %rax
	movq	%rax, -56(%rbp)
	movq	$0, -120(%rbp)
	jmp	.L5
.L8:
	movq	-120(%rbp), %rax
	movq	%rax, -112(%rbp)
	jmp	.L6
.L7:
	movq	-112(%rbp), %rax
	leaq	0(,%rax,8), %rdx
	movq	-88(%rbp), %rax
	addq	%rdx, %rax
	movq	$0, (%rax)
	addq	$1, -112(%rbp)
.L6:
	movq	-120(%rbp), %rdx
	movq	-64(%rbp), %rax
	addq	%rdx, %rax
	cmpq	-112(%rbp), %rax
	jg	.L7
	movl	-132(%rbp), %eax
	sall	$3, %eax
	movslq	%eax, %rdx
	movq	loops_per_thread(,%rdx,8), %rdx
	addq	$1, %rdx
	cltq
	movq	%rdx, loops_per_thread(,%rax,8)
	movq	-64(%rbp), %rax
	addq	%rax, -120(%rbp)
.L5:
	movq	-56(%rbp), %rax
	subq	-64(%rbp), %rax
	cmpq	-120(%rbp), %rax
	jg	.L8
	addq	$1, -128(%rbp)
.L4:
	movl	-140(%rbp), %eax
	cltq
	cmpq	-128(%rbp), %rax
	jg	.L9
	movq	stderr(%rip), %rax
	movq	%rax, %rcx
	movl	$26, %edx
	movl	$1, %esi
	movl	$.LC3, %edi
	call	fwrite
	movsbl	-141(%rbp), %eax
	movl	%eax, %esi
	movl	$.LC4, %edi
	movl	$0, %eax
	call	printf
	movl	$0, %eax
	movq	-8(%rbp), %rcx
	xorq	%fs:40, %rcx
	je	.L11
	call	__stack_chk_fail
.L11:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE24:
	.size	inc, .-inc
	.section	.rodata
.LC5:
	.string	"received SIGINT: %lld\n"
	.align 8
.LC6:
	.string	"total_loops_threads_%d_policy_%d_pin_%d"
.LC7:
	.string	"w"
.LC8:
	.string	"couldn't open the file\n"
.LC9:
	.string	"Loops per thread\n%.2lf\n"
.LC10:
	.string	"couldn't close the file"
	.text
	.globl	sig_handler
	.type	sig_handler, @function
sig_handler:
.LFB25:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$560, %rsp
	movl	%edi, -548(%rbp)
	movq	%fs:40, %rax
	movq	%rax, -8(%rbp)
	xorl	%eax, %eax
	movq	$0, -528(%rbp)
	cmpl	$2, -548(%rbp)
	jne	.L14
	movl	$0, -532(%rbp)
	jmp	.L15
.L16:
	movl	-532(%rbp), %eax
	sall	$3, %eax
	cltq
	movq	loops_per_thread(,%rax,8), %rax
	addq	%rax, -528(%rbp)
	addl	$1, -532(%rbp)
.L15:
	movl	number_of_threads(%rip), %eax
	cmpl	%eax, -532(%rbp)
	jl	.L16
	movq	-528(%rbp), %rax
	movq	%rax, %rsi
	movl	$.LC5, %edi
	movl	$0, %eax
	call	printf
.L14:
	movl	pin(%rip), %esi
	movl	pol(%rip), %ecx
	movl	number_of_threads(%rip), %edx
	leaq	-512(%rbp), %rax
	movl	%esi, %r8d
	movl	$.LC6, %esi
	movq	%rax, %rdi
	movl	$0, %eax
	call	sprintf
	leaq	-512(%rbp), %rax
	movl	$.LC7, %esi
	movq	%rax, %rdi
	call	fopen
	movq	%rax, -520(%rbp)
	cmpq	$0, -520(%rbp)
	jne	.L17
	movl	$.LC8, %edi
	call	perror
.L17:
	pxor	%xmm0, %xmm0
	cvtsi2sdq	-528(%rbp), %xmm0
	movl	number_of_threads(%rip), %eax
	pxor	%xmm1, %xmm1
	cvtsi2sd	%eax, %xmm1
	divsd	%xmm1, %xmm0
	movq	-520(%rbp), %rax
	movl	$.LC9, %esi
	movq	%rax, %rdi
	movl	$1, %eax
	call	fprintf
	movq	-520(%rbp), %rax
	movq	%rax, %rdi
	call	fclose
	testl	%eax, %eax
	je	.L18
	movl	$.LC10, %edi
	call	perror
.L18:
	movl	$1, %edi
	call	exit
	.cfi_endproc
.LFE25:
	.size	sig_handler, .-sig_handler
	.section	.rodata
.LC11:
	.string	"couldn't set up signal"
.LC12:
	.string	"P input: (%s)\n"
.LC13:
	.string	"Use t, r, s, or p as options!"
.LC14:
	.string	"t:s:p"
.LC15:
	.string	"I'm about to pin!"
.LC16:
	.string	"The number of threads is: %d\n"
.LC17:
	.string	"Used cores are:"
.LC18:
	.string	"Error creating thread\n"
.LC19:
	.string	"I'm about to pin!\n"
.LC20:
	.string	"%d (node: %d)\n"
.LC21:
	.string	"Core: %d going to node: %d\n"
	.align 8
.LC22:
	.string	"Something went wrong while setting the affinity!\n"
	.align 32
.LC0:
	.long	0
	.long	4
	.long	8
	.long	12
	.long	16
	.long	20
	.long	24
	.long	28
	.long	32
	.long	36
	.long	40
	.long	44
	.long	1
	.long	5
	.long	9
	.long	13
	.long	17
	.long	21
	.long	25
	.long	29
	.long	33
	.long	37
	.long	41
	.long	45
	.long	2
	.long	6
	.long	10
	.long	14
	.long	18
	.long	22
	.long	26
	.long	30
	.long	34
	.long	38
	.long	42
	.long	46
	.long	3
	.long	7
	.long	11
	.long	15
	.long	19
	.long	23
	.long	27
	.long	31
	.long	35
	.long	39
	.long	43
	.long	47
	.text
	.globl	main
	.type	main, @function
main:
.LFB26:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$888, %rsp
	.cfi_offset 15, -24
	.cfi_offset 14, -32
	.cfi_offset 13, -40
	.cfi_offset 12, -48
	.cfi_offset 3, -56
	movl	%edi, -916(%rbp)
	movq	%rsi, -928(%rbp)
	movq	%fs:40, %rax
	movq	%rax, -56(%rbp)
	xorl	%eax, %eax
	movl	$0, %edi
	call	time
	movl	%eax, %edi
	call	srand
	movl	$sig_handler, %esi
	movl	$2, %edi
	call	signal
	cmpq	$-1, %rax
	jne	.L21
	movl	$.LC11, %edi
	call	perror
.L21:
	movl	$0, pin(%rip)
	jmp	.L22
.L27:
	movl	-896(%rbp), %eax
	cmpl	$115, %eax
	je	.L24
	cmpl	$116, %eax
	je	.L25
	cmpl	$112, %eax
	je	.L26
	jmp	.L49
.L25:
	movq	optarg(%rip), %rax
	movq	%rax, %rdi
	call	atoi
	movl	%eax, number_of_threads(%rip)
	jmp	.L22
.L24:
	movq	optarg(%rip), %rax
	movq	%rax, %rsi
	movl	$.LC12, %edi
	movl	$0, %eax
	call	printf
	movq	optarg(%rip), %rax
	movq	%rax, %rdi
	call	atoi
	movl	%eax, pol(%rip)
	jmp	.L22
.L26:
	movl	$1, pin(%rip)
	jmp	.L22
.L49:
	movl	$.LC13, %edi
	call	puts
.L22:
	movq	-928(%rbp), %rcx
	movl	-916(%rbp), %eax
	movl	$.LC14, %edx
	movq	%rcx, %rsi
	movl	%eax, %edi
	call	getopt
	movl	%eax, -896(%rbp)
	cmpl	$-1, -896(%rbp)
	jne	.L27
	movl	number_of_threads(%rip), %eax
	movq	%rsp, %rdx
	movq	%rdx, %rbx
	movslq	%eax, %rdx
	subq	$1, %rdx
	movq	%rdx, -888(%rbp)
	movslq	%eax, %rdx
	movq	%rdx, %r14
	movl	$0, %r15d
	movslq	%eax, %rdx
	movq	%rdx, %r12
	movl	$0, %r13d
	cltq
	salq	$3, %rax
	leaq	7(%rax), %rdx
	movl	$16, %eax
	subq	$1, %rax
	addq	%rdx, %rax
	movl	$16, %esi
	movl	$0, %edx
	divq	%rsi
	imulq	$16, %rax, %rax
	subq	%rax, %rsp
	movq	%rsp, %rax
	addq	$7, %rax
	shrq	$3, %rax
	salq	$3, %rax
	movq	%rax, -880(%rbp)
	movl	$0, -848(%rbp)
	movl	$48, -844(%rbp)
	movl	$4, -840(%rbp)
	movl	$52, -836(%rbp)
	movl	$8, -832(%rbp)
	movl	$56, -828(%rbp)
	movl	$12, -824(%rbp)
	movl	$60, -820(%rbp)
	movl	$16, -816(%rbp)
	movl	$64, -812(%rbp)
	movl	$20, -808(%rbp)
	movl	$68, -804(%rbp)
	movl	$24, -800(%rbp)
	movl	$72, -796(%rbp)
	movl	$28, -792(%rbp)
	movl	$76, -788(%rbp)
	movl	$32, -784(%rbp)
	movl	$80, -780(%rbp)
	movl	$36, -776(%rbp)
	movl	$84, -772(%rbp)
	leaq	-640(%rbp), %rax
	movl	$.LC0, %esi
	movl	$24, %edx
	movq	%rax, %rdi
	movq	%rdx, %rcx
	rep movsq
	movl	$0, -912(%rbp)
	jmp	.L28
.L29:
	movl	-912(%rbp), %eax
	cltq
	movl	-912(%rbp), %edx
	movl	%edx, -448(%rbp,%rax,4)
	addl	$1, -912(%rbp)
.L28:
	cmpl	$47, -912(%rbp)
	jle	.L29
	movl	$0, -908(%rbp)
	jmp	.L30
.L33:
	movl	pol(%rip), %eax
	testl	%eax, %eax
	jne	.L31
	movl	-908(%rbp), %eax
	cltq
	movl	-640(%rbp,%rax,4), %edx
	movl	-908(%rbp), %eax
	cltq
	movl	%edx, -256(%rbp,%rax,4)
	jmp	.L32
.L31:
	movl	-908(%rbp), %eax
	cltq
	movl	-448(%rbp,%rax,4), %edx
	movl	-908(%rbp), %eax
	cltq
	movl	%edx, -256(%rbp,%rax,4)
.L32:
	addl	$1, -908(%rbp)
.L30:
	cmpl	$47, -908(%rbp)
	jle	.L33
	movl	pin(%rip), %eax
	testl	%eax, %eax
	je	.L34
	movl	$.LC15, %edi
	call	puts
	leaq	-768(%rbp), %rax
	movq	%rax, %rsi
	movl	$0, %eax
	movl	$16, %edx
	movq	%rsi, %rdi
	movq	%rdx, %rcx
	rep stosq
	movl	-256(%rbp), %eax
	cltq
	movq	%rax, -872(%rbp)
	cmpq	$1023, -872(%rbp)
	ja	.L36
	movq	-872(%rbp), %rax
	shrq	$6, %rax
	leaq	0(,%rax,8), %rdx
	leaq	-768(%rbp), %rcx
	addq	%rcx, %rdx
	leaq	0(,%rax,8), %rcx
	leaq	-768(%rbp), %rax
	addq	%rcx, %rax
	movq	(%rax), %rax
	movq	-872(%rbp), %rcx
	andl	$63, %ecx
	movl	$1, %esi
	salq	%cl, %rsi
	movq	%rsi, %rcx
	orq	%rcx, %rax
	movq	%rax, (%rdx)
.L36:
	leaq	-768(%rbp), %rax
	movq	%rax, %rdx
	movl	$128, %esi
	movl	$0, %edi
	call	sched_setaffinity
.L34:
	movl	number_of_threads(%rip), %eax
	movl	%eax, %esi
	movl	$.LC16, %edi
	movl	$0, %eax
	call	printf
	movl	$.LC17, %edi
	call	puts
	movl	$0, -892(%rbp)
	movl	$0, -904(%rbp)
	jmp	.L37
.L44:
	movl	$12, %edi
	call	malloc
	movq	%rax, -864(%rbp)
	movq	-864(%rbp), %rax
	movl	-904(%rbp), %edx
	movl	%edx, (%rax)
	movl	-904(%rbp), %eax
	cltq
	leaq	0(,%rax,8), %rdx
	movq	-880(%rbp), %rax
	leaq	(%rdx,%rax), %rdi
	movq	-864(%rbp), %rax
	movq	%rax, %rcx
	movl	$inc, %edx
	movl	$0, %esi
	call	pthread_create
	testl	%eax, %eax
	je	.L38
	movq	stderr(%rip), %rax
	movq	%rax, %rcx
	movl	$22, %edx
	movl	$1, %esi
	movl	$.LC18, %edi
	call	fwrite
	movl	$1, %eax
	jmp	.L39
.L38:
	movl	pin(%rip), %eax
	testl	%eax, %eax
	je	.L40
	movq	stderr(%rip), %rax
	movq	%rax, %rcx
	movl	$18, %edx
	movl	$1, %esi
	movl	$.LC19, %edi
	call	fwrite
	leaq	-768(%rbp), %rax
	movq	%rax, %rsi
	movl	$0, %eax
	movl	$16, %edx
	movq	%rsi, %rdi
	movq	%rdx, %rcx
	rep stosq
	movl	-904(%rbp), %eax
	cltq
	movl	-256(%rbp,%rax,4), %eax
	cltq
	movq	%rax, -856(%rbp)
	cmpq	$1023, -856(%rbp)
	ja	.L42
	movq	-856(%rbp), %rax
	shrq	$6, %rax
	leaq	0(,%rax,8), %rdx
	leaq	-768(%rbp), %rcx
	addq	%rcx, %rdx
	leaq	0(,%rax,8), %rcx
	leaq	-768(%rbp), %rax
	addq	%rcx, %rax
	movq	(%rax), %rax
	movq	-856(%rbp), %rcx
	andl	$63, %ecx
	movl	$1, %esi
	salq	%cl, %rsi
	movq	%rsi, %rcx
	orq	%rcx, %rax
	movq	%rax, (%rdx)
.L42:
	movl	-904(%rbp), %eax
	cltq
	movl	-256(%rbp,%rax,4), %eax
	cltd
	shrl	$30, %edx
	addl	%edx, %eax
	andl	$3, %eax
	subl	%edx, %eax
	movl	%eax, %edx
	movq	-864(%rbp), %rax
	movl	%edx, 8(%rax)
	movq	-864(%rbp), %rax
	movl	8(%rax), %edx
	movl	-904(%rbp), %eax
	cltq
	movl	-256(%rbp,%rax,4), %eax
	movl	%eax, %esi
	movl	$.LC20, %edi
	movl	$0, %eax
	call	printf
	movq	-864(%rbp), %rax
	movl	8(%rax), %ecx
	movl	-904(%rbp), %eax
	cltq
	movl	-256(%rbp,%rax,4), %edx
	movq	stderr(%rip), %rax
	movl	$.LC21, %esi
	movq	%rax, %rdi
	movl	$0, %eax
	call	fprintf
	movq	-880(%rbp), %rax
	movl	-904(%rbp), %edx
	movslq	%edx, %rdx
	movq	(%rax,%rdx,8), %rax
	leaq	-768(%rbp), %rdx
	movl	$128, %esi
	movq	%rax, %rdi
	call	pthread_setaffinity_np
	testl	%eax, %eax
	je	.L40
	movl	$.LC22, %edi
	call	perror
	movl	$1, %eax
	jmp	.L39
.L40:
	addl	$1, -904(%rbp)
.L37:
	movl	number_of_threads(%rip), %eax
	cmpl	%eax, -904(%rbp)
	jl	.L44
	movl	$10, %edi
	call	putchar
	movl	$0, -900(%rbp)
	jmp	.L45
.L46:
	movq	-880(%rbp), %rax
	movl	-900(%rbp), %edx
	movslq	%edx, %rdx
	movq	(%rax,%rdx,8), %rax
	movl	$0, %esi
	movq	%rax, %rdi
	call	pthread_join
	addl	$1, -900(%rbp)
.L45:
	movl	number_of_threads(%rip), %eax
	cmpl	%eax, -900(%rbp)
	jl	.L46
	movl	$0, %eax
.L39:
	movq	%rbx, %rsp
	movq	-56(%rbp), %rbx
	xorq	%fs:40, %rbx
	je	.L48
	call	__stack_chk_fail
.L48:
	leaq	-40(%rbp), %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE26:
	.size	main, .-main
	.section	.rodata
	.align 8
.LC2:
	.long	0
	.long	1077149696
	.ident	"GCC: (Ubuntu 5.4.0-6ubuntu1~16.04.4) 5.4.0 20160609"
	.section	.note.GNU-stack,"",@progbits
