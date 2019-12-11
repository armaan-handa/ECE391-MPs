
# mp1.S - life-or-death version
# Good luck, and have fun!

.data
	
    # Constants for accessing the fields of a struct keystroke_args
    # struct keystroke_args is defined in rtc.h and mp1.h

	KA_SIZE	  = 14   # 14 bytes when copying
	KA_STACK  = 16   # use 16 bytes on stack for alignment
	DIRECTION = 0
	SELECTOR  = 2
	GUESS     = 4
	HINT      = 9

    # Constants for accessing the fields of a struct game_status
    # struct game_status is defined in rtc.h and mp1.h

	GS_SIZE   = 8	# 8 bytes
	POP 	  = 0
	INF	  = 4

    # Constants for screen size

	SCR_WIDTH  = 80
	SCR_HEIGHT = 20
	SCR_SIZE   = ((SCR_WIDTH) * (SCR_HEIGHT))

    # ASCII cell images

	CELL_LIVE = 0x78
	CELL_DEAD = 0x20


# Data shared between this file and rtc.c helper functions

# This '.globl' directive makes these symbols visible externally
.globl rand_seed, aggression

			.align 8
rand_seed:		.quad 0x0	# Pseudo-random number seed
aggression:		.long 0x0	# Aggression level of virus


# Data private to this file

current_board:		.long 0x0	# Address of current board
next_board:		.long 0x0	# Address of next board
population:		.long 0x0	# Remaining human population
infection:		.long 0x0	# Count of live virus cells
select_image:		.string "==>"	# Picture of the selector
select_blank:		.string "   "	# Picture to overwrite selector
base_array:             .string "ACGT"  # Array of characters for bases
live_cells:		.long 0x0
ioctl_jump_table:	.long mp1_ioctl_startgame, mp1_ioctl_endgame, mp1_ioctl_keystroke, mp1_ioctl_getstatus, mp1_ioctl_vaccinate
guess_table: .byte 0x1, 0x2, 0x4, 0x8 # table to perform and operations based on guess  
direction_table: .long left, down, right, up
.text

# void mp1_poke(void);
# You can use this function to write to video memory.
#
# Interface: Register-based arguments (not C-style)
#    Inputs: %cl  - The byte you wish to write
#            %eax - Offset from the start of video memory that you wish
#                   to write to
#   Outputs: Text-mode video screen is written to at location %eax with
#            the byte in %cl
# Registers: Clobbers EDX

mp1_poke:
		movl    vmem_base_addr(,1),%edx
		movb    %cl,(%edx,%eax,1)
		ret

#   ----------------- Exported functions ---------------------

# void mp1_rtc_tasklet(unsigned long ignore);
# Performs three tasks:
#	(1) updates the life game board and calculates infection count
#	(2) swaps the two boards
#	(3) applies population reduction based on infection count
#	(4) redraws the screen
#	(5) notifies the user code by calling mp1_notify_user
# Inputs   : none
# Outputs  : none
# Registers: Standard C calling convention
.globl neighbor_count
.globl mp1_notify_user
.globl mp1_rtc_tasklet
mp1_rtc_tasklet:
		pushl %ebp
		movl %esp, %ebp
		pushl %ebx # callee save
		pushl %esi
		pushl %edi
		
		cmpl $0, current_board # check is current_board is 0
		jnz current_board_nz
		
		popl %edi # callee save
		popl %esi
		popl %ebx
		leave
		ret

current_board_nz:
		xor %edx, %edx
		movl %edx, live_cells # reset live_cells
		# movl %edx, infection # reset infection val
		call update_next_board
		call swap_boards
		xor %edx, %edx # clear edx
		movl infection, %edx
		movl population, %eax
		subl %edx, %eax # subtract new infection val from population (set population to 0 if it goes neg)
		cmpl $0, %eax
		jg population_g_zero
		movl $0, %eax
population_g_zero:
		mov %eax, population
		push %ecx
		call draw_screen
		pop %ecx
		call mp1_notify_user

		popl %edi # callee save
		popl %esi
		popl %ebx
		leave
		ret

# void update_next_board()
# updates next board and sets infection level
# Inputs: none
# Outputs: none
# Registers: C calling convention
update_next_board:
		pushl %ebp
		movl %esp, %ebp
		pushl %ebx # callee save
		pushl %esi
		pushl %edi
		
		#clear regs
		xor %esi, %esi # index
		# mov $0, %esi
		xor %edi, %edi 
		xor %ecx, %ecx
		# mov %ecx, live_cells 
		mov current_board, %edi # current board base addr
		mov next_board, %ecx	# next board base addr

