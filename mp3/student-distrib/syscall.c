#include "syscall.h"
#include "x86_desc.h"
#include "parsing.h"
#include "filesys.h"
#include "paging.h"
#include "pcb.h"
#include "lib.h"
#include "keyboard.h"
#include "types.h"
#include "i8259.h"
#include "scheduling.h"

#define USER_VMEM (FOUR_MI_B * 32)

int process_count = -1; // number of active processes
int current_terminal = 0; // currently visible terminal

// Stores the PID of the top process in each terminal
// Each terminal's process number starts at its terminal number - 3. Each process on top of another process has that process's PID + 3.
// So, the PID of a process % 3 will always be its terminal number.
int terminal_processes[3] = {-3, -2, -1}; 
// flag to track if swapping off of an execute
int swap_flag = -1;
int halt_flag;

// back buffers for terminal data
char key_bufs[3][BUF_LIMIT];
uint8_t key_buf_idxs[3] = {0,0,0};
extern char key_buf[BUF_LIMIT];
extern uint8_t key_buf_idx;
int screen_x_cache[3] = {0,0,0};
int screen_y_cache[3] = {NUM_ROWS-1,NUM_ROWS-1,NUM_ROWS-1};
extern int screen_x;
extern int screen_y;

uint8_t* map_loc = (uint8_t*) (USER_VMEM + FOUR_MI_B); // Maps to VGA Memory through vidmap
uint8_t* vmem_buffers[3]; // Pointers to 3 back buffers storing video memory

/*
 * swap_terminal
 *   DESCRIPTION: Swaps the terminals that is being accessed.
 *   INPUTS: terminal - desired terminal to swap to
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void swap_terminal (int terminal) {
	pcb_t * pcb = get_pcb();

	// don't do unnecessary swaps
	if (terminal == current_terminal) {
		return;
	}
		
	// check terminal validity
	if (terminal > 2) {
		return;
	}

	// remap video page to itself
	page_on_4kb ((void*) (VIDEO), (void*) (map_loc));
	page_on_4kb ((void*) (VIDEO), (void*) (VIDEO));

	// save keyboard buffer
	key_buf_idxs[current_terminal] = key_buf_idx;
	memcpy(key_bufs[current_terminal], key_buf, BUF_LIMIT);

	// save old video memory to buffer
	memcpy((void*)(VIDEO + (current_terminal+1)*FOUR_KI_B), (void*) VIDEO, FOUR_KI_B);

	// load new memory to buffer
	memcpy((void*) VIDEO, (void*)(VIDEO + (terminal+1)*FOUR_KI_B), FOUR_KI_B);

	// restore keyboard buffer
	memcpy(key_buf, key_bufs[terminal], BUF_LIMIT);
	key_buf_idx = key_buf_idxs[terminal];

	// set video mapping
	if (pcb->terminal == current_terminal) {
		page_on_4kb ((void*) (VIDEO), (void*) (map_loc));
		page_on_4kb ((void*) (VIDEO), (void*) (VIDEO));
	}
	else {
		page_on_4kb ((void*) (vmem_buffers[pcb->terminal]), (void*) (map_loc));
		page_on_4kb ((void*) (vmem_buffers[pcb->terminal]), (void*) (VIDEO));
	}

	// switch terminal
	current_terminal = terminal;

	// If terminal has not been opened, reset terminal
	if (terminal_processes[terminal] < 0) {
		// store ESP and EBP
		asm volatile(
			"movl %%esp, %0       # store old stack pointer \n\
			movl %%ebp, %1		 # store old base pointer \n\
			"
			: "=m" (pcb->esp), "=m" (pcb->ebp)
		);

		swap_flag = 1;
		execute_syscall((uint8_t*) "shell");
	}
	
}

/*
 * halt_syscall
 *   DESCRIPTION: Restores state to before program was executed.
 *   INPUTS: status - value to return to parent program
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Jumps into execute
 */
