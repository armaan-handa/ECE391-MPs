#include "types.h"
#include "scheduling.h"
#include "pcb.h"
#include "paging.h"
#include "x86_desc.h"
#include "rtc.h"
#include "lib.h"
#include "i8259.h"
#include "syscall.h"

#define USER_VMEM (FOUR_MI_B * 32)

uint8_t round_robin_counter = 0; // number of terminal whose process currently being run
uint8_t active_terminal = 0; // ID of visible terminal to switch to. Set by keyboard.

// cursor stuff
extern int screen_y;
extern int screen_x;
extern int screen_x_cache[3]; // one per terminal
extern int screen_y_cache[3];


/*
 *  switch_task
 *	Switchs the active task for the scheduler
 *  Input: new_pid - the pid of the task to switch to
 *  Output: none
 *  Side effects: Stack switching, assigns pages for background tasks
 */
void switch_task (int32_t new_pid) {
	// if pid is invalid
	if(new_pid < 0)
	{
		return;
	}
	
	// get pcbs
	pcb_t * old_pcb = get_pcb();
    pcb_t * new_pcb = (pcb_t*) ((FOUR_MI_B * 2) - ((FOUR_KI_B * 2) * (new_pid + 1)));

	// set cursor to active task cursor
	screen_x_cache[old_pcb->terminal] = screen_x;
	screen_y_cache[old_pcb->terminal] = screen_y;
	screen_y = screen_y_cache[new_pcb->terminal];
	screen_x = screen_x_cache[new_pcb->terminal];

	// check pcb validity
	if (new_pcb == old_pcb) {
		return;
	}
	
	if (new_pcb->terminal > 2) {
		return;
	}

	if (old_pcb->terminal > 2) {
		return;
	}


	// assign video memory paging

	if (new_pcb->terminal == current_terminal) {
		page_on_4kb ((void*) (VIDEO), (void*) (map_loc));
		page_on_4kb ((void*) (VIDEO), (void*) (VIDEO));

	} else {
		page_on_4kb ((void*) (vmem_buffers[new_pcb->terminal]), (void*) (map_loc));
		page_on_4kb ((void*) (vmem_buffers[new_pcb->terminal]), (void*) (VIDEO));
	}

    // restore user space paging to 128MB
    page_on_4mb ((void*) (FOUR_MI_B * 2 + (new_pid + 1) * FOUR_MI_B), (void*) (USER_VMEM));

    // set up TSS entry
	tss.ss0 = KERNEL_DS;
    tss.esp0 = (FOUR_MI_B * 2) - ((FOUR_KI_B * 2) * (new_pid)) - 4;

	// store ESP and EBP
	asm volatile(
		"movl %%esp, %0       # store old stack pointer \n\
		movl %%ebp, %1		 # store old base pointer \n\
		"
		: "=m" (old_pcb->esp), "=m" (old_pcb->ebp)
	);

    // Assembly stuff for return to execute.
    // Return
	asm volatile("movl %0, %%esp \n\
        movl %1, %%ebp"
    : // No Output
    :   "m" (new_pcb->esp), "m" (new_pcb->ebp)
    );
    return;
}

/*
 *  init_pit
 *	Starts the PIT and initalizes the clocking and channels
 *  Input: none
 *  Output: none
 *  Side effects: turns off interupts
 */
void init_pit() {
	outb(SQUARE_WAVE, PIT_COMMAND);						//Tell PIT which channel to set
	outb((_100HZ & 0xFF), PIT_CHAN0);					//Send low 8 bits
	outb((_100HZ >> 8), PIT_CHAN0);	//Send high 8 bits
	enable_irq(PIT_IRQ_NUM);									//Enabling PIT interrupts
	return;
}

/*
 *  pit_handler
 *	Loops through the scheduled tasks and graps locks then passes locks to
 *  	terminal switch to perform the swap
 *  Input: none
 *  Output: none
 *  Side effects: Grabs locks and stores EBP and ESP
 */
void pit_handler() {
	send_eoi(PIT_IRQ_NUM);
	cli();
	// set to next task
	round_robin_counter++;
	round_robin_counter %= 3;
	// reset terminal
	swap_terminal(active_terminal);
	// switch task
	switch_task(terminal_processes[round_robin_counter]);
	sti();
	return;
}