update_next_board_loop:
		inc %esi
		cmp $SCR_SIZE, %esi # check if loop is done
		jge done_update_board_loop

		mov %esi, %eax # copy index into eax
		movl $80, %ebx
		xor %edx, %edx
		div %ebx		# divide by 80 to check remainder for border conditions
		cmpl $0, %edx   # if remainder is 0 then cell is on left border
		je update_next_board_loop
		cmpl $79, %edx	# if remainder is 79 then cell is on right border
		je update_next_board_loop
		mov %esi, %eax # copy index into eax
		cmpl $80, %eax	# if cell idx is less than 80 then cell is on top border
		jl update_next_board_loop
		cmpl $1520, %eax	# if cell idx is greater than or equal to 1520 then cell is on bottom border
		jge update_next_board_loop
		
		mov %edi, %eax # load address of current cell
		add %esi, %eax
		pushl %ecx		# save ecx
		pushl %eax		# push address of current cell as argument for neighbour_count
		call neighbor_count
		add $0x04, %esp	# remove arguemnts from frame
		
		pushl %eax	# push return val of neighbour_count as arg2 for tick_result
		mov %edi, %eax # load address of current cell
		add %esi, %eax
		pushl %eax		# push address of current cell as arg1 for tick_result
		call tick_result
		add $0x08, %esp 	# remove arguments from frame
		
		pop %ecx # restore ecx after function calls
		movb %al, (%ecx, %esi) # update cell in next board
		cmpb $1, %al
		je cell_is_live
		
		jmp update_next_board_loop

cell_is_live:
		xor %eax, %eax
		add $1, live_cells
		jmp update_next_board_loop

done_update_board_loop:
		mov live_cells, %eax
		xor %ebx, %ebx # clear ebx
		mov $10, %ebx
		mul %ebx
		mov %eax, infection

		popl %edi # callee save
		popl %esi
		popl %ebx
		leave
		ret

# void swap_boards()
# Swaps boards
# inputs: none
# outputs: none
# registers: c calling convention
swap_boards:
		pushl %ebp
		movl %esp, %ebp
		pushl %ebx # callee save
		pushl %esi
		pushl %edi

		mov current_board, %ebx
		mov next_board, %esi
		mov %ebx, next_board
		mov %esi, current_board

		popl %edi # callee save
		popl %esi
		popl %ebx
		leave
		ret

# int mp1_ioctl(unsigned long arg, unsigned int cmd)
# The dispatch function for the MP1 ioctls - should use the cmd argument
# and a jumptable to execute one of the specific ioctls implemented below.
# Inputs   : unsigned long arg - parameter to the mp1_ioctl_....
#          : unsigned int cmd  - specifies which mp1_ioctl_... function 
#          :                     to execute
# Outputs  : Returns an integer - depends on which ioctl() function is called
# Registers: Standard C calling convention

.globl mp1_ioctl
mp1_ioctl:

		xor %ecx, %ecx # clear ecx
		mov 8(%esp), %ecx	# check cmd
		cmpl $4, %ecx	# check cmd validity
		jg cmd_is_invalid
		jmp *ioctl_jump_table(, %ecx, 4) # jump to function
cmd_is_invalid:
		mov $-1, %eax # return -1
		ret

# ----------------- Functions private to this file -------------------

# void draw_screen()
# draws the board onto the screen
# inputs: none
# outputs: none
# registers: C Calling convention
draw_screen:	
	# For simplicity, your implementation may assume a SCR_WIDTH of 80
		pushl %ebp
		movl %esp, %ebp
		pushl %ebx # callee save
		pushl %esi
		pushl %edi

		xor %edi, %edi
		xor %esi, %esi
		xor %ebx, %ebx # clear regs
		movl $SCR_SIZE, %esi
		mov current_board, %ebx

draw_screen_loop:
		movb (%ebx, %edi), %al
		cmpb $0, %al
		je draw_scrn_dead

		mov $CELL_LIVE, %cl	# cell is live
		mov %edi, %eax
		shl $1, %eax # mul by 2
		call mp1_poke
		inc %edi
		cmpl %esi, %edi # check if loop done
		jl draw_screen_loop
		jmp draw_scrn_done

