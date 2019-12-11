#ifndef _PCB_H
#define _PCB_H

#include "types.h"
#include "filesys.h"

#define ARG_LIMIT 128

typedef struct pcb_t {
	uint32_t pid; // process id
	struct pcb_t * parent; // parent process
	struct pcb_t * child; // child process
	uint32_t parent_esp;
	uint32_t parent_ebp;
	uint32_t esp; 	// process esp
	uint32_t ebp;	// process ebp
	farray_t file_array[8];
	uint8_t terminal; // terminal process is running on
	uint8_t freq; //For vitualized RTC
	uint8_t freq_wait; //For virtualized RTC, default to 0
	uint8_t args[ARG_LIMIT];	// process arguments
} pcb_t;

// returns pointer to PCB given ESP
extern pcb_t * get_pcb ();
extern void init_farray (pcb_t *);
extern void clean_pcb (pcb_t * ptr);

#endif