int32_t halt_syscall (uint8_t status) {
	int i;
	pcb_t * pcb = get_pcb();
	pcb_t * parent = pcb->parent;
	
	// decrement tracker
	process_count--;
	
	// close files
	for (i = 0; i < FARRAY_SIZE; i++) {
		close_syscall(i);
	}


	if(parent != NULL)
	{
		// reset file array
		init_farray(pcb);

		// update process tracker
		terminal_processes[parent->terminal] = parent->pid;
		// restore parent paging to 128MB
		page_on_4mb ((void*) (FOUR_MI_B * 2 + (terminal_processes[parent->terminal] + 1) * FOUR_MI_B), (void*) (USER_VMEM));

		// set up TSS entry
		tss.esp0 = (FOUR_MI_B * 2) - ((FOUR_KI_B * 2) * (terminal_processes[parent->terminal])) - 4;
		parent->child = NULL;

		// Assembly for return to execute.
		// Restore execute's ESP and EBP from pcb and jump into execute
		asm volatile("movzx %0, %%eax	#zero-extend return value \n\
			movl %1, %%esp \n\
			movl %2, %%ebp \n\
			jmp exec_return \n\
			"
		: // No Output
		: 	"m" (status), "m" (pcb->parent_esp), "m" (pcb->parent_ebp)
		: "%eax"
		);

	}
	else // shell was halted
	{
		// reset file array
		init_farray(pcb);

		// update process tracker
		terminal_processes[pcb->terminal] = -3 + pcb->terminal;
		sti();
		// start a new shell
		execute_syscall((uint8_t*) "shell");
	}


	return -1;
}

/*
 * execute_syscall
 *   DESCRIPTION: Executes a program in the file system
 *   INPUTS: command - a space-divided string containing the name of program and arguments
 *   OUTPUTS: none
 *   RETURN VALUE: Return value of the executed process
 *   SIDE EFFECTS: Modifies stack and paging states, goes to user mode
 */
int32_t execute_syscall (const uint8_t* command){
	int i;
	int ret;
	dentry_t dentry;
	uint8_t buf[28]; // buffer of 28 characters of program data
	char _command[ARG_LIMIT];
	uint8_t args[ARG_LIMIT];
	pcb_t* new_pcb;
	void * prgm_eip;
	void * new_esp;
	void * new_ebp;
	int new_pid;

	if (command == NULL) {
		return -1;
	}

	// load program image into memory
	parseString((char*) command, _command, args);

	// Read file containing desired program
	ret  = read_dentry_by_name((uint8_t*) _command, &dentry);

	// If file does not exist
	if (ret == -1)
	{
		return -1;
	}

	// If file is not a "normal" file (type 2)
	if (dentry.file_type != 2)
	{
		return -1;
	}

	// read 28 bytes from file (EIP is at byte 24:27)
	ret = read_data(dentry.inode_num, 0, buf, 28);
	// file is smaller than executable
	if (ret != 28)
	{
		return -1;
	}

	// executable check magic numbers
	if(buf[0] != 0x7f || buf[1] != 0x45 || buf[2] != 0x4c || buf[3] != 0x46)
	{
		return -1;
	}

	// maintain 6 process limit, because we only want 4 max per terminal, checking if that terminals pid is greater than 8 will prevent 5th program from launching
	if (process_count >= 5 || terminal_processes[current_terminal] >= 8) {
		printf("Program limit reached. \n");
		return 0;
	}

	// increment current process
	new_pid = terminal_processes[current_terminal] + 3;

	// set up page table
	// remap 128 MB in virtual memory to new process, 8MB + process number * 4MB
	page_on_4mb ((void*) (FOUR_MI_B * 2 + (new_pid + 1) * FOUR_MI_B), (void*) (USER_VMEM));

	// attempt to program image to the newly mapped 128MB at offset x48000
	i = program_imgcpy((uint8_t*) _command, (void*) (FOUR_MI_B * 32 + 0x48000));

	// failed to copy program image
	if (i == -1) {
		// shell failed to execute we're doomed
		if (new_pid == -1) {
			return -1;
		}

		// remap 128 MB in virtual memory to parent memory, 8MB + process number * 4MB
		page_on_4mb ((void*) (FOUR_MI_B * 2 + (terminal_processes[current_terminal] + 1) * FOUR_MI_B), (void*) (USER_VMEM));
		return -1;
	}

	// allocate a pcb at 8MB - 8kb * (process number)
	new_pcb = (pcb_t*) ((FOUR_MI_B * 2) - ((FOUR_KI_B * 2) * (new_pid + 1)));
	
	// populate pcb with default values
	clean_pcb (new_pcb);

	// allocate pid
	new_pcb->pid = new_pid;
	new_pcb->child = NULL;

	// initialize file array
	init_farray(new_pcb);
	
	// copy args
	memcpy(new_pcb->args, args, ARG_LIMIT);

	// store ESP and EBP
	asm volatile(
	    "movl %%esp, %0       # store old stack pointer \n\
		movl %%ebp, %1		 # store old base pointer \n\
		"
	    : "=m" (new_pcb->parent_esp), "=m" (new_pcb->parent_ebp)
	);

	// update parent pcb and terminal value
	if (terminal_processes[current_terminal] >= 0) {
		new_pcb->parent = (pcb_t *) ((FOUR_MI_B * 2) - ((FOUR_KI_B * 2) * (terminal_processes[current_terminal] + 1)));
		new_pcb->parent->child = new_pcb;
		new_pcb->terminal = new_pcb->parent->terminal;
	} else {
		new_pcb->parent = NULL;
		new_pcb->terminal = current_terminal;
	}

	// update process tracker
	terminal_processes[new_pcb->terminal] = new_pid;
	process_count++;

	// set up TSS entry
	tss.ss0 = KERNEL_DS;
	tss.esp0 = (FOUR_MI_B * 2) - ((FOUR_KI_B * 2) * (terminal_processes[new_pcb->terminal])) - 4;

	// Create new stack pointer at bottom of block starting at 128MB ((4MB * 32) + 4MB - 4)
    new_esp = (void *) (USER_VMEM + FOUR_MI_B) - 4;
	new_ebp = (void *) (USER_VMEM + FOUR_MI_B) - 4;
	// Extract entry point from bytes [24:27] of program data
	prgm_eip = (void *) (((0xFF & buf[27]) << 24) | ((0xFF & buf[26]) << 16) | ((0xFF & buf[25]) << 8) | (buf[24] & 0xFF));

	new_pcb->esp = (uint32_t) new_esp;
	new_pcb->ebp = (uint32_t) new_ebp;

	// set video memory mapping
	if (new_pcb->terminal == current_terminal) {
		page_on_4kb ((void*) (VIDEO), (void*) (map_loc));
		page_on_4kb ((void*) (VIDEO), (void*) (VIDEO));
	}
	else {
		page_on_4kb ((void*) (vmem_buffers[new_pcb->terminal]), (void*) (map_loc));
		page_on_4kb ((void*) (vmem_buffers[new_pcb->terminal]), (void*) (VIDEO));
	}

	// restore keyboard activity
	if (swap_flag) {
		swap_flag = 0;
	    send_eoi(KEYBOARD_IRQ_NUM);
	}

	// push iret context and iret into process
	asm volatile(
		"cli \n\
		movw %0, %%ax			\n\
		movw %%ax, %%ds			\n\
		pushl %0		# push USER_DS		\n\
		pushl %1		# push new ESP \n\
		pushfl 			# push flags\n\
		pop %%eax		# pop flags \n\
		or $0x200, %%eax # reenable interrupts \n\
		pushl %%eax  	\n\
		pushl %3 		# push USER_CS \n\
		pushl %2		 # push new eip \n\
		iret \n\
		"
	 :  // No output
	 : 	"i" (USER_DS), "m" (new_esp), "m" (prgm_eip), "i" (USER_CS)
	 :  "eax"
	 );

	// upon returning from halt, return to syscall handler
    asm volatile(
		"exec_return: \n\
		movl %%eax, %0			\n\
		sti \n\
		"
	 : "=r" (ret)
	 : // No Input
	 );


	return ret;
}