draw_scrn_dead:
		mov $CELL_DEAD, %cl	# cell is dead
		mov %edi, %eax
		shl $1, %eax # mul by 2
		call mp1_poke
		inc %edi
		cmpl %esi, %edi	#check if loop done
		jl draw_screen_loop

draw_scrn_done:
		popl %edi # callee save
		popl %esi
		popl %ebx
		leave
		ret

.globl mp1_malloc
.globl mp1_free
.globl seed_generator
.globl init_virus
# int mp1_ioctl_startgame (unsigned long seed)
# initializes variables before game starts
# inputs: seed for RNG
# outputs: 0 for success, -1 for failure
# Registers: C calling convention
mp1_ioctl_startgame:

		pushl %ebp
		movl %esp, %ebp
		pushl %ebx # callee save
		pushl %esi
		pushl %edi

		mov 8(%ebp), %eax # save arg to give to seed generator
		push %eax 
		call seed_generator
		add $0x04, %esp # remove args from frame
	
		# mp1_malloc
		push $1600 # size of board
		call mp1_malloc
		add $0x4, %esp # remove args from frame
		mov %eax, current_board # allocated current board
		push $1600 # size of board
		call mp1_malloc
		add $0x4, %esp # remove args from frame
		mov %eax, next_board # allocated next board
		cmpl $0, %eax
		je malloc_failed
		mov current_board, %eax
		cmpl $0, %eax
		je malloc_failed
		mov $0, %ecx
		mov current_board, %ebx
		mov next_board, %edx
		mov $0, %al

init_boards_loop: # set all cells to 0
		movb %al, (%edx, %ecx)
		movb %al, (%ebx, %ecx)
		inc %ecx
		cmpl $SCR_SIZE, %ecx # check if all cells done
		jl init_boards_loop

init_boards_done:		# init_virus on current_board
		mov current_board, %eax
		pushl %eax
		call init_virus
		add $0x04, %esp # remove args from frame
		mov $80, %eax
		mov %eax, aggression	# agression = 80
		mov $8000000, %eax
		mov %eax, population # population = 8m
		mov $0, %eax

		popl %edi # callee save
		popl %esi
		popl %ebx
		leave
		ret
		# return -1 if allocation failed, 0 if success
malloc_failed: # incase allocation fails free mem and boards = 0x0
		movl current_board, %eax
		push %eax
		call mp1_free	# free current board
		add $0x04, %esp
		movl next_board, %eax
		push %eax		# free next_board
		call mp1_free
		add $0x04, %esp
		mov $0x0, %eax
		movl %eax, current_board
		movl %eax, next_board
		mov $-1, %eax # return -1

		popl %edi # callee save
		popl %esi
		popl %ebx
		leave
		ret

# int mp1_ioctl_endgame (unsigned long ignore)
# cleans up driver state at the end of the game
# inputs: none
# outputs: 0 for success
# Registers: C calling convention
mp1_ioctl_endgame:
		pushl %ebp
		movl %esp, %ebp
		pushl %ebx # callee save
		pushl %esi
		pushl %edi

		movl current_board, %eax
		push %eax
		call mp1_free	# free current board
		add $0x04, %esp
		movl next_board, %eax
		push %eax		# free next_board
		call mp1_free
		add $0x04, %esp
		mov $0, %eax
		mov %eax, current_board
		mov %eax, next_board

		mov $0, %eax
		popl %edi # callee save
		popl %esi
		popl %ebx
		leave
		ret

# int mp1_ioctl_keystroke (struct keystroke_args* keystroke_args)
# handles direction keystroke
# inputs: keystroke argument struct
# output: 0 for success, -1 for failure
# Registers: C calling convention
mp1_ioctl_keystroke:
		pushl %ebp
		movl %esp, %ebp
		pushl %ebx # callee save
		pushl %esi
		pushl %edi

		subl $KA_STACK, %esp # allocate local variable on stack 
		mov %esp, %esi
		mov 8(%ebp), %eax
		cmpl $0, %eax
		je copy_failed
		pushl $KA_SIZE
		pushl %eax
		pushl %esi
		call mp1_copy_from_user # copy struct from user space to kernel space
		addl $12, %esp	# remove args from frame
		cmpl $0, %eax
		jne copy_failed
		xor %eax, %eax # clear eax
		movw DIRECTION(%esi), %ax
		jmp *direction_table(, %eax, 4) # jump table to go to correct action
