.text
.globl system_call_handler

# system_call_handler();
# Generic linkage function that takes arguments and calls system call functions
# Inputs   : %eax - Call number
#            %ebx, %ecx, %edx - arguments of system call, first to last
# Outputs  : %eax - return value of system call. -1 on failure
# Registers: Saves all registers. Writes return value in %eax
system_call_handler:
	# check if call number valid in [1,10]
	cli
	cmp		$10, %eax 
	ja		invalid
	cmp 	$0, %eax
	jle     invalid
	
	# store registers
	pushal
	# store flags
	pushfl
	# push arguments of system call
	pushl	%edx 
	pushl	%ecx
	pushl	%ebx
	
	# jump to proper handler
	mov $0x18, %bx
	mov %bx, %ds # set ds
#	mov $0x10, %bx
#	mov %bx, %cs # set cs
	decl %eax
	sti
	call	*syscall_jump_table(, %eax, 4)	
	
	# store return value
	movl	%eax, retval
	
	# pop 12 bytes of arguments off stack
	add		$12, %esp 
#	mov $0x23, %bx
#	mov %bx, %cs # restore cs
	mov $0x2B, %bx
	mov %bx, %ds # restore ds

	# restore flags
	popfl
	
	# restore registers
	popal
	# restore return value
	movl 	retval, %eax
	iret
	
invalid:
	# return failure
	mov 	$-1, %eax 
	iret
	
retval:
	.long 0x0
	
# jump table to system call C functions, ordered by number
syscall_jump_table:
	.long	halt_syscall
	.long	execute_syscall
	.long	read_syscall
	.long	write_syscall
	.long	open_syscall
	.long	close_syscall
	.long	getargs_syscall
	.long	vidmap_syscall
	.long	set_handler_syscall
	.long	sigreturn_syscall

