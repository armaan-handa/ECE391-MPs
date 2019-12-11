#include "pcb.h"
#include "filesys.h"
#include "terminal.h"

// Operations table entries for stdio
static operations_t std_in_ops = {.open_op = open_terminal, .read_op = read_terminal, .write_op = NULL, .close_op = close_terminal };
static operations_t std_out_ops = {.open_op = open_terminal, .read_op = NULL, .write_op = write_terminal, .close_op = close_terminal };


/*
 *  get_pcb
 *	Returns a pointer to the PCB given the value of %esp
 *  Input: esp - value of ESP
 *  Output: Pointer to PCB struct
 */
pcb_t * get_pcb () {
	unsigned int esp;
	asm volatile(
		"movl %%esp, %0"
		 : "=r"(esp) 
		 
	);
		 
	return (pcb_t *) ((unsigned int) esp & 0xFFFFE000); // align address to 8kB
}

/*
 *  clean_pcb
 *	Initializes a clean pcb
 *  Input: pcb_t * ptr - a pointer to the pcb
 *  Output: none
 */
void clean_pcb (pcb_t * ptr) {
	ptr->pid = 0;
	ptr->parent = NULL;
	ptr->child = NULL;
	ptr->parent_esp = 0;
	ptr->parent_ebp = 0;
	ptr->esp = 0;
	ptr->ebp = 0;
	init_farray(ptr);
	ptr->terminal = -1;
	ptr->freq = 2; // default to 2
	ptr->freq_wait = 0;
	*(ptr->args) = '\0';
}

/*
 *  init_farray
 *	Initializes the PCB's file array
 *  Input: ptr - pointer to PCB to populate
 *  Output: None
 */
void init_farray (pcb_t * ptr) {
	int i;
	
	// initialize file array
	// initialize std_in
	ptr->file_array[0].operations_pointer = &std_in_ops;
	ptr->file_array[0].inode_num = 0;
	ptr->file_array[0].file_pos = 0;
	ptr->file_array[0].flags |= 0x1;
	
	// initialize std_out
	ptr->file_array[1].operations_pointer = &std_out_ops;
	ptr->file_array[1].inode_num = 0;
	ptr->file_array[1].file_pos = 0;
	ptr->file_array[1].flags |= 0x1;

	// initialize remaining entries to be empty
	for (i = 2; i < FARRAY_SIZE; i++) {
		ptr->file_array[i].operations_pointer = NULL;
		ptr->file_array[i].inode_num = 0;
		ptr->file_array[i].file_pos = 0;
		ptr->file_array[i].flags = 0;
	}
	
}