up:
		xor %ebx, %ebx
		xor %ecx, %ecx	# clear regs
		movw SELECTOR(%esi), %bx # bx = selector
		movb HINT(%esi, %ebx), %cl	# cl = hint
		movb GUESS(%esi, %ebx), %al # al = guess
		movb guess_table(%eax), %al # give al a certain val to enable bit shift checking
up_loop:		
		shl $1, %al	# change base of guess
		cmpb $8, %al
		jle up_no_cycle
		xor %al, %al
		add $1, %al
up_no_cycle:
		movb %al, %ah	# ah = guess buffer
		and %cl, %ah
		cmpb $0, %ah
		jnz up_down_done
		jmp up_loop
down:
		xor %ebx, %ebx
		xor %ecx, %ecx	# clear regs
		movw SELECTOR(%esi), %bx # bx = selector
		movb HINT(%esi, %ebx), %cl	# cl = hint
		movb GUESS(%esi, %ebx), %al # al = guess
		movb guess_table(%eax), %al # give al a certain val to enable bit shift checking	
down_loop:		
		shr $1, %al	# change base of guess
		cmpb $0, %al
		jne down_no_cycle
		add $8, %al
down_no_cycle:
		movb %al, %ah	# ah = guess buffer
		and %cl, %ah
		cmpb $0, %ah
		jne up_down_done
		jmp down_loop

up_down_done:
		cmpb $1, %al
		je guess_is_A
		cmpb $2, %al
		je guess_is_C
		cmpb $4, %al
		je guess_is_G
		xor %eax, %eax # clear eax
		movb $3, %al
		jmp set_guess
guess_is_A:
		xor %eax, %eax # clear eax
		movb $0, %al
		jmp set_guess
guess_is_C:
		xor %eax, %eax # clear eax
		movb $1, %al
		jmp set_guess
guess_is_G:
		xor %eax, %eax # clear eax
		movb $2, %al
set_guess:
		movb %al, GUESS(%esi, %ebx)
		xor %ecx, %ecx
		leal base_array, %ecx
		movb (%ecx, %eax), %cl
		movl %ebx, %eax
		movl $10, %edx
		mul %edx # eax = 10S
		addl $1778, %eax # 1778 = 22*SCR_WIDTH + 18; eax = 1778 + 10S
		shl $1, %eax # mul by 2
		call mp1_poke
		mov 8(%ebp), %eax
		pushl $KA_SIZE
		pushl %esi
		pushl %eax
		call mp1_copy_to_user # copy struct to user space from kernel space
		addl $12, %esp	# remove args from frame
		cmp $0, %eax
		jne copy_failed
		jmp keystroke_done

right:
		xor %ebx, %ebx	# clear ebx
		movw SELECTOR(%esi), %bx # bx = selector
		call clear_selector
		inc %bx
		cmpw $5, %bx
		jne right_no_cycle
		mov $0, %bx
right_no_cycle:
		movw %bx, SELECTOR(%esi) # update selector
		call print_selector
		jmp right_left_done
left:
		xor %ebx, %ebx # clear ebx
		movw SELECTOR(%esi), %bx # bx = selector
		call clear_selector
		dec %bx
		cmpw $0, %bx
		jge left_no_cycle
		mov $4, %bx
left_no_cycle:
		movw %bx, SELECTOR(%esi) # update selector
		call print_selector

right_left_done:
		mov 8(%ebp), %eax
		pushl $KA_SIZE
		pushl %esi
		pushl %eax
		call mp1_copy_to_user # copy struct to user space from kernel space
		addl $12, %esp	# remove args from frame
		cmp $0, %eax
		jne copy_failed

keystroke_done:
		addl $KA_STACK, %esp # remove local variable from stack
		mov $0, %eax 
		popl %edi # callee save
		popl %esi
		popl %ebx
		leave
		ret

copy_failed:
		addl $KA_STACK, %esp # remove local variable from stack 
		mov $-1, %eax
		popl %edi # callee save
		popl %esi
		popl %ebx
		leave
		ret

