#include "rtc.h"
#include "types.h"
#include "i8259.h"
#include "x86_desc.h"
#include "lib.h"
#include "filesys.h"
#include "syscall.h"
#include "scheduling.h"
#include "pcb.h"

int rtc_flag = 0;
char prev;
int rct_intr_flag;

int executing_terminal = 0;

/* RTC Functions */

/**
  * init_rtc()
  * Initializes the RTC chip by writing appripriate bits to the RTC registers
  * and CMOS port, and also enables irq line 8, which is for RTC
  * Also defaults rate to 2Hz
  * Inputs: None
  * Outputs: None
 */
void
init_rtc() {
	//From OSDevWiki
	outb(RTC_PORTA, RTC_PORT);				// select Status Register A, and disable NMI (by setting the 0x80 bit)
	//outb(0x20, CMOS_PORT);
	outb(RTC_PORTB, RTC_PORT);				// select register B, and disable NMI
	prev=inb(CMOS_PORT);			// read the current value of register B
	outb(RTC_PORTB, RTC_PORT);				// set the index again (a read will reset the index to register D)
	outb((prev | 0x40), CMOS_PORT);		// write the previous value ORed with 0x40. This turns on bit 6 of register B
	enable_irq(RTC_IRQ_NUM);
	//Code for CP2: Initializes to 2Hz
	int initial_rate=0x6;							//1024Hz, From page 19 of manual
	outb(RTC_PORTA, RTC_PORT);				//Set index to register A, and also disable NMI
	prev=inb(CMOS_PORT);							// read the current value of register A
	outb(RTC_PORTA, RTC_PORT);						// reset index to A
	outb((prev & 0xF0) | initial_rate, CMOS_PORT);	//write only our rate to A. Note, rate is the bottom 4 bits.
}

/**
  * cp1_rtc_handler()
  * Temporary interrupt handler for the RTC for use in checkpoint 1. This handler
	* prepares RTC to be sent again by clearing out content of rtc register C
  * Inputs: None
  * Outputs: None
 */
void
cp1_rtc_handler() {
	cli();
	outb(RTC_PORTC, RTC_PORT);		// select register C
	inb(CMOS_PORT);				// just throw away contents
	if(rtc_flag == 1)
	{
		test_interrupts();
	}
	send_eoi(RTC_IRQ_NUM);
	sti();
}


/**
  * rtc_handler()
  * Temporary interrupt handler for the RTC for use in checkpoint 1. This handler
	* prepares RTC to be sent again by clearing out content of rtc register C
  * Inputs: None
  * Outputs: None
 */
void
rtc_handler() {
	int i;
	pcb_t* pcb;
	int pid;
	send_eoi(RTC_IRQ_NUM);
	outb(RTC_PORTC, RTC_PORT);		// select register C
	inb(CMOS_PORT);				// just throw away contents
	
	for (i = 0; i < 3; i++) { 			//Loop through all processes that arent shell, extract pid
		pid = terminal_processes[i];
		if (pid < 3) {					//If process is a shell ignore it 
			continue;
		}
		//Now get each PCB of the processes, and check its counter, if its not 0 then decrement it.
		pcb = (pcb_t*) ((FOUR_MI_B * 2) - ((FOUR_KI_B * 2) * (pid + 1)));
		if (pcb->freq_wait != 0) {
			pcb->freq_wait--;
		}
	}
}

/**
  * open_rtc()
  * Opens RTC and sets to 2Hz
  * Inputs: None
  * Outputs: Returns 0
 */
int32_t
open_rtc(const uint8_t* filename) {
	init_rtc();
	return 0;
}

/**
  * close_rtc()
  * THis does nothing unless we virtualize our RTC (Maybe later)
  * Inputs: None
  * Outputs: Returns 0 always
 */
int32_t
close_rtc(uint32_t fd) {
	return 0;
}

/**
  * write_rtc()
  * Writes a new frequency to the RTC passed through a buffer, returns a 0 on succes
  * -1 else. Insures frequencies passed are of  power of 2. If passed an arg thats not
  * a power of 2 then it will default to 1024Hz
  * Inputs: None
  * Outputs: None
 */
int32_t
write_rtc(uint32_t fd, const void* buf, uint32_t nbytes) {
	int arg;
	arg = *((int*)buf);
	//If the frequency it tries to write is larger than 1024Hz or less than 2Hz, return. Also check for bad input
	if ((int)buf == NULL || arg > 1024 || nbytes != 4 || arg < 2) { 
		return -1; //FAIL
	}
	pcb_t * pcb = get_pcb();
	//Checks if frequency is a power of two, if not return fail. 
	if (arg & (arg-1)) {
		return -1;
	}
	//Write the frequency to the processes pcb
	pcb->freq = arg;
	return nbytes;
}

/**
  * read_rtc()
  * Spins until next interrupt is detected, then returns 0.
  * Inputs: None
  * Outputs: Returns 0
 */
int32_t
read_rtc(uint32_t fd, void* buf, uint32_t nbytes) {
	pcb_t * pcb = get_pcb();
	//Calculate the amount of iterations that the process needs to wait to simulate the desired frequency. 
	pcb->freq_wait = 1024/(pcb->freq);
	while (pcb->freq_wait != 0) {
		// SPINNING
	}
	return 0;
}