/*
 * read_syscall
 *   DESCRIPTION: Read from a desired file
 *   INPUTS: fd - file descriptor number, nbytes- number of bytes to read
 *   OUTPUTS: buf - buffer to write file into
 *   RETURN VALUE: Return value of the read operation on the file, -1 on failure
 *   SIDE EFFECTS: Modifies file array
 */
int32_t read_syscall (int32_t fd, void* buf, int32_t nbytes){
	int ret;
	pcb_t* pcb = get_pcb();

	// if the argument location is not within the user memory
	// 128 MB to 132 MB (4MB * 32 to 4MB * 33)
	if ((uint32_t) buf < (USER_VMEM) || (uint32_t) buf >= (USER_VMEM + FOUR_MI_B)) {
		return -1;
	}

	// Validity of fd
	if (fd < 0 || fd >= FARRAY_SIZE) {
		return -1;
	}

	// fail if file is not open
	if (pcb->file_array[fd].flags == 0) {
		return -1;
	}

	// fail if no valid operation
	if (pcb->file_array[fd].operations_pointer == NULL) {
		return -1;
	}

	if (pcb->file_array[fd].operations_pointer->read_op == NULL) {
		return -1;
	}

	// call handler for file type
	ret = pcb->file_array[fd].operations_pointer->read_op(fd, buf, nbytes);
	return ret;
}

/*
 * write_syscall
 *   DESCRIPTION: Write to a desired file
 *   INPUTS: fd - file descriptor number
 *           buf - buffer containing data to write
 *           nbytes- number of bytes to write
 *   OUTPUTS: none
 *   RETURN VALUE: Return value of the write operation on the file, -1 on failure
 *   SIDE EFFECTS: Modifies file array and desired file
 */
