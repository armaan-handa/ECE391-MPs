#include "paging.h"
#include "types.h"
#include "lib.h"
#include "syscall.h"

// Static arrays for use as page directory and first two pages
static uint32_t page_directory[PAGE_TABLE_SIZE] __attribute__((aligned(FOUR_KI_B))); // creation of a 1KiB directory alligned every 4KiB
static uint32_t active_page_table[PAGE_TABLE_SIZE] __attribute__((aligned(FOUR_KI_B))); // creation of a table of 4KiB pages
static uint32_t vidmap_table[PAGE_TABLE_SIZE] __attribute__((aligned(FOUR_KI_B)));;
/*
 * allow_paging
 *   DESCRIPTION: Sets the system to use enable paging with a given directory
 *   INPUTS: page_directory - address of the page directory
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Turns paging on for the system
 */
void allow_paging(unsigned int pd){
		if (pd == NULL) {
			printf("page_directory invalid\n");
			return;
		}

		 asm volatile(
			"movl %0, %%eax		# get page_directory		\n\
			movl %%eax, %%cr3	# move the directory to CR3	\n\
			movl %%cr4, %%eax \n\
			orl $0x00000010, %%eax #enable PSE \n\
			movl %%eax, %%cr4 \n\
			movl %%cr0, %%eax								\n\
			orl $0x80000001, %%eax	#done to set the paging allowed bit	\n\
			movl %%eax, %%cr0		#move into CR0 to permit paging		\n\
			"
		 :  // no output registers
		 : "m"(pd)
         : "%eax"//,"%cr3","%cr0"
		 );

}

/*
 * init_pages
 *   DESCRIPTION: Generates a page directory and the maps the first two pages to 0MB to 8MB
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Populates static arrays
 */
void init_pages(){
	 	//set all entrys to not present by default.
	 int directory;
 	 unsigned int page;

	 for(directory = 0; directory < PAGE_TABLE_SIZE; directory++)
	 {
		// fills the directory setting their write bit and making them by default not present
		// Turn on bit 1 (R/W) only
	 	page_directory[directory] = 0x00000002;
	 }
	 for(page = 0; page < PAGE_TABLE_SIZE; page++) {
		// populate the first page table
		// offset by 4KiB for each page
		// set pages as R/W and sets them to not present
	    active_page_table[page] = (page * FOUR_KI_B) | 2;
		vidmap_table[page] = 0x00000002;
	 }

	 // turn on pages for video memory
	 active_page_table[VIDEO_PAGE] = VIDEO | 3;
 	 
	 active_page_table[VIDEO_PAGE + 1] = (VIDEO + FOUR_KI_B) | 7;
	 vmem_buffers[0] = (uint8_t*) ((VIDEO + FOUR_KI_B));
	 
	 active_page_table[VIDEO_PAGE + 2] = (VIDEO + 2 * FOUR_KI_B) | 7;
 	 vmem_buffers[1] = (uint8_t*) ((VIDEO + 2 * FOUR_KI_B));
	 
	 active_page_table[VIDEO_PAGE + 3] = (VIDEO + 3 * FOUR_KI_B) | 7;
	 vmem_buffers[2] = (uint8_t*) ((VIDEO + 3 * FOUR_KI_B));


	 // link first two page tables to page directory
	 // set page tables as R/W and present by setting 2 LSB to 1
	 page_directory[0] = ((unsigned int)active_page_table) | 0x7;
	 // turn on 4MiB page in directory
	 page_directory[1] = ((unsigned int) FOUR_MI_B) | 0x83; // set to page size to 4MiB by turning on bit 7, 0b10000011 = 0x83

	 allow_paging((unsigned int) page_directory); // allows paging to happen
}

/*
 * page_on_4mb
 *   DESCRIPTION: Turns on a 4MiB page of memory at a given address
 *   INPUTS: phys_addr - physical memory address to map
 *			 virt_addr - virtual address to map to
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Modifies page directory
 */
void page_on_4mb (void * phys_addr, void * virt_addr) {
	// set to page size to 4MiB by turning on bit 7, 0b10000011 = 0x83
	page_directory[(unsigned int) virt_addr / FOUR_MI_B] = (unsigned int) phys_addr | 0x87;

	// flush tlb
	asm volatile(
		"movl %%cr3, %%eax \n\
		movl %%eax, %%cr3 \n\
		"
	 :  // no output registers
	 :  // no input registers
	 : "%eax" // clobbers eax
	 );
}

/*
 * page_off_4mb
 *   DESCRIPTION: Turns off a 4MiB page of memory at a given address
 *   INPUTS: virt_addr - virtual address to unmap
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Modifies page directory
 */
void page_off_4mb (void * virt_addr) {
	page_directory[(unsigned int) virt_addr / FOUR_MI_B] = 0x0;

	// flush tlb
	asm volatile(
		"movl %%cr3, %%eax \n\
		movl %%eax, %%cr3 \n\
		"
	 :  // no output registers
	 :  // no input registers
	 : "%eax" // clobbers eax
	 );
}

/*
 * page_on_4kb
 *   DESCRIPTION: Turns on a 4KiB page of memory at a given address
 *   INPUTS: phys_addr - physical memory address to map
 *			 virt_addr - virtual address to map to
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Modifies page directory
 */
void page_on_4kb (void * phys_addr, void * virt_addr) {
	int directory_index;
	int table_index;

	// get relevant bits of virtual address to index
	directory_index = (uint32_t) virt_addr / FOUR_MI_B;
	table_index = ((uint32_t) virt_addr / FOUR_KI_B) & 0x3FF; 

	// initialize the table
	if ((page_directory[directory_index] & 0xFFFFF000) == 0) { // check if upper bits of directory entry exist
		page_directory[directory_index] = (uint32_t) vidmap_table | 7; // 7 turns on user/present/RW bits
	}

    ((uint32_t *) (page_directory[directory_index] & 0xFFFFF000))[table_index] = ((uint32_t) (phys_addr) & 0xFFFFF000) | 7;

	// flush tlb
	asm volatile(
		"movl %%cr3, %%eax \n\
		movl %%eax, %%cr3 \n\
		"
	 :  // no output registers
	 :  // no input registers
	 : "%eax" // clobbers eax
	 );
}