# void clear_selector()
# clears selector in order to move it
# inputs: ebx = selector
# outputs: none
# Registers: C calling convention, except for ebx which is used as another input 
clear_selector: # ebx = selector
		pushl %ebp
		movl %esp, %ebp
		pushl %ebx # callee save
		pushl %esi
		pushl %edi

		movl %ebx, %eax
		movl $10, %edx
		mul %edx # eax = 10S
		addl $1774, %eax # 1774 = 22*SCR_WIDTH + 14; eax = 1774 + 10S
		shl $1, %eax # mul by 2
		xor %ebx, %ebx # clear ebx
clear_selector_loop:
		mov select_blank(%ebx), %cl 
		call mp1_poke
		incl %ebx
		add $2, %eax
		cmpl $2, %ebx
		jle clear_selector_loop

		popl %edi # callee save
		popl %esi
		popl %ebx
		leave
		ret

# void print_selector()
# prints new selector in order to move it
# inputs: ebx = selector
# outputs: none
# Registers: C calling convention, except for ebx which is used as another input 
print_selector: # ebx = selector
		pushl %ebp
		movl %esp, %ebp
		pushl %ebx # callee save
		pushl %esi
		pushl %edi

		movl %ebx, %eax
		movl $10, %edx
		mul %edx # eax = 10S
		addl $1774, %eax # 1774 = 22*SCR_WIDTH + 14; eax = 1774 + 10S
		shl $1, %eax # mul by 2
		xor %ebx, %ebx # clear ebx
print_selector_loop:
		mov select_image(%ebx), %cl 
		call mp1_poke
		incl %ebx
		add $2, %eax
		cmpl $2, %ebx
		jle print_selector_loop

		popl %edi # callee save
		popl %esi
		popl %ebx
		leave
		ret

# int mp1_ioctl_getstatus (unsigned long* user_status)
# allows user code to retrieve current population and virus count
# inputs: destination for status
# outputs: 0 for success, -1 for failure
# Registers: C calling convention
mp1_ioctl_getstatus:
		pushl %ebp
		movl %esp, %ebp
		pushl %ebx # callee save
		pushl %esi
		pushl %edi

		subl $GS_SIZE, %esp # allocate local variable on stack
		mov population, %eax
		# mov $POP, %edi
		mov %eax,  POP(%esp)
		mov infection, %eax
		# mov $INF, %edi
		mov %eax, INF(%esp) 
		mov 8(%ebp), %eax
		cmp $0, %eax
		je gs_fail
		movl %esp, %esi
		pushl $GS_SIZE
		pushl %esi
		pushl %eax
		call mp1_copy_to_user # copy struct from user space to kernel space
		cmp $0, %eax
		je gs_success
gs_fail:
		addl $GS_SIZE, %esp # remove local variable from stack
		mov $-1, %eax 
		popl %edi # callee save
		popl %esi
		popl %ebx
		leave
		ret
gs_success:
		addl $GS_SIZE, %esp # remove local variable from stack
		mov $0, %eax 
		popl %edi # callee save
		popl %esi
		popl %ebx
		leave
		ret

.globl generate
# int mp1_ioctl_vaccinate (unsigned long packed_args)
# handles vaccination
# inputs: percentage for determine whether or not to kill cell, amount to increase aggression by
# outputs: always 0
# Registers: C calling convention
mp1_ioctl_vaccinate:
		pushl %ebp
		movl %esp, %ebp
		pushl %ebx # callee save
		pushl %esi
		pushl %edi

		xor %eax, %eax
		xor %ebx, %ebx # clear registers
		mov 8(%ebp), %eax
		mov %ax, %bx # bx = percentage
		shr $16, %eax # ax = aggression increment
		
		mov current_board, %edi
		xor %esi, %esi
vaccinate_loop:
		push %eax # caller save
		call generate
		mov $100, %ecx
		xor %edx, %edx
		div %ecx # divide random number by 100
		cmp %dx, %bx	#compair remainder to percentage
		jl dont_kill_cell
		pop %eax
		xor %edx, %edx
		movb %dl, (%edi, %esi) # kill cell 
		jmp vacc_loop_control

dont_kill_cell:
		pop %eax

vacc_loop_control:
		inc %esi
		cmp $SCR_SIZE, %esi
		jge vacc_loop_done
		jmp vaccinate_loop

vacc_loop_done:
		add aggression, %eax
		mov %eax, aggression

		mov $0, %eax # return 0
		popl %edi # callee save
		popl %esi
		popl %ebx
		leave
		ret