int32_t write_syscall (int32_t fd, const void* buf, int32_t nbytes){
	int ret;
	pcb_t* pcb = get_pcb();

	// if the argument location is not within the user memory
	// 128 MB to 132 MB (4MB * 32 to 4MB * 33)
	if ((uint32_t) buf < (USER_VMEM) || (uint32_t) buf >= (USER_VMEM + FOUR_MI_B)) {
		return -1;
	}

	// Validity of fd
	if (fd < 0 || fd >= FARRAY_SIZE) {
		return -1;
	}

	// fail if file is not open
	if (pcb->file_array[fd].flags == 0) {
		return -1;
	}

	// fail if no valid operation
	if (pcb->file_array[fd].operations_pointer == NULL) {
		return -1;
	}

	if (pcb->file_array[fd].operations_pointer->write_op == NULL) {
		return -1;
	}

	// call handler for file type
	ret = pcb->file_array[fd].operations_pointer->write_op(fd, buf, nbytes);
	return ret;
}

/*
 * open_syscall
 *   DESCRIPTION: Open a desired file and
 *   INPUTS: filename - string containing the file name
 *   OUTPUTS: none
 *   RETURN VALUE: File descriptor for new file, -1 on failure
 *   SIDE EFFECTS: Modifies file array
 */
int32_t open_syscall (const uint8_t* filename){
	// all files have unknown type until opened and assigned a file descriptor in open
	// file_open works for every file type
	return file_open(filename);
}

/*
 * close_syscall
 *   DESCRIPTION: Closes a file
 *   INPUTS: fd - file descriptor of the file
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: Modifies file array
 */
int32_t close_syscall (int32_t fd){
	int ret;
	pcb_t* pcb = get_pcb();

	// Validity of fd. fd 0,1 are std io and should not be closed
	if (fd < 2 || fd >= FARRAY_SIZE) {
		return -1;
	}

	// fail if file is not open
	if (pcb->file_array[fd].flags == 0) {
		return -1;
	}

	// fail if no valid operation
	if (pcb->file_array[fd].operations_pointer == NULL) {
		return -1;
	}

	if (pcb->file_array[fd].operations_pointer->close_op == NULL) {
		return -1;
	}


	// call handler for file type
	ret = pcb->file_array[fd].operations_pointer->close_op(fd);

	// reset fields in file array
	pcb->file_array[fd].operations_pointer = NULL;
	pcb->file_array[fd].inode_num = 0;
	pcb->file_array[fd].file_pos = 0;
	pcb->file_array[fd].flags = 0;

	return ret;
}

/*
 * getargs_syscall
 *   DESCRIPTION: Copies arguments into a given buffer
 *   INPUTS: nbytes - number of bytes to copy
 *	 OUTPUTS: buf - Output buffer
 *   RETURN VALUE: -1 on failure, otherwise 0
 */
int32_t getargs_syscall (uint8_t* buf, int32_t nbytes){
	if(buf == NULL)
		return -1;

	// if the argument location is not within the user memory
	// 128 MB to 132 MB (4MB * 32 to 4MB * 33)
	if ((uint32_t) buf < (USER_VMEM) || (uint32_t) buf >= (USER_VMEM + FOUR_MI_B)) {
		return -1;
	}

	pcb_t* pcb = get_pcb();
	if(pcb->args[0] != '\0')
	{
		// write desired data from pcb
		memcpy(buf, pcb->args, (nbytes > ARG_LIMIT ? ARG_LIMIT : nbytes));
		return 0;
	}

	return -1;
}

/*
 * vidmap_syscall
 *   DESCRIPTION: Maps video memory to a virtual address and gives access to user
 *   INPUTS: none
 *	 OUTPUTS: screen_start - address to write mapped virtual address to
 *   RETURN VALUE: -1
 */
int32_t vidmap_syscall (uint8_t** screen_start){
	// virtual address 132 MB (4MB * 33)
	// pcb_t * pcb = get_pcb();

	if (screen_start == NULL) {
		return -1;
	}

	// if the argument location is not within the user memory
	// 128 MB to 132 MB (4MB * 32 to 4MB * 33)
	if ((uint32_t) screen_start < (USER_VMEM) || (uint32_t) screen_start >= (USER_VMEM + FOUR_MI_B)) {
		return -1;
	}

	// put address into arg
	*screen_start = map_loc;

	return 0;
}

/*
 * set_handler_syscall
 *   DESCRIPTION: Does nothing
 *   INPUTS: ignored
 *	 OUTPUTS: none
 *   RETURN VALUE: -1
 */
int32_t set_handler_syscall (int32_t signum, void* handler){
	return -1;
}

/*
 * sigreturn_syscall
 *   DESCRIPTION: Does nothing
 *   INPUTS: ignored
 *	 OUTPUTS: none
 *   RETURN VALUE: -1
 */
int32_t sigreturn_syscall (void){
	return -1;
}
